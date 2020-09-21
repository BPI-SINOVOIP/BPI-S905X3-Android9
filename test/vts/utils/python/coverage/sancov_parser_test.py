#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import io
import struct
import unittest

import parser
import sancov_parser


class SancovParserTest(unittest.TestCase):
    """Tests for sancov_parser in package vts.utils.python.coverage.

    Ensures error handling, bitness detection, and correct
    parsing of offsets in the file..
    """

    GOLDEN_SANCOV_PATH = 'testdata/sample.sancov'
    GOLDEN_EXPECTED_BITNESS = 64
    GOLDEN_EXPECTED_OFFSETS = (
        12115, 12219, 12463, 12527, 17123, 17311, 17507, 17771, 17975, 17987, 18107, 18167,
        18299, 18503, 18571, 18743, 18755, 18791, 18903, 19127, 19715, 21027, 21123, 21223,
        21335, 21407, 21455, 21611, 21643, 21683, 23227, 23303, 23343, 23503, 23767, 23779,
        23791, 23819, 23867, 24615, 24651, 24743, 24775)

    def testInvalidMagic(self):
        """Asserts that an exception is raised when the magic is invalid.
        """
        stream = io.BytesIO(struct.pack('L', 0xC0BFFFFFFFFFFF10))
        p = sancov_parser.SancovParser(stream)
        with self.assertRaises(parser.FileFormatError) as context:
            p.Parse()
        self.assertTrue('Invalid magic' in str(context.exception))

    def testMagic32(self):
        """Asserts that values are correctly read in 32-bit sancov files.
        """
        stream = io.BytesIO(struct.pack('L', sancov_parser.MAGIC32))
        stream.seek(8)
        values = (1, 2, 3)
        stream.write(struct.pack('III', *values))
        stream.seek(0)
        p = sancov_parser.SancovParser(stream)
        s = p.Parse()
        self.assertEqual(32, p._bitness)
        self.assertEqual(values, s)

    def testMagic64(self):
        """Asserts that values are correctly read in 64-bit sancov files.
        """
        stream = io.BytesIO(struct.pack('L', sancov_parser.MAGIC64))
        stream.seek(8)
        values = (4, 5, 6)
        stream.write(struct.pack('LLL', *values))
        stream.seek(0)
        p = sancov_parser.SancovParser(stream)
        s = p.Parse()
        self.assertEqual(64, p._bitness)
        self.assertEqual(values, s)

    def testGetBitness32(self):
        """Asserts that bitness is correctly determined from a 32-bit sancov file.
        """
        stream = io.BytesIO(struct.pack('L', sancov_parser.MAGIC32))
        p = sancov_parser.SancovParser(stream)
        self.assertEqual(32, p.GetBitness())

    def testGetBitness64(self):
        """Asserts that bitness is correctly determined from a 64-bit sancov file.
        """
        stream = io.BytesIO(struct.pack('L', sancov_parser.MAGIC64))
        p = sancov_parser.SancovParser(stream)
        self.assertEqual(64, p.GetBitness())

    def testGolden(self):
        """Asserts that offsets are correctly parsed from the golden file.
        """
        bitness, offsets = sancov_parser.ParseSancovFile(self.GOLDEN_SANCOV_PATH)
        self.assertEqual(self.GOLDEN_EXPECTED_BITNESS, bitness)
        self.assertEqual(self.GOLDEN_EXPECTED_OFFSETS, offsets)



if __name__ == "__main__":
    unittest.main()
