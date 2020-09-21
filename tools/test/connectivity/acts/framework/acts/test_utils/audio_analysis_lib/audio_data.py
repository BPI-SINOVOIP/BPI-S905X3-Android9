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
"""This module provides abstraction of audio data."""

import contextlib
import copy
import numpy
import struct
from io import StringIO
"""The dict containing information on how to parse sample from raw data.

Keys: The sample format as in aplay command.
Values: A dict containing:
    message: Human-readable sample format.
    dtype_str: Data type used in numpy dtype.  Check
               https://docs.scipy.org/doc/numpy/reference/arrays.dtypes.html
               for supported data type.
    size_bytes: Number of bytes for one sample.
"""
SAMPLE_FORMATS = dict(
    S32_LE=dict(
        message='Signed 32-bit integer, little-endian',
        dtype_str='<i',
        size_bytes=4),
    S16_LE=dict(
        message='Signed 16-bit integer, little-endian',
        dtype_str='<i',
        size_bytes=2))


def get_maximum_value_from_sample_format(sample_format):
    """Gets the maximum value from sample format.

    Args:
        sample_format: A key in SAMPLE_FORMAT.

    Returns:The maximum value the sample can hold + 1.

    """
    size_bits = SAMPLE_FORMATS[sample_format]['size_bytes'] * 8
    return 1 << (size_bits - 1)


class AudioRawDataError(Exception):
    """Error in AudioRawData."""
    pass


class AudioRawData(object):
    """The abstraction of audio raw data.

    @property channel: The number of channels.
    @property channel_data: A list of lists containing samples in each channel.
                            E.g., The third sample in the second channel is
                            channel_data[1][2].
    @property sample_format: The sample format which should be one of the keys
                             in audio_data.SAMPLE_FORMATS.
    """

    def __init__(self, binary, channel, sample_format):
        """Initializes an AudioRawData.

        Args:
            binary: A string containing binary data. If binary is not None,
                       The samples in binary will be parsed and be filled into
                       channel_data.
            channel: The number of channels.
            sample_format: One of the keys in audio_data.SAMPLE_FORMATS.
        """
        self.channel = channel
        self.channel_data = [[] for _ in range(self.channel)]
        self.sample_format = sample_format
        if binary:
            self.read_binary(binary)

    def read_binary(self, binary):
        """Reads samples from binary and fills channel_data.

        Reads samples of fixed width from binary string into a numpy array
        and shapes them into each channel.

        Args:
            binary: A string containing binary data.
        """
        sample_format_dict = SAMPLE_FORMATS[self.sample_format]

        # The data type used in numpy fromstring function. For example,
        # <i4 for 32-bit signed int.
        np_dtype = '%s%d' % (sample_format_dict['dtype_str'],
                             sample_format_dict['size_bytes'])

        # Reads data from a string into 1-D array.
        np_array = numpy.fromstring(binary, dtype=np_dtype)

        n_frames = len(np_array) / self.channel
        # Reshape np_array into an array of shape (n_frames, channel).
        np_array = np_array.reshape(int(n_frames), self.channel)
        # Transpose np_arrya so it becomes of shape (channel, n_frames).
        self.channel_data = np_array.transpose()
