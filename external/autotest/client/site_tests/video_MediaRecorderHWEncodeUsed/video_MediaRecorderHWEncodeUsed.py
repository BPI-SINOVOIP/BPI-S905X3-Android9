# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time
import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import helper_logger

EXTRA_BROWSER_ARGS = ['--use-fake-ui-for-media-stream',
                      '--use-fake-device-for-media-stream',
                      '--enable-experimental-web-platform-features']
JS_INVOCATION = 'startRecording("%s")'
TEST_PROGRESS = 'testProgress'
TIMEOUT = 10

MEDIA_RECORDER_ERROR = 'Media.MediaRecorder.VEAError'
MEDIA_RECORDER_HW_ENC_USED = 'Media.MediaRecorder.VEAUsed'
MEDIA_RECORDER_HW_ENC_USED_BUCKET = 1

class video_MediaRecorderHWEncodeUsed(test.test):
    """This test verifies VEA works in MediaRecorder."""
    version = 1

    def is_test_completed(self):
        """Checks if MediaRecorder test is done.

        @returns True if test complete, False otherwise.

        """
        def test_done():
            """Check the testProgress variable in HTML page."""

            # Wait for test completion on web page.
            test_progress = self.tab.EvaluateJavaScript(TEST_PROGRESS)
            return test_progress == 1

        try:
            utils.poll_for_condition(
                    test_done, timeout=TIMEOUT,
                    exception=error.TestError('Cannot find testProgress.'),
                    sleep_interval=1)
        except error.TestError:
            return False
        else:
            return True

    @helper_logger.video_log_wrapper
    def run_once(self, codec):
        """
        Tests whether VEA works by verifying histogram after MediaRecorder
        records a video.
        """
        with chrome.Chrome(
                extra_browser_args=EXTRA_BROWSER_ARGS +\
                [helper_logger.chrome_vmodule_flag()],
                init_network_controller=True) as cr:
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.tab = cr.browser.tabs.New()
            self.tab.Navigate(cr.browser.platform.http_server.UrlOf(
                    os.path.join(self.bindir, 'loopback_mediarecorder.html')))
            self.tab.WaitForDocumentReadyStateToBeComplete()
            self.tab.EvaluateJavaScript(JS_INVOCATION % codec)
            if not self.is_test_completed():
                logging.error('%s did not complete', test_name)
                raise error.TestFail('Failed %s' % test_name)

            # Waits for histogram updated for the test video.
            histogram_verifier.verify(
                 cr,
                 MEDIA_RECORDER_HW_ENC_USED,
                 MEDIA_RECORDER_HW_ENC_USED_BUCKET)

            # Verify no GPU error happens.
            if histogram_verifier.is_histogram_present(
                    cr,
                    MEDIA_RECORDER_ERROR):
                logging.info(histogram_verifier.get_histogram(
                             cr, MEDIA_RECORDER_ERROR))
                raise error.TestError('GPU Video Encoder Error.')

