#
#   Copyright 2014 - The Android Open Source Project
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

from acts import asserts
from acts import base_test
from acts.test_decorators import test_tracker_info
import acts.test_utils.wifi.wifi_test_utils as wutils
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

WifiEnums = wutils.WifiEnums
MAX_ATTN = 95

class WifiPnoTest(WifiBaseTest):

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = ["attn_vals", "pno_interval"]
        opt_param = ["reference_networks"]
        self.unpack_userparams(
            req_param_names=req_params, opt_param_names=opt_param)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start()

        self.pno_network_a = self.reference_networks[0]['2g']
        self.pno_network_b = self.reference_networks[0]['5g']
        self.attn_a = self.attenuators[0]
        self.attn_b = self.attenuators[1]
        # Disable second AP's networks, so that it does not interfere during PNO
        self.attenuators[2].set_atten(MAX_ATTN)
        self.attenuators[3].set_atten(MAX_ATTN)
        self.set_attns("default")

    def setup_test(self):
        self.dut.droid.wifiStartTrackingStateChange()
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()

    def teardown_test(self):
        self.dut.droid.wifiStopTrackingStateChange()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()
        self.set_attns("default")

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)
        self.dut.cat_adb_log(test_name, begin_time)

    def teardown_class(self):
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """Helper Functions"""

    def set_attns(self, attn_val_name):
        """Sets attenuation values on attenuators used in this test.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
        """
        self.log.info("Set attenuation values to %s",
                      self.attn_vals[attn_val_name])
        try:
            self.attn_a.set_atten(self.attn_vals[attn_val_name][0])
            self.attn_b.set_atten(self.attn_vals[attn_val_name][1])
        except:
            self.log.error("Failed to set attenuation values %s.",
                           attn_val_name)
            raise

    def trigger_pno_and_assert_connect(self, attn_val_name, expected_con):
        """Sets attenuators to disconnect current connection to trigger PNO.
        Validate that the DUT connected to the new SSID as expected after PNO.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
            expected_con: The expected info of the network to we expect the DUT
                to roam to.
        """
        connection_info = self.dut.droid.wifiGetConnectionInfo()
        self.log.info("Triggering PNO connect from %s to %s",
                      connection_info[WifiEnums.SSID_KEY],
                      expected_con[WifiEnums.SSID_KEY])
        self.set_attns(attn_val_name)
        self.log.info("Wait %ss for PNO to trigger.", self.pno_interval)
        time.sleep(self.pno_interval)
        try:
            self.log.info("Connected to %s network after PNO interval"
                          % self.dut.droid.wifiGetConnectionInfo())
            expected_ssid = expected_con[WifiEnums.SSID_KEY]
            verify_con = {WifiEnums.SSID_KEY: expected_ssid}
            wutils.verify_wifi_connection_info(self.dut, verify_con)
            self.log.info("Connected to %s successfully after PNO",
                          expected_ssid)
        finally:
            pass

    def add_and_enable_dummy_networks(self, num_networks):
        """Add some dummy networks to the device and enable them.

        Args:
            num_networks: Number of networks to add.
        """
        ssid_name_base = "pno_dummy_network_"
        for i in range(0, num_networks):
            network = {}
            network[WifiEnums.SSID_KEY] = ssid_name_base + str(i)
            network[WifiEnums.PWD_KEY] = "pno_dummy"
            self.add_network_and_enable(network)

    def add_network_and_enable(self, network):
        """Add a network and enable it.

        Args:
            network : Network details for the network to be added.

        """
        ret = self.dut.droid.wifiAddNetwork(network)
        asserts.assert_true(ret != -1, "Add network %r failed" % network)
        self.dut.droid.wifiEnableNetwork(ret, 0)


    """ Tests Begin """

    @test_tracker_info(uuid="33d3cae4-5fa7-4e90-b9e2-5d3747bba64c")
    def test_simple_pno_connection_to_2g(self):
        """Test PNO triggered autoconnect to a network.

        Steps:
        1. Switch off the screen on the device.
        2. Save 2 valid network configurations (a & b) in the device.
        3. Attenuate 5Ghz network and wait for a few seconds to trigger PNO.
        4. Check the device connected to 2Ghz network automatically.
        """
        self.add_network_and_enable(self.pno_network_a)
        self.add_network_and_enable(self.pno_network_b)
        self.trigger_pno_and_assert_connect("a_on_b_off", self.pno_network_a)

    @test_tracker_info(uuid="39b945a1-830f-4f11-9e6a-9e9641066a96")
    def test_simple_pno_connection_to_5g(self):
        """Test PNO triggered autoconnect to a network.

        Steps:
        1. Switch off the screen on the device.
        2. Save 2 valid network configurations (a & b) in the device.
        3. Attenuate 2Ghz network and wait for a few seconds to trigger PNO.
        4. Check the device connected to 5Ghz network automatically.

        """
        self.add_network_and_enable(self.pno_network_a)
        self.add_network_and_enable(self.pno_network_b)
        self.trigger_pno_and_assert_connect("b_on_a_off", self.pno_network_b)

    @test_tracker_info(uuid="844b15be-ff45-4b09-a11b-0b2b4bb13b22")
    def test_pno_connection_with_multiple_saved_networks(self):
        """Test PNO triggered autoconnect to a network when there are more
        than 16 networks saved in the device.

        16 is the max list size of PNO watch list for most devices. The device
        should automatically pick the 16 latest added networks in the list.
        So add 16 dummy networks and then add 2 valid networks.

        Steps:
        1. Save 16 dummy network configurations in the device.
        2. Run the simple pno test.
        """
        self.add_and_enable_dummy_networks(16)
        self.add_network_and_enable(self.pno_network_a)
        self.add_network_and_enable(self.pno_network_b)
        # Force single scan so that both networks become preferred before PNO.
        wutils.start_wifi_connection_scan(self.dut)
        self.trigger_pno_and_assert_connect("a_on_b_off", self.pno_network_a)

    """ Tests End """
