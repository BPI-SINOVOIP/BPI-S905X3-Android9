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

parser = argparse.ArgumentParser(description='UiBench tests')

parser.add_argument('--out_prefix', dest='out_prefix', action='store', default='default',
                    help='prefix for out directory')

parser.add_argument('--collect', dest='collect', action='store', default='systrace',
                    help='what to collect (default systrace)')

parser.add_argument('--test', dest='test_name', action='store',
                    default='UiBenchJankTests#testResizeHWLayer',
                    help='which test to run')

parser.add_argument('--iterations', dest='iterations', action='store',
                    default=10, type=int,
                    help='Number of times to repeat the tests per run (default 10)')

parser.add_argument('--serial', dest='serial', action='store',
                    help='Serial number of device to test')

parser.add_argument('--all', dest='run_all', action='store_true',
                    help='Run all tests')

parser.add_argument('--reinstall', dest='reinstall', action='store_true',
                    help='Rebuild and reinstall test apks')

parser.add_argument('--reimage', dest='reimage', action='store',
                    default='',
                    help='Flag to reimage target device (kernel-update kernel image | all-update complete android image)')

parser.add_argument('--kernel_path', dest='kernel_path', action='store',
                    default='',
                    help='Path to kernel source directory. Required if reimage option is used')

args = parser.parse_args()

def make_dir(outdir):
    try:
        shutil.rmtree(outdir)
    except:
        print "couldn't remove " + outdir
        pass
    os.makedirs(outdir)

def experiment():
    def run_test(outdir, test_name):
        te._log.info("Running test {}".format(test_name))
        wload.run(outdir, test_name=test_name, iterations=args.iterations, collect=args.collect)

    if args.reimage:
        System.reimage(te, args.kernel_path, args.reimage)

    # Get workload
    wload = Workload.getInstance(te, 'UiBench', args.reinstall)

    outdir=te.res_dir + '_' + args.out_prefix
    make_dir(outdir)

    # Run UiBench
    if args.run_all:
        te._log.info("Running all tests: {}".format(wload.test_list))
        for test in wload.get_test_list():
            test_outdir = os.path.join(outdir, test)
            make_dir(test_outdir)
            run_test(test_outdir, test)
    else:
        test_outdir = os.path.join(outdir, args.test_name)
        make_dir(test_outdir)
        run_test(test_outdir, args.test_name)

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
        # 'cgroups'     # Enable for cgroup support
    ],

    "emeter" : {
        'instrument': 'monsoon',
        'conf': { }
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
