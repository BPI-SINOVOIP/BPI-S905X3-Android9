# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interface for SCPI Protocol.

Helper module to communicate with devices that uses SCPI protocol.

https://en.wikipedia.org/wiki/Standard_Commands_for_Programmable_Instruments

This will be used by RF Switch that was designed to connect WiFi AP and
WiFi Clients RF enclosures for interoperability testing.

"""

import logging
import socket
import sys


class ScpiException(Exception):
    """Exception for SCPI Errors."""

    def __init__(self, msg=None, cause=None):
        messages = []
        if msg:
            messages.append(msg)
        if cause:
            messages.append('Wrapping exception: %s: %s' % (
                type(cause).__name__, str(cause)))
        super(ScpiException, self).__init__(', '.join(messages))


class Scpi(object):
    """Controller for devices using SCPI protocol."""

    SCPI_PORT = 5025
    DEFAULT_READ_LEN = 4096

    CMD_IDENTITY = '*IDN?'
    CMD_RESET = '*RST'
    CMD_STATUS = '*STB?'
    CMD_ERROR_CHECK = 'SYST:ERR?'

    def __init__(self, host, port=SCPI_PORT):
        """
        Controller for devices using SCPI protocol.

        @param host: hostname or IP address of device using SCPI protocol
        @param port: Int SCPI port number (default 5025)

        @raises SCPIException: on error connecting to device

        """
        self.host = host
        self.port = port

        # Open a socket connection for communication with chassis.
        try:
            self.socket = socket.socket()
            self.socket.connect((host, port))
        except (socket.error, socket.timeout) as e:
            logging.error('Error connecting to SCPI device.')
            raise ScpiException(cause=e), None, sys.exc_info()[2]

    def close(self):
        """Close the connection."""
        if hasattr(self, 'socket'):
            self.socket.close()
            del self.socket

    def write(self, data):
        """Send data to socket.

        @param data: Data to send

        @returns number of bytes sent

        """
        return self.socket.send(data)

    def read(self, buffer_size=DEFAULT_READ_LEN):
        """Safely read the query response.

        @param buffer_size: Int max data to read at once (default 4096)

        @returns String data read from the socket

        """
        return str(self.socket.recv(buffer_size))

    def query(self, data, buffer_size=DEFAULT_READ_LEN):
        """Send the query and get response.

        @param data: data (Query) to send
        @param buffer_size: Int max data to read at once (default 4096)

        @returns String data read from the socket

        """
        self.write(data)
        return self.read(buffer_size)

    def info(self):
        """Get Chassis Info.

        @returns dictionary information of Chassis

        """
        # Returns a comma separated text as below converted to dict.
        # 'VTI Instruments Corporation,EX7200-S-11539,138454,3.13.8\n'
        return dict(
            zip(('Manufacturer', 'Model', 'Serial', 'Version'),
                self.query('%s\n' % self.CMD_IDENTITY)
                .strip().split(',', 3)))

    def reset(self):
        """Reset the chassis.

        @returns number of bytes sent
        """
        return self.write('%s\n' % self.CMD_RESET)

    def status(self):
        """Get status of relays.

        @returns Int status of relays

        """
        return int(self.query('%s\n' % self.CMD_STATUS))

    def error_query(self):
        """Check for any error.

        @returns tuple of error code and error message

        """
        code, msg = self.query('%s\n' % self.CMD_ERROR_CHECK).split(', ')
        return int(code), msg.strip().strip('"')
