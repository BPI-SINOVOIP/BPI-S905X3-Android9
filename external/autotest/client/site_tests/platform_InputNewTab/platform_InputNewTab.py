# Copyright 2017 The Chromium OS Authors. All rights reserved.
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


class platform_InputNewTab(test.test):
    """Tests if device suspends using shortcut keys."""
    version = 1
    _WAIT = 15

    def warmup(self):
        """Test setup."""
        # Emulate keyboard.
        # See input_playback. The keyboard is used to play back shortcuts.
        self._player = input_playback.InputPlayback()
        self._player.emulate(input_type='keyboard')
        self._player.find_connected_inputs()

    def _open_tab(self):
        """Use keyboard shortcut to start a new tab."""
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_ctrl+t')
        time.sleep(self._WAIT)

    def _close_tab(self):
        """Use keyboard shortcut to start a new tab."""
        self._player.blocking_playback_of_default_file(
            input_type='keyboard', filename='keyboard_ctrl+w')
        time.sleep(self._WAIT)

    def verify_num_tabs(self, expected_num):
        """Verify number of tabs open.

        @param expected_num: expected number of tabs

        @raises: error.TestFail if number of expected tabs does not equal
                 number of current tabs.

        """
        current_num = len(self.cr.browser.tabs)
        utils.poll_for_condition(
            lambda: current_num == expected_num,
            exception=error.TestFail('Incorrect number of tabs: %s vs %s'
                                     % (current_num, expected_num)),
            timeout=self._WAIT)


    def run_once(self):
        """
        Open and close tabs in new browser. Verify expected
        number of tabs.
        """
        with chrome.Chrome() as cr:
            self.cr = cr
            time.sleep(self._WAIT)

            init_tabs = len(cr.browser.tabs)
            self._open_tab()
            self.verify_num_tabs(init_tabs + 1)
            self._open_tab()
            self.verify_num_tabs(init_tabs + 2)

            self._close_tab()
            self.verify_num_tabs(init_tabs + 1)

    def cleanup(self):
        """Test cleanup."""
        self._player.close()
