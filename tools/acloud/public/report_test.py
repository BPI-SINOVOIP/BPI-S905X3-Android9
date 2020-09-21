#!/usr/bin/env python
#
# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Tests for acloud.public.report."""

import unittest
from acloud.public import report


class ReportTest(unittest.TestCase):
    """Test Report class."""

    def testAddData(self):
        """test AddData."""
        r = report.Report("create")
        r.AddData("devices", {"instance_name": "instance_1"})
        r.AddData("devices", {"instance_name": "instance_2"})
        expected = {
            "devices": [
                {"instance_name": "instance_1"},
                {"instance_name": "instance_2"}
            ]
        }
        self.assertEqual(r.data, expected)

    def testAddError(self):
        """test AddError."""
        r = report.Report("create")
        r.errors.append("some errors")
        r.errors.append("some errors")
        self.assertEqual(r.errors, ["some errors", "some errors"])

    def testSetStatus(self):
        """test SetStatus."""
        r = report.Report("create")
        r.SetStatus(report.Status.SUCCESS)
        self.assertEqual(r.status, "SUCCESS")

        r.SetStatus(report.Status.FAIL)
        self.assertEqual(r.status, "FAIL")

        r.SetStatus(report.Status.BOOT_FAIL)
        self.assertEqual(r.status, "BOOT_FAIL")

        # Test that more severe status won't get overriden.
        r.SetStatus(report.Status.FAIL)
        self.assertEqual(r.status, "BOOT_FAIL")


if __name__ == "__main__":
    unittest.main()
