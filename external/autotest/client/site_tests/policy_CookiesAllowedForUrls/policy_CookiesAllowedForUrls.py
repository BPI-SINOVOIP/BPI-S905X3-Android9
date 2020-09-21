# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_CookiesAllowedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of the CookiesAllowedForUrls policy on Chrome OS behavior.

    This test implicitly verifies one value of the DefaultCookiesSetting
    policy as well. When the DefaultCookiesSetting policy value is set to 2,
    cookies for all URLs shall not be stored (ie, shall be blocked), except
    for the URL patterns specified by the CookiesAllowedForUrls policy.

    The test verifies ChromeOS behaviour for different values of the
    CookiesAllowedForUrls policy, i.e., for the policy value set to Not Set,
    set to a single url/host pattern, or when the policy is set to multiple
    url/host patterns. It also verifies that cookies are blocked for urls that
    are not part of the policy value.

    The corresponding three test cases are NotSet_CookiesBlocked,
    SingleUrl_CookiesAllowed, MultipleUrls_CookiesAllowed, and
    MultipleUrls_CookiesBlocked.
    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_CookiesAllowedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'CookiesAllowedForUrls'
        self.COOKIE_NAME = 'cookie1'
        self.TEST_FILE = 'cookie_status.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.COOKIE_ALLOWED_SINGLE_FILE = [self.WEB_HOST]
        self.COOKIE_ALLOWED_MULTIPLE_FILES = ['http://google.com',
                                              self.WEB_HOST,
                                              'http://doesnotmatter.com']
        self.COOKIE_BLOCKED_MULTIPLE_FILES = ['https://testingwebsite.html',
                                              'https://somewebsite.com',
                                              'http://doesnotmatter.com']
        self.TEST_CASES = {
            'NotSet_Block': None,
            'SingleUrl_Allow': self.COOKIE_ALLOWED_SINGLE_FILE,
            'MultipleUrls_Allow': self.COOKIE_ALLOWED_MULTIPLE_FILES,
            'MultipleUrls_Block': self.COOKIE_BLOCKED_MULTIPLE_FILES
        }
        self.SUPPORTING_POLICIES = {'DefaultCookiesSetting': 2}


    def _is_cookie_blocked(self, url):
        """Return True if cookie is blocked for the URL else return False.

        @param url: Url of the page which is loaded to check whether it's
                    cookie is blocked or stored.
        """
        tab = self.navigate_to_url(url)
        return tab.GetCookieByName(self.COOKIE_NAME) is None


    def _test_cookies_allowed_for_urls(self, policy_value):
        """Verify CrOS enforces CookiesAllowedForUrls policy value.

        When the CookiesAllowedForUrls policy is set to one or more urls/hosts,
        check that cookies are not blocked for the urls/urlpatterns listed in
        the policy value. When set to None, check that cookies are blocked for
        all URLs.

        @param policy_value: policy value for this case.
        @raises: TestFail if cookies are blocked/not blocked based on the
                 corresponding policy values.
        """
        cookie_is_blocked = self._is_cookie_blocked(self.TEST_URL)

        if policy_value and self.WEB_HOST in policy_value:
            if cookie_is_blocked:
                raise error.TestFail('Cookies should be allowed.')
        else:
            if not cookie_is_blocked:
                raise error.TestFail('Cookies should be blocked.')


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        Set the expected |policy_value| and |policies_dict| data defined for
        the specified test |case|, and run the test.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_cookies_allowed_for_urls(case_value)
