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

import pprint
import random
import time

from acts import asserts
from acts import base_test
from acts import signals
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_test_utils import start_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import stop_adb_tcpdump
from acts.test_utils.wifi import wifi_test_utils as wutils

WifiEnums = wutils.WifiEnums

# EAP Macros
EAP = WifiEnums.Eap
EapPhase2 = WifiEnums.EapPhase2
# Enterprise Config Macros
Ent = WifiEnums.Enterprise


class WifiEnterpriseTest(base_test.BaseTestClass):
    def __init__(self, controllers):
        base_test.BaseTestClass.__init__(self, controllers)

    def setup_class(self):
        self.dut = self.android_devices[0]
        wutils.wifi_test_device_init(self.dut)
        # If running in a setup with attenuators, set attenuation on all
        # channels to zero.
        if getattr(self, "attenuators", []):
            for a in self.attenuators:
                a.set_atten(0)
        required_userparam_names = (
            "ca_cert", "client_cert", "client_key", "passpoint_ca_cert",
            "passpoint_client_cert", "passpoint_client_key", "eap_identity",
            "eap_password", "invalid_ca_cert", "invalid_client_cert",
            "invalid_client_key", "fqdn", "provider_friendly_name", "realm",
            "ssid_peap0", "ssid_peap1", "ssid_tls", "ssid_ttls", "ssid_pwd",
            "ssid_sim", "ssid_aka", "ssid_aka_prime", "ssid_passpoint",
            "device_password", "ping_addr")
        self.unpack_userparams(required_userparam_names,
                               roaming_consortium_ids=None,
                               plmn=None)
        # Default configs for EAP networks.
        self.config_peap0 = {
            Ent.EAP: int(EAP.PEAP),
            Ent.CA_CERT: self.ca_cert,
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            Ent.PHASE2: int(EapPhase2.MSCHAPV2),
            WifiEnums.SSID_KEY: self.ssid_peap0
        }
        self.config_peap1 = dict(self.config_peap0)
        self.config_peap1[WifiEnums.SSID_KEY] = self.ssid_peap1
        self.config_tls = {
            Ent.EAP: int(EAP.TLS),
            Ent.CA_CERT: self.ca_cert,
            WifiEnums.SSID_KEY: self.ssid_tls,
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
            WifiEnums.SSID_KEY: self.ssid_ttls
        }
        self.config_pwd = {
            Ent.EAP: int(EAP.PWD),
            Ent.IDENTITY: self.eap_identity,
            Ent.PASSWORD: self.eap_password,
            WifiEnums.SSID_KEY: self.ssid_pwd
        }
        self.config_sim = {
            Ent.EAP: int(EAP.SIM),
            WifiEnums.SSID_KEY: self.ssid_sim,
        }
        self.config_aka = {
            Ent.EAP: int(EAP.AKA),
            WifiEnums.SSID_KEY: self.ssid_aka,
        }
        self.config_aka_prime = {
            Ent.EAP: int(EAP.AKA_PRIME),
            WifiEnums.SSID_KEY: self.ssid_aka_prime,
        }

        # Base config for passpoint networks.
        self.config_passpoint = {
            Ent.FQDN: self.fqdn,
            Ent.FRIENDLY_NAME: self.provider_friendly_name,
            Ent.REALM: self.realm,
            Ent.CA_CERT: self.passpoint_ca_cert
        }
        if self.plmn:
            self.config_passpoint[Ent.PLMN] = self.plmn
        if self.roaming_consortium_ids:
            self.config_passpoint[
                Ent.ROAMING_IDS] = self.roaming_consortium_ids

        # Default configs for passpoint networks.
        self.config_passpoint_tls = dict(self.config_tls)
        self.config_passpoint_tls.update(self.config_passpoint)
        self.config_passpoint_tls[Ent.CLIENT_CERT] = self.passpoint_client_cert
        self.config_passpoint_tls[
            Ent.PRIVATE_KEY_ID] = self.passpoint_client_key
        del self.config_passpoint_tls[WifiEnums.SSID_KEY]
        self.config_passpoint_ttls = dict(self.config_ttls)
        self.config_passpoint_ttls.update(self.config_passpoint)
        del self.config_passpoint_ttls[WifiEnums.SSID_KEY]
        # Set screen lock password so ConfigStore is unlocked.
        self.dut.droid.setDevicePassword(self.device_password)
        self.tcpdump_pid = None

    def teardown_class(self):
        wutils.reset_wifi(self.dut)
        self.dut.droid.disableDevicePassword()
        self.dut.ed.clear_all_events()

    def setup_test(self):
        self.dut.droid.wifiStartTrackingStateChange()
        self.dut.droid.wakeLockAcquireBright()
        self.dut.droid.wakeUpNow()
        wutils.reset_wifi(self.dut)
        self.dut.ed.clear_all_events()
        self.tcpdump_pid = start_adb_tcpdump(self.dut, self.test_name, mask='all')

    def teardown_test(self):
        if self.tcpdump_pid:
            stop_adb_tcpdump(self.dut,
                             self.tcpdump_pid,
                             pull_tcpdump=True)
            self.tcpdump_pid = None
        self.dut.droid.wakeLockRelease()
        self.dut.droid.goToSleepNow()
        self.dut.droid.wifiStopTrackingStateChange()

    def on_fail(self, test_name, begin_time):
        self.dut.cat_adb_log(test_name, begin_time)

    """Helper Functions"""

    def eap_negative_connect_logic(self, config, ad):
        """Tries to connect to an enterprise network with invalid credentials
        and expect a failure.

        Args:
            config: A dict representing an invalid EAP credential.

        Returns:
            True if connection failed as expected, False otherwise.
        """
        with asserts.assert_raises(signals.TestFailure, extras=config):
            verdict = wutils.wifi_connect(ad, config)
        asserts.explicit_pass("Connection failed as expected.")

    def gen_negative_configs(self, config, neg_params):
        """Generic function used to generate negative configs.

        For all the valid configurations, if a param in the neg_params also
        exists in a config, a copy of the config is made with an invalid value
        of the param.

        Args:
            config: A valid configuration.
            neg_params: A dict that has all the invalid values.

        Returns:
            An invalid configurations generated based on the valid
            configuration. Each invalid configuration has a different invalid
            field.
        """
        negative_config = dict(config)
        if negative_config in [self.config_sim, self.config_aka,
                               self.config_aka_prime]:
            negative_config[WifiEnums.SSID_KEY] = 'wrong_hostapd_ssid'
        for k, v in neg_params.items():
            # Skip negative test for TLS's identity field since it's not
            # used for auth.
            if config[Ent.EAP] == EAP.TLS and k == Ent.IDENTITY:
                continue
            if k in config:
                negative_config[k] = v
                negative_config["invalid_field"] = k
        return negative_config

    def gen_negative_eap_configs(self, config):
        """Generates invalid configurations for different EAP authentication
        types.

        For all the valid EAP configurations, if a param that is part of the
        authentication info exists in a config, a copy of the config is made
        with an invalid value of the param.

        Args:
            A valid network configration

        Returns:
            An invalid EAP configuration.
        """
        neg_params = {
            Ent.CLIENT_CERT: self.invalid_client_cert,
            Ent.CA_CERT: self.invalid_ca_cert,
            Ent.PRIVATE_KEY_ID: self.invalid_client_key,
            Ent.IDENTITY: "fake_identity",
            Ent.PASSWORD: "wrong_password"
        }
        return self.gen_negative_configs(config, neg_params)

    def gen_negative_passpoint_configs(self, config):
        """Generates invalid configurations for different EAP authentication
        types with passpoint support.

        Args:
            A valid network configration

        Returns:
            An invalid EAP configuration with passpoint fields.
        """
        neg_params = {
            Ent.CLIENT_CERT: self.invalid_client_cert,
            Ent.CA_CERT: self.invalid_ca_cert,
            Ent.PRIVATE_KEY_ID: self.invalid_client_key,
            Ent.IDENTITY: "fake_identity",
            Ent.PASSWORD: "wrong_password",
            Ent.FQDN: "fake_fqdn",
            Ent.REALM: "where_no_one_has_gone_before",
            Ent.PLMN: "fake_plmn",
            Ent.ROAMING_IDS: [1234567890, 9876543210]
        }
        return self.gen_negative_configs(config, neg_params)

    def eap_connect_toggle_wifi(self,
                                config,
                                *args):
        """Connects to an enterprise network, toggles wifi state and ensures
        that the device reconnects.

        This logic expect the enterprise network to have Internet access.

        Args:
            config: A dict representing a wifi enterprise configuration.
            args: args to be passed to |wutils.eap_connect|.

        Returns:
            True if the connection is successful and Internet access works.
        """
        ad = args[0]
        wutils.wifi_connect(ad, config)
        wutils.toggle_wifi_and_wait_for_reconnection(ad, config, num_of_tries=5)

    """ Tests """

    # EAP connect tests
    """ Test connecting to enterprise networks of different authentication
        types.

        The authentication types tested are:
            EAP-TLS
            EAP-PEAP with different phase2 types.
            EAP-TTLS with different phase2 types.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network.
            2. Send a GET request to a website and check response.

        Expect:
            Successful connection and Internet access through the enterprise
            networks.
    """
    @test_tracker_info(uuid="4e720cac-ea17-4de7-a540-8dc7c49f9713")
    def test_eap_connect_with_config_tls(self):
        wutils.wifi_connect(self.dut, self.config_tls)

    @test_tracker_info(uuid="10e3a5e9-0018-4162-a9fa-b41500f13340")
    def test_eap_connect_with_config_pwd(self):
        wutils.wifi_connect(self.dut, self.config_pwd)

    @test_tracker_info(uuid="b4513f78-a1c4-427f-bfc7-2a6b3da714b5")
    def test_eap_connect_with_config_sim(self):
        wutils.wifi_connect(self.dut, self.config_sim)

    @test_tracker_info(uuid="7d390e30-cb67-4b55-bf00-567adad2d9b0")
    def test_eap_connect_with_config_aka(self):
        wutils.wifi_connect(self.dut, self.config_aka)

    @test_tracker_info(uuid="742f921b-27c3-4b68-a3ca-88e64fe79c1d")
    def test_eap_connect_with_config_aka_prime(self):
        wutils.wifi_connect(self.dut, self.config_aka_prime)

    @test_tracker_info(uuid="d34e30f3-6ef6-459f-b47a-e78ed90ce4c6")
    def test_eap_connect_with_config_ttls_none(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="0dca3a15-472e-427c-8e06-4e38088ee973")
    def test_eap_connect_with_config_ttls_pap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="47c4b459-2cb1-4fc7-b4e7-82534e8e090e")
    def test_eap_connect_with_config_ttls_mschap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="fdb286c7-8069-481d-baf0-c5dd7a31ff03")
    def test_eap_connect_with_config_ttls_mschapv2(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="d9315962-7987-4ee7-905d-6972c78ce8a1")
    def test_eap_connect_with_config_ttls_gtc(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="90a67bd3-30da-4daf-8ab0-d964d7ad19be")
    def test_eap_connect_with_config_peap0_mschapv2(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="3c451ba4-0c83-4eef-bc95-db4c21893008")
    def test_eap_connect_with_config_peap0_gtc(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="6b45157d-0325-417a-af18-11af5d240d79")
    def test_eap_connect_with_config_peap1_mschapv2(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="1663decc-71ae-4f95-a027-8a6dbf9c337f")
    def test_eap_connect_with_config_peap1_gtc(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        wutils.wifi_connect(self.dut, config)

    # EAP connect negative tests
    """ Test connecting to enterprise networks.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network with invalid credentials.

        Expect:
            Fail to establish connection.
    """
    @test_tracker_info(uuid="b2a91f1f-ccd7-4bd1-ab81-19aab3d8ee38")
    def test_eap_connect_negative_with_config_tls(self):
        config = self.gen_negative_eap_configs(self.config_tls)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="6466abde-1d16-4168-9dd8-1e7a0a19889b")
    def test_eap_connect_negative_with_config_pwd(self):
        config = self.gen_negative_eap_configs(self.config_pwd)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="d7742a2a-85b0-409a-99d8-47711ddc5612")
    def test_eap_connect_negative_with_config_sim(self):
        config = self.gen_negative_eap_configs(self.config_sim)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="0ec0de93-cab3-4f41-960b-c0af64ff48c4")
    def test_eap_connect_negative_with_config_aka(self):
        config = self.gen_negative_eap_configs(self.config_aka)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="bb640ea4-32a6-48ea-87c9-f7128fffbbf6")
    def test_eap_connect_negative_with_config_aka_prime(self):
        config = self.gen_negative_eap_configs(self.config_aka_prime)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="86336ada-0ced-45a4-8a22-c4aa23c81111")
    def test_eap_connect_negative_with_config_ttls_none(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="71e0498d-9973-4958-94bd-79051c328920")
    def test_eap_connect_negative_with_config_ttls_pap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="c04142a8-b204-4d2d-98dc-150b16c8397e")
    def test_eap_connect_negative_with_config_ttls_mschap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="625e7aa5-e3e6-4bbe-98c0-5aad8ca1555b")
    def test_eap_connect_negative_with_config_ttls_mschapv2(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="24ea0d80-0a3f-41c2-8e05-d6387e589058")
    def test_eap_connect_negative_with_config_ttls_gtc(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="b7c1f0f8-6338-4501-8e1d-c9b136aaba88")
    def test_eap_connect_negative_with_config_peap0_mschapv2(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="9cf83dcb-38ad-4f75-9ea9-98de1cfaf7f3")
    def test_eap_connect_negative_with_config_peap0_gtc(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="89bb2b6b-d073-402a-bdc1-68ac5f8752a3")
    def test_eap_connect_negative_with_config_peap1_mschapv2(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="2252a864-9ff7-43b5-82d9-afe57d1f5e5f")
    def test_eap_connect_negative_with_config_peap1_gtc(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        config = self.gen_negative_eap_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    # EAP connect config store tests
    """ Test connecting to enterprise networks of different authentication
        types after wifi toggle.

        The authentication types tested are:
            EAP-TLS
            EAP-PEAP with different phase2 types.
            EAP-TTLS with different phase2 types.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network.
            2. Send a GET request to a website and check response.
            3. Toggle wifi.
            4. Ensure that the device reconnects to the same network.

        Expect:
            Successful connection and Internet access through the enterprise
            networks.
    """
    @test_tracker_info(uuid="2a933b7f-27d7-4201-a34f-25b9d8072a8c")
    def test_eap_connect_config_store_with_config_tls(self):
        self.eap_connect_toggle_wifi(self.config_tls, self.dut)

    @test_tracker_info(uuid="08dc071b-9fea-408a-a3f6-d3493869f6d4")
    def test_eap_connect_config_store_with_config_pwd(self):
        self.eap_connect_toggle_wifi(self.config_pwd, self.dut)

    @test_tracker_info(uuid="230cb03e-58bc-41cb-b9b3-7215c2ab2325")
    def test_eap_connect_config_store_with_config_sim(self):
        self.eap_connect_toggle_wifi(self.config_sim, self.dut)

    @test_tracker_info(uuid="dfc3e59c-2309-4598-8c23-bb3fe95ef89f")
    def test_eap_connect_config_store_with_config_aka(self):
        self.eap_connect_toggle_wifi(self.config_aka, self.dut)

    @test_tracker_info(uuid="6050a1d1-4f3a-476d-bf93-638abd066790")
    def test_eap_connect_config_store_with_config_aka_prime(self):
        self.eap_connect_toggle_wifi(self.config_aka_prime, self.dut)

    @test_tracker_info(uuid="03108057-cc44-4a80-8331-77c93694099c")
    def test_eap_connect_config_store_with_config_ttls_none(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="53dd8195-e272-4589-a261-b8fa3607ad8d")
    def test_eap_connect_config_store_with_config_ttls_pap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="640f697b-9c62-4b19-bd76-53b236a152e0")
    def test_eap_connect_config_store_with_config_ttls_mschap(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="f0243684-fae0-46f3-afbd-bf525fc712e2")
    def test_eap_connect_config_store_with_config_ttls_mschapv2(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="49ec7202-3b00-49c3-970a-201360888c74")
    def test_eap_connect_config_store_with_config_ttls_gtc(self):
        config = dict(self.config_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="1c6abfa3-f344-4e28-b891-5481ab79efcf")
    def test_eap_connect_config_store_with_config_peap0_mschapv2(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="2815bc76-49fa-43a5-a4b6-84788f9809d5")
    def test_eap_connect_config_store_with_config_peap0_gtc(self):
        config = dict(self.config_peap0)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="e93f7472-6895-4e36-bff2-9b2dcfd07ad0")
    def test_eap_connect_config_store_with_config_peap1_mschapv2(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="6da72fa0-b858-4475-9559-46fe052d0d64")
    def test_eap_connect_config_store_with_config_peap1_gtc(self):
        config = dict(self.config_peap1)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.eap_connect_toggle_wifi(config, self.dut)

    # Removing 'test_' for all passpoint based testcases as we want to disable
    # them. Adding the valid test cases to self.tests make them run in serial
    # (TODO): gmoturu - Update the passpoint tests to test the valid scenario
    # Passpoint connect tests
    """ Test connecting to enterprise networks of different authentication
        types with passpoint support.

        The authentication types tested are:
            EAP-TLS
            EAP-TTLS with MSCHAPV2 as phase2.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network.
            2. Send a GET request to a website and check response.

        Expect:
            Successful connection and Internet access through the enterprise
            networks with passpoint support.
    """
    @test_tracker_info(uuid="0b942524-bde9-4fc6-ac6a-fef1c247cb8e")
    def passpoint_connect_with_config_passpoint_tls(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        wutils.wifi_connect(self.dut, self.config_passpoint_tls)

    @test_tracker_info(uuid="33a014aa-99e7-4612-b732-54fabf1bf922")
    def passpoint_connect_with_config_passpoint_ttls_none(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="1aba8bf9-2b09-4956-b418-c3f4dadab330")
    def passpoint_connect_with_config_passpoint_ttls_pap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="cd978fc9-a393-4b1e-bba3-1efc52224500")
    def passpoint_connect_with_config_passpoint_ttls_mschap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="bc311ee7-ba64-4c76-a629-b916701bf6a5")
    def passpoint_connect_with_config_passpoint_ttls_mschapv2(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        wutils.wifi_connect(self.dut, config)

    @test_tracker_info(uuid="357e5162-5081-4149-bedd-ef2c0f88b97e")
    def passpoint_connect_with_config_passpoint_ttls_gtc(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        wutils.wifi_connect(self.dut, config)

    # Passpoint connect negative tests
    """ Test connecting to enterprise networks.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network with invalid credentials.

        Expect:
            Fail to establish connection.
    """
    @test_tracker_info(uuid="7b6b44a0-ff70-49b4-94ca-a98bedc18f92")
    def passpoint_connect_negative_with_config_passpoint_tls(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = self.gen_negative_passpoint_configs(self.config_passpoint_tls)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="3dbde40a-e88c-4166-b932-163663a10a41")
    def passpoint_connect_negative_with_config_passpoint_ttls_none(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        config = self.gen_negative_passpoint_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="8ee22ad6-d561-4ca2-a808-9f372fce56b4")
    def passpoint_connect_negative_with_config_passpoint_ttls_pap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        config = self.gen_negative_passpoint_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="db5cefe7-9cb8-47a6-8635-006c80b97012")
    def passpoint_connect_negative_with_config_passpoint_ttls_mschap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        config = self.gen_negative_passpoint_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="8f49496e-80df-48ce-9c51-42f0c6b81aff")
    def passpoint_connect_negative_with_config_passpoint_ttls_mschapv2(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        config = self.gen_negative_passpoint_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    @test_tracker_info(uuid="6561508f-598e-408d-96b6-15b631664be6")
    def passpoint_connect_negative_with_config_passpoint_ttls_gtc(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        config = self.gen_negative_passpoint_configs(config)
        self.eap_negative_connect_logic(config, self.dut)

    # Passpoint connect config store tests
    """ Test connecting to enterprise networks of different authentication
        types with passpoint support after wifi toggle.

        The authentication types tested are:
            EAP-TLS
            EAP-TTLS with MSCHAPV2 as phase2.

        Procedures:
            For each enterprise wifi network
            1. Connect to the network.
            2. Send a GET request to a website and check response.
            3. Toggle wifi.
            4. Ensure that the device reconnects to the same network.

        Expect:
            Successful connection and Internet access through the enterprise
            networks with passpoint support.
    """
    @test_tracker_info(uuid="5d5e6bb0-faea-4a6e-a6bc-c87de997a4fd")
    def passpoint_connect_config_store_with_config_passpoint_tls(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        self.eap_connect_toggle_wifi(self.config_passpoint_tls, self.dut)

    @test_tracker_info(uuid="0c80262d-23c1-439f-ad64-7b8ada5d1962")
    def passpoint_connect_config_store_with_config_passpoint_ttls_none(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.NONE.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="786e424c-b5a6-4fe9-a951-b3de16ebb6db")
    def passpoint_connect_config_store_with_config_passpoint_ttls_pap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.PAP.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="22fd61bf-722a-4016-a778-fc33e94ed211")
    def passpoint_connect_config_store_with_config_passpoint_ttls_mschap(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAP.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="2abd348c-9c66-456b-88ad-55f971717620")
    def passpoint_connect_config_store_with_config_passpoint_ttls_mschapv2(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.MSCHAPV2.value
        self.eap_connect_toggle_wifi(config, self.dut)

    @test_tracker_info(uuid="043e8cdd-db95-4f03-b308-3c8cecf874b1")
    def passpoint_connect_config_store_with_config_passpoint_ttls_gtc(self):
        asserts.skip_if(not self.dut.droid.wifiIsPasspointSupported(),
                        "Passpoint is not supported on %s" % self.dut.model)
        config = dict(self.config_passpoint_ttls)
        config[WifiEnums.Enterprise.PHASE2] = WifiEnums.EapPhase2.GTC.value
        self.eap_connect_toggle_wifi(config, self.dut)
