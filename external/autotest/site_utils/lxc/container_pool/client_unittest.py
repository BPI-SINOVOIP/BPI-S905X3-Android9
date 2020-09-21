#!/usr/bin/python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import socket
import tempfile
import unittest

import common
from autotest_lib.site_utils import lxc
from autotest_lib.site_utils.lxc import unittest_setup
from autotest_lib.site_utils.lxc.container_pool import async_listener
from autotest_lib.site_utils.lxc.container_pool import client


# Timeout for tests.
TIMEOUT = 30


class ClientTests(unittest.TestCase):
    """Unit tests for the Client class."""

    @classmethod
    def setUpClass(cls):
        """Creates a directory for running the unit tests."""
        # Explicitly use /tmp as the tmpdir.  Board specific TMPDIRs inside of
        # the chroot are set to a path that causes the socket address to exceed
        # the maximum allowable length.
        cls.test_dir = tempfile.mkdtemp(prefix='client_unittest_', dir='/tmp')


    @classmethod
    def tearDownClass(cls):
        """Deletes the test directory."""
        shutil.rmtree(cls.test_dir)


    def setUp(self):
        """Per-test setup."""
        # Put each test in its own test dir, so it's hermetic.
        self.test_dir = tempfile.mkdtemp(dir=ClientTests.test_dir)
        self.address = os.path.join(self.test_dir,
                                    lxc.DEFAULT_CONTAINER_POOL_SOCKET)
        self.listener = async_listener.AsyncListener(self.address)
        self.listener.start()


    def tearDown(self):
        self.listener.close()


    def testConnection(self):
        """Tests a basic client connection."""
        # Verify that no connections are pending.
        self.assertIsNone(self.listener.get_connection())

        # Connect a client, then verify that the host connection is established.
        host = None
        with client.Client.connect(self.address, TIMEOUT):
            host = self.listener.get_connection(TIMEOUT)
            self.assertIsNotNone(host)

        # Client closed - check that the host connection also closed.
        self.assertTrue(host.poll(TIMEOUT))
        with self.assertRaises(EOFError):
            host.recv()


    def testConnection_badAddress(self):
        """Tests that connecting to a bad address fails."""
        # Make a bogus address, then assert that the client fails.
        address = '%s.foobar' % self.address
        with self.assertRaises(socket.error):
            client.Client(address, 0)


    def testConnection_timeout(self):
        """Tests that connection attempts time out properly."""
        with tempfile.NamedTemporaryFile(dir=self.test_dir) as tmp:
            with self.assertRaises(socket.timeout):
                client.Client(tmp.name, 0)


    def testConnection_deadLine(self):
        """Tests that the connection times out if no action is ever taken."""
        id = 3
        short_timeout = TIMEOUT/2
        with client.Client.connect(self.address, TIMEOUT) as c:
            self.assertIsNone(c.get_container(id, short_timeout))

if __name__ == '__main__':
    unittest_setup.setup(require_sudo=False)
    unittest.main()
