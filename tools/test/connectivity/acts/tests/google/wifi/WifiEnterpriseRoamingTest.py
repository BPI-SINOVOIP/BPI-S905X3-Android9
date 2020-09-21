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

import pprint
import random
import time

from acts import asserts
from acts import base_test
from acts import signals
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils as wutils

WifiEnums = wutils.WifiEnums

# EAP Macros
EAP = WifiEnums.Eap
EapPhase2 = WifiEnums.EapPhase2

# Enterprise Config Macros
Ent = WifiEnums.Enterprise


class WifiEnterpriseRoamingTest(base_test.BaseTestClass):
    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        req_params = (
            "ent_roaming_ssid",
            "bssid_a",
            "bssid_b",
            "attn_vals",
            # Expected time within which roaming should finish, in seconds.
            "roam_interval",
            "ca_cert",
            "client_cert",
            "client_key",
            "eap_identity",
            "eap_password",
            "device_password")
        self.unpack_userparams(req_params)
        self.config_peap = {
            Ent.EAP: int(EAP.PEAP),
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: int(EapPhase2.MSCHAPV2),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_tls = {
            Ent.EAP: int(EAP.TLS),
            Ent.CA_CERT: self.ca_cert,
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            Ent.CLIENT_CERT: self.client_cert,
            Ent.PRIVATE_KEY_ID: self.client_key,
            Ent.IDENTITY: self.eap_identity,
        }
        self.config_ttls = {
            Ent.EAP: int(EAP.TTLS),
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: int(EapPhase2.MSCHAPV2),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid
        }
        self.config_sim = {
            Ent.EAP: int(EAP.SIM),
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
        }
        self.attn_a = self.attenuators[0]
        self.attn_b = self.attenuators[1]
        # Set screen lock password so ConfigStore is unlocked.
        self.dut.droid.setDevicePassword(self.device_password)
        self.set_attns("default")

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        self.dut.droid.disableDevicePassword()
        self.dut.ed.clear_all_events()
        self.set_attns("default")

    def setup_test(self):
        self.dut.droid.wifiStartTrackingStateChange()
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()

    def teardown_test(self):
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        self.dut.droid.wifiStopTrackingStateChange()
        self.set_attns("default")

    def on_fail(self, test_name, begin_time):
        self.dut.cat_adb_log(test_name, begin_time)

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
            self.log.exception("Failed to set attenuation values %s.",
                           attn_val_name)
            raise

    def trigger_roaming_and_validate(self, attn_val_name, expected_con):
        """Sets attenuators to trigger roaming and validate the DUT connected
        to the BSSID expected.

        Args:
            attn_val_name: Name of the attenuation value pair to use.
            expected_con: The expected info of the network to we expect the DUT
                to roam to.
        """
        self.set_attns(attn_val_name)
        self.log.info("Wait %ss for roaming to finish.", self.roam_interval)
        time.sleep(self.roam_interval)
        try:
            self.dut.droid.wakeLockAcquireBright()
            self.dut.droid.wakeUpNow()
            wutils.verify_wifi_connection_info(self.dut, expected_con)
            expected_bssid = expected_con[WifiEnums.BSSID_KEY]
            self.log.info("Roamed to %s successfully", expected_bssid)
        finally:
            self.dut.droid.wifiLockRelease()
            self.dut.droid.goToSleepNow()

    def roaming_between_a_and_b_logic(self, config):
        """Test roaming between two enterprise APs.

        Steps:
        1. Make bssid_a visible, bssid_b not visible.
        2. Connect to ent_roaming_ssid. Expect DUT to connect to bssid_a.
        3. Make bssid_a not visible, bssid_b visible.
        4. Expect DUT to roam to bssid_b.
        5. Make bssid_a visible, bssid_b not visible.
        6. Expect DUT to roam back to bssid_a.
        """
        expected_con_to_a = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_a,
        }
        expected_con_to_b = {
            WifiEnums.SSID_KEY: self.ent_roaming_ssid,
            WifiEnums.BSSID_KEY: self.bssid_b,
        }
        self.set_attns("a_on_b_off")
        wutils.wifi_connect(self.dut, config)
        wutils.verify_wifi_connection_info(self.dut, expected_con_to_a)
        self.log.info("Roaming from %s to %s", self.bssid_a, self.bssid_b)
        self.trigger_roaming_and_validate("b_on_a_off", expected_con_to_b)
        self.log.info("Roaming from %s to %s", self.bssid_b, self.bssid_a)
        self.trigger_roaming_and_validate("a_on_b_off", expected_con_to_a)

    """ Tests Begin """

    @test_tracker_info(uuid="b15e4b3f-841d-428d-87ac-272f29f06e14")
    def test_roaming_with_config_tls(self):
        self.roaming_between_a_and_b_logic(self.config_tls)

    @test_tracker_info(uuid="d349cfec-b4af-48b2-88b7-744f5de25d43")
    def test_roaming_with_config_ttls_none(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="89b8161c-754e-4138-831d-5fe40f521ce4")
    def test_roaming_with_config_ttls_pap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="d4925470-924b-4d03-8437-83e26b5f2df3")
    def test_roaming_with_config_ttls_mschap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="206b1327-dd9c-4742-8717-e7bf2a04eed6")
    def test_roaming_with_config_ttls_mschapv2(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="c2c0168b-2933-4954-af62-fb41f42dc45a")
    def test_roaming_with_config_ttls_gtc(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="481c4102-8f5b-4fcd-95cc-5e3285f47985")
    def test_roaming_with_config_peap_mschapv2(self):
        config = dict(self.config_peap)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.roaming_between_a_and_b_logic(config)

    @test_tracker_info(uuid="404155d4-33a7-42b3-b369-5f2d63d19f16")
    def test_roaming_with_config_peap_gtc(self):
        config = dict(self.config_peap)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.roaming_between_a_and_b_logic(config)

    """ Tests End """
