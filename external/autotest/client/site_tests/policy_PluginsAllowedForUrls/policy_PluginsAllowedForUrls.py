# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import utils

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.enterprise import enterprise_policy_base
from autotest_lib.client.cros.audio import audio_helper


class policy_PluginsAllowedForUrls(
        enterprise_policy_base.EnterprisePolicyTest):
    """Test PluginsAllowedForUrls policy effect on CrOS behavior.

    This test verifies the behavior of Chrome OS with a set of valid values
    for the PluginsAllowedForUrls user policy, when DefaultPluginsSetting=2
    (i.e., block running of plugins by default, except on sites listed in
    PluginsAllowedForUrls). These valid values are covered by 3 test cases:
    SiteAllowed_Run, SiteNotAllowed_Block, NotSet_Block.

    This test is also configured with DisablePluginFinder=True and
    AllowOutdatedPlugins=False.

    When the policy value is None (as in case NotSet_Block), then running of
    plugins is blocked on every site. When the value is set to one or more
    URLs (as in SiteAllowed_Run and SiteNotAllowed_Block), plugins are blocked
    on every site except for those sites whose domain matches any of the
    listed URLs.

    A related test, policy_PluginsBlockedForUrls, has DefaultPluginsSetting=1
    (i.e., allow running of plugins by default, except on sites in domains
    listed in PluginsBlockedForUrls).
    """
    version = 1

    def initialize(self, **kwargs):
        """Initialize this test."""
        self._initialize_test_constants()
        super(policy_PluginsAllowedForUrls, self).initialize(**kwargs)
        self.start_webserver()


    def _initialize_test_constants(self):
        """Initialize test-specific constants, some from class constants."""
        self.POLICY_NAME = 'PluginsAllowedForUrls'
        self.TEST_FILE = 'plugin_status.html'
        self.TEST_URL = '%s/%s' % (self.WEB_HOST, self.TEST_FILE)
        self.INCLUDES_ALLOWED_URL = ['http://www.bing.com', self.WEB_HOST,
                                     'https://www.yahoo.com']
        self.EXCLUDES_ALLOWED_URL = ['http://www.bing.com',
                                     'https://www.irs.gov/',
                                     'https://www.yahoo.com']
        self.TEST_CASES = {
            'SiteAllowed_Run': self.INCLUDES_ALLOWED_URL,
            'SiteNotAllowed_Block': self.EXCLUDES_ALLOWED_URL,
            'NotSet_Block': None
        }
        self.STARTUP_URLS = ['chrome://policy', 'chrome://settings']
        self.SUPPORTING_POLICIES = {
            'DefaultPluginsSetting': 2,
            'DisablePluginFinder': True,
            'AllowOutdatedPlugins': False,
            'AlwaysAuthorizePlugins': False,
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


    def _stop_flash_if_running(self, timeout_sec=10):
        """Terminate all Shockwave Flash processes.

        @param timeout_sec: maximum seconds to wait for processes to die.
        @raises: error.AutoservPidAlreadyDeadError if Flash process is dead.
        @raises: utils.TimeoutError if Flash processes are still running
                 after timeout_sec.
        """
        def kill_flash_process():
            """Kill all running flash processes."""
            pids = utils.get_process_list('chrome', '--type=ppapi')
            for pid in pids:
                try:
                    utils.nuke_pid(int(pid))
                except error.AutoservPidAlreadyDeadError:
                    pass
            return pids

        utils.poll_for_condition(lambda: kill_flash_process() == [],
                                 timeout=timeout_sec)


    def _is_flash_running(self):
        """Check if a Shockwave Flash process is running.

        @returns: True if one or more flash processes are running.
        """
        flash_pids = utils.get_process_list('chrome', '--type=ppapi')
        return flash_pids != []


    def _test_plugins_allowed_for_urls(self, policy_value):
        """Verify CrOS enforces the PluginsAllowedForUrls policy.

        When PluginsAllowedForUrls is undefined, plugins shall be blocked on
        all pages. When PluginsAllowedForUrls contains one or more URLs,
        plugins shall be allowed only on the pages whose domain matches any of
        the listed URLs.

        @param policy_value: policy value expected.
        """
        # Set a low audio volume to avoid annoying people during tests.
        audio_helper.set_volume_levels(10, 100)

        # Kill any running Shockwave Flash processes.
        self._stop_flash_if_running()

        # Open page with an embedded flash file.
        tab = self.navigate_to_url(self.TEST_URL)
        self._wait_for_page_ready(tab)

        # Check if Shockwave Flash process is running.
        plugin_is_running = self._is_flash_running()
        logging.info('plugin_is_running: %r', plugin_is_running)

        # String |WEB_HOST| will be found in string |policy_value| for
        # cases that expect the plugin to be run.
        if policy_value is not None and self.WEB_HOST in policy_value:
            if not plugin_is_running:
                raise error.TestFail('Plugins should run.')
        else:
            if plugin_is_running:
                raise error.TestFail('Plugins should not run.')
        tab.Close()


    def run_once(self, case):
        """Setup and run the test configured for the specified test case.

        @param case: Name of the test case to run.
        """
        case_value = self.TEST_CASES[case]
        self.SUPPORTING_POLICIES[self.POLICY_NAME] = case_value
        self.setup_case(user_policies=self.SUPPORTING_POLICIES)
        self._test_plugins_allowed_for_urls(case_value)
