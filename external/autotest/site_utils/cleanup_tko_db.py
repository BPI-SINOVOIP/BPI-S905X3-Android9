#!/usr/bin/python
# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Utility to cleanup TKO database by removing old records.
"""

import argparse
import logging
import os
import time

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import logging_config

from chromite.lib import metrics
from chromite.lib import ts_mon_config


CONFIG = global_config.global_config

# SQL command to remove old test results in TKO database.
CLEANUP_TKO_CMD = 'call remove_old_tests_sp()'
CLEANUP_METRIC = 'chromeos/autotest/tko/cleanup_duration'


def parse_options():
    """Parse command line inputs.

    @return: Options to run the script.
    """
    parser = argparse.ArgumentParser()
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

    with ts_mon_config.SetupTsMonGlobalState(service_name='cleanup_tko_db',
                                             indirect=True):
        server = CONFIG.get_config_value(
                    'AUTOTEST_WEB', 'global_db_host',
                    default=CONFIG.get_config_value('AUTOTEST_WEB', 'host'))
        user = CONFIG.get_config_value(
                    'AUTOTEST_WEB', 'global_db_user',
                    default=CONFIG.get_config_value('AUTOTEST_WEB', 'user'))
        password = CONFIG.get_config_value(
                    'AUTOTEST_WEB', 'global_db_password',
                    default=CONFIG.get_config_value('AUTOTEST_WEB', 'password'))
        database = CONFIG.get_config_value(
                    'AUTOTEST_WEB', 'global_db_database',
                    default=CONFIG.get_config_value('AUTOTEST_WEB', 'database'))

        logging.info('Starting cleaning up old records in TKO database %s on '
                     'server %s.', database, server)

        start_time = time.time()
        try:
            with metrics.SecondsTimer(CLEANUP_METRIC,
                                      fields={'success': False}) as fields:
                utils.run_sql_cmd(server, user, password, CLEANUP_TKO_CMD,
                                  database)
                fields['success'] = True
        except:
            logging.exception('Cleanup failed with exception.')
        finally:
            duration = time.time() - start_time
            logging.info('Cleanup attempt finished in %s seconds.', duration)


if __name__ == '__main__':
    main()
