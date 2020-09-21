# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import time
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.graphics import graphics_utils


class graphics_KernelMemory(graphics_utils.GraphicsTest):
    """
    Reads from sysfs to determine kernel gem objects and memory info.
    """
    version = 1

    def initialize(self):
        super(graphics_KernelMemory, self).initialize()

    @graphics_utils.GraphicsTest.failure_report_decorator('graphics_KernelMemory')
    def run_once(self):
        # TODO(ihf): We want to give this test something well-defined to
        # measure. For now that will be the CrOS login-screen memory use.
        # We could also log into the machine using telemetry, but that is
        # still flaky. So for now we, lame as we are, just sleep a bit.
        time.sleep(10.0)

        self._GSC.finalize()
        # We should still be in the login screen and memory use > 0.
        if self._GSC.get_memory_access_errors() > 0:
            raise error.TestFail('Failed: Detected %d errors accessing graphics'
                                 ' memory.' % self.GKM.num_errors)
