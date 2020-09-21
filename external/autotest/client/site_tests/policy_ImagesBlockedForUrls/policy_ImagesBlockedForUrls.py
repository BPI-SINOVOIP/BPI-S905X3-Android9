# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_ImagesBlockedForUrls(enterprise_policy_base.EnterprisePolicyTest):
    """Test ImagesBlockedForUrls policy effect on CrOS look & feel.

    This test verifies the behavior of Chrome OS with a range of valid values
    for the ImagesBlockedForUrls user policies. These values are covered by
    four test cases, named: NotSet_Allow, 1Url_Block, 2Urls_Allow, and
    3Urls_Block.

    When the policy value is None (as in case=NotSet_Allow), then images are
    not blocked on any page. When the value is set to a single domain (such as
    case=1Url_Block), then images are blocked on any page with that domain.
    When set to multiple domains (as in case=2Urls_Allow or 3Urls_Block),
    then images are blocked on any page with a domain that matches any of the
    listed domains.

    Two test cases (1Url_Block, 3Urls_Block) are designed to block images
    on the test page. The other two test cases (NotSet_Allow, 2Urls_Allow)
    are designed to allow images to be shown on the test page.

    Note this test has a dependency on the DefaultImagesSetting policy, which
    is partially tested herein, and by the test policy_ImagesAllowedForUrls.
    For this test, we set DefaultImagesSetting=1 (or null). This allows images
    on all pages except those with a domain listed in ImagesBlockedForUrls.
    For the test of ImagesAllowedForUrls, we set DefaultImagesSetting=2.
    That blocks images on all pages except those with domains listed in
    ImagesAllowedForUrls.

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_ImagesBlockedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'ImagesBlockedForUrls'
        self.TEST_FILE = 'kittens.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.MINIMUM_IMAGE_WIDTH = 640
        self.URL1_DATA = [self.WEB_HOST]
        self.URL2_DATA = ['http://www.bing.com', 'https://www.yahoo.com']
        self.URL3_DATA = ['http://www.bing.com', self.WEB_HOST,
                          'https://www.yahoo.com']
        self.TEST_CASES = {
            'NotSet_Allow': None,
            '1Url_Block': self.URL1_DATA,
            '2Urls_Allow': self.URL2_DATA,
            '3Urls_Block': self.URL3_DATA
        }
        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultImagesSetting': 1,
            'BookmarkBarEnabled': False,
            'RestoreOnStartupURLs': self.STARTUP_URLS,
            'RestoreOnStartup': 4
        }


    def _wait_for_page_ready(self, tab):
        utils.poll_for_condition(
            lambda: tab.EvaluateJavaScript('pageReady'),
            exception=error.TestError('Test page is not ready.'))


    def _is_image_blocked(self, tab):
        image_width = tab.EvaluateJavaScript(
            "document.getElementById('kittens_id').naturalWidth")
        return image_width < self.MINIMUM_IMAGE_WIDTH


    def _test_images_blocked_for_urls(self, policy_value):
        """Verify CrOS enforces the ImagesBlockedForUrls policy.

        When ImagesBlockedForUrls is undefined, images shall be allowed on
        all pages. When ImagesBlockedForUrls contains one or more URLs, images
        shall be blocked on any page whose domain matches any of the listed
        domains.

        @param policy_value: policy value for this case.
        """
        tab = self.navigate_to_url(self.TEST_URL)
        self._wait_for_page_ready(tab)
        image_is_blocked = self._is_image_blocked(tab)

        # String |WEB_HOST| shall be found in string |policy_value| for test
        # cases 1Url_Block and 3Urls_Block, but not for NotSet_Allow and
        # 2Urls_Allow.
        if policy_value is not None and self.WEB_HOST in policy_value:
            if not image_is_blocked:
                raise error.TestFail('Image should be blocked.')
        else:
            if image_is_blocked:
                raise error.TestFail('Image should not be blocked.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_images_blocked_for_urls(case_value)
