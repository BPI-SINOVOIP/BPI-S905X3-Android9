# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome


class cfm_AutotestSmokeTest(test.test):
    """
    Starts a web browser and verifies that nothing crashes.
    """
    version = 1

    def run_once(self):
        """
        Runs the test.
        """
        with chrome.Chrome(init_network_controller = True) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.tab = cr.browser.tabs[0]
            self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'smoke_test.html')))
            self.tab.WaitForDocumentReadyStateToBeComplete()
