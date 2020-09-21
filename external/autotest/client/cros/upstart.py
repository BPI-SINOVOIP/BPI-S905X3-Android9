# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Provides utility methods for interacting with upstart"""

import os

from autotest_lib.client.common_lib import utils

def ensure_running(service_name):
    """Fails if |service_name| is not running.

    @param service_name: name of the service.
    """
    cmd = 'initctl status %s | grep start/running' % service_name
    utils.system(cmd)


def has_service(service_name):
    """Returns true if |service_name| is installed on the system.

    @param service_name: name of the service.
    """
    return os.path.exists('/etc/init/' + service_name + '.conf')


def is_running(service_name):
    """
    Returns true if |service_name| is running.

    @param service_name: name of service
    """
    return utils.system_output('status %s' % service_name).find('start/running') != -1

def restart_job(service_name):
    """
    Restarts an upstart job if it's running.
    If it's not running, start it.

    @param service_name: name of service
    """

    if is_running(service_name):
        utils.system_output('restart %s' % service_name)
    else:
        utils.system_output('start %s' % service_name)

def stop_job(service_name):
   """
   Stops an upstart job.
   Fails if the stop command fails.

   @param service_name: name of service
   """

   utils.system('stop %s' % service_name)
