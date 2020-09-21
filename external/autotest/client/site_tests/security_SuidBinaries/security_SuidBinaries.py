# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error

class security_SuidBinaries(test.test):
    """
    Make sure no surprise binaries become setuid, setgid, or gain filesystem
    capabilities without autotest noticing.
    """
    version = 1

    def _load_baseline_file(self, basename):
        """Load the list of expected files from a given file name.

        @param basename the basename of the file to load.
        @returns a set containing the names of the files listed in the baseline
        file.
        """
        path = os.path.join(self.bindir, basename)
        if os.path.exists(path):
            with open(path) as basefile:
                return set(l.strip() for l in basefile if l.strip()[0] != '#')
        return set()


    def _load_baseline(self, bltype):
        """Load the list of expected files for a given baseline type.

        @param bltype the baseline to load.
        @returns a set containing the names of the files in the board's
        baseline.
        """
        # Baseline common to all boards.
        blname = 'baseline.' + bltype
        blset = self._load_baseline_file(blname)
        # Board-specific baseline.
        board_blname = 'baseline.%s.%s' % (utils.get_current_board(), bltype)
        blset |= self._load_baseline_file(board_blname)
        return blset


    def run_once(self, baseline='suid'):
        """
        Do a find on the system for setuid binaries, compare against baseline.
        Fail if setuid binaries are found on the system but not on the baseline.
        """
        exclude = [ '/proc',
                    '/dev',
                    '/sys',
                    '/run',
                    '/usr/local',
                    '/mnt/stateful_partition',
                  ]
        cmd = 'find / '
        for item in exclude:
            cmd += '-wholename %s -prune -o ' % (item)
        cmd += '-type f '

        permmask = {'suid': '4000', 'sgid': '2000'}

        if baseline in permmask:
            cmd += '-a -perm /%s -print' % (permmask[baseline])
        elif baseline == 'fscap':
            cmd += '-exec getcap {} +'
        else:
            raise error.TestFail("Unknown baseline '%s'!" % (baseline))

        cmd_output = utils.system_output(cmd, ignore_status=True)
        observed_set = set(cmd_output.splitlines())
        baseline_set = self._load_baseline(baseline)

        # Report observed set for debugging.
        for line in observed_set:
            logging.debug('%s: %s', baseline, line)

        # Fail if we find new binaries.
        new = observed_set.difference(baseline_set)
        if len(new) > 0:
            message = 'New %s binaries: %s' % (baseline, ', '.join(new))
            raise error.TestFail(message)

        # Log but not fail if we find missing binaries.
        missing = baseline_set.difference(observed_set)
        if len(missing) > 0:
            for filepath in missing:
                logging.error('Missing %s binary: %s', baseline, filepath)
        else:
            logging.debug('OK: %s baseline matches system', baseline)
