# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.server import test
from autotest_lib.server.cros.multimedia import remote_facade_factory


class enterprise_CFM_AutotestSmokeTest(test.test):
    """
    Server side autotest smoke test for CfM platforms.
    Start the chrome browser, opens 'chrome://version' and then closes the tab.
    """
    version = 1


    def run_once(self, host):
        """Runs the smoke test."""
        factory = remote_facade_factory.RemoteFacadeFactory(host)
        browser_facade = factory.create_browser_facade()
        tab_descriptor = browser_facade.new_tab('chrome://version')
        browser_facade.close_tab(tab_descriptor)

