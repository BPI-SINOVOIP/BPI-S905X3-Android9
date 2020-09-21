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

import pprint
import random
import time

from acts import asserts
from acts import base_test
from acts import signals
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums

class WifiRoamingTest(WifiBaseTest):

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        """Setup required dependencies from config file and configure
           the required networks for testing roaming.

        Returns:
            True if successfully configured the requirements for testing.
        """
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = ("roaming_attn", "roam_interval", "ping_addr", "max_bugreports")
        opt_param = [
            "open_network", "reference_networks", "iperf_server_address"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(ap_count=2)

        asserts.assert_true(
            len(self.reference_networks) > 1,
            "Need at least two psk networks for roaming.")
        asserts.assert_true(
            len(self.open_network) > 1,
            "Need at least two open networks for roaming")
        wutils.wifi_toggle_state(self.dut, True)
        if "iperf_server_address" in self.user_params:
            self.iperf_server = self.iperf_servers[0]
            self.iperf_server.start()

    def teardown_class(self):
        self.dut.ed.clear_all_events()
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]
        self.iperf_server.stop()

    def setup_test(self):
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)

    def on_fail(self, test_name, begin_time):
        self.dut.cat_adb_log(test_name, begin_time)
        self.dut.take_bug_report(test_name, begin_time)

    def roaming_from_AP1_and_AP2(self, AP1_network, AP2_network):
        """Test roaming between two APs.

        Args:
            AP1_network: AP-1's network information.
            AP2_network: AP-2's network information.

        Steps:
        1. Make AP1 visible, AP2 not visible.
        2. Connect to AP1's ssid.
        3. Make AP1 not visible, AP2 visible.
        4. Expect DUT to roam to AP2.
        5. Validate connection information and ping.
        """
        wutils.set_attns(self.attenuators, "AP1_on_AP2_off")
        wutils.wifi_connect(self.dut, AP1_network)
        self.log.info("Roaming from %s to %s", AP1_network, AP2_network)
        wutils.trigger_roaming_and_validate(self.dut, self.attenuators,
            "AP1_off_AP2_on", AP2_network)

    """ Tests Begin.

        The following tests are designed to test inter-SSID Roaming only.

        """
    @test_tracker_info(uuid="db8a46f9-713f-4b98-8d9f-d36319905b0a")
    def test_roaming_between_AP1_to_AP2_open_2g(self):
        AP1_network = self.open_network[0]["2g"]
        AP2_network = self.open_network[1]["2g"]
        self.roaming_from_AP1_and_AP2(AP1_network, AP2_network)

    @test_tracker_info(uuid="0db67d9b-6ea9-4f40-acf2-155c4ecf9dc5")
    def test_roaming_between_AP1_to_AP2_open_5g(self):
        AP1_network = self.open_network[0]["5g"]
        AP2_network = self.open_network[1]["5g"]
        self.roaming_from_AP1_and_AP2(AP1_network, AP2_network)

    @test_tracker_info(uuid="eabc7319-d962-4bef-b679-725e9ff00420")
    def test_roaming_between_AP1_to_AP2_psk_2g(self):
        AP1_network = self.reference_networks[0]["2g"]
        AP2_network = self.reference_networks[1]["2g"]
        self.roaming_from_AP1_and_AP2(AP1_network, AP2_network)

    @test_tracker_info(uuid="1cf9c681-4ff0-45c1-9719-f01629f6a7f7")
    def test_roaming_between_AP1_to_AP2_psk_5g(self):
        AP1_network = self.reference_networks[0]["5g"]
        AP2_network = self.reference_networks[1]["5g"]
        self.roaming_from_AP1_and_AP2(AP1_network, AP2_network)

    """ Tests End """
