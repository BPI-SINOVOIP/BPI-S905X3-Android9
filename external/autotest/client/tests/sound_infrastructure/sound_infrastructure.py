# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import os
import re
import stat
import subprocess

from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import test
from autotest_lib.client.bin import utils

_SND_DEV_DIR = '/dev/snd/'
_SND_DEFAULT_MASK = 'controlC*'

class sound_infrastructure(test.test):
    """
    Tests that the expected sound infrastructure is present.

    Check that at least one playback and capture device exists and that their
    permissions are configured properly.

    """
    version = 2
    _NO_PLAYBACK_BOARDS_LIST = []
    _NO_RECORDER_BOARDS_LIST = ['veyron_mickey']
    _NO_AUDIO_DEVICE_LIST = ['veyron_rialto']

    def check_snd_dev_perms(self, filename):
        desired_mode = (stat.S_IRUSR | stat.S_IWUSR | stat.S_IRGRP |
                        stat.S_IWGRP | stat.S_IFCHR)
        st = os.stat(filename)
        if (st.st_mode != desired_mode):
            raise error.TestFail("Incorrect permissions for %s" % filename)

    def check_sound_files(self, playback=True, record=True):
        """Checks sound files present in snd directory.

        @param playback: Checks playback device.
        @param record: Checks record device.

        @raises: error.TestFail if sound file is missing.

        """
        patterns = {'^controlC(\d+)': False}
        if playback:
            patterns['^pcmC(\d+)D(\d+)p$'] = False
        if record:
            patterns['^pcmC(\d+)D(\d+)c$'] = False

        filenames = os.listdir(_SND_DEV_DIR)

        for filename in filenames:
            for pattern in patterns:
                if re.match(pattern, filename):
                    patterns[pattern] = True
                    self.check_snd_dev_perms(_SND_DEV_DIR + filename)

        for pattern in patterns:
            if not patterns[pattern]:
                raise error.TestFail("Missing device %s" % pattern)

    def check_device_list(self, playback=True, record=True):
        """Checks sound card and device list by alsa utils command.

        @param playback: Checks playback sound card and devices.
        @param record: Checks record sound card and devices.

        @raises: error.TestFail if no playback/record devices found.

        """
        no_cards_pattern = '.*no soundcards found.*'
        if playback:
            aplay = subprocess.Popen(["aplay", "-l"], stderr=subprocess.PIPE)
            aplay_list = aplay.communicate()[1]
            if aplay.returncode or re.match(no_cards_pattern, aplay_list):
                raise error.TestFail("No playback devices found by aplay")

        if record:
            no_cards_pattern = '.*no soundcards found.*'
            arecord = subprocess.Popen(
                    ["arecord", "-l"], stderr=subprocess.PIPE)
            arecord_list = arecord.communicate()[1]
            if arecord.returncode or re.match(no_cards_pattern, arecord_list):
                raise error.TestFail("No record devices found by arecord")

    def run_once(self):
        board = utils.get_board().lower()
        snd_control = len(glob.glob(_SND_DEV_DIR + _SND_DEFAULT_MASK))
        if board in self._NO_AUDIO_DEVICE_LIST:
            if snd_control:
                raise error.TestError('%s is not supposed to have sound control!' % board)
            else:
                return
        record = board not in self._NO_RECORDER_BOARDS_LIST
        playback = board not in self._NO_PLAYBACK_BOARDS_LIST
        self.check_sound_files(playback, record)
        self.check_device_list(playback, record)
