# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from math import ceil, floor, sqrt


def get_kth_percentile(num_list, k):
    """
    Computes the k-th percentile of a list of numbers.

    @param num_list: list with numbers.
    @param k: the percentile to be computed.

    @returns the kth percentile value of the list.

    """
    if not num_list:
        return 0
    assert k >= 0 and k <= 1
    i = k * (len(num_list) - 1)
    c, f = int(ceil(i)), int(floor(i))
    if c == f:
        return num_list[c]
    return round((i - f) * num_list[c] + (c - i) * num_list[f], 2)


def get_median(num_list):
    """
    Computes the median of a list of numbers.

    @param num_list: a list with numbers.

    @returns the median value of the numbers.

    """
    if not num_list:
        return 0
    num_list.sort()
    size = len(num_list)
    if size % 2 != 0:
        return num_list[size / 2]
    return round((num_list[size / 2] + num_list[size / 2 - 1]) / 2.0, 2)


def get_average(num_list):
    """
    Computes mean of a list of numbers.

    @param num_list: a list with numbers.

    @returns the average value computed from the list of numbers.

    """
    if not num_list:
        return 0
    return round(float(sum(num_list)) / len(num_list) , 2)


def get_std_dev(num_list):
    """
    Computes standard deviation of a list of numbers.

    @param num_list: a list with numbers.

    @returns Standard deviation computed from the list of numbers.

    """
    n = len(num_list)
    if not num_list or n == 1:
        return 0
    mean = float(sum(num_list)) / n
    variance = sum([(elem - mean) ** 2 for elem in num_list]) / (n -1)
    return round(sqrt(variance), 2)