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

from vts.runners.host import base_test
from vts.testcases.template.param_test import param_test
from vts.utils.python.controllers import android_device
from vts.utils.python.hal import hal_service_name_utils
from vts.utils.python.precondition import precondition_utils


class HalHidlHostTest(param_test.ParamTestClass):
    """Base class to run a host-driver hidl hal test.

    Attributes:
        dut: AndroidDevice, the device under test as config
        shell: ShellMirrorObject, shell mirror
        TEST_HAL_SERVICES: a set of hal services accessed in the test.
    """
    TEST_HAL_SERVICES = set()

    # @Override
    def initParams(self):
        """Get the service combination according to the registered test HAL."""
        self.dut = self.registerController(android_device)[0]
        self.shell = self.dut.shell
        service_instance_combinations = self._GetServiceInstanceCombinations()
        self.params = service_instance_combinations

    # @Override
    def setUpClass(self):
        """Basic setup process for host-side hidl hal tests.

        Test precondition check, prepare for profiling and coverage measurement
        if enabled.
        """
        # Testability check.
        if not precondition_utils.CanRunHidlHalTest(
                self, self.dut, self.shell, self.run_as_compliance_test):
            self.skipAllTests("precondition check for hidl hal tests didn't pass")
            return

        # Initialization for coverage measurement.
        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.InitializeDeviceCoverage(self.dut)
            if self.TEST_HAL_SERVICES:
                self.coverage.SetHalNames(self.TEST_HAL_SERVICES)
                self.coverage.SetCoverageReportFilePrefix(self.test_module_name + self.abi_bitness)

        # Enable profiling.
        if self.profiling.enabled:
            self.profiling.EnableVTSProfiling(self.shell)

    # @Override
    def tearDownClass(self):
        """Basic cleanup process for host-side hidl hal tests.

        If profiling is enabled for the test, collect the profiling data
        and disable profiling after the test is done.
        If coverage is enabled for the test, collect the coverage data and
        upload it to dashboard.
        """
        if self.isSkipAllTests():
            return

        if self.coverage.enabled and self.coverage.global_coverage:
            self.coverage.SetCoverageData(dut=self.dut, isGlobal=True)

        if self.profiling.enabled:
            self.profiling.ProcessAndUploadTraceData()

    # @Override
    def setUp(self):
        """Setup process for each test case."""
        if self.profiling.enabled:
            self.profiling.EnableVTSProfiling(self.shell)

    # @Override
    def tearDown(self):
        """Cleanup process for each test case."""
        if self.profiling.enabled:
            self.profiling.ProcessTraceDataForTestCase(self.dut)
            self.profiling.DisableVTSProfiling(self.shell)

    # @Override
    def getParamTag(self, param):
        """Concatenate names for all services passed as the param test tag.

        Args:
            param: a list of service instances. e.g [s1/n1, s2/n2]

        Returns:
            a string of concatenated service names. e.g. n1/n2
        """
        names = map(lambda instance: instance.split("/")[1], param)
        return "({})".format(",".join(names))

    def getHalServiceName(self, hal_service):
        """Get corresponding name for hal_service from the current parameter.

        The current parameter should be a list of service instances with the
        format [hal_service/name], e.g [s1/n1, s2/n2]

        Args:
            hal_service: string, hal@version e.g. foo@1.0

        Returns:
            Name for hal_service, "default" if could not find the hal_service in
            the list of service instances.
        """
        for instance in self.cur_param:
            service, name = instance.split("/")
            if service == hal_service:
                return str(name)
        # In case could not find the name for given hal_service, fall back to
        # use the "default" name.
        logging.warning(
            "Could not find the service name for %s, using default name instead",
            hal_service)
        return "default"

    def _GetServiceInstanceCombinations(self):
        """Create combinations of instances for registered HAL services.

        Returns:
            A list of instance combination for registered HAL.
        """
        registered_services = copy.copy(self.TEST_HAL_SERVICES)
        service_instances = {}

        for service in registered_services:
            testable, service_names = hal_service_name_utils.GetHalServiceName(
                self.shell, service, self.abi_bitness,
                self.run_as_compliance_test)
            if not testable:
                self.skipAllTests("Hal: %s is not testable, "
                                  "skip all tests." % service)
                return []
            if service_names:
                service_instances[service] = service_names
            else:
                self.skipAllTests("No service name found for: %s, "
                                  "skip all tests." % service)
                return []
        logging.info("registered service instances: %s", service_instances)

        return hal_service_name_utils.GetServiceInstancesCombinations(
                registered_services, service_instances)
