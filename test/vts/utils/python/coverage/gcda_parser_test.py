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
import os
import unittest

from vts.utils.python.coverage import arc_summary
from vts.utils.python.coverage import block_summary
from vts.utils.python.coverage import file_summary
from vts.utils.python.coverage import function_summary
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage.parser_test import MockStream


class GCDAParserTest(unittest.TestCase):
    """Tests for GCDA parser of vts.utils.python.coverage.
    """

    GOLDEN_GCNO_PATH = 'testdata/sample.gcno'
    GOLDEN_GCDA_PATH = 'testdata/sample.gcda'

    def setUp(self):
        """Creates a stream for each test.
      """
        self.stream = MockStream(gcda_parser.GCDAParser.MAGIC)

    def testReadFunction(self):
        """Verifies that the correct function is read and returned.
        """
        ident = 100
        checksum = 0
        fs = file_summary.FileSummary()
        func = function_summary.FunctionSummary(ident, 'test', 'test.c', 0)
        fs.functions[ident] = func
        self.stream = MockStream.concat_int(self.stream, ident)
        self.stream = MockStream.concat_int(self.stream, 0)
        self.stream = MockStream.concat_int(self.stream, 0)
        self.stream = MockStream.concat_string(self.stream, 'test')
        length = 5
        parser = gcda_parser.GCDAParser(self.stream)
        parser.file_summary = fs
        func = parser.ReadFunction(5)
        assert (func.ident == ident)

    def testReadCountsNormal(self):
        """Verifies that counts are read correctly.

        Verifies that arcs are marked as resolved and count is correct.
        """
        n = 5
        fs = file_summary.FileSummary()
        func = function_summary.FunctionSummary(0, 'test', 'test.c', 0)
        blocks = [block_summary.BlockSummary(i, 0) for i in range(n)]
        func.blocks = blocks
        fs.functions[func.ident] = func
        for i in range(1, n):
            arc = arc_summary.ArcSummary(blocks[0], blocks[i], 0)
            blocks[0].exit_arcs.append(arc)
            blocks[i].entry_arcs.append(arc)
            self.stream = MockStream.concat_int64(self.stream, i)
        parser = gcda_parser.GCDAParser(self.stream)
        parser.file_summary = fs
        parser.ReadCounts(func)
        for i, arc in zip(range(1, n), blocks[0].exit_arcs):
            self.assertEqual(i, arc.count)
            self.assertTrue(arc.resolved)

    def testReadCountsFakeOrOnTree(self):
        """Verifies that counts are read correctly when there are skipped arcs.

        Verifies that the fake arc and the arc on the tree are skipped while other
        arcs are read and resolved correctly.
        """
        n = 10
        fs = file_summary.FileSummary()
        func = function_summary.FunctionSummary(0, 'test', 'test.c', 0)
        blocks = [block_summary.BlockSummary(i, 0) for i in range(n)]
        func.blocks = blocks
        fs.functions[func.ident] = func

        arc = arc_summary.ArcSummary(blocks[0], blocks[1],
                                     arc_summary.ArcSummary.GCOV_ARC_FAKE)
        blocks[0].exit_arcs.append(arc)
        blocks[1].entry_arcs.append(arc)

        arc = arc_summary.ArcSummary(blocks[0], blocks[2],
                                     arc_summary.ArcSummary.GCOV_ARC_ON_TREE)
        blocks[0].exit_arcs.append(arc)
        blocks[2].entry_arcs.append(arc)

        for i in range(3, n):
            arc = arc_summary.ArcSummary(blocks[0], blocks[i], 0)
            blocks[0].exit_arcs.append(arc)
            blocks[i].entry_arcs.append(arc)
            self.stream = MockStream.concat_int64(self.stream, i)

        parser = gcda_parser.GCDAParser(self.stream)
        parser.file_summary = fs
        parser.ReadCounts(func)
        self.assertFalse(blocks[0].exit_arcs[0].resolved)
        self.assertFalse(blocks[0].exit_arcs[1].resolved)
        for i, arc in zip(range(3, n), blocks[0].exit_arcs[2:]):
            self.assertEqual(i, arc.count)
            self.assertTrue(arc.resolved)

    def testSampleFile(self):
        """Asserts correct parsing of sample GCDA file.

        Verifies the block coverage counts for each function.
        """
        dir_path = os.path.dirname(os.path.realpath(__file__))
        gcno_path = os.path.join(dir_path, self.GOLDEN_GCNO_PATH)
        gcda_path = os.path.join(dir_path, self.GOLDEN_GCDA_PATH)
        summary = gcno_parser.ParseGcnoFile(gcno_path)
        gcda_parser.ParseGcdaFile(gcda_path, summary)
        # Function: main
        expected_list = [2, 0, 2, 2, 2, 0, 2, 2, 500, 502, 2, 2]
        for index, expected in zip(range(len(expected_list)), expected_list):
            self.assertEqual(summary.functions[3].blocks[index].count,
                             expected)

        # Function: testFunctionName
        expected_list = [2, 2, 2, 2, 2]
        for index, expected in zip(range(len(expected_list)), expected_list):
            self.assertEqual(summary.functions[4].blocks[index].count,
                             expected)


if __name__ == "__main__":
    unittest.main()
