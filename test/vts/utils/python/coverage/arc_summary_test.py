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


class ArcSummaryTest(unittest.TestCase):
    """Tests for ArcSummary of vts.utils.python.coverage.
    """

    def testResolveRemove(self):
        """Verifies that fake, non-fallthrough arc are resolved correctly.

        The arc should be removed as an exit arc from the source.
        """
        src = block_summary.BlockSummary(0, 0)
        dst = block_summary.BlockSummary(1, 0)
        flag = arc_summary.ArcSummary.GCOV_ARC_FAKE
        arc = arc_summary.ArcSummary(src, dst, flag)
        src.exit_arcs.append(arc)
        dst.entry_arcs.append(arc)
        self.assertTrue(arc.Resolve())
        self.assertEqual(len(src.exit_arcs), 0)

    def testResolveFromSource(self):
        """Verifies that arcs can be resolved from the source.

        In the case when the source has fully-resolved entry arcs, the arc
        count should be resolved from the source. I.e. there is only one
        missing arc and it can be solved for from the source.
        """
        middle = block_summary.BlockSummary(-1, 0)
        n = 10

        # Create resolved arcs entering the middle block
        for ident in range(n):
            block = block_summary.BlockSummary(ident, 0)
            arc = arc_summary.ArcSummary(block, middle, 0)
            arc.resolved = True
            arc.count = 1
            block.exit_arcs.append(arc)
            middle.entry_arcs.append(arc)

        # Create resolved arcs exiting the middle block
        for ident in range(n, 2 * n - 1):
            block = block_summary.BlockSummary(ident, 0)
            arc = arc_summary.ArcSummary(middle, block, 0)
            arc.resolved = True
            arc.count = 1
            block.entry_arcs.append(arc)
            middle.exit_arcs.append(arc)

        # Create one unresolved arc exiting the middle
        last = block_summary.BlockSummary(2 * n - 1, 0)
        arc = arc_summary.ArcSummary(middle, last, 0)
        middle.exit_arcs.append(arc)
        last.entry_arcs.append(arc)
        self.assertTrue(arc.Resolve())
        self.assertTrue(arc.resolved)
        self.assertEqual(arc.count, 1)

    def testResolveFromDest(self):
        """Verifies that arcs can be resolved from the destination block.

        In the case when the source has fully-resolved exit arcs, the arc
        count should be resolved from the source. I.e. there is only one
        missing arc and it can be solved for from the destination.
        """
        middle = block_summary.BlockSummary(-1, 0)
        n = 10

        # Create resolved arcs exiting the middle block
        for ident in range(n):
            block = block_summary.BlockSummary(ident, 0)
            arc = arc_summary.ArcSummary(middle, block, 0)
            arc.resolved = True
            arc.count = 1
            block.entry_arcs.append(arc)
            middle.exit_arcs.append(arc)

        # Create resolved arcs entering the middle block
        for ident in range(n, 2 * n - 1):
            block = block_summary.BlockSummary(ident, 0)
            arc = arc_summary.ArcSummary(block, middle, 0)
            arc.resolved = True
            arc.count = 1
            block.exit_arcs.append(arc)
            middle.entry_arcs.append(arc)

        # Create one unresolved arc entering the middle
        block = block_summary.BlockSummary(2 * n - 1, 0)
        arc = arc_summary.ArcSummary(block, middle, 0)
        middle.entry_arcs.append(arc)
        block.exit_arcs.append(arc)
        self.assertTrue(arc.Resolve())
        self.assertTrue(arc.resolved)
        self.assertEqual(arc.count, 1)


if __name__ == "__main__":
    unittest.main()
