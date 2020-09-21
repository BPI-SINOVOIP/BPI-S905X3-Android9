# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import random

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.cfm import cfm_base_test
from autotest_lib.client.common_lib.cros import power_cycle_usb_util
from autotest_lib.client.common_lib.cros.cfm.usb import cfm_usb_devices
from autotest_lib.client.common_lib.cros.cfm.usb import usb_device_collector


LONG_TIMEOUT = 20
SHORT_TIMEOUT = 5

class enterprise_CFM_MimoSanity(cfm_base_test.CfmBaseTest):
    """Tests the following fuctionality works on CFM enrolled devices:
           1. Verify CfM has Camera, Speaker and Mimo connected.
           2. Verify all peripherals have expected usb interfaces.
           3. Verify after rebooting CfM Mimo is present.
           4. Verify after powercycle Mimo Mimo comes back.
    """
    version = 1


    def _power_cycle_mimo_device(self):
        """Power Cycle Mimo device"""
        logging.info('Plan to power cycle Mimo')
        try:
            power_cycle_usb_util.power_cycle_usb_vidpid(
                self._host, self._board,
                self._mimo.vendor_id, self._mimo.product_id)
        except KeyError:
           raise error.TestFail('Could not find target device: %s',
                                self._mimo.product)


    def _test_power_cycle_mimo(self):
        """Power Cycle Mimo device for multiple times"""
        self._power_cycle_mimo_device()
        logging.info('Powercycle done for %s (%s)',
                     self._mimo.product, self._mimo.vid_pid)
        time.sleep(LONG_TIMEOUT)
        self._kernel_usb_sanity_test()


    def _check_peripherals(self):
        """
        Check CfM has camera, speaker and MiMO connected.
        @returns list of peripherals found.
        """
        atruses = self.device_collector.get_devices_by_spec(
                cfm_usb_devices.ATRUS)
        if not atruses:
            raise error.TestFail('Expected to find connected speakers.')
        self._atrus = atruses[0]

        huddlys = self.device_collector.get_devices_by_spec(
                cfm_usb_devices.HUDDLY_GO)
        if not huddlys:
            raise error.TestFail('Expected to find a connected camera.')
        self._huddly = huddlys[0]


        displays = self.device_collector.get_devices_by_spec(
                *cfm_usb_devices.ALL_MIMO_DISPLAYS)
        if not displays:
            raise error.TestFail('Expected a MiMO display to be connected.')
        if len(displays) != 1:
            raise error.TestFail('Expected exactly one MiMO display to be '
                                 'connected. Found %d' % len(displays))
        self._mimo = displays[0]


        controllers = self.device_collector.get_devices_by_spec(
            cfm_usb_devices.MIMO_VUE_HID_TOUCH_CONTROLLER)
        if not controllers:
            raise error.TestFail('Expected a MiMO controller to be connected.')
        if len(controllers) != 1:
            raise error.TestFail('Expected exactly one MiMO controller to be '
                                 'connected. Found %d' % len(controllers))
        self._touch_controller = controllers[0]

    def _check_device_interfaces_match_spec(self, spec):
        for device in self.device_collector.get_devices_by_spec(spec):
            if not device.interfaces_match_spec(spec):
                raise error.TestFail(
                    'Device %s has unexpected interfaces.'
                    'Expected: %s. Actual: %s' % (device, spec.interfaces,
                                                  spec.interfaces))

    def _kernel_usb_sanity_test(self):
        """
        Check connected camera, speaker and Mimo have expected usb interfaces.
        """
        self._check_device_interfaces_match_spec(self._atrus)
        self._check_device_interfaces_match_spec(self._huddly)
        self._check_device_interfaces_match_spec(self._mimo)
        self._check_device_interfaces_match_spec(self._touch_controller)

    def _test_reboot(self):
        """Reboot testing for Mimo."""

        boot_id = self._host.get_boot_id()
        self._host.reboot()
        self._host.wait_for_restart(old_boot_id=boot_id)
        self.cfm_facade.restart_chrome_for_cfm()
        time.sleep(SHORT_TIMEOUT)
        if self._is_meeting:
            self.cfm_facade.wait_for_meetings_telemetry_commands()
        else:
            self.cfm_facade.wait_for_hangouts_telemetry_commands()
        self._kernel_usb_sanity_test()


    def _test_mimo_in_call(self) :
        """
        Start a hangout session and end the session after random time.

        @raises error.TestFail if any of the checks fail.
        """
        logging.info('Joining meeting...')
        if self._is_meeting:
            self.cfm_facade.start_meeting_session()
        else:
            self.cfm_facade.start_new_hangout_session('mimo-sanity-test')
        time.sleep(random.randrange(SHORT_TIMEOUT, LONG_TIMEOUT))

        # Verify USB data in-call.
        self._kernel_usb_sanity_test()

        if self._is_meeting:
            self.cfm_facade.end_meeting_session()
        else:
            self.cfm_facade.end_hangout_session()
        logging.info('Session has ended.')

        # Verify USB devices after leaving the call.
        self._kernel_usb_sanity_test()
        time.sleep(SHORT_TIMEOUT)


    def run_once(self, repetitions, is_meeting):
        """
        Runs the test.

        @param repetitions: amount of reboot cycles to perform.
        """
        # Remove 'board:' prefix.
        self._board = self._host.get_board().split(':')[1]
        self._is_meeting = is_meeting

        self.device_collector = usb_device_collector.UsbDeviceCollector(
            self._host)
        self._check_peripherals()
        self._kernel_usb_sanity_test()

        if self._is_meeting:
            self.cfm_facade.wait_for_meetings_telemetry_commands()
        else:
            self.cfm_facade.wait_for_hangouts_telemetry_commands()

        for i in xrange(1, repetitions + 1):
            logging.info('Running test cycle %d/%d', i, repetitions)
            self._test_reboot()
            self._test_mimo_in_call()
            self._test_power_cycle_mimo()
