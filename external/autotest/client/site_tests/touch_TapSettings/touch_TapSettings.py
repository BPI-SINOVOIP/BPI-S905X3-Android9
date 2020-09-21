# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import itertools
import logging
import re
import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros import touch_playback_test_base


class touch_TapSettings(touch_playback_test_base.touch_playback_test_base):
    """Toggles tap-to-click and tap dragging settings to ensure correctness."""
    version = 1

    _TEST_TIMEOUT = 1  # Number of seconds the test will wait for a click.
    _MOUSE_DESCRIPTION = 'apple_mouse.prop'
    _CLICK_NAME = 'tap'
    _DRAG_NAME = 'tap-drag-right'


    def _check_for_click(self, expected):
        """Playback and check whether tap-to-click occurred.  Fail if needed.

        @param expected: True if clicking should happen, else False.
        @raises: TestFail if actual value does not match expected.

        """
        expected_count = 1 if expected else 0
        self._events.clear_previous_events()
        self._playback(self._filepaths[self._CLICK_NAME])
        time.sleep(self._TEST_TIMEOUT)
        actual_count = self._events.get_click_count()
        if actual_count is not expected_count:
            self._events.log_events()
            raise error.TestFail('Expected clicks=%s, actual=%s.'
                                 % (expected_count, actual_count))


    def _check_for_drag(self, expected):
        """Playback and check whether tap dragging occurred.  Fail if needed.

        @param expected: True if dragging should happen, else False.
        @raises: TestFail if actual value does not match expected.

        """
        self._events.clear_previous_events()
        self._blocking_playback(self._filepaths[self._DRAG_NAME])
        self._events.wait_for_events_to_complete()

        # Find a drag in the reported input events.
        events_log = self._events.get_events_log()
        log_search = re.search('mousedown.*\n(mousemove.*\n)+mouseup',
                               events_log, re.MULTILINE)
        actual_dragging = log_search != None
        actual_click_count = self._events.get_click_count()
        actual = actual_dragging and actual_click_count == 1

        if actual is not expected:
            self._events.log_events()
            raise error.TestFail('Tap dragging movement was %s; expected %s.  '
                                 'Saw %s clicks.'
                                 % (actual, expected, actual_click_count))


    def _is_testable(self):
        """Return True if test can run on this device, else False.

        @raises: TestError if host has no touchpad when it should.

        """
        # Raise error if no touchpad detected.
        if not self._has_touchpad:
            raise error.TestError('No touchpad found on this device!')

        # Check if playback files are available on DUT to run test.
        self._filepaths = self._find_test_files(
                'touchpad', [self._CLICK_NAME, self._DRAG_NAME])
        if not self._filepaths:
            logging.info('Missing gesture files, Aborting test.')
            return False

        return True


    def run_once(self):
        """Entry point of this test."""
        if not self._is_testable():
            return

        # Log in and start test.
        with chrome.Chrome(autotest_ext=True,
                           init_network_controller=True) as cr:
            # Setup.
            self._set_autotest_ext(cr.autotest_ext)
            self._open_events_page(cr)
            self._emulate_mouse()
            self._center_cursor()

            # Check default setting values.
            logging.info('Checking for default setting values.')
            self._check_for_click(True)
            self._check_for_drag(False)

            # Toggle settings in all combinations and check.
            options = [True, False]
            option_pairs = itertools.product(options, options)
            for (click_value, drag_value) in option_pairs:
                self._center_cursor()
                self._set_tap_to_click(click_value)
                self._set_tap_dragging(drag_value)
                self._check_for_click(click_value)
                self._check_for_drag(click_value and drag_value)
