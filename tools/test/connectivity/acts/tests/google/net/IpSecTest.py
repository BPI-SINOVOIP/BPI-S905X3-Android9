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

from acts import asserts
from acts import base_test
from acts.controllers import adb
from acts.test_decorators import test_tracker_info
from acts.test_utils.net import connectivity_const as cconst
from acts.test_utils.net import ipsec_test_utils as iutils
from acts.test_utils.net import socket_test_utils as sutils
from acts.test_utils.tel.tel_test_utils import start_adb_tcpdump
from acts.test_utils.tel.tel_test_utils import stop_adb_tcpdump
from acts.test_utils.wifi import wifi_test_utils as wutils

import random

WLAN = "wlan0"


class IpSecTest(base_test.BaseTestClass):
    """ Tests for UID networking """

    def setup_class(self):
        """ Setup devices for tests and unpack params """
        self.dut_a = self.android_devices[0]
        self.dut_b = self.android_devices[1]

        req_params = ("wifi_network",)
        self.unpack_userparams(req_params)
        wutils.wifi_connect(self.dut_a, self.wifi_network)
        wutils.wifi_connect(self.dut_b, self.wifi_network)

        self.ipv4_dut_a = self.dut_a.droid.connectivityGetIPv4Addresses(WLAN)[0]
        self.ipv4_dut_b = self.dut_b.droid.connectivityGetIPv4Addresses(WLAN)[0]
        self.ipv6_dut_a = self.dut_a.droid.connectivityGetIPv6Addresses(WLAN)[0]
        self.ipv6_dut_b = self.dut_b.droid.connectivityGetIPv6Addresses(WLAN)[0]

        self.tcpdump_pid_a = None
        self.tcpdump_file_a = None
        self.tcpdump_pid_b = None
        self.tcpdump_file_b = None

        self.crypt_auth_combos = iutils.generate_random_crypt_auth_combo()

    def teardown_class(self):
        for ad in self.android_devices:
            wutils.reset_wifi(ad)

    def setup_test(self):
        self.tcpdump_pid_a = start_adb_tcpdump(
            self.dut_a, self.test_name, mask='all')
        self.tcpdump_pid_b = start_adb_tcpdump(
            self.dut_b, self.test_name, mask='all')

    def teardown_test(self):
        if self.tcpdump_pid_a:
            stop_adb_tcpdump(
                self.dut_a, self.tcpdump_pid_a, pull_tcpdump=True)
        if self.tcpdump_pid_b:
            stop_adb_tcpdump(
                self.dut_b, self.tcpdump_pid_b, pull_tcpdump=True)
        self.tcpdump_pid_a = None
        self.tcpdump_pid_b = None

    def on_fail(self, test_name, begin_time):
        self.dut_a.take_bug_report(test_name, begin_time)
        self.dut_b.take_bug_report(test_name, begin_time)

    """ Helper functions begin """

    def _test_spi_allocate_release_req_spi(self, dut, ip):
        req_spi = random.randint(1, 999999999)
        self.log.info("IP addr: %s" % ip)
        self.log.info("Request SPI: %s" % req_spi)
        key = dut.droid.ipSecAllocateSecurityParameterIndex(ip, req_spi)
        spi = dut.droid.ipSecGetSecurityParameterIndex(key)
        self.log.info("Got: %s" % spi)
        dut.droid.ipSecReleaseSecurityParameterIndex(key)
        asserts.assert_true(req_spi == spi, "Incorrect SPI")
        spi = dut.droid.ipSecGetSecurityParameterIndex(key)
        self.log.info("SPI after release: %s" % spi)
        asserts.assert_true(spi == 0, "Release SPI failed")

    def _test_spi_allocate_release_random_spi(self, dut, ip):
        self.log.info("IP addr: %s" % ip)
        key = dut.droid.ipSecAllocateSecurityParameterIndex(ip)
        spi = dut.droid.ipSecGetSecurityParameterIndex(key)
        self.log.info("Got: %s" % spi)
        dut.droid.ipSecReleaseSecurityParameterIndex(key)
        spi = dut.droid.ipSecGetSecurityParameterIndex(key)
        self.log.info("SPI after release: %s" % spi)
        asserts.assert_true(spi == 0, "Release SPI failed")

    def _test_transport_mode_transform_file_descriptors(self,
                                                        domain,
                                                        sock_type,
                                                        udp_encap = False):
        """Test transport mode tranform with android.system.Os sockets"""

        # dut objects & ip addrs
        dut_a = self.dut_a
        dut_b = self.dut_b
        port = random.randint(5000, 6000)
        udp_encap_port = 4500
        ip_a = self.ipv4_dut_a
        ip_b = self.ipv4_dut_b
        if domain == cconst.AF_INET6:
            ip_a = self.ipv6_dut_a
            ip_b = self.ipv6_dut_b
        self.log.info("DUT_A IP addr: %s" % ip_a)
        self.log.info("DUT_B IP addr: %s" % ip_b)
        self.log.info("Port: %s" % port)

        # create crypt and auth keys
        cl, auth_algo, al, trunc_bits = random.choice(self.crypt_auth_combos)
        crypt_key = iutils.make_key(cl)
        auth_key = iutils.make_key(al)
        crypt_algo = cconst.CRYPT_AES_CBC

        # open sockets
        fd_a = sutils.open_android_socket(dut_a, domain, sock_type, ip_a, port)
        fd_b = sutils.open_android_socket(dut_b, domain, sock_type, ip_b, port)

        # allocate SPIs
        spi_keys_a = iutils.allocate_spis(dut_a, ip_a, ip_b)
        in_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[0])
        out_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[1])
        spi_keys_b = iutils.allocate_spis(dut_b, ip_b, ip_a, out_spi, in_spi)

        # open udp encap sockets
        udp_encap_a = None
        udp_encap_b = None
        if udp_encap:
            udp_encap_a = dut_a.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
            udp_encap_b = dut_b.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
        self.log.info("UDP Encap: %s" % udp_encap_a)
        self.log.info("UDP Encap: %s" % udp_encap_b)

        # create transforms
        transforms_a = iutils.create_transport_mode_transforms(
            dut_a, spi_keys_a, ip_a, ip_b, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_a)
        transforms_b = iutils.create_transport_mode_transforms(
            dut_b, spi_keys_b, ip_b, ip_a, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_b)

        # apply transforms to socket
        iutils.apply_transport_mode_transforms_file_descriptors(
            dut_a, fd_a, transforms_a)
        iutils.apply_transport_mode_transforms_file_descriptors(
            dut_b, fd_b, transforms_b)

        # listen, connect, accept if TCP socket
        if sock_type == cconst.SOCK_STREAM:
            server_fd = fd_b
            self.log.info("Setup TCP connection")
            dut_b.droid.listenSocket(fd_b)
            dut_a.droid.connectSocket(fd_a, ip_b, port)
            fd_b = dut_b.droid.acceptSocket(fd_b)
            asserts.assert_true(fd_b, "accept() failed")

        # Send message from one dut to another
        sutils.send_recv_data_android_sockets(
            dut_a, dut_b, fd_a, fd_b, ip_b, port)

        # verify ESP packets
        iutils.verify_esp_packets([dut_a, dut_b])

        # remove transforms from socket
        iutils.remove_transport_mode_transforms_file_descriptors(dut_a, fd_a)
        iutils.remove_transport_mode_transforms_file_descriptors(dut_b, fd_b)

        # destroy transforms
        iutils.destroy_transport_mode_transforms(dut_a, transforms_a)
        iutils.destroy_transport_mode_transforms(dut_b, transforms_b)

        # close udp encap sockets
        if udp_encap:
            dut_a.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_a)
            dut_b.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_b)

        # release SPIs
        iutils.release_spis(dut_a, spi_keys_a)
        iutils.release_spis(dut_b, spi_keys_b)

        # Send message from one dut to another
        sutils.send_recv_data_android_sockets(
            dut_a, dut_b, fd_a, fd_b, ip_b, port)

        # close sockets
        sutils.close_android_socket(dut_a, fd_a)
        sutils.close_android_socket(dut_b, fd_b)
        if sock_type == cconst.SOCK_STREAM:
            sutils.close_android_socket(dut_b, server_fd)

    def _test_transport_mode_transform_datagram_sockets(self,
                                                        domain,
                                                        udp_encap = False):
        """ Test transport mode transform datagram sockets """

        # dut objects and ip addrs
        dut_a = self.dut_a
        dut_b = self.dut_b
        port = random.randint(5000, 6000)
        udp_encap_port = 4500
        ip_a = self.ipv4_dut_a
        ip_b = self.ipv4_dut_b
        if domain == cconst.AF_INET6:
            ip_a = self.ipv6_dut_a
            ip_b = self.ipv6_dut_b
        self.log.info("DUT_A IP addr: %s" % ip_a)
        self.log.info("DUT_B IP addr: %s" % ip_b)
        self.log.info("Port: %s" % port)

        # create crypt and auth keys
        cl, auth_algo, al, trunc_bits = random.choice(self.crypt_auth_combos)
        crypt_key = iutils.make_key(cl)
        auth_key = iutils.make_key(al)
        crypt_algo = cconst.CRYPT_AES_CBC

        # open sockets
        socket_a = sutils.open_datagram_socket(dut_a, ip_a, port)
        socket_b = sutils.open_datagram_socket(dut_b, ip_b, port)

        # allocate SPIs
        spi_keys_a = iutils.allocate_spis(dut_a, ip_a, ip_b)
        in_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[0])
        out_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[1])
        spi_keys_b = iutils.allocate_spis(dut_b, ip_b, ip_a, out_spi, in_spi)

        # open udp encap sockets
        udp_encap_a = None
        udp_encap_b = None
        if udp_encap:
            udp_encap_a = dut_a.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
            udp_encap_b = dut_b.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
        self.log.info("UDP Encap: %s" % udp_encap_a)
        self.log.info("UDP Encap: %s" % udp_encap_b)

        # create transforms
        transforms_a = iutils.create_transport_mode_transforms(
            dut_a, spi_keys_a, ip_a, ip_b, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_a)
        transforms_b = iutils.create_transport_mode_transforms(
            dut_b, spi_keys_b, ip_b, ip_a, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_b)

        # apply transforms
        iutils.apply_transport_mode_transforms_datagram_socket(
            dut_a, socket_a, transforms_a)
        iutils.apply_transport_mode_transforms_datagram_socket(
            dut_b, socket_b, transforms_b)

        # send and verify message from one dut to another
        sutils.send_recv_data_datagram_sockets(
            dut_a, dut_b, socket_a, socket_b, ip_b, port)

        # verify ESP packets
        iutils.verify_esp_packets([dut_a, dut_b])

        # remove transforms
        iutils.remove_transport_mode_transforms_datagram_socket(dut_a, socket_a)
        iutils.remove_transport_mode_transforms_datagram_socket(dut_b, socket_b)

        # destroy transforms
        iutils.destroy_transport_mode_transforms(dut_a, transforms_a)
        iutils.destroy_transport_mode_transforms(dut_b, transforms_b)

        # close udp encap sockets
        if udp_encap:
            dut_a.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_a)
            dut_b.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_b)

        # release SPIs
        iutils.release_spis(dut_a, spi_keys_a)
        iutils.release_spis(dut_b, spi_keys_b)

        # Send and verify message from one dut to another
        sutils.send_recv_data_datagram_sockets(
            dut_a, dut_b, socket_a, socket_b, ip_b, port)

        # close sockets
        sutils.close_datagram_socket(dut_a, socket_a)
        sutils.close_datagram_socket(dut_b, socket_b)

    def _test_transport_mode_transform_sockets(self, domain, udp_encap = False):

        # dut objects and ip addrs
        dut_a = self.dut_a
        dut_b = self.dut_b
        port = random.randint(5000, 6000)
        ip_a = self.ipv4_dut_a
        ip_b = self.ipv4_dut_b
        if domain == cconst.AF_INET6:
            ip_a = self.ipv6_dut_a
            ip_b = self.ipv6_dut_b
        self.log.info("DUT_A IP addr: %s" % ip_a)
        self.log.info("DUT_B IP addr: %s" % ip_b)
        self.log.info("Port: %s" % port)
        udp_encap_port = 4500

        # open sockets
        server_sock = sutils.open_server_socket(dut_b, ip_b, port)
        sock_a, sock_b = sutils.open_connect_socket(
            dut_a, dut_b, ip_a, ip_b, port, port, server_sock)

        # create crypt and auth keys
        cl, auth_algo, al, trunc_bits = random.choice(self.crypt_auth_combos)
        crypt_key = iutils.make_key(cl)
        auth_key = iutils.make_key(al)
        crypt_algo = cconst.CRYPT_AES_CBC

        # allocate SPIs
        spi_keys_a = iutils.allocate_spis(dut_a, ip_a, ip_b)
        in_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[0])
        out_spi = dut_a.droid.ipSecGetSecurityParameterIndex(spi_keys_a[1])
        spi_keys_b = iutils.allocate_spis(dut_b, ip_b, ip_a, out_spi, in_spi)

        # open udp encap sockets
        udp_encap_a = None
        udp_encap_b = None
        if udp_encap:
            udp_encap_a = dut_a.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
            udp_encap_b = dut_b.droid.ipSecOpenUdpEncapsulationSocket(
                udp_encap_port)
        self.log.info("UDP Encap: %s" % udp_encap_a)
        self.log.info("UDP Encap: %s" % udp_encap_b)

        # create transforms
        transforms_a = iutils.create_transport_mode_transforms(
            dut_a, spi_keys_a, ip_a, ip_b, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_a)
        transforms_b = iutils.create_transport_mode_transforms(
            dut_b, spi_keys_b, ip_b, ip_a, crypt_algo, crypt_key, auth_algo,
            auth_key, trunc_bits, udp_encap_b)

        # apply transform to sockets
        iutils.apply_transport_mode_transforms_socket(dut_a, sock_a, transforms_a)
        iutils.apply_transport_mode_transforms_socket(dut_b, sock_b, transforms_b)

        # send message from one dut to another
        sutils.send_recv_data_sockets(dut_a, dut_b, sock_a, sock_b)

        # verify esp packets
        iutils.verify_esp_packets([dut_a, dut_b])

        # remove transforms from socket
        iutils.remove_transport_mode_transforms_socket(dut_a, sock_a)
        iutils.remove_transport_mode_transforms_socket(dut_b, sock_b)

        # destroy transforms
        iutils.destroy_transport_mode_transforms(dut_a, transforms_a)
        iutils.destroy_transport_mode_transforms(dut_b, transforms_b)

        # close udp encap sockets
        if udp_encap:
            dut_a.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_a)
            dut_b.droid.ipSecCloseUdpEncapsulationSocket(udp_encap_b)

        # release SPIs
        iutils.release_spis(dut_a, spi_keys_a)
        iutils.release_spis(dut_b, spi_keys_b)

        # send message from one dut to another
        sutils.send_recv_data_sockets(dut_a, dut_b, sock_a, sock_b)

        # close sockets
        sutils.close_socket(dut_a, sock_a)
        sutils.close_socket(dut_b, sock_b)
        sutils.close_server_socket(dut_b, server_sock)

    """ Helper functions end """

    """ Tests begin """

    @test_tracker_info(uuid="877577e2-94e8-46d3-8fee-330327adba1e")
    def test_spi_allocate_release_requested_spi_ipv4(self):
        """
        Steps:
          1. Request SPI for IPv4 dest addr
          2. SPI should be generated with the requested value
          3. Close SPI
        """
        self._test_spi_allocate_release_req_spi(self.dut_a, self.ipv4_dut_b)
        self._test_spi_allocate_release_req_spi(self.dut_b, self.ipv4_dut_a)

    @test_tracker_info(uuid="0be4c204-0e63-43fd-a7ff-8647eef21066")
    def test_spi_allocate_release_requested_spi_ipv6(self):
        """
        Steps:
          1. Request SPI for IPv6 dest addr
          2. SPI should be generated with the requested value
          3. Close SPI
        """
        self._test_spi_allocate_release_req_spi(self.dut_a, self.ipv6_dut_b)
        self._test_spi_allocate_release_req_spi(self.dut_b, self.ipv6_dut_a)

    @test_tracker_info(uuid="75eea49c-5621-4410-839f-f6e7bdd792a0")
    def test_spi_allocate_release_random_spi_ipv4(self):
        """
        Steps:
          1. Request SPI for IPv4 dest addr
          2. A random SPI should be generated
          3. Close SPI
        """
        self._test_spi_allocate_release_random_spi(self.dut_a, self.ipv4_dut_b)
        self._test_spi_allocate_release_random_spi(self.dut_b, self.ipv4_dut_a)

    @test_tracker_info(uuid="afad9e48-5573-4b3f-8dfd-c7a77eda4e1e")
    def test_spi_allocate_release_random_spi_ipv6(self):
        """
        Steps:
          1. Request SPI for IPv6 dest addr
          2. A random SPI should be generated
          3. Close SPI
        """
        self._test_spi_allocate_release_random_spi(self.dut_a, self.ipv6_dut_b)
        self._test_spi_allocate_release_random_spi(self.dut_b, self.ipv6_dut_a)

    @test_tracker_info(uuid="97817f4d-159a-4692-93f7-a38b162a565e")
    def test_allocate_release_multiple_random_spis_ipv4(self):
        """
        Steps:
          1. Request 10 SPIs at a time
          2. After 8, the remaining should return 0
        """
        res = True
        spi_key_list = []
        spi_list = []
        for i in range(10):
            spi_key = self.dut_a.droid.ipSecAllocateSecurityParameterIndex(
                self.ipv4_dut_a)
            spi_key_list.append(spi_key)
            spi = self.dut_a.droid.ipSecGetSecurityParameterIndex(spi_key)
            spi_list.append(spi)
        if spi_list.count(0) > 2:
            self.log.error("Valid SPIs is less than 8: %s" % spi_list)
            res = False

        spi_list = []
        for key in spi_key_list:
            self.dut_a.droid.ipSecReleaseSecurityParameterIndex(key)
            spi = self.dut_a.droid.ipSecGetSecurityParameterIndex(key)
            spi_list.append(spi)
        if spi_list.count(0) != 10:
            self.log.error("Could not release all SPIs")
            res = False

        return res

    @test_tracker_info(uuid="efb22e2a-1c2f-43fc-85fa-5d9a5b8e2705")
    def test_spi_allocate_release_requested_spi_ipv4_stress(self):
        """
        Steps:
          1. Request and release requested SPIs for IPv4
        """
        for i in range(15):
            self._test_spi_allocate_release_req_spi(self.dut_a, self.ipv4_dut_b)
            self._test_spi_allocate_release_req_spi(self.dut_b, self.ipv4_dut_a)

    @test_tracker_info(uuid="589749b7-3e6c-4a19-8404-d4a725d63dfd")
    def test_spi_allocate_release_requested_spi_ipv6_stress(self):
        """
        Steps:
          1. Request and release requested SPIs for IPv6
        """
        for i in range(15):
            self._test_spi_allocate_release_req_spi(self.dut_a, self.ipv6_dut_b)
            self._test_spi_allocate_release_req_spi(self.dut_b, self.ipv6_dut_a)

    @test_tracker_info(uuid="4a48130a-3f69-4390-a587-20848dee4777")
    def test_spi_allocate_release_random_spi_ipv4_stress(self):
        """
        Steps:
          1. Request and release random SPIs for IPv4
        """
        for i in range(15):
            self._test_spi_allocate_release_random_spi(
                self.dut_a, self.ipv4_dut_b)
            self._test_spi_allocate_release_random_spi(
                self.dut_b, self.ipv4_dut_a)

    @test_tracker_info(uuid="b56f6b65-cd71-4462-a739-e29d60e90ae8")
    def test_spi_allocate_release_random_spi_ipv6_stress(self):
        """
        Steps:
          1. Request and release random SPIs for IPv6
        """
        for i in range(15):
            self._test_spi_allocate_release_random_spi(
                self.dut_a, self.ipv6_dut_b)
            self._test_spi_allocate_release_random_spi(
                self.dut_b, self.ipv6_dut_a)

    """ android.system.Os sockets """

    @test_tracker_info(uuid="65c9ab9a-1128-4ba0-a1a1-a9feb99ce712")
    def test_transport_mode_udp_ipv4_file_descriptors_no_nat_no_encap(self):
        """
        Steps:
          1. Encrypt a android.system.Os IPv4 UDP socket with transport mode
             transform with UDP encap on a non-NAT network
          2. Verify encyption and decryption of packets
          3. Remove transform and verify that packets are unencrypted
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET,
                                                             cconst.SOCK_DGRAM)

    @test_tracker_info(uuid="66956266-f18b-490c-b859-08530c6745c9")
    def test_transport_mode_udp_ipv4_file_descriptors_no_nat_with_encap(self):
        """
        Steps:
          1. Encrypt a android.system.Os IPv4 UDP socket with transport mode
             transform with UDP encap on a non-NAT network
          2. Verify encyption and decryption of packets
          3. Remove transform and verify that packets are unencrypted
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET,
                                                             cconst.SOCK_DGRAM,
                                                             True)

    @test_tracker_info(uuid="467a1e7b-e508-439f-be24-a213bec4a9f0")
    def test_transport_mode_tcp_ipv4_file_descriptors_no_nat_no_encap(self):
        """
        Steps:
          1. Encrypt a android.system.Os IPv4 TCP socket with transport mode
             transform without UDP encap on a non-NAT network
          2. Verify encyption and decryption of packets
          3. Remove transform and verify that packets are unencrypted
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET,
                                                             cconst.SOCK_STREAM)

    @test_tracker_info(uuid="877a9516-2b1c-4b52-8931-3a9a55d57875")
    def test_transport_mode_tcp_ipv4_file_descriptors_no_nat_with_encap(self):
        """
        Steps:
          1. Encrypt a android.system.Os IPv4 TCP socket with transport mode
             transform with UDP encap on a non-NAT network
          2. Verify encyption and decryption of packets
          3. Remove transform and verify that packets are unencrypted
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET,
                                                             cconst.SOCK_STREAM,
                                                             True)

    @test_tracker_info(uuid="a2dd8dc7-99c8-4420-b586-b4cd68f4197e")
    def test_transport_mode_udp_ipv6_file_descriptors(self):
        """ Verify transport mode transform on android.Os datagram sockets ipv4

        Steps:
          1. Encrypt a android.system.Os IPv6 UDP socket with transport mode
             transform
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET6,
                                                             cconst.SOCK_DGRAM)

    @test_tracker_info(uuid="7bc49d79-ffa8-4946-a25f-11daed4061cc")
    def test_transport_mode_tcp_ipv6_file_descriptors(self):
        """ Verify transport mode on android.system.Os stream sockets ipv6

        Steps:
          1. Encrypt a android.system.Os IPv6 TCP socket with transport mode
             transform
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_file_descriptors(cconst.AF_INET6,
                                                             cconst.SOCK_STREAM)

    """ Datagram socket tests """

    @test_tracker_info(uuid="7ff29fc9-41fd-44f4-81e9-ce8c3afd9304")
    def test_transport_mode_udp_ipv4_datagram_sockets_no_nat_no_encap(self):
        """ Verify transport mode transform on datagram sockets ipv6 - UDP

        Steps:
          1. Encypt a DatagramSocket IPv6 socket with transport mode transform
             without UDP encap on a non-NAT network
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_datagram_sockets(cconst.AF_INET)

    @test_tracker_info(uuid="552af945-a23a-49f2-9ceb-b0d1e7c1d50b")
    def test_transport_mode_udp_ipv4_datagram_sockets_no_nat_with_encap(self):
        """ Verify transport mode transform on datagram sockets ipv6 - UDP

        Steps:
          1. Encypt a DatagramSocket IPv6 socket with transport mode transform
             with UDP encap on a non-NAT network
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_datagram_sockets(cconst.AF_INET, True)

    @test_tracker_info(uuid="dd3a08c4-d393-46c4-b550-bc551ceb15f7")
    def test_transport_mode_udp_ipv6_datagram_sockets(self):
        """ Verify transport mode transform on datagram sockets ipv6 - UDP

        Steps:
          1. Encypt a DatagramSocket IPv6 socket with transport mode transform
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_datagram_sockets(cconst.AF_INET6)

    """ Socket & server socket tests """

    @test_tracker_info(uuid="f851b5da-790a-45c6-8968-1a347ef30cf2")
    def test_transport_mode_tcp_ipv4_sockets_no_nat_no_encap(self):
        """ Verify transport mode transform on sockets ipv4 - TCP

        Steps:
          1. Encypt a Socket IPv4 socket with transport mode transform without
             UDP encap on a non-NAT network
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_sockets(cconst.AF_INET)

    @test_tracker_info(uuid="11982874-8625-40c1-aae5-8327217bb3c4")
    def test_transport_mode_tcp_ipv4_sockets_no_nat_with_encap(self):
        """ Verify transport mode transform on sockets ipv4 - TCP

        Steps:
          1. Encypt a Socket IPv4 socket with transport mode transform with
             UDP encap on a non-NAT network
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_sockets(cconst.AF_INET, True)

    @test_tracker_info(uuid="ba8f98c7-2123-4082-935f-a5fd5e6fb461")
    def test_transport_mode_tcp_ipv6_sockets(self):
        """ Verify transport mode transform on sockets ipv6 - TCP

        Steps:
          1. Encypt a Socket IPv6 socket with transport mode transform
          2. Verify that packets are encrypted and decrypted at the other end
          3. Remove tranform and verify that unencrypted packets are sent
        """
        self._test_transport_mode_transform_sockets(cconst.AF_INET6)

    """ Tests end """
