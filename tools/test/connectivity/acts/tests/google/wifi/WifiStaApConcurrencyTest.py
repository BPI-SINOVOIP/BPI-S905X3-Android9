#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
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

import itertools
import pprint
import queue
import time

import acts.base_test
import acts.signals as signals
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils as utils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums

# Channels to configure the AP for various test scenarios.
WIFI_NETWORK_AP_CHANNEL_2G = 1
WIFI_NETWORK_AP_CHANNEL_5G = 36
WIFI_NETWORK_AP_CHANNEL_5G_DFS = 132

class WifiStaApConcurrencyTest(WifiBaseTest):
    """Tests for STA + AP concurrency scenarions.

    Test Bed Requirement:
    * Two Android devices (For AP)
    * One Wi-Fi network visible to the device (for STA).
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        self.dut_client = self.android_devices[1]
        wutils.wifi_test_device_init(self.dut)
        wutils.wifi_test_device_init(self.dut_client)
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

        req_params = ["AccessPoint", "dbs_supported_models"]
        opt_param = ["iperf_server_address"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if self.dut.model not in self.dbs_supported_models:
            asserts.skip(
                ("Device %s does not support dual interfaces.")
                % self.dut.model)

        if "iperf_server_address" in self.user_params:
            self.iperf_server = self.iperf_servers[0]
        if hasattr(self, 'iperf_server'):
            self.iperf_server.start()

        # Set the client wifi state to on before the test begins.
        wutils.wifi_toggle_state(self.dut_client, True)

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        self.turn_location_off_and_scan_toggle_off()
        wutils.wifi_toggle_state(self.dut, False)

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.stop_wifi_tethering(self.dut)
        wutils.reset_wifi(self.dut)
        wutils.reset_wifi(self.dut_client)
        self.access_points[0].close()
        del self.user_params["reference_networks"]
        del self.user_params["open_network"]

    def teardown_class(self):
        if hasattr(self, 'iperf_server'):
            self.iperf_server.stop()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    """Helper Functions"""
    def configure_ap(self, channel_2g=None, channel_5g=None):
        """Configure and bring up AP on required channel.

        Args:
            channel_2g: The channel number to use for 2GHz network.
            channel_5g: The channel number to use for 5GHz network.

        """
        if not channel_2g:
            self.legacy_configure_ap_and_start(channel_5g=channel_5g)
        elif not channel_5g:
            self.legacy_configure_ap_and_start(channel_2g=channel_2g)
        else:
            self.legacy_configure_ap_and_start(channel_2g=channel_2g,
                channel_5g=chanel_5g)
        self.wpapsk_2g = self.reference_networks[0]["2g"]
        self.wpapsk_5g = self.reference_networks[0]["5g"]

    def turn_location_on_and_scan_toggle_on(self):
        """ Turns on wifi location scans.
        """
        acts.utils.set_location_service(self.dut, True)
        self.dut.droid.wifiScannerToggleAlwaysAvailable(True)
        msg = "Failed to turn on location service's scan."
        asserts.assert_true(self.dut.droid.wifiScannerIsAlwaysAvailable(), msg)

    def turn_location_off_and_scan_toggle_off(self):
        """ Turns off wifi location scans.
        """
        acts.utils.set_location_service(self.dut, False)
        self.dut.droid.wifiScannerToggleAlwaysAvailable(False)
        msg = "Failed to turn off location service's scan."
        asserts.assert_true(not self.dut.droid.wifiScannerIsAlwaysAvailable(), msg)

    def run_iperf_client(self, params):
        """Run iperf traffic after connection.

        Args:
            params: A tuple of network info and AndroidDevice object.
        """
        if "iperf_server_address" in self.user_params:
            wait_time = 5
            network, ad = params
            SSID = network[WifiEnums.SSID_KEY]
            self.log.info("Starting iperf traffic through {}".format(SSID))
            time.sleep(wait_time)
            port_arg = "-p {}".format(self.iperf_server.port)
            success, data = ad.run_iperf_client(self.iperf_server_address,
                                                port_arg)
            self.log.debug(pprint.pformat(data))
            asserts.assert_true(success, "Error occurred in iPerf traffic.")

    def connect_to_wifi_network_and_verify(self, params):
        """Connection logic for open and psk wifi networks.

        Args:
            params: A tuple of network info and AndroidDevice object.
        """
        network, ad = params
        droid = ad.droid
        ed = ad.ed
        SSID = network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            ad, SSID);
        wutils.wifi_connect(ad, network, num_of_tries=3)

    def confirm_softap_in_scan_results(self, ap_ssid):
        """Confirm the ap started by wifi tethering is seen in scan results.

        Args:
            ap_ssid: SSID of the ap we are looking for.
        """
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut_client, ap_ssid);

    def create_softap_config(self):
        """Create a softap config with ssid and password."""
        ap_ssid = "softap_" + utils.rand_ascii_str(8)
        ap_password = utils.rand_ascii_str(8)
        self.dut.log.info("softap setup: %s %s", ap_ssid, ap_password)
        config = {wutils.WifiEnums.SSID_KEY: ap_ssid}
        config[wutils.WifiEnums.PWD_KEY] = ap_password
        return config

    def start_softap_and_verify(self, band):
        """Test startup of softap

        1. Brinup AP mode.
        2. Verify SoftAP active using the client device.
        """
        config = self.create_softap_config()
        wutils.start_wifi_tethering(self.dut,
                                    config[wutils.WifiEnums.SSID_KEY],
                                    config[wutils.WifiEnums.PWD_KEY], band)
        self.confirm_softap_in_scan_results(config[wutils.WifiEnums.SSID_KEY])

    def connect_to_wifi_network_and_start_softap(self, nw_params, softap_band):
        """Test concurrenct wifi connection and softap.
        This helper method first makes a wifi conenction and then starts SoftAp.

        Args:
            nw_params: Params for network STA connection.
            softap_band: Band for the AP.

        1. Bring up wifi.
        2. Establish connection to a network.
        3. Bring up softap and verify AP is seen on a client device.
        4. Run iperf on the wifi connection to the network.
        """
        wutils.wifi_toggle_state(self.dut, True)
        self.connect_to_wifi_network_and_verify((nw_params, self.dut))
        self.start_softap_and_verify(softap_band)
        self.run_iperf_client((nw_params, self.dut))
        # Verify that both softap & wifi is enabled concurrently.
        self.verify_wifi_and_softap_enabled()

    def start_softap_and_connect_to_wifi_network(self, nw_params, softap_band):
        """Test concurrenct wifi connection and softap.
        This helper method first starts SoftAp and then makes a wifi conenction.

        Args:
            nw_params: Params for network STA connection.
            softap_band: Band for the AP.

        1. Bring up softap and verify AP is seen on a client device.
        2. Bring up wifi.
        3. Establish connection to a network.
        4. Run iperf on the wifi connection to the network.
        """
        self.start_softap_and_verify(softap_band)
        wutils.wifi_toggle_state(self.dut, True)
        self.connect_to_wifi_network_and_verify((nw_params, self.dut))
        self.run_iperf_client((nw_params, self.dut))
        # Verify that both softap & wifi is enabled concurrently.
        self.verify_wifi_and_softap_enabled()

    def verify_wifi_and_softap_enabled(self):
        """Helper to verify both wifi and softap is enabled
        """
        asserts.assert_true(self.dut.droid.wifiCheckState(),
                            "Wifi is not reported as running");
        asserts.assert_true(self.dut.droid.wifiIsApEnabled(),
                             "SoftAp is not reported as running")

    """Tests"""
    @test_tracker_info(uuid="c396e7ac-cf22-4736-a623-aa6d3c50193a")
    def test_wifi_connection_2G_softap_2G(self):
        """Tests connection to 2G network followed by bringing up SoftAp on 2G.
        """
        self.configure_ap(channel_2g=WIFI_NETWORK_AP_CHANNEL_2G)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_2g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="1cd6120d-3db4-4624-9bae-55c976533a48")
    def test_wifi_connection_5G_softap_5G(self):
        """Tests connection to 5G network followed by bringing up SoftAp on 5G.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="5f980007-3490-413e-b94e-7700ffab8534")
    def test_wifi_connection_5G_DFS_softap_5G(self):
        """Tests connection to 5G DFS network followed by bringing up SoftAp on 5G.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G_DFS)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="d05d5d44-c738-4372-9f01-ce2a640a2f25")
    def test_wifi_connection_5G_softap_2G(self):
        """Tests connection to 5G network followed by bringing up SoftAp on 2G.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="909ac713-1ad3-4dad-9be3-ad60f00ed25e")
    def test_wifi_connection_5G_DFS_softap_2G(self):
        """Tests connection to 5G DFS network followed by bringing up SoftAp on 2G.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G_DFS)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="e8de724a-25d3-4801-94cc-22e9e0ecc8d1")
    def test_wifi_connection_2G_softap_5G(self):
        """Tests connection to 2G network followed by bringing up SoftAp on 5G.
        """
        self.configure_ap(channel_2g=WIFI_NETWORK_AP_CHANNEL_2G)
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_2g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="647f4e17-5c7a-4249-98af-f791d163a39f")
    def test_wifi_connection_5G_softap_2G_with_location_scan_on(self):
        """Tests connection to 5G network followed by bringing up SoftAp on 2G
        with location scans turned on.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.turn_location_on_and_scan_toggle_on()
        self.connect_to_wifi_network_and_start_softap(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="4aa56c11-e5bc-480b-bd61-4b4ee577a5da")
    def test_softap_2G_wifi_connection_2G(self):
        """Tests bringing up SoftAp on 2G followed by connection to 2G network.
        """
        self.configure_ap(channel_2g=WIFI_NETWORK_AP_CHANNEL_2G)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_2g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="5f954957-ad20-4de1-b20c-6c97d0463bdd")
    def test_softap_5G_wifi_connection_5G(self):
        """Tests bringing up SoftAp on 5G followed by connection to 5G network.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="1306aafc-a07e-4654-ba78-674f90cf748e")
    def test_softap_5G_wifi_connection_5G_DFS(self):
        """Tests bringing up SoftAp on 5G followed by connection to 5G DFS network.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G_DFS)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="5e28e8b5-3faa-4cff-a782-13a796d7f572")
    def test_softap_5G_wifi_connection_2G(self):
        """Tests bringing up SoftAp on 5G followed by connection to 2G network.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_2G)

    @test_tracker_info(uuid="a2c62bc6-9ccd-4bc4-8a23-9a1b5d0b4b5c")
    def test_softap_2G_wifi_connection_5G(self):
        """Tests bringing up SoftAp on 2G followed by connection to 5G network.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="a2c62bc6-9ccd-4bc4-8a23-9a1b5d0b4b5c")
    def test_softap_2G_wifi_connection_5G_DFS(self):
        """Tests bringing up SoftAp on 2G followed by connection to 5G DFS network.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G_DFS)
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_5G)

    @test_tracker_info(uuid="aa23a3fc-31a1-4d5c-8cf5-2eb9fdf9e7ce")
    def test_softap_5G_wifi_connection_2G_with_location_scan_on(self):
        """Tests bringing up SoftAp on 5G followed by connection to 2G network
        with location scans turned on.
        """
        self.configure_ap(channel_5g=WIFI_NETWORK_AP_CHANNEL_5G)
        self.turn_location_on_and_scan_toggle_on()
        self.start_softap_and_connect_to_wifi_network(
            self.wpapsk_5g, WIFI_CONFIG_APBAND_2G)
