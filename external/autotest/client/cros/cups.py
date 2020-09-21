# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils as sys_utils
from autotest_lib.client.cros import upstart
"""Provides utility methods for CUPS."""


def has_cups_upstart():
  """Returns True if cups is installed under upstart."""
  return upstart.has_service('cupsd')


def has_cups_systemd():
  """Returns True if cups is running under systemd.

  Attempts to start cups if it is not already running.
  """
  return sys_utils.has_systemd() and (
      (sys_utils.get_service_pid('cups') != 0) or
      (sys_utils.start_service('cups', ignore_status=True) == 0))


def has_cups_or_die():
  """Checks if the cups dameon is installed.  Raises TestNAError if it is not.

  TestNA skips the test.
  """
  if not (has_cups_upstart() or has_cups_systemd()):
    raise error.TestNAError('No cupsd service found')
