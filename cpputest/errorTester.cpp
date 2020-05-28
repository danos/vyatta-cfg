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
#include "transaction.h"
#include "internal.h"
#include "common_mocks.h"
}

#define PATH_FIELD "error-path"
#define MESSAGE_FIELD "error-message"
#define APP_TAG_FIELD "error-app-tag"

#define EMPTY_MESSAGE ""
#define EMPTY_PATH ""

#define TEST_MESSAGE "a message"
#define TEST_MESSAGE_WITH_NL "a message\n"
#define TEST_PATH "path/to/node"
#define TEST_PATH_SLASH_PREFIX "/path/to/node"
#define TEST_PATH_SLASH_SUFFIX "path/to/node/"
#define TEST_SPACE_PATH "path to node"
#define TEST_APP_TAG "an-app-tag-value"

#define TEST_SINGLE_ELEM_PATH "path"

#define TEST_MESSAGE_2 "another message"
#define TEST_PATH_2 "path/to/another/node"
#define TEST_SPACE_PATH_2 "path to another node"

#define TEST_OP "TestOperation"

#define TEST_REQ_ID 222

#define VALIDATE "Validate"
#define COMMIT "Commit"
#define CONFIRMED_COMMIT "ConfirmedCommit"
#define COMMENT "Commit comment"

struct configd_conn test_conn;
struct response test_resp;
struct request test_req;
static struct configd_error test_err;

// The <Format> group tests formatting of mgmt errors for CLI output.
// We populate the error via a config_error created in turn from incoming
// JSON as this (a) tests this code, (b) creates the error in the same way as
// production code and (c) allocates memory the same way as production code so
// we can use the error free() function and have cpputest check for memory
// leaks.  If we alloc in test code and free in production (or vice versa) we'd
// have to turn off this detection, which is a bad thing (TM).
TEST_GROUP(Format)
{
	void setup()
	{
		memset(&test_conn, 0, sizeof(configd_conn));
		test_conn.req_id = TEST_REQ_ID;
		test_conn.session_id = strdup("TEST_SESSION_ID");

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

// 2 field names, plus content, need to be provided.  These can be
// <error-message>, <error-path> and <error-app-tag>.  This allows for
// one or both of the first 2 to be specified.
void populate_single_test_error(
	const char *field1,
	const char *value1,
	const char *field2,
	const char *value2)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-type", "err-type",
		"error-severity", "some-severity",
		"error-tag", "some-tag",
		field1, value1,
		field2, value2,
		"id", TEST_REQ_ID + 1)); // send_request() increments ID
}

// Simulate a configd method call and check returned 'text' field in error is
// formatted for the CLI as expected.  Provided 'operation' is essentially
// arbitrary, but tests are spread across the 3 different options to ensure
// they are all exercised.
void test_single_error_format(
	const char *operation,
	const char *field1,
	const char *value1,
	const char *field2,
	const char *value2,
	const char *exp_path,
	const char *exp_msg)
{
	populate_single_test_error(field1, value1, field2, value2);

	if (!strcmp(VALIDATE, operation))
	{
		POINTERS_EQUAL(NULL, configd_validate(&test_conn, &test_err));
	}
	else if (!strcmp(COMMIT, operation))
	{
		POINTERS_EQUAL(NULL, configd_commit(&test_conn, COMMENT, &test_err));
	}
	else if (!strcmp(CONFIRMED_COMMIT, operation))
	{
		POINTERS_EQUAL(
			NULL,
			configd_confirmed_commit(
				&test_conn, COMMENT,
				1, // confirmed
				"TIMEOUT",
				"PERSIST",
				"PERSIST_ID",
				&test_err));
	}

	char *exp_output;
	int retval = asprintf(
		&exp_output, "[%s]\n\n%s\n\n[[%s]] failed.\n\n%s failed!\n",
		exp_path, exp_msg, exp_path, operation);
	CHECK_FALSE(retval == -1);

	STRCMP_EQUAL(exp_output, test_err.text);
}

void populate_multiple_test_errors(
	const char *path1, const char *message1,
	const char *path2, const char *message2)
{
	set_incoming_rpc_json(json_pack(
		"{snsns{s[{ssssssssss}{ssssssssss}]}si}",
		"result",
		"error",
		"mgmterrorlist", "error-list",
		"error-severity", "some-severity0",
		"error-tag", "some-tag0",
		"error-type", "err-type0",
		"error-message", message1,
		"error-path", path1,
		"error-severity", "some-severity1",
		"error-tag", "some-tag1",
		"error-type", "err-type1",
		"error-message", message2,
		"error-path", path2,
		"id", TEST_REQ_ID + 1)); // send_request() increments ID
}

// Really just proves called function doesn't crash with null error.
TEST(Format, cfgd_error_format_null_error)
{
	test_err.text = strdup(TEST_MESSAGE);
	configd_error_format_for_commit_or_val(NULL, TEST_OP);

	STRCMP_EQUAL(TEST_MESSAGE, test_err.text);

	free(test_err.text);
	test_err.text = NULL;
}

TEST(Format, cfgd_error_format_not_mgmt_error)
{
	set_incoming_rpc_json(json_pack(
		"{snsss{s[]}si}",
		"result",
		"error", TEST_MESSAGE,
		"mgmterrorlist", "error-list",
		"id", TEST_REQ_ID + 1)); // send_request() increments ID

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));
	configd_error_format_for_commit_or_val(&test_err, TEST_OP);

	STRCMP_EQUAL(TEST_MESSAGE_WITH_NL, test_err.text);
}

TEST(Format, cfgd_error_format_single_error)
{
	test_single_error_format(
		VALIDATE,
		PATH_FIELD, TEST_PATH,
		MESSAGE_FIELD, TEST_MESSAGE,
		TEST_SPACE_PATH, TEST_MESSAGE);
}

TEST(Format, cfgd_error_format_no_path)
{
	test_single_error_format(
		VALIDATE,
		APP_TAG_FIELD, TEST_APP_TAG,
		MESSAGE_FIELD, TEST_MESSAGE,
		EMPTY_PATH, TEST_MESSAGE);
}

TEST(Format, cfgd_error_format_empty_path)
{
	test_single_error_format(
		COMMIT,
		PATH_FIELD, EMPTY_PATH,
		MESSAGE_FIELD, TEST_MESSAGE,
		EMPTY_PATH, TEST_MESSAGE);
}

TEST(Format, cfgd_error_format_slash_prefixed_path)
{
	test_single_error_format(
		CONFIRMED_COMMIT,
		PATH_FIELD, TEST_PATH_SLASH_PREFIX,
		MESSAGE_FIELD, TEST_MESSAGE,
		TEST_SPACE_PATH, TEST_MESSAGE);
}

TEST(Format, cfgd_error_format_slash_suffixed_path)
{
	test_single_error_format(
		VALIDATE,
		PATH_FIELD, TEST_PATH_SLASH_SUFFIX,
		MESSAGE_FIELD, TEST_MESSAGE,
		TEST_SPACE_PATH, TEST_MESSAGE);
}

TEST(Format, cfgd_error_format_single_element_path)
{
	test_single_error_format(
		COMMIT,
		PATH_FIELD, TEST_SINGLE_ELEM_PATH,
		APP_TAG_FIELD, TEST_APP_TAG,
		TEST_SINGLE_ELEM_PATH, EMPTY_MESSAGE);
}

TEST(Format, cfgd_error_format_no_message)
{
	test_single_error_format(
		CONFIRMED_COMMIT,
		PATH_FIELD, TEST_PATH,
		APP_TAG_FIELD, TEST_APP_TAG,
		TEST_SPACE_PATH, EMPTY_MESSAGE);
}

TEST(Format, cfgd_error_format_empty_message)
{
	test_single_error_format(
		VALIDATE,
		PATH_FIELD, TEST_PATH,
		MESSAGE_FIELD, EMPTY_MESSAGE,
		TEST_SPACE_PATH, EMPTY_MESSAGE);
}

TEST(Format, cfgd_error_format_multiple_errors)
{
	// Interestingly, these are unpacked in reverse order.  Let's hope it's
	// deterministic or the STRCMP_EQUAL test will fail ...
	populate_multiple_test_errors(
		TEST_PATH, TEST_MESSAGE,
		TEST_PATH_2, TEST_MESSAGE_2);

	LONGS_EQUAL(-1, get_int(&test_conn, &test_req, &test_err));

	configd_error_format_for_commit_or_val(&test_err, TEST_OP);

	char *exp_output;
	int retval = asprintf(
		&exp_output,
		"[%s]\n\n%s\n\n[[%s]] failed.\n[%s]\n\n%s\n\n[[%s]] failed.\n\n%s failed!\n",
		TEST_SPACE_PATH_2, TEST_MESSAGE_2, TEST_SPACE_PATH_2,
		TEST_SPACE_PATH, TEST_MESSAGE, TEST_SPACE_PATH,
		TEST_OP);
	CHECK_FALSE(retval == -1);

	STRCMP_EQUAL(exp_output, test_err.text);
}

TEST(Format, single_error_unformatted)
{
	populate_single_test_error(
		PATH_FIELD, TEST_PATH,
		MESSAGE_FIELD, TEST_MESSAGE);

	// 'discard' is not formatted, and uses get_int() versus above tests
	// which all use get_str().
	POINTERS_EQUAL(NULL, configd_discard(&test_conn, &test_err));

	STRCMP_EQUAL(TEST_MESSAGE, test_err.text);
}

TEST(Format, multiple_error_unformatted)
{
	populate_multiple_test_errors(
		TEST_PATH, TEST_MESSAGE,
		TEST_PATH_2, TEST_MESSAGE_2);

	// 'discard' is not formatted, and uses get_int() versus above tests
	// which all use get_str().
	POINTERS_EQUAL(NULL, configd_discard(&test_conn, &test_err));

	char *exp_output;
	int retval = asprintf(
		&exp_output, "%s\n%s\n", TEST_MESSAGE, TEST_MESSAGE_2);
	CHECK_FALSE(retval == -1);

	STRCMP_EQUAL(exp_output, test_err.text);
}