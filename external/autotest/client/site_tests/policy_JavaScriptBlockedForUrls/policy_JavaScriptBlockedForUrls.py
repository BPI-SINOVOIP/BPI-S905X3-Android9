# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_JavaScriptBlockedForUrls(
    enterprise_policy_base.EnterprisePolicyTest):
    """Test JavaScriptBlockedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the JavaScriptBlockedForUrls user policy, covered by four named test
    cases: NotSet_Allow, SingleUrl_Block, MultipleUrls_Allow, and
    MultipleUrls_Block.

    When the policy value is None (as in test case=NotSet_Allow), then
    JavaScript execution be allowed on any page. When the policy value is set
    to a single URL pattern (as in test case=SingleUrl_Block), then
    JavaScript execution will be blocked on any page that matches that
    pattern. When set to multiple URL patterns (as case=MultipleUrls_Allow
    and MultipleUrls_Block) then JavaScript execution will be blocked on any
    page with an URL that matches any of the listed patterns.

    Two test cases (NotSet_Allow, MultipleUrls_Allow) are designed to allow
    JavaScript execution the test page. The other two test cases
    (NotSet_Allow, MultipleUrls_Block) are designed to block JavaScript
    execution on the test page.

    Note this test has a dependency on the DefaultJavaScriptSetting user
    policy, which is tested partially herein and in the test
    policy_JavaScriptAllowedForUrls. For this test, we set
    DefaultJavaScriptSetting=1. This allows JavaScript execution on all pages
    except those with a URL matching a pattern in JavaScriptBlockedForUrls.
    In the test policy_JavaScriptAllowedForUrls, we set
    DefaultJavaScriptSetting=2. That test blocks JavaScript execution on all
    pages except those with an URL matching a pattern in
    JavaScriptAllowedForUrls.

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_JavaScriptBlockedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'JavaScriptBlockedForUrls'
        self.TEST_FILE = 'js_test.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.TEST_CASES = {
            'NotSet_Allow': None,
            'SingleUrl_Block': [self.WEB_HOST],
            'MultipleUrls_Allow': ['http://www.bing.com',
                                   'https://www.yahoo.com'],
            'MultipleUrls_Block': ['http://www.bing.com',
                                   self.TEST_URL,
                                   'https://www.yahoo.com']
        }

        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultJavaScriptSetting': 1,
            'BookmarkBarEnabled': False,
            'RestoreOnStartupURLs': self.STARTUP_URLS,
            'RestoreOnStartup': 4
        }


    def _can_execute_javascript(self, tab):
        """Determine whether JavaScript is allowed to run on the given page.

        @param tab: browser tab containing JavaScript to run.

        """
        try:
            utils.poll_for_condition(
                lambda: tab.EvaluateJavaScript('jsAllowed', timeout=2),
                exception=error.TestError('Test page is not ready.'))
            return True
        except:
            return False


    def _test_javascript_blocked_for_urls(self, policy_value):
        """Verify CrOS enforces the JavaScriptBlockedForUrls policy.

        When JavaScriptBlockedForUrls is undefined, JavaScript execution shall
        be allowed on all pages. When JavaScriptBlockedForUrls contains one or
        more URL patterns, JavaScript execution shall be blocked only on the
        pages whose URL matches any of the listed patterns.

        Note: This test does not use self.navigate_to_url(), because it can
        not depend on methods that evaluate or execute JavaScript.

        @param policy_value: policy value for this case.

        """
        tab = self.cr.browser.tabs.New()
        tab.Activate()
        tab.Navigate(self.TEST_URL)
        time.sleep(1)

        utils.poll_for_condition(
            lambda: tab.url == self.TEST_URL,
            exception=error.TestError('Test page is not ready.'))
        javascript_is_allowed = self._can_execute_javascript(tab)

        if policy_value is not None and (self.WEB_HOST in policy_value or
                                         self.TEST_URL in policy_value):
            # If |WEB_HOST| is in |policy_value|, then JavaScript execution
            # should be blocked. If execution is allowed, raise an error.
            if javascript_is_allowed:
                raise error.TestFail('JavaScript should be blocked.')
        else:
            if not javascript_is_allowed:
                raise error.TestFail('JavaScript should be allowed.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_javascript_blocked_for_urls(case_value)
