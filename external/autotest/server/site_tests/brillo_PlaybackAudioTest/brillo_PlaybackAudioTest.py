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


# The amount of time to wait when producing silence (i.e. no playback).
_SILENCE_DURATION_SECS = 5

# Number of channels to generate.
_DEFAULT_NUM_CHANNELS = 1
# Sine wave sample rate (48kHz).
_DEFAULT_SAMPLE_RATE = 48000
# Sine wave default sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2
# Default sine wave frequency.
_DEFAULT_SINE_FREQUENCY = 440
# Default duration of the sine wave in seconds.
_DEFAULT_DURATION_SECS = 10

class brillo_PlaybackAudioTest(test.test):
    """Verify that basic audio playback works."""
    version = 1

    def __init__(self, *args, **kwargs):
        super(brillo_PlaybackAudioTest, self).__init__(*args, **kwargs)
        self.host = None


    def _get_playback_cmd(self, method, dut_play_file):
        """Get the playback command to execute based on the playback method.

        @param method: A string specifiying which method to use.
        @param dut_play_file: A string containing the path to the file to play
                              on the DUT.
        @return: A string containing the command to play audio using the
                 specified method.

        @raises TestError: Invalid playback method.
        """
        if dut_play_file:
            if method == 'libmedia':
                return ('brillo_audio_test --playback --libmedia '
                        '--filename=%s' % dut_play_file)
            elif method == 'stagefright':
                return ('brillo_audio_test --playback --stagefright '
                        '--filename=%s' % dut_play_file)
            elif method == 'opensles':
                return 'slesTest_playFdPath %s 0' % dut_play_file
        else:
            if method == 'libmedia':
                return 'brillo_audio_test --playback --libmedia --sine'
            elif method == 'stagefright':
                return 'brillo_audio_test --playback --stagefright --sine'
            elif method == 'opensles':
                return 'slesTest_sawtoothBufferQueue'
        raise error.TestError('Test called with invalid playback method.')


    def test_playback(self, fb_query, playback_cmd, sample_width, sample_rate,
                      num_channels, play_file_path=None):
        """Performs a playback test.

        @param fb_query: A feedback query.
        @param playback_cmd: The playback generating command, or None for no-op.
        @param play_file_path: A string of the path to the file being played.
        @param sample_width: Sample width to test playback at.
        @param sample_rate: Sample rate to test playback at.
        @param num_channels: Number of channels to test playback with.
        """
        fb_query.prepare(sample_width=sample_width,
                         sample_rate=sample_rate,
                         duration_secs=self.duration_secs,
                         num_channels=num_channels)
        if playback_cmd:
            self.host.run(playback_cmd)
        else:
            time.sleep(_SILENCE_DURATION_SECS)
        if play_file_path:
            fb_query.validate(audio_file=play_file_path)
        else:
            fb_query.validate()


    def test_audio(self, fb_client, playback_method, sample_rate, sample_width,
                   num_channels):
        """Test audio playback with the given parameters.

        @param fb_client: A feedback client implementation.
        @param playback_method: A string representing a playback method to use.
                                Either 'opensles', 'libmedia', or 'stagefright'.
        @param sample_rate: Sample rate to test playback at.
        @param sample_width: Sample width to test playback at.
        @param num_channels: Number of channels to test playback with.
        """
        logging.info('Testing silent playback')
        fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_SILENT)
        self.test_playback(fb_query=fb_query,
                           playback_cmd=None,
                           sample_rate=sample_rate,
                           sample_width=sample_width,
                           num_channels=num_channels)

        dut_play_file = None
        host_filename = None
        if self.use_file:
           host_filename, dut_play_file = audio_utils.generate_sine_file(
                   self.host, num_channels, sample_rate, sample_width,
                   self.duration_secs, _DEFAULT_SINE_FREQUENCY, self.temp_dir)

        logging.info('Testing audible playback')
        fb_query = fb_client.new_query(client.QUERY_AUDIO_PLAYBACK_AUDIBLE)
        playback_cmd = self._get_playback_cmd(playback_method, dut_play_file)

        self.test_playback(fb_query=fb_query,
                           playback_cmd=playback_cmd,
                           sample_rate=sample_rate,
                           sample_width=sample_width,
                           num_channels=num_channels,
                           play_file_path=host_filename)


    def run_once(self, host, fb_client, playback_method, use_file=False,
                 sample_widths_arr=[_DEFAULT_SAMPLE_WIDTH],
                 sample_rates_arr=[_DEFAULT_SAMPLE_RATE],
                 num_channels_arr=[_DEFAULT_NUM_CHANNELS],
                 duration_secs=_DEFAULT_DURATION_SECS):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param fb_client: A feedback client implementation.
        @param playback_method: A string representing a playback method to use.
                                Either 'opensles', 'libmedia', or 'stagefright'.
        @param use_file: Use a file to test audio. Must be used with
                         playback_method 'opensles'.
        @param sample_widths_arr: Array of sample widths to test playback at.
        @param sample_rates_arr: Array of sample rates to test playback at.
        @param num_channels_arr: Array of number of channels to test playback
                                 with.
        @param duration_secs: Duration to play file for.
        """
        self.host = host
        self.duration_secs = duration_secs
        self.use_file = use_file
        self.temp_dir = tempfile.mkdtemp(dir=fb_client.tmp_dir)
        failed_params = []
        with fb_client.initialize(self, host):
            for sample_rate in sample_rates_arr:
                for sample_width in sample_widths_arr:
                    for num_channels in num_channels_arr:
                        logging.info('Running test with the following params:')
                        logging.info('Sample rate: %d', sample_rate)
                        logging.info('Sample width: %d', sample_width)
                        logging.info('Number of channels: %d', num_channels)

                        try:
                            self.test_audio(fb_client=fb_client,
                                            playback_method=playback_method,
                                            sample_rate=sample_rate,
                                            sample_width=sample_width,
                                            num_channels=num_channels)
                        except error.TestFail:
                            logging.info('Test failed.')
                            failed_params.append((sample_rate, sample_width,
                                                  num_channels))
                        finally:
                            # Sleep to avoid conflict between different tests.
                            time.sleep(duration_secs)

        if failed_params == []:
            logging.info('All tests successfully passed.')
        else:
            logging.error('The following combinations failed:')
            for param in failed_params:
                logging.error('Sample rate: %i, Sample width: %i, Num Channels '
                              '%i', param[0], param[1], param[2])
            raise error.TestFail('Some of the tests failed to pass.')
