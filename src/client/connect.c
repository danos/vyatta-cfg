/*
   Copyright (c) 2018-2019, AT&T Intellectual Property. All rights reserved.
   Copyright (c) 2013-2014 by Brocade Communications Systems, Inc.
   All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
*/

#include <argz.h>
#include <envz.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "rpc.h"
#include "connect.h"
#include "log.h"
#include "error.h"
#include "internal.h"

#define DEFAULT_CONFIG_SOCKET "/var/run/vyatta/configd/main.sock"

#define DEBUG 1
#undef DEBUG

#ifdef DEBUG
static void msg_json(const json_t *jobj, const char *pfx)
{
	char *jstr = json_dumps(jobj, 0);
	if (jstr) {
		msg_out("%s: %s\n", pfx, jstr);
		free(jstr);
	}
}
#else
#define msg_json(...)
#endif


void response_free(struct response *resp)
{
	if ((resp->type != INT) && (resp->type != STATUS))
		free(resp->result.str_val);
}


/* There are no communication timeouts as some operations
 * (e.g., commit) can take an unbounded amount of time.
 */
int configd_open_connection(struct configd_conn *conn)
{
	int flags;
	socklen_t len;
	int local_errno;
	struct sockaddr_un configd = { .sun_family = AF_UNIX };

	char *sock_addr = getenv("VYATTA_CONFIG_SOCKET");
	if (sock_addr == NULL) {
		sock_addr = DEFAULT_CONFIG_SOCKET;
	}
	snprintf(configd.sun_path, sizeof(configd.sun_path), "%s", sock_addr);

	if (!conn) {
		errno = EFAULT;
		return -1;
	}

	memset(conn, 0, sizeof(*conn));
	conn->fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (conn->fd == -1)
		return -1;
	if ((flags = fcntl(conn->fd, F_GETFD)) != -1)
		fcntl(conn->fd, F_SETFD, flags | FD_CLOEXEC);

	len = SUN_LEN(&configd);
	if (connect(conn->fd, (struct sockaddr *)&configd, len) == -1) {
		local_errno = errno;
		goto error;
	}

	conn->fp = fdopen(conn->fd, "r+");
	if (!conn->fp) {
		local_errno = errno;
		msg_err("Unable to open socket stream to configd\n");
		goto error;
	}
	conn->session_id = strdup("");
	return 0;

error:
	configd_close_connection(conn);
	errno = local_errno;
	return -1;
}

void configd_close_connection(struct configd_conn *conn)
{
	if (conn->fp)
		fclose(conn->fp);
	if (conn->fd != -1)
		close(conn->fd);
	free(conn->session_id);
}


int configd_set_session_id(struct configd_conn *conn, const char *session_id)
{
	if (!conn || !session_id) {
		errno = EFAULT;
		return -1;
	}

	if (conn->session_id)
		free(conn->session_id);

	conn->session_id = strdup(session_id);
	return 0;
}


int send_request(struct configd_conn *conn, const struct request *req)
{
	json_t *jreq;
	json_error_t jerr;
	int result = -1;
	char *jstr;

	if (!conn || !req || !req->args) {
		errno = EFAULT;
		return -1;
	}

	/* Consumes ref to req->args on success */
	jreq = json_pack_ex(&jerr, 0, "{s:s, s:o, s:i}",
			    "method", req->fn,
			    "params", req->args,
			    "id", ++(conn->req_id));
	if (!jreq || json_is_null(jreq)) {
		msg_err("Unable to pack configd request: %s\n", jerr.text);
		return -1;
	}

	msg_json(jreq, __func__); /* debugging */
	jstr = json_dumps(jreq, JSON_COMPACT);
	if (jstr) {
		result = write(conn->fd, jstr, strlen(jstr));
		free(jstr);
	}

	json_decref(jreq);
	return result;
}

static struct vector *jarray_to_vector(json_t *jobj)
{
	int local_errno = 0;
	struct vector *v = NULL;
	size_t arr_len, i;
	size_t argz_len = 0;
	char *argz = NULL;

	arr_len = json_array_size(jobj);
	if (argz_create_sep("", 0, &argz, &argz_len)) {
		errno = ENOMEM;
		return NULL;
	}

	for (i = 0; i < arr_len; ++i) {
		json_t *jval;
		const char *vstr;

		jval = json_array_get(jobj, i);
		vstr = json_string_value(jval);
		if (!vstr) {
			local_errno = EINVAL;
			goto error;
		}

		if (argz_add(&argz, &argz_len, vstr)) {
			local_errno = ENOMEM;
			goto error;
		}
	}

	v = vector_new(argz, argz_len);
	if (!v) {
		local_errno = ENOMEM;
		goto error;
	}
	goto done;

error:
	free(argz);
done:
	errno = local_errno;
	return v;
}

static struct map *jobj_to_map(json_t *jobj)
{
	int local_errno = 0;
	struct map *m = NULL;
	size_t argz_len = 0;
	char *argz = NULL;

	const char *key;
	json_t *jval;

	if (argz_create_sep("", 0, &argz, &argz_len)) {
		errno = ENOMEM;
		return NULL;
	}

	json_object_foreach(jobj, key, jval) {
		const char *vstr;

		vstr = json_string_value(jval);
		if (!vstr) {
			local_errno = EINVAL;
			goto error;
		}

		if (envz_add(&argz, &argz_len, key, vstr)) {
			local_errno = errno;
			goto error;
		}
	}

	m = map_new(argz, argz_len);
	if (!m) {
		local_errno = ENOMEM;
		goto error;
	}
	goto done;

error:
	free(argz);
done:
	errno = local_errno;
	return m;
}

int recv_response(struct configd_conn *conn, struct response *resp)
{
	json_t *jresp = NULL;
	json_t *jresult;
	json_t *jobj;
	int ret = -1;
	json_error_t jerr;

	if (!conn || !resp) {
		errno = EFAULT;
		return -1;
	}

	memset(&resp->result, 0, sizeof(resp->result));
	while (jresp == NULL && !feof(conn->fp))
		jresp = json_loadf(conn->fp, JSON_DISABLE_EOF_CHECK, &jerr);

	if (!jresp || json_is_null(jresp)) {
		msg_err("%s: %s\n", __func__, jerr.text);
		return -1;
	}

	msg_json(jresp, __func__); /* debugging */

	/* The result object may be null. If so, it is an error result. */
	jresult = json_object_get(jresp, "result");
	if (!json_is_null(jresult)) {
		int resp_type = json_typeof(jresult);
		switch (resp_type) {
			case JSON_TRUE:
				resp->type = INT;
				resp->result.int_val = 1;
				break;
			case JSON_FALSE:
				resp->type = INT;
				resp->result.int_val = 0;
				break;
			case JSON_INTEGER:
				resp->type = INT;
				resp->result.int_val = json_integer_value(jresult);
				break;
			case JSON_STRING:
				resp->type = STRING;
				resp->result.str_val = strdup(json_string_value(jresult));
				break;
			case JSON_ARRAY:
				resp->type = VECTOR;
				resp->result.v = jarray_to_vector(jresult);
				if (!resp->result.v)
					goto done;
				break;
			case JSON_OBJECT:
				resp->type = MAP;
				resp->result.m = jobj_to_map(jresult);
				if (!resp->result.m)
					goto done;
				break;
			default:
				msg_err("Unhandled configd result type %d\n", resp_type);
				break;
		}
	} else {
		resp->type = ERROR;
		jobj = json_object_get(jresp, "error");
		if (!json_is_string(jobj)) {
			msg_err("configd error response must be a string\n");
			goto done;
		}
		resp->result.str_val = strdup(json_string_value(jobj));
	}

	jobj = json_object_get(jresp, "id");
	if (!json_is_integer(jobj)) {
		msg_err("configd response id must be an integer\n");
		goto done;
	}
	resp->id = json_integer_value(jobj);
	ret = (conn->req_id == resp->id) ? 0 : -1;
done:
	json_decref(jresp);
	return ret;
}

int get_int(struct configd_conn *conn, struct request *req, struct configd_error *error)
{
	struct response resp;

	if (!conn || !req) {
		errno = EFAULT;
		return -1;
	}

	if (send_request(conn, req) == -1) {
		if (!error)
			msg_err("Error sending configd int request\n");
		error_setf(error, "Error sending request");
		return -1;
	}

	if (recv_response(conn, &resp) == -1) {
		if (!error)
			msg_err("Error receiving configd int response\n");
		error_setf(error, "Error receiving response");
		goto error;
	}

	switch (resp.type) {
	case INT:
		return resp.result.int_val;
	case ERROR:
		if (error)
			error_setf(error, "%s\n", resp.result.str_val);
		break;
	default:
		break;

	}
error:
	response_free(&resp);
	return -1;
}

char *get_str(struct configd_conn *conn, struct request *req, struct configd_error *error)
{
	struct response resp;

	if (!conn || !req) {
		errno = EFAULT;
		return NULL;
	}

	if (send_request(conn, req) == -1) {
		if (!error)
			msg_err("Error sending configd string request\n");
		error_setf(error, "Error sending request");
		return NULL;
	}

	if (recv_response(conn, &resp) == -1) {
		if (!error)
			msg_err("Error receiving configd string response\n");
		error_setf(error, "Error receiving response");
		goto error;
	}

	switch (resp.type) {
	case STRING:
		return resp.result.str_val;
	case ERROR:
		if (!error)
			msg_err("%s\n", resp.result.str_val);
		error_setf(error, "%s\n", resp.result.str_val);
		break;
	default:
		break;

	}
error:
	response_free(&resp);
	return NULL;
}

struct vector *get_vector(struct configd_conn *conn, struct request *req, struct configd_error *error)
{
	struct response resp;

	if (!conn || !req) {
		errno = EFAULT;
		return NULL;
	}

	if (send_request(conn, req) == -1) {
		if (!error)
			msg_err("Error sending configd vector request\n");
		error_setf(error, "Error sending request");
		return NULL;
	}

	if (recv_response(conn, &resp) == -1) {
		if (!error)
			msg_err("Error receiving configd vector response\n");
		error_setf(error, "Error receiving response");
		goto error;
	}

	switch (resp.type) {
	case VECTOR:
		return resp.result.v;
	case ERROR:
		if (!error)
			msg_err("%s\n", resp.result.str_val);
		error_setf(error, "%s\n", resp.result.str_val);
		break;
	default:
		break;

	}
error:
	response_free(&resp);
	return NULL;
}

struct map *get_map(struct configd_conn *conn, struct request *req, struct configd_error *error)
{
	struct response resp;

	if (!conn || !req) {
		errno = EFAULT;
		return NULL;
	}

	if (send_request(conn, req) == -1) {
		if (!error)
			msg_err("Error sending configd map request\n");
		error_setf(error, "Error sending request");
		return NULL;
	}

	if (recv_response(conn, &resp) == -1) {
		if (!error)
			msg_err("Error receiving configd map response\n");
		error_setf(error, "Error receiving response");
		goto error;
	}

	switch (resp.type) {
	case MAP:
		return resp.result.m;
	case ERROR:
		if (!error)
			msg_err("%s\n", resp.result.str_val);
		error_setf(error, "%s\n", resp.result.str_val);
		break;
	default:
		break;

	}
error:
	response_free(&resp);
	return NULL;
}
