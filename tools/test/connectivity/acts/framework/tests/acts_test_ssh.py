#!/usr/bin/env python3

# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import inspect
import io
import os
import sys
import time
import unittest
import re

from acts.controllers.utils_lib.ssh import connection
from acts.controllers.utils_lib.ssh import settings

SSH_HOST = None
SSH_USER = None


class SshTestCases(unittest.TestCase):
    def setUp(self):
        self.settings = settings.SshSettings(SSH_HOST, SSH_USER)

    def test_ssh_run(self):
        """Test running an ssh command.

        Test that running an ssh command works.
        """
        conn = connection.SshConnection(self.settings)

        result = conn.run('echo "Hello World"')
        self.assertTrue(result.stdout.find('Hello World') > -1)

    def test_ssh_env(self):
        """Test that special envirment variables get sent over ssh.

        Test that given a dictonary of enviroment variables they will be sent
        to the remote host.
        """
        conn = connection.SshConnection(self.settings)

        result = conn.run('printenv', env={'MYSPECIALVAR': 20})
        self.assertTrue(result.stdout.find('MYSPECIALVAR=20'))

    def test_ssh_permission_denied(self):
        """Test that permission denied works.

        Connect to a remote host using an invalid username and see if we are
        rejected.
        """
        with self.assertRaises(connection.Error):
            bad_settings = settings.SshSettings(SSH_HOST, SSH_USER + 'BAD')
            conn = connection.SshConnection(bad_settings)
            result = conn.run('echo "Hello World"')

    def test_ssh_unknown_host(self):
        """Test that permission denied works.

        Connect to a remote host using an invalid username and see if we are
        rejected.
        """
        with self.assertRaises(connection.Error):
            bad_settings = settings.SshSettings('BADHOSTNAME', SSH_USER)
            conn = connection.SshConnection(bad_settings)
            result = conn.run('echo "Hello World"')


if __name__ == '__main__':
    # Get host info from command line arguments.
    if len(sys.argv) < 3:
        raise ValueError("usage: %s <username> <hostname to ssh to>" %
                         sys.argv[0])
    SSH_HOST = sys.argv[2]
    SSH_USER = sys.argv[1]
    unittest.main(argv=sys.argv[0:1] + sys.argv[3:])
