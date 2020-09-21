#!/usr/bin/env python
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging

from vts.runners.host import asserts
from vts.runners.host import base_test
from vts.runners.host import test_runner

from vts.runners.host import const


class LibcTest(base_test.BaseTestClass):
    """A basic test of the libc API."""

    def setUpClass(self):
        self.dut = self.android_devices[0]
        self.dut.lib.InitSharedLib(
            target_type="bionic_libc",
            target_basepaths=["/system/lib64"],
            target_version=1.0,
            target_filename="libc.so",
            bits=64,
            handler_name="libc",
            target_package="lib.ndk.bionic")

    def testOpenCloseLocalSocketStream(self):
        """Tests open and close socket operations for local communication.

        Uses local addresses and a streaming socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_UNIX,
                                          self.dut.lib.libc.SOCK_STREAM, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")

    def testOpenCloseLocalSocketDatagram(self):
        """Tests open and close socket operations for local communication.

        Uses local addresses and a datagram socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_UNIX,
                                          self.dut.lib.libc.SOCK_DGRAM, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")

    def testOpenCloseLocalSocketRaw(self):
        """Tests open and close socket operations for local communication.

        Uses local addresses and a raw socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_UNIX,
                                          self.dut.lib.libc.SOCK_RAW, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")

    def testOpenCloseLocalSocketSequential(self):
        """Tests open and close socket operations for local communication.

        Uses local addresses and a sequential socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_UNIX,
                                          self.dut.lib.libc.SOCK_SEQPACKET, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")

    def testOpenCloseINETSocketStream(self):
        """Tests open and close socket operations for INET communication.

        Uses IP addresses and a streaming socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_INET,
                                          self.dut.lib.libc.SOCK_STREAM, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")

    def testOpenCloseINETSocketDatagram(self):
        """Tests open and close socket operations for INET communication.

        Uses IP addresses and a datagram socket.
        """
        result = self.dut.lib.libc.socket(self.dut.lib.libc.PF_INET,
                                          self.dut.lib.libc.SOCK_DGRAM, 0)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.socket: could not create socket.")

        result = self.dut.lib.libc.close(
            result.return_type.scalar_value.int32_t)
        asserts.assertNotEqual(result.return_type.scalar_value.int32_t, -1,
                               "libc.close: unable to close socket.")


if __name__ == "__main__":
    test_runner.main()
