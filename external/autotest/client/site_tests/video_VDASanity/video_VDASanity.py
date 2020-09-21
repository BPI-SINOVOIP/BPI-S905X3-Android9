# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.cros import chrome_binary_test
from autotest_lib.client.cros.video import helper_logger


DOWNLOAD_BASE = ('http://commondatastorage.googleapis.com'
                 '/chromiumos-test-assets-public/')
BINARY = 'video_decode_accelerator_unittest'

class video_VDASanity(chrome_binary_test.ChromeBinaryTest):
    """
    VDA sanity autotest runs binary video_decode_accelerator_unittest on a list
    of videos.
    """
    version = 1

    @helper_logger.video_log_wrapper
    @chrome_binary_test.nuke_chrome
    def run_once(self, test_cases):
        for (path, width, height, frame_num, frag_num, profile) in test_cases:
            video_path = os.path.join(self.bindir, 'video.download')
            test_video_data = '%s:%d:%d:%d:%d:%d:%d:%d' % (
                video_path, width, height, frame_num, frag_num, 0, 0, profile)
            try:
                self._download_video(path, video_path)
                self._run_test_case(test_video_data)
            finally:
                self._remove_if_exists(video_path)

    def _download_video(self, download_path, local_file):
        url = DOWNLOAD_BASE + download_path
        logging.info('Downloading "%s" to "%s"', url, local_file)
        file_utils.download_file(url, local_file)

    def _run_test_case(self, test_video_data):
        cmd_line_list = [
            '--test_video_data="%s"' % test_video_data,
            '--gtest_filter=VideoDecodeAcceleratorTest.NoCrash',
            helper_logger.chrome_vmodule_flag(),
            '--ozone-platform=gbm',
        ]
        cmd_line = ' '.join(cmd_line_list)
        self.run_chrome_test_binary(BINARY, cmd_line)

    def _remove_if_exists(self, filepath):
        try:
            os.remove(filepath)
        except OSError:
            pass
