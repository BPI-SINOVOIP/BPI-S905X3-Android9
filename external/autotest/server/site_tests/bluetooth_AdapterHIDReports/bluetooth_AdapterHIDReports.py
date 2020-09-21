# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth tests about sending bluetooth HID reports."""

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests
from autotest_lib.server.cros.multimedia import remote_facade_factory


class bluetooth_AdapterHIDReports(
        bluetooth_adapter_tests.BluetoothAdapterTests):
    """Server side bluetooth tests about sending bluetooth HID reports.

    This test tries to send HID reports to a DUT and verifies if the DUT
    could receive the reports correctly. For the time being, only bluetooth
    mouse events are tested. Bluetooth keyboard events will be supported
    later.
    """

    TEST_SLEEP_SECS = 5

    def _run_mouse_tests(self, device):
        """Run all bluetooth mouse reports tests.

        @param device: the bluetooth HID device.

        """
        self.test_mouse_left_click(device)
        self.test_mouse_right_click(device)
        self.test_mouse_move_in_x(device, 80)
        self.test_mouse_move_in_y(device, -50)
        self.test_mouse_move_in_xy(device, -60, 100)
        self.test_mouse_scroll_down(device, 70)
        self.test_mouse_scroll_up(device, 40)
        self.test_mouse_click_and_drag(device, 90, 30)


    def run_once(self, host, device_type, num_iterations=1, min_pass_count=1,
                 suspend_resume=False, reboot=False):
        """Running Bluetooth HID reports tests.

        @param host: the DUT, usually a chromebook
        @param device_type : the bluetooth HID device type, e.g., 'MOUSE'
        @param num_iterations: the number of rounds to execute the test
        @param min_pass_count: the minimal pass count to pass this test

        """
        self.host = host
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        self.bluetooth_facade = factory.create_bluetooth_hid_facade()
        self.input_facade = factory.create_input_facade()
        self.check_chameleon()

        pass_count = 0
        self.total_fails = {}
        for iteration in xrange(1, num_iterations + 1):
            self.fails = []

            # Get the bluetooth device object.
            device = self.get_device(device_type)

            # Reset the adapter and set it pairable.
            self.test_reset_on_adapter()
            self.test_pairable()

            # Let the adapter pair, and connect to the target device.
            time.sleep(self.TEST_SLEEP_SECS)
            self.test_discover_device(device.address)
            time.sleep(self.TEST_SLEEP_SECS)
            self.test_pairing(device.address, device.pin, trusted=True)
            time.sleep(self.TEST_SLEEP_SECS)
            self.test_connection_by_adapter(device.address)

            if suspend_resume:
                self.suspend_resume()

                time.sleep(self.TEST_SLEEP_SECS)
                self.test_device_is_paired(device.address)

                # After a suspend/resume, we should check if the device is
                # connected.
                # NOTE: After a suspend/resume, the RN42 kit remains connected.
                #       However, this is not expected behavior for all bluetooth
                #       peripherals.
                time.sleep(self.TEST_SLEEP_SECS)
                self.test_device_is_connected(device.address)

                time.sleep(self.TEST_SLEEP_SECS)
                self.test_device_name(device.address, device.name)

            if reboot:
                self.host.reboot()

                # NOTE: We need to recreate the bluetooth_facade after a reboot.
                self.bluetooth_facade = factory.create_bluetooth_hid_facade()
                self.input_facade = factory.create_input_facade()

                time.sleep(self.TEST_SLEEP_SECS)
                self.test_device_is_paired(device.address)

                time.sleep(self.TEST_SLEEP_SECS)
                self.test_connection_by_device(device)

                time.sleep(self.TEST_SLEEP_SECS)
                self.test_device_name(device.address, device.name)

            # Run tests about mouse reports.
            if device_type.endswith('MOUSE'):
                self._run_mouse_tests(device)

            # Disconnect the device, and remove the pairing.
            self.test_disconnection_by_adapter(device.address)
            self.test_remove_pairing(device.address)

            if bool(self.fails):
                self.total_fails['Round %d' % iteration] = self.fails
            else:
                pass_count += 1

            fail_count = iteration - pass_count
            logging.info('===  (pass = %d, fail = %d) / total %d  ===\n',
                         pass_count, fail_count, num_iterations)

        if pass_count < min_pass_count:
            raise error.TestFail(self.total_fails)
