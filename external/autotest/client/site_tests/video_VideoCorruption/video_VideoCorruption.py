# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player
from autotest_lib.client.cros.video import helper_logger


class video_VideoCorruption(test.test):
    """This test verifies playing corrupted video in Chrome."""
    version = 1

    @helper_logger.video_log_wrapper
    def run_once(self, video):
        """Tests whether Chrome handles corrupted videos gracefully.

        @param video: Sample corrupted video file to be played in Chrome.
        """
        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag(),
                init_network_controller=True) as cr:
            shutil.copy2(constants.VIDEO_HTML_FILEPATH, self.bindir)
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            html_fullpath = os.path.join(self.bindir, 'video.html')
            url = cr.browser.platform.http_server.UrlOf(html_fullpath)
            player = native_html5_player.NativeHtml5Player(tab,
                 full_url = url,
                 video_id = 'video',
                 video_src_path = video,
                 event_timeout = 120)
            #This is a corrupted video, so it cann't load for checking canplay.
            player.load_video(wait_for_canplay=False)
            # Expect corruption being detected after loading corrupted video.
            player.wait_for_error()
