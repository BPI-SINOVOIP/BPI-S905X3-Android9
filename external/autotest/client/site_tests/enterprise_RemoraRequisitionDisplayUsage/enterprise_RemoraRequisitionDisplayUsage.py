# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome, enrollment
from autotest_lib.client.common_lib import error
from py_utils import TimeoutException


class enterprise_RemoraRequisitionDisplayUsage(test.test):
    """Start enrollment and ensure that the window is shown on a Mimo display"""
    version = 1

    def supports_display_fetching(self, oobe):
        """Return whether Chromium supports fetching the primary display name"""
        oobe.WaitForJavaScriptCondition(
            'typeof Oobe !== \'undefined\'', timeout=10)

        return oobe.EvaluateJavaScript(
            '"getPrimaryDisplayNameForTesting" in Oobe')

    def assert_mimo_is_primary(self, oobe):
        """Fails the test if the Mimo is not the primary display"""
        oobe.ExecuteJavaScript('window.__oobe_display = ""')

        mimo_is_primary = ("Oobe.getPrimaryDisplayNameForTesting().then("
            "display => window.__oobe_display = display);"
            "window.__oobe_display.indexOf('MIMO') >= 0")

        try:
            oobe.WaitForJavaScriptCondition(mimo_is_primary, timeout=10)
        except TimeoutException:
            display = oobe.EvaluateJavaScript('window.__oobe_display')
            raise error.TestFail(
                'Primary display is {}, not Mimo'.format(display))

    def run_once(self):
        with chrome.Chrome(auto_login=False) as cr:
            enrollment.SwitchToRemora(cr.browser)

            if not self.supports_display_fetching(cr.browser.oobe):
                return

            self.assert_mimo_is_primary(cr.browser.oobe)
