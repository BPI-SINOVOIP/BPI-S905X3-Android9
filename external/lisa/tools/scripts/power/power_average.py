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
import argparse
import pandas as pd
import numpy as np

from bart.common.Utils import area_under_curve

# Get averages from a sample csv file for a certain time interval.
# This can be used to find the power averages during a time interval of
# interest. For example, when trying to compute the power average during
# suspend, the first portion of the samples collected should be ignored.

class PowerAverage:
    @staticmethod
    def get(path, column=None, sample_rate_hz=None, start=None, end=None,
            remove_outliers=False):

        # Read csv into dataframe
        with open(path) as f:
            df = pd.read_csv(f)

        if start or end:
            # The rate is needed to calculate the time interval
            if not sample_rate_hz:
                raise RuntimeError('start and/or end cannot be requested without'
                        ' the sample_rate_hz')

            # Change dataframe index from sample index to sample time
            sample_period = 1. / sample_rate_hz
            df.index = np.linspace(0, sample_period * len(df), num=len(df))

            # Drop all samples after the end index
            if end:
                end_idx = np.abs(df.index - end).argmin()
                df.drop(df.index[end_idx:], inplace=True)

            # Drop all samples before the start index
            if start:
                start_idx = np.abs(df.index - start).argmin()
                df.drop(df.index[:start_idx], inplace=True)

        if remove_outliers:
            if not column:
                raise RuntimeError('remove_outliers cannot be requested without'
                        ' a column')
            # Remove the bottom 10% and upper 10% of the data
            percentile = np.percentile(df[column], [10, 90])
            df = df[(df[column] > percentile[0]) & (df[column] < percentile[1])]

        # If no samples remain, throw error
        if df.empty:
            raise RuntimeError('No energy data collected')

        # If a column is specified, only return that column's average
        if column:
            return df[column].mean()

        # Else return the average of each column in the dataframe
        return [ df[column].mean() for column in df ]


parser = argparse.ArgumentParser(
        description="Get the averages from a sample.csv. Optionally specify a"
                    " time interval over which to calculate the sample. If"
                    " start or end is set, sample_rate_hz must also be set")

parser.add_argument("--path", "-p", type=str, required=True,
                    help="Path to file to read samples from.")

parser.add_argument("--column", "-c", type=str, default=None,
                    help="The name of a sample.csv column. When supplied,"
                    " only this column will be averaged.")

parser.add_argument("--sample_rate_hz", "-r", type=int, default=None,
                    help="The sample rate in hz of the given file.")

parser.add_argument("--start", "-s", type=float, default=None,
                    help="The start of the interval.")

parser.add_argument("--end", "-e", type=float, default=None,
                    help="The end of the interval.")

parser.add_argument("--remove_outliers", "-o", type=bool, default=False,
                    help="Remove the outliers from a column."
                    " The column argument must also be specified. (default False)")

if __name__ == "__main__":
    args = parser.parse_args()

    print PowerAverage.get(args.path, args.column, args.sample_rate_hz,
            args.start, args.end, args.remove_outliers)
