# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import logging
import subprocess

from autotest_lib.client.common_lib import error

cpu_list = {
    'Intel(R) Celeron(R) 2955U @ 1.40GHz'     : 'intel_celeron_2955U',
    'Intel(R) Celeron(R) 2957U @ 1.40GHz'     : 'intel_celeron_2957U',
    'Intel(R) Celeron(R) CPU 1007U @ 1.50GHz' : 'intel_celeron_1007U',
    'Intel(R) Celeron(R) CPU 847 @ 1.10GHz'   : 'intel_celeron_847',
    'Intel(R) Celeron(R) CPU 867 @ 1.30GHz'   : 'intel_celeron_867',
    'Intel(R) Celeron(R) CPU 877 @ 1.40GHz'   : 'intel_celeron_877',
    'Intel(R) Celeron(R) CPU B840 @ 1.90GHz'  : 'intel_celeron_B840',
    'Intel(R) Core(TM) i3-4005U CPU @ 1.70GHz': 'intel_i3_4005U',
    'Intel(R) Core(TM) i3-4010U CPU @ 1.70GHz': 'intel_i3_4010U',
    'Intel(R) Core(TM) i3-4030U CPU @ 1.90GHz': 'intel_i3_4030U',
    'Intel(R) Core(TM) i5-2450M CPU @ 2.50GHz': 'intel_i5_2450M',
    'Intel(R) Core(TM) i5-2467M CPU @ 1.60GHz': 'intel_i5_2467M',
    'Intel(R) Core(TM) i5-2520M CPU @ 2.50GHz': 'intel_i5_2520M',
    'Intel(R) Core(TM) i5-3427U CPU @ 1.80GHz': 'intel_i7_3427U',
    'Intel(R) Core(TM) i7-4600U CPU @ 2.10GHz': 'intel_i7_4600U',
}


def detect():
    """
    Returns CPU type. That is 'Model name' column of 'lscpu'.
    For example, 'Intel(R) Core(TM) i5-7Y57 CPU @ 1.20GHz'
    Raise exception if a processor isn't a known one.

    @returns string: CPU type.
    """

    def get_cpu_name(cpu_str):
        # Check if cpu_str is an expected cpu and return an original name
        # describing it. Otherwise an exception is thrown.
        logging.debug('Parse CPU type: %s', cpu_str)
        return cpu_list[cpu_str]

    try:
        lscpu_result = subprocess.check_output(['lscpu'])
    except subprocess.CalledProcessError:
        logging.exception('lscpu failed.')
        raise error.TestFail('Fail to execute "lscpu"')

    logging.debug('The result of lscpu.')
    for cpu_info in lscpu_result.splitlines():
        logging.debug(cpu_info)
        if not ':' in cpu_info:
            continue
        key, value = cpu_info.split(':', 1)
        if key == 'Model name':
            return get_cpu_name(value.strip())

    raise error.TestFail("Couldn't get CPU type.")
