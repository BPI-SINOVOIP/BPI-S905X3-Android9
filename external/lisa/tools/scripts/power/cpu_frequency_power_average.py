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

from __future__ import division
import os
import re
import json
import glob
import argparse
import pandas as pd
import numpy as np

from power_average import PowerAverage

# This script computes the cluster power cost and cpu power costs at each
# frequency for each cluster. The output can be used in power models or power
# profiles.

def average(values):
    return sum(values) / len(values)

class SampleReader:
    def __init__(self, results_dir, column):
        self.results_dir = results_dir
        self.column = column

    def get(self, filename):
        files = glob.glob(os.path.join(self.results_dir, filename))
        if len(files) != 1:
            raise ValueError('Multiple files match pattern')
        return PowerAverage.get(files[0], self.column)

class Cpu:
    def __init__(self, platform_file, sample_reader):
        self.platform_file = platform_file
        self.sample_reader = sample_reader
        # This is the additional cost when any cluster is on. It is seperate from
        # the cluster cost because it is not duplicated when a second cluster
        # turns on.
        self.active_cost = -1.0

        # Read in the cluster and frequency information from the plaform.json
        with open(platform_file, 'r') as f:
            platform = json.load(f)
        self.clusters = {i : Cluster(self.sample_reader, i, platform["clusters"][i],
                platform["freqs"][i]) for i in sorted(platform["clusters"])}

        if len(self.clusters) != 2:
            raise ValueError('Only cpus with 2 clusters are supported')

        self.compute_costs()

    def compute_costs(self):
        # Compute initial core costs by freq. These are necessary for computing the
        # cluster and active costs. However, since the cluster and active costs are computed
        # using averages across all cores and frequencies, we will need to adjust the
        # core cost at the end.
        #
        # For example: The total cpu cost of core 0 on cluster 0 running at
        # a given frequency is 25. We initally compute the core cost as 10.
        # However the active and cluster averages end up as 9 and 3. 10 + 9 + 3 is
        # 22 not 25. We can adjust the core cost 13 to cover this error.
        for cluster in self.clusters:
            self.clusters[cluster].compute_initial_core_costs()

        # Compute the cluster costs
        cluster0 = self.clusters.values()[0]
        cluster1 = self.clusters.values()[1]
        cluster0.compute_cluster_cost(cluster1)
        cluster1.compute_cluster_cost(cluster0)

        # Compute the active cost as an average of computed active costs by cluster
        self.active_cost = average([self.clusters[cluster].compute_active_cost() for cluster in self.clusters])

        # Compute final core costs. This will help correct for any errors introduced
        # by the averaging of the cluster and active costs.
        for cluster in self.clusters:
            self.clusters[cluster].compute_final_core_costs(self.active_cost)

    def get_clusters(self):
        with open(self.platform_file, 'r') as f:
            platform = json.load(f)
        return platform["clusters"]

    def get_active_cost(self):
        return self.active_cost

    def get_cluster_cost(self, cluster):
        return self.clusters[cluster].get_cluster_cost()

    def get_cores(self, cluster):
        return self.clusters[cluster].get_cores()

    def get_core_freqs(self, cluster):
        return self.clusters[cluster].get_freqs()

    def get_core_cost(self, cluster, freq):
        return self.clusters[cluster].get_core_cost(freq)

    def dump(self):
        print 'Active cost: {}'.format(self.active_cost)
        for cluster in self.clusters:
            self.clusters[cluster].dump()

class Cluster:
    def __init__(self, sample_reader, handle, cores, freqs):
        self.sample_reader = sample_reader
        self.handle = handle
        self.cores = cores
        self.cluster_cost = -1.0
        self.core_costs = {freq:-1.0 for freq in freqs}

    def compute_initial_core_costs(self):
        # For every frequency, freq
        for freq, _ in self.core_costs.iteritems():
            total_costs = []
            core_costs = []

            # Store the total cost for turning on 1 to len(cores) on the
            # cluster at freq
            for cnt in range(1, len(self.cores)+1):
                total_costs.append(self.get_sample_avg(cnt, freq))

            # Compute the additional power cost of turning on another core at freq.
            for i in range(len(total_costs)-1):
                core_costs.append(total_costs[i+1] - total_costs[i])

            # The initial core cost is the average of the additional power to add
            # a core at freq
            self.core_costs[freq] = average(core_costs)

    def compute_final_core_costs(self, active_cost):
        # For every frequency, freq
        for freq, _ in self.core_costs.iteritems():
            total_costs = []
            core_costs = []

            # Store the total cost for turning on 1 to len(cores) on the
            # cluster at freq
            for core_cnt in range(1, len(self.cores)+1):
                total_costs.append(self.get_sample_avg(core_cnt, freq))

            # Recompute the core cost as the sample average minus the cluster and
            # active costs divided by the number of cores on. This will help
            # correct for any error introduced by averaging the cluster and
            # active costs.
            for i, total_cost in enumerate(total_costs):
                core_cnt = i + 1
                core_costs.append((total_cost - self.cluster_cost - active_cost) / (core_cnt))

            # The final core cost is the average of the core costs at freq
            self.core_costs[freq] = average(core_costs)

    def compute_cluster_cost(self, other_cluster=None):
        # Create a template for the file name. For each frequency we will be able
        # to easily substitute it into the file name.
        template = '{}_samples.csv'.format('_'.join(sorted(
                ['cluster{}-cores?-freq{{}}'.format(self.handle),
                'cluster{}-cores?-freq{}'.format(other_cluster.get_handle(),
                other_cluster.get_min_freq())])))

        # Get the cost of running a single cpu at min frequency on the other cluster
        cluster_costs = []
        other_cluster_total_cost = other_cluster.get_sample_avg(1, other_cluster.get_min_freq())

        # For every frequency
        for freq, core_cost in self.core_costs.iteritems():
            # Get the cost of running a single core on this cluster at freq and
            # a single core on the other cluster at min frequency
            total_cost = self.sample_reader.get(template.format(freq))
            # Get the cluster cost by subtracting all the other costs from the
            # total cost so that the only cost that remains is the cluster cost
            # of this cluster
            cluster_costs.append(total_cost - core_cost - other_cluster_total_cost)

        # Return the average calculated cluster cost
        self.cluster_cost = average(cluster_costs)

    def compute_active_cost(self):
        active_costs = []

        # For every frequency
        for freq, core_cost in self.core_costs.iteritems():
            # For every core
            for i, core in enumerate(self.cores):
                core_cnt = i + 1
                # Subtract the core and cluster costs from each total cost.
                # The remaining cost is the active cost
                active_costs.append(self.get_sample_avg(core_cnt, freq)
                        - core_cost*core_cnt - self.cluster_cost)

        # Return the average active cost
        return average(active_costs)

    def get_handle(self):
        return self.handle

    def get_min_freq(self):
        return min(self.core_costs, key=self.core_costs.get)

    def get_sample_avg(self, core_cnt, freq):
        core_str = ''.join('{}-'.format(self.cores[i]) for i in range(core_cnt))
        filename = 'cluster{}-cores{}freq{}_samples.csv'.format(self.handle, core_str, freq)
        return self.sample_reader.get(filename)

    def get_cluster_cost(self):
        return self.cluster_cost

    def get_cores(self):
        return self.cores

    def get_freqs(self):
        return self.core_costs.keys()

    def get_core_cost(self, freq):
        return self.core_costs[freq]

    def dump(self):
        print 'Cluster {} cost: {}'.format(self.handle, self.cluster_cost)
        for freq in sorted(self.core_costs):
            print '\tfreq {} cost: {}'.format(freq, self.core_costs[freq])

class CpuFrequencyPowerAverage:
    @staticmethod
    def get(results_dir, platform_file, column):
        sample_reader = SampleReader(results_dir, column)
        cpu = Cpu(platform_file, sample_reader)
        return cpu


parser = argparse.ArgumentParser(
        description="Get the cluster cost and cpu cost per frequency. Optionally"
                    " specify a time interval over which to calculate the sample.")

parser.add_argument("--column", "-c", type=str, required=True,
                    help="The name of the column in the samples.csv's that"
                    " contain the power values to average.")

parser.add_argument("--results_dir", "-d", type=str,
                    default=os.path.join(os.environ["LISA_HOME"],
                    "results/CpuFrequency_default"),
                    help="The results directory to read from. (default"
                    " LISA_HOME/results/CpuFrequency_default)")

parser.add_argument("--platform_file", "-p", type=str,
                    default=os.path.join(os.environ["LISA_HOME"],
                    "results/CpuFrequency/platform.json"),
                    help="The results directory to read from. (default"
                    " LISA_HOME/results/CpuFrequency/platform.json)")

if __name__ == "__main__":
    args = parser.parse_args()

    cpu = CpuFrequencyPowerAverage.get(args.results_dir, args.platform_file, args.column)
    cpu.dump()
