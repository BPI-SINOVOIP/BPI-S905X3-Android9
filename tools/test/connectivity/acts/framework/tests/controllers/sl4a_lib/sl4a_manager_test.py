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
import mock
import unittest

from acts.controllers.sl4a_lib import sl4a_manager
from acts.controllers.sl4a_lib import rpc_client


class Sl4aManagerFactoryTest(unittest.TestCase):
    """Tests the sl4a_manager module-level functions."""

    def setUp(self):
        """Clears the Sl4aManager cache."""
        sl4a_manager._all_sl4a_managers = {}

    def test_create_manager(self):
        """Tests sl4a_manager.create_sl4a_manager().

        Tests that a new Sl4aManager is returned without an error.
        """
        adb = mock.Mock()
        adb.serial = 'SERIAL'
        sl4a_man = sl4a_manager.create_sl4a_manager(adb)
        self.assertEqual(sl4a_man.adb, adb)

    def test_create_sl4a_manager_return_already_created_manager(self):
        """Tests sl4a_manager.create_sl4a_manager().

        Tests that a second call to create_sl4a_manager() does not create a
        new Sl4aManager, and returns the first created Sl4aManager instead.
        """
        adb = mock.Mock()
        adb.serial = 'SERIAL'
        first_manager = sl4a_manager.create_sl4a_manager(adb)

        adb_same_serial = mock.Mock()
        adb_same_serial.serial = 'SERIAL'
        second_manager = sl4a_manager.create_sl4a_manager(adb)

        self.assertEqual(first_manager, second_manager)

    def test_create_sl4a_manager_multiple_devices_with_one_manager_each(self):
        """Tests sl4a_manager.create_sl4a_manager().

        Tests that when create_s4l4a_manager() is called for different devices,
        each device gets its own Sl4aManager object.
        """
        adb_1 = mock.Mock()
        adb_1.serial = 'SERIAL'
        first_manager = sl4a_manager.create_sl4a_manager(adb_1)

        adb_2 = mock.Mock()
        adb_2.serial = 'DIFFERENT_SERIAL_NUMBER'
        second_manager = sl4a_manager.create_sl4a_manager(adb_2)

        self.assertNotEqual(first_manager, second_manager)


class Sl4aManagerTest(unittest.TestCase):
    """Tests the sl4a_manager.Sl4aManager class."""
    ATTEMPT_INTERVAL = .25
    MAX_WAIT_ON_SERVER_SECONDS = 1
    _SL4A_LAUNCH_SERVER_CMD = ''
    _SL4A_CLOSE_SERVER_CMD = ''
    _SL4A_ROOT_FIND_PORT_CMD = ''
    _SL4A_USER_FIND_PORT_CMD = ''
    _SL4A_START_SERVICE_CMD = ''

    @classmethod
    def setUpClass(cls):
        # Copy all module constants before testing begins.
        Sl4aManagerTest.ATTEMPT_INTERVAL = \
            sl4a_manager.ATTEMPT_INTERVAL
        Sl4aManagerTest.MAX_WAIT_ON_SERVER_SECONDS = \
            sl4a_manager.MAX_WAIT_ON_SERVER_SECONDS
        Sl4aManagerTest._SL4A_LAUNCH_SERVER_CMD = \
            sl4a_manager._SL4A_LAUNCH_SERVER_CMD
        Sl4aManagerTest._SL4A_CLOSE_SERVER_CMD = \
            sl4a_manager._SL4A_CLOSE_SERVER_CMD
        Sl4aManagerTest._SL4A_ROOT_FIND_PORT_CMD = \
            sl4a_manager._SL4A_ROOT_FIND_PORT_CMD
        Sl4aManagerTest._SL4A_USER_FIND_PORT_CMD = \
            sl4a_manager._SL4A_USER_FIND_PORT_CMD
        Sl4aManagerTest._SL4A_START_SERVICE_CMD = \
            sl4a_manager._SL4A_START_SERVICE_CMD

    def setUp(self):
        # Restore all module constants at the beginning of each test case.
        sl4a_manager.ATTEMPT_INTERVAL = \
            Sl4aManagerTest.ATTEMPT_INTERVAL
        sl4a_manager.MAX_WAIT_ON_SERVER_SECONDS = \
            Sl4aManagerTest.MAX_WAIT_ON_SERVER_SECONDS
        sl4a_manager._SL4A_LAUNCH_SERVER_CMD = \
            Sl4aManagerTest._SL4A_LAUNCH_SERVER_CMD
        sl4a_manager._SL4A_CLOSE_SERVER_CMD = \
            Sl4aManagerTest._SL4A_CLOSE_SERVER_CMD
        sl4a_manager._SL4A_ROOT_FIND_PORT_CMD = \
            Sl4aManagerTest._SL4A_ROOT_FIND_PORT_CMD
        sl4a_manager._SL4A_USER_FIND_PORT_CMD = \
            Sl4aManagerTest._SL4A_USER_FIND_PORT_CMD
        sl4a_manager._SL4A_START_SERVICE_CMD = \
            Sl4aManagerTest._SL4A_START_SERVICE_CMD

        # Reset module data at the beginning of each test.
        sl4a_manager._all_sl4a_managers = {}

    def test_sl4a_ports_in_use(self):
        """Tests sl4a_manager.Sl4aManager.sl4a_ports_in_use

        Tests to make sure all server ports are returned with no duplicates.
        """
        adb = mock.Mock()
        manager = sl4a_manager.Sl4aManager(adb)
        session_1 = mock.Mock()
        session_1.server_port = 12345
        manager.sessions[1] = session_1
        session_2 = mock.Mock()
        session_2.server_port = 15973
        manager.sessions[2] = session_2
        session_3 = mock.Mock()
        session_3.server_port = 12345
        manager.sessions[3] = session_3
        session_4 = mock.Mock()
        session_4.server_port = 67890
        manager.sessions[4] = session_4
        session_5 = mock.Mock()
        session_5.server_port = 75638
        manager.sessions[5] = session_5

        returned_ports = manager.sl4a_ports_in_use

        # No duplicated ports.
        self.assertEqual(len(returned_ports), len(set(returned_ports)))
        # One call for each session
        self.assertSetEqual(set(returned_ports), {12345, 15973, 67890, 75638})

    @mock.patch('time.sleep', return_value=None)
    def test_start_sl4a_server_uses_all_retries(self, _):
        """Tests sl4a_manager.Sl4aManager.start_sl4a_server().

        Tests to ensure that _start_sl4a_server retries and successfully returns
        a port.
        """
        adb = mock.Mock()
        adb.shell = lambda _, **kwargs: ''

        side_effects = []
        expected_port = 12345
        for _ in range(int(sl4a_manager.MAX_WAIT_ON_SERVER_SECONDS /
                           sl4a_manager.ATTEMPT_INTERVAL) - 1):
            side_effects.append(None)
        side_effects.append(expected_port)

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager._get_open_listening_port = mock.Mock(side_effect=side_effects)
        try:
            found_port = manager.start_sl4a_server(0)
            self.assertTrue(found_port)
        except rpc_client.Sl4aConnectionError:
            self.fail('start_sl4a_server failed to respect FIND_PORT_RETRIES.')

    @mock.patch('time.sleep', return_value=None)
    def test_start_sl4a_server_fails_all_retries(self, _):
        """Tests sl4a_manager.Sl4aManager.start_sl4a_server().

        Tests to ensure that start_sl4a_server throws an error if all retries
        fail.
        """
        adb = mock.Mock()
        adb.shell = lambda _, **kwargs: ''

        side_effects = []
        for _ in range(int(sl4a_manager.MAX_WAIT_ON_SERVER_SECONDS /
                           sl4a_manager.ATTEMPT_INTERVAL)):
            side_effects.append(None)

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager._get_open_listening_port = mock.Mock(side_effect=side_effects)
        try:
            manager.start_sl4a_server(0)
            self.fail('Sl4aConnectionError was not thrown.')
        except rpc_client.Sl4aConnectionError:
            pass

    def test_get_all_ports_command_uses_root_cmd(self):
        """Tests sl4a_manager.Sl4aManager._get_all_ports_command().

        Tests that _get_all_ports_command calls the root command when root is
        available.
        """
        adb = mock.Mock()
        adb.is_root = lambda: True
        command = 'ngo45hke3b4vie3mv5ni93,vfu3j'
        sl4a_manager._SL4A_ROOT_FIND_PORT_CMD = command

        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertEqual(manager._get_all_ports_command(), command)

    def test_get_all_ports_command_escalates_to_root(self):
        """Tests sl4a_manager.Sl4aManager._call_get_ports_command().

        Tests that _call_get_ports_command calls the root command when adb is
        user but can escalate to root.
        """
        adb = mock.Mock()
        adb.is_root = lambda: False
        adb.ensure_root = lambda: True
        command = 'ngo45hke3b4vie3mv5ni93,vfu3j'
        sl4a_manager._SL4A_ROOT_FIND_PORT_CMD = command

        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertEqual(manager._get_all_ports_command(), command)

    def test_get_all_ports_command_uses_user_cmd(self):
        """Tests sl4a_manager.Sl4aManager._call_get_ports_command().

        Tests that _call_get_ports_command calls the user command when root is
        unavailable.
        """
        adb = mock.Mock()
        adb.is_root = lambda: False
        adb.ensure_root = lambda: False
        command = 'ngo45hke3b4vie3mv5ni93,vfu3j'
        sl4a_manager._SL4A_USER_FIND_PORT_CMD = command

        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertEqual(manager._get_all_ports_command(), command)

    def test_get_open_listening_port_no_port_found(self):
        """Tests sl4a_manager.Sl4aManager._get_open_listening_port().

        Tests to ensure None is returned if no open port is found.
        """
        adb = mock.Mock()
        adb.shell = lambda _: ''

        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertIsNone(manager._get_open_listening_port())

    def test_get_open_listening_port_no_new_port_found(self):
        """Tests sl4a_manager.Sl4aManager._get_open_listening_port().

        Tests to ensure None is returned if the ports returned have all been
        marked as in used.
        """
        adb = mock.Mock()
        adb.shell = lambda _: '12345 67890'

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager._sl4a_ports = {'12345', '67890'}
        self.assertIsNone(manager._get_open_listening_port())

    def test_get_open_listening_port_port_is_avaiable(self):
        """Tests sl4a_manager.Sl4aManager._get_open_listening_port().

        Tests to ensure a port is returned if a port is found and has not been
        marked as used.
        """
        adb = mock.Mock()
        adb.shell = lambda _: '12345 67890'

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager._sl4a_ports = {'12345'}
        self.assertEqual(manager._get_open_listening_port(), 67890)

    def test_is_sl4a_installed_is_true(self):
        """Tests sl4a_manager.Sl4aManager.is_sl4a_installed().

        Tests is_sl4a_installed() returns true when pm returns data
        """
        adb = mock.Mock()
        adb.shell = lambda _, **kwargs: 'asdf'
        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertTrue(manager.is_sl4a_installed())

    def test_is_sl4a_installed_is_false(self):
        """Tests sl4a_manager.Sl4aManager.is_sl4a_installed().

        Tests is_sl4a_installed() returns true when pm returns data
        """
        adb = mock.Mock()
        adb.shell = lambda _, **kwargs: ''
        manager = sl4a_manager.create_sl4a_manager(adb)
        self.assertFalse(manager.is_sl4a_installed())

    def test_start_sl4a_throws_error_on_sl4a_not_installed(self):
        """Tests sl4a_manager.Sl4aManager.start_sl4a_service().

        Tests that a MissingSl4aError is thrown when SL4A is not installed.
        """
        adb = mock.Mock()

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager.is_sl4a_installed = lambda: False
        try:
            manager.start_sl4a_service()
            self.fail('An error should have been thrown.')
        except rpc_client.MissingSl4AError:
            pass

    def test_start_sl4a_starts_sl4a_if_not_running(self):
        """Tests sl4a_manager.Sl4aManager.start_sl4a_service().

        Tests that SL4A is started if it was not already running.
        """
        adb = mock.Mock()
        adb.shell = mock.Mock(side_effect=['', '', ''])

        manager = sl4a_manager.create_sl4a_manager(adb)
        manager.is_sl4a_installed = lambda: True
        try:
            manager.start_sl4a_service()
        except rpc_client.MissingSl4AError:
            self.fail('An error should not have been thrown.')
        adb.shell.assert_called_with(sl4a_manager._SL4A_START_SERVICE_CMD)

    def test_create_session_uses_oldest_server_port(self):
        """Tests sl4a_manager.Sl4aManager.create_session().

        Tests that when no port is given, the oldest server port opened is used
        as the server port for a new session. The oldest server port can be
        found by getting the oldest session's server port.
        """
        adb = mock.Mock()

        manager = sl4a_manager.create_sl4a_manager(adb)
        # Ignore starting SL4A.
        manager.start_sl4a_service = lambda: None

        session_1 = mock.Mock()
        session_1.server_port = 12345
        session_2 = mock.Mock()
        session_2.server_port = 67890
        session_3 = mock.Mock()
        session_3.server_port = 67890

        manager.sessions[3] = session_3
        manager.sessions[1] = session_1
        manager.sessions[2] = session_2

        with mock.patch.object(
                rpc_client.RpcClient, '__init__', return_value=None):
            created_session = manager.create_session()

        self.assertEqual(created_session.server_port, session_1.server_port)

    def test_create_session_uses_random_port_when_no_session_exists(self):
        """Tests sl4a_manager.Sl4aManager.create_session().

        Tests that when no port is given, and no SL4A server exists, the server
        port for the session is set to 0.
        """
        adb = mock.Mock()

        manager = sl4a_manager.create_sl4a_manager(adb)
        # Ignore starting SL4A.
        manager.start_sl4a_service = lambda: None

        with mock.patch.object(
                rpc_client.RpcClient, '__init__', return_value=None):
            created_session = manager.create_session()

        self.assertEqual(created_session.server_port, 0)

    def test_terminate_all_session_call_terminate_on_all_sessions(self):
        """Tests sl4a_manager.Sl4aManager.terminate_all_sessions().

        Tests to see that the manager has called terminate on all sessions.
        """
        called_terminate_on = list()

        def called_on(session):
            called_terminate_on.append(session)

        adb = mock.Mock()
        manager = sl4a_manager.Sl4aManager(adb)

        session_1 = mock.Mock()
        session_1.terminate = lambda *args, **kwargs: called_on(session_1)
        manager.sessions[1] = session_1
        session_4 = mock.Mock()
        session_4.terminate = lambda *args, **kwargs: called_on(session_4)
        manager.sessions[4] = session_4
        session_5 = mock.Mock()
        session_5.terminate = lambda *args, **kwargs: called_on(session_5)
        manager.sessions[5] = session_5

        manager._get_all_ports = lambda: []
        manager.terminate_all_sessions()
        # No duplicates calls to terminate.
        self.assertEqual(
            len(called_terminate_on), len(set(called_terminate_on)))
        # One call for each session
        self.assertSetEqual(
            set(called_terminate_on), {session_1, session_4, session_5})

    def test_terminate_all_session_close_each_server(self):
        """Tests sl4a_manager.Sl4aManager.terminate_all_sessions().

        Tests to see that the manager has called terminate on all sessions.
        """
        closed_ports = list()

        def close(command):
            if str.isdigit(command):
                closed_ports.append(command)
            return ''

        adb = mock.Mock()
        adb.shell = close
        sl4a_manager._SL4A_CLOSE_SERVER_CMD = '%s'
        ports_to_close = {'12345', '67890', '24680', '13579'}

        manager = sl4a_manager.Sl4aManager(adb)
        manager._sl4a_ports = set(ports_to_close)
        manager._get_all_ports = lambda: []
        manager.terminate_all_sessions()

        # No duplicate calls to close port
        self.assertEqual(len(closed_ports), len(set(closed_ports)))
        # One call for each port
        self.assertSetEqual(ports_to_close, set(closed_ports))

    def test_obtain_sl4a_server_starts_new_server(self):
        """Tests sl4a_manager.Sl4aManager.obtain_sl4a_server().

        Tests that a new server can be returned if the server does not exist.
        """
        adb = mock.Mock()
        manager = sl4a_manager.Sl4aManager(adb)
        manager.start_sl4a_server = mock.Mock()

        manager.obtain_sl4a_server(0)

        self.assertTrue(manager.start_sl4a_server.called)

    @mock.patch(
        'acts.controllers.sl4a_lib.sl4a_manager.Sl4aManager.sl4a_ports_in_use',
        new_callable=mock.PropertyMock)
    def test_obtain_sl4a_server_returns_existing_server(
            self, sl4a_ports_in_use):
        """Tests sl4a_manager.Sl4aManager.obtain_sl4a_server().

        Tests that an existing server is returned if it is already opened.
        """
        adb = mock.Mock()
        manager = sl4a_manager.Sl4aManager(adb)
        manager.start_sl4a_server = mock.Mock()
        sl4a_ports_in_use.return_value = [12345]

        ret = manager.obtain_sl4a_server(12345)

        self.assertFalse(manager.start_sl4a_server.called)
        self.assertEqual(12345, ret)


if __name__ == '__main__':
    unittest.main()
