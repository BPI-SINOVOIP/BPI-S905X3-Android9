# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_RestoreOnStartupURLs(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of RestoreOnStartupURLs policy on Chrome OS behavior.

    This test verifies the behavior of Chrome OS for a range of valid values
    in the RestoreOnStartupURLs user policy. It also exercises the dependent
    user policy RestoreOnStartup, which must be set to 4 to utilize the
    specified startup URLs, and to None to when no URLs are specified.

    The combination of policy values are covered by three test cases named:
    NotSet_NoTabs, SingleUrl_1Tab, and MultipleUrls_3Tabs.
    - Case NotSet_NoTabs opens no tabs. This is the default behavior for
      un-managed user and guest user sessions.
    - Case SingleUrl_1Tab opens a single tab to chrome://settings.
    - Case MultipleUrls_3Tabs opens 3 tabs, in order, to the following pages:
      'chrome://policy', 'chrome://settings', and 'chrome://histograms'

    """
    version = 1

    POLICY_NAME = 'RestoreOnStartupURLs'
    URLS1_DATA = ['chrome://settings']
    URLS3_DATA = ['chrome://policy', 'chrome://settings',
                  'chrome://histograms']
    NEWTAB_URLS = ['chrome://newtab',
                   'https://www.google.com/_/chrome/newtab?espv=2&ie=UTF-8']

    TEST_CASES = {
        'NotSet_NoTabs': None,
        'SingleUrl_1Tab': URLS1_DATA,
        'MultipleUrls_3Tabs': URLS3_DATA
    }

    def _test_startup_urls(self, policy_value):
        """Verify CrOS enforces RestoreOnStartupURLs policy value.

        When RestoreOnStartupURLs policy is set to one or more URLs, check
        that a tab is opened to each URL. When set to None, check that no tab
        is opened.

        @param policy_value: policy value expected.

        """
        # Get list of open tab urls from browser; Convert unicode to text;
        # Strip any trailing '/' character reported by devtools.
        tab_urls = [tab.url.encode('utf8').rstrip('/')
                    for tab in self.cr.browser.tabs]

        # Telemetry always opens a 'newtab' tab if no startup tabs are opened.
        if policy_value is None:
            if len(tab_urls) != 1 or tab_urls[0] not in self.NEWTAB_URLS:
                raise error.TestFail('Unexpected tabs: %s (expected: NEWTAB)' %
                                     tab_urls)

        # Compare open tabs with expected tabs by |policy_value|.
        elif set(tab_urls) != set(policy_value):
            raise error.TestFail('Unexpected tabs: %s (expected: %s)' %
                                 (tab_urls, policy_value))


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| string and |policies_dict| data based
        on the test |case|. Set RestoreOnStartup=4 when there is 1 or more
        startup urls given. Otherwise, set to None.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        if case_value == None:
            user_policies = {'RestoreOnStartup': None}
        else:
            user_policies = {'RestoreOnStartup': 4}
        user_policies[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=user_policies)
        self._test_startup_urls(case_value)
