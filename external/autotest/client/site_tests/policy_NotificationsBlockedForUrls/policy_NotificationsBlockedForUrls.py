# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base


class policy_NotificationsBlockedForUrls(
        enterprise_policy_base.EnterprisePolicyTest):
    """Test NotificationsBlockedForUrls policy effect on CrOS behavior.

    This test verifies the behavior of Chrome OS with a set of valid values
    for the NotificationsBlockedForUrls user policy, when
    DefaultNotificationSetting=1 (i.e., allow notifications, except on
    sites listed in NotificationsBlockedForUrls). These valid values are
    covered by 3 test cases: SiteBlocked_Block, SiteAllowed_Show,
    NotSet_Show.

    When the policy value is None (as in case NotSet_Show), then notifications
    are displayed on every site. When the value is set to one or more URLs (as
    in SiteBlocked_Block and SiteAllowed_Show), notifications are allowed
    on every site except for those sites whose domain matches any of the
    listed URLs.

    A related test, policy_NotificationsAllowedForUrls, has
    DefaultNotificationsSetting=2 i.e., do not allow display of notifications by
    default, except on sites in domains listed in NotificationsAllowedForUrls).

    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_NotificationsBlockedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'NotificationsBlockedForUrls'
        self.TEST_FILE = 'notification_test_page.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.INCLUDES_BLOCKED_URL = ['http://www.bing.com', self.WEB_HOST,
                                     'https://www.yahoo.com']
        self.EXCLUDES_BLOCKED_URL = ['http://www.bing.com',
                                     'https://www.irs.gov/',
                                     'https://www.yahoo.com']
        self.TEST_CASES = {
            'SiteBlocked_Block': self.INCLUDES_BLOCKED_URL,
            'SiteAllowed_Show': self.EXCLUDES_BLOCKED_URL,
            'NotSet_Show': None
        }
        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultNotificationsSetting': 1,
            'BookmarkBarEnabled': True,
            'EditBookmarksEnabled': True,
            'RestoreOnStartupURLs': self.STARTUP_URLS,
            'RestoreOnStartup': 4
        }


    def _wait_for_page_ready(self, tab):
        """Wait for JavaScript on page in |tab| to set the pageReady flag.

        @param tab: browser tab with page to load.

        """
        utils.poll_for_condition(
            lambda: tab.EvaluateJavaScript('pageReady'),
            exception=error.TestError('Test page is not ready.'))


    def _are_notifications_blocked(self, tab):
        """Check if Notifications are blocked.

        @param: chrome tab which has test page loaded.

        @returns True if Notifications are blocked, else returns False.

        """
        notification_permission = tab.EvaluateJavaScript(
                'Notification.permission')
        if notification_permission not in ['granted', 'denied', 'default']:
            error.TestFail('Unable to capture Notification Setting.')
        return notification_permission == 'denied'


    def _test_notifications_blocked_for_urls(self, policy_value):
        """Verify CrOS enforces the NotificationsBlockedForUrls policy.

        When NotificationsBlockedForUrls is undefined, notifications shall be
        allowed on all pages. When NotificationsBlockedForUrls contains one or
        more URLs, notifications shall be blocked only on the pages whose
        domain matches any of the listed URLs.

        @param policy_value: policy value for this case.

        """
        tab = self.navigate_to_url(self.TEST_URL)
        self._wait_for_page_ready(tab)
        notifications_blocked = self._are_notifications_blocked(tab)
        logging.info('Notifications are blocked: %r', notifications_blocked)

        # String |WEB_HOST| will be found in string |policy_value| for
        # cases that expect the Notifications to be blocked.
        if policy_value is not None and self.WEB_HOST in policy_value:
            if not notifications_blocked:
                raise error.TestFail('Notifications should be blocked.')
        else:
            if notifications_blocked:
                raise error.TestFail('Notifications should be allowed.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.

        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_notifications_blocked_for_urls(case_value)
