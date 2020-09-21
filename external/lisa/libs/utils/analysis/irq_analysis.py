# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, Google, ARM Limited and contributors.
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

""" IRQ Analysis Module"""
from analysis_module import AnalysisModule
import matplotlib.pyplot as plt
import pylab as pl
import pandas as pd
import math

class IRQAnalysis(AnalysisModule):
    """
    Support for plotting IRQ Analysis data

    :param trace: input Trace object
    :type trace: :mod:`libs.utils.Trace`
    """
    def __init__(self, trace):
        super(IRQAnalysis, self).__init__(trace)

    def plotIRQHistogram(self, title="Histogram of IRQ events", irq=None, bin_size_s=0.25):
        """
        plot a histogram of irq events.
        :param title: title to afix to the plot
        :type  title: str

        :param name: the irq name or number to plot
        :type  name: str or int

        :param bin_size_s: the size of the bins for the histogram in seconds
        :type  bin_size_s: float
        """
        df = self._dfg_trace_event('irq_handler_entry')

        if len(df) == 0:
            self._log.warning('no irq events to plot')
            return

        if type(irq) is int:
            irqs = df[df.irq == irq]
        elif type(irq) is str:
            irqs = df[df.name == irq]
        else:
            raise TypeError('must give an irq number or name to plot')

        pd.options.mode.chained_assignment = None
        irqs['timestamp'] = irqs.index

        fig, axes = plt.subplots()
        plt.suptitle(title, y=.97, fontsize=16, horizontalalignment='center')

        if len(irqs) > 0:
            name = irqs.iloc[0]["name"]
            irq_number = irqs.iloc[0]["irq"]

            bin_range = pl.frange(self._trace.x_min, self._trace.x_max, bin_size_s)
            irqs.hist(ax=axes, column='timestamp', bins=bin_range,
                      grid=True, facecolor='green', alpha=0.5, edgecolor="b",
                      range=(self._trace.x_min, self._trace.x_max))
        else:
            self._log.warning('no irq events to plot')
            return

        axes.set_title("irq_name = " + str(name) + ", bin size (s) " + str(bin_size_s))
        axes.set_xlim(self._trace.x_min, self._trace.x_max)
        axes.set_ylabel('number of IRQs')
        axes.set_xlabel('seconds')
        axes.grid(True)

        figname = '{}/{}{}.png'\
            .format(self._trace.plots_dir, self._trace.plots_prefix, "irq_" + str(irq_number) + "_" + str(name))
        pl.savefig(figname, bbox_inches='tight')
