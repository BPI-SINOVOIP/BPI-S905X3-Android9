# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from autotest_lib.client.common_lib import utils


def has_hal(hal_library_name, host=None, accept_64bit=True, accept_32bit=True):
    """Detect if the host has the given HAL.

    Note that a board can have several HALs, even of a same type.  libhardware
    picks among them at runtime based on values in system properties.  So if
    hal_library_name == 'gralloc', we might find that we have both
    gralloc.brilloemulator.so and gralloc.default.so.  This function will
    not speculate about which will be loaded at runtime.

    @param hal_library_name: string name of the hal (e.g. gralloc or camera).
    @param host: optional host object representing a remote DUT.  If None,
            then we'll look for the HAL on localhost.
    @param accept_64bit: True iff a 64 bit version of the library suffices.
    @param accept_32bit: True iff a 32 bit version of the library suffices.

    @return True iff an appropriate library is found on the device.

    """
    run = utils.run if host is None else host.run
    paths = []
    if accept_64bit:
        paths.append('/system/lib64/hw')
    if accept_32bit:
        paths.append('/system/lib/hw')
    for path in paths:
        result = run('find %s -name %s.*.so 2>/dev/null' %
                     (path, hal_library_name), ignore_status=True)
        if result.exit_status == 0 and result.stdout.strip():
            return True

    return False
