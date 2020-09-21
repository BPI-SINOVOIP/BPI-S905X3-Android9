#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google, and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import logging
from conf import LisaLogging
LisaLogging.setup()

import json
import os
import devlib
from env import TestEnv
import argparse

# Setup target configuration
conf = {
    # Target platform and board
    "platform"     : 'android',
    # Useful for reading names of little/big cluster
    # and energy model info, its device specific and use
    # only if needed for analysis
    # "board"        : 'pixel',
    # Device
    # By default the device connected is detected, but if more than 1
    # device, override the following to get a specific device.
    # "device"       : "HT66N0300080",
    # Folder where all the results will be collected
    "results_dir" : "BinderThroughputTest",
    # Define devlib modules to load
    "modules"     : [
        'cpufreq',      # enable CPUFreq support
        'cpuidle',      # enable cpuidle support
        # 'cgroups'     # Enable for cgroup support
    ],
    "emeter" : {
        'instrument': 'monsoon',
        'conf': { }
    },
    "systrace": {
        'extra_categories': ['binder_driver'],
        "extra_events": ["binder_transaction_alloc_buf"],
    },
    # Tools required by the experiments
    "tools"   : [ 'taskset'],
}

te = TestEnv(conf, wipe=False)
target = te.target

def experiment(workers, payload, iterations, cpu):
    target.cpufreq.set_all_governors("performance")
    cmd = "taskset -a {} /data/local/tmp/binderThroughputTest -p ".format(cpu)
    cmd += "-s {} -i {} -w {}".format(payload, iterations, workers)
    return target.execute(cmd)

parser = argparse.ArgumentParser(
        description="Run binderThroughputTest and collect output.")

parser.add_argument("--test", "-t", type=str,
                    choices=["cs", "payload"],
                    default="cs",
                    help="cs: vary number of cs_pairs while control payload.\n"
                    "payload: vary payload size while control cs_pairs.")
parser.add_argument("--out_prefix", "-o", type=str,
                    help="The kernel image used for running the test.")
parser.add_argument("--cpu", "-c", type=str,
                    default='f',
                    help="cpus on which to run the tests.")
parser.add_argument("--iterations", "-i", type=int,
                    default=10000,
                    help="Number of iterations to run the transaction.")

parser.add_argument("--cs_pairs", type=int, nargs='+',
                    default=[1, 2, 3, 4, 5],
                    help="Varying client-server pairs.")
parser.add_argument("--payload", type=int, default=0,
                    help="Fixed payload in varying cs pairs.")

parser.add_argument("--payloads", type=int, nargs='+',
                    default=[0, 4096, 4096*2, 4096*4, 4096*8],
                    help="Varying payloads.")
parser.add_argument("--cs_pair", type=int, default=1,
                    help="Fixed single cs pair in varying payload.")

if __name__ == "__main__":
    args = parser.parse_args()

    results = []
    if args.test == "cs":
        for cs in args.cs_pairs:
            result = experiment(cs*2, args.payload, args.iterations, args.cpu)
            results.append(result)

        out_file = os.path.join(te.res_dir,
                                args.out_prefix + "_cs_" + str(args.payload))
        with open(out_file, 'w') as f:
            for r in results:
                f.write("%s\n" % r)
    else:
        for pl in args.payloads:
            result = experiment(args.cs_pair*2, pl, args.iterations, args.cpu)
            results.append(result)

        out_file = os.path.join(te.res_dir,
                                args.out_prefix + \
                                "_payload_" + str(args.cs_pair))
        with open(out_file, 'w') as f:
            for r in results:
                f.write("%s\n" % r)
