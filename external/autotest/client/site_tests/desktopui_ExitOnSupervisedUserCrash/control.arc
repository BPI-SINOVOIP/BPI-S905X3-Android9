# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

AUTHOR = "cmasone, antrim"
NAME = "desktopui_ExitOnSupervisedUserCrash.arc"
ATTRIBUTES = "suite:bvt-arc"
TIME = "SHORT"
TEST_CATEGORY = "General"
TEST_CLASS = "desktopui"
TEST_TYPE = "client"
JOB_RETRIES = 2
DEPENDENCIES = "arc"
ARC_MODE = "enabled"

DOC = """
This test synthetically informs the session_manager that a supervised user
account is being created and verifies that a crash during that operation
triggers session termination.
"""

job.run_test('desktopui_ExitOnSupervisedUserCrash', arc_mode=ARC_MODE, tag='arc')
