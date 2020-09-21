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
from vts.utils.python.coverage import function_summary


class FunctionSummaryTest(unittest.TestCase):
    """Tests for FunctionSummary of vts.utils.python.coverage.
    """

    def setUp(self):
        """Creates a function summary and a chain of blocks.

        Creates an arc between adjacent blocks. All arcs are left with default
        values (unresolved, count of 0).
        """
        self.n = 10
        self.count = 5
        self.function_summary = function_summary.FunctionSummary(0, 'test',
                                                                 'test.c', 0)
        self.function_summary.blocks = [block_summary.BlockSummary(i, 0)
                                        for i in range(self.n)]
        self.arcs = []
        for i in range(1, self.n):
            arc = arc_summary.ArcSummary(self.function_summary.blocks[i - 1],
                                         self.function_summary.blocks[i], 0)
            self.function_summary.blocks[i - 1].exit_arcs.append(arc)
            self.function_summary.blocks[i].entry_arcs.append(arc)
            self.arcs.append(arc)

    def testResolveChainStart(self):
        """Tests for correct resolution in a long chain.

        Test resolution for the case when there is a chain of unresolved arcs
        after a resolved arc.
        """
        self.arcs[0].resolved = True
        self.arcs[0].count = self.count

        self.function_summary.Resolve()
        for arc in self.arcs:
            self.assertTrue(arc.resolved)
            self.assertEqual(self.count, arc.count)
            self.assertEqual(self.count, arc.src_block.count)
            self.assertEqual(self.count, arc.dst_block.count)

    def testResolveChainEnd(self):
        """Tests for correct resolution in a long chain.

        Test resolution for the case when there is a chain of unresolved arcs
        before a resolved arc.
        """
        self.arcs[-1].resolved = True
        self.arcs[-1].count = self.count

        self.function_summary.Resolve()
        for arc in self.arcs:
            self.assertTrue(arc.resolved)
            self.assertEqual(self.count, arc.count)
            self.assertEqual(self.count, arc.src_block.count)
            self.assertEqual(self.count, arc.dst_block.count)

    def testResolveFailure(self):
        """Tests for failure when no progress can be made.

        Test that Resolve() returns False when there is not enough information
        to resolve any arcs.
        """
        self.assertFalse(self.function_summary.Resolve())


if __name__ == "__main__":
    unittest.main()
