# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Server side bluetooth adapter stress tests involving suspend resume.
First we test powering on the adapter, suspend/resume the DUT, and make sure
the adapter is still powered on and in a working state.

Next we test powering off the adapter, suspend/resume, and verify the adapter
is still powered off.
"""

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.bluetooth import bluetooth_adapter_tests
from autotest_lib.server.cros.multimedia import bluetooth_le_facade_adapter


test_case_log = bluetooth_adapter_tests.test_case_log


class bluetooth_AdapterSuspendResume(
        bluetooth_adapter_tests.BluetoothAdapterTests):
    """Server side bluetooth adapter suspend resume test."""

    # ---------------------------------------------------------------
    # Definitions of all test cases
    # ---------------------------------------------------------------

    @test_case_log
    def test_case_adapter_on_SR(self):
        """Test Case: Power on - SR"""
        self.test_power_on_adapter()
        self.test_bluetoothd_running()
        self.suspend_resume()
        self.test_bluetoothd_running()
        self.test_adapter_work_state()
        self.test_power_on_adapter()

    @test_case_log
    def test_case_adapter_off_SR(self):
        """Test Case: Power on - SR"""
        self.test_power_off_adapter()
        self.test_bluetoothd_running()
        self.suspend_resume()
        self.test_power_off_adapter()
        self.test_bluetoothd_running()


    def run_once(self, host, num_iterations=1):
        """Running Bluetooth adapter suspend resume autotest.

        @param host: device under test host.
        @param num_iterations: number of times to perform suspend resume tests.

        """
        self.host = host
        ble_adapter = bluetooth_le_facade_adapter.BluetoothLEFacadeRemoteAdapter
        self.bluetooth_le_facade = ble_adapter(self.host)
        self.bluetooth_facade = self.bluetooth_le_facade

        for i in xrange(num_iterations):
            logging.debug('Starting loop #%d', i)
            self.test_case_adapter_on_SR()
            self.test_case_adapter_off_SR()

        if self.fails:
            raise error.TestFail(self.fails)
