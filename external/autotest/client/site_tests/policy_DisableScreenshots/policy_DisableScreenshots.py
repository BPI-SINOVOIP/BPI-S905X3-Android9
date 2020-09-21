# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import logging
import os
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base
from autotest_lib.client.cros.input_playback import input_playback

POLL_TIMEOUT = 5
POLL_FREQUENCY = 0.5


class policy_DisableScreenshots(
        enterprise_policy_base.EnterprisePolicyTest):
    """
    Test DisableScreenshots policy effect on ChromerOS behavior.

    This test verifies the behavior of Chrome OS with a set of valid values
    for the DisableScreenshots user policy ie, the policy value is set to True,
    False or is Unset.
    These valid values are covered by the test cases: DisableScreenshot_Block,
    NotSet_Allow and False_Allow.

    When the policy value is None or is set to False (as in the cases
    NotSet_Allow and False_Allow), then screenshots will be captured on pressing
    the Ctrl and F5 keyboard buttons. When the value is set to True (as in case
    DisableScreenshot_Block), screenshot capture is disabled.

    """
    version = 1

    def initialize(self, **kwargs):
        """Emulate a keyboard in order to play back the screenshot shortcut."""
        self._initialize_test_constants()
        super(policy_DisableScreenshots, self).initialize(**kwargs)
        self.player = input_playback.InputPlayback()
        self.player.emulate(input_type='keyboard')
        self.player.find_connected_inputs()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'DisableScreenshots'
        self._DOWNLOADS = '/home/chronos/user/Downloads/'
        self._SCREENSHOT_PATTERN = 'Screenshot*'
        self._SCREENSHOT_FILENAME = self._DOWNLOADS + self._SCREENSHOT_PATTERN

        self.TEST_CASES = {
            'DisableScreenshot_Block': True,
            'False_Allow': False,
            'NotSet_Allow': None
        }
        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'RestoreOnStartupURLs': self.STARTUP_URLS,
            'RestoreOnStartup': 4
        }


    def _capture_screenshot(self):
        """Capture a screenshot by pressing Ctrl +F5."""
        self.player.blocking_playback_of_default_file(
                input_type='keyboard', filename='keyboard_ctrl+f5')


    def _screenshot_file_exists(self):
        """
        Return True if screenshot was captured. else returns False.

        @returns boolean indicating if screenshot file was saved or not.

        """
        try:
            utils.poll_for_condition(
                    lambda: len(glob.glob(self._SCREENSHOT_FILENAME)) > 0,
                    timeout=POLL_TIMEOUT,
                    sleep_interval=POLL_FREQUENCY)
        except utils.TimeoutError:
            logging.info('Screenshot file not found.')
            return False

        logging.info('Screenshot file found.')
        return True


    def _delete_screenshot_files(self):
        """Delete existing screenshot files, if any."""
        for filename in glob.glob(self._SCREENSHOT_FILENAME):
            os.remove(filename)


    def cleanup(self):
        """Cleanup files created in this test, if any and close the player."""
        self._delete_screenshot_files()
        self.player.close()
        super(policy_DisableScreenshots, self).cleanup()


    def _test_screenshot_disabled(self, policy_value):
        """
        Verify CrOS enforces the DisableScreenshots policy.

        When DisableScreenshots policy value is undefined, screenshots shall
        be captured via the keyboard shortcut Ctrl + F5.
        When DisableScreenshots policy is set to True screenshots shall not
        be captured.

        @param policy_value: policy value for this case.

        """
        logging.info('Deleting preexisting Screenshot files.')
        self._delete_screenshot_files()

        self._capture_screenshot()
        screenshot_file_captured = self._screenshot_file_exists()
        if policy_value:
            if screenshot_file_captured:
                raise error.TestFail('Screenshot should not be captured')
        elif not screenshot_file_captured:
            raise error.TestFail('Screenshot should be captured')


    def run_once(self, case):
        """
        Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_screenshot_disabled(case_value)
