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


class BlockSummary(object):
    """Summarizes a block of code from a .gcno file.

    Contains all the lines in the block and the entry/exit
    arcs from/to other basic blocks.

    Attributes:
        flag: integer flag value.
        entry_arcs: list of ArcSummary objects for each arc
        entering the code block.
        exit_arcs: list of ArcSummary objects for each arc exiting
        the code block.
        lines: list of line numbers represented by the basic block.
    """

    def __init__(self, index, flag):
        """Inits the block summary with provided values.

        Initializes entryArcs, exitArcs, and lines to the empty list.
        Stores the block flag in the flag attribute.

        Args:
            index: the basic block number
            flag: integer block flag.
        """
        self.index = index
        self.flag = flag
        self.entry_arcs = []
        self.exit_arcs = []
        self.count = 0
        self.lines = []

    def __str__(self):
        """Serializes the block summary as a string.

        Returns:
            String representation of the block.
        """
        output = '\tBlock: %i, Count: %i' % (self.index, self.count)
        if len(self.lines):
            output += ', Lines: ' + ', '.join(str(line) for line in self.lines)
        if len(self.entry_arcs):
            output += ('\r\n\t\t' + str(self.index) + '  <--  ' + ', '.join(
                str(a.src_block.index) for a in self.entry_arcs))
        if len(self.exit_arcs):
            output += ('\r\n\t\t' + str(self.index) + '  -->  ' + ', '.join(
                str(a.dst_block.index) for a in self.exit_arcs))
        return output + '\r\n'
