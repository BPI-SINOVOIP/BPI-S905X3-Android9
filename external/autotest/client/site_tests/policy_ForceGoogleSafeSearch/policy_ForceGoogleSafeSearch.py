# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_ForceGoogleSafeSearch(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of ForceGoogleSafeSearch policy on Chrome OS behavior.

    This test verifies that the ForceGoogleSafeSearch user policy controls
    whether Chrome OS enforces the use of Google Safe Search. The test covers
    all valid policy values across three test cases: NotSet_NotSafe,
    False_NotSafe, and True_Safe.

    A test case shall pass if the omnibox appends (or does not append) the
    safe parameter to the search URL when the policy is set true (or is set
    false or is not set). A test case shall fail if the above behavior is
    not enforced.

    Note that DefaultSearchProviderEnabled must be not set (i.e., set to
    None) for this test to work.

    """
    version = 1

    POLICY_NAME = 'ForceGoogleSafeSearch'
    TEST_CASES = {
        'True_Safe': True,
        'False_NotSafe': False,
        'NotSet_NotSafe': None
    }
    SUPPORTING_POLICIES = {
        'DefaultSearchProviderEnabled': None
    }
    GOOGLE_SEARCH_URL = 'https://www.google.com/search?q=kittens'

    def _test_force_safe_search(self, policy_value):
        """Verify CrOS enforces ForceGoogleSafeSearch policy.

        When ForceGoogleSafeSearch is set true, then Chrome OS shall append
        the safe search parameter to the Google Search URL, and set it active.
        When set false or is not set, then Chrome OS shall not append the safe
        search parameter to the search URL.

        @param policy_value: policy value for this case.

        """
        tab = self.navigate_to_url(self.GOOGLE_SEARCH_URL)
        is_safe_search_active = True if '&safe=active' in tab.url else False
        if policy_value == True:
            if not is_safe_search_active:
                raise error.TestFail('Safe search should be active.')
        else:
            if is_safe_search_active:
                raise error.TestFail('Safe search should not be active.')

    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_force_safe_search(case_value)
