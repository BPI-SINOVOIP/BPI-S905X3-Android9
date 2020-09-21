# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This file contains things that are shared by arc.py and arc_util.py.

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error


# Ask Chrome to start ARC instance and the script will block until ARC's boot
# completed event.
ARC_MODE_ENABLED = "enabled"
# Similar to "enabled", except that it will not block.
ARC_MODE_ENABLED_ASYNC = "enabled_async"
# Ask Chrome to not start ARC instance.  This is the default.
ARC_MODE_DISABLED = "disabled"
# All available ARC options.
ARC_MODES = [ARC_MODE_ENABLED, ARC_MODE_ENABLED_ASYNC, ARC_MODE_DISABLED]

_BOOT_CHECK_INTERVAL_SECONDS = 2
_WAIT_FOR_ANDROID_BOOT_SECONDS = 120


def wait_for_android_boot(timeout=None):
    """Sleep until Android has completed booting or timeout occurs."""
    if timeout is None:
        timeout = _WAIT_FOR_ANDROID_BOOT_SECONDS

    def _is_android_booted():
        output = utils.system_output(
            'android-sh -c "getprop sys.boot_completed"', ignore_status=True)
        return output.strip() == '1'

    logging.info('Waiting for Android to boot completely.')
    utils.poll_for_condition(condition=_is_android_booted,
                             desc='Android has booted',
                             timeout=timeout,
                             exception=error.TestFail('Android did not boot!'),
                             sleep_interval=_BOOT_CHECK_INTERVAL_SECONDS)
    logging.info('Android has booted completely.')
