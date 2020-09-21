# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time
import os.path

from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import chrome
from autotest_lib.client.cros.input_playback import input_playback


class platform_InputScreenshot(test.test):
    """Tests if key combinations will create a screenshot."""
    version = 1
    _WAIT = 5
    _TMP = '/tmp'
    _DOWNLOADS = '/home/chronos/user/Downloads'
    _SCREENSHOT = 'Screenshot*'
    _ERROR = list()
    _MIN_SIZE = 1000

    def warmup(self):
        """Test setup."""
        # Emulate keyboard.
        # See input_playback. The keyboard is used to play back shortcuts.
        # Remove all screenshots.
        self.player = input_playback.InputPlayback()
        self.player.emulate(input_type='keyboard')
        self.player.find_connected_inputs()
        self.remove_screenshot()


    def remove_screenshot(self):
        """Remove all screenshots."""
        utils.system_output('rm -f %s/%s %s/%s' %(self._TMP, self._SCREENSHOT,
                            self._DOWNLOADS, self._SCREENSHOT))


    def confirm_file_exist(self, filepath):
        """Check if screenshot file can be found and with minimum size.

        @param filepath file path.

        @raises: error.TestFail if screenshot file does not exist.

        """
        if not os.path.isdir(filepath):
            raise error.TestNAError("%s folder is not found" % filepath)

        if not (utils.system_output('sync; sleep 2; find %s -name "%s"'
                                    % (filepath, self._SCREENSHOT))):
            self._ERROR.append('Screenshot was not found under:%s' % filepath)

        filesize = utils.system_output('ls -l %s/%s | cut -d" " -f5'
                                       % (filepath, self._SCREENSHOT))
        if filesize < self._MIN_SIZE:
            self._ERROR.append('Screenshot size:%s at %s is wrong'
                               % (filesize, filepath))


    def create_screenshot(self):
        """Create a screenshot."""
        self.player.blocking_playback_of_default_file(
               input_type='keyboard', filename='keyboard_ctrl+f5')
        time.sleep(self._WAIT)


    def run_once(self):
        # Screenshot under /tmp without login.
        self.create_screenshot()
        self.confirm_file_exist(self._TMP)

        # Screenshot under /Downloads after login.
        with chrome.Chrome() as cr:
            self.create_screenshot()
            self.confirm_file_exist(self._DOWNLOADS)

        if self._ERROR:
            raise error.TestFail('; '.join(self._ERROR))

    def cleanup(self):
        """Test cleanup."""
        self.player.close()
        self.remove_screenshot()
