# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import fnmatch
import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import helper_logger

WAIT_TIMEOUT_S = 180

class video_VideoSeek(test.test):
    """This test verifies video seek works in Chrome."""
    version = 1

    def is_skipping_test(self, codec, is_switchres):
        """Determine whether this test should skip.

        @param codec: the codec to be tested, ex. 'vp8', 'vp9', 'h264'.
        @param is_switchres: bool, True if using switch resolution video.
        """
        blacklist = [
                # (board, codec, is_switchres); None if don't care.

                # "board" supports Unix shell-type wildcards

                # Disable vp8 switchres for nyan devices temporarily due to:
                # crbug/699260
                ('nyan', 'vp8', True), ('nyan_*', 'vp8', True)
        ]

        board = utils.get_current_board()

        for entry in blacklist:
            if ((entry[0] is None or fnmatch.fnmatch(board, entry[0])) and
                (entry[1] is None or codec == entry[1]) and
                (entry[2] is None or is_switchres == entry[2])):
                return True

        return False


    @helper_logger.video_log_wrapper
    def run_once(self, codec, is_switchres, video):
        """Tests whether video seek works by random seeks forward and backward.

        @param codec: the codec to be tested, ex. 'vp8', 'vp9', 'h264'.
        @param is_switchres: bool, True if using switch resolution video.
        @param video: Sample video file to be seeked in Chrome.
        """
        if self.is_skipping_test(codec, is_switchres):
            logging.info('Skipping test run on this board.')
            return  # return immediately to pass this test

        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag(),
                init_network_controller=True) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            tab.Navigate(cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir, 'video.html')))
            tab.WaitForDocumentReadyStateToBeComplete()

            tab.EvaluateJavaScript('loadSourceAndRunSeekTest("%s")' % video)

            def get_seek_test_status():
                seek_test_status = tab.EvaluateJavaScript('getSeekTestStatus()')
                logging.info('Seeking: %s', seek_test_status)
                return seek_test_status

            utils.poll_for_condition(
                    lambda: get_seek_test_status() == 'pass',
                    exception=error.TestError('Seek test is stuck and timeout'),
                    timeout=WAIT_TIMEOUT_S,
                    sleep_interval=1)
