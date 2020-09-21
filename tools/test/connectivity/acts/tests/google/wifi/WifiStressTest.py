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

import pprint
import time

import acts.base_test
import acts.test_utils.wifi.wifi_test_utils as wutils
import acts.utils

from acts import asserts
from acts import signals
from acts import utils
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest
WifiEnums = wutils.WifiEnums

WAIT_FOR_AUTO_CONNECT = 40
WAIT_BEFORE_CONNECTION = 30

TIMEOUT = 1
PING_ADDR = 'www.google.com'

class WifiStressTest(WifiBaseTest):
    """WiFi Stress test class.

    Test Bed Requirement:
    * Two Android device
    * Several Wi-Fi networks visible to the device, including an open Wi-Fi
      network.
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        self.dut_client = self.android_devices[1]
        wutils.wifi_test_device_init(self.dut)
        req_params = []
        opt_param = [
            "open_network", "reference_networks", "iperf_server_address",
            "stress_count", "stress_hours"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(ap_count=2)

        asserts.assert_true(
            len(self.reference_networks) > 0,
            "Need at least one reference network with psk.")
        self.wpa_2g = self.reference_networks[0]["2g"]
        self.wpa_5g = self.reference_networks[0]["5g"]
        self.open_2g = self.open_network[0]["2g"]
        self.open_5g = self.open_network[0]["5g"]
        self.networks = [self.wpa_2g, self.wpa_5g, self.open_2g, self.open_5g]
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
        wutils.reset_wifi(self.dut)

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        if hasattr(self, 'iperf_server'):
            self.iperf_server.stop()
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""

    def scan_and_connect_by_ssid(self, network):
        """Scan for network and connect using network information.

        Args:
            network: A dictionary representing the network to connect to.

        """
        ssid = network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(self.dut,
            ssid)
        wutils.wifi_connect(self.dut, network, num_of_tries=3)

    def scan_and_connect_by_id(self, network, net_id):
        """Scan for network and connect using network id.

        Args:
            net_id: Integer specifying the network id of the network.

        """
        ssid = network[WifiEnums.SSID_KEY]
        wutils.start_wifi_connection_scan_and_ensure_network_found(self.dut,
            ssid)
        wutils.wifi_connect_by_id(self.dut, net_id)

    def run_ping(self, sec):
        """Run ping for given number of seconds.

        Args:
            sec: Time in seconds to run teh ping traffic.

        """
        self.log.info("Running ping for %d seconds" % sec)
        result = self.dut.adb.shell("ping -w %d %s" %(sec, PING_ADDR),
            timeout=sec+1)
        self.log.debug("Ping Result = %s" % result)
        if "100% packet loss" in result:
            raise signals.TestFailure("100% packet loss during ping")

    """Tests"""

    @test_tracker_info(uuid="cd0016c6-58cf-4361-b551-821c0b8d2554")
    def test_stress_toggle_wifi_state(self):
        """Toggle WiFi state ON and OFF for N times."""
        for count in range(self.stress_count):
            """Test toggling wifi"""
            try:
                self.log.debug("Going from on to off.")
                wutils.wifi_toggle_state(self.dut, False)
                self.log.debug("Going from off to on.")
                startTime = time.time()
                wutils.wifi_toggle_state(self.dut, True)
                startup_time = time.time() - startTime
                self.log.debug("WiFi was enabled on the device in %s s." %
                    startup_time)
            except:
                signals.TestFailure(details="", extras={"Iterations":"%d" %
                    self.stress_count, "Pass":"%d" %count})
        raise signals.TestPass(details="", extras={"Iterations":"%d" %
            self.stress_count, "Pass":"%d" %(count+1)})

    @test_tracker_info(uuid="49e3916a-9580-4bf7-a60d-a0f2545dcdde")
    def test_stress_connect_traffic_disconnect_5g(self):
        """Test to connect and disconnect from a network for N times.

           Steps:
               1. Scan and connect to a network.
               2. Run IPerf to upload data for few seconds.
               3. Disconnect.
               4. Repeat 1-3.

        """
        for count in range(self.stress_count):
            try:
                net_id = self.dut.droid.wifiAddNetwork(self.wpa_5g)
                asserts.assert_true(net_id != -1, "Add network %r failed" % self.wpa_5g)
                self.scan_and_connect_by_id(self.wpa_5g, net_id)
                # Start IPerf traffic from phone to server.
                # Upload data for 10s.
                args = "-p {} -t {}".format(self.iperf_server.port, 10)
                self.log.info("Running iperf client {}".format(args))
                result, data = self.dut.run_iperf_client(self.iperf_server_address, args)
                if not result:
                    self.log.debug("Error occurred in iPerf traffic.")
                    self.run_ping(10)
                wutils.wifi_forget_network(self.dut,self.wpa_5g[WifiEnums.SSID_KEY])
                time.sleep(WAIT_BEFORE_CONNECTION)
            except:
                raise signals.TestFailure("Network connect-disconnect failed."
                    "Look at logs", extras={"Iterations":"%d" %
                        self.stress_count, "Pass":"%d" %count})
        raise signals.TestPass(details="", extras={"Iterations":"%d" %
            self.stress_count, "Pass":"%d" %(count+1)})

    @test_tracker_info(uuid="e9827dff-0755-43ec-8b50-1f9756958460")
    def test_stress_connect_long_traffic_5g(self):
        """Test to connect to network and hold connection for few hours.

           Steps:
               1. Scan and connect to a network.
               2. Run IPerf to download data for few hours.
               3. Verify no WiFi disconnects/data interruption.

        """
        try:
            self.scan_and_connect_by_ssid(self.wpa_5g)
            # Start IPerf traffic from server to phone.
            # Download data for 5 hours.
            sec = self.stress_hours * 60 * 60
            args = "-p {} -t {} -R".format(self.iperf_server.port, sec)
            self.log.info("Running iperf client {}".format(args))
            result, data = self.dut.run_iperf_client(self.iperf_server_address,
                args, timeout=sec+1)
            if not result:
                self.log.debug("Error occurred in iPerf traffic.")
                self.run_ping(sec)
        except:
            raise signals.TestFailure("Network long-connect failed."
                "Look at logs", extras={"Total Hours":"%d" %self.stress_hours,
                    "Seconds Run":"UNKNOWN"})
        raise signals.TestPass(details="", extras={"Total Hours":"%d" %
            self.stress_hours, "Seconds":"%d" %sec})

    @test_tracker_info(uuid="d367c83e-5b00-4028-9ed8-f7b875997d13")
    def test_stress_wifi_failover(self):
        """This test does aggressive failover to several networks in list.

           Steps:
               1. Add and enable few networks.
               2. Let device auto-connect.
               3. Remove the connected network.
               4. Repeat 2-3.
               5. Device should connect to a network until all networks are
                  exhausted.

        """
        for count in range(int(self.stress_count/4)):
            ssids = list()
            for network in self.networks:
                ssids.append(network[WifiEnums.SSID_KEY])
                ret = self.dut.droid.wifiAddNetwork(network)
                asserts.assert_true(ret != -1, "Add network %r failed" % network)
                self.dut.droid.wifiEnableNetwork(ret, 0)
            time.sleep(WAIT_FOR_AUTO_CONNECT)
            cur_network = self.dut.droid.wifiGetConnectionInfo()
            cur_ssid = cur_network[WifiEnums.SSID_KEY]
            self.log.info("Cur_ssid = %s" % cur_ssid)
            for i in range(0,len(self.networks)):
                self.log.debug("Forget network %s" % cur_ssid)
                wutils.wifi_forget_network(self.dut, cur_ssid)
                time.sleep(WAIT_FOR_AUTO_CONNECT)
                cur_network = self.dut.droid.wifiGetConnectionInfo()
                cur_ssid = cur_network[WifiEnums.SSID_KEY]
                self.log.info("Cur_ssid = %s" % cur_ssid)
                if i == len(self.networks) - 1:
                    break
                if cur_ssid not in ssids:
                    raise signals.TestFailure("Device did not failover to the "
                        "expected network. SSID = %s" % cur_ssid)
            network_config = self.dut.droid.wifiGetConfiguredNetworks()
            self.log.info("Network Config = %s" % network_config)
            if len(network_config):
                raise signals.TestFailure("All the network configurations were not "
                    "removed. Configured networks = %s" % network_config,
                        extras={"Iterations":"%d" % self.stress_count,
                            "Pass":"%d" %(count*4)})
        raise signals.TestPass(details="", extras={"Iterations":"%d" %
            self.stress_count, "Pass":"%d" %((count+1)*4)})

    @test_tracker_info(uuid="2c19e8d1-ac16-4d7e-b309-795144e6b956")
    def test_stress_softAP_startup_and_stop_5g(self):
        """Test to bring up softAP and down for N times.

        Steps:
            1. Bring up softAP on 5G.
            2. Check for softAP on teh client device.
            3. Turn ON WiFi.
            4. Verify softAP is turned down and WiFi is up.

        """
        # Set country code explicitly to "US".
        self.dut.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
        self.dut_client.droid.wifiSetCountryCode(wutils.WifiEnums.CountryCode.US)
        ap_ssid = "softap_" + utils.rand_ascii_str(8)
        ap_password = utils.rand_ascii_str(8)
        self.dut.log.info("softap setup: %s %s", ap_ssid, ap_password)
        config = {wutils.WifiEnums.SSID_KEY: ap_ssid}
        config[wutils.WifiEnums.PWD_KEY] = ap_password
        for count in range(self.stress_count):
            initial_wifi_state = self.dut.droid.wifiCheckState()
            wutils.start_wifi_tethering(self.dut,
                ap_ssid,
                ap_password,
                WifiEnums.WIFI_CONFIG_APBAND_5G)
            wutils.start_wifi_connection_scan_and_ensure_network_found(
                self.dut_client, ap_ssid)
            wutils.stop_wifi_tethering(self.dut)
            asserts.assert_false(self.dut.droid.wifiIsApEnabled(),
                                 "SoftAp failed to shutdown!")
            time.sleep(TIMEOUT)
            cur_wifi_state = self.dut.droid.wifiCheckState()
            if initial_wifi_state != cur_wifi_state:
               raise signals.TestFailure("Wifi state was %d before softAP and %d now!" %
                    (initial_wifi_state, cur_wifi_state),
                        extras={"Iterations":"%d" % self.stress_count,
                            "Pass":"%d" %count})
        raise signals.TestPass(details="", extras={"Iterations":"%d" %
            self.stress_count, "Pass":"%d" %(count+1)})

    @test_tracker_info(uuid="eb22e26b-95d1-4580-8c76-85dfe6a42a0f")
    def test_stress_wifi_roaming(self):
        AP1_network = self.reference_networks[0]["5g"]
        AP2_network = self.reference_networks[1]["5g"]
        wutils.set_attns(self.attenuators, "AP1_on_AP2_off")
        self.scan_and_connect_by_ssid(AP1_network)
        # Reduce iteration to half because each iteration does two roams.
        for count in range(int(self.stress_count/2)):
            self.log.info("Roaming iteration %d, from %s to %s", count,
                           AP1_network, AP2_network)
            try:
                wutils.trigger_roaming_and_validate(self.dut, self.attenuators,
                    "AP1_off_AP2_on", AP2_network)
                self.log.info("Roaming iteration %d, from %s to %s", count,
                               AP2_network, AP1_network)
                wutils.trigger_roaming_and_validate(self.dut, self.attenuators,
                    "AP1_on_AP2_off", AP1_network)
            except:
                raise signals.TestFailure("Roaming failed. Look at logs",
                    extras={"Iterations":"%d" %self.stress_count, "Pass":"%d" %
                        (count*2)})
        raise signals.TestPass(details="", extras={"Iterations":"%d" %
            self.stress_count, "Pass":"%d" %((count+1)*2)})

