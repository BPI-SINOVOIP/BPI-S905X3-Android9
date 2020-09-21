#!/usr/bin/env python3.4
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the 'License');
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an 'AS IS' BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import time
from acts.test_decorators import test_tracker_info
from acts.test_utils.power import PowerWiFiBaseTest as PWBT
from acts.test_utils.wifi import wifi_power_test_utils as wputils
from acts.controllers import packet_sender as pkt_utils

RA_SHORT_LIFETIME = 3
RA_LONG_LIFETIME = 1000
DNS_LONG_LIFETIME = 300
DNS_SHORT_LIFETIME = 3


class PowerWiFimulticastTest(PWBT.PowerWiFiBaseTest):
    def set_connection(self):
        """Setup connection between AP and client.

        Setup connection between AP and phone, change DTIMx1 and get information
        such as IP addresses to prepare packet construction.

        """
        attrs = ['screen_status', 'wifi_band']
        indices = [2, 4]
        self.decode_test_configs(attrs, indices)
        # Change DTIMx1 on the phone to receive all Multicast packets
        rebooted = wputils.change_dtim(
            self.dut, gEnableModulatedDTIM=1, gMaxLIModulatedDTIM=10)
        self.dut.log.info('DTIM value of the phone is now DTIMx1')
        if rebooted:
            self.dut_rockbottom()

        self.setup_ap_connection(
            self.main_network[self.test_configs.wifi_band])
        # Wait for DHCP with timeout of 60 seconds
        wputils.wait_for_dhcp(self.pkt_sender.interface)

        # Set the desired screen status
        if self.test_configs.screen_status == 'OFF':
            self.dut.droid.goToSleepNow()
            self.dut.log.info('Screen is OFF')
        time.sleep(5)

    def sendPacketAndMeasure(self, packet):
        """Packet injection template function

        Args:
            packet: packet to be sent/inject
        """
        # Start sending packets and measure power
        self.pkt_sender.start_sending(packet, self.interval)
        self.measure_power_and_validate()

    # Test cases - screen OFF
    @test_tracker_info(uuid='b5378aaf-7949-48ac-95fb-ee94c85d49c3')
    def test_screen_OFF_band_5g_directed_arp(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.ArpGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='3b5d348d-70bf-483d-8736-13da569473aa')
    def test_screen_OFF_band_5g_misdirected_arp(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.ArpGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv4_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='8e534d3b-5a25-429a-a1bb-8119d7d28b5a')
    def test_screen_OFF_band_5g_directed_ns(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.NsGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='536d716d-f30b-4d20-9976-e2cbc36c3415')
    def test_screen_OFF_band_5g_misdirected_ns(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.NsGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv6_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='5eed3174-8e94-428e-8527-19a9b5a90322')
    def test_screen_OFF_band_5g_ra_short(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(RA_SHORT_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='67867bae-f1c5-44a4-9bd0-2b832ac8059c')
    def test_screen_OFF_band_5g_ra_long(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(RA_LONG_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='db19bc94-3513-45c4-b3a5-d6219649d0bb')
    def test_screen_OFF_band_5g_directed_dhcp_offer(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.DhcpOfferGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='a8059869-40ee-4cf3-a957-4b7aed03fcf9')
    def test_screen_OFF_band_5g_misdirected_dhcp_offer(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.DhcpOfferGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.mac_dst_fake, self.ipv4_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='6e663f0a-3eb5-46f6-a79e-311baebd5d2a')
    def test_screen_OFF_band_5g_ra_rnds_short(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(
            RA_LONG_LIFETIME, enableDNS=True, dns_lifetime=DNS_SHORT_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='84d2f1ff-bd4f-46c6-9b06-826d9b14909c')
    def test_screen_OFF_band_5g_ra_rnds_long(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(
            RA_LONG_LIFETIME, enableDNS=True, dns_lifetime=DNS_LONG_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='4a17e74f-3e7f-4e90-ac9e-884a7c13cede')
    def test_screen_OFF_band_5g_directed_ping6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='ab249e0d-58ba-4b55-8a81-e1e4fb04780a')
    def test_screen_OFF_band_5g_misdirected_ping6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv6_dst_fake, pkt_utils.MAC_BROADCAST)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='e37112e6-5c35-4c89-8d15-f5a44e69be0b')
    def test_screen_OFF_band_5g_directed_ping4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='afd4a011-63a9-46c3-8a75-13f515ba8475')
    def test_screen_OFF_band_5g_misdirected_ping4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv4_dst_fake, pkt_utils.MAC_BROADCAST)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='03f0e845-fd66-4120-a79d-5eb64d49b6cd')
    def test_screen_OFF_band_5g_mdns6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Mdns6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='dcbb0aec-512d-48bd-b743-024697ce511b')
    def test_screen_OFF_band_5g_mdns4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Mdns4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    # Test cases: screen ON
    @test_tracker_info(uuid='b9550149-bf36-4f86-9b4b-6e900756a90e')
    def test_screen_ON_band_5g_directed_arp(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.ArpGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='406dffae-104e-46cb-9ec2-910aac7aca39')
    def test_screen_ON_band_5g_misdirected_arp(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.ArpGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv4_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='be4cb543-c710-4041-a770-819e82a6d164')
    def test_screen_ON_band_5g_directed_ns(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.NsGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='de21d24f-e03e-47a1-8bbb-11953200e870')
    def test_screen_ON_band_5g_misdirected_ns(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.NsGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv6_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='b424a170-5095-4b47-82eb-50f7b7fdf35d')
    def test_screen_ON_band_5g_ra_short(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(RA_SHORT_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='ab627e59-2ee8-4c0d-970b-eeb1d1cecdc1')
    def test_screen_ON_band_5g_ra_long(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(RA_LONG_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='ee6514ab-1814-44b9-ba01-63f77ba77c34')
    def test_screen_ON_band_5g_directed_dhcp_offer(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.DhcpOfferGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='eaebfe98-32da-4ebc-bca7-3b7026d99a4f')
    def test_screen_ON_band_5g_misdirected_dhcp_offer(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.DhcpOfferGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.mac_dst_fake, self.ipv4_dst_fake)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='f0e2193f-bf6a-441b-b9c1-bb7b65787cd5')
    def test_screen_ON_band_5g_ra_rnds_short(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(
            RA_LONG_LIFETIME, enableDNS=True, dns_lifetime=DNS_SHORT_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='62b99cd7-75bf-45be-b93f-bb037a13b3e2')
    def test_screen_ON_band_5g_ra_rnds_long(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.RaGenerator(**self.pkt_gen_config)
        packet = pkt_gen.generate(
            RA_LONG_LIFETIME, enableDNS=True, dns_lifetime=DNS_LONG_LIFETIME)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='4088af4c-a64b-4fc1-848c-688936cc6c12')
    def test_screen_ON_band_5g_directed_ping6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='3179e327-e6ac-4dae-bb8a-f3940f21094d')
    def test_screen_ON_band_5g_misdirected_ping6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv6_dst_fake, pkt_utils.MAC_BROADCAST)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='90c70e8a-74fd-4878-89c6-5e15c3ede318')
    def test_screen_ON_band_5g_directed_ping4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='dcfabbc7-a7e1-4a92-a38d-8ebe7aa2e063')
    def test_screen_ON_band_5g_misdirected_ping4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Ping4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate(self.ipv4_dst_fake, pkt_utils.MAC_BROADCAST)
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='117814db-f94d-4239-a7ab-033482b1da52')
    def test_screen_ON_band_5g_mdns6(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Mdns6Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)

    @test_tracker_info(uuid='ce6ad7e2-21f3-4e68-9c0d-d0e14e0a7c53')
    def test_screen_ON_band_5g_mdns4(self):
        self.set_connection()
        self.pkt_gen_config = wputils.create_pkt_config(self)
        pkt_gen = pkt_utils.Mdns4Generator(**self.pkt_gen_config)
        packet = pkt_gen.generate()
        self.sendPacketAndMeasure(packet)
