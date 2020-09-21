# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, file_utils, utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import helper_logger


# Chrome flags to use fake camera and skip camera permission.
EXTRA_BROWSER_ARGS = ['--use-fake-device-for-media-stream',
                      '--use-fake-ui-for-media-stream']
FAKE_FILE_ARG = '--use-file-for-fake-video-capture="%s"'
DOWNLOAD_BASE = 'http://commondatastorage.googleapis.com/chromiumos-test-assets-public/crowd/'

HISTOGRAMS_URL = 'chrome://histograms/'


class video_ChromeRTCHWDecodeUsed(test.test):
    """The test verifies HW Encoding for WebRTC video."""
    version = 1


    def is_skipping_test(self):
        """Determine whether this test should skip."""
        blacklist = [
                # (board, milestone); None if don't care.

                # kevin did support hw decode, but not ready in M54 and M55.
                ('kevin', 54), ('kevin', 55),

                # elm and hana support hw decode since M57.
                ('elm', 56), ('hana', 56),
        ]

        entry = (utils.get_current_board(), utils.get_chrome_milestone())
        for black_entry in blacklist:
            for i, to_match in enumerate(black_entry):
                if to_match and str(to_match) != entry[i]:
                    break
            else:
                return True

        return False

    def start_loopback(self, cr):
        """
        Opens WebRTC loopback page.

        @param cr: Autotest Chrome instance.
        """
        tab = cr.browser.tabs.New()
        tab.Navigate(cr.browser.platform.http_server.UrlOf(
            os.path.join(self.bindir, 'loopback.html')))
        tab.WaitForDocumentReadyStateToBeComplete()

    @helper_logger.video_log_wrapper
    def run_once(self, video_name, histogram_name, histogram_bucket_val):
        if self.is_skipping_test():
            raise error.TestNAError('Skipping test run on this board.')

        # Download test video.
        url = DOWNLOAD_BASE + video_name
        local_path = os.path.join(self.bindir, video_name)
        file_utils.download_file(url, local_path)

        # Start chrome with test flags.
        EXTRA_BROWSER_ARGS.append(FAKE_FILE_ARG % local_path)
        EXTRA_BROWSER_ARGS.append(helper_logger.chrome_vmodule_flag())
        with chrome.Chrome(extra_browser_args=EXTRA_BROWSER_ARGS,
                           init_network_controller=True) as cr:
            # Open WebRTC loopback page.
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            self.start_loopback(cr)

            # Make sure decode is hardware accelerated.
            histogram_verifier.verify(cr, histogram_name, histogram_bucket_val)
