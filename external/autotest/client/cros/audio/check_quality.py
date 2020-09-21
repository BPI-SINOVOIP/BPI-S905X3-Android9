#!/usr/bin/python

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Command line tool to analyze wave file and detect artifacts."""

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

# Normal autotest environment.
try:
    import common
    from autotest_lib.client.cros.audio import audio_analysis
    from autotest_lib.client.cros.audio import audio_data
    from autotest_lib.client.cros.audio import audio_quality_measurement
# Standalone execution without autotest environment.
except ImportError:
    import audio_analysis
    import audio_data
    import audio_quality_measurement


# Holder for quality parameters used in audio_quality_measurement module.
QualityParams = collections.namedtuple('QualityParams',
      ['block_size_secs',
       'frequency_error_threshold',
       'delay_amplitude_threshold',
       'noise_amplitude_threshold',
       'burst_amplitude_threshold'])


def add_args(parser):
    """Adds command line arguments."""
    parser.add_argument('filename', metavar='FILE', type=str,
                        help='The wav or raw file to check.'
                             'The file format is determined by file extension.'
                             'For raw format, user must also pass -b, -r, -c'
                             'for bit width, rate, and number of channels.')
    parser.add_argument('--debug', action='store_true', default=False,
                        help='Show debug message.')
    parser.add_argument('--spectral-only', action='store_true', default=False,
                        help='Only do spectral analysis on each channel.')
    parser.add_argument('--freqs', metavar='FREQ', type=float,
                        nargs='*',
                        help='Expected frequencies in the channels. '
                             'Frequencies are separated by space. '
                             'E.g.: --freqs 1000 2000. '
                             'It means only the first two '
                             'channels (1000Hz, 2000Hz) are to be checked. '
                             'Unwanted channels can be specified by 0. '
                             'E.g.: --freqs 1000 0 2000 0 3000. '
                             'It means only channe 0,2,4 are to be examined.')
    parser.add_argument('--freq-threshold', metavar='FREQ_THRESHOLD', type=float,
                        default=5,
                        help='Frequency difference threshold in Hz. '
                             'Default is 5Hz')
    parser.add_argument('--ignore-high-freq', metavar='HIGH_FREQ_THRESHOLD',
                        type=float, default=5000,
                        help='Frequency threshold in Hz to be ignored for '
                             'high frequency. Default is 5KHz')
    parser.add_argument('--output-file', metavar='OUTPUT_FILE', type=str,
                        help='Output file to dump analysis result in JSON format')
    parser.add_argument('-b', '--bit-width', metavar='BIT_WIDTH', type=int,
                        default=32,
                        help='For raw file. Bit width of a sample. '
                             'Assume sample format is little-endian signed int. '
                             'Default is 32')
    parser.add_argument('-r', '--rate', metavar='RATE', type=int,
                        default=48000,
                        help='For raw file. Sampling rate. Default is 48000')
    parser.add_argument('-c', '--channel', metavar='CHANNEL', type=int,
                        default=8,
                        help='For raw file. Number of channels. '
                             'Default is 8.')

    # Arguments for quality measurement customization.
    parser.add_argument(
             '--quality-block-size-secs',
             metavar='BLOCK_SIZE_SECS', type=float,
             default=audio_quality_measurement.DEFAULT_BLOCK_SIZE_SECS,
             help='Block size for quality measurement. '
                  'Refer to audio_quality_measurement module for detail.')
    parser.add_argument(
             '--quality-frequency-error-threshold',
             metavar='FREQ_ERR_THRESHOLD', type=float,
             default=audio_quality_measurement.DEFAULT_FREQUENCY_ERROR,
             help='Frequency error threshold for identifying sine wave'
                  'in quality measurement. '
                  'Refer to audio_quality_measurement module for detail.')
    parser.add_argument(
             '--quality-delay-amplitude-threshold',
             metavar='DELAY_AMPLITUDE_THRESHOLD', type=float,
             default=audio_quality_measurement.DEFAULT_DELAY_AMPLITUDE_THRESHOLD,
             help='Amplitude ratio threshold for identifying delay in sine wave'
                  'in quality measurement. '
                  'Refer to audio_quality_measurement module for detail.')
    parser.add_argument(
             '--quality-noise-amplitude-threshold',
             metavar='NOISE_AMPLITUDE_THRESHOLD', type=float,
             default=audio_quality_measurement.DEFAULT_NOISE_AMPLITUDE_THRESHOLD,
             help='Amplitude ratio threshold for identifying noise in sine wave'
                  'in quality measurement. '
                  'Refer to audio_quality_measurement module for detail.')
    parser.add_argument(
             '--quality-burst-amplitude-threshold',
             metavar='BURST_AMPLITUDE_THRESHOLD', type=float,
             default=audio_quality_measurement.DEFAULT_BURST_AMPLITUDE_THRESHOLD,
             help='Amplitude ratio threshold for identifying burst in sine wave'
                  'in quality measurement. '
                  'Refer to audio_quality_measurement module for detail.')


def parse_args(parser):
    """Parses args."""
    args = parser.parse_args()
    return args


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

        @param filename: file name of the wave file.

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

        @param filename: The wave file to be read.

        @raises: RuntimeError: sox is not installed.

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

        @param filename: The wave file to be read.

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

        @param raw_data: An audio_data.AudioRawData object.
        @param rate: Sampling rate.

        """
        self._raw_data = raw_data
        self._rate = rate
        self._spectrals = []
        self._quality_result = []


    def do_spectral_analysis(self, ignore_high_freq, check_quality,
                             quality_params):
        """Gets the spectral_analysis result.

        @param ignore_high_freq: Ignore high frequencies above this threshold.
        @param check_quality: Check quality of each channel.
        @param quality_params: A QualityParams object for quality measurement.

        """
        self.has_data()
        for channel_idx in xrange(self._raw_data.channel):
            signal = self._raw_data.channel_data[channel_idx]
            max_abs = max(numpy.abs(signal))
            logging.debug('Channel %d max abs signal: %f', channel_idx, max_abs)
            if max_abs == 0:
                logging.info('No data on channel %d, skip this channel',
                              channel_idx)
                continue

            saturate_value = audio_data.get_maximum_value_from_sample_format(
                    self._raw_data.sample_format)
            normalized_signal = audio_analysis.normalize_signal(
                    signal, saturate_value)
            logging.debug('saturate_value: %f', saturate_value)
            logging.debug('max signal after normalized: %f', max(normalized_signal))
            spectral = audio_analysis.spectral_analysis(
                    normalized_signal, self._rate)

            logging.debug('Channel %d spectral:\n%s', channel_idx,
                          pprint.pformat(spectral))

            # Ignore high frequencies above the threshold.
            spectral = [(f, c) for (f, c) in spectral if f < ignore_high_freq]

            logging.info('Channel %d spectral after ignoring high frequencies '
                          'above %f:\n%s', channel_idx, ignore_high_freq,
                          pprint.pformat(spectral))

            if check_quality:
                quality = audio_quality_measurement.quality_measurement(
                        signal=normalized_signal,
                        rate=self._rate,
                        dominant_frequency=spectral[0][0],
                        block_size_secs=quality_params.block_size_secs,
                        frequency_error_threshold=quality_params.frequency_error_threshold,
                        delay_amplitude_threshold=quality_params.delay_amplitude_threshold,
                        noise_amplitude_threshold=quality_params.noise_amplitude_threshold,
                        burst_amplitude_threshold=quality_params.burst_amplitude_threshold)

                logging.debug('Channel %d quality:\n%s', channel_idx,
                              pprint.pformat(quality))
                self._quality_result.append(quality)

            self._spectrals.append(spectral)


    def has_data(self):
        """Checks if data has been set.

        @raises QualityCheckerError: if data or rate is not set yet.

        """
        if not self._raw_data or not self._rate:
            raise QualityCheckerError('Data and rate is not set yet')


    def check_freqs(self, expected_freqs, freq_threshold):
        """Checks the dominant frequencies in the channels.

        @param expected_freq: A list of frequencies. If frequency is 0, it
                              means this channel should be ignored.
        @param freq_threshold: The difference threshold to compare two
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
                        'Failed at channel %d: %f is too far away from %f' % (
                                idx, dominant_freq, expected_freq))


    def check_quality(self):
        """Checks the quality measurement results on each channel.

        @raises: QualityFailure when there is artifact.

        """
        error_msgs = []

        for idx, quality_res in enumerate(self._quality_result):
            artifacts = quality_res['artifacts']
            if artifacts['noise_before_playback']:
                error_msgs.append(
                        'Found noise before playback: %s' % (
                                artifacts['noise_before_playback']))
            if artifacts['noise_after_playback']:
                error_msgs.append(
                        'Found noise after playback: %s' % (
                                artifacts['noise_after_playback']))
            if artifacts['delay_during_playback']:
                error_msgs.append(
                        'Found delay during playback: %s' % (
                                artifacts['delay_during_playback']))
            if artifacts['burst_during_playback']:
                error_msgs.append(
                        'Found burst during playback: %s' % (
                                artifacts['burst_during_playback']))
        if error_msgs:
            raise QualityFailure('Found bad quality: %s', '\n'.join(error_msgs))


    def dump(self, output_file):
        """Dumps the result into a file in json format.

        @param output_file: A file path to dump spectral and quality
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


def read_audio_file(args):
    """Reads audio file.

    @param args: The namespace parsed from command line arguments.

    @returns: A tuple (raw_data, rate) where raw_data is
              audio_data.AudioRawData, rate is sampling rate.

    """
    if args.filename.endswith('.wav'):
        wavefile = WaveFile(args.filename)
        raw_data = wavefile.raw_data
        rate = wavefile.rate
    elif args.filename.endswith('.raw'):
        binary = None
        with open(args.filename, 'r') as f:
            binary = f.read()

        raw_data = audio_data.AudioRawData(
                binary=binary,
                channel=args.channel,
                sample_format='S%d_LE' % args.bit_width)
        rate = args.rate
    else:
        raise CheckQualityError(
                'File format for %s is not supported' % args.filename)

    return raw_data, rate


def get_quality_params(args):
    """Gets quality parameters in arguments.

    @param args: The namespace parsed from command line arguments.

    @returns: A QualityParams object.

    """
    quality_params = QualityParams(
            block_size_secs=args.quality_block_size_secs,
            frequency_error_threshold=args.quality_frequency_error_threshold,
            delay_amplitude_threshold=args.quality_delay_amplitude_threshold,
            noise_amplitude_threshold=args.quality_noise_amplitude_threshold,
            burst_amplitude_threshold=args.quality_burst_amplitude_threshold)

    return quality_params


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description='Check signal quality of a wave file. Each channel should'
                    ' either be all zeros, or sine wave of a fixed frequency.')
    add_args(parser)
    args = parse_args(parser)

    level = logging.DEBUG if args.debug else logging.INFO
    format = '%(asctime)-15s:%(levelname)s:%(pathname)s:%(lineno)d: %(message)s'
    logging.basicConfig(format=format, level=level)

    raw_data, rate = read_audio_file(args)

    checker = QualityChecker(raw_data, rate)

    quality_params = get_quality_params(args)

    checker.do_spectral_analysis(ignore_high_freq=args.ignore_high_freq,
                                 check_quality=(not args.spectral_only),
                                 quality_params=quality_params)

    if args.output_file:
        checker.dump(args.output_file)

    if args.freqs:
        checker.check_freqs(args.freqs, args.freq_threshold)

    if not args.spectral_only:
        checker.check_quality()
