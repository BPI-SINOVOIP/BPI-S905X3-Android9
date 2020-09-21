#!/usr/bin/env python
#
#   Copyright 2017 - The Android Open Source Project
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
"""
Test pairing of an Android Device to a Sony XB2 Bluetooth speaker
"""
import logging
import time

from acts.controllers.relay_lib.relay import SynchronizeRelays
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest

log = logging


class SonyXB2PairingTest(BluetoothBaseTest):
    DISCOVERY_TIME = 5

    def __init__(self, controllers):
        BluetoothBaseTest.__init__(self, controllers)
        self.dut = self.android_devices[0]
        # Do factory reset and then do delay for 3-seconds
        self.dut.droid.bluetoothFactoryReset()
        time.sleep(3)
        self.sony_xb2_speaker = self.relay_devices[0]

    def setup_test(self):
        super(BluetoothBaseTest, self).setup_test()
        self.sony_xb2_speaker.setup()
        self.sony_xb2_speaker.power_on()
        # Wait for a moment between pushing buttons
        time.sleep(0.25)
        self.sony_xb2_speaker.enter_pairing_mode()

    def teardown_test(self):
        super(BluetoothBaseTest, self).teardown_test()
        self.sony_xb2_speaker.power_off()
        self.sony_xb2_speaker.clean_up()

    def _perform_classic_discovery(self, scan_time=DISCOVERY_TIME):
        self.dut.droid.bluetoothStartDiscovery()
        time.sleep(scan_time)
        self.dut.droid.bluetoothCancelDiscovery()
        return self.dut.droid.bluetoothGetDiscoveredDevices()

    @BluetoothBaseTest.bt_test_wrap
    def test_speaker_on(self):
        """Test if the Sony XB2 speaker is powered on.

        Use scanning to determine if the speaker is powered on.

        Steps:
        1. Put the speaker into pairing mode. (Hold the button)
        2. Perform a scan on the DUT
        3. Check the scan list for the device.

        Expected Result:
        Speaker is found.

        Returns:
          Pass if True
          Fail if False

        TAGS: ACTS_Relay
        Priority: 1
        """

        for device in self._perform_classic_discovery():
            if device['address'] == self.sony_xb2_speaker.mac_address:
                self.dut.log.info("Desired device with MAC address %s found!",
                                  self.sony_xb2_speaker.mac_address)
                return True
        return False

    @BluetoothBaseTest.bt_test_wrap
    def test_speaker_off(self):
        """Test if the Sony XB2 speaker is powered off.

        Use scanning to determine if the speaker is powered off.

        Steps:
        1. Power down the speaker
        2. Put the speaker into pairing mode. (Hold the button)
        3. Perform a scan on the DUT
        4. Check the scan list for the device.

        Expected Result:
        Speaker is not found.

        Returns:
          Pass if True
          Fail if False

        TAGS: ACTS_Relay
        Priority: 1
        """
        # Specific part of the test, turn off the speaker
        self.sony_xb2_speaker.power_off()

        device_not_found = True
        for device in self._perform_classic_discovery():
            if device['address'] == self.sony_xb2_speaker.mac_address:
                self.dut.log.info(
                    "Undesired device with MAC address %s found!",
                    self.sony_xb2_speaker.mac_address)
                device_not_found = False

        # Set the speaker back to the normal for tear_down()
        self.sony_xb2_speaker.power_on()
        # Give the relay and speaker some time, before it is turned off.
        time.sleep(5)
        return device_not_found

    @BluetoothBaseTest.bt_test_wrap
    def test_pairing(self):
        """Test pairing between a phone and Sony XB2 speaker.

        Test the Sony XB2 speaker can be paired to phone.

        Steps:
        1. Find the MAC address of remote controller from relay config file.
        2. Start the device paring process.
        3. Enable remote controller in pairing mode.
        4. Verify the remote is paired.

        Expected Result:
        Speaker is paired.

        Returns:
          Pass if True
          Fail if False

        TAGS: ACTS_Relay
        Priority: 1
        """

        # BT scan activity
        self._perform_classic_discovery()
        self.dut.droid.bluetoothDiscoverAndBond(
            self.sony_xb2_speaker.mac_address)

        end_time = time.time() + 20
        self.dut.log.info("Verifying devices are bonded")
        while (time.time() < end_time):
            bonded_devices = self.dut.droid.bluetoothGetBondedDevices()
            for d in bonded_devices:
                if d['address'] == self.sony_xb2_speaker.mac_address:
                    self.dut.log.info("Successfully bonded to device.")
                    self.log.info(
                        "Sony XB2 Bonded devices:\n{}".format(bonded_devices))
                    return True
        # Timed out trying to bond.
        self.dut.log.info("Failed to bond devices.")

        return False
