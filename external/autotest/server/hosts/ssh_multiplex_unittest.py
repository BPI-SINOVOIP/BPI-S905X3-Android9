#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common

from autotest_lib.server.hosts import ssh_multiplex


class ConnectionPoolTest(unittest.TestCase):
    """ Test for SSH Connection Pool """
    def test_get(self):
        """ We can get MasterSsh object for a host from the pool """
        p = ssh_multiplex.ConnectionPool()
        conn1 = p.get('host', 'user', 22)
        self.assertIsNotNone(conn1)

        conn2 = p.get('host', 'user', 22)
        self.assertEquals(conn1, conn2)


if __name__ == '__main__':
    unittest.main()
