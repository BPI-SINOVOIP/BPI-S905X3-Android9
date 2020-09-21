# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_PopupsAllowedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """Test PopupsAllowedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the PopupsAllowedForUrls user policy, when DefaultPopupsSetting=2
    (i.e., block popups by default on all pages except those in domains listed
    in PopupsAllowedForUrls). These valid values are covered by 4 test cases,
    named: NotSet_Blocked, 1Url_Allowed, 2Urls_Blocked, and 3Urls_Allowed.

    When the policy value is None (as in case NotSet_Blocked), then popups are
    blocked on any page. When the value is set to one or more URLs (as in
    1Url_Allowed, 2Urls_Blocked, and 3Urls_Allowed), popups are allowed only
    on pages with a domain that matches any of the listed URLs, and blocked on
    any of those that do not match.

    As noted above, this test requires the DefaultPopupsSetting policy to be
    set to 2. A related test, policy_PopupsBlockedForUrls, requires the value
    to be set to 1. That value allows popups on all pages except those with
    domains listed in PopupsBlockedForUrls.
    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_PopupsAllowedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'PopupsAllowedForUrls'
        self.TEST_FILE = 'popup_status.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)

        self.URL1_DATA = [self.WEB_HOST]
        self.URL2_DATA = ['http://www.bing.com', 'https://www.yahoo.com']
        self.URL3_DATA = ['http://www.bing.com', self.WEB_HOST,
                          'https://www.yahoo.com']
        self.TEST_CASES = {
            'NotSet_Block': None,
            '1Url_Allow': self.URL1_DATA,
            '2Urls_Block': self.URL2_DATA,
            '3Urls_Allow': self.URL3_DATA
        }
        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultPopupsSetting': 2,
            'BookmarkBarEnabled': False,
            'RestoreOnStartupURLs': self.STARTUP_URLS,
            'RestoreOnStartup': 4
        }

    def _wait_for_page_ready(self, tab):
        utils.poll_for_condition(
            lambda: tab.EvaluateJavaScript('pageReady'),
            exception=error.TestError('Test page is not ready.'))


    def _test_popups_allowed_for_urls(self, policy_value):
        """Verify CrOS enforces the PopupsAllowedForUrls policy.

        When PopupsAllowedForUrls is undefined, popups shall be blocked on
        all pages. When PopupsAllowedForUrls contains one or more URLs,
        popups shall be allowed only on the pages whose domain matches any of
        the listed URLs.

        @param policy_value: policy value expected.
        """
        tab = self.navigate_to_url(self.TEST_URL)
        self._wait_for_page_ready(tab)
        is_blocked = tab.EvaluateJavaScript('isPopupBlocked();')

        # String |WEB_HOST| will be found in string |policy_value| for
        # test cases 1Url_Allowed and 3Urls_Allowed, but not for cases
        # NotSet_Blocked and 2Urls_Blocked.
        if policy_value is not None and self.WEB_HOST in policy_value:
            if is_blocked:
                raise error.TestFail('Popups should not be blocked.')
        else:
            if not is_blocked:
                raise error.TestFail('Popups should be blocked.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_popups_allowed_for_urls(case_value)
