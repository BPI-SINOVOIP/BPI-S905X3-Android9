# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import arc
from autotest_lib.client.cros.graphics import graphics_utils

_CROS_BIN_DIR = '/usr/local/bin/'
_SDCARD_DIR = '/sdcard/'
_EXEC_DIR = '/data/executables/'
_TEST_COMMAND = 'test -e'
_POSSIBLE_BINARIES = ['gralloctest_amd64', 'gralloctest_arm', 'gralloctest_x86']

# The tests still can be run individually, though we run with the 'all' option
# Run ./gralloctest in Android to get a list of options.
_OPTION = 'all'

# GraphicsTest should go first as it will pass initialize/cleanup function
# to ArcTest. GraphicsTest initialize would not be called if ArcTest goes first
class graphics_Gralloc(graphics_utils.GraphicsTest, arc.ArcTest):
    """gralloc test."""
    version = 1
    _executables = []

    def arc_setup(self):
        super(graphics_Gralloc, self).arc_setup()
        # Get the executable from CrOS and copy it to Android container. Due to
        # weird permission issues inside the container, we first have to copy
        # the test to /sdcard/, then move it to a /data/ subdirectory we create.
        # The permissions on the exectuable have to be modified as well.
        arc.adb_root()
        arc._android_shell('mkdir -p %s' % (_EXEC_DIR))
        for binary in _POSSIBLE_BINARIES:
            cros_path = os.path.join(_CROS_BIN_DIR, binary)
            cros_cmd = '%s %s' % (_TEST_COMMAND, cros_path)
            job = utils.run(cros_cmd, ignore_status=True)
            if job.exit_status:
                continue

            sdcard_path = os.path.join(_SDCARD_DIR, binary)
            arc.adb_cmd('-e push %s %s' % (cros_path, sdcard_path))

            exec_path = os.path.join(_EXEC_DIR, binary)
            arc._android_shell('mv %s %s' % (sdcard_path, exec_path))
            arc._android_shell('chmod o+rwx %s' % (exec_path))
            self._executables.append(exec_path)

    def arc_teardown(self):
        for executable in self._executables:
        # Remove test contents from Android container.
            arc._android_shell('rm -rf %s' % (executable))
        super(graphics_Gralloc, self).arc_teardown()

    def run_once(self):
        gpu_family = utils.get_gpu_family()
        if not self._executables:
            raise error.TestFail('Failed: No executables found on %s' %
                                  gpu_family)

        logging.debug('Running %d executables', len(self._executables))
        for executable in self._executables:
            try:
                cmd = '%s %s' % (executable, _OPTION)
                stdout = arc._android_shell(cmd)
            except Exception:
                logging.error('Exception running %s', cmd)
                raise error.TestFail('Failed: gralloc on %s' % gpu_family)
            # Look for the regular expression indicating failure.
            for line in stdout.splitlines():
                match = re.search(r'\[  FAILED  \]', stdout)
                if match:
                    self.add_failures(line)
                    logging.error(line)
                else:
                    logging.debug(stdout)

        if self.get_failures():
            raise error.TestFail('Failed: gralloc on %s in %s.' %
                                 (gpu_family, self.get_failures()))
