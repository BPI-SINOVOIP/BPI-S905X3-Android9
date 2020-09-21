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

import os
import pandas as pd
import sqlite3
import argparse
import shutil

# Run a comparison experiment between 2 Jankbench result directories
# containing multiple subtests. This makes it easy to compare results
# between 2 jankbench runs.
#
# Sample run:
# ./compare_jankbench.py --baseline='./results/Jankbench_baseline'
# --compare-with='./results/Jankbench_kernelchange'
#
# The output will be something like (only showing 25% and 50%):
#                       25% compare  25%_diff     50%_compare 50%_diff
#  test_name
#  image_list_view       2.11249      0.0178108    5.7952  0.0242445
#  list_view             2.02227      -3.65839     5.74957  -0.095421
#  shadow_grid           6.00877      -0.000898    6.23746 -0.0057695
#  high_hitrate_text     5.81625      0.0264913    6.03504  0.0017795
#
# (Note that baseline_df is only used for calculations.

JANKBENCH_DB_NAME = 'BenchmarkResults'

def get_results(out_dir):
    """
    Extract data from results db and return as a pandas dataframe

    :param out_dir: Output directory for a run of the Jankbench workload
    :type out_dir: str
    """
    path = os.path.join(out_dir, JANKBENCH_DB_NAME)
    columns = ['_id', 'name', 'run_id', 'iteration', 'total_duration', 'jank_frame']
    data = []
    conn = sqlite3.connect(path)
    for row in conn.execute('SELECT {} FROM ui_results'.format(','.join(columns))):
        data.append(row)
    return pd.DataFrame(data, columns=columns)

def build_stats_df(test_outdir):
    """
    Build a .describe() df with statistics
    """
    stats_dfs = []
    for t in tests:
        test_dir = os.path.join(test_outdir, t)
        res_df = get_results(test_dir).describe(percentiles=[0.25,0.5,0.75,0.9,0.95,0.99])
        stats_df = res_df['total_duration']
        stats_df['test_name'] = t
        stats_dfs.append(stats_df)
    fdf = pd.concat(stats_dfs, axis = 1).T
    fdf.set_index('test_name', inplace=True)
    return fdf


parser = argparse.ArgumentParser(description='Jankbench comparisons')

parser.add_argument('--baseline', dest='baseline', action='store', default='default',
                    required=True, help='baseline out directory')

parser.add_argument('--compare-with', dest='compare_with', action='store', default='default',
                    required=True, help='out directory to compare with baseline')

args = parser.parse_args()

# Get list of Jankbench tests available
tests = os.listdir(args.baseline)
tests = [t for t in tests if os.path.isdir(os.path.join(args.baseline, t))]

# Build a baseline df (experiment baseline - say without kernel change)
#  compare df (experiment with change)
#  diff (difference in stats between compare and baseline)

baseline_df = build_stats_df(args.baseline)
compare_df = build_stats_df(args.compare_with)
diff = compare_df - baseline_df

diff.columns = [str(col) + '_diff' for col in diff.columns]
baseline_df.columns = [str(col) + '_baseline' for col in baseline_df.columns]
compare_df.columns  = [str(col) + '_compare' for col in compare_df.columns]

final_df = pd.concat([compare_df, diff], axis=1)
final_df = final_df.reindex_axis(sorted(final_df.columns), axis=1)

# Print the results
print final_df
