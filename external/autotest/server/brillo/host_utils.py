# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utilities used with Brillo hosts."""

import contextlib
import logging

import common
from autotest_lib.client.bin import utils


_RUN_BACKGROUND_TEMPLATE = '( %(cmd)s ) </dev/null >/dev/null 2>&1 & echo -n $!'

_WAIT_CMD_TEMPLATE = """\
to=%(timeout)d; \
while test ${to} -ne 0; do \
  test $(ps %(pid)d | wc -l) -gt 1 || break; \
  sleep 1; \
  to=$((to - 1)); \
done; \
test ${to} -ne 0 -o $(ps %(pid)d | wc -l) -eq 1 \
"""


def run_in_background(host, cmd):
    """Runs a command in the background on the DUT.

    @param host: A host object representing the DUT.
    @param cmd: The command to run.

    @return The background process ID (integer).
    """
    background_cmd = _RUN_BACKGROUND_TEMPLATE % {'cmd': cmd}
    return int(host.run_output(background_cmd).strip())


def wait_for_process(host, pid, timeout=-1):
    """Waits for a process on the DUT to terminate.

    @param host: A host object representing the DUT.
    @param pid: The process ID (integer).
    @param timeout: Number of seconds to wait; default is wait forever.

    @return True if process terminated within the alotted time, False otherwise.
    """
    wait_cmd = _WAIT_CMD_TEMPLATE % {'pid': pid, 'timeout': timeout}
    return host.run(wait_cmd, ignore_status=True).exit_status == 0


@contextlib.contextmanager
def connect_to_ssid(host, ssid, passphrase):
    """Connects to a given ssid.

    @param host: A host object representing the DUT.
    @param ssid: A string representing the ssid to connect to. If ssid is None,
                 assume that host is already connected to wifi.
    @param passphrase: A string representing the passphrase to the ssid.
                       Defaults to None.
    """
    try:
        if ssid is None:
            # No ssid is passed. It is assumed that the host is already
            # connected to wifi.
            logging.warning('This test assumes that the device is connected to '
                            'wifi. If it is not, this test will fail.')
            yield
        else:
            logging.info('Connecting to ssid %s', ssid)
            # Update the weaved init.rc to stop privet. This sets up shill in
            # client mode allowing it to connect to wifi.
            host.remount()
            host.run('sed \'s/service weaved \/system\/bin\/weaved/'
                     'service weaved \/system\/bin\/weaved --disable_privet/\' '
                     '-i /system/etc/init/weaved.rc')
            host.reboot()
            utils.poll_for_condition(
                    lambda: 'running' in host.run('getprop init.svc.shill'
                                                  ).stdout,
                    sleep_interval=1, timeout=300,
                    desc='shill was not started by init')
            logging.info('Connecting to wifi')
            wifi_cmd = ('shill_setup_wifi --ssid=%s '
                        '--wait-for-online-seconds=%i' % (ssid, 300))
            if passphrase:
                wifi_cmd += ' --passphrase=%s' % passphrase
            host.run(wifi_cmd)
            yield
    finally:
        if ssid:
            # If we connected to a ssid, disconnect.
            host.remount()
            host.run('sed \'s/service weaved \/system\/bin\/weaved '
                     '--disable_privet/service weaved \/system\/bin\/weaved/\' '
                     '-i /system/etc/init/weaved.rc')
            host.run('stop weaved')
            host.run('start weaved')
