# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/network/rf_switch/scpi.py."""

import socket
import unittest

from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.server.cros.network.rf_switch import rf_mocks
from autotest_lib.server.cros.network.rf_switch import scpi


class ScpiTestBase(unittest.TestCase):
    """Base class for scpi unit test cases."""

    _HOST = '1.2.3.4'
    _PORT = 5015

    def setUp(self):
        self.god = mock.mock_god(debug=False)

    def tearDown(self):
        self.god.unstub_all()


class ScpiInitTest(ScpiTestBase):
    """Unit test for the scpi initializing."""

    def test_scpi_init(self):
        """Verify socket connection creations for SCPI."""
        self.god.stub_function(socket, 'socket')
        sock = rf_mocks.SocketMock(socket, socket)
        socket.socket.expect_call().and_return(sock)

        self.god.stub_function(sock, 'connect')
        sock.connect.expect_call((self._HOST, self._PORT))
        mock_scpi = rf_mocks.ScpiMock(self._HOST, self._PORT)
        self.god.check_playback()

        self.assertEquals(mock_scpi.host, self._HOST)
        self.assertEquals(mock_scpi.port, self._PORT)


class ScpiMethodTests(ScpiTestBase):
    """Unit tests for scpi Methods."""

    def setUp(self):
        """Define mock god and mock scpi."""
        super(ScpiMethodTests, self).setUp()
        self.mock_scpi = rf_mocks.ScpiMock(
            self._HOST, self._PORT, test_interface=True)

    def test_write_method(self):
        """Verify write method sends correct command."""
        test_command = 'this is a command'

        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s' % test_command)

        # call reset and see correct command is send to socket.
        self.mock_scpi.write(test_command)
        self.god.check_playback()

    def test_read_method(self):
        """Verify read method."""
        test_buffer = 'This is a response.'

        # Mock socket receive to send our test buffer.
        self.god.stub_function_to_return(
                self.mock_scpi.socket, 'recv', test_buffer)

        response = self.mock_scpi.read()

        # check we got correct information back
        self.assertEqual(test_buffer, response, 'Read response did not match')

    def test_query_method(self):
        """Verify Query command send and receives response back."""
        test_command = 'this is a command'
        test_buffer = 'This is a response.'

        # Mock socket receive to send our test buffer.
        self.god.stub_function_to_return(
                self.mock_scpi.socket, 'recv', test_buffer)

        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s' % test_command)

        # call reset and see correct command is send to socket.
        response = self.mock_scpi.query(test_command)
        self.god.check_playback()

        # check we got correct information back
        self.assertEqual(test_buffer, response, 'Read response did not match')

    def test_error_query_method(self):
        """Verify error query returns correct error and message."""
        code = 101
        msg = 'Error Message'

        self.god.stub_function_to_return(
                self.mock_scpi.socket, 'recv', '%d, "%s"' % (code, msg))

        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s\n' %
                                               scpi.Scpi.CMD_ERROR_CHECK)

        # call info and see correct command is send to socket.
        code_recv, msg_recv = self.mock_scpi.error_query()
        self.god.check_playback()

        # check we got right information back
        self.assertEqual(code, code_recv, 'Error code did not match')
        self.assertEqual(msg, msg_recv, 'Error message did not match')

    def test_info_method(self):
        """Verify info method returns correct information."""
        info = {
            'Manufacturer': 'Company Name',
            'Model': 'Model Name',
            'Serial': '1234567890',
            'Version': '1.2.3.4.5'
        }
        self.god.stub_function_to_return(
                self.mock_scpi.socket, 'recv', '%s,%s,%s,%s' % (
                        info['Manufacturer'], info['Model'], info['Serial'],
                        info['Version']))

        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s\n' % scpi.Scpi.CMD_IDENTITY)

        # call info and see correct command is send to socket.
        info_recv = self.mock_scpi.info()
        self.god.check_playback()

        # check we got right information back
        self.assertDictEqual(info, info_recv, 'Info returned did not match')

    def test_reset_method(self):
        """Verify reset method."""
        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s\n' % scpi.Scpi.CMD_RESET)

        # call reset and see correct command is send to socket.
        self.mock_scpi.reset()
        self.god.check_playback()

    def test_status_method(self):
        """Verify status method."""
        status = 1

        self.god.stub_function_to_return(self.mock_scpi.socket, 'recv', status)

        # monitor send for correct command.
        self.god.stub_function(self.mock_scpi.socket, 'send')
        self.mock_scpi.socket.send.expect_call('%s\n' %
                                               scpi.Scpi.CMD_STATUS)

        # call info and see correct command is send to socket.
        status_recv = self.mock_scpi.status()
        self.god.check_playback()

        # check we got right information back
        self.assertEqual(status, status_recv, 'returned status did not match')

if __name__ == '__main__':
    unittest.main()
