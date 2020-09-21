#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
import acts.test_utils.power.PowerBaseTest as PBT
from acts.test_utils.bt.bt_test_utils import enable_bluetooth
from acts.test_utils.bt.bt_test_utils import disable_bluetooth

BT_BASE_UUID = '00000000-0000-1000-8000-00805F9B34FB'
BT_CLASSICAL_DATA = [1, 2, 3]
BLE_LOCATION_SCAN_ENABLE = 'settings put global ble_scan_always_enabled 1'
BLE_LOCATION_SCAN_DISABLE = 'settings put global ble_scan_always_enabled 0'
START_PMC_CMD = 'am start -n com.android.pmc/com.android.pmc.PMCMainActivity'
PMC_VERBOSE_CMD = 'setprop log.tag.PMC VERBOSE'
PMC_BASE_SCAN = 'am broadcast -a com.android.pmc.BLESCAN --es ScanMode '


class PowerBTBaseTest(PBT.PowerBaseTest):
    """Base class for BT power related tests.

    Inherited from the PowerBaseTest class
    """

    def setup_test(self):

        super().setup_test()
        # Reset BT to factory defaults
        self.dut.droid.bluetoothFactoryReset()
        time.sleep(2)
        # Start PMC app.
        self.log.info('Start PMC app...')
        self.dut.adb.shell(START_PMC_CMD)
        self.dut.adb.shell(PMC_VERBOSE_CMD)

    def teardown_test(self):
        """Tear down necessary objects after test case is finished.

        Bring down the AP interface, delete the bridge interface, stop the
        packet sender, and reset the ethernet interface for the packet sender
        """
        super().teardown_test()
        self.dut.droid.bluetoothFactoryReset()
        self.dut.adb.shell(BLE_LOCATION_SCAN_DISABLE)

    def teardown_class(self):
        """Clean up the test class after tests finish running

        """
        super().teardown_class()
        self.dut.droid.bluetoothFactoryReset()

    def phone_setup_for_BT(self, bt_on, ble_on, screen_status):
        """Sets the phone and Bluetooth in the desired state

        Args:
            bt_on: Enable/Disable BT
            ble_on: Enable/Disable BLE
            screen_status: screen ON or OFF
        """

        # Check if we are enabling a background scan
        # TODO: Turn OFF cellular wihtout having to turn ON airplane mode
        if bt_on == 'OFF' and ble_on == 'ON':
            self.dut.adb.shell(BLE_LOCATION_SCAN_ENABLE)
            self.dut.droid.connectivityToggleAirplaneMode(False)
            time.sleep(2)

        # Turn ON/OFF BT
        if bt_on == 'ON':
            enable_bluetooth(self.dut.droid, self.dut.ed)
            self.dut.log.info('BT is ON')
        else:
            disable_bluetooth(self.dut.droid)
            self.dut.droid.bluetoothDisableBLE()
            self.dut.log.info('BT is OFF')
        time.sleep(2)

        # Turn ON/OFF BLE
        if ble_on == 'ON':
            self.dut.droid.bluetoothEnableBLE()
            self.dut.log.info('BLE is ON')
        else:
            self.dut.droid.bluetoothDisableBLE()
            self.dut.log.info('BLE is OFF')
        time.sleep(2)

        # Set the desired screen status
        if screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
            self.dut.log.info('Screen is OFF')
        time.sleep(2)

    def start_pmc_ble_scan(self,
                           scan_mode,
                           offset_start,
                           scan_time,
                           idle_time=None,
                           num_reps=1):
        """Starts a generic BLE scan via the PMC app

        Args:
            dut: object of the android device under test
            scan mode: desired BLE scan type
            offset_start: Time delay in seconds before scan starts
            scan_time: active scan time
            idle_time: iddle time (i.e., no scans occuring)
            num_reps: Number of repetions of the ative+idle scan sequence
        """
        scan_dur = scan_time
        if not idle_time:
            idle_time = 0.2 * scan_time
            scan_dur = 0.8 * scan_time

        first_part_msg = '%s%s --es StartTime %d --es ScanTime %d' % (
            PMC_BASE_SCAN, scan_mode, offset_start, scan_dur)

        msg = '%s --es NoScanTime %d --es Repetitions %d' % (first_part_msg,
                                                             idle_time,
                                                             num_reps)

        self.dut.log.info('Sent BLE scan broadcast message: %s', msg)
        self.dut.adb.shell(msg)
