#!/usr/bin/python
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging, optparse, os, shutil, re, string
from autotest_lib.client.common_lib import error
from autotest_lib.client.bin import utils, test

class kernel_Bootcache(test.test):
    """Run the boot cache test on 3.8 Kernel version
    """
    version = 1
    KERNEL_VER = '3.8'
    Bin = '/usr/local/opt/punybench/bin/'


    def initialize(self):
        self.results = []
        self.job.drop_caches_between_iterations = True


    def _run(self, cmd, args):
        """Run a puny test

        Prepends the path to the puny benchmark bin.

        Args:
          cmd: command to be run
          args: arguments for the command
        """
        result = utils.system_output(
            os.path.join(self.Bin, cmd) + ' ' + args)
        logging.debug(result)
        return result


    def run_once(self, args=[]):
        kernel_ver = os.uname()[2]
        if utils.compare_versions(kernel_ver, self.KERNEL_VER) != 0:
            raise error.TestNAError('Applicable to 3.8 Kernel only')

        """Run the boot cache test.
        """
        self._run('bootcachetest', "")
