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
import sys
import time

import acts.base_test
import acts.signals as signals
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils as utils

from acts import asserts
from acts.controllers.ap_lib import hostapd_constants
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_2G
from acts.test_utils.tel.tel_test_utils import WIFI_CONFIG_APBAND_5G
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest
from threading import Thread

WifiEnums = wutils.WifiEnums
WIFI_CONFIG_APBAND_AUTO = -1

class WifiSoftApAcsTest(WifiBaseTest):
    """Tests for Automatic Channel Selection.

    Test Bed Requirement:
    * Two Android devices and an AP.
    * 2GHz and 5GHz  Wi-Fi network visible to the device.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        self.dut_client = self.android_devices[1]
        wutils.wifi_test_device_init(self.dut)
        wutils.wifi_test_device_init(self.dut_client)
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
        req_params = []
        opt_param = ["iperf_server_address", "reference_networks"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)
        if "iperf_server_address" in self.user_params:
            self.iperf_server = self.iperf_servers[0]
        if hasattr(self, 'iperf_server'):
            self.iperf_server.start()

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.stop_wifi_tethering(self.dut)
        wutils.reset_wifi(self.dut)
        wutils.reset_wifi(self.dut_client)
        try:
            if "AccessPoint" in self.user_params:
                del self.user_params["reference_networks"]
                del self.user_params["open_network"]
        except:
            pass
        self.access_points[0].close()

    def teardown_class(self):
        if hasattr(self, 'iperf_server'):
            self.iperf_server.stop()

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    """Helper Functions"""

    def run_iperf_client(self, params):
        """Run iperf traffic after connection.

        Args:
            params: A tuple of network info and AndroidDevice object.

        """
        if "iperf_server_address" in self.user_params:
            network, ad = params
            SSID = network[WifiEnums.SSID_KEY]
            self.log.info("Starting iperf traffic through {}".format(SSID))
            port_arg = "-p {} -t {}".format(self.iperf_server.port, 3)
            success, data = ad.run_iperf_client(self.iperf_server_address,
                                                port_arg)
            self.log.debug(pprint.pformat(data))
            asserts.assert_true(success, "Error occurred in iPerf traffic.")
            self.log.info("Finished iperf traffic through {}".format(SSID))

    def start_softap_and_verify(self, band):
        """Bring-up softap and verify AP mode and in scan results.

        Args:
            band: The band to use for softAP.

        """
        config = wutils.create_softap_config()
        wutils.start_wifi_tethering(self.dut,
                                    config[wutils.WifiEnums.SSID_KEY],
                                    config[wutils.WifiEnums.PWD_KEY], band=band)
        asserts.assert_true(self.dut.droid.wifiIsApEnabled(),
                             "SoftAp is not reported as running")
        wutils.start_wifi_connection_scan_and_ensure_network_found(
            self.dut_client, config[wutils.WifiEnums.SSID_KEY])
        return config

    def get_softap_acs(self, softap):
        """Connect to the softap on client-dut and get the softap channel
           information.

        Args:
            softap: The softap network configuration information.

        """
        wutils.connect_to_wifi_network(self.dut_client, softap,
            check_connectivity=False)
        softap_info = self.dut_client.droid.wifiGetConnectionInfo()
        self.log.debug("DUT is connected to softAP %s with details: %s" %
                       (softap[wutils.WifiEnums.SSID_KEY], softap_info))
        frequency = softap_info['frequency']
        return hostapd_constants.CHANNEL_MAP[frequency]

    def configure_ap(self, channel_2g=None, channel_5g=None):
        """Configure and bring up AP on required channel.

        Args:
            channel_2g: The channel number to use for 2GHz network.
            channel_5g: The channel number to use for 5GHz network.

        """
        if "AccessPoint" in self.user_params:
            if not channel_2g:
                self.legacy_configure_ap_and_start(channel_5g=channel_5g)
            elif not channel_5g:
                self.legacy_configure_ap_and_start(channel_2g=channel_2g)
            else:
                self.legacy_configure_ap_and_start(channel_2g=channel_2g,
                    channel_5g=chanel_5g)

    def start_traffic_and_softap(self, network, softap_band):
        """Start iPerf traffic on client dut, during softAP bring-up on dut.

        Args:
            network: Network information of the network to connect to.
            softap_band: The band to use for softAP.

        """
        if not network:
            # For a clean environment just bring up softap and return channel.
            softap = self.start_softap_and_verify(softap_band)
            channel = self.get_softap_acs(softap)
            return channel
        # Connect to the AP and start IPerf traffic, while we bring up softap.
        wutils.connect_to_wifi_network(self.dut_client, network)
        t = Thread(target=self.run_iperf_client,args=((network,self.dut_client),))
        t.setDaemon(True)
        t.start()
        time.sleep(1)
        softap = self.start_softap_and_verify(softap_band)
        t.join()
        channel = self.get_softap_acs(softap)
        return channel

    def verify_acs_channel(self, chan, avoid_chan):
        """Verify ACS algorithm by ensuring that softAP came up on a channel,
           different than the active channels.

        Args:
            chan: The channel number softap came-up on.
            avoid_chan: The channel to avoid during this test.

        """
        if avoid_chan in range(1,12):
            avoid_chan2 = hostapd_constants.AP_DEFAULT_CHANNEL_5G
        elif avoid_chan in range(36, 166):
            avoid_chan2 = hostapd_constants.AP_DEFAULT_CHANNEL_2G
        if chan == avoid_chan or chan == avoid_chan2:
            raise signals.TestFailure("ACS chose the same channel that the "
                "AP was beaconing on. Channel = %d" % chan)

    """Tests"""
    @test_tracker_info(uuid="3507bd18-e787-4380-8725-1872916d4267")
    def test_softap_2G_clean_env(self):
        """Test to bring up SoftAp on 2GHz in clean environment."""
        network = None
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        if not chan in range(1, 12):
            raise signals.TestFailure("ACS chose incorrect channel %d for 2GHz "
                "band" % chan)

    @test_tracker_info(uuid="3d18da8b-d29a-45f9-8018-5348e10099e9")
    def test_softap_5G_clean_env(self):
        """Test to bring up SoftAp on 5GHz in clean environment."""
        network = None
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        if not chan in range(36, 166):
            # Note: This does not treat DFS channel separately.
            raise signals.TestFailure("ACS chose incorrect channel %d for 5GHz "
                "band" % chan)

    @test_tracker_info(uuid="cc353bda-3831-4d6e-b990-e501b8e4eafe")
    def test_softap_auto_clean_env(self):
        """Test to bring up SoftAp on AUTO-band in clean environment."""
        network = None
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_AUTO)
        if not chan in range(36, 166):
            # Note: This does not treat DFS channel separately.
            raise signals.TestFailure("ACS chose incorrect channel %d for 5GHz "
                "band" % chan)

    @test_tracker_info(uuid="a5f6a926-76d2-46a7-8136-426e35b5a5a8")
    def test_softap_2G_avoid_channel_1(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=1)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="757e2019-b027-40bf-a562-2b01f3e5957e")
    def test_softap_5G_avoid_channel_1(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=1)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="b96e39d1-9041-4662-a55f-22641c2e2b02")
    def test_softap_2G_avoid_channel_2(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=2)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="941c4e2b-ae35-4b49-aa81-13d3dc44b5b6")
    def test_softap_5G_avoid_channel_2(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=2)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="444c4a34-7f6b-4f02-9802-2e896e7d1796")
    def test_softap_2G_avoid_channel_3(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=3)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="eccd06b1-6df5-4144-8fda-1504cb822375")
    def test_softap_5G_avoid_channel_3(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=3)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="fb257644-2081-4c3d-8394-7a308dde0047")
    def test_softap_2G_avoid_channel_4(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=4)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="88b9cd16-4541-408a-8607-415fe60001f2")
    def test_softap_5G_avoid_channel_4(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=4)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="b3626ec8-50e8-412c-bdbe-5c5ade647d7b")
    def test_softap_2G_avoid_channel_5(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=5)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="45c4396b-9b0c-44f3-adf2-ea9c86fcab1d")
    def test_softap_5G_avoid_channel_5(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=5)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="f70634e7-c6fd-403d-8cd7-439fbbda6af0")
    def test_softap_2G_avoid_channel_6(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=6)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="f3341136-10bc-44e2-b9a8-2d27d3284b73")
    def test_softap_5G_avoid_channel_6(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=6)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="8129594d-1608-448b-8548-5a8c4022f2a1")
    def test_softap_2G_avoid_channel_7(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=7)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="7b470b82-d19b-438c-8f98-ce697e0eb474")
    def test_softap_5G_avoid_channel_7(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=7)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="11540182-d471-4bf0-8f8b-add89443c329")
    def test_softap_2G_avoid_channel_8(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=8)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="1280067c-389e-42e9-aa75-6bfbd61340f3")
    def test_softap_5G_avoid_channel_8(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=9)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="6feeb83c-2723-49cb-93c1-6297d4a3d853")
    def test_softap_2G_avoid_channel_9(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=9)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="49a110cd-03e8-4e99-9327-5123eab40902")
    def test_softap_5G_avoid_channel_9(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=9)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="a03c9e45-8763-4b5c-bead-e574fb9899a2")
    def test_softap_2G_avoid_channel_10(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=10)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="c1a1d272-a646-4c2d-8425-09d2ae6ae8e6")
    def test_softap_5G_avoid_channel_10(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=10)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="f38d8911-92d4-4dcd-ba23-1e1667fa1f5a")
    def test_softap_2G_avoid_channel_11(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_2g=11)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="24cc35ba-45e3-4b7a-9bc9-25b7abe92fa9")
    def test_softap_5G_avoid_channel_11(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_2g=11)
        network = self.reference_networks[0]["2g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="85aef720-4f3c-43bb-9de0-615b88c2bfe0")
    def test_softap_2G_avoid_channel_36(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=36)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="433e8db3-93b5-463e-a83c-0d4b9b9a8700")
    def test_softap_5G_avoid_channel_36(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=36)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="326e0e42-3219-4e63-a18d-5dc32c58e7d8")
    def test_softap_2G_avoid_channel_40(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=40)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="45953c03-c978-4775-a39b-fb7e70c8990a")
    def test_softap_5G_avoid_channel_40(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=40)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="e8e89cec-aa27-4780-8ff8-546d5af820f7")
    def test_softap_2G_avoid_channel_44(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=44)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="5e386d7d-d4c9-40cf-9333-06da55e11ba1")
    def test_softap_5G_avoid_channel_44(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=44)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="cb51dfca-f8de-4dfc-b513-e590c838c766")
    def test_softap_2G_avoid_channel_48(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=48)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="490b8ed1-196c-4941-b06b-5f0721ca440b")
    def test_softap_5G_avoid_channel_48(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=48)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="c5ab141b-e145-4cc1-b0d7-dd610cbfb462")
    def test_softap_2G_avoid_channel_149(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=149)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="108d7ef8-6fe7-49ba-b684-3820e881fcf0")
    def test_softap_5G_avoid_channel_149(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=149)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="f6926f40-0afc-41c5-9b38-c95a99788ff5")
    def test_softap_2G_avoid_channel_153(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=153)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="3d7b653b-c094-4c57-8e6a-047629b05216")
    def test_softap_5G_avoid_channel_153(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=153)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="b866ceea-d3ca-45d4-964a-4edea96026e6")
    def test_softap_2G_avoid_channel_157(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=157)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="03cb9163-bca3-442e-9691-6df82f8c51c7")
    def test_softap_2G_avoid_channel_157(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=157)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="ae10f23a-da70-43c8-9991-2c2f4a602724")
    def test_softap_2G_avoid_channel_161(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=161)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="521686c2-acfa-42d1-861b-aa10ac4dad34")
    def test_softap_5G_avoid_channel_161(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=161)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="77ebecd7-c036-463f-b77d-2cd70d89bc81")
    def test_softap_2G_avoid_channel_165(self):
        """Test to configure AP and bring up SoftAp on 2G."""
        self.configure_ap(channel_5g=165)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_2G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)

    @test_tracker_info(uuid="85d9386d-fe60-4708-9f91-75bbf8bec54f")
    def test_softap_5G_avoid_channel_165(self):
        """Test to configure AP and bring up SoftAp on 5G."""
        self.configure_ap(channel_5g=165)
        network = self.reference_networks[0]["5g"]
        chan = self.start_traffic_and_softap(network, WIFI_CONFIG_APBAND_5G)
        avoid_chan = int(sys._getframe().f_code.co_name.split('_')[-1])
        self.verify_acs_channel(chan, avoid_chan)
