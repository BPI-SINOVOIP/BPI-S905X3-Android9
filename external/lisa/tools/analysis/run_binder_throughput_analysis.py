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

from trace import Trace

import trappy
import numpy as np
import matplotlib.pyplot as plt
import pandas as pd

import re
import argparse

class BinderThroughputAnalysis:
    """
    For deserializing and plotting results obtained from binderThroughputTest
    """

    def __init__(self, index):
        self.latency = []
        self.throughput = []
        self.labels = []
        self.index = index

    def add_data(self, label, latency, throughput):
        """
        Append the latency and throughput measurements of one kernel
        image to the global dataframe.

        :param label: identifier of the kernel image this data is collected from
        :type label: string

        :param latency: latency measurements of a series of experiments
        :type latency: list of floats

        :param throughput: throughput measurements of a series of experiments
        :type throughput: list of floats
        """
        self.latency.append(latency)
        self.throughput.append(throughput)
        self.labels.append(label)

    def _dfg_latency_df(self):
        np_latency = np.array(self.latency)
        data = np.transpose(np_latency.reshape(
            len(self.labels), len(self.latency[0])))
        return pd.DataFrame(data, columns=self.labels, index=self.index)

    def _dfg_throughput_df(self):
        np_throughput = np.array(self.throughput)
        data = np.transpose(np_throughput.reshape(
            len(self.labels),len(self.throughput[0])))
        return pd.DataFrame(data, columns=self.labels, index=self.index)

    def write_to_file(self, path):
        """
        Write the result dataframe of combining multiple kernel images to file.
        Example dataframe:
              kernel1_cs_4096  kernel2_cs_4096
        1           35.7657         35.3081
        2           37.3145         35.7540
        3           39.4055         39.0940
        4           44.4658         40.3857
        5           55.9990         51.6852

        :param path: the file to write the result dataframe to
        :type path: string
        """
        with open(path, 'w') as f:
            f.write(self._dfg_latency_df().to_string())
            f.write('\n\n')
            f.write(self._dfg_throughput_df().to_string())
            f.write('\n\n')

    def _plot(self, xlabel, ylabel):
        plt.xlabel(xlabel)
        plt.ylabel(ylabel)
        ax = plt.gca()
        ax.set_ylim(ymin=0)
        plt.show()

    def plot_latency(self, xlabel, ylabel):
        self._dfg_latency_df().plot(rot=0)
        self._plot(xlabel, ylabel)

    def plot_throughput(self, xlabel, ylabel):
        self._dfg_throughput_df().plot(rot=0)
        self._plot(xlabel, ylabel)

def deserialize(stream):
    """
    Convert stdout from running binderThroughputTest into a list
    of [avg_latency, iters] pair.
    """
    result = {}
    lines = stream.split("\n")
    for l in lines:
        if "average" in l:
            latencies = re.findall("\d+\.\d+|\d+", l)
            result["avg_latency"] = float(latencies[0])*1000
        if "iterations" in l:
            result["iters"] = float(l.split()[-1])
    return result

parser = argparse.ArgumentParser(
        description="Visualize latency and throughput across"
    "kernel images given binderThroughputTest output.")

parser.add_argument("--test", "-t", type=str,
                    choices=["cs", "payload"],
                    default="cs",
                    help="cs: vary number of cs_pairs while control payload.\n"
                    "payload: vary payload size, control number of cs_pairs.")

parser.add_argument("--paths", "-p", type=str, nargs='+',
                    help="Paths to files to read test output from.")

parser.add_argument("--out_file", "-o", type=str,
                    help="Out file to save dataframes.")

parser.add_argument("--cs_pairs", type=int, nargs='?',
                    default=[1,2,3,4,5],
                    help="Vary client-server pairs as index.")

parser.add_argument("--payloads", type=int, nargs='?',
                    default=[0, 4096, 4096*2, 4096*4, 4096*8],
                    help="Vary payloads as index.")

if __name__ == "__main__":
    args = parser.parse_args()

    if args.test == "cs":
        analysis = BinderThroughputAnalysis(args.cs_pairs)
    else:
        analysis = BinderThroughputAnalysis(args.payloads)

    for path in args.paths:
        with open(path, 'r') as f:
            results = f.read().split("\n\n")[:-1]
            results = list(map(deserialize, results))

            latency = [r["avg_latency"] for r in results]
            throughput = [r["iters"] for r in results]
            analysis.add_data(path.split('/')[-1], latency, throughput)

    analysis.write_to_file(args.out_file)
    analysis.plot_latency(args.test, "latency (microseconds")
    analysis.plot_throughput(args.test, "iterations/sec")
