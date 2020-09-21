#!/usr/bin/env python3.4
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

import copy
import pprint
import time

import acts.base_test
import acts.test_utils.wifi.wifi_test_utils as wutils

from acts import asserts
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi.WifiBaseTest import WifiBaseTest

SCAN_TIME = 30
WAIT_TIME = 2


class WifiPreFlightTest(WifiBaseTest):
    """ Pre-flight checks for Wifi tests.

    Test Bed Requirement:
    * One Android device
    * 4 reference networks - two 2G and two 5G networks
    * Attenuators to attenuate each reference network

    Tests:
    * Check if reference networks show up in wifi scan
    * Check if attenuators attenuate the correct network
    """

    def __init__(self, controllers):
        WifiBaseTest.__init__(self, controllers)
        self.WIFI_2G = "2g"
        self.WIFI_5G = "5g"
        self.PASSWORD = "password"
        self.MIN_SIGNAL_LEVEL = -45

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        wutils.wifi_toggle_state(self.dut, True)

        # Get reference networks as a list
        opt_params = ["reference_networks"]
        self.unpack_userparams(opt_param_names=opt_params)

        if "AccessPoint" in self.user_params:
            self.legacy_configure_ap_and_start(ap_count=2)
        networks = []
        for ref_net in self.reference_networks:
            networks.append(ref_net[self.WIFI_2G])
            networks.append(ref_net[self.WIFI_5G])
        self.reference_networks = networks
        asserts.assert_true(
            len(self.reference_networks) == 4,
            "Need at least 4 reference network with psk.")

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        for a in self.attenuators:
            a.set_atten(0)
        if "AccessPoint" in self.user_params:
            del self.user_params["reference_networks"]
            del self.user_params["open_network"]

    """ Helper functions """
    def _find_reference_networks_no_attn(self):
        """ Verify that when ATTN set to 0, all reference networks
            show up in the scanned results

            Args:
            1. List of reference networks

            Returns:
            1. List of networks not found. Empty if all reference
               networks are found
        """
        found_networks = copy.deepcopy(self.target_networks)
        start_time = time.time()
        while(time.time() < start_time + SCAN_TIME):
            if not found_networks:
                break
            time.sleep(WAIT_TIME)
            scanned_networks = self.dut.droid.wifiGetScanResults()
            self.log.info("SCANNED RESULTS %s" % scanned_networks)
            for net in self.target_networks:
                if net in found_networks:
                    result = wutils.match_networks(net, scanned_networks)
                    if result and result[0]['level'] > self.MIN_SIGNAL_LEVEL:
                        found_networks.remove(net)
                    elif result:
                        self.log.warn("Signal strength for %s is low: %sdBm"
                                      % (net, result[0]['level']))
        return found_networks

    def _find_network_after_setting_attn(self, target_network):
        """ Find network after setting attenuation

            Args:
            1. target_network to find in the scanned_results

            Returns:
            1. True if
               a. if max_attn is set and target_network not found
            2. False if not
        """
        start_time = time.time()
        while(time.time() < start_time + SCAN_TIME):
            time.sleep(WAIT_TIME)
            scanned_networks = self.dut.droid.wifiGetScanResults()
            self.log.info("SCANNED RESULTS %s" % scanned_networks)
            result = wutils.match_networks(target_network, scanned_networks)
            if not result:
                return True
        return False

    """ Tests """
    def test_attenuators(self):
        """ Test if attenuating a channel, disables the correct
            reference network

            Reference networks for each testbed should match
            attenuators as follows

            wh_ap1_2g - channel 1
            wh_ap1_5g - channel 2
            wh_ap2_2g - channel 3
            wh_ap2_5g - channel 4

            Steps:
            1. Set attenuation on each channel to 95
            2. Verify that the corresponding network does not show
               up in the scanned results
        """
        # Set attenuation to 0 and verify reference
        # networks show up in the scanned results
        self.log.info("Verify if all reference networks show with "
                      "attenuation set to 0")
        if getattr(self, "attenuators", []):
            for a in self.attenuators:
                a.set_atten(0)
        self.target_networks = []
        for ref_net in self.reference_networks:
            self.target_networks.append( {'BSSID': ref_net['bssid']} )
        result = self._find_reference_networks_no_attn()
        asserts.assert_true(not result,
                            "Did not find or signal strength too low "
                            "for the following reference networks\n%s\n" % result)

        # attenuate 1 channel at a time and find the network
        self.log.info("Verify if attenuation channel matches with "
                      "correct reference network")
        found_networks = []
        for i in range(len(self.attenuators)):
            target_network = {}
            target_network['BSSID'] = self.reference_networks[i]['bssid']

            # set the attn to max and verify target network is not found
            self.attenuators[i].set_atten(95)
            result = self._find_network_after_setting_attn(target_network)
            if result:
                target_network['ATTN'] = i
                found_networks.append(target_network)

        asserts.assert_true(not found_networks,
                            "Attenuators did not match the networks\n %s\n"
                            % pprint.pformat(found_networks))
