# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import time
import shutil
import logging

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error, utils
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import histogram_verifier
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player
from autotest_lib.client.cros.video import helper_logger


class video_ChromeHWDecodeUsed(test.test):
    """This test verifies VDA works in Chrome."""
    version = 1

    def is_skipping_test(self, codec):
        """Determine whether this test should skip.

        @param codec: the codec to be tested. Example values: 'vp8', 'vp9', 'h264'.
        """
        blacklist = [
                # (board, milestone, codec); None if don't care.

                # kevin did support hw decode, but not ready in M54 and M55.
                ('kevin', 54, 'vp8'),('kevin', 55, 'vp8')
        ]

        entry = (utils.get_current_board(), utils.get_chrome_milestone(), codec)
        for black_entry in blacklist:
            for i, to_match in enumerate(black_entry):
                if to_match and str(to_match) != entry[i]:
                    break
            else:
                return True

        return False

    @helper_logger.video_log_wrapper
    def run_once(self, codec, is_mse, video_file, arc_mode=None):
        """
        Tests whether VDA works by verifying histogram for the loaded video.

        @param is_mse: bool, True if the content uses MSE, False otherwise.
        @param video_file: Sample video file to be loaded in Chrome.

        """
        if self.is_skipping_test(codec):
            raise error.TestNAError('Skipping test run on this board.')

        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag(),
                arc_mode=arc_mode,
                init_network_controller=True) as cr:
            # This will execute for MSE video by accesing shaka player
            if is_mse:
                 tab1 = cr.browser.tabs.New()
                 tab1.Navigate(video_file)
                 tab1.WaitForDocumentReadyStateToBeComplete()
                 # Running the test longer to check errors and longer playback
                 # for MSE videos.
                 time.sleep(30)
            else:
                 #This execute for normal video for downloading html file
                 shutil.copy2(constants.VIDEO_HTML_FILEPATH, self.bindir)
                 video_path = os.path.join(constants.CROS_VIDEO_DIR,
                                           'files', video_file)
                 shutil.copy2(video_path, self.bindir)

                 cr.browser.platform.SetHTTPServerDirectories(self.bindir)
                 tab = cr.browser.tabs.New()
                 html_fullpath = os.path.join(self.bindir, 'video.html')
                 url = cr.browser.platform.http_server.UrlOf(html_fullpath)

                 player = native_html5_player.NativeHtml5Player(
                         tab,
                         full_url = url,
                         video_id = 'video',
                         video_src_path = video_file,
                         event_timeout = 120)
                 player.load_video()
                 player.play()
                 # Waits until the video ends or an error happens.
                 player.wait_ended_or_error()

            # Waits for histogram updated for the test video.
            histogram_verifier.verify(
                 cr,
                 constants.MEDIA_GVD_INIT_STATUS,
                 constants.MEDIA_GVD_BUCKET)

            # Verify no GPU error happens.
            if histogram_verifier.is_histogram_present(
                    cr,
                    constants.MEDIA_GVD_ERROR):
                logging.info(histogram_verifier.get_histogram(
                             cr, constants.MEDIA_GVD_ERROR))
                raise error.TestError('GPU Video Decoder Error.')

            # Verify the video ends successully for normal videos.
            if not is_mse and player.check_error():
                raise error.TestError('player did not end successully '\
                                      '(HTML5 Player Error %s: %s)'
                                      % player.get_error_info())
