# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib.cros import chrome


class desktopui_MusLogin(test.test):
    """Verifies chrome --mus starts up and logs in correctly."""
    version = 1


    def run_once(self):
        """Entry point of this test."""

        # Flaky on nyan_* boards. http://crbug.com/717275
        boards_to_skip = ['nyan_big', 'nyan_kitty', 'nyan_blaze']
        if utils.get_current_board() in boards_to_skip:
          logging.warning('Skipping test run on this board.')
          return

        # GPU info collection via devtools SystemInfo.getInfo does not work
        # under mus due to differences in how the GPU process is configured.
        # http://crbug.com/669965
        mus_browser_args = ['--mus', '--gpu-no-complete-info-collection']

        logging.info('Testing Chrome --mus startup.')
        with chrome.Chrome(auto_login=False,
                           extra_browser_args=mus_browser_args):
            logging.info('Chrome --mus started and loaded OOBE.')

        logging.info('Testing Chrome --mus login.')
        with chrome.Chrome(extra_browser_args=mus_browser_args):
            logging.info('Chrome login with --mus succeeded.')
