# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_StylusTaps(touch_playback_test_base.touch_playback_test_base):
    """Checks that stylus touches are translated into clicks."""
    version = 1

    _CLICK_NAME = 'tap'

    def _check_for_click(self):
        """Playback and check whether click occurred.  Fail if not.

        @raises: TestFail if no click or hover occurred.

        """
        cases = [(self._CLICK_NAME, 1)]
        for (filename, expected_count) in cases:
            self._events.clear_previous_events()
            self._blocking_playback(filepath=self._filepaths[filename],
                                    touch_type='stylus')
            self._events.wait_for_events_to_complete()

            actual_count = self._events.get_click_count()
            if actual_count is not expected_count:
                self._events.log_events()
                raise error.TestFail('Saw %d clicks when %s were expected!' % (
                        actual_count, expected_count))

    def run_once(self):
        """Entry point of this test."""
        self._filepaths = self._find_test_files('stylus', [self._CLICK_NAME])
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return

        # Log in and start test.
        with chrome.Chrome(init_network_controller=True) as cr:
            self._open_events_page(cr)
            self._events.set_prevent_defaults(False)
            self._check_for_click()
