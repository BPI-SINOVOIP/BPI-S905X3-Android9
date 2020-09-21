# Copyright (c) 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.client.common_lib.cros.network import xmlrpc_datatypes
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base


class network_WiFi_SetOptionalDhcpProperties(
        wifi_cell_test_base.WiFiCellTestBase):
    """Test that the optional DHCP properties are used in DHCP Requests."""
    version = 1

    FREQUENCY_MHZ = 2412
    HOSTNAME_PROPERTY = 'DHCPProperty.Hostname'
    VENDORCLASS_PROPERTY = 'DHCPProperty.VendorClass'
    HOSTNAME_VALUE = 'testHostName'
    VENDORCLASS_VALUE = 'testVendorClass'

    def assert_hostname_and_vendorclass_are_present(self, pcap_result):
        """
        Confirm DHCP Request contains HostName and VendorClass properties.

        @param pcap_result: RemoteCaptureResult tuple.

        """

        logging.info('Analyzing packet capture...')
        dut_src_display_filter = ('wlan.sa==%s' %
                               self.context.client.wifi_mac)

        dhcp_filter = ('(bootp.option.dhcp == 3) '
                       'and (bootp.option.vendor_class_id == %s) '
                       'and (bootp.option.hostname == %s)'
                       % (self.VENDORCLASS_VALUE, self.HOSTNAME_VALUE));
        dhcp_frames = tcpdump_analyzer.get_frames(pcap_result.local_pcap_path,
                                                  dhcp_filter,
                                                  bad_fcs='include')
        if not dhcp_frames:
            raise error.TestFail('Packet capture did not contain a DHCP '
                                 'negotiation!')


    def run_once(self):
        """
        Test that optional DHCP properties are used in DHCP Request packets.

        The test will temporarily set the DHCPProperty.VendorClass and
        DHCPProperty.Hostname DBus properties for the Manager.  The test
        resets the properties to their original values before completion.
        During the test, the DUT should send packets with the optional DHCP
        property settings in DHCP Requests.  If they are not in the packet,
        the test will fail.

        """

        configuration = hostap_config.HostapConfig(frequency=self.FREQUENCY_MHZ)
        self.context.configure(configuration)

        # set hostname and vendorclass for this test
        client = self.context.client
        with client.set_dhcp_property(self.HOSTNAME_PROPERTY, self.HOSTNAME_VALUE):
            with client.set_dhcp_property(self.VENDORCLASS_PROPERTY,
                                          self.VENDORCLASS_VALUE):
                self.context.capture_host.start_capture(
                        configuration.frequency,
                        ht_type=configuration.ht_packet_capture_mode)
                assoc_params = xmlrpc_datatypes.AssociationParameters()
                assoc_params.ssid = self.context.router.get_ssid(instance=0)
                self.context.assert_connect_wifi(assoc_params)
                self.context.assert_ping_from_dut()
                results = self.context.capture_host.stop_capture()
        if len(results) != 1:
          raise error.TestError('Expected to generate one packet '
                                'capture but got %d captures instead.' %
                                len(results))
        self.assert_hostname_and_vendorclass_are_present(results[0])
