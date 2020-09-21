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

import json
import logging

from vts.runners.host import asserts
from vts.runners.host import const

VTS_TESTABILITY_CHECKER_32 = "/data/local/tmp/vts_testability_checker32"
VTS_TESTABILITY_CHECKER_64 = "/data/local/tmp/vts_testability_checker64"


def GetHalServiceName(shell, hal, bitness="64", run_as_compliance_test=False):
    """Determine whether to run a VTS test against a HAL and get the service
    names of the given hal if determine to run.

    Args:
        shell: the ShellMirrorObject to execute command on the device.
        hal: string, the FQName of a HAL service, e.g.,
                     android.hardware.foo@1.0::IFoo
        bitness: string, the bitness of the test.
        run_as_compliance_test: boolean, whether it is a compliance test.

    Returns:
        a boolean whether to run the test against the given hal.
        a set containing all service names for the given HAL.
    """

    binary = VTS_TESTABILITY_CHECKER_64
    if bitness == "32":
        binary = VTS_TESTABILITY_CHECKER_32
    # Give permission to execute the binary.
    shell.Execute("chmod 755 %s" % binary)
    cmd = binary
    if run_as_compliance_test:
        cmd += " -c "
    cmd += " -b " + bitness + " " + hal
    cmd_results = shell.Execute(str(cmd))
    asserts.assertFalse(
        any(cmd_results[const.EXIT_CODE]),
        "Failed to run vts_testability_checker. ErrorCode: %s\n STDOUT: %s\n STDERR: %s\n"
        % (cmd_results[const.EXIT_CODE][0], cmd_results[const.STDOUT][0],
           cmd_results[const.STDERR][0]))
    result = json.loads(cmd_results[const.STDOUT][0])
    if str(result['Testable']).lower() == "true":
        return True, set(result['instances'])
    else:
        return False, ()


class CombMode(object):
    """Enum for service name combination mode"""
    FULL_PERMUTATION = 0
    NAME_MATCH = 1
    NO_COMBINATION = 2


def GetServiceInstancesCombinations(services,
                                    service_instances,
                                    mode=CombMode.FULL_PERMUTATION):
    """Create combinations of instances for all services.

    Args:
        services: list, all services used in the test. e.g. [s1, s2]
        service_instances: dictionary, mapping of each service and the
                           corresponding service name(s).
                           e.g. {"s1": ["n1"], "s2": ["n2", "n3"]}
        mode: CombMode that determines the combination strategy.

    Returns:
        A list of service instance combinations.
    """
    if mode == CombMode.FULL_PERMUTATION:
        return GetServiceInstancesFullCombinations(services, service_instances)
    elif mode == CombMode.NAME_MATCH:
        return GetServiceInstancesNameMatchCombinations(
            services, service_instances)
    else:
        logging.warning("Unknown comb mode, use default comb mode instead.")
        return GetServiceInstancesFullCombinations(services, service_instances)


def GetServiceInstancesFullCombinations(services, service_instances):
    """Create all combinations of instances for all services.

    Create full permutation for registered service instances, e.g.
    s1 have instances (n1, n2) and s2 have instances (n3, n4), return all
    permutations of s1 and s2, i.e. (s1/n1, s2/n3), (s1/n1, s2/n4),
    (s1/n2, s2/n3) and (s1/n2, s2/n4)

    Args:
        services: list, all services used in the test. e.g. [s1, s2]
        service_instances: dictionary, mapping of each service and the
                           corresponding service name(s).
                           e.g. {"s1": ["n1"], "s2": ["n2", "n3"]}

    Returns:
        A list of all service instance combinations.
        e.g. [[s1/n1, s2/n2], [s1/n1, s2/n3]]
    """
    service_instance_combinations = []
    if not services or (service_instances and type(service_instances) != dict):
        return service_instance_combinations
    service = services.pop()
    pre_instance_combs = GetServiceInstancesCombinations(
        services, service_instances)
    if service not in service_instances or not service_instances[service]:
        return pre_instance_combs
    for name in service_instances[service]:
        if not pre_instance_combs:
            new_instance_comb = [service + '/' + name]
            service_instance_combinations.append(new_instance_comb)
        else:
            for instance_comb in pre_instance_combs:
                new_instance_comb = [service + '/' + name]
                new_instance_comb.extend(instance_comb)
                service_instance_combinations.append(new_instance_comb)

    return service_instance_combinations


def GetServiceInstancesNameMatchCombinations(services, service_instances):
    """Create service instance combinations with same name for services.

    Create combinations for services with the same instance name, e.g.
    both s1 and s2 have two instances with name n1, and n2, return
    the service instance combination of s1 and s2 with same instance name,
    i.e. (s1/n1, s2/n1) and (s1/n2, s2/n2)

    Args:
        services: list, all services used in the test. e.g. [s1, s2]
        service_instances: dictionary, mapping of each service and the
                           corresponding service name(s).
                           e.g. {"s1": ["n1", "n2"], "s2": ["n1", "n2"]}

    Returns:
        A list of service instance combinations.
        e.g. [[s1/n1, s2/n1], [s1/n2, s2/n2]]
    """
    service_instance_combinations = []
    instance_names = set()
    for service in services:
        if service not in service_instances or not service_instances[service]:
            logging.error("Does not found instance for service: %s", service)
            return []
        if not instance_names:
            instance_names = set(service_instances[service])
        else:
            instance_names.intersection_update(set(service_instances[service]))

    for name in instance_names:
        instance_comb = []
        for service in services:
            new_instance = [service + '/' + name]
            instance_comb.extend(new_instance)
        service_instance_combinations.append(instance_comb)

    return service_instance_combinations
