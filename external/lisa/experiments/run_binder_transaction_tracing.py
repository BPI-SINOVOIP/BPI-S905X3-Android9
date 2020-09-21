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

from time import sleep
import os
import re
import argparse
import pandas as pd
import matplotlib.pyplot as plt
from android import System
from env import TestEnv

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
    "results_dir" : "BinderTransactionTracing",
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

    "skip_nrg_model" : True,
}

te = TestEnv(conf, wipe=False)
target = te.target

def run_page_stats(duration, frequency):
    procs = {}
    for i in range(int(duration/frequency)):
        ss = target.execute("cat /d/binder/stats")
        proc_dump = re.split("\nproc ", ss)[1:]

        for proc in proc_dump[1:]:
            lines = proc.split("\n  ")
            proc_id = lines[0]
            page = re.search("pages: (\d+:\d+:\d+)", proc)
            active, lru, free = map(int, page.group(1).split(":"))
            if proc_id not in procs:
                procs[proc_id] = {"alloc": [], "lru": []}
            procs[proc_id]["alloc"].append(active + lru)
            procs[proc_id]["lru"].append(lru)

        sleep(frequency)
    for proc in procs:
        df = pd.DataFrame(data={"alloc": procs[proc]["alloc"],
                             "lru": procs[proc]["lru"]})
        df.plot(title="proc " + proc)
        plt.show()

def experiment(duration_s, cmd, frequency):
    """
    Starts systrace and run a command on target if specified. If
    no command is given, collect the trace for duration_s seconds.

    :param duration_s: duration to collect systrace
    :type duration_s: int

    :param cmd: command to execute on the target
    :type cmd: string

    :param frequency: sampling frequency for page stats
    :type frequency: float
    """
    systrace_output = System.systrace_start(
        te, os.path.join(te.res_dir, 'trace.html'), conf=conf)
    systrace_output.expect("Starting tracing")

    if frequency:
        run_page_stats(duration_s, frequency)
    elif cmd:
        target.execute(cmd)
    else:
        sleep(duration_s)

    systrace_output.sendline("")
    System.systrace_wait(te, systrace_output)
    te.platform_dump(te.res_dir)

parser = argparse.ArgumentParser(
    description="Collect systrace for binder events while executing"
    "a command on the target or wait for duration_s seconds")

parser.add_argument("--duration", "-d", type=int, default=0,
                    help="How long to collect the trace in seconds.")
parser.add_argument("--command", "-c", type=str, default="",
                    help="Command to execute on the target.")
parser.add_argument("--pagestats", "-p", nargs="?", type=float, default=None,
                    const=0.1, help="Run binder page stats analysis."
                    "Optional argument for sample interval in seconds.")

if __name__ == "__main__":
    args = parser.parse_args()
    experiment(args.duration, args.command, args.pagestats)
