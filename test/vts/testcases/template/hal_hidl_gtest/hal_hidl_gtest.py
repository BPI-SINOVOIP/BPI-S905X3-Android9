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

import copy
import logging

from vts.runners.host import const
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.gtest_binary_test import gtest_binary_test
from vts.testcases.template.gtest_binary_test import gtest_test_case
from vts.utils.python.cpu import cpu_frequency_scaling
from vts.utils.python.hal import hal_service_name_utils
from vts.utils.python.precondition import precondition_utils

# The pattern indicating a full hal test name including the service name info.
# e.g. TM.TC(default)_32bit
_HAL_TEST_NAME_PATTERN = ".*\(.*\).*"

class HidlHalGTest(gtest_binary_test.GtestBinaryTest):
    """Base class to run a VTS target-side HIDL HAL test.

    Attributes:
        DEVICE_TEST_DIR: string, temp location for storing binary
        TAG_PATH_SEPARATOR: string, separator used to separate tag and path
        tags: all the tags that appeared in binary list
        testcases: list of GtestTestCase objects, list of test cases to run
        _cpu_freq: CpuFrequencyScalingController instance of a target device.
        _dut: AndroidDevice, the device under test as config
        _target_hals: List of String, the targeting hal service of the test.
                      e.g (["android.hardware.foo@1.0::IFoo"])
    """

    def setUpClass(self):
        """Checks precondition."""
        super(HidlHalGTest, self).setUpClass()
        if not hasattr(self, "_target_hals"):
            self._target_hals = []

        opt_params = [keys.ConfigKeys.IKEY_SKIP_IF_THERMAL_THROTTLING,
                      keys.ConfigKeys.IKEY_DISABLE_CPU_FREQUENCY_SCALING]
        self.getUserParams(opt_param_names=opt_params)

        self._skip_if_thermal_throttling = self.getUserParam(
            keys.ConfigKeys.IKEY_SKIP_IF_THERMAL_THROTTLING,
            default_value=False)
        self._disable_cpu_frequency_scaling = self.getUserParam(
            keys.ConfigKeys.IKEY_DISABLE_CPU_FREQUENCY_SCALING,
            default_value=True)

        if not self.isSkipAllTests():
            self._cpu_freq = cpu_frequency_scaling.CpuFrequencyScalingController(
                self._dut)
            if self._disable_cpu_frequency_scaling:
                logging.info("Disable CPU frequency scaling")
                self._cpu_freq.DisableCpuScaling()
        else:
            self._cpu_freq = None

        if not self.isSkipAllTests():
            ret = precondition_utils.CanRunHidlHalTest(
                self, self._dut, self.shell, self.run_as_compliance_test)
            if not ret:
                self.skipAllTests("HIDL HAL precondition check failed.")

        if self.sancov.enabled and self._target_hals:
            self.sancov.InitializeDeviceCoverage(self._dut,
                                                 self._target_hals)
        if self.coverage.enabled and self._target_hals:
            self.coverage.SetHalNames(self._target_hals)
            self.coverage.SetCoverageReportFilePrefix(self.test_module_name + self.abi_bitness)

    def CreateTestCases(self):
        """Create testcases and conditionally enable passthrough mode.

        Create testcases as defined in HidlHalGtest. If the passthrough option
        is provided in the configuration or if coverage is enabled, enable
        passthrough mode on the test environment.
        """
        super(HidlHalGTest, self).CreateTestCases()

        passthrough_opt = self.getUserParam(
            keys.ConfigKeys.IKEY_PASSTHROUGH_MODE, default_value=False)

        # Enable coverage if specified in the configuration.
        if passthrough_opt:
            self._EnablePassthroughMode()

    # @Override
    def CreateTestCase(self, path, tag=''):
        """Create a list of GtestTestCase objects from a binary path.

        Support testing against different service names by first executing a
        dummpy test case which lists all the registered hal services. Then
        query the service name(s) for each registered service with lshal.
        For each service name, create a new test case each with the service
        name as an additional argument.

        Args:
            path: string, absolute path of a gtest binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of GtestTestCase objects.
        """
        initial_test_cases = super(HidlHalGTest, self).CreateTestCase(path,
                                                                      tag)
        if not initial_test_cases:
            return initial_test_cases
        # first, run one test with --list_registered_services.
        list_service_test_case = copy.copy(initial_test_cases[0])
        list_service_test_case.args += " --list_registered_services"
        results = self.shell.Execute(list_service_test_case.GetRunCommand())
        if (results[const.EXIT_CODE][0]):
            logging.error("Failed to list test cases from binary %s",
                          list_service_test_case.path)
        # parse the results to get the registered service list.
        registered_services = []
        comb_mode = hal_service_name_utils.CombMode.FULL_PERMUTATION
        # TODO: consider to use a standard data format (e.g. json) instead of
        # parsing the print output.
        for line in results[const.STDOUT][0].split("\n"):
            line = str(line)
            if line.startswith("hal_service: "):
                service = line[len("hal_service: "):]
                registered_services.append(service)
            if line.startswith("service_comb_mode: "):
                comb_mode = int(line[len("service_comb_mode: "):])

        # If no service registered, return the initial test cases directly.
        if not registered_services:
            logging.error("No hal service registered.")
            return initial_test_cases

        self._target_hals = copy.copy(registered_services)

        # find the correponding service name(s) for each registered service and
        # store the mapping in dict service_instances.
        service_instances = {}
        for service in registered_services:
            testable, service_names = hal_service_name_utils.GetHalServiceName(
                self.shell, service, self.abi_bitness,
                self.run_as_compliance_test)
            if not testable:
                self.skipAllTests("Hal: %s is not testable, "
                                  "skip all tests." % service)
                return initial_test_cases
            if not service_names:
                self.skipAllTests("No service name found for: %s, skip all tests." % service)
                # If any of the test services are not available, return the
                # initial test cases directly.
                return initial_test_cases
            else:
                service_instances[service] = service_names
        logging.info("registered service instances: %s", service_instances)
        logging.info("service comb mode: %d", comb_mode)

        # If request NO_COMBINATION mode, return the initial test cases directly.
        if comb_mode == hal_service_name_utils.CombMode.NO_COMBINATION:
            return initial_test_cases

        # get all the combination of service instances.
        service_instance_combinations = hal_service_name_utils.GetServiceInstancesCombinations(
            registered_services, service_instances, comb_mode);

        new_test_cases = []
        appendix_list = []
        for instance_combination in service_instance_combinations:
            for test_case in initial_test_cases:
                new_test_case = copy.copy(test_case)
                service_name_list = []
                for instance in instance_combination:
                    new_test_case.args += " --hal_service_instance=" + instance
                    service_name_list.append(instance[instance.find('/')+1:])
                name_appendix = "({0})".format(",".join(service_name_list))
                new_test_case.name_appendix = name_appendix
                new_test_cases.append(new_test_case)
            appendix_list.append(name_appendix)
        self.test_filter.ExpandAppendix(appendix_list, _HAL_TEST_NAME_PATTERN)
        return new_test_cases

    def _EnablePassthroughMode(self):
        """Enable passthrough mode by setting getStub to true.

        This funciton should be called after super class' setupClass method.
        If called before setupClass, user_params will be changed in order to
        trigger setupClass method to invoke this method again.
        """
        if self.testcases:
            for test_case in self.testcases:
                envp = ' %s=true' % const.VTS_HAL_HIDL_GET_STUB
                test_case.envp += envp
        else:
            logging.warn('No test cases are defined yet. Maybe setupClass '
                         'has not been called. Changing user_params to '
                         'enable passthrough mode option.')
            self.user_params[keys.ConfigKeys.IKEY_PASSTHROUGH_MODE] = True

    def setUp(self):
        """Skips the test case if thermal throttling lasts for 30 seconds."""
        super(HidlHalGTest, self).setUp()

        if (self._skip_if_thermal_throttling and
                getattr(self, "_cpu_freq", None)):
            self._cpu_freq.SkipIfThermalThrottling(retry_delay_secs=30)

    def tearDown(self):
        """Skips the test case if there is thermal throttling."""
        if (self._skip_if_thermal_throttling and
                getattr(self, "_cpu_freq", None)):
            self._cpu_freq.SkipIfThermalThrottling()

        super(HidlHalGTest, self).tearDown()

    def tearDownClass(self):
        """Turns off CPU frequency scaling."""
        if (not self.isSkipAllTests() and getattr(self, "_cpu_freq", None)
            and self._disable_cpu_frequency_scaling):
            logging.info("Enable CPU frequency scaling")
            self._cpu_freq.EnableCpuScaling()

        if self.sancov.enabled and self._target_hals:
            self.sancov.FlushDeviceCoverage(self._dut, self._target_hals)
            self.sancov.ProcessDeviceCoverage(self._dut,
                                              self._target_hals)
            self.sancov.Upload()

        super(HidlHalGTest, self).tearDownClass()


if __name__ == "__main__":
    test_runner.main()
