#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import queue
import socket
import threading
import unittest

from host_controller.tradefed import remote_client
from host_controller.tradefed import remote_operation


class MockRemoteManagerThread(threading.Thread):
    """A thread which mocks remote manager.

    Attributes:
        HOST: Local host name.
        PORT: The port that the remote manager listens to.
        _remote_mgr_socket: The remote manager socket.
        _response_queue: A queue.Queue object containing the response strings.
        _timeout: Socket timeout in seconds.
        last_error: The exception which caused this thread to terminate.
    """
    HOST = remote_client.LOCALHOST
    PORT = 32123

    def __init__(self, timeout):
        """Creates and listens to remote manager socket."""
        super(MockRemoteManagerThread, self).__init__()
        self._response_queue = queue.Queue()
        self._timeout = timeout
        self.last_error = None
        self._remote_mgr_socket = socket.socket()
        try:
            self._remote_mgr_socket.settimeout(self._timeout)
            self._remote_mgr_socket.bind((self.HOST, self.PORT))
            self._remote_mgr_socket.listen(1)
        except socket.error:
            self._remote_mgr_socket.close()

    def _Respond(self, response_str):
        """Accepts a client connection and responds.

        Args:
            response_str: The response string.
        """
        (server_socket, client_address) = self._remote_mgr_socket.accept()
        try:
            server_socket.settimeout(self._timeout)
            # Receive until connection is closed
            while not server_socket.recv(4096):
                pass
            server_socket.send(response_str)
        finally:
            server_socket.close()

    def AddResponse(self, response_str):
        """Add a response string to the queue.

        Args:
            response_str: The response string.
        """
        self._response_queue.put_nowait(response_str)

    def CloseSocket(self):
        """Closes the remote manager socket."""
        if self._remote_mgr_socket:
            self._remote_mgr_socket.close()
            self._remote_mgr_socket = None

    # @Override
    def run(self):
        """Sends the queued responses to the clients."""
        try:
            while True:
                response_str = self._response_queue.get()
                self._response_queue.task_done()
                if response_str is None:
                    break
                self._Respond(response_str)
        except socket.error as e:
            self.last_error = e
        finally:
            self.CloseSocket()


class RemoteClientTest(unittest.TestCase):
    """A test for remote_client.RemoteClient.

    Attributes:
        _remote_mgr_thread: An instance of MockRemoteManagerThread.
        _client: The remote_client.RemoteClient being tested.
    """

    def setUp(self):
        """Creates remote manager thread."""
        self._remote_mgr_thread = MockRemoteManagerThread(5)
        self._remote_mgr_thread.daemon = True
        self._remote_mgr_thread.start()
        self._client = remote_client.RemoteClient(self._remote_mgr_thread.HOST,
                                                  self._remote_mgr_thread.PORT,
                                                  5)

    def tearDown(self):
        """Terminates remote manager thread."""
        self._remote_mgr_thread.AddResponse(None)
        self._remote_mgr_thread.join(15)
        self._remote_mgr_thread.CloseSocket()
        self.assertFalse(self._remote_mgr_thread.is_alive(),
                         "Cannot stop remote manager thread.")
        if self._remote_mgr_thread.last_error:
            raise self._remote_mgr_thread.last_error

    def testListDevice(self):
        """Tests ListDevices operation."""
        self._remote_mgr_thread.AddResponse('{"serials": []}')
        self._client.ListDevices()

    def testAddCommand(self):
        """Tests AddCommand operation."""
        self._remote_mgr_thread.AddResponse('{}')
        self._client.SendOperation(remote_operation.AddCommand(0, "COMMAND"))

    def testMultipleOperations(self):
        """Tests sending multiple operations via one connection."""
        self._remote_mgr_thread.AddResponse('{}\n{}')
        self._client.SendOperations(remote_operation.ListDevices(),
                                    remote_operation.ListDevices())

    def testExecuteCommand(self):
        """Tests executing a command and waiting for result."""
        self._remote_mgr_thread.AddResponse('{}')
        self._client.SendOperation(remote_operation.AllocateDevice("serial123"))
        self._remote_mgr_thread.AddResponse('{}')
        self._client.SendOperation(remote_operation.ExecuteCommand(
                "serial123", "vts", "-m", "SampleShellTest"))

        self._remote_mgr_thread.AddResponse('{"status": "EXECUTING"}')
        result = self._client.WaitForCommandResult("serial123",
                                                   timeout=0.5, poll_interval=1)
        self.assertIsNone(result, "Client returns result before command finishes.")

        self._remote_mgr_thread.AddResponse('{"status": "EXECUTING"}')
        self._remote_mgr_thread.AddResponse('{"status": "INVOCATION_SUCCESS"}')
        result = self._client.WaitForCommandResult("serial123",
                                                   timeout=5, poll_interval=1)
        self._remote_mgr_thread.AddResponse('{}')
        self._client.SendOperation(remote_operation.FreeDevice("serial123"))
        self.assertIsNotNone(result, "Client doesn't return command result.")

    def testSocketError(self):
        """Tests raising exception when socket error occurs."""
        self.assertRaises(socket.timeout, self._client.ListDevices)
        self._remote_mgr_thread.AddResponse(None)
        self.assertRaises(socket.error, self._client.ListDevices)

    def testRemoteOperationException(self):
        """Tests raising exception when response is an error."""
        self._remote_mgr_thread.AddResponse('{"error": "unit test"}')
        self.assertRaises(remote_operation.RemoteOperationException,
                          self._client.ListDevices)


if __name__ == "__main__":
    unittest.main()
