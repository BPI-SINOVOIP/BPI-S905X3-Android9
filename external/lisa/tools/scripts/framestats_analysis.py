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
#
# Analysis of framestats deltas between 2+ runs
# This analysis outputs potential regressions when comparing
# a baseline folder output of microbenchmarks like UiBench with
# a test folder with similar stats.  The base and test folders
# can contain the output of one or more runs of run_uibench.py or
# similar script.

# For example:
# python framestats_analysis.py --baseline_dir=/usr/local/google/home/andresoportus/b/master2/external/lisa/results/UiBench_walleye_em_baseline_b/ --results_dir=/usr/local/google/home/andresoportus/b/master2/external/lisa/results/UiBench_walleye_em_first/ --threshold_ave 0.322
# No handlers could be found for logger "EnergyModel"
# Potential regression in:
#                  avg-frame-time-50                avg-frame-time-90                avg-frame-time-95                avg-frame-time-99
# UiBenchJankTests#testTrivialAnimation
# all base         [ 5.  5.  5.  5.  5.  5.  5.]    [ 5.  5.  5.  5.  5.  5.  5.]    [ 5.  5.  5.  5.  5.  5.  5.]    [ 5.   5.2  5.1  5.2  5.9  5.2  5.2]
# all test         [ 5.  5.  5.  5.  5.  5.]        [ 5.  5.  5.  5.  5.  5.]        [ 5.  5.  5.  5.  5.  5.]        [ 5.4  6.   5.3  5.8  6.   5.1]
# ave base         [ 5.]                            [ 5.]                            [ 5.]                            [ 5.25714286]
# ave test         [ 5.]                            [ 5.]                            [ 5.]                            [ 5.6]
# avg delta        [ 0.]                            [ 0.]                            [ 0.]                            [ 0.34285714]

import os
import sys
import argparse
import numpy as np
import pandas as pd
from collections import OrderedDict
from android.workloads.uibench import UiBench

averages = ['avg-frame-time-50', 'avg-frame-time-90',
            'avg-frame-time-95', 'avg-frame-time-99']
stats_file = 'framestats.txt'

"""
Parses a directory to find stats

:param stats_dir: path to stats dir
:type stats_dir: str

:param stats_file: name of stats file
:type stats_file: str
"""
def parse_stats_dir(stats_dir, stats_file):
    df = {}
    run_name = os.path.basename(os.path.normpath(stats_dir))
    for root, dirs, files in os.walk(stats_dir):
        if stats_file in files:
            test_name = os.path.basename(os.path.normpath(root))
            if test_name not in df:
                df[test_name] = pd.DataFrame()
            df[test_name] = df[test_name].append(UiBench.get_results(root))
    return df

"""
Prints a header line

:param df: pandas dataframe to extract columns names from
:type df: pd.DataFrame
"""
def print_header(df):
    sys.stdout.write('{:16}'.format(''))
    for c in df.columns:
        sys.stdout.write(' {:32}'.format(str(c)))
    sys.stdout.write('\n')

"""
Prints a pandas DataFrame as a row

:param df: pandas dataframe to extract values from
:type df: pd.DataFrame
"""
def print_as_row(name, df):
    sys.stdout.write('{:16}'.format(name))
    for c in df.columns:
        sys.stdout.write(' {:32}'.format(str(df[c].values)))
    sys.stdout.write('\n')

def main():
    header_printed = False
    pd.set_option('precision', 2)
    base = parse_stats_dir(args.baseline_dir, stats_file)
    test = parse_stats_dir(args.results_dir, stats_file)
    for name in base:
        try:
            # DataFrame created from the Series output of mean() needs to
            # be transposed to make averages be columns again
            ave_base = pd.DataFrame(base[name][averages].mean()).T
            ave_test = pd.DataFrame(test[name][averages].mean()).T
            if args.verbose or \
               ((ave_test - ave_base) > args.threshold_ave).iloc[0].any():
                if not header_printed:
                    if not args.verbose:
                        print "Potential regression in:"
                    print_header(base[name][averages])
                    header_printed = True
                print name
                print_as_row('all base', base[name][averages])
                print_as_row('all test', test[name][averages])
                print_as_row('ave base', ave_base)
                print_as_row('ave test', ave_test)
                print_as_row('avg delta', ave_test - ave_base)
        except KeyError:
            sys.stdout.write('\n')
    if not header_printed:
        print "No regression found"

if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Framestats analysis")

    parser.add_argument("--results_dir", "-d", type=str,
                        default=os.path.join(os.environ["LISA_HOME"],
                                             "results/UiBench_default"),
                        help="The test directory to read from. (default \
                        LISA_HOME/restuls/UiBench_deafult)")
    parser.add_argument("--baseline_dir", "-b", type=str, required=True,
                        help="The directory that provides baseline test to \
                        compare against")
    parser.add_argument("--threshold_ave", "-t", type=float, default=0.99,
                        help="Amount to filter noise in average values")
    parser.add_argument("--verbose", "-v", action='store_true',
                        help="Verbose output")
    args = parser.parse_args()
    main()
