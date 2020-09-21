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
import errno
import mock
from socket import timeout
from socket import error as socket_error
import unittest
from mock import patch

from acts.controllers.sl4a_lib import sl4a_ports
from acts.controllers.sl4a_lib import sl4a_session
from acts.controllers.sl4a_lib import rpc_client


class Sl4aSessionTest(unittest.TestCase):
    """Tests the sl4a_session.Sl4aSession class."""

    def test_is_alive_true_on_not_terminated(self):
        """Tests sl4a_session.Sl4aSession.is_alive.

        Tests that the session is_alive when it has not been terminated.
        """
        session = mock.Mock()
        session._terminated = False
        session.is_alive = sl4a_session.Sl4aSession.is_alive
        self.assertNotEqual(session._terminated, session.is_alive)

    def test_is_alive_false_on_terminated(self):
        """Tests sl4a_session.Sl4aSession.is_alive.

        Tests that the session is_alive when it has not been terminated.
        """
        session = mock.Mock()
        session._terminated = True
        session.is_alive = sl4a_session.Sl4aSession.is_alive
        self.assertNotEqual(session._terminated, session.is_alive)

    @patch('acts.controllers.sl4a_lib.event_dispatcher.EventDispatcher')
    def test_get_event_dispatcher_create_on_none(self, _):
        """Tests sl4a_session.Sl4aSession.get_event_dispatcher.

        Tests that a new event_dispatcher is created if one does not exist.
        """
        session = mock.Mock()
        session._event_dispatcher = None
        ed = sl4a_session.Sl4aSession.get_event_dispatcher(session)
        self.assertTrue(session._event_dispatcher is not None)
        self.assertEqual(session._event_dispatcher, ed)

    def test_get_event_dispatcher_returns_existing_event_dispatcher(self):
        """Tests sl4a_session.Sl4aSession.get_event_dispatcher.

        Tests that the existing event_dispatcher is returned.
        """
        session = mock.Mock()
        session._event_dispatcher = 'Something that is not None'
        ed = sl4a_session.Sl4aSession.get_event_dispatcher(session)
        self.assertEqual(session._event_dispatcher, ed)

    def test_create_client_side_connection_hint_already_in_use(self):
        """Tests sl4a_session.Sl4aSession._create_client_side_connection().

        Tests that if the hinted port is already in use, the function will
        call itself with a hinted port of 0 (random).
        """
        session = mock.Mock()
        session._create_client_side_connection = mock.Mock()
        with mock.patch('socket.socket') as socket:
            # Throw an error when trying to bind to the hinted port.
            error = OSError()
            error.errno = errno.EADDRINUSE
            socket_instance = mock.Mock()
            socket_instance.bind = mock.Mock()
            socket_instance.bind.side_effect = error
            socket.return_value = socket_instance

            sl4a_session.Sl4aSession._create_client_side_connection(
                session, sl4a_ports.Sl4aPorts(1, 2, 3))

        fn = session._create_client_side_connection
        self.assertEqual(fn.call_count, 1)
        # Asserts that the 1st argument (Sl4aPorts) sent to the function
        # has a client port of 0.
        self.assertEqual(fn.call_args_list[0][0][0].client_port, 0)

    def test_create_client_side_connection_catches_timeout(self):
        """Tests sl4a_session.Sl4aSession._create_client_side_connection().

        Tests that the function will raise an Sl4aConnectionError upon timeout.
        """
        session = mock.Mock()
        session._create_client_side_connection = mock.Mock()
        error = timeout()
        with mock.patch('socket.socket') as socket:
            # Throw an error when trying to bind to the hinted port.
            socket_instance = mock.Mock()
            socket_instance.connect = mock.Mock()
            socket_instance.connect.side_effect = error
            socket.return_value = socket_instance

            with self.assertRaises(rpc_client.Sl4aConnectionError):
                sl4a_session.Sl4aSession._create_client_side_connection(
                    session, sl4a_ports.Sl4aPorts(0, 2, 3))

    def test_create_client_side_connection_hint_taken_during_fn(self):
        """Tests sl4a_session.Sl4aSession._create_client_side_connection().

        Tests that the function will call catch an EADDRNOTAVAIL OSError and
        call itself again, this time with a hinted port of 0 (random).
        """
        session = mock.Mock()
        session._create_client_side_connection = mock.Mock()
        error = socket_error()
        error.errno = errno.EADDRNOTAVAIL
        with mock.patch('socket.socket') as socket:
            # Throw an error when trying to bind to the hinted port.
            socket_instance = mock.Mock()
            socket_instance.connect = mock.Mock()
            socket_instance.connect.side_effect = error
            socket.return_value = socket_instance

            sl4a_session.Sl4aSession._create_client_side_connection(
                session, sl4a_ports.Sl4aPorts(0, 2, 3))

        fn = session._create_client_side_connection
        self.assertEqual(fn.call_count, 1)
        # Asserts that the 1st argument (Sl4aPorts) sent to the function
        # has a client port of 0.
        self.assertEqual(fn.call_args_list[0][0][0].client_port, 0)

    def test_create_client_side_connection_re_raises_uncaught_errors(self):
        """Tests sl4a_session.Sl4aSession._create_client_side_connection().

        Tests that the function will re-raise any socket error that does not
        have errno.EADDRNOTAVAIL.
        """
        session = mock.Mock()
        session._create_client_side_connection = mock.Mock()
        error = socket_error()
        # Some error that isn't EADDRNOTAVAIL
        error.errno = errno.ESOCKTNOSUPPORT
        with mock.patch('socket.socket') as socket:
            # Throw an error when trying to bind to the hinted port.
            socket_instance = mock.Mock()
            socket_instance.connect = mock.Mock()
            socket_instance.connect.side_effect = error
            socket.return_value = socket_instance

            with self.assertRaises(socket_error):
                sl4a_session.Sl4aSession._create_client_side_connection(
                    session, sl4a_ports.Sl4aPorts(0, 2, 3))

    def test_terminate_only_closes_if_not_terminated(self):
        """Tests sl4a_session.Sl4aSession.terminate()

        Tests that terminate only runs termination steps if the session has not
        already been terminated.
        """
        session = mock.Mock()
        session._terminate_lock = mock.MagicMock()
        session._terminated = True
        sl4a_session.Sl4aSession.terminate(session)

        self.assertFalse(session._event_dispatcher.close.called)
        self.assertFalse(session.rpc_client.terminate.called)

    def test_terminate_closes_session_first(self):
        """Tests sl4a_session.Sl4aSession.terminate()

        Tests that terminate only runs termination steps if the session has not
        already been terminated.
        """
        session = mock.Mock()
        session._terminate_lock = mock.MagicMock()
        session._terminated = True
        sl4a_session.Sl4aSession.terminate(session)

        self.assertFalse(session._event_dispatcher.close.called)
        self.assertFalse(session.rpc_client.terminate.called)


if __name__ == '__main__':
    unittest.main()
