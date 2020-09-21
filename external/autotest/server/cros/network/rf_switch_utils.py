# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import common
import logging

from autotest_lib.server import frontend
from autotest_lib.server import site_utils

RF_SWITCH_LABEL = 'rf_switch'
LOCK_REASON = 'Locked for the tests.'


def allocate_rf_switch():
    """Allocates a RF Switch.

    Locks the available RF Switch if it was discovered via AFE to prevent tests
    stomping on each other.

    @return an instance of AFE host object or None.
    """
    afe = frontend.AFE(
            debug=True, server=site_utils.get_global_afe_hostname())
    # Get a list of hosts with label rf_switch that are not locked.
    rf_switch_hosts = afe.get_hosts(label=RF_SWITCH_LABEL, locked=False)
    if len(rf_switch_hosts) > 0:
        for rf_switch in rf_switch_hosts:
            # Lock the first available RF Switch in the list.
            if afe.lock_host(rf_switch.hostname, LOCK_REASON):
                return rf_switch
            logging.error(
                    'RF Switch %s could not be locked' % rf_switch.hostname)
    else:
        logging.debug('No RF Switches are available for tests.')


def deallocate_rf_switch(rf_switch_host):
    """Unlocks a RF Switch.

    @param rf_switch_host: an instance of AFE Host object to unlock.

    @return True if rf_switch is unlocked, False otherwise.
    """
    afe = frontend.AFE(
            debug=True, server=site_utils.get_global_afe_hostname())
    afe.unlock_hosts([rf_switch_host.hostname])
    rf_switch = afe.get_hosts(hostnames=(rf_switch_host.hostname,))
    return not rf_switch[0].locked
