# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Unit tests for server/cros/network/rf_switch/rf_switch.py."""

import unittest

from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.server.cros.network.rf_switch import rf_mocks
from autotest_lib.server.cros.network.rf_switch import rf_switch
from autotest_lib.server.cros.network.rf_switch import scpi


class RfSwitchTestCases(unittest.TestCase):
    """Unit tests for RfSwitch Methods."""

    _HOST = '1.2.3.4'

    def setUp(self):
        self.god = mock.mock_god(debug=False, fail_fast=True)
        self.mock_rf_switch = rf_mocks.RfSwitchMock(self._HOST)
        self.code = 0
        self.msg = ''

    def tearDown(self):
        self.god.unstub_all()

    def _populate_stack_for_cmd(self, cmd):
        """Helper to add call stacks to verify in mock.

        @param cmd: command to verify
        """
        # monitor send for correct command.
        self.god.stub_function(self.mock_rf_switch.socket, 'send')
        self.mock_rf_switch.socket.send.expect_call('%s' % cmd)
        self.mock_rf_switch.socket.send.expect_call(
            '%s\n' % scpi.Scpi.CMD_ERROR_CHECK)
        self.mock_rf_switch.socket.send.expect_call(
            '%s\n' % rf_switch.RfSwitch._CMD_WAIT)

        self.god.stub_function_to_return(
            self.mock_rf_switch.socket, 'recv', '%d, "%s"' % (
                self.code, self.msg))

    def _populate_stack_for_query_commands(self, cmd, response):
        """Helper to add call stacks for query commands

        @param cmd: command to verify
        @param response: response to mock return
        """
        self.god.stub_function(self.mock_rf_switch.socket, 'send')
        self.mock_rf_switch.socket.send.expect_call('%s' % cmd)

        self.god.stub_function_to_return(
            self.mock_rf_switch.socket, 'recv', response)

    def test_send_cmd_check_error(self):
        """Verify send_cmd_check_error."""
        test_command = 'This is a command\n'
        self._populate_stack_for_cmd(test_command)
        self.mock_rf_switch.send_cmd_check_error(test_command)
        self.god.check_playback()

    def test_send_cmd_check_error_throws_error(self):
        """Verify send_cmd_check_error throws error."""
        test_command = 'This is a command'
        code = 101
        msg = 'Error Message'

        self.god.stub_function_to_return(
            self.mock_rf_switch.socket, 'recv', '%d, "%s"' % (code, msg))
        with self.assertRaises(rf_switch.RfSwitchException):
            self.mock_rf_switch.send_cmd_check_error(test_command)

    def test_close_relays(self):
        """Verify close_relays."""
        relays = 'R1:R3'
        self._populate_stack_for_cmd(
            '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_CLOSE_RELAYS, relays))
        self.mock_rf_switch.close_relays(relays)
        self.god.check_playback()

    def test_relays_closed(self):
        """Verify relays_closed."""
        relays = 'R1:R3'
        status = '1,0,1'
        cmd = '%s (@%s)\n' % (
            rf_switch.RfSwitch._CMD_CHECK_RELAYS_CLOSED, relays)
        self._populate_stack_for_query_commands(cmd, status)
        self.mock_rf_switch.relays_closed(relays)
        self.god.check_playback()

    def test_open_relays(self):
        """Verify open_relays."""
        relays = 'R1:R3'
        self._populate_stack_for_cmd(
            '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_OPEN_RELAYS, relays))
        self.mock_rf_switch.open_relays(relays)
        self.god.check_playback()

    def test_open_all_relays(self):
        """Verify open_all_relays."""
        self._populate_stack_for_cmd(
            '%s\n' % rf_switch.RfSwitch._CMD_OPEN_ALL_RELAYS)
        self.mock_rf_switch.open_all_relays()
        self.god.check_playback()

    def test_set_verify_on(self):
        """Verify set_verify for on."""
        relays = 'R1:R3'
        status = True
        self._populate_stack_for_cmd(
            '%s 1,(@%s)\n' % (rf_switch.RfSwitch._CMD_SET_VERIFY, relays))
        self.mock_rf_switch.set_verify(relays, status)
        self.god.check_playback()

    def test_set_verify_off(self):
        """Verify set_verify for off."""
        relays = 'R1:R3'
        status = False
        self._populate_stack_for_cmd(
            '%s 0,(@%s)\n' % (rf_switch.RfSwitch._CMD_SET_VERIFY, relays))
        self.mock_rf_switch.set_verify(relays, status)
        self.god.check_playback()

    def test_get_verify(self):
        """Verify get_verify."""
        relays = 'R1:R3'
        status = '1,0,1'
        cmd = '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_GET_VERIFY, relays)
        self._populate_stack_for_query_commands(cmd, status)
        self.mock_rf_switch.get_verify(relays)
        self.god.check_playback()

    def test_set_verify_inverted_true(self):
        """Verify set_verify_inverted."""
        relays = 'R1:R3'
        status = True
        cmd = '%s INV,(@%s)\n' % (
            rf_switch.RfSwitch._CMD_SET_VERIFY_INVERTED, relays)
        self._populate_stack_for_cmd(cmd)
        self.mock_rf_switch.set_verify_inverted(relays, status)
        self.god.check_playback()

    def test_set_verify_inverted_false(self):
        """Verify set_verify_inverted."""
        relays = 'R1:R3'
        status = False
        cmd = '%s NORM,(@%s)\n' % (
            rf_switch.RfSwitch._CMD_SET_VERIFY_INVERTED, relays)
        self._populate_stack_for_cmd(cmd)
        self.mock_rf_switch.set_verify_inverted(relays, status)
        self.god.check_playback()

    def test_get_verify_inverted(self):
        """Verify get_verify_inverted."""
        relays = 'R1:R3'
        status = '1,0,1'
        cmd = '%s (@%s)\n' % (
            rf_switch.RfSwitch._CMD_GET_VERIFY_INVERTED, relays)
        self._populate_stack_for_query_commands(cmd, status)
        self.mock_rf_switch.get_verify_inverted(relays)
        self.god.check_playback()

    def test_get_verify_state(self):
        """Verify get_verify_state."""
        relays = 'R1:R3'
        status = '1,0,1'
        cmd = '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_GET_VERIFY_STATE, relays)
        self._populate_stack_for_query_commands(cmd, status)
        self.mock_rf_switch.get_verify_state(relays)
        self.god.check_playback()

    def test_busy(self):
        """Verify busy."""
        status = '1'
        cmd = '%s\n' % rf_switch.RfSwitch._CMD_CHECK_BUSY
        self._populate_stack_for_query_commands(cmd, status)
        busy = self.mock_rf_switch.busy
        self.god.check_playback()
        self.assertTrue(busy)

    def test_not_busy(self):
        """Verify busy."""
        status = '0'
        cmd = '%s\n' % rf_switch.RfSwitch._CMD_CHECK_BUSY
        self._populate_stack_for_query_commands(cmd, status)
        busy = self.mock_rf_switch.busy
        self.god.check_playback()
        self.assertFalse(busy)

    def test_wait(self):
        """Verify wait."""
        self.god.stub_function(self.mock_rf_switch.socket, 'send')
        self.mock_rf_switch.socket.send.expect_call(
            '%s\n' % rf_switch.RfSwitch._CMD_WAIT)
        self.mock_rf_switch.wait()
        self.god.check_playback()

    def test_get_attenuation(self):
        """Verify get_attenuation."""
        ap = 1
        attenuation = 100
        relays = rf_switch.RfSwitch._AP_ATTENUATOR_RELAYS_SHORT[ap]
        return_relays = '1,1,0,1,1,0,0'
        cmd = '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_CHECK_RELAYS_CLOSED,
                              relays)
        self._populate_stack_for_query_commands(cmd, return_relays)
        response = self.mock_rf_switch.get_attenuation(ap)
        self.god.check_playback()
        self.assertEquals(response, attenuation,
                          'get_attenuation returned %s, did not match %s' % (
                              response, attenuation))

    def test_set_attenuation(self):
        """Verify set_attenuation."""
        ap = 1
        ap_relays = 'k1_49,k1_50,k1_51,k1_52,k1_53,k1_54,R1_9'
        attenuation = 100
        relays_for_attenuation = 'k1_53,k1_52,k1_50,k1_49'

        self._populate_stack_for_cmd(
            '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_OPEN_RELAYS, ap_relays))
        self._populate_stack_for_cmd('%s (@%s)\n' % (
            rf_switch.RfSwitch._CMD_CLOSE_RELAYS, relays_for_attenuation))
        self.mock_rf_switch.set_attenuation(ap, attenuation)
        self.god.check_playback()

    def test_set_attenuation_all(self):
        """Verify we can set same attenuation to all."""

        # 0 should close all relays
        for x in xrange(rf_switch.RfSwitch._MAX_ENCLOSURE):
            relays = ','.join(rf_switch.RfSwitch._AP_ATTENUATOR_RELAYS[x + 1])
            reverse_relays = ','.join(
                rf_switch.RfSwitch._AP_ATTENUATOR_RELAYS[x + 1][::-1])
            self._populate_stack_for_cmd(
                '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_OPEN_RELAYS, relays))
            self._populate_stack_for_cmd('%s (@%s)\n' % (
                rf_switch.RfSwitch._CMD_CLOSE_RELAYS, reverse_relays))
        self.mock_rf_switch.set_attenuation(0, 0)

        # 127 should open all (close none)
        for x in xrange(rf_switch.RfSwitch._MAX_ENCLOSURE):
            relays = ','.join(rf_switch.RfSwitch._AP_ATTENUATOR_RELAYS[x + 1])
            self._populate_stack_for_cmd(
                '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_OPEN_RELAYS, relays))
            self._populate_stack_for_cmd('%s (@)\n' % (
                rf_switch.RfSwitch._CMD_CLOSE_RELAYS))
        self.mock_rf_switch.set_attenuation(0, 127)
        self.god.check_playback()

    def test_set_attenuation_raise_error(self):
        """Verify set_attenuation will raise error."""
        with self.assertRaises(ValueError):
            self.mock_rf_switch.set_attenuation(5, 100)
        with self.assertRaises(ValueError):
            self.mock_rf_switch.set_attenuation(-1, 100)

    def test_connect_ap_client(self):
        """Verify connect_ap_client sets correct relays."""
        relays = 'k1_1,k1_25'
        ap = 1
        client = 1
        self._populate_stack_for_cmd(
            '%s (@%s)\n' % (rf_switch.RfSwitch._CMD_CLOSE_RELAYS, relays))
        self.mock_rf_switch.connect_ap_client(ap, client)
        self.god.check_playback()

if __name__ == '__main__':
    unittest.main()
