#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
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

import logging
import queue
import sys
import time

from acts import base_test
from acts import signals
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_constants
from acts.test_utils.wifi import wifi_test_utils as wutils


class WifiServiceApiTest(base_test.BaseTestClass):
    """This class tests the API surface of WifiManager in different wifi states.

       Attributes:
       The tests in this class only require one DUT.
       The tests in this class do not require a SIM (but it is ok if one is
           present).
    """


    TEST_SSID_PREFIX = "test_config_"
    CONFIG_ELEMENT = 'config'
    NETWORK_ID_ELEMENT = 'network_id'

    def setup_class(self):
        """ Sets up the required dependencies from the config file and
            configures the device for WifiService API tests.

            Returns:
            True is successfully configured the requirements for testig.
        """
        self.dut = self.android_devices[0]
        # Do a simple version of init - mainly just sync the time and enable
        # verbose logging.  We would also like to test with phones in less
        # constrained states (or add variations where we specifically
        # constrain).
        utils.require_sl4a((self.dut, ))
        utils.sync_device_time(self.dut)

        # Enable verbose logging on the dut
        self.dut.droid.wifiEnableVerboseLogging(1)
        if self.dut.droid.wifiGetVerboseLoggingLevel() != 1:
            raise signals.TestFailure(
                    "Failed to enable WiFi verbose logging on the dut.")

    def teardown_class(self):
        wutils.reset_wifi(self.dut)

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)

    def create_and_save_wifi_network_config(self):
        """ Create a config with random SSID and password.

            Returns:
            A tuple with the config and networkId for the newly created and saved network.
        """
        config_ssid = self.TEST_SSID_PREFIX + utils.rand_ascii_str(8)
        config_password = utils.rand_ascii_str(8)
        self.dut.log.info("creating config: %s %s", config_ssid, config_password)
        config = {wutils.WifiEnums.SSID_KEY: config_ssid}
        config[wutils.WifiEnums.PWD_KEY] = config_password

        # Now save the config.
        network_id = self.dut.droid.wifiAddNetwork(config)
        self.dut.log.info("saved config: network_id = %s", network_id)
        return {self.NETWORK_ID_ELEMENT: network_id, self.CONFIG_ELEMENT: config}

    def check_network_config_saved(self, config):
        """ Get the configured networks and check of the provided config
            is present.  This method only checks if the SSID is the same.
            TODO: should do a deeper check to make sure this is the
            correct config.

            Args:
                config: WifiConfig for a network.

            Returns:
                True if the WifiConfig is present.
        """
        networks = self.dut.droid.wifiGetConfiguredNetworks()
        if not networks:
            return False
        ssid_key = wutils.WifiEnums.SSID_KEY
        for network in networks:
            if config[ssid_key] == network[ssid_key]:
                return True
        return False

    def forget_network(self, network_id):
        """ Simple method to call wifiForgetNetwork and wait for confirmation
            callback.  The method returns False if it was not removed.

            Returns:
                True if network was successfully deleted.
        """
        self.dut.log.info("deleting config: networkId = %s", network_id)
        self.dut.droid.wifiForgetNetwork(network_id)
        try:
            event = self.dut.ed.pop_event(wifi_constants.WIFI_FORGET_NW_SUCCESS, 10)
            return True
        except queue.Empty:
            self.dut.log.error("Failed to forget network")
            return False


    """ Tests Begin """
    @test_tracker_info(uuid="f4df08c2-d3d5-4032-a433-c15f55130d4a")
    def test_remove_config_wifi_enabled(self):
        """ Test if config can be deleted when wifi is enabled.

            1. Enable wifi, if needed
            2. Create and save a random config.
            3. Confirm the config is present.
            4. Remove the config.
            5. Confirm the config is not listed.
        """
        wutils.wifi_toggle_state(self.dut, True)
        test_network = self.create_and_save_wifi_network_config()
        if not self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Test network not found in list of configured networks.")
        if not self.forget_network(test_network[self.NETWORK_ID_ELEMENT]):
            raise signals.TestFailure(
                    "Test network not deleted from configured networks.")
        if self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Deleted network was in configured networks list.")

    @test_tracker_info(uuid="9af96c7d-a316-4d57-ba5f-c992427c237b")
    def test_remove_config_wifi_disabled(self):
        """ Test if config can be deleted when wifi is disabled.

            1. Enable wifi, if needed
            2. Create and save a random config.
            3. Confirm the config is present.
            4. Disable wifi.
            5. Remove the config.
            6. Confirm the config is not listed.
        """
        wutils.wifi_toggle_state(self.dut, True)
        test_network = self.create_and_save_wifi_network_config()
        if not self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Test network not found in list of configured networks.")
        wutils.wifi_toggle_state(self.dut, False)
        if not self.forget_network(test_network[self.NETWORK_ID_ELEMENT]):
            raise signals.TestFailure("Failed to delete network.")
        if self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Test network was found in list of configured networks.")

    @test_tracker_info(uuid="79204ae6-323b-4257-a2cb-2225d44199d4")
    def test_retrieve_config_wifi_enabled(self):
        """ Test if config can be retrieved when wifi is enabled.

            1. Enable wifi
            2. Create and save a random config
            3. Retrieve the config
            4. Remove the config (clean up from the test)
        """
        wutils.wifi_toggle_state(self.dut, True)
        test_network = self.create_and_save_wifi_network_config()

        if not self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Test network not found in list of configured networks.")
        if not self.forget_network(test_network[self.NETWORK_ID_ELEMENT]):
            raise signals.TestFailure("Failed to delete network.")

    @test_tracker_info(uuid="58fb4f81-bc19-43e1-b0af-89dbd17f45b2")
    def test_retrieve_config_wifi_disabled(self):
        """ Test if config can be retrieved when wifi is disabled.

            1. Disable wifi
            2. Create and save a random config
            3. Retrieve the config
            4. Remove the config (clean up from the test)
        """
        wutils.wifi_toggle_state(self.dut, False)
        test_network = self.create_and_save_wifi_network_config()
        if not self.check_network_config_saved(test_network[self.CONFIG_ELEMENT]):
            raise signals.TestFailure(
                    "Test network not found in list of configured networks.")
        if not self.forget_network(test_network[self.NETWORK_ID_ELEMENT]):
            raise signals.TestFailure("Failed to delete network.")

    """ Tests End """


if __name__ == "__main__":
      pass
