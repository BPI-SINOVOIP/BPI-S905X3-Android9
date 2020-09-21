#
# Copyright (C) 2016 The Android Open Source Project
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

class LinuxKselftestTestcase(object):
    """Stores information needed to run the test case.

    Attrubutes:
        _testname: string, name of the testcase.
        _testsuite: string, name of suite this testcase belongs to.
        _test_cmd: string, shell command used to invoke this testcase.
        _supported_arch: string list, architectures this testcase supports,
            e.g. ["arm", "x86"].
        _supported_bits: int list, bit version (32 or 64) of the testcase that
            are currently supported, e.g. [32, 64].
    """
    def __init__(self, testsuite, test_cmd, supported_arch, supported_bits):
        self._testsuite = testsuite
        self._testname = "%s/%s" % (testsuite, test_cmd)
        self._test_cmd = "./%s" % test_cmd
        self._supported_arch = supported_arch
        self._supported_bits = supported_bits

    @property
    def testname(self):
        """Get test name."""
        return self._testname

    @property
    def testsuite(self):
        """Get test suite."""
        return self._testsuite

    @property
    def test_cmd(self):
        """Get test command."""
        return self._test_cmd

    @property
    def supported_arch(self):
        """Get list of architectures this test can run against."""
        return self._supported_arch

    @property
    def supported_bits(self):
        """Get list of versions (32 or 64 bit) of the test case that can run."""
        return self._supported_bits

    def IsRelevant(self, cpu_abi, n_bit):
        """Checks whether this test case can run in n_bit against this cpu_abi.

        Returns:
            True if this testcase can run; False otherwise.
        """
        if cpu_abi is None or n_bit is None:
            return False

        if not n_bit in self._supported_bits:
            return False

        for arch in self._supported_arch:
            if cpu_abi.find(arch) >= 0:
                return True

        return False

