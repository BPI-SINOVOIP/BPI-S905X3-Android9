# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import time

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.power import sys_power

class power_VideoSuspend(test.test):
    """Suspend the system with a video playing."""
    version = 1

    def run_once(self, video_urls=None):
        if video_urls is None:
            raise error.TestError('no videos to play')

        with chrome.Chrome(init_network_controller=True) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'play.html')))
            tab.WaitForDocumentReadyStateToBeComplete()

            for url in video_urls:
                self._suspend_with_video(cr.browser, tab, url)


    def _check_video_is_playing(self, tab):
        def _get_current_time():
            return tab.EvaluateJavaScript('player.currentTime')

        old_time = _get_current_time()
        utils.poll_for_condition(
            condition=lambda: _get_current_time() > old_time,
            exception=error.TestError('Player stuck until timeout.'))


    def _suspend_with_video(self, browser, tab, video_url):
        logging.info('testing %s', video_url)
        tab.EvaluateJavaScript('play("%s")' % video_url)

        self._check_video_is_playing(tab)

        time.sleep(2)
        sys_power.kernel_suspend(10)
        time.sleep(2)

        self._check_video_is_playing(tab)
