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
from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test
from vts.testcases.fuzz.template.libfuzzer_test import libfuzzer_test_case


class FuncFuzzerTest(libfuzzer_test.LibFuzzerTest):
    """Runs function fuzzer tests on target.

    Attributes:
        _dut: AndroidDevice, the device under test as config.
        _test_cases: LibFuzzerTestCase list, list of test cases to run.
        _vts_spec_parser: VtsSpecParser, used to parse .vts files.
    """

    def setUpClass(self):
        """Creates a remote shell instance, and copies data files."""
        required_params = [
            keys.ConfigKeys.IKEY_DATA_FILE_PATH,
            keys.ConfigKeys.IKEY_HAL_HIDL_PACKAGE_NAME,
        ]
        self.getUserParams(required_params)
        logging.info('%s: %s', keys.ConfigKeys.IKEY_DATA_FILE_PATH,
                     self.data_file_path)
        logging.info('%s: %s', keys.ConfigKeys.IKEY_HAL_HIDL_PACKAGE_NAME,
                     self.hal_hidl_package_name)

        self._dut = self.registerController(android_device, False)[0]
        self._dut.startAdbLogcat()
        self._dut.adb.shell('mkdir %s -p' % config.FUZZER_TEST_DIR)
        self._vts_spec_parser = vts_spec_utils.VtsSpecParser(
            self.data_file_path)

    def _RegisteredInterfaces(self, hal_package):
        """Returns a list of registered interfaces for a given hal package.

        Args:
            hal_package: string, name of hal package,
                e.g. android.hardware.nfc@1.0

        Returns:
            list of string, list of interfaces from this package that are
                registered on device under test.
        """
        # TODO: find a more robust way to query registered interfaces.
        cmd = '"lshal | grep -v \* | grep -o %s::[[:alpha:]]* | sort -u"' % hal_package
        out = str(self._dut.adb.shell(cmd)).split()
        interfaces = map(lambda x: x.split('::')[-1], out)
        return interfaces

    def _FuzzerBinHostPath(self, hal_package, vts_spec_name):
        """Returns path to fuzzer binary on host."""
        vts_spec_name = vts_spec_name.replace('.vts', '')
        bin_name = hal_package + '-vts.func_fuzzer.' + vts_spec_name
        bin_host_path = os.path.join(self.data_file_path, 'DATA', 'bin',
                                       bin_name)
        return str(bin_host_path)

    def _CreateTestCasesFromSpec(self, hal_package, vts_spec_name,
                                 vts_spec_proto):
        """Creates LibFuzzerTestCases.

        Args:
            hal_package: string, name of hal package,
                e.g. android.hardware.nfc@1.0
            vts_spec_name: string, e.g. 'Nfc.vts'.

        Returns:
            LibFuzzerTestCase list, one per function of interface corresponding
                to vts_spec_name.
        """
        test_cases = []
        for api in vts_spec_proto.interface.api:
            additional_params = {'vts_target_func': api.name}
            libfuzzer_params = config.FUZZER_DEFAULT_PARAMS
            bin_host_path = self._FuzzerBinHostPath(hal_package, vts_spec_name)
            test_case = libfuzzer_test_case.LibFuzzerTestCase(
                bin_host_path, libfuzzer_params, additional_params)
            test_case.test_name = api.name
            test_cases.append(test_case)
        return test_cases

    # Override
    def CreateTestCases(self):
        """See base class."""
        hal_package = self.hal_hidl_package_name
        hal_name, hal_version = vts_spec_utils.HalPackageToNameAndVersion(
            hal_package)
        vts_spec_names = self._vts_spec_parser.VtsSpecNames(hal_name,
                                                            hal_version)

        registered_interfaces = self._RegisteredInterfaces(
            self.hal_hidl_package_name)
        test_cases = []
        for vts_spec_name in vts_spec_names:
            vts_spec_proto = self._vts_spec_parser.VtsSpecProto(
                hal_name, hal_version, vts_spec_name)
            if not vts_spec_proto.component_name in registered_interfaces:
                continue
            test_cases += self._CreateTestCasesFromSpec(
                hal_package, vts_spec_name, vts_spec_proto)
        return test_cases


if __name__ == '__main__':
    test_runner.main()
