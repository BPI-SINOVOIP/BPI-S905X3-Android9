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
import time

from acts import asserts
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel import tel_defines
from acts.test_utils.tel import tel_test_utils as tel_utils
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_AUTO
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

class WifiSoftApTest(WifiBaseTest):

    def setup_class(self):
        """It will setup the required dependencies from config file and configure
           the devices for softap mode testing.

        Returns:
            True if successfully configured the requirements for testing.
        """
        self.dut = self.android_devices[0]
        self.dut_client = self.android_devices[1]
        req_params = []
        opt_param = ["open_network"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)
        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()
        self.open_network = self.open_network[0]["2g"]
        # Do a simple version of init - mainly just sync the time and enable
        # verbose logging.  This test will fail if the DUT has a sim and cell
        # data is disabled.  We would also like to test with phones in less
        # constrained states (or add variations where we specifically
        # constrain).
        utils.require_sl4a((self.dut, self.dut_client))
        utils.sync_device_time(self.dut)
        utils.sync_device_time(self.dut_client)
        # Set country code explicitly to "US".
        self.dut.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
        self.dut_client.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
        # Enable verbose logging on the duts
        self.dut.droid.wifiEnableVerboseLogging(1)
        asserts.assert_equal(self.dut.droid.wifiGetVerboseLoggingLevel(), 1,
            "Failed to enable WiFi verbose logging on the softap dut.")
        self.dut_client.droid.wifiEnableVerboseLogging(1)
        asserts.assert_equal(self.dut_client.droid.wifiGetVerboseLoggingLevel(), 1,
            "Failed to enable WiFi verbose logging on the client dut.")

    def teardown_class(self):
        wutils.stop_wifi_tethering(self.dut)
        wutils.reset_wifi(self.dut)
        wutils.reset_wifi(self.dut_client)
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut_client.take_bug_report(test_name, begin_time)

    """ Helper Functions """
    def create_softap_config(self):
        """Create a softap config with ssid and password."""
        ap_ssid = "softap_" + utils.rand_ascii_str(8)
        ap_password = utils.rand_ascii_str(8)
        self.dut.log.info("softap setup: %s %s", ap_ssid, ap_password)
        config = {wutils.WifiEnums.SSID_KEY: ap_ssid}
        config[wutils.WifiEnums.PWD_KEY] = ap_password
        return config

    def confirm_softap_in_scan_results(self, ap_ssid):
        """Confirm the ap started by wifi tethering is seen in scan results.

        Args:
            ap_ssid: SSID of the ap we are looking for.
        """
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut_client, ap_ssid);

    def confirm_softap_not_in_scan_results(self, ap_ssid):
        """Confirm the ap started by wifi tethering is not seen in scan results.

        Args:
            ap_ssid: SSID of the ap we are looking for.
        """
        wutils.start_wifi_connection_scan_and_ensure_network_not_found(
            self.dut_client, ap_ssid);

    def check_cell_data_and_enable(self):
        """Make sure that cell data is enabled if there is a sim present.

        If a sim is active, cell data needs to be enabled to allow provisioning
        checks through (when applicable).  This is done to relax hardware
        requirements on DUTs - without this check, running this set of tests
        after other wifi tests may cause failures.
        """
        # We do have a sim.  Make sure data is enabled so we can tether.
        if not self.dut.droid.telephonyIsDataEnabled():
            self.dut.log.info("need to enable data")
            self.dut.droid.telephonyToggleDataConnection(True)
            asserts.assert_true(self.dut.droid.telephonyIsDataEnabled(),
                                "Failed to enable cell data for softap dut.")

    def validate_full_tether_startup(self, band=None, hidden=None):
        """Test full startup of wifi tethering

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        initial_wifi_state = self.dut.droid.wifiCheckState()
        initial_cell_state = tel_utils.is_sim_ready(self.log, self.dut)
        self.dut.log.info("current state: %s", initial_wifi_state)
        self.dut.log.info("is sim ready? %s", initial_cell_state)
        if initial_cell_state:
            self.check_cell_data_and_enable()
        config = self.create_softap_config()
        wutils.start_wifi_tethering(self.dut,
                                    config[wutils.WifiEnums.SSID_KEY],
                                    config[wutils.WifiEnums.PWD_KEY], band, hidden)
        if hidden:
            # First ensure it's not seen in scan results.
            self.confirm_softap_not_in_scan_results(
                config[wutils.WifiEnums.SSID_KEY])
            # If the network is hidden, it should be saved on the client to be
            # seen in scan results.
            config[wutils.WifiEnums.HIDDEN_KEY] = True
            ret = self.dut_client.droid.wifiAddNetwork(config)
            asserts.assert_true(ret != -1, "Add network %r failed" % config)
            self.dut_client.droid.wifiEnableNetwork(ret, 0)
        self.confirm_softap_in_scan_results(config[wutils.WifiEnums.SSID_KEY])
        wutils.stop_wifi_tethering(self.dut)
        asserts.assert_false(self.dut.droid.wifiIsApEnabled(),
                             "SoftAp is still reported as running")
        if initial_wifi_state:
            wutils.wait_for_wifi_state(self.dut, True)
        elif self.dut.droid.wifiCheckState():
            asserts.fail("Wifi was disabled before softap and now it is enabled")

    """ Tests Begin """

    @test_tracker_info(uuid="495f1252-e440-461c-87a7-2c45f369e129")
    def test_check_wifi_tethering_supported(self):
        """Test check for wifi tethering support.

         1. Call method to check if wifi hotspot is supported
        """
        # TODO(silberst): wifiIsPortableHotspotSupported() is currently failing.
        # Remove the extra check and logging when b/30800811 is resolved
        hotspot_supported = self.dut.droid.wifiIsPortableHotspotSupported()
        tethering_supported = self.dut.droid.connectivityIsTetheringSupported()
        self.log.info(
            "IsPortableHotspotSupported: %s, IsTetheringSupported %s." % (
            hotspot_supported, tethering_supported))
        asserts.assert_true(hotspot_supported,
                            "DUT should support wifi tethering but is reporting false.")
        asserts.assert_true(tethering_supported,
                            "DUT should also support wifi tethering when called from ConnectivityManager")

    @test_tracker_info(uuid="09c19c35-c708-48a5-939b-ac2bbb403d54")
    def test_full_tether_startup(self):
        """Test full startup of wifi tethering in default band.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup()

    @test_tracker_info(uuid="6437727d-7db1-4f69-963e-f26a7797e47f")
    def test_full_tether_startup_2G(self):
        """Test full startup of wifi tethering in 2G band.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="970272fa-1302-429b-b261-51efb4dad779")
    def test_full_tether_startup_5G(self):
        """Test full startup of wifi tethering in 5G band.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="f76ed37a-519a-48b4-b260-ee3fc5a9cae0")
    def test_full_tether_startup_auto(self):
        """Test full startup of wifi tethering in auto-band.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_AUTO)

    @test_tracker_info(uuid="d26ee4df-5dcb-4191-829f-05a10b1218a7")
    def test_full_tether_startup_2G_hidden(self):
        """Test full startup of wifi tethering in 2G band using hidden AP.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_2G, True)

    @test_tracker_info(uuid="229cd585-a789-4c9a-8948-89fa72de9dd5")
    def test_full_tether_startup_5G_hidden(self):
        """Test full startup of wifi tethering in 5G band using hidden AP.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_5G, True)

    @test_tracker_info(uuid="d546a143-6047-4ffd-b3c6-5ec81a38001f")
    def test_full_tether_startup_auto_hidden(self):
        """Test full startup of wifi tethering in auto-band using hidden AP.

        1. Report current state.
        2. Switch to AP mode.
        3. verify SoftAP active.
        4. Shutdown wifi tethering.
        5. verify back to previous mode.
        """
        self.validate_full_tether_startup(WIFI_CONFIG_APBAND_AUTO, True)

    @test_tracker_info(uuid="b2f75330-bf33-4cdd-851a-de390f891ef7")
    def test_tether_startup_while_connected_to_a_network(self):
        """Test full startup of wifi tethering in auto-band while the device
        is connected to a network.

        1. Connect to an open network.
        2. Turn on AP mode (in auto band).
        3. Verify SoftAP active.
        4. Make a client connect to the AP.
        5. Shutdown wifi tethering.
        6. Ensure that the client disconnected.
        """
        wutils.wifi_toggle_state(self.dut, True)
        wutils.wifi_connect(self.dut, self.open_network)
        config = self.create_softap_config()
        wutils.start_wifi_tethering(self.dut,
                                    config[wutils.WifiEnums.SSID_KEY],
                                    config[wutils.WifiEnums.PWD_KEY],
                                    WIFI_CONFIG_APBAND_AUTO)
        asserts.assert_true(self.dut.droid.wifiIsApEnabled(),
                             "SoftAp is not reported as running")
        # local hotspot may not have internet connectivity
        wutils.wifi_connect(self.dut_client, config, check_connectivity=False)
        wutils.stop_wifi_tethering(self.dut)
        wutils.wait_for_disconnect(self.dut_client)

    """ Tests End """


if __name__ == "__main__":
    pass
