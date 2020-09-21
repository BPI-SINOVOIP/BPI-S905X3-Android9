# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import time

from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import utils

def pkill_process(process_name, is_full_name=True,
                  timeout_seconds=60, host=None,
                  ignore_status=False):
    """Run pkill against a process until it dies.

    @param process_name: the name of a process.
    @param is_full_name: True iff the value of |process_name| is the complete
            name of the process as understood by pkill.
    @param timeout_seconds: number of seconds to wait for proceess to die.
    @param host: host object to kill the process on.  Defaults to killing
            processes on our localhost.
    @param ignore_status: True iff we should ignore whether we actually
            managed to kill the given process.

    """
    run = host.run if host is not None else utils.run
    full_flag = '-f' if is_full_name else ''
    kill_cmd = 'pkill %s "%s"' % (full_flag, process_name)

    result = run(kill_cmd, ignore_status=True)
    start_time = time.time()
    while (0 == result.exit_status and
            time.time() - start_time < timeout_seconds):
        time.sleep(0.3)
        result = run(kill_cmd, ignore_status=True)

    if result.exit_status == 0 and not ignore_status:
        r = run('cat /proc/`pgrep %s`/status' % process_name,
                ignore_status=True)
        raise error.TestError('Failed to kill proccess "%s":\n%s' %
                (process_name, r.stdout))
