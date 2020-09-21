# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.input_playback import input_playback
from telemetry.core import exceptions


class platform_InputBrowserNav(test.test):
    """Tests if device suspends using shortcut keys."""
    version = 1
    _WAIT = 15
    URL_1 = 'https://www.youtube.com'
    URL_2 = 'https://www.yahoo.com'

    def warmup(self):
        """Test setup."""
        # Emulate keyboard.
        # See input_playback. The keyboard is used to play back shortcuts.
        self._player = input_playback.InputPlayback()
        self._player.emulate(input_type='keyboard')
        self._player.find_connected_inputs()

    def test_back(self, tab):
        """Use keyboard shortcut to test Back (F1) key.

        @param tab: current tab.

        """
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_f1')
        time.sleep(self._WAIT)
        self.verify_url(tab, self.URL_1)

    def test_forward(self, tab):
        """Use keyboard shortcut to test Forward (F2) key.

        @param tab: current tab.

        """
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_f2')
        time.sleep(self._WAIT)
        self.verify_url(tab, self.URL_2)

    def test_refresh(self, tab):
        """Use keyboard shortcut to test Refresh (F3) key.

        @param tab: current tab.

        @raises error.TestFail if refresh unsuccessful.

        """
        # Set JS variable to initial true value to confirm refresh worked.
        # We are expecting this variable not to exist after the refresh.
        tab.EvaluateJavaScript("not_refreshed=true")
        js = 'not_refreshed == true'
        utils.wait_for_value(
            lambda: tab.EvaluateJavaScript(js),
            expected_value=True)

        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_f3')
        time.sleep(self._WAIT)

        # Verify we are still on the second url.
        self.verify_url(tab, self.URL_2)
        # Check to see not_refresh does not exist (results in exception).
        # If it does, the refresh was not successful.
        try:
            not_refresh = tab.EvaluateJavaScript(js)
            raise error.TestFail("Refresh unsuccesful.")
        except exceptions.EvaluateException:
            logging.info("Refresh successful.")

    def verify_url(self, tab, correct_url):
        """Verify tab's current url is the url wanted.

        @param tab: current tab.
        @param correct_url: url wanted.

        @raises: error.TestFail if incorrect url.

        """
        current_url = tab.url.encode('utf8').rstrip('/')
        utils.poll_for_condition(
            lambda: current_url == correct_url,
            exception=error.TestFail('Incorrect navigation: %s'
                                     % current_url),
            timeout=self._WAIT)

    def run_once(self):
        """
        Open browser, navigate to urls and test
        forward, backward, and refresh functions.

        """
        with chrome.Chrome() as cr:
            tab = cr.browser.tabs[0]
            logging.info('Initially navigate to %s.' % self.URL_1)
            tab.Navigate(self.URL_1)
            time.sleep(self._WAIT)
            logging.info('Next, navigate to %s.' % self.URL_2)
            tab.Navigate(self.URL_2)

            self.test_back(tab)
            self.test_forward(tab)
            self.test_refresh(tab)

    def cleanup(self):
        """Test cleanup."""
        self._player.close()
