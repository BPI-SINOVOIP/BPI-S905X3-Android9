#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""Audio Analysis tool to analyze wave file and detect artifacts."""

import argparse
import collections
import json
import logging
import math
import numpy
import os
import pprint
import subprocess
import tempfile
import wave

import acts.test_utils.audio_analysis_lib.audio_analysis as audio_analysis
import acts.test_utils.audio_analysis_lib.audio_data as audio_data
import acts.test_utils.audio_analysis_lib.audio_quality_measurement as \
 audio_quality_measurement

# Holder for quality parameters used in audio_quality_measurement module.
QualityParams = collections.namedtuple('QualityParams', [
    'block_size_secs', 'frequency_error_threshold',
    'delay_amplitude_threshold', 'noise_amplitude_threshold',
    'burst_amplitude_threshold'
])

DEFAULT_QUALITY_BLOCK_SIZE_SECS = 0.0015
DEFAULT_BURST_AMPLITUDE_THRESHOLD = 1.4
DEFAULT_DELAY_AMPLITUDE_THRESHOLD = 0.6
DEFAULT_FREQUENCY_ERROR_THRESHOLD = 0.5
DEFAULT_NOISE_AMPLITUDE_THRESHOLD = 0.5


class WaveFileException(Exception):
    """Error in WaveFile."""
    pass


class WaveFormatExtensibleException(Exception):
    """Wave file is in WAVE_FORMAT_EXTENSIBLE format which is not supported."""
    pass


class WaveFile(object):
    """Class which handles wave file reading.

    Properties:
        raw_data: audio_data.AudioRawData object for data in wave file.
        rate: sampling rate.

    """

    def __init__(self, filename):
        """Inits a wave file.

        Args:
            filename: file name of the wave file.

        """
        self.raw_data = None
        self.rate = None

        self._wave_reader = None
        self._n_channels = None
        self._sample_width_bits = None
        self._n_frames = None
        self._binary = None

        try:
            self._read_wave_file(filename)
        except WaveFormatExtensibleException:
            logging.warning(
                'WAVE_FORMAT_EXTENSIBLE is not supproted. '
                'Try command "sox in.wav -t wavpcm out.wav" to convert '
                'the file to WAVE_FORMAT_PCM format.')
            self._convert_and_read_wav_file(filename)

    def _convert_and_read_wav_file(self, filename):
        """Converts the wav file and read it.

        Converts the file into WAVE_FORMAT_PCM format using sox command and
        reads its content.

        Args:
            filename: The wave file to be read.

        Raises:
            RuntimeError: sox is not installed.

        """
        # Checks if sox is installed.
        try:
            subprocess.check_output(['sox', '--version'])
        except:
            raise RuntimeError('sox command is not installed. '
                               'Try sudo apt-get install sox')

        with tempfile.NamedTemporaryFile(suffix='.wav') as converted_file:
            command = ['sox', filename, '-t', 'wavpcm', converted_file.name]
            logging.debug('Convert the file using sox: %s', command)
            subprocess.check_call(command)
            self._read_wave_file(converted_file.name)

    def _read_wave_file(self, filename):
        """Reads wave file header and samples.

        Args:
            filename: The wave file to be read.

        @raises WaveFormatExtensibleException: Wave file is in
                                               WAVE_FORMAT_EXTENSIBLE format.
        @raises WaveFileException: Wave file format is not supported.

        """
        try:
            self._wave_reader = wave.open(filename, 'r')
            self._read_wave_header()
            self._read_wave_binary()
        except wave.Error as e:
            if 'unknown format: 65534' in str(e):
                raise WaveFormatExtensibleException()
            else:
                logging.exception('Unsupported wave format')
                raise WaveFileException()
        finally:
            if self._wave_reader:
                self._wave_reader.close()

    def _read_wave_header(self):
        """Reads wave file header.

        @raises WaveFileException: wave file is compressed.

        """
        # Header is a tuple of
        # (nchannels, sampwidth, framerate, nframes, comptype, compname).
        header = self._wave_reader.getparams()
        logging.debug('Wave header: %s', header)

        self._n_channels = header[0]
        self._sample_width_bits = header[1] * 8
        self.rate = header[2]
        self._n_frames = header[3]
        comptype = header[4]
        compname = header[5]

        if comptype != 'NONE' or compname != 'not compressed':
            raise WaveFileException('Can not support compressed wav file.')

    def _read_wave_binary(self):
        """Reads in samples in wave file."""
        self._binary = self._wave_reader.readframes(self._n_frames)
        format_str = 'S%d_LE' % self._sample_width_bits
        self.raw_data = audio_data.AudioRawData(
            binary=self._binary,
            channel=self._n_channels,
            sample_format=format_str)


class QualityCheckerError(Exception):
    """Error in QualityChecker."""
    pass


class CompareFailure(QualityCheckerError):
    """Exception when frequency comparison fails."""
    pass


class QualityFailure(QualityCheckerError):
    """Exception when quality check fails."""
    pass


class QualityChecker(object):
    """Quality checker controls the flow of checking quality of raw data."""

    def __init__(self, raw_data, rate):
        """Inits a quality checker.

        Args:
            raw_data: An audio_data.AudioRawData object.
            rate: Sampling rate in samples per second. Example inputs: 44100,
            48000

        """
        self._raw_data = raw_data
        self._rate = rate
        self._spectrals = []
        self._quality_result = []

    def do_spectral_analysis(self, ignore_high_freq, check_quality,
                             quality_params):
        """Gets the spectral_analysis result.

        Args:
            ignore_high_freq: Ignore high frequencies above this threshold.
            check_quality: Check quality of each channel.
            quality_params: A QualityParams object for quality measurement.

        """
        self.has_data()
        for channel_idx in range(self._raw_data.channel):
            signal = self._raw_data.channel_data[channel_idx]
            max_abs = max(numpy.abs(signal))
            logging.debug('Channel %d max abs signal: %f', channel_idx,
                          max_abs)
            if max_abs == 0:
                logging.info('No data on channel %d, skip this channel',
                             channel_idx)
                continue

            saturate_value = audio_data.get_maximum_value_from_sample_format(
                self._raw_data.sample_format)
            normalized_signal = audio_analysis.normalize_signal(
                signal, saturate_value)
            logging.debug('saturate_value: %f', saturate_value)
            logging.debug('max signal after normalized: %f',
                          max(normalized_signal))
            spectral = audio_analysis.spectral_analysis(
                normalized_signal, self._rate)

            logging.debug('Channel %d spectral:\n%s', channel_idx,
                          pprint.pformat(spectral))

            # Ignore high frequencies above the threshold.
            spectral = [(f, c) for (f, c) in spectral if f < ignore_high_freq]

            logging.info('Channel %d spectral after ignoring high frequencies '
                         'above %f:\n%s', channel_idx, ignore_high_freq,
                         pprint.pformat(spectral))

            try:
                if check_quality:
                    quality = audio_quality_measurement.quality_measurement(
                        signal=normalized_signal,
                        rate=self._rate,
                        dominant_frequency=spectral[0][0],
                        block_size_secs=quality_params.block_size_secs,
                        frequency_error_threshold=quality_params.
                        frequency_error_threshold,
                        delay_amplitude_threshold=quality_params.
                        delay_amplitude_threshold,
                        noise_amplitude_threshold=quality_params.
                        noise_amplitude_threshold,
                        burst_amplitude_threshold=quality_params.
                        burst_amplitude_threshold)

                    logging.debug('Channel %d quality:\n%s', channel_idx,
                                  pprint.pformat(quality))
                    self._quality_result.append(quality)
                self._spectrals.append(spectral)
            except Exception as error:
                logging.warning(
                    "Failed to analyze channel {} with error: {}".format(
                        channel_idx, error))

    def has_data(self):
        """Checks if data has been set.

        Raises:
            QualityCheckerError: if data or rate is not set yet.

        """
        if not self._raw_data or not self._rate:
            raise QualityCheckerError('Data and rate is not set yet')

    def check_freqs(self, expected_freqs, freq_threshold):
        """Checks the dominant frequencies in the channels.

        Args:
            expected_freq: A list of frequencies. If frequency is 0, it
                              means this channel should be ignored.
            freq_threshold: The difference threshold to compare two
                               frequencies.

        """
        logging.debug('expected_freqs: %s', expected_freqs)
        for idx, expected_freq in enumerate(expected_freqs):
            if expected_freq == 0:
                continue
            if not self._spectrals[idx]:
                raise CompareFailure(
                    'Failed at channel %d: no dominant frequency' % idx)
            dominant_freq = self._spectrals[idx][0][0]
            if abs(dominant_freq - expected_freq) > freq_threshold:
                raise CompareFailure(
                    'Failed at channel %d: %f is too far away from %f' %
                    (idx, dominant_freq, expected_freq))

    def check_quality(self):
        """Checks the quality measurement results on each channel.

        Raises:
            QualityFailure when there is artifact.

        """
        error_msgs = []

        for idx, quality_res in enumerate(self._quality_result):
            artifacts = quality_res['artifacts']
            if artifacts['noise_before_playback']:
                error_msgs.append('Found noise before playback: %s' %
                                  (artifacts['noise_before_playback']))
            if artifacts['noise_after_playback']:
                error_msgs.append('Found noise after playback: %s' %
                                  (artifacts['noise_after_playback']))
            if artifacts['delay_during_playback']:
                error_msgs.append('Found delay during playback: %s' %
                                  (artifacts['delay_during_playback']))
            if artifacts['burst_during_playback']:
                error_msgs.append('Found burst during playback: %s' %
                                  (artifacts['burst_during_playback']))
        if error_msgs:
            raise QualityFailure('Found bad quality: %s',
                                 '\n'.join(error_msgs))

    def dump(self, output_file):
        """Dumps the result into a file in json format.

        Args:
            output_file: A file path to dump spectral and quality
                            measurement result of each channel.

        """
        dump_dict = {
            'spectrals': self._spectrals,
            'quality_result': self._quality_result
        }
        with open(output_file, 'w') as f:
            json.dump(dump_dict, f)

    def has_data(self):
        """Checks if data has been set.

        Raises:
            QualityCheckerError: if data or rate is not set yet.

        """
        if not self._raw_data or not self._rate:
            raise QualityCheckerError('Data and rate is not set yet')

    def check_freqs(self, expected_freqs, freq_threshold):
        """Checks the dominant frequencies in the channels.

        Args:
            expected_freq: A list of frequencies. If frequency is 0, it
                              means this channel should be ignored.
            freq_threshold: The difference threshold to compare two
                               frequencies.

        """
        logging.debug('expected_freqs: %s', expected_freqs)
        for idx, expected_freq in enumerate(expected_freqs):
            if expected_freq == 0:
                continue
            if not self._spectrals[idx]:
                raise CompareFailure(
                    'Failed at channel %d: no dominant frequency' % idx)
            dominant_freq = self._spectrals[idx][0][0]
            if abs(dominant_freq - expected_freq) > freq_threshold:
                raise CompareFailure(
                    'Failed at channel %d: %f is too far away from %f' %
                    (idx, dominant_freq, expected_freq))

    def check_quality(self):
        """Checks the quality measurement results on each channel.

        Raises:
            QualityFailure when there is artifact.

        """
        error_msgs = []

        for idx, quality_res in enumerate(self._quality_result):
            artifacts = quality_res['artifacts']
            if artifacts['noise_before_playback']:
                error_msgs.append('Found noise before playback: %s' %
                                  (artifacts['noise_before_playback']))
            if artifacts['noise_after_playback']:
                error_msgs.append('Found noise after playback: %s' %
                                  (artifacts['noise_after_playback']))
            if artifacts['delay_during_playback']:
                error_msgs.append('Found delay during playback: %s' %
                                  (artifacts['delay_during_playback']))
            if artifacts['burst_during_playback']:
                error_msgs.append('Found burst during playback: %s' %
                                  (artifacts['burst_during_playback']))
        if error_msgs:
            raise QualityFailure('Found bad quality: %s',
                                 '\n'.join(error_msgs))

    def dump(self, output_file):
        """Dumps the result into a file in json format.

        Args:
            output_file: A file path to dump spectral and quality
                            measurement result of each channel.

        """
        dump_dict = {
            'spectrals': self._spectrals,
            'quality_result': self._quality_result
        }
        with open(output_file, 'w') as f:
            json.dump(dump_dict, f)


class CheckQualityError(Exception):
    """Error in check_quality main function."""
    pass


def read_audio_file(filename, channel, bit_width, rate):
    """Reads audio file.

    Args:
        filename: The wav or raw file to check.
        channel: For raw file. Number of channels.
        bit_width: For raw file. Bit width of a sample.
        rate: Sampling rate in samples per second. Example inputs: 44100,
        48000


    Returns:
        A tuple (raw_data, rate) where raw_data is audio_data.AudioRawData, rate
            is sampling rate.

    """
    if filename.endswith('.wav'):
        wavefile = WaveFile(filename)
        raw_data = wavefile.raw_data
        rate = wavefile.rate
    elif filename.endswith('.raw'):
        binary = None
        with open(filename, 'rb') as f:
            binary = f.read()
        raw_data = audio_data.AudioRawData(
            binary=binary, channel=channel, sample_format='S%d_LE' % bit_width)
    else:
        raise CheckQualityError(
            'File format for %s is not supported' % filename)

    return raw_data, rate


def get_quality_params(
        quality_block_size_secs, quality_frequency_error_threshold,
        quality_delay_amplitude_threshold, quality_noise_amplitude_threshold,
        quality_burst_amplitude_threshold):
    """Gets quality parameters in arguments.

    Args:
        quality_block_size_secs: Input block size in seconds.
        quality_frequency_error_threshold: Input the frequency error
        threshold.
        quality_delay_amplitude_threshold: Input the delay aplitutde
        threshold.
        quality_noise_amplitude_threshold: Input the noise aplitutde
        threshold.
        quality_burst_amplitude_threshold: Input the burst aplitutde
        threshold.

    Returns:
        A QualityParams object.

    """
    quality_params = QualityParams(
        block_size_secs=quality_block_size_secs,
        frequency_error_threshold=quality_frequency_error_threshold,
        delay_amplitude_threshold=quality_delay_amplitude_threshold,
        noise_amplitude_threshold=quality_noise_amplitude_threshold,
        burst_amplitude_threshold=quality_burst_amplitude_threshold)

    return quality_params


def quality_analysis(
        filename,
        output_file,
        bit_width,
        rate,
        channel,
        freqs=None,
        freq_threshold=5,
        ignore_high_freq=5000,
        spectral_only=False,
        quality_block_size_secs=DEFAULT_QUALITY_BLOCK_SIZE_SECS,
        quality_burst_amplitude_threshold=DEFAULT_BURST_AMPLITUDE_THRESHOLD,
        quality_delay_amplitude_threshold=DEFAULT_DELAY_AMPLITUDE_THRESHOLD,
        quality_frequency_error_threshold=DEFAULT_FREQUENCY_ERROR_THRESHOLD,
        quality_noise_amplitude_threshold=DEFAULT_NOISE_AMPLITUDE_THRESHOLD,
):
    """ Runs various functions to measure audio quality base on user input.

    Args:
        filename: The wav or raw file to check.
        output_file: Output file to dump analysis result in JSON format.
        bit_width: For raw file. Bit width of a sample.
        rate: Sampling rate in samples per second. Example inputs: 44100,
        48000
        channel: For raw file. Number of channels.
        freqs: Expected frequencies in the channels.
        freq_threshold: Frequency difference threshold in Hz.
        ignore_high_freq: Frequency threshold in Hz to be ignored for high
        frequency. Default is 5KHz
        spectral_only: Only do spectral analysis on each channel.
        quality_block_size_secs: Input block size in seconds.
        quality_frequency_error_threshold: Input the frequency error
        threshold.
        quality_delay_amplitude_threshold: Input the delay aplitutde
        threshold.
        quality_noise_amplitude_threshold: Input the noise aplitutde
        threshold.
        quality_burst_amplitude_threshold: Input the burst aplitutde
        threshold.
    """
    format = '%(asctime)-15s:%(levelname)s:%(pathname)s:%(lineno)d: %(message)s'
    logging.basicConfig(format=format, level=logging.INFO)
    raw_data, rate = read_audio_file(filename, channel, bit_width, rate)

    checker = QualityChecker(raw_data, rate)

    quality_params = get_quality_params(
        quality_block_size_secs, quality_frequency_error_threshold,
        quality_delay_amplitude_threshold, quality_noise_amplitude_threshold,
        quality_burst_amplitude_threshold)

    checker.do_spectral_analysis(
        ignore_high_freq=ignore_high_freq,
        check_quality=(not spectral_only),
        quality_params=quality_params)

    checker.dump(output_file)

    if freqs:
        checker.check_freqs(freqs, freq_threshold)

    if not spectral_only:
        checker.check_quality()
