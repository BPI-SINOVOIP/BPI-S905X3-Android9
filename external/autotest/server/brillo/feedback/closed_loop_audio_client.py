# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Feedback implementation for audio with closed-loop cable."""

import logging
import os
import tempfile

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.feedback import client
from autotest_lib.server.brillo import audio_utils
from autotest_lib.server.brillo import host_utils


# Constants used when recording playback.
#
_REC_FILENAME = 'rec_file.wav'
_REC_DURATION = 10

# Number of channels to record.
_DEFAULT_NUM_CHANNELS = 1
# Recording sample rate (48kHz).
_DEFAULT_SAMPLE_RATE = 48000
# Recording sample format is signed 16-bit PCM (two bytes).
_DEFAULT_SAMPLE_WIDTH = 2
# Default frequency to generate audio at (used for recording).
_DEFAULT_FREQUENCY = 440

# The peak when recording silence is 5% of the max volume.
_SILENCE_THRESHOLD = 0.05


def _max_volume(sample_width):
    """Returns the maximum possible volume.

    This is the highest absolute value of an integer of a given width.
    If the sample width is one, then we assume an unsigned intger. For all other
    sample sizes, we assume that the format is signed.

    @param sample_width: The sample width in bytes.
    """
    return (1 << 8) if sample_width == 1 else (1 << (sample_width * 8 - 1))


class Client(client.Client):
    """Audio closed-loop feedback implementation.

    This class (and the queries it instantiates) perform playback and recording
    of audio on the DUT itself, with the assumption that the audio in/out
    connections are cross-wired with a cable. It provides some shared logic
    that queries can use for handling the DUT as well as maintaining shared
    state between queries (such as an audible volume threshold).
    """

    def __init__(self):
        """Construct the client library."""
        super(Client, self).__init__()
        self.host = None
        self.dut_tmp_dir = None
        self.tmp_dir = None


    def set_audible_threshold(self, threshold):
        """Sets the audible volume threshold.

        @param threshold: New threshold value.
        """
        self.audible_threshold = threshold


    # Interface overrides.
    #
    def _initialize_impl(self, test, host):
        """Initializes the feedback object.

        @param test: An object representing the test case.
        @param host: An object representing the DUT.
        """
        self.host = host
        self.tmp_dir = test.tmpdir
        self.dut_tmp_dir = host.get_tmp_dir()


    def _finalize_impl(self):
        """Finalizes the feedback object."""
        pass


    def _new_query_impl(self, query_id):
        """Instantiates a new query.

        @param query_id: A query identifier.

        @return A query object.

        @raise error.TestError: Query is not supported.
        """
        if query_id == client.QUERY_AUDIO_PLAYBACK_SILENT:
            return SilentPlaybackAudioQuery(self)
        elif query_id == client.QUERY_AUDIO_PLAYBACK_AUDIBLE:
            return AudiblePlaybackAudioQuery(self)
        elif query_id == client.QUERY_AUDIO_RECORDING:
            return RecordingAudioQuery(self)
        else:
            raise error.TestError('Unsupported query (%s)' % query_id)


class _PlaybackAudioQuery(client.OutputQuery):
    """Playback query base class."""

    def __init__(self, client):
        """Constructor.

        @param client: The instantiating client object.
        """
        super(_PlaybackAudioQuery, self).__init__()
        self.client = client
        self.dut_rec_filename = None
        self.local_tmp_dir = None
        self.recording_pid = None


    def _get_local_rec_filename(self):
        """Waits for recording to finish and copies the file to the host.

        @return A string of the local filename containing the recorded audio.

        @raise error.TestError: Error while validating the recording.
        """
        # Wait for recording to finish.
        timeout = _REC_DURATION + 5
        if not host_utils.wait_for_process(self.client.host,
                                           self.recording_pid, timeout):
            raise error.TestError(
                    'Recording did not terminate within %d seconds' % timeout)

        _, local_rec_filename = tempfile.mkstemp(
                prefix='recording-', suffix='.wav', dir=self.local_tmp_dir)
        self.client.host.get_file(self.dut_rec_filename,
                                  local_rec_filename, delete_dest=True)
        return local_rec_filename


    # Implementation overrides.
    #
    def _prepare_impl(self,
                      sample_width=_DEFAULT_SAMPLE_WIDTH,
                      sample_rate=_DEFAULT_SAMPLE_RATE,
                      num_channels=_DEFAULT_NUM_CHANNELS,
                      duration_secs=_REC_DURATION):
        """Implementation of query preparation logic.

        @sample_width: Sample width to record at.
        @sample_rate: Sample rate to record at.
        @num_channels: Number of channels to record at.
        @duration_secs: Duration (in seconds) to record for.
        """
        self.num_channels = num_channels
        self.sample_rate = sample_rate
        self.sample_width = sample_width
        self.dut_rec_filename = os.path.join(self.client.dut_tmp_dir,
                                             _REC_FILENAME)
        self.local_tmp_dir = tempfile.mkdtemp(dir=self.client.tmp_dir)

        # Trigger recording in the background.
        cmd = ('slesTest_recBuffQueue -c%d -d%d -r%d -%d %s' %
               (num_channels, duration_secs, sample_rate, sample_width,
                self.dut_rec_filename))
        logging.info("Recording cmd: %s", cmd)
        self.recording_pid = host_utils.run_in_background(self.client.host, cmd)


class SilentPlaybackAudioQuery(_PlaybackAudioQuery):
    """Implementation of a silent playback query."""

    def __init__(self, client):
        super(SilentPlaybackAudioQuery, self).__init__(client)


    # Implementation overrides.
    #
    def _validate_impl(self):
        """Implementation of query validation logic."""
        local_rec_filename = self._get_local_rec_filename()
        try:
              silence_peaks = audio_utils.check_wav_file(
                      local_rec_filename,
                      num_channels=self.num_channels,
                      sample_rate=self.sample_rate,
                      sample_width=self.sample_width)
        except ValueError as e:
            raise error.TestFail('Invalid file attributes: %s' % e)

        silence_peak = max(silence_peaks)
        # Fail if the silence peak volume exceeds the maximum allowed.
        max_vol = _max_volume(self.sample_width) * _SILENCE_THRESHOLD
        if silence_peak > max_vol:
            logging.error('Silence peak level (%d) exceeds the max allowed '
                          '(%d)', silence_peak, max_vol)
            raise error.TestFail('Environment is too noisy')

        # Update the client audible threshold, if so instructed.
        audible_threshold = silence_peak * 15
        logging.info('Silent peak level (%d) is below the max allowed (%d); '
                     'setting audible threshold to %d',
                     silence_peak, max_vol, audible_threshold)
        self.client.set_audible_threshold(audible_threshold)


class AudiblePlaybackAudioQuery(_PlaybackAudioQuery):
    """Implementation of an audible playback query."""

    def __init__(self, client):
        super(AudiblePlaybackAudioQuery, self).__init__(client)


    def _check_peaks(self):
        """Ensure that peak recording volume exceeds the threshold."""
        local_rec_filename = self._get_local_rec_filename()
        try:
              audible_peaks = audio_utils.check_wav_file(
                      local_rec_filename,
                      num_channels=self.num_channels,
                      sample_rate=self.sample_rate,
                      sample_width=self.sample_width)
        except ValueError as e:
            raise error.TestFail('Invalid file attributes: %s' % e)

        min_channel, min_audible_peak = min(enumerate(audible_peaks),
                                            key=lambda p: p[1])
        if min_audible_peak < self.client.audible_threshold:
            logging.error(
                    'Audible peak level (%d) is less than expected (%d) for '
                    'channel %d', min_audible_peak,
                    self.client.audible_threshold, min_channel)
            raise error.TestFail(
                    'The played audio peak level is below the expected '
                    'threshold. Either playback did not work, or the volume '
                    'level is too low. Check the audio connections and '
                    'settings on the DUT.')

        logging.info('Audible peak level (%d) exceeds the threshold (%d)',
                     min_audible_peak, self.client.audible_threshold)


    # Implementation overrides.
    #
    def _validate_impl(self, audio_file=None):
        """Implementation of query validation logic.

        @audio_file: File to compare recorded audio to.
        """
        self._check_peaks()
        # If the reference audio file is available, then perform an additional
        # check.
        if audio_file:
            local_rec_filename = self._get_local_rec_filename()
            audio_utils.compare_file(reference_audio_filename=audio_file,
                                     test_audio_filename=local_rec_filename)


class RecordingAudioQuery(client.InputQuery):
    """Implementation of a recording query."""

    def __init__(self, client):
        super(RecordingAudioQuery, self).__init__()
        self.client = client


    def _prepare_impl(self, use_file=False,
                      sample_width=_DEFAULT_SAMPLE_WIDTH,
                      sample_rate=_DEFAULT_SAMPLE_RATE,
                      num_channels=_DEFAULT_NUM_CHANNELS,
                      duration_secs=_REC_DURATION,
                      frequency=_DEFAULT_FREQUENCY):
        """Implementation of query preparation logic.

        @param use_file: A bool to indicate whether a file should be used for
                         playback. The other arguments are only valid if
                         use_file is True.
        @param sample_width: Size of samples in bytes.
        @param sample_rate: Recording sample rate in hertz.
        @param num_channels: Number of channels to use for playback.
        @param duration_secs: Number of seconds to play audio for.
        @param frequency: Frequency of sine wave to generate.
        """
        self.use_file = use_file
        self.sample_rate = sample_rate
        self.sample_width = sample_width
        self.num_channels = num_channels
        self.duration_secs = duration_secs
        self.frequency = frequency


    def _emit_impl(self):
        """Implementation of query emission logic."""
        if self.use_file:
            self.reference_filename, dut_play_file = \
                    audio_utils.generate_sine_file(
                            self.client.host, self.num_channels,
                            self.sample_rate, self.sample_width,
                            self.duration_secs, self.frequency,
                            self.client.tmp_dir)
            playback_cmd = 'slesTest_playFdPath %s 0' % dut_play_file
            self.client.host.run(playback_cmd)
        else:
            self.client.host.run('slesTest_sawtoothBufferQueue')


    def _validate_impl(self, captured_audio_file,
                       peak_percent_min=1, peak_percent_max=100):
        """Implementation of query validation logic.

        @param captured_audio_file: Path to the recorded WAV file.
        @peak_percent_min: Lower bound on peak recorded volume as percentage of
            max molume (0-100). Default is 1%.
        @peak_percent_max: Upper bound on peak recorded volume as percentage of
            max molume (0-100). Default is 100% (no limit).
        """
        try:
            recorded_peaks = audio_utils.check_wav_file(
                    captured_audio_file, num_channels=self.num_channels,
                    sample_rate=self.sample_rate,
                    sample_width=self.sample_width)
        except ValueError as e:
            raise error.TestFail('Recorded audio file is invalid: %s' % e)

        max_volume = _max_volume(self.sample_width)
        peak_min = max_volume * peak_percent_min / 100
        peak_max = max_volume * peak_percent_max / 100
        for channel, recorded_peak in enumerate(recorded_peaks):
            if recorded_peak < peak_min:
                logging.error(
                        'Recorded audio peak level (%d) is less than expected '
                        '(%d) for channel %d', recorded_peak, peak_min, channel)
                raise error.TestFail(
                        'The recorded audio peak level is below the expected '
                        'threshold. Either recording did not capture the '
                        'produced audio, or the recording level is too low. '
                        'Check the audio connections and settings on the DUT.')

            if recorded_peak > peak_max:
                logging.error(
                        'Recorded audio peak level (%d) is more than expected '
                        '(%d) for channel %d', recorded_peak, peak_max, channel)
                raise error.TestFail(
                        'The recorded audio peak level exceeds the expected '
                        'maximum. Either recording captured much background '
                        'noise, or the recording level is too high. Check the '
                        'audio connections and settings on the DUT.')
        if self.use_file:
            audio_utils.compare_file(
                reference_audio_filename=self.reference_filename,
                test_audio_filename=captured_audio_file)
