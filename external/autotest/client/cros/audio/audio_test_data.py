#!/usr/bin/env python
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides audio test data."""

import logging
import os
import subprocess

from autotest_lib.client.cros.audio import audio_data
from autotest_lib.client.cros.audio import sox_utils


class AudioTestDataException(Exception):
    """Exception for audio test data."""
    pass


class AudioTestData(object):
    """Class to represent audio test data."""
    def __init__(self, data_format=None, path=None, frequencies=None,
                 duration_secs=None):
        """
        Initializes an audio test file.

        @param data_format: A dict containing data format including
                            file_type, sample_format, channel, and rate.
                            file_type: file type e.g. 'raw' or 'wav'.
                            sample_format: One of the keys in
                                           audio_data.SAMPLE_FORMAT.
                            channel: number of channels.
                            rate: sampling rate.
        @param path: The path to the file.
        @param frequencies: A list containing the frequency of each channel in
                            this file. Only applicable to data of sine tone.
        @param duration_secs: Duration of test file in seconds.

        @raises: AudioTestDataException if the path does not exist.

        """
        self.data_format = data_format
        if not os.path.exists(path):
            raise AudioTestDataException('Can not find path %s' % path)
        self.path = path
        self.frequencies = frequencies
        self.duration_secs = duration_secs


    def get_binary(self):
        """The binary of test data.

        @returns: The binary of test data.

        """
        with open(self.path, 'rb') as f:
            return f.read()


    def convert(self, data_format, volume_scale, path=None):
        """Converts the data format and returns a new AudioTestData object.

        Converts the source file at self.path to a new data format.
        The destination file path is self.path with a suffix. E.g.
        original_path = '/tmp/test.raw'
        data_format = dict(file_type='raw', sample_format='S32_LE',
                           channel=2, rate=48000)
        new_path = '/tmp/test_raw_48000_S32_LE_2.raw'

        This method returns a new AudioTestData object so the original object is
        not changed.

        @param data_format: A dict containing new data format.
        @param volume_scale: A float for volume scale used in sox command.
                              E.g. 1.0 is the same. 0.5 to scale volume by
                              half. -1.0 to invert the data.
        @param path: The path to the file of new AudioTestData. If this is None,
                     this function will add the suffix described above to the
                     path of the source file.

        @returns: A new AudioTestData object with converted format and new path.

        """
        if path:
            new_path = path
        else:
            original_path_without_ext, _ = os.path.splitext(self.path)
            new_ext = '.' + data_format['file_type']
            # New path will be the composition of original name, new data
            # format, and new file type as extension.
            new_path = (original_path_without_ext + '_' +
                        '_'.join(str(x) for x in data_format.values()) +
                        new_ext)

        logging.debug('src data_format: %s', self.data_format)
        logging.debug('dst data_format: %s', data_format)

        # If source file has header, use that header.
        if self.data_format['file_type'] != 'raw':
            use_src_header = True
            channels_src = None
            rate_src = None
            bits_src = None
        else:
            use_src_header = False
            channels_src = self.data_format['channel']
            rate_src = self.data_format['rate']
            bits_src = audio_data.SAMPLE_FORMATS[
                    self.data_format['sample_format']]['size_bytes'] * 8

        # If dst file type is not raw, write file format into header of dst
        # file.
        use_dst_header = data_format['file_type'] != 'raw'

        sox_utils.convert_format(
                path_src=self.path,
                channels_src=channels_src,
                rate_src=rate_src,
                bits_src=bits_src,
                path_dst=new_path,
                channels_dst=data_format['channel'],
                rate_dst=data_format['rate'],
                bits_dst=audio_data.SAMPLE_FORMATS[
                        data_format['sample_format']]['size_bytes'] * 8,
                volume_scale=volume_scale,
                use_src_header=use_src_header,
                use_dst_header=use_dst_header)

        new_test_data = AudioTestData(path=new_path,
                                      data_format=data_format)

        return new_test_data


    def delete(self):
        """Deletes the file at self.path."""
        os.unlink(self.path)


class FakeTestData(object):
    def __init__(self, frequencies, url=None, duration_secs=None):
        """A fake test data which contains properties but no real data.

        This is useful when we need to pass an AudioTestData object into a test
        or audio_test_utils.check_recorded_frequency.

        @param frequencies: A list containing the frequency of each channel in
                            this file. Only applicable to data of sine tone.
        @param url: The URL to the test file.
        @param duration_secs: The duration of the file in seconds.

        """
        self.frequencies = frequencies
        self.url = url
        self.duration_secs = duration_secs


def GenerateAudioTestData(data_format, path, frequencies=None,
            duration_secs=None, volume_scale=None):
    """Generates audio test data with specified format and frequencies.

    @param data_format: A dict containing data format including
                        file_type, sample_format, channel, and rate.
                        file_type: file type e.g. 'raw' or 'wav'.
                        sample_format: One of the keys in
                                       audio_data.SAMPLE_FORMAT.
                        channel: number of channels.
                        rate: sampling rate.
    @param path: The path to the file.
    @param frequencies: A list containing the frequency of each channel in
                        this file. Only applicable to data of sine tone.
    @param duration_secs: Duration of test file in seconds.
    @param volume_scale: A float for volume scale used in sox command.
                         E.g. 0.5 to scale volume by half. -1.0 to invert.

    @returns an AudioTestData object.
    """
    sample_format = audio_data.SAMPLE_FORMATS[data_format['sample_format']]
    bits = sample_format['size_bytes'] * 8

    if volume_scale:
        path_without_ext, ext = os.path.splitext(path)
        sox_file_path = os.path.join(path_without_ext + "_temp" + ext)
    else:
        sox_file_path = path

    command = sox_utils.generate_sine_tone_cmd(
            filename=sox_file_path,
            channels=data_format['channel'],
            bits=bits,
            rate=data_format['rate'],
            duration=duration_secs,
            frequencies=frequencies,
            raw=(data_format['file_type'] == 'raw'))

    logging.info(' '.join(command))
    subprocess.check_call(command)

    test_data = AudioTestData(data_format=data_format, path=sox_file_path,
            frequencies=frequencies, duration_secs=duration_secs)

    if volume_scale:
        converted_test_data = test_data.convert(data_format, volume_scale, path)
        test_data.delete()
        return converted_test_data
    else:
        return test_data


AUDIO_PATH = os.path.join(os.path.dirname(__file__))

"""
This test data contains frequency sweep from 20Hz to 20000Hz in two channels.
Left channel sweeps from 20Hz to 20000Hz, while right channel sweeps from
20000Hz to 20Hz. The sweep duration is 2 seconds. The begin and end of the file
is padded with 0.4 seconds of silence. The file is two-channel raw data with
each sample being a signed 16-bit integer in little-endian with sampling rate
48000 samples/sec.
"""
SWEEP_TEST_FILE = AudioTestData(
        path=os.path.join(AUDIO_PATH, 'pad_sweep_pad_16.raw'),
        data_format=dict(file_type='raw',
                         sample_format='S16_LE',
                         channel=2,
                         rate=48000))

"""
This test data contains fixed frequency sine wave in two channels.
Left channel is 2KHz, while right channel is 1KHz. The duration is 6 seconds.
The file format is two-channel raw data with each sample being a signed
16-bit integer in little-endian with sampling rate 48000 samples/sec.
"""
FREQUENCY_TEST_FILE = AudioTestData(
        path=os.path.join(AUDIO_PATH, 'fix_2k_1k_16.raw'),
        data_format=dict(file_type='raw',
                         sample_format='S16_LE',
                         channel=2,
                         rate=48000),
        frequencies=[2000, 1000])

"""
This test data contains fixed frequency sine wave in two channels.
Left and right channel are both 440Hz. The duration is 10 seconds.
The file format is two-channel raw data with each sample being a signed
16-bit integer in little-endian with sampling rate 48000 samples/sec.
The volume is 0.1. The small volume is to avoid distortion when played
on Chameleon.
"""
SIMPLE_FREQUENCY_TEST_FILE = AudioTestData(
        path=os.path.join(AUDIO_PATH, 'fix_440_16.raw'),
        data_format=dict(file_type='raw',
                         sample_format='S16_LE',
                         channel=2,
                         rate=48000),
        frequencies=[440, 440])

"""
This test data contains fixed frequency sine wave in two channels.
Left and right channel are both 1330 Hz. The duration is 10 seconds.
The file format is two-channel raw data with each sample being a signed
16-bit integer in little-endian with sampling rate 48000 samples/sec.
The volume is 0.1. The small volume is to avoid distortion when played
on Chameleon.
"""
SIMPLE_FREQUENCY_TEST_1330_FILE = AudioTestData(
        path=os.path.join(AUDIO_PATH, 'fix_1330_16.raw'),
        data_format=dict(file_type='raw',
                         sample_format='S16_LE',
                         channel=2,
                         rate=48000),
        frequencies=[1330, 1330])

"""
This test data contains fixed frequency sine wave in two channels.
Left and right channel are both 440Hz. The duration is 10 seconds.
The file format is two-channel raw data with each sample being a signed
16-bit integer in little-endian with sampling rate 48000 samples/sec.
The volume is 0.5. The larger volume is needed to test internal
speaker of Cros device because the microphone of Chameleon is not sensitive
enough.
"""
SIMPLE_FREQUENCY_SPEAKER_TEST_FILE = AudioTestData(
        path=os.path.join(AUDIO_PATH, 'fix_440_16_half.raw'),
        data_format=dict(file_type='raw',
                         sample_format='S16_LE',
                         channel=2,
                         rate=48000),
        frequencies=[440, 440])

"""
Media test verification for 256Hz frequency (headphone audio).
"""
MEDIA_HEADPHONE_TEST_FILE = FakeTestData(frequencies=[256, 256])

"""
Media test verification for 512Hz frequency (onboard speakers).
"""
MEDIA_SPEAKER_TEST_FILE = FakeTestData(frequencies=[512, 512])

"""
Test file for 10 min playback for headphone. Left frequency is 1350Hz, right
frequency is 870 Hz, and amplitude is 0.85.
"""
HEADPHONE_10MIN_TEST_FILE = FakeTestData(
        frequencies=[1350, 870],
        url=('http://commondatastorage.googleapis.com/chromiumos-test-assets-'
             'public/audio_test/chameleon/Headphone/L1350_R870_A085_10min.wav'),
        duration_secs=600)
