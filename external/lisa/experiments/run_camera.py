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
from android import Screen, Workload, System
from trace import Trace
import trappy
import pandas as pd
import sqlite3
import argparse
import shutil

workloads = ['CameraPreview', 'CameraStartup']

parser = argparse.ArgumentParser(description='Camera Workload tests')

parser.add_argument('--out_prefix', dest='out_prefix', action='store', default='default',
                    help='prefix for out directory')

parser.add_argument('--collect', dest='collect', action='store', default='systrace',
                    help='what to collect (default systrace)')

parser.add_argument('--duration', dest='duration_s', action='store',
                    type=int,
                    help='Duration of test (default 30s)')

parser.add_argument('--serial', dest='serial', action='store',
                    help='Serial number of device to test')

parser.add_argument('--workload', dest='workload', action='store',
                    default='CameraPreview', help='Camera workload to run (ex. CameraPreview)')

args = parser.parse_args()

if args.workload not in workloads:
    raise RuntimeError('Invalid workload specified')

def experiment():
    # Get workload
    wload = Workload.getInstance(te, args.workload)

    outdir=te.res_dir + '_' + args.out_prefix
    try:
        shutil.rmtree(outdir)
    except:
        print "coulnd't remove " + outdir
        pass
    os.makedirs(outdir)

    # Run Camera
    # Note: The default time duration of the test is specificified within the workload
    if args.duration_s:
        wload.run(outdir, duration_s=args.duration_s, collect=args.collect)
    else:
        wload.run(outdir, collect=args.collect)

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
    "results_dir" : args.workload,

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
        'extra_categories': ['camera']
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

results = experiment()
