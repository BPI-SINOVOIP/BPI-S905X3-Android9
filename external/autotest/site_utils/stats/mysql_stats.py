#!/usr/bin/python

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Queries a MySQL database and emits status metrics to Monarch.

Note: confusingly, 'Innodb_buffer_pool_reads' is actually the cache-misses, not
the number of reads to the buffer pool.  'Innodb_buffer_pool_read_requests'
corresponds to the number of reads the the buffer pool.
"""
import logging
import sys

import MySQLdb
import time

import common

from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.cros import retry

from chromite.lib import metrics
from chromite.lib import ts_mon_config

AT_DIR='/usr/local/autotest'
DEFAULT_USER = global_config.global_config.get_config_value(
        'CROS', 'db_backup_user', type=str, default='')
DEFAULT_PASSWD = global_config.global_config.get_config_value(
        'CROS', 'db_backup_password', type=str, default='')

LOOP_INTERVAL = 60

EMITTED_STATUSES_COUNTERS = [
        'bytes_received',
        'bytes_sent',
        'connections',
        'Innodb_buffer_pool_read_requests',
        'Innodb_buffer_pool_reads',
        'Innodb_row_lock_waits',
        'questions',
        'slow_queries',
        'threads_created',
]

EMITTED_STATUS_GAUGES = [
        'Innodb_row_lock_time_avg',
        'Innodb_row_lock_current_waits',
        'threads_running',
        'threads_connected',
]


class RetryingConnection(object):
    """Maintains a db connection and a cursor."""
    INITIAL_SLEEP_SECONDS = 20
    MAX_TIMEOUT_SECONDS = 60 * 60

    def __init__(self, *args, **kwargs):
        self.args = args
        self.kwargs = kwargs
        self.db = None
        self.cursor = None

    def Connect(self):
        """Establishes a MySQL connection and creates a cursor."""
        self.db = MySQLdb.connect(*self.args, **self.kwargs)
        self.cursor = self.db.cursor()

    def Reconnect(self):
        """Attempts to close the connection, then reconnects."""
        try:
            self.cursor.close()
            self.db.close()
        except MySQLdb.Error:
            pass
        self.Connect()

    def RetryWith(self, func):
        """Run a function, retrying on OperationalError."""
        return retry.retry(
            MySQLdb.OperationalError,
            delay_sec=self.INITIAL_SLEEP_SECONDS,
            timeout_min=self.MAX_TIMEOUT_SECONDS,
            callback=self.Reconnect
        )(func)()

    def Execute(self, *args, **kwargs):
        """Runs .execute on the cursor, reconnecting on failure."""
        def _Execute():
            return self.cursor.execute(*args, **kwargs)
        return self.RetryWith(_Execute)

    def Fetchall(self):
        """Runs .fetchall on the cursor."""
        return self.cursor.fetchall()


def GetStatus(connection, status):
    """Get the status variable from the database, retrying on failure.

    @param connection: MySQLdb cursor to query with.
    @param status: Name of the status variable.
    @returns The mysql query result.
    """
    connection.Execute('SHOW GLOBAL STATUS LIKE "%s";' % status)
    output = connection.Fetchall()[0][1]

    if not output:
        logging.error('Cannot find any global status like %s', status)

    return int(output)


def QueryAndEmit(baselines, conn):
    """Queries MySQL for important stats and emits Monarch metrics

    @param baselines: A dict containing the initial values for the cumulative
                      metrics.
    @param conn: The mysql connection object.
    """
    for status in EMITTED_STATUSES_COUNTERS:
        metric_name = 'chromeos/autotest/afe_db/%s' % status.lower()
        delta = GetStatus(conn, status) - baselines[status]
        metrics.Counter(metric_name).set(delta)

    for status in EMITTED_STATUS_GAUGES:
        metric_name = 'chromeos/autotest/afe_db/%s' % status.lower()
        metrics.Gauge(metric_name).set(GetStatus(conn, status))

    pages_free = GetStatus(conn, 'Innodb_buffer_pool_pages_free')
    pages_total = GetStatus(conn, 'Innodb_buffer_pool_pages_total')

    metrics.Gauge('chromeos/autotest/afe_db/buffer_pool_pages').set(
        pages_free, fields={'used': False})

    metrics.Gauge('chromeos/autotest/afe_db/buffer_pool_pages').set(
        pages_total - pages_free, fields={'used': True})


def main():
    """Sets up ts_mon and repeatedly queries MySQL stats"""
    logging.basicConfig(stream=sys.stdout, level=logging.INFO)
    conn = RetryingConnection('localhost', DEFAULT_USER, DEFAULT_PASSWD)
    conn.Connect()

    # TODO(crbug.com/803566) Use indirect=False to mitigate orphan mysql_stats
    # processes overwhelming shards.
    with ts_mon_config.SetupTsMonGlobalState('mysql_stats', indirect=False):
      QueryLoop(conn)


def QueryLoop(conn):
    """Queries and emits metrics every LOOP_INTERVAL seconds.

    @param conn: The mysql connection object.
    """
    # Get the baselines for cumulative metrics. Otherwise the windowed rate at
    # the very beginning will be extremely high as it shoots up from 0 to its
    # current value.
    baselines = dict((s, GetStatus(conn, s))
                     for s in EMITTED_STATUSES_COUNTERS)

    while True:
        now = time.time()
        QueryAndEmit(baselines, conn)
        time_spent = time.time() - now
        sleep_duration = LOOP_INTERVAL - time_spent
        time.sleep(max(0, sleep_duration))


if __name__ == '__main__':
  main()
