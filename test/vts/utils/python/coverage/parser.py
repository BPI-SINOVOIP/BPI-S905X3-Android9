#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
"""Generic parser class for reading GCNO and GCDA files.

Implements read functions for strings, 32-bit integers, and
64-bit integers.
"""

import struct


class FileFormatError(Exception):
    """Exception for invalid file format.

    Thrown when an unexpected value type is read from the file stream
    or when the end of file is reached unexpectedly."""

    pass


class GcovStreamParserUtil(object):
    """Parser object for storing the stream and format information.

    Attributes:
        stream: File stream object for a GCNO file
        format: Character denoting the endianness of the file
        checksum: The checksum (int) of the file
    """

    def __init__(self, stream, magic):
        """Inits the parser with the input stream.

        The byte order is set by default to little endian and the summary file
        is instantiated with an empty GCNOSummary object.

        Args:
            stream: An input binary file stream to a .gcno file
            gcno_summary: The summary from a parsed gcno file
        """
        self.stream = stream
        self.format = '<'

        tag = self.ReadInt()
        self.version = ''.join(
            struct.unpack(self.format + 'ssss', self.stream.read(4)))
        self.checksum = self.ReadInt()

        if tag != magic:
            tag = struct.unpack('>I', struct.pack('<I', tag))[0]
            if tag == magic:  #  switch endianness
                self.format = '>'
            else:
                raise FileFormatError('Invalid file format.')

    def ReadInt(self):
        """Reads and returns an integer from the stream.

        Returns:
          A 4-byte integer from the stream attribute.

        Raises:
          FileFormatError: Corrupt file.
        """
        try:
            return struct.unpack(self.format + 'I', self.stream.read(4))[0]
        except (TypeError, ValueError, struct.error) as error:
            raise FileFormatError('Corrupt file.')

    def ReadInt64(self):
        """Reads and returns a 64-bit integer from the stream.

        Returns:
            An 8-byte integer from the stream attribute.

        Raises:
            FileFormatError: Corrupt file.
        """
        lo = self.ReadInt()
        hi = self.ReadInt()
        return (hi << 32) | lo

    def ReadString(self):
        """Reads and returns a string from the stream.

        First reads an integer denoting the number of words to read,
        then reads and returns the string with trailing padding characters
        stripped.

        Returns:
            A string from the stream attribute.

        Raises:
            FileFormatError: End of file reached.
        """
        length = self.ReadInt() << 2
        if length > 0:
            try:
                return ''.join(
                    struct.unpack(self.format + 's' * length, self.stream.read(
                        length))).rstrip('\x00')
            except (TypeError, ValueError, struct.error):
                raise FileFormatError('Corrupt file.')
        return str()
