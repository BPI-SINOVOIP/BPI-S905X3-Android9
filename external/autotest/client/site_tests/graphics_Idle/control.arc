# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

AUTHOR = 'chromeos-gfx'
NAME = "graphics_Idle.arc"
ATTRIBUTES = "suite:bvt-arc"
PURPOSE = "Verify that graphics behaves as expected on idle."
CRITERIA = """
This test will fail if we don't see the appropriate GPU idle states.
It depends on the browser not using graphics continuously in guest/kiosk mode.
"""
TIME = "SHORT"
TEST_CATEGORY = "Functional"
TEST_CLASS = "graphics"
TEST_TYPE = "client"
DEPENDENCIES = "arc"
JOB_RETRIES = 2
BUG_TEMPLATE = {
    'labels': ['Cr-OS-Kernel-Graphics'],
}
ARC_MODE = "enabled"

DOC = """
This test checks that the GPU is in the proper state when idle (RC6, panel,
clocks...)
"""

job.run_test('graphics_Idle', arc_mode=ARC_MODE, tag='arc')
