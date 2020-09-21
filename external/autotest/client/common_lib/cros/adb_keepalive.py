#!/usr/bin/python

# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import logging
import pipes
import re
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import logging_config

_ADB_POLLING_INTERVAL_SECONDS = 10
_ADB_CONNECT_INTERVAL_SECONDS = 1


def _get_adb_options(target, socket):
    """Get adb global options."""
    # ADB 1.0.36 does not support -L adb socket option. Parse the host and port
    # part from the socket instead.
    # https://developer.android.com/studio/command-line/adb.html#issuingcommands
    pattern = r'^[^:]+:([^:]+):(\d+)$'
    match = re.match(pattern, socket)
    if not match:
        raise ValueError('Unrecognized socket format: %s' % socket)
    server_host, server_port = match.groups()
    return '-s %s -H %s -P %s' % (
        pipes.quote(target), pipes.quote(server_host), pipes.quote(server_port))


def _run_adb_cmd(cmd, adb_option="", **kwargs):
    """Run adb command.

    @param cmd: command to issue with adb. (Ex: connect, devices)
    @param target: Device to connect to.
    @param adb_option: adb global option configuration.

    @return: the stdout of the command.
    """
    adb_cmd = 'adb %s %s' % (adb_option, cmd)
    output = utils.system_output(adb_cmd, **kwargs)
    logging.debug('%s: %s', adb_cmd, output)
    return output


def _is_adb_connected(target, adb_option=""):
    """Return true if adb is connected to the container.

    @param target: Device to connect to.
    @param adb_option: adb global option configuration.
    """
    output = _run_adb_cmd(
        'get-state', adb_option=adb_option, ignore_status=True)
    return output.strip() == 'device'


def _ensure_adb_connected(target, adb_option=""):
    """Ensures adb is connected to the container, reconnects otherwise.

    @param target: Device to connect to.
    @param adb_option: adb global options configuration.
    """
    while not _is_adb_connected(target, adb_option):
        logging.info('adb not connected. attempting to reconnect')
        _run_adb_cmd('connect %s' % pipes.quote(target),
                     adb_option=adb_option, ignore_status=True)
        time.sleep(_ADB_CONNECT_INTERVAL_SECONDS)


if __name__ == '__main__':
    logging_config.LoggingConfig().configure_logging(verbose=True)
    parser = argparse.ArgumentParser(description='ensure adb is connected')
    parser.add_argument('target', help='Device to connect to')
    parser.add_argument('--socket', help='ADB server socket.',
                        default='tcp:localhost:5037')
    args = parser.parse_args()
    adb_option = _get_adb_options(args.target, args.socket)

    logging.info('Starting adb_keepalive for target %s on socket %s',
                 args.target, args.socket)
    while True:
        try:
            time.sleep(_ADB_POLLING_INTERVAL_SECONDS)
            _ensure_adb_connected(args.target, adb_option=adb_option)
        except KeyboardInterrupt:
            logging.info('Shutting down')
            break
