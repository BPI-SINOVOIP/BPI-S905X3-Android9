#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility to check the replication delay of the slave databases.

The utility checks the value of Seconds_Behind_Master of slave databases,
including:
Slave databases of AFE database, retrieved from server database.
Readonly replicas of TKO database, passed in by option --replicas.
"""

import argparse
import logging
import os
import re

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import logging_config
from autotest_lib.frontend import setup_django_environment
from autotest_lib.site_utils import server_manager_utils

from chromite.lib import metrics


CONFIG = global_config.global_config

# SQL command to remove old test results in TKO database.
SLAVE_STATUS_CMD = 'show slave status\G'
DELAY_TIME_REGEX = 'Seconds_Behind_Master:\s(\d+)'
DELAY_METRICS = 'chromeos/autotest/database/seconds_behind_master'
# A large delay to report to metrics indicating the replica is in error.
LARGE_DELAY = 1000000

def check_delay(server, user, password):
    """Check the delay of a given slave database server.

    @param server: Hostname or IP address of the MySQL server.
    @param user: User name to log in the MySQL server.
    @param password: Password to log in the MySQL server.
    """
    try:
        result = utils.run_sql_cmd(server, user, password, SLAVE_STATUS_CMD)
        search = re.search(DELAY_TIME_REGEX, result, re.MULTILINE)
        if search:
            delay = int(search.group(1))
            metrics.SecondsDistribution(DELAY_METRICS).add(
                    delay, fields={'server': server})
            logging.debug('Seconds_Behind_Master of server %s is %d.', server,
                          delay)
        else:
            # The value of Seconds_Behind_Master could be NULL, report a large
            # number to indicate database error.
            metrics.SecondsDistribution(DELAY_METRICS).add(
                    LARGE_DELAY, fields={'server': server})
            logging.error('Failed to get Seconds_Behind_Master of server %s '
                          'from slave status:\n %s', server, result)
    except error.CmdError:
        logging.exception('Failed to get slave status of server %s.', server)


def parse_options():
    """Parse command line inputs.

    @return: Options to run the script.
    """
    parser = argparse.ArgumentParser()
    parser.add_argument('-r', '--replicas', nargs='+',
                        help='IP addresses of readonly replicas of TKO.')
    parser.add_argument('-l', '--logfile', type=str,
                        default=None,
                        help='Path to the log file to save logs.')
    return parser.parse_args()


def main():
    """Main script."""
    options = parse_options()
    log_config = logging_config.LoggingConfig()
    if options.logfile:
        log_config.add_file_handler(
                file_path=os.path.abspath(options.logfile), level=logging.DEBUG)

    db_user = CONFIG.get_config_value('AUTOTEST_WEB', 'user')
    db_password = CONFIG.get_config_value('AUTOTEST_WEB', 'password')

    global_db_user = CONFIG.get_config_value(
                'AUTOTEST_WEB', 'global_db_user', default=db_user)
    global_db_password = CONFIG.get_config_value(
                'AUTOTEST_WEB', 'global_db_password', default=db_password)

    logging.info('Start checking Seconds_Behind_Master of slave databases')

    for replica in options.replicas:
        check_delay(replica, global_db_user, global_db_password)

    slaves = server_manager_utils.get_servers(
            role='database_slave', status='primary')
    for slave in slaves:
        check_delay(slave.hostname, db_user, db_password)

    logging.info('Finished checking.')


if __name__ == '__main__':
    main()
