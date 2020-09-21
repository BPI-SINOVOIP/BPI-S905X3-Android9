#!/usr/bin/python
#
# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, os
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.power import power_utils

class security_SMMLocked(test.test):
    """
    Verify SMM has SMRAM unmapped and that the SMM registers are locked.
    """
    version = 1
    executable = 'smm'

    def setup(self):
        os.chdir(self.srcdir)
        utils.make(self.executable)

    def run_once(self):
        errors = 0
        cpu_arch = utils.get_cpu_arch()
        if cpu_arch == "arm":
            logging.debug('ok: skipping SMM test for %s.', cpu_arch)
            return

        cpu_arch = power_utils.get_x86_cpu_arch()
        if cpu_arch == 'Stoney':
            # The SMM registers (MSRC001_0112 and MSRC001_0113) can be
            # locked from being altered by setting MSRC001_0015[SmmLock].
            # Bit 0 : 1=SMM code in the ASeg and TSeg range and the SMM
            #         registers are read-only and SMI interrupts are
            #         not intercepted in SVM for Stoney.
            Stoney_SMMLock = {'0xc0010015':  [('0', 1)]}
            self._registers = power_utils.Registers()
            errors = self._registers.verify_msr(Stoney_SMMLock)
        else:
            r = utils.run("%s/%s" % (self.srcdir, self.executable),
                          stdout_tee=utils.TEE_TO_LOGS,
                          stderr_tee=utils.TEE_TO_LOGS,
                          ignore_status=True)
            if r.exit_status != 0 or len(r.stderr) > 0:
                raise error.TestFail(r.stderr)
            if 'skipping' in r.stdout:
                logging.debug(r.stdout)
                return
            if 'ok' not in r.stdout:
                raise error.TestFail(r.stdout)
        if errors:
            logging.error('Test Failed for %s', cpu_arch)
            raise error.TestFail(r.stdout)
