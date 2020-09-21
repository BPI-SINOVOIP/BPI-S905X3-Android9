# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.network import random_mac_address_test_base

class network_WiFi_RandomMACAddress(
        random_mac_address_test_base.RandomMACAddressTestBase):
    """
    Test that the MAC address is randomized during scans when we
    are not connected to an AP already.
    """

    version = 1

    def run_once(self):
        """Body of the test."""
        client = self.context.client
        dut_hw_mac = client.wifi_mac

        # Enable MAC address randomization in shill.
        with client.mac_address_randomization(True):
            self.start_capture()
            self.request_scans()
            frames = self.stop_capture_and_get_probe_requests()

        if not frames:
            raise error.TestFail('No probe requests were found!')
        elif any(frame.source_addr == dut_hw_mac for frame in frames):
            raise error.TestFail('Found probe requests with hardware MAC!')
