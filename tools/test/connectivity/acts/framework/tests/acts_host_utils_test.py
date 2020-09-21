#!/usr/bin/env python3
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import socket
import unittest

from acts.controllers.utils_lib import host_utils


class ActsHostUtilsTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.controllers.adb.
    """

    def test_detects_udp_port_in_use(self):
        test_s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        test_s.bind(('localhost', 0))
        port = test_s.getsockname()[1]
        try:
            self.assertFalse(host_utils.is_port_available(port))
        finally:
            test_s.close()

    def test_detects_tcp_port_in_use(self):
        test_s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        test_s.bind(('localhost', 0))
        port = test_s.getsockname()[1]
        try:
            self.assertFalse(host_utils.is_port_available(port))
        finally:
            test_s.close()


if __name__ == "__main__":
    unittest.main()
