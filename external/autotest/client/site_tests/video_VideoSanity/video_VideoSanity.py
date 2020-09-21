# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os
import shutil
import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib.cros import chrome, arc_common
from autotest_lib.client.cros.video import constants
from autotest_lib.client.cros.video import native_html5_player
from autotest_lib.client.cros.video import helper_logger

class video_VideoSanity(test.test):
    """This test verify the media elements and video sanity.

    - verify support for mp4, vp8 and vp9  media.
    - verify html5 video playback.

    """
    version = 2


    @helper_logger.video_log_wrapper
    def run_once(self, video_file, arc_mode=False):
        """
        Tests whether the requested video is playable

        @param video_file: Sample video file to be played in Chrome.

        """
        blacklist = [
            # (board, arc_mode) # None means don't care
            ('x86-mario', None),
            ('x86-zgb', None),
            # The result on elm and oak is flaky in arc mode.
            # TODO(wuchengli): remove them once crbug.com/679864 is fixed.
            ('elm', True),
            ('oak', True)]

        dut_board = utils.get_current_board()
        for (blacklist_board, blacklist_arc_mode) in blacklist:
            if blacklist_board == dut_board:
                if blacklist_arc_mode is None or blacklist_arc_mode == arc_mode:
                    logging.info("Skipping test run on this board.")
                    return
                break

        if arc_mode:
            arc_mode_str = arc_common.ARC_MODE_ENABLED
        else:
            arc_mode_str = arc_common.ARC_MODE_DISABLED
        with chrome.Chrome(
                extra_browser_args=helper_logger.chrome_vmodule_flag(),
                arc_mode=arc_mode_str,
                init_network_controller=True) as cr:
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
             player.verify_video_can_play(constants.PLAYBACK_TEST_TIME_S)
