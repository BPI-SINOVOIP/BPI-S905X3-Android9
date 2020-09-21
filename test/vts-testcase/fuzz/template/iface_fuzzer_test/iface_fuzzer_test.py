#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
import os

from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.utils.python.controllers import adb
from vts.utils.python.controllers import android_device
from vts.utils.python.common import vts_spec_utils

from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test_config as config
from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test_case
from vts.testcases.fuzz.template.func_fuzzer_test import func_fuzzer_test


class IfaceFuzzerTest(func_fuzzer_test.FuncFuzzerTest):
    """Runs iface_fuzzer tests on target.

    Attributes:
        See func_fuzzer_test.
    """
    _VTS_SPEC_DIR_TARGET = os.path.join(config.FUZZER_TEST_DIR, 'spec')

    def _PushVtsResources(self, hal_name, hal_version):
        """Pushes resources needed for test to target device.

        Args:
            hal_name: string, name of the hal, e.g. 'vibrator'.
            hal_version: string, version of the hal, e.g '7.4'
        """
        # Push .vts spec files.
        hal_name_dir = vts_spec_utils.HalNameDir(hal_name)
        src_dir = os.path.join(self.data_file_path, 'spec', 'hardware',
                               'interfaces', hal_name_dir, hal_version, 'vts')
        dst_dir = os.path.join(self._VTS_SPEC_DIR_TARGET, hal_name_dir,
                               hal_version)

        # Push corresponding VTS drivers.
        driver_name = 'android.hardware.%s@%s-vts.driver.so' % (hal_name,
                                                                hal_version)
        asan_path = os.path.join(self.data_file_path, 'DATA', 'asan', 'system')
        driver32 = os.path.join(asan_path, 'lib', driver_name)
        driver64 = os.path.join(asan_path, 'lib64', driver_name)
        try:
            self._dut.adb.push(src_dir, dst_dir)
            self._dut.adb.push(driver32, 'data/local/tmp/32')
            self._dut.adb.push(driver64, 'data/local/tmp/64')
        except adb.AdbError as e:
            logging.exception(e)

    def _VtsSpecDirsTarget(self, hal_name, hal_version):
        """Returns a list of directories on target with .vts specs.

        Args:
            hal_name: string, name of the hal, e.g. 'vibrator'.
            hal_version: string, version of the hal, e.g '7.4'

        Returns:
            string list, directories on target
        """
        hal_name_dir = vts_spec_utils.HalNameDir(hal_name)
        spec_dirs = [os.path.join(self._VTS_SPEC_DIR_TARGET, hal_name_dir,
                                  hal_version)]

        imported_hals = self._vts_spec_parser.ImportedHals(hal_name,
                                                           hal_version)
        for name, version in imported_hals:
            spec_dirs.append(
                os.path.join(self._VTS_SPEC_DIR_TARGET,
                             vts_spec_utils.HalNameDir(name), version))
        return spec_dirs

    # Override
    def CreateTestCases(self):
        """See base class."""
        hal_package = self.hal_hidl_package_name
        hal_name, hal_version = vts_spec_utils.HalPackageToNameAndVersion(
            hal_package)

        imported_hals = self._vts_spec_parser.IndirectImportedHals(hal_name,
                                                                   hal_version)
        self._PushVtsResources(hal_name, hal_version)
        for name, version in imported_hals:
            self._PushVtsResources(name, version)

        registered_interfaces = self._RegisteredInterfaces(hal_package)
        spec_dirs = ':'.join(self._VtsSpecDirsTarget(hal_name, hal_version))

        test_cases = []
        for iface in registered_interfaces:
            additional_params = {
                'vts_spec_dir': spec_dirs,
                'vts_exec_size': 16,
                'vts_target_iface': iface,
            }
            libfuzzer_params = config.FUZZER_DEFAULT_PARAMS.copy()
            libfuzzer_params.update({
                'max_len': 16777216,
                'max_total_time': 600,
            })
            bin_host_path = os.path.join(self.data_file_path, 'DATA', 'bin',
                                         'vts_proto_fuzzer')
            test_case = libfuzzer_test_case.LibFuzzerTestCase(
                bin_host_path, libfuzzer_params, additional_params)
            test_case.test_name = iface
            test_cases.append(test_case)

        return test_cases

    # Override
    def LogCrashReport(self, test_case):
        """See base class."""
        # Re-run the failing test case in debug mode.
        logging.info('Attempting to reproduce the failure.')
        repro_cmd = '"%s %s"' % (test_case.GetRunCommand(debug_mode=True),
                                 config.FUZZER_TEST_CRASH_REPORT)
        self._dut.adb.shell(repro_cmd, no_except=True)


if __name__ == '__main__':
    test_runner.main()
