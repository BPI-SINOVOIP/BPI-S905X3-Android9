# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Set of Mocks and Stubs for test SCPI & RF Switch.

Implement a set of Mocks and Stubs which will be used in
unittest for SCPI and RF Switch.
"""

import socket

from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.server.cros.network.rf_switch import rf_switch
from autotest_lib.server.cros.network.rf_switch import scpi


class SocketMock(mock.mock_class):
    """Mock socket for scpi test."""

    def __init__(self, cls, name, *args, **kwargs):
        mock.mock_class.__init__(self, cls, name, *args, **kwargs)
        self.host = None
        self.port = None
        self.recv_buffer = ''
        self.send_data = ''

    def close(self):
        pass

    def connect(self, host, port):
        self.host = host
        self.port = port

    def send(self, data):
        self.send_data = data

    def recv(self, size):
        if len(self.recv_val) > size:
            return self.recv_val[:size]
        return self.recv_val


class ScpiMock(scpi.Scpi):
    """Mock object for Scpi."""

    def __init__(self, host='1.2.3.4', port=12345, test_interface=False):
        if test_interface:
            self.socket = SocketMock(socket, socket)
            self.socket.connect(host, port)
        else:
            scpi.Scpi.__init__(self, host, port)


class RfSwitchMock(rf_switch.RfSwitch):
    """Mock object for RfSwitch."""

    def __init__(self, host='1.2.3.4'):
        self.socket = SocketMock(socket, socket)
        self.socket.connect(host, scpi.Scpi.SCPI_PORT)
