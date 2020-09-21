# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_ForceYouTubeSafetyMode(
    enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of ForceYouTubeSafetyMode policy on Chrome OS behavior.

    This test verifies that the ForceYouTubeSafetyMode user policy controls
    whether Chrome OS forces YouTube to use Safety Mode. When the policy is
    set true, Chrome shall add the 'YouTube-Safety-Mode: Active' header to
    any YouTube URL request. When the policy is set false or is not set,
    Chrome shall not add the header. The presence of the header causes YouTube
    to activate Restricted Mode. The absence of the header causes YouTube to
    use the mode last set by the user (as stored in a cookie), or default to
    inactive if the user has not set the mode.

    The test covers all valid policy values across three test cases:
    NotSet_SafetyInactive, False_SafetyInactive, True_SafetyActive.

    A test case passes when https://www.youtube.com page indicates that
    'Restricted Mode' is On (or Off) when the policy is set true (or set false
    or not set). A test case shall fail if the above behavior is not enforced.

    """
    version = 1

    POLICY_NAME = 'ForceYouTubeSafetyMode'
    TEST_CASES = {
        'True_SafetyActive': True,
        'False_SafetyInactive': False,
        'NotSet_SafetyInactive': None
    }
    SUPPORTING_POLICIES = {
        'DefaultSearchProviderEnabled': None
    }
    YOUTUBE_SEARCH_URL = 'https://www.youtube.com/results?search_query=kittens'

    def _test_force_youtube_safety(self, policy_value):
        """Verify CrOS enforces ForceYouTubeSafetyMode policy.

        @param policy_value: policy value for this case.

        """
        is_safety_mode_active = self._is_restricted_mode_active()
        if policy_value == True:
            if not is_safety_mode_active:
                raise error.TestFail('Restricted Mode should be active.')
        else:
            if is_safety_mode_active:
                raise error.TestFail('Restricted Mode should not be active.')

    def _is_restricted_mode_active(self):
        """Check whether the safety-mode-message is displayed.

        When Restricted Mode is enabled on www.youtube.com, a warning message
        is displayed at the top of the screen saying that some results have
        been removed. The message is in <p class="safety-mode-message">.

        @returns: True if the safety-mode-message is displayed.

        """
        tab = self.navigate_to_url(self.YOUTUBE_SEARCH_URL)
        is_restricted_mode_active = tab.EvaluateJavaScript(
            'document.getElementsByClassName("safety-mode-message").length')
        logging.info('restricted mode active: %s', is_restricted_mode_active)
        tab.Close()
        return is_restricted_mode_active

    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_force_youtube_safety(case_value)
