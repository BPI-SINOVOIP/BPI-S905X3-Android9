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
import socket
import socketserver
import threading

from vts.runners.host import errors
from vts.proto import AndroidSystemControlMessage_pb2 as SysMsg
from vts.proto import ComponentSpecificationMessage_pb2 as CompSpecMsg
from vts.utils.python.mirror import pb2py

_functions = dict()  # Dictionary to hold function pointers


class CallbackServerError(errors.VtsError):
    """Raised when an error occurs in VTS TCP server."""


class CallbackRequestHandler(socketserver.StreamRequestHandler):
    """The request handler class for our server."""

    def handle(self):
        """Receives requests from clients.

        When a callback happens on the target side, a request message is posted
        to the host side and is handled here. The message is parsed and the
        appropriate callback function on the host side is called.
        """
        header = self.rfile.readline().strip()
        try:
            len = int(header)
        except ValueError:
            if header:
                logging.exception("Unable to convert '%s' into an integer, which "
                                  "is required for reading the next message." %
                                  header)
                raise
            else:
                logging.error('CallbackRequestHandler received empty message header. Skipping...')
                return
        # Read the request message.
        received_data = self.rfile.read(len)
        logging.debug("Received callback message: %s", received_data)
        request_message = SysMsg.AndroidSystemCallbackRequestMessage()
        request_message.ParseFromString(received_data)
        logging.debug('Handling callback ID: %s', request_message.id)
        response_message = SysMsg.AndroidSystemCallbackResponseMessage()
        # Call the appropriate callback function and construct the response
        # message.
        if request_message.id in _functions:
            callback_args = []
            for arg in request_message.arg:
                callback_args.append(pb2py.Convert(arg))
            args = tuple(callback_args)
            _functions[request_message.id](*args)
            response_message.response_code = SysMsg.SUCCESS
        else:
            logging.error("Callback function ID %s is not registered!",
                          request_message.id)
            response_message.response_code = SysMsg.FAIL

        # send the response back to client
        message = response_message.SerializeToString()
        # self.request is the TCP socket connected to the client
        self.request.sendall(message)


class CallbackServer(object):
    """This class creates TCPServer in separate thread.

    Attributes:
        _server: an instance of socketserver.TCPServer.
        _port: this variable maintains the port number used in creating
               the server connection.
        _ip: variable to hold the IP Address of the host.
        _hostname: IP Address to which initial connection is made.
    """

    def __init__(self):
        self._server = None
        self._port = 0  # Port 0 means to select an arbitrary unused port
        self._ip = ""  # Used to store the IP address for the server
        self._hostname = "localhost"  # IP address to which initial connection is made

    def RegisterCallback(self, callback_func):
        """Registers a callback function.

        Args:
            callback_func: The function to register.

        Returns:
            string, Id of the registered callback function.

        Raises:
            CallbackServerError is raised if the func_id is already registered.
        """
        if self.GetCallbackId(callback_func):
            raise CallbackServerError("Function is already registered")
        id = 0
        if _functions:
            id = int(max(_functions, key=int)) + 1
        _functions[str(id)] = callback_func
        return str(id)

    def UnregisterCallback(self, func_id):
        """Removes a callback function from the registry.

        Args:
            func_id: The ID of the callback function to remove.

        Raises:
            CallbackServerError is raised if the func_id is not registered.
        """
        try:
            _functions.pop(func_id)
        except KeyError:
            raise CallbackServerError(
                "Can't remove function ID '%s', which is not registered." %
                func_id)

    def GetCallbackId(self, callback_func):
        """Get ID of the callback function.  Registers a callback function.

        Args:
            callback_func: The function to register.

        Returns:
            string, Id of the callback function if found, None otherwise.
        """
        return _functions.get(callback_func, None)

    def Start(self, port=0):
        """Starts the server.

        Args:
            port: integer, number of the port on which the server listens.
                  Default is 0, which means auto-select a port available.

        Returns:
            IP Address, port number

        Raises:
            CallbackServerError is raised if the server fails to start.
        """
        try:
            self._server = socketserver.TCPServer(
                (self._hostname, port), CallbackRequestHandler)
            self._ip, self._port = self._server.server_address

            # Start a thread with the server.
            # Each request will be handled in a child thread.
            server_thread = threading.Thread(target=self._server.serve_forever)
            server_thread.daemon = True
            server_thread.start()
            logging.info('TcpServer %s started (%s:%s)', server_thread.name,
                         self._ip, self._port)
            return self._ip, self._port
        except (RuntimeError, IOError, socket.error) as e:
            logging.exception(e)
            raise CallbackServerError(
                'Failed to start CallbackServer on (%s:%s).' %
                (self._hostname, port))

    def Stop(self):
        """Stops the server.
        """
        self._server.shutdown()
        self._server.server_close()

    @property
    def ip(self):
        return self._ip

    @property
    def port(self):
        return self._port
