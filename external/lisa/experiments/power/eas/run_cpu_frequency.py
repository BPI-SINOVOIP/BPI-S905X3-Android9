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
from time import sleep

parser = argparse.ArgumentParser(description='CpuFrequency tests')

parser.add_argument('--out_prefix', dest='out_prefix', action='store',
                    default='default',
                    help='prefix for out directory')

parser.add_argument('--continue', dest='cont', action='store',
                    default=False, type=bool,
                    help='continue previous experiment with same prefix')

parser.add_argument('--duration', dest='duration_s', action='store',
                    default=30, type=int,
                    help='Duration of test (default 30s)')

parser.add_argument('--serial', dest='serial', action='store',
                    help='Serial number of device to test')

args = parser.parse_args()

CRITICAL_TASKS = [
    "/system/bin/sh", "adbd", "/init"
]

def outfiles(cluster, cpus, freq):
    prefix = 'cluster{}-cores{}freq{}_'.format(str(cluster), ''.join('{}-'.format(cpu) for cpu in cpus), freq)
    samples = '{}samples.csv'.format(prefix)
    energy = '{}energy.json'.format(prefix)
    return energy, samples

def update_cpus(target, on_cpus, off_cpus):
    for cpu in on_cpus:
        target.hotplug.online(cpu)

    for cpu in off_cpus:
        target.hotplug.offline(cpu)

def run_dhrystone(target, dhrystone, outdir, energy, samples, on_cpus):
    # Run dhrystone benchmark for longer than the requested time so
    # we have extra time to set up the measuring device
    for on_cpu in on_cpus:
        target.execute('nohup taskset {:x} {} -t {} -r {}  2>/dev/null 1>/dev/null &'.format(1 << (on_cpu), dhrystone, 1, args.duration_s+30))

    # Start measuring
    te.emeter.reset()

    # Sleep for the required time
    sleep(args.duration_s)

    # Stop measuring
    te.emeter.report(outdir, out_energy=energy, out_samples=samples)

    # Since we are using nohup, the benchmark doesn't show up in
    # process list. Instead sleep until we can be sure the benchmark
    # is dead.
    sleep(30)

def single_cluster(cpus, sandbox_cg, isolated_cg, dhrystone, outdir):
    # For each cluster
    for i, cluster in enumerate(CLUSTERS):

        # For each frequency on the cluster
        for freq in target.cpufreq.list_frequencies(cluster[0]):

            # Keep track of offline and online cpus
            off_cpus = cpus[:]
            on_cpus = []

            # For each cpu in the cluster
            for cpu in cluster:
                # Add the current cpu to the online list and remove it from the
                # offline list
                on_cpus.append(cpu)
                off_cpus.remove(cpu)

                # Switch the output file so the previous samples are not overwritten
                energy, samples = outfiles(i, on_cpus, freq)

                # If we are continuing from a previous experiment and this set has
                # already been run, skip it
                if args.cont and os.path.isfile(os.path.join(outdir, energy)) and os.path.isfile(os.path.join(outdir, samples)):
                    continue

                # Bring the on_cpus online take the off_cpus offline
                update_cpus(target, on_cpus, off_cpus)
                target.cpufreq.set_frequency(cpu, freq)

                # Update sandbox and isolated cgroups
                sandbox_cg.set(cpus=on_cpus)
                isolated_cg.set(cpus=off_cpus)

                # Run the benchmark
                run_dhrystone(target, dhrystone, outdir, energy, samples, on_cpus)

    # Restore all the cpus
    target.hotplug.online_all()


def multiple_clusters(cpus, sandbox_cg, isolated_cg, dhrystone, outdir):
    # Keep track of offline and online cpus
    off_cpus = cpus[:]
    on_cpus = []
    prefix = ''

    if len(CLUSTERS) != 2:
        print 'Only 2 clusters is supported.'
        return

    # For each cluster
    for i, cluster in enumerate(CLUSTERS):
        # A cpu in each cluster
        cpu = cluster[0]

        freq = target.cpufreq.list_frequencies(cpu)[0]

        # Set frequency to min
        target.cpufreq.set_frequency(cpu, freq)

        # Keep cpu on
        on_cpus.append(cpu)
        off_cpus.remove(cpu)

        prefix = '{}cluster{}-cores{}-freq{}_'.format(prefix, i, cpu, freq)

    # Update cgroups to reflect on_cpus and off_cpus
    sandbox_cg.set(cpus=on_cpus)
    isolated_cg.set(cpus=off_cpus)

    # Bring the on_cpus online take the off_cpus offline
    update_cpus(target, on_cpus, off_cpus)

    # For one cpu in each cluster
    for i, cpu in enumerate(on_cpus):

        # For each frequency on the cluster
        for freq in target.cpufreq.list_frequencies(cpu):

            # Switch the output file so the previous samples are not overwritten
            curr_prefix = prefix.replace('cores{}-freq{}'.format(cpu,
                    target.cpufreq.list_frequencies(cpu)[0]),
                    'cores{}-freq{}'.format(cpu, freq))
            samples = '{}samples.csv'.format(curr_prefix)
            energy = '{}energy.json'.format(curr_prefix)

            # If we are continuing from a previous experiment and this set has
            # already been run, skip it
            if args.cont and os.path.isfile(os.path.join(outdir, energy)) and os.path.isfile(os.path.join(outdir, samples)):
                continue

            # Set frequency
            target.cpufreq.set_frequency(cpu, freq)

            # Run the benchmark
            run_dhrystone(target, dhrystone, outdir, energy, samples, on_cpus)

        # Reset frequency to min
        target.cpufreq.set_frequency(cpu, target.cpufreq.list_frequencies(cpu)[0])

    # Restore all the cpus
    target.hotplug.online_all()


def experiment():
    # Check if the dhyrstone binary is on the device
    dhrystone = os.path.join(target.executables_directory, 'dhrystone')
    if not target.file_exists(dhrystone):
        raise RuntimeError('dhrystone could not be located here: {}'.format(
                dhrystone))

    # Create results directory
    outdir=te.res_dir + '_' + args.out_prefix
    if not args.cont:
        try:
            shutil.rmtree(outdir)
        except:
            print "couldn't remove " + outdir
            pass
    if not os.path.exists(outdir):
        os.makedirs(outdir)

    # Get clusters and cpus
    cpus = [cpu for cluster in CLUSTERS for cpu in cluster]

    # Prevent screen from dozing
    Screen.set_doze_always_on(target, on=False)

    # Turn on airplane mode
    System.set_airplane_mode(target, on=True)

    # Turn off screen
    Screen.set_screen(target, on=False)

    # Stop thermal engine and perfd
    target.execute("stop thermal-engine")
    target.execute("stop perfd")

    # Take a wakelock
    System.wakelock(target, take=True)

    # Store governors so they can be restored later
    governors = [ target.cpufreq.get_governor(cpu) for cpu in cpus]

    # Set the governer to userspace so the cpu frequencies can be set
    target.hotplug.online_all()
    target.cpufreq.set_all_governors('userspace')

    # Freeze all non critical tasks
    target.cgroups.freeze(exclude=CRITICAL_TASKS)

    # Remove all userspace tasks from the cluster
    sandbox_cg, isolated_cg = target.cgroups.isolate([])

    # Run measurements on single cluster
    single_cluster(cpus, sandbox_cg, isolated_cg, dhrystone, outdir)

    # Run measurements on multiple clusters
    multiple_clusters(cpus, sandbox_cg, isolated_cg, dhrystone, outdir)

    # Restore all governors
    for i, governor in enumerate(governors):
        target.cpufreq.set_governor(cpus[i], governor)

    # Restore non critical tasks
    target.cgroups.freeze(thaw=True)

    # Release wakelock
    System.wakelock(target, take=False)

    # Stop thermal engine and perfd
    target.execute("start thermal-engine")
    target.execute("start perfd")

    # Dump platform
    te.platform_dump(outdir)

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
    "results_dir" : "CpuFrequency",

    # Define devlib modules to load
    "modules"     : [
        'cpufreq',      # enable CPUFreq support
        'hotplug',      # enable hotplug support
        'cgroups',      # enable cgroups support
    ],

    "emeter" : {
        'instrument': 'monsoon',
        'conf': { }
    },

    # Tools required by the experiments
    "tools"     : [ ],

    "skip_nrg_model" : True,
}

if args.serial:
    my_conf["device"] = args.serial

# Initialize a test environment using:
te = TestEnv(my_conf, wipe=False)
target = te.target
CLUSTERS = te.topology.get_level('cluster')

results = experiment()
