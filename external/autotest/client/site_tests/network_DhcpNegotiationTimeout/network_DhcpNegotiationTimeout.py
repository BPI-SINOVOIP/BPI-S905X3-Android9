# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils
from autotest_lib.client.cros import dhcp_test_base

class network_DhcpNegotiationTimeout(dhcp_test_base.DhcpTestBase):
    """The DHCP Negotiation Timeout class.

    Sets up a virtual ethernet pair, stops the DHCP server on the pair,
    restarts shill, and waits for DHCP to timeout.

    After the timeout interval, checks if the same shill process is
    running. If not, report failure.

    """
    SHILL_DHCP_TIMEOUT_SECONDS = 30


    @staticmethod
    def get_daemon_pid(daemon_name):
        """
        Get the PID of a running daemon.

        @return The PID as an integer.

        """
        pid = utils.get_service_pid(daemon_name)
        if pid == 0:
            raise error.TestFail('Failed to get the pid of %s' % daemon_name)
        return pid

    def test_body(self):
        """Test main loop."""
        self.server.stop()
        utils.restart_service("shill")
        start_pid = self.get_daemon_pid("shill")

        time.sleep(self.SHILL_DHCP_TIMEOUT_SECONDS + 2)
        end_pid = self.get_daemon_pid("shill")
        if end_pid != start_pid:
            raise error.TestFail("shill restarted (probably crashed)")
