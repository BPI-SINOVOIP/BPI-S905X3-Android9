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

import logging
import os
import random
import socket
import threading
import time

from acts import asserts
from acts import base_test
from acts import test_runner
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.tel.tel_data_utils import wait_for_cell_data_connection
from acts.test_utils.tel.tel_test_utils import start_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import stop_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import verify_http_connection
from acts.test_utils.wifi import wifi_test_utils as wutils

from scapy.all import TCP
from scapy.all import UDP
from scapy.all import rdpcap

DNS_QUAD9 = "dns.quad9.net"
PRIVATE_DNS_MODE_OFF = "off"
PRIVATE_DNS_MODE_OPPORTUNISTIC = "opportunistic"
PRIVATE_DNS_MODE_STRICT = "hostname"
RST = 0x04

class DnsOverTlsTest(base_test.BaseTestClass):
    """ Tests for Wifi Tethering """

    def setup_class(self):
        """ Setup devices for tethering and unpack params """

        self.dut = self.android_devices[0]
        wutils.reset_wifi(self.dut)
        self.dut.droid.telephonyToggleDataConnection(True)
        wait_for_cell_data_connection(self.log, self.dut, True)
        asserts.assert_true(
            verify_http_connection(self.log, self.dut),
            "HTTP verification failed on cell data connection")
        req_params = ("wifi_network_with_dns_tls", "wifi_network_no_dns_tls",
                      "ping_hosts")
        self.unpack_userparams(req_params)
        self.tcpdump_pid = None
        self.tcpdump_file = None

    """ Helper functions """

    def _start_tcp_dump(self, ad):
        """ Start tcpdump on the give dut

        Args:
            1. ad: dut to run tcpdump on
        """
        if self.tcpdump_pid:
            stop_adb_tcpdump(ad, self.tcpdump_pid, pull_tcpdump=False)
        self.tcpdump_pid = start_adb_tcpdump(ad, self.test_name, mask='all')

    def _stop_tcp_dump(self, ad):
        """ Stop tcpdump and pull it to the test run logs

        Args:
            1. ad: dut to pull tcpdump from
        """
        file_name = ad.adb.shell("ls /sdcard/tcpdump")
        file_name = os.path.join(ad.log_path, "TCPDUMP_%s" % ad.serial,
                                 file_name.split('/')[-1])
        if self.tcpdump_pid:
            stop_adb_tcpdump(ad, self.tcpdump_pid, pull_tcpdump=True)
            self.tcpdump_pid = None
        return os.path.join(ad.log_path, file_name)

    def _verify_dns_queries_over_tls(self, pcap_file, tls=True):
        """ Verify if DNS queries were over TLS or not

        Args:
            1. pcap_file: tcpdump file
            2. tls: if queries should be over TLS or port 853
        """
        try:
            packets = rdpcap(pcap_file)
        except Scapy_Exception:
            asserts.fail("Not a valid pcap file")
        for pkt in packets:
            summary = "%s" % pkt.summary()
            if tls and UDP in pkt and pkt[UDP].dport == 53 and \
                "connectivitycheck.gstatic.com." not in summary and \
                "www.google.com" not in summary:
                asserts.fail("Found query to port 53: %s" % summary)
            elif not tls and TCP in pkt and pkt[TCP].dport == 853 and \
                not pkt[TCP].flags:
                asserts.fail("Found query to port 853: %s" % summary)

    def _verify_rst_packets(self, pcap_file):
        """ Verify if RST packets are found in the pcap file """
        packets = rdpcap(pcap_file)
        for pkt in packets:
            if TCP in pkt and pkt[TCP].flags == RST:
                asserts.fail("Found RST packets: %s" % pkt.summary())

    def _test_private_dns_mode(self, network, dns_mode, use_tls,
                               hostname = None):
        """ Test private DNS mode """
        # connect to wifi
        wutils.reset_wifi(self.dut)
        if network:
            wutils.wifi_connect(self.dut, network)
        time.sleep(1) # wait till lte network becomes active - network = None

        # start tcpdump on the device
        self._start_tcp_dump(self.dut)

        # set private dns mode
        if dns_mode == PRIVATE_DNS_MODE_OFF:
            self.dut.droid.setPrivateDnsMode(False)
        elif hostname:
            self.dut.droid.setPrivateDnsMode(True, hostname)
        else:
            self.dut.droid.setPrivateDnsMode(True)
        mode = self.dut.droid.getPrivateDnsMode()
        asserts.assert_true(mode == dns_mode,
                            "Failed to set private DNS mode to %s" % dns_mode)

        # ping hosts should pass
        for host in self.ping_hosts:
            self.log.info("Pinging %s" % host)
            asserts.assert_true(wutils.validate_connection(self.dut, host),
                                "Failed to ping host %s" % host)

        # stop tcpdump
        pcap_file = self._stop_tcp_dump(self.dut)
        self.log.info("TCPDUMP file is: %s" % pcap_file)

        # verify DNS queries
        self._verify_dns_queries_over_tls(pcap_file, use_tls)

    """ Test Cases """

    @test_tracker_info(uuid="2957e61c-d333-45fb-9ff9-2250c9c8535a")
    def test_private_dns_mode_off_wifi_no_dns_tls_server(self):
        """ Verify private dns mode off

        Steps:
            1. Set private dns mode off
            2. Connect to wifi network. DNS/TLS server is not set
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 53
        """
        self._test_private_dns_mode(self.wifi_network_no_dns_tls,
                                    PRIVATE_DNS_MODE_OFF, False)

    @test_tracker_info(uuid="ea036d22-25af-4df0-b6cc-0027bc1efbe9")
    def test_private_dns_mode_off_wifi_with_dns_tls_server(self):
        """ Verify private dns mode off

        Steps:
            1. Set private dns mode off
            2. Connect to wifi network. DNS server is set to 9.9.9.9, 8.8.8.8
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 53
        """
        self._test_private_dns_mode(self.wifi_network_with_dns_tls,
                                    PRIVATE_DNS_MODE_OFF, False)

    @test_tracker_info(uuid="4227abf4-0a75-4b4d-968c-dfc63052f5db")
    def test_private_dns_mode_opportunistic_wifi_no_dns_tls_server(self):
        """ Verify private dns opportunistic mode

        Steps:
            1. Set private dns mode to opportunistic
            2. Connect to wifi network. DNS/TLS server is not set
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 53
        """
        self._test_private_dns_mode(self.wifi_network_no_dns_tls,
                                    PRIVATE_DNS_MODE_OPPORTUNISTIC, False)

    @test_tracker_info(uuid="0c97cfef-4313-4346-b05b-395de63c5c3f")
    def test_private_dns_mode_opportunistic_wifi_with_dns_tls_server(self):
        """ Verify private dns opportunistic mode

        Steps:
            1. Set private dns mode to opportunistic
            2. Connect to wifi network. DNS server is set to 9.9.9.9, 8.8.8.8
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 853
        """
        self._test_private_dns_mode(self.wifi_network_with_dns_tls,
                                    PRIVATE_DNS_MODE_OPPORTUNISTIC, True)

    @test_tracker_info(uuid="b70569f1-2613-49d0-be49-fd3464dde305")
    def test_private_dns_mode_strict_wifi_no_dns_tls_server(self):
        """ Verify private dns strict mode

        Steps:
            1. Set private dns mode to strict
            2. Connect to wifi network. DNS/TLS server is not set
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 853
        """
        self._test_private_dns_mode(self.wifi_network_no_dns_tls,
                                    PRIVATE_DNS_MODE_STRICT, True,
                                    DNS_QUAD9)

    @test_tracker_info(uuid="85738b52-823b-4c59-a0d5-219e2fab2929")
    def test_private_dns_mode_strict_wifi_with_dns_tls_server(self):
        """ Verify private dns strict mode

        Steps:
            1. Set private dns mode to strict
            2. Connect to wifi network. DNS server is set to 9.9.9.9, 8.8.8.8
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 853
        """
        self._test_private_dns_mode(self.wifi_network_with_dns_tls,
                                    PRIVATE_DNS_MODE_STRICT, True,
                                    DNS_QUAD9)

    @test_tracker_info(uuid="727e280a-d2bd-463f-b2a1-653d4b3f7f29")
    def test_private_dns_mode_off_lte(self):
        """ Verify private dns off mode

        Steps:
            1. Set private dns mode to off
            2. Reset wifi and enable LTE on DUT
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 53
        """
        self._test_private_dns_mode(None, PRIVATE_DNS_MODE_OFF, False)

    @test_tracker_info(uuid="b16f6e2c-a24f-4efe-9003-2bfaf28b8d5e")
    def test_private_dns_mode_opportunistic_lte(self):
        """ Verify private dns opportunistic mode

        Steps:
            1. Set private dns mode to opportunistic mode
            2. Reset wifi and enable LTE on DUT
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 853
        """
        self._test_private_dns_mode(None, PRIVATE_DNS_MODE_OPPORTUNISTIC, True)

    @test_tracker_info(uuid="edfa7bfe-3e52-46b4-9d72-7c6c300b3680")
    def test_private_dns_mode_strict_lte(self):
        """ Verify private dns strict mode

        Steps:
            1. Set private dns mode to strict mode
            2. Reset wifi and enable LTE on DUT
            3. Verify ping works to differnt hostnames
            4. Verify that all queries go to port 853
        """
        self._test_private_dns_mode(None, PRIVATE_DNS_MODE_STRICT, True,
                                    DNS_QUAD9)

    @test_tracker_info(uuid="1426673a-7728-4df7-8de5-dfb3529ada62")
    def test_dns_server_link_properties_strict_mode(self):
        """ Verify DNS server in the link properties when set in strict mode

        Steps:
            1. Set DNS server hostname in Private DNS settings (stict mode)
            2. Verify that DNS server set in settings is in link properties
            3. Verify for WiFi as well as LTE
        """
        # start tcpdump on device
        self._start_tcp_dump(self.dut)

        # set private DNS to strict mode
        self.dut.droid.setPrivateDnsMode(True, DNS_QUAD9)
        mode = self.dut.droid.getPrivateDnsMode()
        specifier = self.dut.droid.getPrivateDnsSpecifier()
        asserts.assert_true(
            mode == PRIVATE_DNS_MODE_STRICT and specifier == DNS_QUAD9,
            "Failed to set private DNS strict mode")

        # connect DUT to wifi network
        wutils.wifi_connect(self.dut, self.wifi_network_no_dns_tls)
        for host in self.ping_hosts:
            wutils.validate_connection(self.dut, host)

        # DNS server in link properties for wifi network
        link_prop = self.dut.droid.connectivityGetActiveLinkProperties()
        dns_servers = link_prop['DnsServers']
        wifi_dns_servers = [each for lst in dns_servers for each in lst]
        self.log.info("Link prop: %s" % wifi_dns_servers)

        # DUT is on LTE data
        wutils.reset_wifi(self.dut)
        time.sleep(1) # wait till lte network becomes active
        for host in self.ping_hosts:
            wutils.validate_connection(self.dut, host)

        # DNS server in link properties for cell network
        link_prop = self.dut.droid.connectivityGetActiveLinkProperties()
        dns_servers = link_prop['DnsServers']
        lte_dns_servers = [each for lst in dns_servers for each in lst]
        self.log.info("Link prop: %s" % lte_dns_servers)

        # stop tcpdump on device
        pcap_file = self._stop_tcp_dump(self.dut)
        self.log.info("TCPDUMP file is: %s" % pcap_file)

        # Verify DNS server in link properties
        asserts.assert_true(DNS_QUAD9 in wifi_dns_servers,
                            "Hostname not in link properties - wifi network")
        asserts.assert_true(DNS_QUAD9 in lte_dns_servers,
                            "Hostname not in link properites - cell network")

    @test_tracker_info(uuid="525a6f2d-9751-474e-a004-52441091e427")
    def test_dns_over_tls_no_reset_packets(self):
        """ Verify there are no TCP packets with RST flags

        Steps:
            1. Enable opportunistic or strict mode
            2. Ping hosts and verify that there are TCP pkts with RST flags
        """
        # start tcpdump on device
        self._start_tcp_dump(self.dut)

        # set private DNS to opportunistic mode
        self.dut.droid.setPrivateDnsMode(True)
        mode = self.dut.droid.getPrivateDnsMode()
        asserts.assert_true(mode == PRIVATE_DNS_MODE_OPPORTUNISTIC,
                            "Failed to set private DNS opportunistic mode")

        # connect DUT to wifi network
        wutils.wifi_connect(self.dut, self.wifi_network_with_dns_tls)
        for host in self.ping_hosts:
            wutils.validate_connection(self.dut, host)

        # stop tcpdump on device
        pcap_file = self._stop_tcp_dump(self.dut)
        self.log.info("TCPDUMP file is: %s" % pcap_file)

        # check that there no RST TCP packets
        self._verify_rst_packets(pcap_file)

    @test_tracker_info(uuid="af6e34f1-3ad5-4ab0-b3b9-53008aa08294")
    def test_private_dns_mode_strict_invalid_hostnames(self):
        """ Verify that invalid hostnames are not saved for strict mode

        Steps:
            1. Set private DNS to strict mode with invalid hostname
            2. Verify that invalid hostname is not saved
        """
        invalid_hostnames = ["!%@&!*", "12093478129", "9.9.9.9", "sdkfjhasdf"]
        for hostname in invalid_hostnames:
            self.dut.droid.setPrivateDnsMode(True, hostname)
            mode = self.dut.droid.getPrivateDnsMode()
            specifier = self.dut.droid.getPrivateDnsSpecifier()
            wutils.wifi_connect(self.dut, self.wifi_network_no_dns_tls)
            asserts.assert_true(
                mode == PRIVATE_DNS_MODE_STRICT and specifier != hostname,
                "Able to set invalid private DNS strict mode")
