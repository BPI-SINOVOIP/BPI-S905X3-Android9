# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os.path
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils as sys_utils
from autotest_lib.client.cros import cups
from autotest_lib.client.cros import upstart


class platform_CUPSDaemon(test.test):
    """
    Runs some sanity tests for cupsd and the upstart-socket-bridge
    socket-activation.
    """
    version = 1

    _CUPS_SOCK_PATH = '/run/cups/cups.sock'


    def check_cups_is_responding(self):
        """
        Run a basic sanity test to be sure CUPS is operating.
        """

        # Try a simple CUPS command; timeout/fail if it takes too long (i.e.,
        # socket may exist, but it may not get passed off to cupsd properly).
        utils.system_output('lpstat -W all', timeout=10)


    def wait_for_path_exists(self, path, timeout=5):
        """
        Wait for a path to exist, with timeout.

        @param path: path to look for
        @param timeout: time to wait, in seconds

        @return true if path is found; false otherwise
        """
        while timeout > 0:
            if os.path.exists(path):
                return True

            time.sleep(1)
            timeout -= 1

        return os.path.exists(path)


    def run_upstart_tests(self):
        """
        Run some sanity tests for cupsd and the upstart-socket-bridge
        socket-activation.
        """
        upstart.ensure_running('upstart-socket-bridge')

        if not self.wait_for_path_exists(self._CUPS_SOCK_PATH):
            raise error.TestFail('Missing CUPS socket: %s', self._CUPS_SOCK_PATH)

        # Make sure CUPS is stopped, so we can test on-demand launch.
        if upstart.is_running('cupsd'):
            upstart.stop_job('cupsd')

        self.check_cups_is_responding()

        # Now try stopping socket bridge, to see it clean up its files.
        upstart.stop_job('upstart-socket-bridge')
        upstart.stop_job('cupsd')

        if os.path.exists(self._CUPS_SOCK_PATH):
            raise error.TestFail('CUPS socket was not cleaned up: %s', self._CUPS_SOCK_PATH)

        # Create dummy file, to see if upstart-socket-bridge will clear it out
        # properly.
        utils.system('touch %s' % self._CUPS_SOCK_PATH)

        upstart.restart_job('upstart-socket-bridge')

        if not os.path.exists(self._CUPS_SOCK_PATH):
            raise error.TestFail('Missing CUPS socket: %s', self._CUPS_SOCK_PATH)

        self.check_cups_is_responding()


    def run_systemd_tests(self):
        """
        Check if cupsd is running and responsive.
        """
        sys_utils.stop_service('cups', ignore_status=False)
        sys_utils.start_service('cups.socket', ignore_status=False)

        if not self.wait_for_path_exists(self._CUPS_SOCK_PATH):
            raise error.TestFail('Missing CUPS socket: %s', self._CUPS_SOCK_PATH)

        # Test that cupsd is automatically launched through socket activation.
        self.check_cups_is_responding()

        sys_utils.stop_service('cups', ignore_status=False)
        sys_utils.stop_service('cups.socket', ignore_status=False)

        # Dummy file to test that cups.socket handles lingering file properly.
        utils.system('touch %s' % self._CUPS_SOCK_PATH)

        sys_utils.start_service('cups.socket', ignore_status=False)

        if not os.path.exists(self._CUPS_SOCK_PATH):
            raise error.TestFail('Missing CUPS socket: %s', self._CUPS_SOCK_PATH)

        self.check_cups_is_responding()


    def run_once(self):
        """
        Run some sanity tests for cupsd.
        """
        # Check if CUPS is installed for this system or raise TestNA.
        cups.has_cups_or_die()

        if sys_utils.has_systemd():
            self.run_systemd_tests()
        else:
            self.run_upstart_tests()
