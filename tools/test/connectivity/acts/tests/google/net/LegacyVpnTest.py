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
import os
import pprint
import re
import time
import urllib.request

from acts import asserts
from acts import base_test
from acts import test_runner
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.wifi import wifi_test_utils
from acts.test_utils.net import connectivity_const

VPN_CONST = connectivity_const.VpnProfile
VPN_TYPE = connectivity_const.VpnProfileType
VPN_PARAMS = connectivity_const.VpnReqParams


class LegacyVpnTest(base_test.BaseTestClass):
    """ Tests for Legacy VPN in Android

        Testbed requirement:
            1. One Android device
            2. A Wi-Fi network that can reach the VPN servers
    """

    def setup_class(self):
        """ Setup wi-fi connection and unpack params
        """
        self.dut = self.android_devices[0]
        required_params = dir(VPN_PARAMS)
        required_params = [x for x in required_params if not x.startswith('__')]
        self.unpack_userparams(required_params)
        wifi_test_utils.wifi_test_device_init(self.dut)
        wifi_test_utils.wifi_connect(self.dut, self.wifi_network)
        time.sleep(3)

    def teardown_class(self):
        """ Reset wifi to make sure VPN tears down cleanly
        """
        wifi_test_utils.reset_wifi(self.dut)

    def on_fail(self, test_name, begin_time):
        self.dut.take_bug_report(test_name, begin_time)

    def download_load_certs(self, vpn_type, vpn_server_addr, ipsec_server_type):
        """ Download the certificates from VPN server and push to sdcard of DUT

            Args:
                VPN type name

            Returns:
                Client cert file name on DUT's sdcard
        """
        url = "http://%s%s%s" % (vpn_server_addr,
                                 self.cert_path_vpnserver,
                                 self.client_pkcs_file_name)
        local_cert_name = "%s_%s_%s" % (vpn_type.name,
                                        ipsec_server_type,
                                        self.client_pkcs_file_name)
        local_file_path = os.path.join(self.log_path, local_cert_name)
        try:
            ret = urllib.request.urlopen(url)
            with open(local_file_path, "wb") as f:
                f.write(ret.read())
        except:
            asserts.fail("Unable to download certificate from the server")
        f.close()
        self.dut.adb.push("%s sdcard/" % local_file_path)
        return local_cert_name

    def generate_legacy_vpn_profile(self, vpn_type, vpn_server_addr, ipsec_server_type):
        """ Generate legacy VPN profile for a VPN

            Args:
                VpnProfileType
        """
        vpn_profile = {VPN_CONST.USER: self.vpn_username,
                       VPN_CONST.PWD: self.vpn_password,
                       VPN_CONST.TYPE: vpn_type.value,
                       VPN_CONST.SERVER: vpn_server_addr,}
        vpn_profile[VPN_CONST.NAME] = "test_%s_%s" % (vpn_type.name,ipsec_server_type)
        if vpn_type.name == "PPTP":
            vpn_profile[VPN_CONST.NAME] = "test_%s" % vpn_type.name
        psk_set = set(["L2TP_IPSEC_PSK", "IPSEC_XAUTH_PSK"])
        rsa_set = set(["L2TP_IPSEC_RSA", "IPSEC_XAUTH_RSA", "IPSEC_HYBRID_RSA"])
        if vpn_type.name in psk_set:
            vpn_profile[VPN_CONST.IPSEC_SECRET] = self.psk_secret
        elif vpn_type.name in rsa_set:
            cert_name = self.download_load_certs(vpn_type,
                                                 vpn_server_addr,
                                                 ipsec_server_type)
            vpn_profile[VPN_CONST.IPSEC_USER_CERT] = cert_name.split('.')[0]
            vpn_profile[VPN_CONST.IPSEC_CA_CERT] = cert_name.split('.')[0]
            self.dut.droid.installCertificate(vpn_profile,cert_name,self.cert_password)
        else:
            vpn_profile[VPN_CONST.MPPE] = self.pptp_mppe
        return vpn_profile

    def verify_ping_to_vpn_ip(self, connected_vpn_info):
        """ Verify if IP behind VPN server is pingable
            Ping should pass, if VPN is connected
            Ping should fail, if VPN is disconnected

            Args:
                connected_vpn_info which specifies the VPN connection status
        """
        ping_result = None
        pkt_loss = "100% packet loss"
        try:
            ping_result = self.dut.adb.shell("ping -c 3 -W 2 %s"
                                             % self.vpn_verify_address)
        except adb.AdbError:
            pass
        return ping_result and pkt_loss not in ping_result

    def legacy_vpn_connection_test_logic(self, vpn_profile):
        """ Test logic for each legacy VPN connection

            Steps:
                1. Generate profile for the VPN type
                2. Establish connection to the server
                3. Verify that connection is established using LegacyVpnInfo
                4. Verify the connection by pinging the IP behind VPN
                5. Stop the VPN connection
                6. Check the connection status
                7. Verify that ping to IP behind VPN fails

            Args:
                VpnProfileType (1 of the 6 types supported by Android)
        """
        # Wait for sometime so that VPN server flushes all interfaces and
        # connections after graceful termination
        time.sleep(10)
        self.dut.adb.shell("ip xfrm state flush")
        logging.info("Connecting to: %s", vpn_profile)
        self.dut.droid.vpnStartLegacyVpn(vpn_profile)
        time.sleep(connectivity_const.VPN_TIMEOUT)
        connected_vpn_info = self.dut.droid.vpnGetLegacyVpnInfo()
        asserts.assert_equal(connected_vpn_info["state"],
                             connectivity_const.VPN_STATE_CONNECTED,
                             "Unable to establish VPN connection for %s"
                             % vpn_profile)
        ping_result = self.verify_ping_to_vpn_ip(connected_vpn_info)
        ip_xfrm_state = self.dut.adb.shell("ip xfrm state")
        match_obj = re.search(r'hmac(.*)', "%s" % ip_xfrm_state)
        if match_obj:
            ip_xfrm_state = format(match_obj.group(0)).split()
            self.log.info("HMAC for ESP is %s " % ip_xfrm_state[0])
        self.dut.droid.vpnStopLegacyVpn()
        asserts.assert_true(ping_result,
                            "Ping to the internal IP failed. "
                            "Expected to pass as VPN is connected")
        connected_vpn_info = self.dut.droid.vpnGetLegacyVpnInfo()
        asserts.assert_true(not connected_vpn_info,
                            "Unable to terminate VPN connection for %s"
                            % vpn_profile)

    """ Test Cases """
    @test_tracker_info(uuid="d2ac5a65-41fb-48de-a0a9-37e589b5456b")
    def test_legacy_vpn_pptp(self):
        """ Verify PPTP VPN connection """
        vpn = VPN_TYPE.PPTP
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[2])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="99af78dd-40b8-483a-8344-cd8f67594971")
    def legacy_vpn_l2tp_ipsec_psk_libreswan(self):
        """ Verify L2TP IPSec PSK VPN connection to
            libreSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][2],
            self.ipsec_server_type[2])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="e67d8c38-92c3-4167-8b6c-a49ef939adce")
    def legacy_vpn_l2tp_ipsec_rsa_libreswan(self):
        """ Verify L2TP IPSec RSA VPN connection to
            libreSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][2],
            self.ipsec_server_type[2])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="8b3517dc-6a3b-44c2-a85d-bd7b969df3cf")
    def legacy_vpn_ipsec_xauth_psk_libreswan(self):
        """ Verify IPSec XAUTH PSK VPN connection to
            libreSwan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][2],
            self.ipsec_server_type[2])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="abac663d-1d91-4b87-8e94-11c6e44fb07b")
    def legacy_vpn_ipsec_xauth_rsa_libreswan(self):
        """ Verify IPSec XAUTH RSA VPN connection to
            libreSwan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][2],
            self.ipsec_server_type[2])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="84140d24-53c0-4f6c-866f-9d66e04442cc")
    def test_legacy_vpn_l2tp_ipsec_psk_openswan(self):
        """ Verify L2TP IPSec PSK VPN connection to
            openSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][1],
            self.ipsec_server_type[1])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="f7087592-7eed-465d-bfe3-ed7b6d9d5f9a")
    def test_legacy_vpn_l2tp_ipsec_rsa_openswan(self):
        """ Verify L2TP IPSec RSA VPN connection to
            openSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][1],
            self.ipsec_server_type[1])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="ed78973b-13ee-4dd4-b998-693ab741c6f8")
    def test_legacy_vpn_ipsec_xauth_psk_openswan(self):
        """ Verify IPSec XAUTH PSK VPN connection to
            openSwan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][1],
            self.ipsec_server_type[1])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="cfd125c4-b64c-4c49-b8e4-fbf05a9be8ec")
    def test_legacy_vpn_ipsec_xauth_rsa_openswan(self):
        """ Verify IPSec XAUTH RSA VPN connection to
            openSwan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][1],
            self.ipsec_server_type[1])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="419370de-0aa1-4a56-8c22-21567fa1cbb7")
    def test_legacy_vpn_l2tp_ipsec_psk_strongswan(self):
        """ Verify L2TP IPSec PSk VPN connection to
            strongSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[0])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="f7694081-8bd6-4e31-86ec-d538c4ff1f2e")
    def test_legacy_vpn_l2tp_ipsec_rsa_strongswan(self):
        """ Verify L2TP IPSec RSA VPN connection to
            strongSwan server
        """
        vpn = VPN_TYPE.L2TP_IPSEC_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[0])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="2f86eb98-1e05-42cb-b6a6-fd90789b6cde")
    def test_legacy_vpn_ipsec_xauth_psk_strongswan(self):
        """ Verify IPSec XAUTH PSK connection to
            strongSwan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_PSK
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[0])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="af0cd7b1-e86c-4327-91b4-e9062758f2cf")
    def test_legacy_vpn_ipsec_xauth_rsa_strongswan(self):
        """ Verify IPSec XAUTH RSA connection to
            strongswan server
        """
        vpn = VPN_TYPE.IPSEC_XAUTH_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[0])
        self.legacy_vpn_connection_test_logic(vpn_profile)

    @test_tracker_info(uuid="7b970d0a-1c7d-4a5a-b406-4815e190ef26")
    def test_legacy_vpn_ipsec_hybrid_rsa_strongswan(self):
        """ Verify IPSec Hybrid RSA connection to
            strongswan server
        """
        vpn = VPN_TYPE.IPSEC_HYBRID_RSA
        vpn_profile = self.generate_legacy_vpn_profile(
            vpn, self.vpn_server_addresses[vpn.name][0],
            self.ipsec_server_type[0])
        self.legacy_vpn_connection_test_logic(vpn_profile)
