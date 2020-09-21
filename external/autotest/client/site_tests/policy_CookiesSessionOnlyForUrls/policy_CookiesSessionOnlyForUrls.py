# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_CookiesSessionOnlyForUrls(
        enterprise_policy_base.EnterprisePolicyTest):
    """
    Test effect of CookiesSessionOnlyForUrls policy on Chrome OS.

    The test verifies ChromeOS behaviour and appearance for a set of valid
    values of the CookiesSessionOnlyForUrls user policy, when user policy
    DefaultCookiesSetting=2 (block cookies for all URLs). Generally, cookies
    shall not be stored for any visted page, except for those whose domain
    matches an URL pattern specified in CookiesSessionOnlyForUrls. Also,
    these URL patterns shall have their behavior set to 'Clear on exit',
    indicating that they are marked for deletion when Chrome exits.

    If the policy value Not set, then no visited page is allowed to store
    cookies. In the same way, if the URL of the visited page is not listed in
    the policy, then the visted page is not allowed to store cookies. If the
    URL of the visited page is listed in the policy, then the page is allowed
    to store cookies for the current session only. The corresponding test
    cases are NotSet_Block, UrlNotIn_Block, and UrlIsIn_Allow.

    Note that this test does not verify that cookies set to 'Clear on exit'
    are actually deleted when the session ends. That functionality is tested
    by the Chrome team.

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_CookiesSessionOnlyForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'CookiesSessionOnlyForUrls'
        self.COOKIE_NAME = 'cookie1'
        self.TEST_FILE = 'cookie_status.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.COOKIE_EXCEPTIONS_PAGE = (
            'chrome://settings-frame/contentExceptions#cookies')
        self.COOKIE_ALLOWED_MULTIPLE_URLS = ['https://testingwebsite.html',
                                             self.WEB_HOST,
                                             'http://doesnotmatter.com']
        self.COOKIE_BLOCKED_MULTIPLE_URLS = ['https://testingwebsite.html',
                                             'https://somewebsite.com',
                                             'http://doesnotmatter.com']
        self.TEST_CASES = {
            'UrlIsIn_Allow': self.COOKIE_ALLOWED_MULTIPLE_URLS,
            'UrlNotIn_Block': self.COOKIE_BLOCKED_MULTIPLE_URLS,
            'NotSet_Block': None
        }
        self.SUPPORTING_POLICIES = {'DefaultCookiesSetting': 2}


    def _is_cookie_blocked(self, url):
        """
        Return True if cookie is blocked for the URL, else return False.

        @param url: URL of the page to load.

        """
        tab = self.navigate_to_url(url)
        cookie_value = tab.GetCookieByName(self.COOKIE_NAME)
        tab.Close()
        return cookie_value is None


    def _is_cookie_clear_on_exit(self, url):
        """
        Return True if cookie for |url| has behavior set to 'Clear on exit'.

        @param url: string url pattern for cookie exception.
        @returns: True if cookie behavior is set to 'Clear on exit'.
        """
        js_cmd = ('''
          var exception_area=document.getElementById('content-settings-exceptions-area');
          var contents=exception_area.getElementsByClassName('content-area')[0];
          var contents_children = contents.children;
          var cookie_idx = -1;
          var cookie_behavior = '';
          for (var i=0; i<contents_children.length; i++) {
            var content = contents_children[i];
            var type = content.getAttribute('contenttype');
            if (type == 'cookies') {
              var cookie_items = content.getElementsByClassName('deletable-item');
              for (var j=0; j<cookie_items.length; j++) {
                var cookie_item = cookie_items[j];
                var cookie_pattern = cookie_item.getElementsByClassName('exception-pattern')[0];
                var pattern = cookie_pattern.innerText.trim();
                var cookie_setting = cookie_item.getElementsByClassName('exception-setting')[0];
                var setting = cookie_setting.innerText.trim();
                if (pattern == '%s') {
                  cookie_idx = j;
                  cookie_behavior = setting;
                  break;
                }
              }
              break;
            }
            if (cookie_idx >= 0) { break; }
          }
          cookie_behavior;
        ''' % url)
        tab = self.navigate_to_url(self.COOKIE_EXCEPTIONS_PAGE)
        cookie_behavior = self.get_elements_from_page(tab, js_cmd)
        tab.Close()
        return cookie_behavior == 'Clear on exit'


    def _test_cookies_allowed_for_urls(self, policy_value):
        """
        Verify CrOS enforces CookiesSessionOnlyForUrls policy value.

        When CookiesSessionOnlyForUrls policy is set to a list of one or more
        more urls, verify that cookies are allowed for a page that matches a
        URL pattern in the list, but are blocked for a page whose URL pattern
        is not in the list. When set to None, verify that cookies are
        blocked for all URLs.

        @param policy_value: policy value expected.

        @raises: TestFail if cookies are blocked/not blocked based on the
                 policy value.

        """
        cookie_is_blocked = self._is_cookie_blocked(self.TEST_URL)
        if policy_value and self.WEB_HOST in policy_value:
            if cookie_is_blocked:
                raise error.TestFail('Cookie should be allowed.')
        else:
            if not cookie_is_blocked:
                raise error.TestFail('Cookie should be blocked.')

        cookie_is_clear_on_exit = self._is_cookie_clear_on_exit(self.WEB_HOST)
        if policy_value and self.WEB_HOST in policy_value:
            if not cookie_is_clear_on_exit:
                raise error.TestFail('Cookie should be Clear on exit.')
        else:
            if cookie_is_clear_on_exit:
                raise error.TestFail('Cookie should not be Clear on exit.')


    def run_once(self, case):
        """
        Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_cookies_allowed_for_urls(case_value)
