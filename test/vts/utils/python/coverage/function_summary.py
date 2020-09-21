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


class FunctionSummary(object):
    """Summarizes a function and its blocks from .gcno file.

    Attributes:
        blocks: list of BlockSummary objects for each block in the function.
        ident: integer function identifier.
        name: function name.
        src_file_name: name of source file containing the function.
        first_line_number: integer line number at which the function begins in
            srcFile.
    """

    def __init__(self, ident, name, src_file_name, first_line_number):
        """Inits the function summary with provided values.

        Stores the identification string, name, source file name, and
        first line number in the object attributes. Initializes the block
        attribute to the empty list.

        Args:
            ident: integer function identifier.
            name: function name.
            src_file_name: name of source file containing the function.
            first_line_number: integer line number at which the function begins in
              the source file.
        """
        self.blocks = []
        self.ident = ident
        self.name = name
        self.src_file_name = src_file_name
        self.first_line_number = first_line_number

    def Resolve(self):
        """Resolves the block and arc counts.

        Using the edges that were resolved by the GCDA file,
        counts are resolved in the unresolved arcs. Then, block
        counts are resolved by summing the counts along arcs entering
        the block.

        Returns:
            True if the counts could be resolved and False otherwise.
        """

        unresolved_arcs = []
        for block in self.blocks:
            for arc in block.exit_arcs:
                if not arc.resolved:
                    unresolved_arcs.append(arc)

        index = 0
        prev_length = len(unresolved_arcs) + 1
        # Resolve the arc counts
        while len(unresolved_arcs) > 0:
            index = index % len(unresolved_arcs)
            if index == 0 and len(unresolved_arcs) == prev_length:
                return False
            else:
                prev_length = len(unresolved_arcs)
            arc = unresolved_arcs[index]
            if arc.Resolve():
                unresolved_arcs.remove(arc)
            else:
                index = index + 1

        # Resolve the block counts
        for block in self.blocks:
            if len(block.entry_arcs):
                block.count = sum(arc.count for arc in block.entry_arcs)
            else:
                block.count = sum(arc.count for arc in block.exit_arcs)

        return True

    def __str__(self):
        """Serializes the function summary as a string.

        Returns:
            String representation of the functions and its blocks.
        """
        output = ('Function:  %s : %s\r\n\tFirst Line Number:%i\r\n' %
                  (self.src_file_name, self.name, self.first_line_number))
        for b in self.blocks:
            output += str(b)
        return output
