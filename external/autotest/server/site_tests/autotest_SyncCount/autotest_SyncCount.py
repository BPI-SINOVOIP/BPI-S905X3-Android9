# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging

from autotest_lib.client.common_lib import error
from autotest_lib.server import hosts
from autotest_lib.server import test

class autotest_SyncCount(test.test):
  version = 1

  def run_once(self, ntuple):
    for machine in ntuple:
      logging.info('Using host %s', machine)

    if len(ntuple) != 2:
      raise error.TestFail('Expected %s hosts, got %s' % (2, len(ntuple)))
