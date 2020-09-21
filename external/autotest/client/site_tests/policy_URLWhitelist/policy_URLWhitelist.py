# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_URLWhitelist(enterprise_policy_base.EnterprisePolicyTest):
    """Test effect of URLWhitleist policy on Chrome OS behavior.

    Navigate to all the websites in the BLOCKED_URLS_LIST. Verify that the
    websites specified by the URLWhitelist policy value are allowed. Also
    verify that the websites not in the URLWhitelist policy value are blocked.

    Two test cases (SinglePage_Allowed, MultiplePages_Allowed) are designed to
    verify that the URLs specified in the URLWhitelist policy are allowed.
    The third test case (NotSet_Blocked) is designed to verify that all of
    the URLs are blocked since the URLWhitelistlist policy is set to None.

    The test case shall pass if the URLs that are part of the URLWhitelist
    policy value are allowed. The test case shall also pass if the URLs that
    are not part of the URLWhitelist policy value are blocked. The test case
    shall fail if the above behavior is not enforced.

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_URLWhitelist, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'URLWhitelist'
        self.URL_BASE = '%s/%s' % (self.WEB_HOST, 'website')
        self.BLOCKED_URLS_LIST = [self.URL_BASE + website for website in
                                  ['/website1.html',
                                   '/website2.html',
                                   '/website3.html']]
        self.SINGLE_WHITELISTED_FILE = self.BLOCKED_URLS_LIST[:1]
        self.MULTIPLE_WHITELISTED_FILES = self.BLOCKED_URLS_LIST[:2]
        self.BLOCKED_USER_MESSAGE = 'Webpage Blocked'
        self.BLOCKED_ERROR_MESSAGE = 'ERR_BLOCKED_BY_ADMINISTRATOR'

        self.TEST_CASES = {
            'NotSet_Blocked': None,
            'SinglePage_Allowed': self.SINGLE_WHITELISTED_FILE,
            'MultiplePages_Allowed': self.MULTIPLE_WHITELISTED_FILES
        }
        self.SUPPORTING_POLICIES = {'URLBlacklist': self.BLOCKED_URLS_LIST}


    def _scrape_text_from_webpage(self, tab):
        """Return a list of filtered text on the web page.

        @param tab: tab containing the website to be parsed.
        @raises: TestFail if the expected text was not found on the page.
        """
        parsed_message_string = ''
        parsed_message_list = []
        page_scrape_cmd = 'document.getElementById("main-message").innerText;'
        try:
            parsed_message_string = tab.EvaluateJavaScript(page_scrape_cmd)
        except Exception as err:
                raise error.TestFail('Unable to find the expected '
                                     'text content on the test '
                                     'page: %s\n %r'%(tab.url, err))
        logging.info('Parsed message:%s', parsed_message_string)
        parsed_message_list = [str(word) for word in
                               parsed_message_string.split('\n') if word]
        return parsed_message_list


    def _is_url_blocked(self, url):
        """Return True if the URL is blocked else returns False.

        @param url: The URL to be checked whether it is blocked.
        """
        parsed_message_list = []
        tab = self.navigate_to_url(url)
        parsed_message_list = self._scrape_text_from_webpage(tab)
        if len(parsed_message_list) == 2 and \
                parsed_message_list[0] == 'Website enabled' and \
                parsed_message_list[1] == 'Website is enabled':
            return False

        # Check if accurate user error message is shown on the error page.
        if parsed_message_list[0] != self.BLOCKED_USER_MESSAGE or \
                parsed_message_list[1] != self.BLOCKED_ERROR_MESSAGE:
            logging.warning('The Blocked page user notification '
                            'messages, %s and %s are not displayed on '
                            'the blocked page. The messages may have '
                            'been modified. Please check and update the '
                            'messages in this file accordingly.',
                            self.BLOCKED_USER_MESSAGE,
                            self.BLOCKED_ERROR_MESSAGE)
        return True


    def _test_url_whitelist(self, policy_value):
        """Verify CrOS enforces URLWhitelist policy value.

        Navigate to all the websites in the BLOCKED_URLS_LIST. Verify that
        the websites specified by the URLWhitelist policy value allowed.
        Also verify that the websites not in the URLWhitelist policy value
        are blocked.

        @param policy_value: policy value expected.

        @raises: TestFail if url is blocked/not blocked based on the
                 corresponding policy values.
        """
        for url in self.BLOCKED_URLS_LIST:
            url_is_blocked = self._is_url_blocked(url)
            if policy_value:
                if url in policy_value and url_is_blocked:
                    raise error.TestFail('The URL %s should have been '
                                         'allowed by policy, but it '
                                         'was blocked.' % url)
                elif url not in policy_value and not url_is_blocked:
                    raise error.TestFail('The URL %s should have been '
                                         'blocked by policy, but it '
                                         'was allowed.' % url)

            elif not url_is_blocked:
                raise error.TestFail('The URL %s should have been blocked'
                                     'by policy, but it was allowed.' % url)


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_url_whitelist(case_value)
