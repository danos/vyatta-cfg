/*
	Copyright (c) 2020 AT&T Intellectual Property.

	SPDX-License-Identifier: GPL-2.0-only
*/

#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C"
{
#include <netinet/ip.h>
#include <jansson.h>
#include <string.h>

#include <vyatta-util/map.h>
#include <vyatta-util/vector.h>

#include "connect.h"
#include "error.h"
#include "rpc.h"
#include "internal.h"
#include "common_mocks.h"
}

#define TEST_REQ_ID 123

#define TEST_STRING "some string"

#define TEST_ERROR "error occurred"

struct configd_conn test_conn;
struct response test_resp;
struct request test_req;
static struct configd_error test_err;

// Set of tests to work through recv_response logic for handling result (all
// different object types), error and mgmt_error return types.
//
// The subsequent set of tests verifies the extraction of mgmt_error data, so
// that is not checked here.
TEST_GROUP(RecvResponse)
{
	void setup()
	{
		memset(&test_conn, 0, sizeof(configd_conn));

		test_conn.req_id = TEST_REQ_ID;

		memset(&test_resp, 0, sizeof(response));
	}

	void teardown()
	{
		response_free(&test_resp);

		mock().checkExpectations();
		mock().clear();
	}; // Trailing ';' stops VS code misaligning code inside TEST_GROUP.
};

TEST(RecvResponse, result_true)
{
	set_incoming_rpc_json(json_pack(
		"{sbsns{s[]}si}",
		"result", 1,
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(INT, test_resp.type);
	LONGS_EQUAL(1, test_resp.result.int_val);
}

TEST(RecvResponse, result_false)
{
	set_incoming_rpc_json(json_pack(
		"{sbsns{s[]}si}",
		"result", 0,
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(INT, test_resp.type);
	LONGS_EQUAL(0, test_resp.result.int_val);
}

TEST(RecvResponse, result_int)
{
	set_incoming_rpc_json(json_pack(
		"{sisns{s[]}si}",
		"result", 66,
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(INT, test_resp.type);
	LONGS_EQUAL(66, test_resp.result.int_val);
}

TEST(RecvResponse, result_string)
{
	set_incoming_rpc_json(json_pack(
		"{sssns{s[]}si}",
		"result", TEST_STRING,
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(STRING, test_resp.type);
	STRCMP_EQUAL(TEST_STRING, test_resp.result.str_val);
}

TEST(RecvResponse, result_vector)
{
	const char *str = NULL;
	set_incoming_rpc_json(json_pack(
		"{s[sss]sns{s[]}si}",
		"result", "string1", "string2", "string3",
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(VECTOR, test_resp.type);
	LONGS_EQUAL(3, vector_count(test_resp.result.v))

	str = vector_next(test_resp.result.v, str);
	STRCMP_EQUAL("string1", str)

	str = vector_next(test_resp.result.v, str);
	STRCMP_EQUAL("string2", str)

	str = vector_next(test_resp.result.v, str);
	STRCMP_EQUAL("string3", str)
}

TEST(RecvResponse, result_map)
{
	set_incoming_rpc_json(json_pack(
		"{s{ssss}sns{s[]}si}",
		"result", "m1key", "m1entry", "m2key", "m2entry",
		"error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(MAP, test_resp.type);

	STRCMP_EQUAL("m1entry", map_get(test_resp.result.m, "m1key"));
	STRCMP_EQUAL("m2entry", map_get(test_resp.result.m, "m2key"));
}

TEST(RecvResponse, error_valid)
{
	set_incoming_rpc_json(json_pack(
		"{snsss{s[]}si}",
		"result",
		"error", "some error",
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(0, retval);

	LONGS_EQUAL(ERROR, test_resp.type);
	STRCMP_EQUAL("some error", test_resp.result.str_val);
}

TEST(RecvResponse, error_invalid)
{
	set_incoming_rpc_json(json_pack(
		"{snsis{s[]}si}",
		"result",
		"error", 42,
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID));

	mock().expectOneCall("msg_err").withParameter(
		"fmt", "configd error response must be a string\n");

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(-1, retval);

	LONGS_EQUAL(ERROR, test_resp.type);
	STRCMP_EQUAL(NULL, test_resp.result.str_val);
}

TEST(RecvResponse, id_invalid)
{
	// Arbitrary choice of error vs result here.  'id' is the value that
	// matters.
	set_incoming_rpc_json(json_pack(
		"{snsss{s[]}ss}",
		"result",
		"error", TEST_ERROR,
		"mgmterrorlist", "error-list",
		"id", "should be integer"));

	mock().expectOneCall("msg_err").withParameter(
		"fmt", "configd response id must be an integer\n");

	int retval = recv_response(&test_conn, &test_resp);

	LONGS_EQUAL(-1, retval);

	LONGS_EQUAL(ERROR, test_resp.type);
	STRCMP_EQUAL(TEST_ERROR, test_resp.result.str_val);
}

// The <Error> group tests handling of parsing / extracting a MgmtError from
// the JSON returned from configd.  Ideally we'd test with direct calls into
// the code in error.c but this would mean a lot of refactoring / awkward code
// to construct a map of the (already parsed) incoming JSON. It is easier
// to call down through the get_int/string/vector/map functions instead and
// mock up the send_request() operation.
TEST_GROUP(Error)
{
	void setup()
	{
		memset(&test_conn, 0, sizeof(configd_conn));
		test_conn.req_id = TEST_REQ_ID;

		memset(&test_req, 0, sizeof(request));
		test_req.fn = "some_method";
		test_req.args = json_pack("{ss}", "key", "value");

		memset(&test_err, 0, sizeof(struct configd_error));
	}

	void teardown()
	{
		configd_error_free(&test_err);

		mock().checkExpectations();
		mock().clear();
	}; // Trailing ';' stops VS code misaligning code inside TEST_GROUP.
};

// JSON dump of a 'real' mgmt-error, albeit with a second error-info element
// added on the sending side to show how multiple elements get packed, and the
// start of a second error in the list to show how the JSON elements fit
// together.
// {
//    "result" : null,
//    "id" : 5,
//    "mgmterror" : {
//       "error-list" : [
//          {
//             "error-tag" : "unknown-element",
//             "error-type" : "application",
//             "error-path" : "/system",
//             "error-info" : [
//                {
//                   "bad-element" : "logan"
//                },
//                {
//                   "bad-attribute" : "just testing"
//                }
//             ],
//             "error-severity" : "error",
//             "error-message" : "An unexpected element is present."
//          },
//          {
//             "error-tag": ...
//          }
//       ]
//    },
//    "error" : null
// }

TEST(Error, create_mgmt_error_all_fields_except_info)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-app-tag", "some-app-tag",
		"error-message", "some message or other",
		"error-path", "some-path",
		"error-severity", "some-severity",
		"error-tag", "some-tag",
		"error-type", "err-type",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	LONGS_EQUAL(1, configd_error_num_mgmt_errors(&test_err));

	struct configd_mgmt_error **err_list =
		configd_error_mgmt_error_list(&test_err);

	struct configd_mgmt_error *me = err_list[0];

	// Check error
	STRCMP_EQUAL("some-app-tag", configd_mgmt_error_app_tag(me));
	STRCMP_EQUAL("some message or other", configd_mgmt_error_message(me));
	STRCMP_EQUAL("some-path", configd_mgmt_error_path(me));
	STRCMP_EQUAL("some-severity", configd_mgmt_error_severity(me));
	STRCMP_EQUAL("some-tag", configd_mgmt_error_tag(me));
	STRCMP_EQUAL("err-type", configd_mgmt_error_type(me));

	// Make sure we also populate the basic error 'text' field for any apps
	// that can't or just don't make use of the MgmtError info.
	STRCMP_EQUAL("some message or other", test_err.text);
}

TEST(Error, create_mgmt_error_only_some_fields)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-message", "some message or other",
		"error-severity", "some-severity",
		"error-tag", "some-tag",
		"error-type", "err-type",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	LONGS_EQUAL(1, configd_error_num_mgmt_errors(&test_err));

	struct configd_mgmt_error **err_list =
		configd_error_mgmt_error_list(&test_err);

	struct configd_mgmt_error *me = err_list[0];

	// Check error
	STRCMP_EQUAL(NULL, configd_mgmt_error_app_tag(me));
	STRCMP_EQUAL("some message or other", configd_mgmt_error_message(me));
	STRCMP_EQUAL(NULL, configd_mgmt_error_path(me));
	STRCMP_EQUAL("some-severity", configd_mgmt_error_severity(me));
	STRCMP_EQUAL("some-tag", configd_mgmt_error_tag(me));
	STRCMP_EQUAL("err-type", configd_mgmt_error_type(me));

	// Make sure we also populate the basic error 'text' field for any apps
	// that can't or just don't make use of the MgmtError info.
	STRCMP_EQUAL("some message or other", test_err.text);
}

TEST(Error, create_mgmt_error_extract_info)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{sssssss[{ss}{ss}]}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-severity", "some-severity",
		"error-tag", "some-tag",
		"error-type", "err-type",
		"error-info",
		"bad-attribute", "illegal attribute found",
		"bad-element", "non-existent-element present",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	LONGS_EQUAL(1, configd_error_num_mgmt_errors(&test_err));

	struct configd_mgmt_error **err_list =
		configd_error_mgmt_error_list(&test_err);

	struct configd_mgmt_error *me = err_list[0];
	struct map *info = me->info;
	const char *value = NULL;

	value = map_get(info, "bad-attribute");
	STRCMP_EQUAL("illegal attribute found", value);

	value = map_get(info, "bad-element");
	STRCMP_EQUAL("non-existent-element present", value);
}

TEST(Error, create_mgmt_error_extract_multiple_errors)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssss}{ssssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-severity", "some-severity0",
		"error-tag", "some-tag0",
		"error-type", "err-type0",
		"error-message", "some message or other0",
		"error-severity", "some-severity1",
		"error-tag", "some-tag1",
		"error-type", "err-type1",
		"error-message", "some message or other1",
		"error-path", "some-path1",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	LONGS_EQUAL(2, configd_error_num_mgmt_errors(&test_err));

	struct configd_mgmt_error **err_list =
		configd_error_mgmt_error_list(&test_err);

	// Check error 0
	struct configd_mgmt_error *me0 = err_list[0];
	STRCMP_EQUAL(NULL, configd_mgmt_error_app_tag(me0));
	STRCMP_EQUAL("some message or other0", configd_mgmt_error_message(me0));
	STRCMP_EQUAL(NULL, configd_mgmt_error_path(me0));
	STRCMP_EQUAL("some-severity0", configd_mgmt_error_severity(me0));
	STRCMP_EQUAL("some-tag0", configd_mgmt_error_tag(me0));
	STRCMP_EQUAL("err-type0", configd_mgmt_error_type(me0));

	// Check error 1
	struct configd_mgmt_error *me1 = err_list[1];
	STRCMP_EQUAL(NULL, configd_mgmt_error_app_tag(me1));
	STRCMP_EQUAL("some message or other1", configd_mgmt_error_message(me1));
	STRCMP_EQUAL("some-path1", configd_mgmt_error_path(me1));
	STRCMP_EQUAL("some-severity1", configd_mgmt_error_severity(me1));
	STRCMP_EQUAL("some-tag1", configd_mgmt_error_tag(me1));
	STRCMP_EQUAL("err-type1", configd_mgmt_error_type(me1));

	// Make sure we also populate the basic error 'text' field for any apps
	// that can't or just don't make use of the MgmtError info.
	STRCMP_EQUAL(
		"some message or other0\nsome message or other1\n", test_err.text);
}

// Tricky to test all failure scenarios, but checking that the basic free()
// logic works when we are extracting the second (or subsequent) error is
// a reasonable test.  If this correctly frees the previously extracted error
// then the logic should be fine.
TEST(Error, extract_mgmt_error_list_2nd_error_not_map)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssss}[ss]]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-severity", "some-severity0",
		"error-tag", "some-tag0",
		"error-type", "err-type0",
		"error-message", "some message or other0",
		"unexpected", "more-unexpected",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	mock().expectOneCall("msg_err").withParameter(
		"fmt", "Unexpected JSON data type for configd mgmt-error\n");
	mock().expectOneCall("msg_err").withParameter(
		"fmt", "Unable to extract mgmt error.");

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	STRCMP_EQUAL("Error receiving response", test_err.text);
}

TEST(Error, handle_rpc_mgmt_error_fail)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-app-tag", "some-app-tag",
		"error-message", "some message or other",
		"error-path", "some-path",
		"error-severity", "some-severity",
		"error-tag", "some-tag",
		"error-type", "err-type",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, NULL));
}
