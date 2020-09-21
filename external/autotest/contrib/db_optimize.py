#!/usr/bin/python
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This script will run optimize table for chromeos_autotest_db

This script might have notable impact on the mysql performance as it locks
tables and rebuilds indexes. So be careful when running it on production
systems.
"""

import argparse
import logging
import socket
import subprocess
import sys

import common
from autotest_lib.frontend import database_settings_helper
from autotest_lib.scheduler import email_manager
from autotest_lib.server import utils

# Format Appears as: [Date] [Time] - [Msg Level] - [Message]
LOGGING_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'
STATS_KEY = 'db_optimize.%s' % socket.gethostname()

def main_without_exception_handling():
    database_settings = database_settings_helper.get_default_db_config()
    command = ['mysqlcheck',
               '-o', database_settings['NAME'],
               '-u', database_settings['USER'],
               '-p%s' % database_settings['PASSWORD'],
               # we want to do db optimation on each master/slave
               # in rotation. Do not write otimize table to bin log
               # so that it won't be picked up by slaves automatically
               '--skip-write-binlog',
               ]
    subprocess.check_call(command)


def should_optimize():
    """Check if the server should run db_optimize.

    Only shard should optimize db.

    @returns: True if it should optimize db otherwise False.
    """
    return utils.is_shard()


def parse_args():
    """Parse command line arguments"""
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--check_server', action='store_true',
                        help='Check if the server should optimize db.')
    return parser.parse_args()


def main():
    """Main."""
    args = parse_args()

    logging.basicConfig(level=logging.INFO, format=LOGGING_FORMAT)
    logging.info('Calling: %s', sys.argv)

    if args.check_server and not should_optimize():
        print 'Only shard can run db optimization.'
        return

    try:
        main_without_exception_handling()
    except Exception as e:
        message = 'Uncaught exception; terminating db_optimize.'
        email_manager.manager.log_stacktrace(message)
        logging.exception(message)
        raise
    finally:
        email_manager.manager.send_queued_emails()
    logging.info('db_optimize completed.')


if __name__ == '__main__':
    main()
