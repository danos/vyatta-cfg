/*
 * Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
 *
 * Copyright (c) 2014 by Brocade Communications Systems, Inc.
 * All rights reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-only
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>
#include <libxml/tree.h>
#include <vyatta-cfg/client/connect.h>
#include <vyatta-cfg/client/node.h>
#include <vyatta-cfg/client/rpc_types.h>

static int process_object(xmlNodePtr *out, json_t *obj);
static int process_array(xmlNodePtr out, json_t *arr);

static int process_array(xmlNodePtr out, json_t *arr)
{
	for (size_t i = 0; i < json_array_size(arr); i++) {
		json_t *v = json_array_get(arr, i);
		xmlNodePtr ch;
		if (process_object(&ch, v) != 0) {
			return 1;
		}
		xmlAddChild(out, ch);
	}
	return 0;
}

static int process_object(xmlNodePtr *out, json_t *obj) 
{
        const char *name = json_string_value(json_object_get(obj, "name"));
	if (name == NULL) {
		return -1;
	}

        (*out) = xmlNewNode(NULL, name);

        json_t *children = json_object_get(obj, "children");
        if (children == NULL) {
		const char *val = json_string_value(json_object_get(obj, "value"));
		if (val != NULL) {
                	xmlNodeAddContent((*out), val);
		}
        } else {
		if (process_array((*out), children) != 0) {
			fprintf(stderr, "failed to process children of: %s\n", name);
			return -1;
		}
        }

        return 0;
}

static int tree_to_xml(char **out, char *tree)
{
        json_t *obj = NULL;
	xmlBufferPtr resultbuffer = NULL;
        xmlNodePtr xt, aux_node;
	int ret = 0;

        if (tree == NULL) {
                return -1;
        }
	resultbuffer = xmlBufferCreate();
	if (resultbuffer == NULL) {
		return -1;
	}

	xmlDocPtr doc = xmlNewDoc("1.0");

	obj = json_loads(tree, 0, NULL);

	/*Process json into xml*/
        if (process_object(&xt, obj) != 0) {
		ret = -1;
		goto done;
	}
	xmlDocSetRootElement(doc, xt);

	/* Format for netconf (strip root) */
        for (aux_node = xt->children; aux_node != NULL; aux_node = aux_node->next) {
                xmlNodeDump(resultbuffer, doc, aux_node, 4, 1);
        }

	/* get the buffer out of libxml2 land */
	(*out) = strdup((const char *) xmlBufferContent(resultbuffer));

done:
	xmlBufferFree(resultbuffer);
	xmlFreeDoc(doc);
	json_decref(obj);

        return ret;
}

int main(int argc, char **argv) {
	char *data = NULL, *tree = NULL, *sid = NULL;
	struct connection conn = {0};


	if (open_connection(&conn) != 0) {
		fprintf(stderr, "failed to open connection to configd\n");
		return 1;
	}

	sid = getenv("VYATTA_CONFIG_SID");
	if (sid != NULL) {
		set_session_id(&conn, sid);
	}

	tree = tree_get(&conn, SESSION, "", NULL);
	if (tree == NULL) {
		fprintf(stderr, "failed to get tree\n");
		return 1;
	}
	if (tree_to_xml(&data, tree) != 0) {
		fprintf(stderr, "failed to convert tree\n");
		return 1;
	}

	printf("%s\n", data);
	close_connection(&conn);
	free(tree);
	free(data);
	return 0;
}
