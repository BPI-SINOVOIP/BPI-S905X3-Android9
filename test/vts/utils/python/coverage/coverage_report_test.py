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

from vts.utils.python.coverage import gcno_parser
from vts.utils.python.coverage import gcda_parser
from vts.utils.python.coverage import coverage_report


class CoverageReportTest(unittest.TestCase):
    """Unit tests for CoverageReport of vts.utils.python.coverage.
    """

    GOLDEN_GCNO_PATH = 'testdata/sample.gcno'
    GOLDEN_GCDA_PATH = 'testdata/sample.gcda'

    @classmethod
    def setUpClass(cls):
        dir_path = os.path.dirname(os.path.realpath(__file__))
        gcno_path = os.path.join(dir_path, cls.GOLDEN_GCNO_PATH)
        with open(gcno_path, 'rb') as file:
            gcno_summary = gcno_parser.GCNOParser(file).Parse()
        gcda_path = os.path.join(dir_path, cls.GOLDEN_GCDA_PATH)
        with open(gcda_path, 'rb') as file:
            parser = gcda_parser.GCDAParser(file)
            parser.Parse(gcno_summary)
        cls.gcno_summary = gcno_summary

    def testGenerateLineCoverageVector(self):
        """Tests that coverage vector is correctly generated.

        Runs GenerateLineCoverageVector on sample file and checks
        result.
        """
        coverage_dict = dict()
        exclude_paths = []
        src_lines_counts = coverage_report.GenerateLineCoverageVector(
            self.gcno_summary, exclude_paths, coverage_dict)
        expected = {'sample.c': [-1, -1, -1, -1, 2, -1, -1, -1, -1, -1, 2,
                    2, 2, -1, 2, -1, 2, 0, -1, 2, -1, -1, 2, 2, 502,
                    500, -1, -1, 2, -1, 2, -1, -1, -1, 2, -1,
                    -1, -1, -1, 2, 2, 2]}
        self.assertEqual(coverage_dict, expected)


if __name__ == "__main__":
    unittest.main()
