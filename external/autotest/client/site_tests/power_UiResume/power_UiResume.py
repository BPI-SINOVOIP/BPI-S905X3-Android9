# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros import arc, chrome


class power_UiResume(arc.ArcTest):
    """
    Suspend the system while simulating user behavior.

    If ARC is present, open ARC before suspending. If ARC is not present, log
    into Chrome before suspending. To suspend, call autotest power_Resume. This
    reduces duplicate code, while cover all 3 cases: with ARC, with Chrome but
    without ARC, and without Chrome.

    """
    version = 3

    def initialize(self):
        """
        Entry point. Initialize ARC if it is enabled on the DUT, otherwise log
        in Chrome browser.

        """
        self._arc_available = utils.is_arc_available()
        if self._arc_available:
            super(power_UiResume, self).initialize()
        else:
            self._chrome = chrome.Chrome()


    def run_once(self, max_devs_returned=10, seconds=0,
                 ignore_kernel_warns=False):
        """
        Run client side autotest power_Resume, to reduce duplicate code.

        """
        self.job.run_test(
                'power_Resume',
                max_devs_returned=max_devs_returned,
                seconds=seconds,
                ignore_kernel_warns=ignore_kernel_warns,
                measure_arc=self._arc_available)


    def cleanup(self):
        """
        Clean up ARC if it is enabled on the DUT, otherwise close the Chrome
        browser.
        """
        if self._arc_available:
            super(power_UiResume, self).cleanup()
        else:
            self._chrome.close()
