#!/usr/bin/python
# Copyright (c) 2019, AT&T Intellectual Property. All rights reserved.
#
# Copyright (c) 2015 by Brocade Communications Systems, Inc.
# All rights reserved.
#
# SPDX-License-Identifier: LGPL-2.1-only

# Simple tests to validate functionality of SWIG generated python API
# Run on vRouter.
# Some paths may need custom tweaking
#
# NOTE: Python unittest framework runs tests in *lexicographical* order.

import os
import unittest
from vyatta import configd

def cmp_map(m1, m2):
    if len(m1) != len(m2):
        return False
    for key in m1:
        if key in m2 and m1[key] != m2[key]:
            return False
    return True

def setUpModule():
    global client
    global sessid
    client = configd.Client()
    sessid = str(os.getpid())

class TestSessionSetup(unittest.TestCase):
    def tearDown(self):
        client.session_teardown()

    def test_session_setup_invalid(self):
        with self.assertRaises(configd.Exception) as cm:
            result = client.session_setup("")
        self.assertEqual(cm.exception.what(), "must specify a session identifier")

    def test_session_setup_name(self):
        client.session_setup(sessid)
        result = client.session_exists()
        self.assertEqual(result, True, "Unable to create named session")

class TestSessionMethods(unittest.TestCase):
    def setUp(self):
        client.session_setup(sessid)

    def tearDown(self):
        client.session_teardown()

    def test_session_saved(self):
        result = client.session_saved()
        self.assertEqual(result, False, "Incorrectly marked saved")
        client.session_mark_saved()
        result = client.session_saved()
        self.assertEqual(result, True, "Incorrectly marked unsaved")
        client.session_mark_unsaved()
        result = client.session_saved()
        self.assertEqual(result, False, "Incorrectly marked saved after SessionUnsaved")

    def test_session_lock(self):
        result = client.session_locked()
        self.assertEqual(result, False, "Incorrectly locked")
        client.session_lock()
        result = client.session_locked()
        self.assertEqual(result, True, "Incorrectly unlocked")
        client.session_unlock()
        result = client.session_locked()
        self.assertEqual(result, False, "Incorrectly locked after unlock")

    def test_session_changed(self):
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        result = client.session_changed()
        self.assertEqual(result, False, "Unable to validate path %s" % path)
        if client.node_exists(client.AUTO, path):
            client.delete(path)
        else:
            client.set(path)
        result = client.session_changed()
        self.assertEqual(result, True, "Unable to validate path %s" % path)

class TestTransactionMethods(unittest.TestCase):
    def setUp(self):
        client.session_setup(sessid)

    def tearDown(self):
        client.session_teardown()

    def test_transaction_validate_path(self):
        path = ["interfaces", "dataplane", "dp0s3"]
        invalid_path = ["interfaces", "dataplane", "3"]
        result = client.validate_path(path)
        self.assertEqual(result, "", "Invalid path: %s" % result)
        with self.assertRaises(configd.Exception) as cm:
            result = client.validate_path(invalid_path)
        self.assertEqual(cm.exception.what(), 'Configuration path: interfaces dataplane [3] is not valid\n\nMust be one of the following:\n  <dpFpNpS>\n  <dpFemN>\n  <dpFsN>\n\nValue validation failed\n')

    def test_transaction_set(self):
        invalid_path = ["system", "ntp", "3.ca.pool.ntp.org"]
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        with self.assertRaises(configd.Exception) as cm:
            result = client.set(invalid_path)
        self.assertEqual(cm.exception.what(), 'Configuration path: system [ntp] is not valid\n\nPath is invalid\n\nSet failed\n')
        result = client.set(path)
        self.assertEqual(result, "", "Unable to set path %s" % path)

    def test_transaction_validate(self):
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        client.set(path)
        result = client.validate()
        self.assertEqual(result, "", "Unable to validate transaction")

    def test_transaction_discard(self):
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        client.set(path)
        result = client.discard()
        self.assertEqual(result, "", "Unable to discard changes")
        result = client.session_changed()
        self.assertEqual(result, False, "Session incorrectly marked changed")

    def test_transaction_delete(self):
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        client.set(path)
        result = client.delete(path)
        self.assertEqual(result, "", "Unable to delete path %s" % path)
        result = client.node_exists(client.AUTO, path)
        self.assertEqual(result, False, "Node incorrectly exists")

    def test_transaction_commit(self):
        exp_result = "[system ntp]\nStopping NTP server: ntpd.\nStarting NTP server: ntpd.\n\n"
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        # Make sure there is a change to commit
        if client.node_exists(client.AUTO, path):
            client.delete(path)
        else:
            client.set(path)
        result = client.commit("")
        self.assertEqual(result, exp_result, "Unable to commit")
        # Clean up if we added a node
        if client.node_exists(client.AUTO, path):
            client.delete(path)
            client.commit("")

    def test_transaction_save_load(self):
        exp_result = "[system ntp]\nStopping NTP server: ntpd.\nStarting NTP server: ntpd.\n\n"
        path = ["system", "ntp", "server", "3.ca.pool.ntp.org"]
        if client.node_exists(client.AUTO, path):
            client.delete(path)
            client.commit("")
        client.set(path)
        client.commit("")
        client.save()
        client.delete(path)

        result = client.load("/config/config.boot")
        self.assertEqual(result, True, "Unable to load")
        result = client.delete(path)
        self.assertEqual(result, "", "Unable to modify path %s; load failed" % path)
        client.commit("")
        client.save()

class TestTemplateMethods(unittest.TestCase):
    def test_template_get(self):
        path = ["system", "ntp", "server"]
        exp_result = {'help': 'Network Time Protocol (NTP) server','key': 'tagnode','tag': '1','type': 'txt'}
        result = client.template_get(path)
        self.assertEqual(cmp_map(result, exp_result), True, "Incorrect help returned")

    def test_template_get_children(self):
        path = ["system"]
        exp_result = ['acm', 'alg', 'config-management', 'config-sync', 'console', 'domain-name', 'domain-search', 'fips', 'host-name', 'ip', 'ipv6', 'login', 'name-server', 'ntp', 'options', 'package', 'power-profile', 'session', 'static-host-mapping', 'syslog', 'tacplus-options', 'time-zone']
        result = sorted(client.template_get_children(path))
        self.assertEqual(result, exp_result, "Incorrect children returned")

    def test_template_get_allowed(self):
        path = ["system", "console", "device", "ttyS0", "speed"]
        exp_result = ['115200', '1200', '19200', '2400', '38400', '4800', '57600', '9600']
        result = sorted(client.template_get_allowed(path))
        self.assertEqual(result, exp_result, "Incorrect allowed returned")

    def test_template_validate_path(self):
        path = ["system", "console", "device", "ttyS0", "speed", "1"]
        result = client.template_validate_path(path)
        self.assertEqual(result, True, "Invalid path: %s" % path)
        path = ["foo", "bar", "baz"]
        result = client.template_validate_path(path)
        self.assertEqual(result, False, "Unexpected valid path: %s" % path)

    def test_template_validate_value(self):
        path = ["system", "console", "device", "ttyS0", "speed", "115200"]
        result = client.template_validate_values(path)
        self.assertEqual(result, True, "Incorrectly innvalid path: %s" % path)
        path = ["system", "console", "device", "ttyS0", "speed", "1"]
        result = client.template_validate_values(path)
        self.assertEqual(result, False, "Incorrectly valid path: %s" % path)

    def test_get_help(self):
        path = ["system", "console", "device"]
        exp_result = {'ttyS0': 'Serial console device name'}
        result = client.get_help(path, False)
        self.assertEqual(cmp_map(result, exp_result), True, "Incorrect help returned")

        exp_result = {'<pattern>': 'Serial console device name','ttyS0': 'Serial console device name'}
        result = client.get_help(path, True)
        self.assertEqual(cmp_map(result, exp_result), True, "Incorrect help returned")

class TestNodeMethods(unittest.TestCase):
    def setUp(self):
        client.session_setup(sessid)

    def tearDown(self):
        client.session_teardown()

    def test_node_exists(self):
        path = ["system", "console", "device", "ttyS0", "speed", "115200"]
        result = client.node_exists(client.AUTO, path)
        self.assertEqual(result, False, "Incorrect path existence")
        path = ["system", "console", "device", "ttyS0", "speed"]
        result = client.node_exists(client.AUTO, path)
        self.assertEqual(result, True, "Incorrect path existence")

    def test_node_is_default(self):
        path = ["system", "console", "device", "ttyS0", "speed"]
        result = client.node_is_default(client.AUTO, path)
        self.assertEqual(result, True, "Incorrectly set as default")

    def test_node_get(self):
        path = ["system", "console", "device", "ttyS0"]
        exp_result = ('speed',)
        result = client.node_get(client.RUNNING, path)
        self.assertEqual(result, exp_result, "Incorrect node result")

    @unittest.skip("node_get_status test not implemented yet")
    def test_node_get_status(self):
        result = client.node_get_status(client.AUTO, path)

    def test_node_get_type(self):
        path = ["system", "name-server"]
        result = client.node_get_type(path)
        self.assertEqual(result, client.MULTI, "Incorrect node type for path %s" % path)
        path = ["system", "console"]
        result = client.node_get_type(path)
        self.assertEqual(result, client.CONTAINER, "Incorrect node type for path %s" % path)
        path = ["system", "console", "device"]
        result = client.node_get_type(path)
        self.assertEqual(result, client.TAG, "Incorrect node type for path %s" % path)
        path = ["system", "console", "device", "ttyS0"]
        result = client.node_get_type(path)
        self.assertEqual(result, client.CONTAINER, "Incorrect node type for path %s" % path)
        path = ["system", "console", "device", "ttyS0", "speed"]
        result = client.node_get_type(path)
        self.assertEqual(result, client.LEAF, "Incorrect node type for path %s" % path)

if __name__ == '__main__':
    unittest.main()
