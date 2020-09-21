#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import getopt
import os
import sys
import tf_runner
from unittest import loader
import unittest

class TradefedProgram(unittest.TestProgram):
    """ Main Runner Class that should be used to run the tests. This runner ensure that the
    reporting is compatible with Tradefed.
    """

    def __init__(self, module='__main__', defaultTest=None,
                 argv=None, testRunner=None,
                 testLoader=loader.defaultTestLoader, exit=True,
                 verbosity=1, failfast=None, catchbreak=None, buffer=None, serial=None):
      self.serial = None
      self.extra_options = []
      super(TradefedProgram, self).__init__()

    def parseArgs(self, argv):
        if len(argv) > 1 and argv[1].lower() == 'discover':
            self._do_discovery(argv[2:])
            return

        long_opts = ['help', 'verbose', 'quiet', 'failfast', 'catch', 'buffer',
                     'serial=', 'extra_options=']
        try:
            options, args = getopt.getopt(argv[1:], 'hHvqfcbs:e:', long_opts)
            for opt, value in options:
                if opt in ('-h','-H','--help'):
                    self.usageExit()
                if opt in ('-q','--quiet'):
                    self.verbosity = 0
                if opt in ('-v','--verbose'):
                    self.verbosity = 2
                if opt in ('-f','--failfast'):
                    if self.failfast is None:
                        self.failfast = True
                    # Should this raise an exception if -f is not valid?
                if opt in ('-c','--catch'):
                    if self.catchbreak is None and installHandler is not None:
                        self.catchbreak = True
                    # Should this raise an exception if -c is not valid?
                if opt in ('-b','--buffer'):
                    if self.buffer is None:
                        self.buffer = True
                    # Should this raise an exception if -b is not valid?
                if opt in ('-s', '--serial'):
                    if self.serial is None:
                        self.serial = value
                if opt in ('-e', '--extra_options'):
                    self.extra_options.append(value)
            if len(args) == 0 and self.defaultTest is None:
                # createTests will load tests from self.module
                self.testNames = None
            elif len(args) > 0:
                self.testNames = args
                if __name__ == '__main__':
                    # to support python -m unittest ...
                    self.module = None
            else:
                self.testNames = (self.defaultTest,)
            self.createTests()
        except getopt.error, msg:
            self.usageExit(msg)

    def runTests(self):
        if self.testRunner is None:
            self.testRunner = tf_runner.TfTextTestRunner(verbosity=self.verbosity,
                                                         failfast=self.failfast,
                                                         buffer=self.buffer,
                                                         resultclass=tf_runner.TextTestResult,
                                                         serial=self.serial,
                                                         extra_options=self.extra_options)
        super(TradefedProgram, self).runTests()

main = TradefedProgram

def main_run():
    TradefedProgram(module=None)
