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
# This experiment enables CGroup tracing for UiBench workloads
# The main difference between the run_uibench.py experiment is:
# - post_collect_start hook used to dump fake cgroup events
# - extra event: 'cgroup_attach_task' passed to systrace_start

import logging

from conf import LisaLogging
LisaLogging.setup()
import json
import os
import devlib
from env import TestEnv
from android import Screen, Workload, System
from trace import Trace
import trappy
import pandas as pd
import sqlite3
import argparse
import shutil
import time

parser = argparse.ArgumentParser(description='UiBench tests')

parser.add_argument('--out_prefix', dest='out_prefix', action='store', default='cgroup',
                    help='prefix for out directory')

parser.add_argument('--collect', dest='collect', action='store', default='systrace',
                    help='what to collect (default systrace)')

parser.add_argument('--test', dest='test_name', action='store',
                    default='UiBenchJankTests#testGLTextureView',
                    help='which test to run')

parser.add_argument('--iterations', dest='iterations', action='store',
                    default=10, type=int,
                    help='Number of times to repeat the tests per run (default 10)')

parser.add_argument('--serial', dest='serial', action='store',
                    help='Serial number of device to test')

args = parser.parse_args()

def experiment(outdir):
    # Get workload
    wload = Workload.getInstance(te, 'UiBench')

    try:
        shutil.rmtree(outdir)
    except:
        print "coulnd't remove " + outdir
        pass
    os.makedirs(outdir)

    # Run UiBench
    wload.run(outdir, test_name=args.test_name, iterations=args.iterations, collect=args.collect)

    # Dump platform descriptor
    te.platform_dump(te.res_dir)

    te._log.info('RESULTS are in out directory: {}'.format(outdir))

# Setup target configuration
my_conf = {

    # Target platform and board
    "platform"     : 'android',

    # Useful for reading names of little/big cluster
    # and energy model info, its device specific and use
    # only if needed for analysis
    # "board"        : 'pixel',

    # Device
    # By default the device connected is detected, but if more than 1
    # device, override the following to get a specific device.
    # "device"       : "HT6880200489",

    # Folder where all the results will be collected
    "results_dir" : "UiBench",

    # Define devlib modules to load
    "modules"     : [
        'cpufreq',      # enable CPUFreq support
        'cpuidle',      # enable cpuidle support
        'cgroups'       # Enable for cgroup support, doing this also enables cgroup tracing
    ],

    "emeter" : {
        'instrument': 'monsoon',
        'conf': { }
    },

    "systrace": {
        # Mandatory events for CGroup tracing
        'extra_events': ['cgroup_attach_task', 'sched_process_fork']
    },

    # Tools required by the experiments
    "tools"   : [ 'taskset'],

    "skip_nrg_model" : True,
}

if args.serial:
    my_conf["device"] = args.serial

# Initialize a test environment using:
te = TestEnv(my_conf, wipe=False)
target = te.target

outdir=te.res_dir + '_' + args.out_prefix
results = experiment(outdir)

trace_file = os.path.join(outdir, "trace.html")
tr = Trace(None, trace_file,
    cgroup_info = {
	'cgroups': ['foreground', 'background', 'system-background', 'top-app', 'rt'],
	'controller_ids': { 4: 'cpuset', 2: 'schedtune' }
    },
    events=[ 'sched_switch', 'cgroup_attach_task_devlib', 'cgroup_attach_task', 'sched_process_fork' ],
    normalize_time=False)

tr.data_frame.cpu_residencies_cgroup('schedtune')
tr.analysis.residency.plot_cgroup('schedtune', idle=False)
tr.analysis.residency.plot_cgroup('schedtune', idle=True)
