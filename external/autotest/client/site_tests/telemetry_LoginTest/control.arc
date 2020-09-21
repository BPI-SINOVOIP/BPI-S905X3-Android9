# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

AUTHOR = "Chrome OS Lab Infrastructure Team"
NAME = "telemetry_LoginTest.arc"
PURPOSE = "Login through telemetry."
CRITERIA = """
This test will fail if telemetry login is broken, or if
the autotest extension doesn't load/is inaccessible, or if
chrome.autotestPrivate.loginStatus doesn't work.
"""
ATTRIBUTES = "suite:bvt-arc"
TIME = "SHORT"
TEST_CATEGORY = "Functional"
TEST_CLASS = "login"
TEST_TYPE = "client"
DEPENDENCIES = "arc"
JOB_RETRIES = 2
ARC_MODE = "enabled"

DOC = """
This is a test that checks that login completed successfully through telemetry,
and exercises the extension api chrome.autotestPrivate.
"""

job.run_test('telemetry_LoginTest', arc_mode=ARC_MODE, tag='arc')
