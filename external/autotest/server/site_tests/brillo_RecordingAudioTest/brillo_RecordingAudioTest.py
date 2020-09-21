# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import tempfile
import time

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.server import test
from autotest_lib.server.brillo import host_utils


# Number of channels to record.
_DEFAULT_NUM_CHANNELS = 1
# Recording sample rate (48kHz).
_DEFAULT_SAMPLE_RATE = 48000
# Recording sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2
# Default sine wave frequency.
_DEFAULT_SINE_FREQUENCY = 440
# Default recording duration.
_DEFAULT_DURATION_SECS = 10

_REC_FILENAME = 'rec_file.wav'

class brillo_RecordingAudioTest(test.test):
    """Verify that audio recording works."""
    version = 1


    def __init__(self, *args, **kwargs):
        super(brillo_RecordingAudioTest, self).__init__(*args, **kwargs)
        self.host = None


    def _get_recording_cmd(self, recording_method, duration_secs, sample_width,
                           sample_rate, num_channels, rec_file):
        """Get a recording command based on the method.

        @param recording_method: A string specifying the recording method to
                                 use.
        @param duration_secs: Duration (in secs) to record audio for.
        @param sample_width: Size of samples in bytes.
        @param sample_rate: Recording sample rate in hertz.
        @param num_channels: Number of channels to use for recording.
        @param rec_file: WAV file to store recorded audio to.

        @return: A string containing the command to record audio using the
                 specified method.

        @raises TestError: Invalid recording method.
        """
        if recording_method == 'libmedia':
            return ('brillo_audio_test --record --libmedia '
                    '--duration_secs=%d --num_channels=%d --sample_rate=%d '
                    '--sample_size=%d --filename=%s' %
                    (duration_secs, num_channels, sample_rate, sample_width,
                     rec_file) )
        elif recording_method == 'stagefright':
            return ('brillo_audio_test --record --stagefright '
                    '--duration_secs=%d --num_channels=%d --sample_rate=%d '
                    '--filename=%s' %
                    (duration_secs, num_channels, sample_rate, rec_file))
        elif recording_method == 'opensles':
            return ('slesTest_recBuffQueue -c%d -d%d -r%d -%d %s' %
                    (num_channels, duration_secs, sample_rate, sample_width,
                     rec_file))
        else:
            raise error.TestError('Test called with invalid recording method.')


    def test_recording(self, fb_query, recording_method, sample_width,
                       sample_rate, num_channels, duration_secs):
        """Performs a recording test.

        @param fb_query: A feedback query.
        @param recording_method: A string representing a recording method to
                                 use.
        @param sample_width: Size of samples in bytes.
        @param sample_rate: Recording sample rate in hertz.
        @param num_channels: Number of channels to use for recording.
        @param duration_secs: Duration to record for.

        @raise error.TestError: An error occurred while executing the test.
        @raise error.TestFail: The test failed.
        """
        dut_tmpdir = self.host.get_tmp_dir()
        dut_rec_file = os.path.join(dut_tmpdir, _REC_FILENAME)
        cmd = self._get_recording_cmd(recording_method=recording_method,
                                      duration_secs=duration_secs,
                                      sample_width=sample_width,
                                      sample_rate=sample_rate,
                                      num_channels=num_channels,
                                      rec_file=dut_rec_file)
        timeout = duration_secs + 5
        fb_query.prepare(use_file=self.use_file,
                         sample_width=sample_width,
                         sample_rate=sample_rate,
                         num_channels=num_channels,
                         frequency=_DEFAULT_SINE_FREQUENCY)
        logging.info('Recording command: %s', cmd)
        logging.info('Beginning audio recording')
        pid = host_utils.run_in_background(self.host, cmd)
        logging.info('Requesting audio input')
        fb_query.emit()
        logging.info('Waiting for recording to terminate')
        if not host_utils.wait_for_process(self.host, pid, timeout):
            raise error.TestError(
                    'Recording did not terminate within %d seconds' % timeout)
        _, local_rec_file = tempfile.mkstemp(prefix='recording-',
                                             suffix='.wav', dir=self.tmpdir)
        self.host.get_file(dut_rec_file, local_rec_file, delete_dest=True)
        logging.info('Validating recorded audio')
        fb_query.validate(captured_audio_file=local_rec_file)


    def run_once(self, host, fb_client, recording_method, use_file=False,
                 sample_widths_arr=[_DEFAULT_SAMPLE_WIDTH],
                 sample_rates_arr=[_DEFAULT_SAMPLE_RATE],
                 num_channels_arr=[_DEFAULT_NUM_CHANNELS],
                 duration_secs=_DEFAULT_DURATION_SECS):
        """Runs the test.

        @param host: A host object representing the DUT.
        @param fb_client: A feedback client implementation.
        @param recording_method: A string representing a playback method to use.
                                 Either 'opensles', 'libmedia', or
                                 'stagefright'.
        @param use_file: Use a file to test audio. Must be used with
                         playback_method 'opensles'.
        @param sample_widths_arr: Array of sample widths to test record at.
        @param sample_rates_arr: Array of sample rates to test record at.
        @param num_channels_arr: Array of number of channels to test record with.
        @param duration_secs: Duration to record for.


        @raise TestError: Something went wrong while trying to execute the test.
        @raise TestFail: The test failed.
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
                        fb_query = fb_client.new_query(
                                client.QUERY_AUDIO_RECORDING)
                        logging.info('Running test with the following params:')
                        logging.info('Sample rate: %d', sample_rate)
                        logging.info('Sample width: %d', sample_width)
                        logging.info('Number of channels: %d', num_channels)

                        try:
                            self.test_recording(fb_query=fb_query,
                                    recording_method=recording_method,
                                    sample_width=sample_width,
                                    sample_rate=sample_rate,
                                    num_channels=num_channels,
                                    duration_secs=duration_secs)
                        except error.TestFail as e:
                            logging.info('Test failed: %s.' % e)
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
