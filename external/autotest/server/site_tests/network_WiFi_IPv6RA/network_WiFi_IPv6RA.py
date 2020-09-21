# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import wifi_cell_test_base
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.packet_generation import IP_utils

LIFETIME_FRACTION = 6
LIFETIME = 180
INTERVAL = 2

class network_WiFi_IPv6RA(wifi_cell_test_base.WiFiCellTestBase):
    """Test that we can drop/accept various IPv6 RAs."""
    version = 1

    def _cleanup(self, client_conf, router, ip_utils):
        """Deconfigure AP and cleanup test resources.

        @param client_conf: association parameters for test.
        @param router: Router host object.
        @param ip_utils object to cleanup.

        """
        self.context.client.shill.disconnect(client_conf.ssid)
        self.context.client.shill.delete_entries_for_ssid(client_conf.ssid)
        self.context.router.deconfig()
        self.context.capture_host.stop_capture()
        ip_utils.cleanup_scapy()


    def run_once(self):
        """Sets up a router, connects to it, and sends RAs."""
        client_conf = xmlrpc_datatypes.AssociationParameters()
        client_mac = self.context.client.wifi_mac
        self.context.router.deconfig()
        ap_config = hostap_config.HostapConfig(channel=6,
                mode=hostap_config.HostapConfig.MODE_11G,
                ssid='OnHubWiFi_ch6_802.11g')
        self.context.configure(ap_config)
        self.context.capture_host.start_capture(2437, ht_type='HT20')
        client_conf.ssid = self.context.router.get_ssid()
        assoc_result = self.context.assert_connect_wifi(client_conf)
        self.context.client.collect_debug_info(client_conf.ssid)

        with self.context.client.assert_no_disconnects():
            self.context.assert_ping_from_dut()
        if self.context.router.detect_client_deauth(client_mac):
            raise error.TestFail(
                'Client de-authenticated during the test')

        # Sleep for 10 seconds to put the phone in WoW mode.
        time.sleep(10)

        ip_utils = IP_utils.IPutils(self.context.router.host)
        ip_utils.install_scapy()
        ra_count = ip_utils.get_icmp6intype134(self.context.client.host)

        # Start scapy to send RA to the phone's MAC
        ip_utils.send_ra(mac=client_mac, interval=0, count=1)
        ra_count_latest = ip_utils.get_icmp6intype134(self.context.client.host)

        # The phone should accept the first unique RA in sequence.
        if ra_count_latest != ra_count + 1:
            logging.debug('Device dropped the first RA in sequence')
            raise error.TestFail('Device dropped the first RA in sequence.')

        # Generate and send 'x' number of duplicate RAs, for 1/6th of the the
        # lifetime of the original RA. Test assumes that the original RA has a
        # lifetime of 180s. Hence, all RAs received within the next 30s of the
        # original RA should be filtered.
        ra_count = ra_count_latest
        count = LIFETIME / LIFETIME_FRACTION / INTERVAL
        ip_utils.send_ra(mac=client_mac, interval=INTERVAL, count=count)
        ra_count_latest = ip_utils.get_icmp6intype134(self.context.client.host)
        pkt_loss = count - (ra_count_latest - ra_count)
        percentage_loss = float(pkt_loss) / count * 100

        # Fail test if at least 90% of RAs were not dropped.
        if percentage_loss < 90:
            logging.debug('Device did not filter duplicate RAs correctly.'
                          '%d Percent of duplicate RAs were accepted' %
                          (100 - percentage_loss))
            raise error.TestFail('Device accepted a duplicate RA.')

        # Any new RA after this should be accepted.
        ip_utils.send_ra(mac=client_mac, interval=INTERVAL, count=2)
        ra_count_latest = ip_utils.get_icmp6intype134(self.context.client.host)
        if ra_count_latest != ra_count + 1:
            logging.debug('Device did not accept new RA after 1/6th time'
                          ' interval.')
            raise error.TestFail('Device dropped a valid RA in sequence.')

        self._cleanup(client_conf, self.context.router.host, ip_utils)
