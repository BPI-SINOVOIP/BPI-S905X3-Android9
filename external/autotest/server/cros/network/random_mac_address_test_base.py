# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros.network import tcpdump_analyzer
from autotest_lib.server import site_linux_system
from autotest_lib.server.cros.network import hostap_config
from autotest_lib.server.cros.network import wifi_cell_test_base

class RandomMACAddressTestBase(wifi_cell_test_base.WiFiCellTestBase):
    """An abstract base class for MAC address randomization tests in WiFi cells.

       MAC address randomization tests will test the ability of the DUT to
       successfully randomize the MAC address it sends in probe requests when
       such a feature is supported and configured, and that it does not do so
       otherwise.
    """

    # Approximate number of seconds to perform a full scan.
    REQUEST_SCAN_DELAY = 5

    # Default number of shill scans to run.
    DEFAULT_NUM_SCANS = 5

    def initialize(self, host):
        super(RandomMACAddressTestBase, self).initialize(host)
        self._ap_config = hostap_config.HostapConfig(channel=1)


    def warmup(self, host, raw_cmdline_args, additional_params=None):
        super(RandomMACAddressTestBase, self).warmup(
                host, raw_cmdline_args, additional_params)

        self.context.router.require_capabilities(
                [site_linux_system.LinuxSystem.CAPABILITY_MULTI_AP_SAME_BAND])

        self.context.configure(self._ap_config)


    def start_capture(self):
        """
        Start packet capture.
        """
        logging.debug('Starting packet capture')
        self.context.capture_host.start_capture(
                self._ap_config.frequency,
                ht_type=self._ap_config.ht_packet_capture_mode)


    def request_scans(self, num_scans=DEFAULT_NUM_SCANS):
        """
        Helper to request a few scans with the configured randomization.

        @param num_scans: The number of scans to perform.
        """
        for i in range(num_scans):
            # Request scan through shill rather than iw because iw won't
            # set the random MAC flag in the scan request netlink packet.
            self.context.client.shill.request_scan()
            time.sleep(self.REQUEST_SCAN_DELAY)


    def _frame_matches_ssid(self, frame):
        # Empty string is a wildcard SSID.
        return frame.ssid == '' or frame.ssid == self.context.router.get_ssid()


    def stop_capture_and_get_probe_requests(self):
        """
        Stop packet capture and return probe requests from the DUT.
        have the hardware MAC address.
        """
        logging.debug('Stopping packet capture')
        results = self.context.capture_host.stop_capture()
        if len(results) != 1:
            raise error.TestError('Expected to generate one packet '
                                  'capture but got %d captures instead.',
                                  len(results))

        logging.debug('Analyzing packet capture...')
        # Get all the frames in chronological order.
        frames = tcpdump_analyzer.get_frames(
                results[0].local_pcap_path,
                tcpdump_analyzer.WLAN_PROBE_REQ_ACCEPTOR,
                bad_fcs='discard')

        return [frame for frame in frames if self._frame_matches_ssid(frame)]
