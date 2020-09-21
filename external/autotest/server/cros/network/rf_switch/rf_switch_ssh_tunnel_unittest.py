# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/network/rf_switch/rf_switch_ssh_tunnel.py."""

import unittest
import socket

from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.server.cros.network.rf_switch import rf_switch
from autotest_lib.server.cros.network.rf_switch import rf_switch_ssh_tunnel
from autotest_lib.server.cros.network.rf_switch import scpi_ssh_tunnel


class RfSwitchSshTunnelTestCases(unittest.TestCase):
    """Unit tests for RfSwitchSshTunnel."""

    _HOST = '1.2.3.4'
    _PROXY_HOST = '4.3.2.1'
    _PROXY_USER = 'user'
    _PROXY_PASSWORD = 'VerySecret'

    def setUp(self):
        self.god = mock.mock_god(debug=False)
        self.god.stub_class(socket, 'socket')

    def tearDown(self):
        self.god.unstub_all()

    def testCorrectParentInitializerCalled(self):
        """RFSwitchSshTunnel calls correct parent."""
        self.god.stub_class_method(scpi_ssh_tunnel.ScpiSshTunnel, '__init__')
        self.god.stub_class_method(rf_switch.RfSwitch, '__init__')
        scpi_ssh_tunnel.ScpiSshTunnel.__init__.expect_any_call()
        rf_switch_ssh_tunnel.RfSwitchSshTunnel(self._HOST,
                                               self._PROXY_HOST,
                                               self._PROXY_USER,
                                               self._PROXY_PASSWORD)
        self.god.check_playback()
        self.assertEqual(0, rf_switch.RfSwitch.__init__.num_calls,
                         'RfSwitch Initializer was called.')

if __name__ == '__main__':
    unittest.main()
