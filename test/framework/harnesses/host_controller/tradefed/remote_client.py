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

import logging
import socket
import time

from host_controller.tradefed import remote_operation

LOCALHOST = "localhost"
DEFAULT_PORT = 30103


class RemoteClient(object):
    """The class for sending remote operations to TradeFed."""

    def __init__(self, host=LOCALHOST, port=DEFAULT_PORT, timeout=None):
        """Initializes the client of TradeFed remote manager.

        Args:
            _host: The host name of the remote manager.
            _port: The port of the remote manager.
            _timeout: The connect and receive timeout in seconds
        """
        self._host = host
        self._port = port
        self._timeout = timeout if timeout else socket.getdefaulttimeout()

    def SendOperations(self, *operations):
        """Sends a list of operations and waits for each response.

        Args:
            *operations: A list of remote_operation.RemoteOperation objects.

        Returns:
            A list of JSON objects.

        Raises:
            socket.error if fails to communicate with remote manager.
            remote_operation.RemoteOperationException if any operation fails or
            has no response.
        """
        op_socket = socket.create_connection((self._host, self._port),
                                             self._timeout)
        logging.info("Connect to %s:%d", self._host, self._port)
        try:
            if self._timeout is not None:
                op_socket.settimeout(self._timeout)
            ops_str = "\n".join(str(op) for op in operations)
            logging.info("Send: %s", ops_str)
            op_socket.send(ops_str)
            op_socket.shutdown(socket.SHUT_WR)
            resp_str = ""
            while True:
                buf = op_socket.recv(4096)
                if not buf:
                    break
                resp_str += buf
        finally:
            op_socket.close()
        logging.info("Receive: %s", resp_str)
        resp_lines = [x for x in resp_str.split("\n") if x]
        if len(resp_lines) != len(operations):
            raise remote_operation.RemoteOperationException(
                    "Unexpected number of responses: %d" % len(resp_lines))
        return [operations[i].ParseResponse(resp_lines[i])
                for i in range(len(resp_lines))]

    def SendOperation(self, operation):
        """Sends one operation and waits for its response.

        Args:
            operation: A remote_operation.RemoteOperation object.

        Returns:
            A JSON object.

        Raises:
            socket.error if fails to communicate with remote manager.
            remote_operation.RemoteOperationException if the operation fails or
            has no response.
        """
        return self.SendOperations(operation)[0]

    def ListDevices(self):
        """Sends ListDevices operation.

        Returns:
            A list of device_info.DeviceInfo which are the devices connected to
            the host.
        """
        json_obj = self.SendOperation(remote_operation.ListDevices())
        return remote_operation.ParseListDevicesResponse(json_obj)

    def WaitForCommandResult(self, serial, timeout, poll_interval=5):
        """Sends a series of operations to wait until a command finishes.

        Args:
            serial: The serial number of the device.
            timeout: A float, the timeout in seconds.
            poll_interval: A float, the interval of each GetLastCommandResult
                           operation in seconds.

        Returns:
            A JSON object which is the result of the command.
            None if timeout.

            Sample
            {'status': 'INVOCATION_SUCCESS',
             'free_device_state': 'AVAILABLE'}
        """
        deadline = time.time() + timeout
        get_result_op = remote_operation.GetLastCommandResult(serial)
        while True:
            result = self.SendOperation(get_result_op)
            if result["status"] != "EXECUTING":
                return result
            if time.time() > deadline:
                return None
            time.sleep(poll_interval)
            if time.time() > deadline:
                return None
