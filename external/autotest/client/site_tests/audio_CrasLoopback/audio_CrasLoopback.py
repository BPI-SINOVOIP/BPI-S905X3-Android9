# Copyright (c) 2011 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import os

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import audio_test_data
from autotest_lib.client.cros.audio import cmd_utils
from autotest_lib.client.cros.audio import cras_utils


TEST_DURATION = 1

class audio_CrasLoopback(audio_helper.cras_rms_test):
    """Verifies audio playback and capture function."""
    version = 1

    @staticmethod
    def wait_for_active_stream_count(expected_count):
        utils.poll_for_condition(
            lambda: cras_utils.get_active_stream_count() == expected_count,
            exception=error.TestError(
                'Timeout waiting active stream count to become %d' %
                 expected_count))


    def run_once(self):
        """Entry point of this test."""

        # Generate sine raw file that lasts 5 seconds.
        raw_path = os.path.join(self.bindir, '5SEC.raw')
        data_format = dict(file_type='raw', sample_format='S16_LE',
                channel=2, rate=48000)
        raw_file = audio_test_data.GenerateAudioTestData(
            path=raw_path,
            data_format=data_format,
            duration_secs=5,
            frequencies=[440, 440],
            volume_scale=0.9)

        recorded_file = os.path.join(self.resultsdir, 'cras_recorded.raw')

        self.wait_for_active_stream_count(0)
        p = cmd_utils.popen(cras_utils.playback_cmd(raw_file.path))
        try:
            self.wait_for_active_stream_count(1)
            cras_utils.capture(recorded_file, duration=TEST_DURATION)

            # Make sure the audio is still playing.
            if p.poll() != None:
                raise error.TestError('playback stopped')
        finally:
            cmd_utils.kill_or_log_returncode(p)
            raw_file.delete()

        rms_value = audio_helper.get_rms(recorded_file)[0]
        self.write_perf_keyval({'rms_value': rms_value})

