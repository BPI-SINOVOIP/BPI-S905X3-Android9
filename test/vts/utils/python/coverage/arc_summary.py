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


class ArcSummary(object):
    """Summarizes an arc from a .gcno file.

    Attributes:
        src_block_index: integer index of the source basic block.
        dstBlockIndex: integer index of the destination basic block.
        on_tree: True iff arc has flag GCOV_ARC_ON_TREE.
        fake: True iff arc has flag GCOV_ARC_FAKE.
        fallthrough: True iff arc has flag GCOV_ARC_FALLTHROUGH.
        resolved: True iff the arc's count has been resolved.
        count: Integer number of times the arc was covered.
    """

    GCOV_ARC_ON_TREE = 1
    GCOV_ARC_FAKE = 1 << 1
    GCOV_ARC_FALLTHROUGH = 1 << 2

    def __init__(self, src_block, dst_block, flag):
        """Inits the arc summary with provided values.

        Stores the source and destination block indices and parses
        the arc flag.

        Args:
            src_block: BlockSummary of source block.
            dst_block: BlockSummary of destination block.
            flag: integer flag for the given arc.
        """

        self.src_block = src_block
        self.dst_block = dst_block
        self.on_tree = bool(flag & self.GCOV_ARC_ON_TREE)
        self.fake = bool(flag & self.GCOV_ARC_FAKE)
        self.fallthrough = bool(flag & self.GCOV_ARC_FALLTHROUGH)
        self.resolved = False
        self.count = 0

    def Resolve(self):
        """Resolves the arc count and returns True if successful.

        Uses the property that the sum of counts of arcs entering a
        node is equal to the sum of counts of arcs leaving a node. The
        exception to this rule is fake non-fallthrough nodes, which have
        no exit edges. In this case, remove the arc as an exit arc from
        the source so that the source can be resolved.

        Returns:
            True if the arc could be resolved and False otherwise.
        """
        if self.fake and not self.fallthrough:
            try:
                self.src_block.exit_arcs.remove(self)
            except ValueError:
                pass
        elif (len(self.src_block.entry_arcs) > 0 and
              all(a.resolved for a in self.src_block.entry_arcs) and
              all(a.resolved for a in self.src_block.exit_arcs if a != self)):
            in_flow = sum(a.count for a in self.src_block.entry_arcs)
            out_flow = sum(a.count for a in self.src_block.exit_arcs
                           if a != self)
            self.count = in_flow - out_flow
            self.resolved = True
        elif (len(self.dst_block.exit_arcs) > 0 and
              all(a.resolved for a in self.dst_block.exit_arcs) and
              all(a.resolved for a in self.dst_block.entry_arcs if a != self)):
            out_flow = sum(a.count for a in self.dst_block.exit_arcs)
            in_flow = sum(a.count for a in self.dst_block.entry_arcs
                          if a != self)
            self.count = out_flow - in_flow
            self.resolved = True
        else:
            return False
        return True
