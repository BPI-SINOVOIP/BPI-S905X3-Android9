# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil

from autotest_lib.client.bin import test
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player
from autotest_lib.client.cros.video import helper_logger

WAIT_TIMEOUT_S = 30

class video_VideoReload(test.test):
    """This test verifies reloading video works in Chrome."""
    version = 1

    @helper_logger.video_log_wrapper
    def run_once(self, video_file):
        """Tests whether Chrome reloads video after reloading the tab.

        @param video_file: fullpath to video to play.
        """
        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag(),
                init_network_controller=True) as cr:
            shutil.copy2(constants.VIDEO_HTML_FILEPATH, self.bindir)
            cr.browser.platform.SetHTTPServerDirectories(self.bindir)
            tab = cr.browser.tabs[0]
            html_fullpath = os.path.join(self.bindir, 'video.html')
            url = cr.browser.platform.http_server.UrlOf(html_fullpath)
            player = native_html5_player.NativeHtml5Player(
                 tab,
                 full_url = url,
                 video_id = 'video',
                 video_src_path = video_file,
                 event_timeout = 30)
            player.load_video()
            player.play()
            player.verify_video_can_play(5)
            player.reload_page()
            player.load_video()
            player.play()
            player.verify_video_can_play(5)
