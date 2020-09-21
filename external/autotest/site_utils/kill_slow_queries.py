#!/usr/bin/python
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Kill slow queries in local autotest database."""

import logging
import optparse
import sys
import time

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.site_utils import gmail_lib
from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils.stats import mysql_stats

try:
    from chromite.lib import metrics
    from chromite.lib import ts_mon_config
except ImportError:
    metrics = utils.metrics_mock
    ts_mon_config = utils.metrics_mock

AT_DIR='/usr/local/autotest'
DEFAULT_USER = global_config.global_config.get_config_value(
        'CROS', 'db_backup_user', type=str, default='')
DEFAULT_PASSWD = global_config.global_config.get_config_value(
        'CROS', 'db_backup_password', type=str, default='')
DEFAULT_MAIL = global_config.global_config.get_config_value(
        'SCHEDULER', 'notify_email', type=str, default='')


def parse_options():
    """Parse the command line arguments."""
    usage = 'usage: %prog [options]'
    parser = optparse.OptionParser(usage=usage)
    parser.add_option('-u', '--user', default=DEFAULT_USER,
                      help='User to login to the Autotest DB. Default is the '
                           'one defined in config file.')
    parser.add_option('-p', '--password', default=DEFAULT_PASSWD,
                      help='Password to login to the Autotest DB. Default is '
                           'the one defined in config file.')
    parser.add_option('-t', '--timeout', type=int, default=300,
                      help='Timeout boundry of the slow database query. '
                           'Default is 300s')
    parser.add_option('-m', '--mail', default=DEFAULT_MAIL,
                      help='Mail address to send the summary to. Default is '
                           'ChromeOS infra Deputy')
    options, args = parser.parse_args()
    return parser, options, args


def verify_options_and_args(options, args):
    """Verify the validity of options and args.

    @param options: The parsed options to verify.
    @param args: The parsed args to verify.

    @returns: True if verification passes, False otherwise.
    """
    if args:
        logging.error('Unknown arguments: ' + str(args))
        return False

    if not (options.user and options.password):
        logging.error('Failed to get the default user of password for Autotest'
                      ' DB. Please specify them through the command line.')
        return False
    return True


def format_the_output(slow_queries):
    """Convert a list of slow queries into a readable string format.

    e.g. [(a, b, c...)]  -->
         "Id: a
          Host: b
          User: c
          ...
         "
    @param slow_queries: A list of tuples, one tuple contains all the info about
                         one single slow query.

    @returns: one clean string representation of all the slow queries.
    """
    query_str_list = [('Id: %s\nUser: %s\nHost: %s\ndb: %s\nCommand: %s\n'
                       'Time: %s\nState: %s\nInfo: %s\n') %
                      q for q in slow_queries]
    return '\n'.join(query_str_list)


def kill_slow_queries(user, password, timeout):
    """Kill the slow database queries running beyond the timeout limit.

    @param user: User to login to the Autotest DB.
    @param password: Password to login to the Autotest DB.
    @param timeout: Timeout limit to kill the slow queries.

    @returns: a tuple, first element is the string representation of all the
              killed slow queries, second element is the total number of them.
    """
    cursor = mysql_stats.RetryingConnection('localhost', user, password)
    cursor.Connect()

    # Get the processlist.
    cursor.Execute('SHOW FULL PROCESSLIST')
    processlist = cursor.Fetchall()
    # Filter out the slow queries and kill them.
    slow_queries = [p for p in processlist if p[4]=='Query' and p[5]>=timeout]
    queries_str = ''
    num_killed_queries = 0
    if slow_queries:
        queries_str = format_the_output(slow_queries)
        queries_ids = [q[0] for q in slow_queries]
        logging.info('Start killing following slow queries\n%s', queries_str)
        for query_id in queries_ids:
            logging.info('Killing %s...', query_id)
            cursor.Execute('KILL %d' % query_id)
            logging.info('Done!')
            num_killed_queries += 1
    else:
        logging.info('No slow queries over %ds!', timeout)
    return (queries_str, num_killed_queries)


def main():
    """Main entry."""
    # Clear all loggers to make sure the following basicConfig take effect.
    logging.shutdown()
    reload(logging)
    logging.basicConfig(format='%(asctime)s %(message)s',
                        datefmt='%m/%d/%Y %H:%M:%S', level=logging.DEBUG)

    with ts_mon_config.SetupTsMonGlobalState(service_name='kill_slow_queries',
                                             indirect=True):
        count = 0
        parser, options, args = parse_options()
        if not verify_options_and_args(options, args):
            parser.print_help()
            return 1
        try:
            while True:
                result_log_strs, count = kill_slow_queries(
                    options.user, options.password, options.timeout)
                if result_log_strs:
                    gmail_lib.send_email(
                        options.mail,
                        'Successfully killed slow autotest db queries',
                        'Below are killed queries:\n%s' % result_log_strs)
                    m = 'chromeos/autotest/afe_db/killed_slow_queries'
                    metrics.Counter(m).increment_by(count)
                time.sleep(options.timeout)
        except Exception as e:
            m = 'chromeos/autotest/afe_db/failed_to_kill_query'
            metrics.Counter(m).increment()
            logging.error('Failed to kill slow db queries.\n%s', e)
            raise


if __name__ == '__main__':
    sys.exit(main())

