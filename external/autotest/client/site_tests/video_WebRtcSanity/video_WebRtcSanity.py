# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import helper_logger

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream',
                      '--use-fake-device-for-media-stream']

# Polling timeout.
SHORT_TIMEOUT_IN_SECS = 45


class video_WebRtcSanity(test.test):
    """Local getUserMedia test with fake webcam at VGA and 720p."""
    version = 1

    def start_getusermedia(self, cr):
        """Opens the test page.

        @param cr: Autotest Chrome instance.
        """
        cr.browser.platform.SetHTTPServerDirectories(self.bindir)

        self.tab = cr.browser.tabs[0]
        self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                os.path.join(self.bindir, 'getusermedia.html')))
        self.tab.WaitForDocumentReadyStateToBeComplete()

    def wait_test_completed(self, timeout_secs):
        """Waits until the test is done.

        @param timeout_secs Max time to wait in seconds.

        @returns True if test completed, False otherwise.

        """
        def _test_done():
            status = self.tab.EvaluateJavaScript('getStatus()')
            logging.debug(status);
            return status != 'running'

        utils.poll_for_condition(
            _test_done, timeout=timeout_secs, sleep_interval=1,
            desc = 'getusermedia.html reports itself as finished')

    @helper_logger.video_log_wrapper
    def run_once(self):
        """Runs the test."""
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS +\
                           [helper_logger.chrome_vmodule_flag()],
                           init_network_controller=True) as cr:
            self.start_getusermedia(cr)
            self.wait_test_completed(SHORT_TIMEOUT_IN_SECS)
            self.verify_successful()


    def verify_successful(self):
        """Checks the results of the test.

        @raises TestError if an error occurred.
        """
        status = self.tab.EvaluateJavaScript('getStatus()')
        logging.info('Status: %s', status)
        if status != 'ok-video-playing':
            raise error.TestFail('Failed: %s' % status)
