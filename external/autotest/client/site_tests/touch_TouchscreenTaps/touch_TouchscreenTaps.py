# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_TouchscreenTaps(touch_playback_test_base.touch_playback_test_base):
    """Checks that touchscreen presses are translated into clicks."""
    version = 1

    _CLICK_NAME = 'tap'


    def _check_for_click(self):
        """Playback and check whether click occurred.  Fail if not.

        @raises: TestFail if no click occurred.

        """
        self._events.clear_previous_events()
        self._blocking_playback(filepath=self._filepaths[self._CLICK_NAME],
                                touch_type='touchscreen')
        self._events.wait_for_events_to_complete()

        actual_count = self._events.get_click_count()
        if actual_count is not 1:
            self._events.log_events()
            raise error.TestFail('Saw %d clicks!' % actual_count)


    def _is_testable(self):
        """Return True if test can run on this device, else False.

        @raises: TestError if host has no touchscreen.

        """
        # Raise error if no touchscreen detected.
        if not self._has_touchscreen:
            raise error.TestError('No touchscreen found on this device!')

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files(
                'touchscreen', [self._CLICK_NAME])
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return False

        return True


    def run_once(self):
        """Entry point of this test."""
        if not self._is_testable():
            return

        # Log in and start test.
        with chrome.Chrome(init_network_controller=True) as cr:
            self._open_events_page(cr)
            self._events.set_prevent_defaults(False)
            self._check_for_click()
