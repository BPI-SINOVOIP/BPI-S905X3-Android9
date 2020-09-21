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
import json
import argparse
from trace import Trace

def run_queue_analysis(trace, threshold):
    """
    Plot the queuing delay distribution and
    severely delayed tasks.

    :param trace: input Trace object
    :type trace: :mod:`libs.utils.Trace`

    :param threshold: plot transactions taken longer than threshold
    :type threshold: int
    """
    df = trace.data_frame.queue_df()
    trace.analysis.binder_transaction.plot_samples(df, "delta_t",
                                                   "transaction samples",
                                                   "wait time (microseconds)")
    trace.analysis.binder_transaction.plot_tasks(df, threshold, "__comm_x",
                                                 "delta_t", "tasks",
                                                 "wait time (microseconds)")
    return df

def run_buffer_analysis(trace, threshold):
    """
    Plot the buffer size distribution and big transactions.

    :param trace: input Trace object
    :type trace: :mod:`libs.utils.Trace`

    :param threshold: plot buffers larger than threshold
    :type threshold: int
    """
    df = trace.data_frame.alloc_df()
    trace.analysis.binder_transaction.plot_samples(df, "size",
                                                   "sample trace points",
                                                   "buffersize (bytes)")
    trace.analysis.binder_transaction.plot_tasks(df, threshold,
                                                 "__pid_x", "size",
                                                 "proc_id",
                                                 "buffersize (bytes)")
    return df

def run_alloc_analysis(trace, threshold):
    """
    Plot the runtime of buffer allocator.

    :param trace: input Trace object
    :type trace: :mod:`libs.utils.Trace`

    :param threshold: plot allocations took less than threshold
    :type threshold: int
    """
    df = trace.data_frame.alloc_df()
    trace.analysis.binder_transaction.plot_samples(df, "delta_t",
                                                   "transaction samples",
                                                   "alloc time (microseconds)",
                                                   ymax=threshold)
    return df

parser = argparse.ArgumentParser(
        description="Plot transaction data and write dataframe to a csv file.")
parser.add_argument("--res_dir","-d", type=str,
                    help="Directory that contains trace.html.")
parser.add_argument("--analysis_type", "-t", type=str,
                    choices=["queue", "buffer", "alloc"],
                    help="Analysis type. Available options: 'queue'"
                    "(target queue wait time), 'buffer' (binder buffer size)"
                    "and 'alloc' (allocation time).")
parser.add_argument("--threshold", "-th", type=int,
                    help="Threshold above which a task or buffer will be"
                    "captured or below which an allocation will be captured"
                    "(microseconds for queuing delay and allocation time,"
                    "bytes for buffer size).")

if __name__ == "__main__":
    args = parser.parse_args()

    platform_file = os.path.join(args.res_dir, "platform.json")
    with open(platform_file, 'r') as fh:
        platform = json.load(fh)
    trace_file = os.path.join(args.res_dir, "trace.html")
    events = ["binder_transaction",
              "binder_transaction_received",
              "binder_transaction_alloc_buf"]
    trace = Trace(platform, trace_file, events)

    if args.analysis_type == "queue":
        df = run_queue_analysis(trace, args.threshold)
    elif args.analysis_type == "buffer":
        df = run_buffer_analysis(trace, args.threshold)
    else:
        df = run_alloc_analysis(trace, args.threshold)

    df.to_csv(os.path.join(args.res_dir, "analysis_df.csv"))
