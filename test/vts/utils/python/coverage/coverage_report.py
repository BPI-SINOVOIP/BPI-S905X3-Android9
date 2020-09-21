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
"""Generates coverage reports using outputs from GCC.

The GenerateCoverageReport() function returns HTML to display the coverage
at each line of code in a provided source file. Coverage information is
parsed from .gcno and .gcda file contents and combined with the source file
to reconstruct a coverage report. GenerateLineCoverageVector() is a helper
function that produces a vector of line counts and GenerateCoverageHTML()
uses the vector and source to produce the HTML coverage report.
"""

import cgi
import io
import logging
import os
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import gcno_parser

GEN_TAG = "/gen/"

def GenerateLineCoverageVector(gcno_file_summary, exclude_paths, coverage_dict):
    """Process the gcno_file_summary and update the coverage dictionary.

    Create a coverage vector for each source file contained in gcno_file_summary
    and update the corresponding item in coverage_dict.

    Args:
        gcno_file_summary: FileSummary object after gcno and gcda files have
                           been parsed.
        exclude_paths: a list of paths should be ignored in the coverage report.
        coverage_dict: a dictionary for each source file and its corresponding
                       coverage vector.
    """
    for ident in gcno_file_summary.functions:
        func = gcno_file_summary.functions[ident]
        file_name = func.src_file_name
        if GEN_TAG in file_name:
            logging.debug("Skip generated source file %s.", file_name)
            continue
        skip_file = False
        for path in exclude_paths:
            if file_name.startswith(path):
                skip_file = True
                break
        if skip_file:
            logging.debug("Skip excluded source file %s.", file_name)
            continue

        src_lines_counts = coverage_dict[file_name] if file_name in coverage_dict else []
        for block in func.blocks:
            for line in block.lines:
                if line > len(src_lines_counts):
                    src_lines_counts.extend([-1] *
                                            (line - len(src_lines_counts)))
                if src_lines_counts[line - 1] < 0:
                    src_lines_counts[line - 1] = 0
                src_lines_counts[line - 1] += block.count
        coverage_dict[file_name] = src_lines_counts


def GetCoverageStats(src_lines_counts):
    """Returns the coverage stats.

    Args:
        src_lines_counts: A list of non-negative integers or -1 representing
                          the number of times the i-th line was executed.
                          -1 indicates a line that is not executable.

    Returns:
        integer, the number of lines instrumented for coverage measurement
        integer, the number of executed or covered lines
    """
    total = 0
    covered = 0
    if not src_lines_counts or not isinstance(src_lines_counts, list):
        logging.error("GetCoverageStats: input invalid.")
        return total, covered

    for line in src_lines_counts:
        if line < 0:
            continue
        total += 1
        if line > 0:
            covered += 1
    return total, covered

