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

from vts.utils.python.coverage import arc_summary
from vts.utils.python.coverage import block_summary
from vts.utils.python.coverage import function_summary
from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage.parser_test import MockStream


class GCNOParserTest(unittest.TestCase):
    """Tests for GCNO parser of vts.utils.python.coverage.

    Ensures error handling, byte order detection, and correct
    parsing of functions, blocks, arcs, and lines.
    """

    GOLDEN_GCNO_PATH = 'testdata/sample.gcno'

    def setUp(self):
        """Creates a stream for each test.
        """
        self.stream = MockStream()

    def testReadFunction(self):
        """Asserts that the function is read correctly.

        Verifies that ident, name, source file name,
        and first line number are all read correctly.
        """
        ident = 102010
        self.stream = MockStream.concat_int(self.stream, ident)
        self.stream = MockStream.concat_int(self.stream, 0)
        self.stream = MockStream.concat_int(self.stream, 0)
        name = "TestFunction"
        src_file_name = "TestSouceFile.c"
        first_line_number = 102
        self.stream = MockStream.concat_string(self.stream, name)
        self.stream = MockStream.concat_string(self.stream, src_file_name)
        self.stream = MockStream.concat_int(self.stream, first_line_number)
        parser = gcno_parser.GCNOParser(self.stream)
        summary = parser.ReadFunction()
        self.assertEqual(name, summary.name)
        self.assertEqual(ident, summary.ident)
        self.assertEqual(src_file_name, summary.src_file_name)
        self.assertEqual(first_line_number, summary.first_line_number)

    def testReadBlocks(self):
        """Asserts that blocks are correctly read from the stream.

        Tests correct values for flag and index.
        """
        n_blocks = 10
        func = function_summary.FunctionSummary(0, "func", "src.c", 1)
        for i in range(n_blocks):
            self.stream = MockStream.concat_int(self.stream, 3 * i)
        parser = gcno_parser.GCNOParser(self.stream)
        parser.ReadBlocks(n_blocks, func)
        self.assertEqual(len(func.blocks), n_blocks)
        for i in range(n_blocks):
            self.assertEqual(func.blocks[i].flag, 3 * i)
            self.assertEqual(func.blocks[i].index, i)

    def testReadArcsNormal(self):
        """Asserts that arcs are correctly read from the stream.

        Does not test the use of flags. Validates that arcs are
        created in both blocks and the source/destination are
        correct for each.
        """
        n_blocks = 50
        func = function_summary.FunctionSummary(0, "func", "src.c", 1)
        func.blocks = [block_summary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        src_block_index = 0
        skip = 2
        self.stream = MockStream.concat_int(self.stream, src_block_index)
        for i in range(src_block_index + 1, n_blocks, skip):
            self.stream = MockStream.concat_int(self.stream, i)
            self.stream = MockStream.concat_int(
                self.stream, 0)  #  no flag applied to the arc
        parser = gcno_parser.GCNOParser(self.stream)
        n_arcs = len(range(src_block_index + 1, n_blocks, skip))
        parser.ReadArcs(n_arcs * 2 + 1, func)
        j = 0
        for i in range(src_block_index + 1, n_blocks, skip):
            self.assertEqual(
                func.blocks[src_block_index].exit_arcs[j].src_block.index,
                src_block_index)
            self.assertEqual(
                func.blocks[src_block_index].exit_arcs[j].dst_block.index, i)
            self.assertEqual(func.blocks[i].entry_arcs[0].src_block.index,
                             src_block_index)
            self.assertEqual(func.blocks[i].entry_arcs[0].dst_block.index, i)
            j += 1

    def testReadArcFlags(self):
        """Asserts that arc flags are correctly interpreted.
        """
        n_blocks = 5
        func = function_summary.FunctionSummary(0, "func", "src.c", 1)
        func.blocks = [block_summary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        self.stream = MockStream.concat_int(self.stream,
                                            0)  #  source block index

        self.stream = MockStream.concat_int(self.stream, 1)  #  normal arc
        self.stream = MockStream.concat_int(self.stream, 0)

        self.stream = MockStream.concat_int(self.stream, 2)  #  on-tree arc
        self.stream = MockStream.concat_int(
            self.stream, arc_summary.ArcSummary.GCOV_ARC_ON_TREE)

        self.stream = MockStream.concat_int(self.stream, 3)  #  fake arc
        self.stream = MockStream.concat_int(
            self.stream, arc_summary.ArcSummary.GCOV_ARC_FAKE)

        self.stream = MockStream.concat_int(self.stream, 4)  #  fallthrough arc
        self.stream = MockStream.concat_int(
            self.stream, arc_summary.ArcSummary.GCOV_ARC_FALLTHROUGH)

        parser = gcno_parser.GCNOParser(self.stream)
        parser.ReadArcs(4 * 2 + 1, func)

        self.assertFalse(func.blocks[0].exit_arcs[0].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[0].fake)
        self.assertFalse(func.blocks[0].exit_arcs[0].fallthrough)

        self.assertTrue(func.blocks[0].exit_arcs[1].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[1].fake)
        self.assertFalse(func.blocks[0].exit_arcs[1].fallthrough)

        self.assertFalse(func.blocks[0].exit_arcs[2].on_tree)
        self.assertTrue(func.blocks[0].exit_arcs[2].fake)
        self.assertFalse(func.blocks[0].exit_arcs[2].fallthrough)

        self.assertFalse(func.blocks[0].exit_arcs[3].on_tree)
        self.assertFalse(func.blocks[0].exit_arcs[3].fake)
        self.assertTrue(func.blocks[0].exit_arcs[3].fallthrough)

    def testReadLines(self):
        """Asserts that lines are read correctly.

        Blocks must have correct references to the lines contained
        in the block.
        """
        self.stream = MockStream.concat_int(self.stream, 2)  #  block number
        self.stream = MockStream.concat_int(self.stream, 0)  #  dummy
        name = "src.c"
        name_length = int(
            math.ceil(1.0 * len(name) / MockStream.BYTES_PER_WORD)) + 1
        self.stream = MockStream.concat_string(self.stream, name)
        n_arcs = 5
        for i in range(1, n_arcs + 1):
            self.stream = MockStream.concat_int(self.stream, i)

        n_blocks = 5
        func = function_summary.FunctionSummary(0, "func", name, 1)
        func.blocks = [block_summary.BlockSummary(i, 3 * i)
                       for i in range(n_blocks)]
        parser = gcno_parser.GCNOParser(self.stream)
        parser.ReadLines(n_arcs + name_length + 3, func)
        self.assertEqual(len(func.blocks[2].lines), 5)
        self.assertEqual(func.blocks[2].lines, range(1, 6))

    def testSampleFile(self):
        """Asserts correct parsing of sample GCNO file.

        Verifies the blocks and lines for each function in
        the file.
        """
        dir_path = os.path.dirname(os.path.realpath(__file__))
        file_path = os.path.join(dir_path, self.GOLDEN_GCNO_PATH)
        summary = gcno_parser.ParseGcnoFile(file_path)
        self.assertEqual(len(summary.functions), 2)

        # Check function: testFunctionName
        func = summary.functions[4]
        self.assertEqual(func.name, 'testFunctionName')
        self.assertEqual(func.src_file_name, 'sample.c')
        self.assertEqual(func.first_line_number, 35)
        self.assertEqual(len(func.blocks), 5)
        expected_list = [[], [], [35, 40, 41], [42], []]
        for index, expected in zip(range(5), expected_list):
            self.assertEqual(func.blocks[index].lines, expected)

        # Check function: main
        func = summary.functions[3]
        self.assertEqual(func.name, 'main')
        self.assertEqual(func.first_line_number, 5)
        self.assertEqual(len(func.blocks), 12)
        self.assertEqual(func.blocks[0].lines, [])
        expected_list = [[], [], [5, 11, 12, 13], [15], [17], [18], [20],
                         [23, 24, 25], [26, 25], [], [29], [31]]
        for index, expected in zip(range(12), expected_list):
            self.assertEqual(func.blocks[index].lines, expected)


if __name__ == "__main__":
    unittest.main()
