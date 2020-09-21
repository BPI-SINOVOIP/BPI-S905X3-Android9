#!/usr/bin/env python
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

import math
import os
import struct
import unittest

from vts.utils.python.coverage import parser

MAGIC = 0x67636e6f


class MockStream(object):
    """MockStream object allows for mocking file reading behavior.

    Allows for adding integers and strings to the file stream in a
    specified byte format and then reads them as if from a file.

    Attributes:
        content: the byte list representing a file stream
        cursor: the index into the content such that everything before it
                has been read already.
    """
    BYTES_PER_WORD = 4

    def __init__(self, magic=MAGIC, format='<'):
        self.format = format
        self.magic = magic
        version = struct.unpack(format + 'I', '*802')[0]
        self.content = struct.pack(format + 'III', magic, version, 0)
        self.cursor = 0

    @classmethod
    def concat_int(cls, stream, integer):
        """Returns the stream with a binary formatted integer concatenated.

        Args:
            stream: the stream to which the integer will be concatenated.
            integer: the integer to be concatenated to the content stream.
            format: the string format decorator to apply to the integer.

        Returns:
            The content with the binary-formatted integer concatenated.
        """
        new_content = stream.content + struct.pack(stream.format + 'I',
                                                   integer)
        s = MockStream(stream.magic, stream.format)
        s.content = new_content
        s.cursor = stream.cursor
        return s

    @classmethod
    def concat_int64(cls, stream, integer):
        """Returns the stream with a binary formatted int64 concatenated.

        Args:
            stream: the stream to which the integer will be concatenated.
            integer: the 8-byte int to be concatenated to the content stream.
            format: the string format decorator to apply to the long.

        Returns:
            The content with the binary-formatted int64 concatenated.
        """
        lo = ((1 << 32) - 1) & integer
        hi = (integer - lo) >> 32
        new_content = stream.content + struct.pack(stream.format + 'II', lo,
                                                   hi)
        s = MockStream(stream.magic, stream.format)
        s.content = new_content
        s.cursor = stream.cursor
        return s

    @classmethod
    def concat_string(cls, stream, string):
        """Returns the stream with a binary formatted string concatenated.

        Preceeds the string with an integer representing the number of
        words in the string. Pads the string so that it is word-aligned.

        Args:
            stream: the stream to which the string will be concatenated.
            string: the string to be concatenated to the content stream.
            format: the string format decorator to apply to the integer.

        Returns:
            The content with the formatted binary string concatenated.
        """
        byte_count = len(string)
        word_count = int(
            math.ceil(byte_count * 1.0 / MockStream.BYTES_PER_WORD))
        padding = '\x00' * (
            MockStream.BYTES_PER_WORD * word_count - byte_count)
        new_content = stream.content + struct.pack(
            stream.format + 'I', word_count) + bytes(string + padding)
        s = MockStream(stream.magic, stream.format)
        s.content = new_content
        s.cursor = stream.cursor
        return s

    def read(self, n_bytes):
        """Reads the specified number of bytes from the content stream.

        Args:
            n_bytes: integer number of bytes to read.

        Returns:
            The string of length n_bytes beginning at the cursor location
            in the content stream.
        """
        content = self.content[self.cursor:self.cursor + n_bytes]
        self.cursor += n_bytes
        return content


class ParserTest(unittest.TestCase):
    """Tests for stream parser of vts.utils.python.coverage.

    Ensures error handling, byte order detection, and correct
    parsing of integers and strings.
    """

    def setUp(self):
        """Creates a stream for each test.
      """
        self.stream = MockStream()

    def testLittleEndiannessInitialization(self):
        """Tests parser init  with little-endian byte order.

        Verifies that the byte-order is correctly detected.
        """
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(p.format, '<')

    def testBigEndiannessInitialization(self):
        """Tests parser init with big-endian byte order.

        Verifies that the byte-order is correctly detected.
        """
        self.stream = MockStream(format='>')
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(p.format, '>')

    def testReadIntNormal(self):
        """Asserts that integers are correctly read from the stream.

        Tests the normal case--when the value is actually an integer.
        """
        integer = 2016
        self.stream = MockStream.concat_int(self.stream, integer)
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(p.ReadInt(), integer)

    def testReadIntEof(self):
        """Asserts that an error is thrown when the EOF is reached.
        """
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertRaises(parser.FileFormatError, p.ReadInt)

    def testReadInt64(self):
        """Asserts that longs are read correctly.
        """
        number = 68719476836
        self.stream = MockStream.concat_int64(self.stream, number)
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(number, p.ReadInt64())

        self.stream = MockStream(format='>')
        self.stream = MockStream.concat_int64(self.stream, number)
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(number, p.ReadInt64())

    def testReadStringNormal(self):
        """Asserts that strings are correctly read from the stream.

        Tests the normal case--when the string is correctly formatted.
        """
        test_string = "This is a test."
        self.stream = MockStream.concat_string(self.stream, test_string)
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertEqual(p.ReadString(), test_string)

    def testReadStringError(self):
        """Asserts that invalid string format raises error.

        Tests when the string length is too short and EOF is reached.
        """
        test_string = "This is a test."
        byte_count = len(test_string)
        word_count = int(round(byte_count / 4.0))
        padding = '\x00' * (4 * word_count - byte_count)
        test_string_padded = test_string + padding
        content = struct.pack('<I', word_count + 1)  #  will cause EOF error
        content += bytes(test_string_padded)
        self.stream.content += content
        p = parser.GcovStreamParserUtil(self.stream, MAGIC)
        self.assertRaises(parser.FileFormatError, p.ReadString)


if __name__ == "__main__":
    unittest.main()
