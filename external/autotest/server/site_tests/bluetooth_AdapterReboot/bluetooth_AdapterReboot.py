# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth adapter stress tests involving reboot.
First we test powering on the adapter, reboot the DUT, and make sure
the adapter is still powered on and in a working state.

Next we test powering off the adapter, reboot, and verify the adapter
is still powered off.
"""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests
from autotest_lib.server.cros.multimedia import bluetooth_le_facade_adapter


test_case_log = bluetooth_adapter_tests.test_case_log


class bluetooth_AdapterReboot(
        bluetooth_adapter_tests.BluetoothAdapterTests):
    """Server side bluetooth adapter reboot test."""

    # ---------------------------------------------------------------
    # Definitions of all test cases
    # ---------------------------------------------------------------

    @test_case_log
    def test_case_adapter_on_reboot(self):
        """Test Case: Power on - reboot"""
        self.test_power_on_adapter()
        self.test_bluetoothd_running()
        self.host.reboot()
        # NOTE: We need to recreate the bluetooth_facade after a reboot.
        self.bluetooth_facade = self.ble_adapter(self.host)
        self.test_bluetoothd_running()
        self.test_adapter_work_state()
        self.test_power_on_adapter()

    @test_case_log
    def test_case_adapter_off_reboot(self):
        """Test Case: Power on - reboot"""
        self.test_power_off_adapter()
        self.test_bluetoothd_running()
        self.host.reboot()
        # NOTE: We need to recreate the bluetooth_facade after a reboot.
        self.bluetooth_facade = self.ble_adapter(self.host)
        self.test_power_off_adapter()
        self.test_bluetoothd_running()


    def run_once(self, host, num_iterations=1):
        """Running Bluetooth adapter reboot autotest.

        @param host: device under test host.
        @param num_iterations: number of times to perform reboot tests.

        """
        self.host = host
        self.ble_adapter = bluetooth_le_facade_adapter.BluetoothLEFacadeRemoteAdapter
        self.bluetooth_facade = self.ble_adapter(self.host)

        for i in xrange(num_iterations):
            logging.debug('Starting loop #%d', i)
            self.test_case_adapter_on_reboot()
            self.test_case_adapter_off_reboot()

        if self.fails:
            raise error.TestFail(self.fails)
