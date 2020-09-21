# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
This module contains the status enums for use by Hosts in the
database. It is a stand alone module as these status strings
are need from vairous disconnected pieces of code.
"""

from autotest_lib.client.common_lib import enum

Status = enum.Enum('Verifying', 'Running', 'Ready', 'Repairing',
'Repair Failed', 'Cleaning', 'Pending', 'Resetting',
'Provisioning', string_values=True)

# States associated with a DUT that is doing nothing, whether or not
# it's eligible to run a test.
IDLE_STATES = {Status.READY, Status.REPAIR_FAILED}

# States associated with a DUT that is not available for jobs.  Note that a
# locked host is also unavailable no matter the status.
UNAVAILABLE_STATES = {Status.REPAIR_FAILED, Status.REPAIRING, Status.VERIFYING}
