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
import mock
import unittest

from acts.controllers.sl4a_lib import rpc_client, rpc_connection

MOCK_RESP = b'{"id": 0, "result": 123, "error": null, "status": 1, "uid": 1}'
MOCK_RESP_UNKNOWN_UID = b'{"id": 0, "result": 123, "error": null, "status": 0}'
MOCK_RESP_WITH_ERROR = b'{"id": 0, "error": 1, "status": 1, "uid": 1}'


class MockSocketFile(object):
    def __init__(self, resp):
        self.resp = resp
        self.last_write = None

    def write(self, msg):
        self.last_write = msg

    def readline(self):
        return self.resp

    def flush(self):
        pass


class RpcConnectionTest(unittest.TestCase):
    """This test class has unit tests for the implementation of everything
    under acts.controllers.android, which is the RPC client module for sl4a.
    """

    @staticmethod
    def mock_rpc_connection(response=MOCK_RESP,
                            uid=rpc_connection.UNKNOWN_UID):
        """Sets up a faked socket file from the mock connection."""
        fake_file = MockSocketFile(response)
        fake_conn = mock.MagicMock()
        fake_conn.makefile.return_value = fake_file
        adb = mock.Mock()
        ports = mock.Mock()

        return rpc_connection.RpcConnection(
            adb, ports, fake_conn, fake_file, uid=uid)

    def test_open_chooses_init_on_unknown_uid(self):
        """Tests rpc_connection.RpcConnection.open().

        Tests that open uses the init start command when the uid is unknown.
        """

        def pass_on_init(start_command):
            if not start_command == rpc_connection.Sl4aConnectionCommand.INIT:
                self.fail(
                    'Must call "init". Called "%s" instead.' % start_command)

        connection = self.mock_rpc_connection()
        connection._initiate_handshake = pass_on_init
        connection.open()

    def test_open_chooses_continue_on_known_uid(self):
        """Tests rpc_connection.RpcConnection.open().

        Tests that open uses the continue start command when the uid is known.
        """

        def pass_on_continue(start_command):
            if start_command != rpc_connection.Sl4aConnectionCommand.CONTINUE:
                self.fail('Must call "continue". Called "%s" instead.' %
                          start_command)

        connection = self.mock_rpc_connection(uid=1)
        connection._initiate_handshake = pass_on_continue
        connection.open()

    def test_initiate_handshake_returns_uid(self):
        """Tests rpc_connection.RpcConnection._initiate_handshake().

        Test that at the end of a handshake with no errors the client object
        has the correct parameters.
        """
        connection = self.mock_rpc_connection()
        connection._initiate_handshake(
            rpc_connection.Sl4aConnectionCommand.INIT)

        self.assertEqual(connection.uid, 1)

    def test_initiate_handshake_returns_unknown_status(self):
        """Tests rpc_connection.RpcConnection._initiate_handshake().

        Test that when the handshake is given an unknown uid then the client
        will not be given a uid.
        """
        connection = self.mock_rpc_connection(MOCK_RESP_UNKNOWN_UID)
        connection._initiate_handshake(
            rpc_connection.Sl4aConnectionCommand.INIT)

        self.assertEqual(connection.uid, rpc_client.UNKNOWN_UID)

    def test_initiate_handshake_no_response(self):
        """Tests rpc_connection.RpcConnection._initiate_handshake().

        Test that if a handshake receives no response then it will give a
        protocol error.
        """
        connection = self.mock_rpc_connection(b'')

        with self.assertRaises(
                rpc_client.Sl4aProtocolError,
                msg=rpc_client.Sl4aProtocolError.NO_RESPONSE_FROM_HANDSHAKE):
            connection._initiate_handshake(
                rpc_connection.Sl4aConnectionCommand.INIT)

    def test_cmd_properly_formatted(self):
        """Tests rpc_connection.RpcConnection._cmd().

        Tests that the command sent is properly formatted.
        """
        connection = self.mock_rpc_connection(MOCK_RESP)
        connection._cmd('test')
        self.assertIn(
            connection._socket_file.last_write,
            [b'{"cmd": "test", "uid": -1}\n', b'{"uid": -1, "cmd": "test"}\n'])

    def test_get_new_ticket(self):
        """Tests rpc_connection.RpcConnection.get_new_ticket().

        Tests that a new number is always given for get_new_ticket().
        """
        connection = self.mock_rpc_connection(MOCK_RESP)
        self.assertEqual(connection.get_new_ticket() + 1,
                         connection.get_new_ticket())


if __name__ == "__main__":
    unittest.main()
