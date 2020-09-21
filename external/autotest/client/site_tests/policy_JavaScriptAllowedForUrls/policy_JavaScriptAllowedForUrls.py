# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_JavaScriptAllowedForUrls(
    enterprise_policy_base.EnterprisePolicyTest):
    """Test JavaScriptAllowedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the JavaScriptAllowedForUrls user policies. These values are covered
    by four test cases, named: NotSet_Block, SingleUrl_Allow,
    MultipleUrls_Block, and MultipleUrls_Allow.

    When the policy value is None (as in case=NotSet_Block), then
    JavaScript will be blocked on any page. When the value is set to a single
    URL pattern (as in case=SingleUrl_Allow), JavaScript will be allowed on
    any page that matches that pattern. When set to multiple URL patterns (as
    in case=MultipleUrls_Block or MultipleUrls_Allow) then JavaScript will
    be allowed on any page with a URL that matches any of the listed patterns.

    Two test cases (SingleUrl_Allow, MultipleUrls_Allow) are designed to allow
    JavaScript to run on the test page. The other two test cases
    (NotSet_Block, MultipleUrls_Block) are designed to block JavaScript
    from running on the test page.

    Note this test has a dependency on the DefaultJavaScriptSetting policy,
    which is partially tested herein, and in policy_JavaScriptBlockedForUrls.
    For this test, we set DefaultJavaScriptSetting=2. This blocks JavaScript
    on all pages except those with a URL matching a pattern in
    JavaScriptAllowedForUrls. For the test policy_JavaScriptBlockedForUrls, we
    set DefaultJavaScriptSetting=1. That allows JavaScript to be run on all
    pages except those with URLs that match patterns listed in
    JavaScriptBlockedForUrls.

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_JavaScriptAllowedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'JavaScriptAllowedForUrls'
        self.TEST_FILE = 'js_test.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.TEST_CASES = {
            'NotSet_Block': None,
            'SingleUrl_Allow': [self.WEB_HOST],
            'MultipleUrls_Block': ['http://www.bing.com',
                                   'https://www.yahoo.com'],
            'MultipleUrls_Allow': ['http://www.bing.com',
                                   self.TEST_URL,
                                   'https://www.yahoo.com']
        }

        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultJavaScriptSetting': 2,
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


    def _test_javascript_allowed_for_urls(self, policy_value):
        """Verify CrOS enforces the JavaScriptAllowedForUrls policy.

        When JavaScriptAllowedForUrls is undefined, JavaScript execution shall
        be blocked on all pages. When JavaScriptAllowedForUrls contains one or
        more URL patterns, JavaScript execution shall be allowed only on the
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
            # should be allowed. If execution is blocked, raise an error.
            if not javascript_is_allowed:
                raise error.TestFail('JavaScript should be allowed.')
        else:
            if javascript_is_allowed:
                raise error.TestFail('JavaScript should be blocked.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_javascript_allowed_for_urls(case_value)
