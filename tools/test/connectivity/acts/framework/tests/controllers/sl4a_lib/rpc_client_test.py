#!/usr/bin/env python3
#
#   Copyright 2018 - The Android Open Source Project
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
import unittest

import mock

from acts.controllers.sl4a_lib import rpc_client


class BreakoutError(Exception):
    """Thrown to prove program execution."""


class RpcClientTest(unittest.TestCase):
    """Tests the rpc_client.RpcClient class."""

    def test_terminate_warn_on_working_connections(self):
        """Tests rpc_client.RpcClient.terminate().

        Tests that if some connections are still working, we log this before
        closing the connections.
        """
        session = mock.Mock()

        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        client._log = mock.Mock()
        client._working_connections = [mock.Mock()]

        client.terminate()

        self.assertTrue(client._log.warning.called)

    def test_terminate_closes_all_connections(self):
        """Tests rpc_client.RpcClient.terminate().

        Tests that all free and working connections have been closed.
        """
        session = mock.Mock()

        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        client._log = mock.Mock()
        working_connections = [mock.Mock() for _ in range(3)]
        free_connections = [mock.Mock() for _ in range(3)]
        client._free_connections = free_connections
        client._working_connections = working_connections

        client.terminate()

        for connection in working_connections + free_connections:
            self.assertTrue(connection.close.called)

    def test_get_free_connection_get_available_client(self):
        """Tests rpc_client.RpcClient._get_free_connection().

        Tests that an available client is returned if one exists.
        """

        def fail_on_wrong_execution():
            self.fail('The program is not executing the expected path. '
                      'Tried to return an available free client, ended up '
                      'sleeping to wait for client instead.')

        session = mock.Mock()

        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        expected_connection = mock.Mock()
        client._free_connections = [expected_connection]
        client._lock = mock.MagicMock()

        with mock.patch('time.sleep') as sleep_mock:
            sleep_mock.side_effect = fail_on_wrong_execution

            connection = client._get_free_connection()

        self.assertEqual(connection, expected_connection)
        self.assertTrue(expected_connection in client._working_connections)
        self.assertEqual(len(client._free_connections), 0)

    def test_get_free_connection_continues_upon_connection_taken(self):
        """Tests rpc_client.RpcClient._get_free_connection().

        Tests that if the free connection is taken while trying to acquire the
        lock to reserve it, the thread gives up the lock and tries again.
        """

        def empty_list():
            client._free_connections.clear()

        def fail_on_wrong_execution():
            self.fail('The program is not executing the expected path. '
                      'Tried to return an available free client, ended up '
                      'sleeping to wait for client instead.')

        session = mock.Mock()

        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        client._free_connections = mock.Mock()
        client._lock = mock.MagicMock()
        client._lock.acquire.side_effect = empty_list
        client._free_connections = [mock.Mock()]

        with mock.patch('time.sleep') as sleep_mock:
            sleep_mock.side_effect = fail_on_wrong_execution

            try:
                client._get_free_connection()
            except IndexError:
                self.fail('Tried to pop free connection when another thread'
                          'has taken it.')
        # Assert that the lock has been freed.
        self.assertEqual(client._lock.acquire.call_count,
                         client._lock.release.call_count)

    def test_get_free_connection_sleep(self):
        """Tests rpc_client.RpcClient._get_free_connection().

        Tests that if the free connection is taken, it will wait for a new one.
        """

        session = mock.Mock()

        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        client._free_connections = []
        client.max_connections = 0
        client._lock = mock.MagicMock()
        client._free_connections = []

        with mock.patch('time.sleep') as sleep_mock:
            sleep_mock.side_effect = BreakoutError()
            try:
                client._get_free_connection()
            except BreakoutError:
                # Assert that the lock has been freed.
                self.assertEqual(client._lock.acquire.call_count,
                                 client._lock.release.call_count)
                # Asserts that the sleep has been called.
                self.assertTrue(sleep_mock.called)
                # Asserts that no changes to connections happened
                self.assertEqual(len(client._free_connections), 0)
                self.assertEqual(len(client._working_connections), 0)
                return True
        self.fail('Failed to hit sleep case')

    def test_release_working_connection(self):
        """Tests rpc_client.RpcClient._release_working_connection.

        Tests that the working connection is moved into the free connections.
        """
        session = mock.Mock()
        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())

        connection = mock.Mock()
        client._working_connections = [connection]
        client._free_connections = []
        client._release_working_connection(connection)

        self.assertTrue(connection in client._free_connections)
        self.assertFalse(connection in client._working_connections)

    def test_future(self):
        """Tests rpc_client.RpcClient.future.

        """
        session = mock.Mock()
        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())

        self.assertEqual(client.future, client._async_client)

    def test_getattr(self):
        """Tests rpc_client.RpcClient.__getattr__.

        Tests that the name, args, and kwargs are correctly passed to self.rpc.
        """
        session = mock.Mock()
        client = rpc_client.RpcClient(session.uid, session.adb.serial,
                                      lambda _: mock.Mock(),
                                      lambda _: mock.Mock())
        client.rpc = mock.MagicMock()
        fn = client.fake_function_please_do_not_be_implemented

        fn('arg1', 'arg2', kwarg1=1, kwarg2=2)
        client.rpc.assert_called_with(
            'fake_function_please_do_not_be_implemented',
            'arg1',
            'arg2',
            kwarg1=1,
            kwarg2=2)


if __name__ == '__main__':
    unittest.main()
