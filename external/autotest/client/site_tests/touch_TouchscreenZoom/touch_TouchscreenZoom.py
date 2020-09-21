# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_TouchscreenZoom(touch_playback_test_base.touch_playback_test_base):
    """Plays back touchscreen zoom and checks for correct page movement.

    Test zooms in and then out, checking that direction is correct.

    """
    version = 1

    _DIRECTIONS = ['in', 'out']
    _FILENAME_FMT_STR = 'zoom-%s'


    def _check_zoom_in_one_direction(self, direction):
        logging.info('Starting to zoom %s', direction)
        start_level = self._events.get_page_width()
        self._events.clear_previous_events()

        self._playback(self._filepaths[direction], 'touchscreen')

        self._events.wait_for_events_to_complete()
        end_level = self._events.get_page_width()
        delta = end_level - start_level

        if delta == 0:
            self._events.log_events()
            raise error.TestFail('No page zoom occurred!')

        if ((delta > 0 and direction == 'in') or
            (delta < 0 and direction == 'out')):
            self._events.log_events()
            raise error.TestFail('Zoom was in the wrong direction!')


    def _is_testable(self):
        """Return True if test can run on this device, else False.

        @raises: TestError if host has no touchscreen when it should.

        """
        # Raise error if no touchscreen detected.
        if not self._has_touchscreen:
            raise error.TestError('No touchscreen found on this device!')

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files_from_directions(
                'touchscreen', self._FILENAME_FMT_STR, self._DIRECTIONS)
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

            # Zoom in and out on test page.
            for direction in self._DIRECTIONS:
                self._check_zoom_in_one_direction(direction)
