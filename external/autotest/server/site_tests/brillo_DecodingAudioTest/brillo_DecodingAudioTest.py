# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import tempfile
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.server import test
from autotest_lib.server.brillo import audio_utils


# Number of channels to generate.
_DEFAULT_NUM_CHANNELS = 2
# Sine wave sample rate (44.1kHz).
_DEFAULT_SAMPLE_RATE = 44100
# Sine wave default sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2
# Default sine wave frequency.
_DEFAULT_SINE_FREQUENCY = 440
# Default duration of the sine wave in seconds.
_DEFAULT_DURATION_SECS = 10

class brillo_DecodingAudioTest(test.test):
    """Verify that basic audio playback works."""
    version = 1


    def run_once(self, host, fb_client, file_format,
                 duration_secs=_DEFAULT_DURATION_SECS):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param fb_client: A feedback client implementation.
        @param file_format: A string represeting the format to the audio
                            encoding to use.
        @param duration_secs: Duration to play file for.
        """
        self.temp_dir = tempfile.mkdtemp(dir=fb_client.tmp_dir)
        with fb_client.initialize(self, host):
            logging.info('Testing silent playback to get silence threshold')
            fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_SILENT)
            fb_query.prepare()
            time.sleep(duration_secs)
            fb_query.validate()

            logging.info('Generate mp3 file')
            local_filename, dut_play_file = audio_utils.generate_sine_file(
                    host, _DEFAULT_NUM_CHANNELS, _DEFAULT_SAMPLE_RATE,
                    _DEFAULT_SAMPLE_WIDTH, _DEFAULT_DURATION_SECS,
                    _DEFAULT_SINE_FREQUENCY, self.temp_dir, file_format)

            fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_AUDIBLE)

            fb_query.prepare(sample_width=_DEFAULT_SAMPLE_WIDTH,
                             sample_rate=_DEFAULT_SAMPLE_RATE,
                             duration_secs=duration_secs,
                             num_channels=_DEFAULT_NUM_CHANNELS)

            playback_cmd = 'slesTest_playFdPath %s 0' % dut_play_file
            logging.info('Testing decode playback')
            host.run(playback_cmd)
            fb_query.validate(audio_file=local_filename)
