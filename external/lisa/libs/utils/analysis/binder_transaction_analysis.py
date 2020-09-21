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
import pandas as pd
import matplotlib.pyplot as plt
from analysis_module import AnalysisModule

from devlib.utils.misc import memoized

class BinderTransactionAnalysis(AnalysisModule):
    """
    An analysis wrapper for visualizing binder transactions.

    This class is currently used to plot transaction buffer
    sizes and queuing delays.
    """
    to_micro_second = 1000000

    def __init__(self, trace):
        """
        Initialized by the directory that contains systrace output

        :param trace: input Trace object
        :type trace: :mod:`libs.utils.Trace`
        """
        super(BinderTransactionAnalysis, self).__init__(trace)

    @memoized
    def _dfg_alloc_df(self):
        """
        Get a dataframe that captures the time spent in a transaction
        allocation and the size of the buffer allocated sorted by time.

        Transaction and transaction_alloc_buf dataframes are joined
        on transaction(debug_id)

        Example of df returned:
        transaction (debug_id) | pid | delta_t | size
        """
        df_start = self._dfg_trace_event("binder_transaction")
        df_start["start_time"] = df_start.index
        df_end = self._dfg_trace_event("binder_transaction_alloc_buf")
        df_end["end_time"] = df_end.index
        df = pd.merge(df_start, df_end, on="transaction")
        df = df[["transaction", "__comm_x", "__pid_x",
                 "start_time", "end_time",
                 "data_size", "offsets_size"]]
        df["delta_t"] = (df["end_time"] - df["start_time"]) \
                        * BinderTransactionAnalysis.to_micro_second
        df["size"] = df["data_size"] - df["offsets_size"]
        df = df.loc[df["__comm_x"] == "binderThroughpu"] \
             [["transaction", "__pid_x", "delta_t", "size"]].sort("delta_t")
        return df

    @memoized
    def _dfg_queue_df(self):
        """
        Get a dataframe that captures start time, end time,
        and the delta between when a transaction is issued and
        when it is received by the target.

        Transaction and transaction_received dataframes are joined
        on transaction(debug_id)

        Example df:
        transaction (debug_id) | name | start | end | delta
        """
        df_send = self._dfg_trace_event("binder_transaction")
        df_send["start_time"] = df_send.index

        df_recv = self._dfg_trace_event("binder_transaction_received")
        df_recv["end_time"] = df_recv.index

        df = pd.merge(df_send, df_recv, on="transaction")
        df = df[["transaction", "__comm_x", "start_time", "end_time"]]
        df["delta_t"] = (df["end_time"] - df["start_time"]) \
                        * BinderTransactionAnalysis.to_micro_second
        return df

    def plot_samples(self, df, y_axis, xlabel, ylabel,
                     ymin=0, ymax=None, x_axis="index"):
        """
        Generate a plot that features the distribution of y_axis column
        in the given dataframe. x_axis represents the sample points.

        :param y_axis: column name of the dataframe we want to plot
        :type y_axis: str

        :param xlabel: label that appears on the plot's x-axis
        :type xlabel: str

        :param ylabel: label that appears on the plot's y-axis
        :type ylabel: str
        """
        df_sorted = df.sort_values(by=y_axis, ascending=True)
        df_sorted[x_axis] = range(len(df_sorted.index))
        df_sorted.plot(kind="scatter", x=x_axis, y=y_axis)
        ax = plt.gca()
        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylabel)
        ax.set_ylim(ymin=ymin)
        if ymax:
            ax.set_ylim(ymax=ymax)
        plt.show()

    def plot_tasks(self, df, threshold, x_axis, y_axis, xlabel, ylabel):
        """
        Generate a plot that features the tasks whose y_axis column
        in the dataframe is above a certain threshold.

        :param x_axis: column name of the dataframe we want to group
        together and use as the x-axis index in the plot
        :type x_axis: str

        :param y_axis: column name of the dataframe we want to plot
        :type y_axis: str

        :param xlabel: label that appears on the plot's x-axis
        :type xlabel: str

        :param ylabel: label that appears on the plot's y-axis
        :type ylabel: str
        """
        df_sorted = df.sort_values(by=y_axis, ascending=False)
        df_top = df_sorted[df_sorted[y_axis] > threshold]\
                 .groupby(x_axis).head(1)
        df_top.plot(kind="bar", y=y_axis, x=x_axis)
        ax = plt.gca()
        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylabel)
        plt.show()
