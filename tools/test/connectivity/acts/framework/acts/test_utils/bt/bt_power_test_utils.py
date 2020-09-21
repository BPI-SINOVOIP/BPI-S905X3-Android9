#!/usr/bin/env python3.4
#
#   Copyright 2017 Google, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time

from acts.test_utils.wifi import wifi_power_test_utils as wputils
from acts.test_utils.bt.bt_test_utils import enable_bluetooth
from acts.test_utils.bt.bt_test_utils import disable_bluetooth

BT_BASE_UUID = '00000000-0000-1000-8000-00805F9B34FB'
BT_CLASSICAL_DATA = [1, 2, 3]
BLE_LOCATION_SCAN_ENABLE = 'settings put global ble_scan_always_enabled 1'
BLE_LOCATION_SCAN_DISABLE = 'settings put global ble_scan_always_enabled 0'
START_PMC_CMD = 'am start -n com.android.pmc/com.android.pmc.PMCMainActivity'
PMC_VERBOSE_CMD = 'setprop log.tag.PMC VERBOSE'
PMC_BASE_SCAN = 'am broadcast -a com.android.pmc.BLESCAN --es ScanMode '


def phone_setup_for_BT(dut, bt_on, ble_on, screen_status):
    """Sets the phone and Bluetooth in the desired state

    Args:
        dut: object of the android device under test
        bt_on: Enable/Disable BT
        ble_on: Enable/Disable BLE
        screen_status: screen ON or OFF
    """
    # Initialize the dut to rock-bottom state
    wputils.dut_rockbottom(dut)
    time.sleep(2)

    # Check if we are enabling a background scan
    # TODO: Turn OFF cellular wihtout having to turn ON airplane mode
    if bt_on == 'OFF' and ble_on == 'ON':
        dut.adb.shell(BLE_LOCATION_SCAN_ENABLE)
        dut.droid.connectivityToggleAirplaneMode(False)
        time.sleep(2)

    # Turn ON/OFF BT
    if bt_on == 'ON':
        enable_bluetooth(dut.droid, dut.ed)
        dut.log.info('BT is ON')
    else:
        disable_bluetooth(dut.droid)
        dut.droid.bluetoothDisableBLE()
        dut.log.info('BT is OFF')
    time.sleep(2)

    # Turn ON/OFF BLE
    if ble_on == 'ON':
        dut.droid.bluetoothEnableBLE()
        dut.log.info('BLE is ON')
    else:
        dut.droid.bluetoothDisableBLE()
        dut.log.info('BLE is OFF')
    time.sleep(2)

    # Set the desired screen status
    if screen_status == 'OFF':
        dut.droid.goToSleepNow()
        dut.log.info('Screen is OFF')
    time.sleep(2)


def start_pmc_ble_scan(dut,
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
                                                         idle_time, num_reps)

    dut.log.info('Sent BLE scan broadcast message: %s', msg)
    dut.adb.shell(msg)
