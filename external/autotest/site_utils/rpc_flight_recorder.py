#!/usr/bin/env python
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Standalone service to monitor AFE servers and report to ts_mon"""
import sys
import time
import multiprocessing
import urllib2

import common
from autotest_lib.client.common_lib import global_config
from autotest_lib.frontend.afe.json_rpc import proxy
from autotest_lib.server import frontend
# import needed to setup host_attributes
# pylint: disable=unused-import
from autotest_lib.server import site_host_attributes
from autotest_lib.site_utils import server_manager_utils
from chromite.lib import commandline
from chromite.lib import cros_logging as logging
from chromite.lib import metrics
from chromite.lib import ts_mon_config

METRIC_ROOT = 'chromeos/autotest/blackbox/afe_rpc'
METRIC_RPC_CALL_DURATIONS = METRIC_ROOT + '/rpc_call_durations'
METRIC_TICK = METRIC_ROOT + '/tick'
METRIC_MONITOR_ERROR = METRIC_ROOT + '/afe_monitor_error'

FAILURE_REASONS = {
        proxy.JSONRPCException: 'JSONRPCException',
        }

def afe_rpc_call(hostname):
    """Perform one rpc call set on server

    @param hostname: server's hostname to poll
    """
    afe_monitor = AfeMonitor(hostname)
    try:
        afe_monitor.run()
    except Exception as e:
        metrics.Counter(METRIC_MONITOR_ERROR).increment(
                fields={'target_hostname': hostname})
        logging.exception(e)


def update_shards(shards, shards_lock, period=600, stop_event=None):
    """Updates dict of shards

    @param shards: list of shards to be updated
    @param shards_lock: shared lock for accessing shards
    @param period: time between polls
    @param stop_event: Event that can be set to stop polling
    """
    while(not stop_event or not stop_event.is_set()):
        start_time = time.time()

        logging.debug('Updating Shards')
        new_shards = set(server_manager_utils.get_shards())

        with shards_lock:
            current_shards = set(shards)
            rm_shards = current_shards - new_shards
            add_shards = new_shards - current_shards

            if rm_shards:
                for s in rm_shards:
                    shards.remove(s)

            if add_shards:
                shards.extend(add_shards)

        if rm_shards:
            logging.info('Servers left production: %s', str(rm_shards))

        if add_shards:
            logging.info('Servers entered production: %s',
                    str(add_shards))

        wait_time = (start_time + period) - time.time()
        if wait_time > 0:
            time.sleep(wait_time)


def poll_rpc_servers(servers, servers_lock, shards=None, period=60,
                     stop_event=None):
    """Blocking function that polls all servers and shards

    @param servers: list of servers to poll
    @param servers_lock: lock to be used when accessing servers or shards
    @param shards: list of shards to poll
    @param period: time between polls
    @param stop_event: Event that can be set to stop polling
    """
    pool = multiprocessing.Pool(processes=multiprocessing.cpu_count() * 4)

    while(not stop_event or not stop_event.is_set()):
        start_time = time.time()
        with servers_lock:
            all_servers = set(servers).union(shards)

        logging.debug('Starting Server Polling: %s', ', '.join(all_servers))
        pool.map(afe_rpc_call, all_servers)

        logging.debug('Finished Server Polling')

        metrics.Counter(METRIC_TICK).increment()

        wait_time = (start_time + period) - time.time()
        if wait_time > 0:
            time.sleep(wait_time)


class RpcFlightRecorder(object):
    """Monitors a list of AFE"""
    def __init__(self, servers, with_shards=True, poll_period=60):
        """
        @param servers: list of afe services to monitor
        @param with_shards: also record status on shards
        @param poll_period: frequency to poll all services, in seconds
        """
        self._manager = multiprocessing.Manager()

        self._poll_period = poll_period

        self._servers = self._manager.list(servers)
        self._servers_lock = self._manager.RLock()

        self._with_shards = with_shards
        self._shards = self._manager.list()
        self._update_shards_ps = None
        self._poll_rpc_server_ps = None

        self._stop_event = multiprocessing.Event()

    def start(self):
        """Call to start recorder"""
        if(self._with_shards):
            shard_args = [self._shards, self._servers_lock]
            shard_kwargs = {'stop_event': self._stop_event}
            self._update_shards_ps = multiprocessing.Process(
                    name='update_shards',
                    target=update_shards,
                    args=shard_args,
                    kwargs=shard_kwargs)

            self._update_shards_ps.start()

        poll_args = [self._servers, self._servers_lock]
        poll_kwargs= {'shards':self._shards,
                     'period':self._poll_period,
                     'stop_event':self._stop_event}
        self._poll_rpc_server_ps = multiprocessing.Process(
                name='poll_rpc_servers',
                target=poll_rpc_servers,
                args=poll_args,
                kwargs=poll_kwargs)

        self._poll_rpc_server_ps.start()

    def close(self):
        """Send close event to all sub processes"""
        self._stop_event.set()


    def termitate(self):
        """Terminate processes"""
        self.close()
        if self._poll_rpc_server_ps:
            self._poll_rpc_server_ps.terminate()

        if self._update_shards_ps:
            self._update_shards_ps.terminate()

        if self._manager:
            self._manager.shutdown()


    def join(self, timeout=None):
        """Blocking call until closed and processes complete

        @param timeout: passed to each process, so could be >timeout"""
        if self._poll_rpc_server_ps:
            self._poll_rpc_server_ps.join(timeout)

        if self._update_shards_ps:
            self._update_shards_ps.join(timeout)

def _failed(fields, msg_str, reason, err=None):
    """Mark current run failed

    @param fields, ts_mon fields to mark as failed
    @param msg_str, message string to be filled
    @param reason: why it failed
    @param err: optional error to log more debug info
    """
    fields['success'] = False
    fields['failure_reason'] = reason
    logging.warning("%s failed - %s", msg_str, reason)
    if err:
        logging.debug("%s fail_err - %s", msg_str, str(err))

class AfeMonitor(object):
    """Object that runs rpc calls against the given afe frontend"""

    def __init__(self, hostname):
        """
        @param hostname: hostname of server to monitor, string
        """
        self._hostname = hostname
        self._afe = frontend.AFE(server=self._hostname)
        self._metric_fields = {'target_hostname': self._hostname}


    def run_cmd(self, cmd, expected=None):
        """Runs rpc command and log metrics

        @param cmd: string of rpc command to send
        @param expected: expected result of rpc
        """
        metric_fields = self._metric_fields.copy()
        metric_fields['command'] = cmd
        metric_fields['success'] = True
        metric_fields['failure_reason'] = ''

        with metrics.SecondsTimer(METRIC_RPC_CALL_DURATIONS,
                fields=dict(metric_fields), scale=0.001) as f:

            msg_str = "%s:%s" % (self._hostname, cmd)


            try:
                result = self._afe.run(cmd)
                logging.debug("%s result = %s", msg_str, result)
                if expected is not None and expected != result:
                    _failed(f, msg_str, 'IncorrectResponse')

            except urllib2.HTTPError as e:
                _failed(f, msg_str, 'HTTPError:%d' % e.code)

            except Exception as e:
                _failed(f, msg_str, FAILURE_REASONS.get(type(e), 'Unknown'),
                        err=e)

                if type(e) not in FAILURE_REASONS:
                    raise

            if f['success']:
                logging.info("%s success", msg_str)


    def run(self):
        """Tests server and returns the result"""
        self.run_cmd('get_server_time')
        self.run_cmd('ping_db', [True])


def get_parser():
    """Returns argparse parser"""
    parser = commandline.ArgumentParser(description=__doc__)

    parser.add_argument('-a', '--afe', action='append', default=[],
                        help='Autotest FrontEnd server to monitor')

    parser.add_argument('-p', '--poll-period', type=int, default=60,
                        help='Frequency to poll AFE servers')

    parser.add_argument('--no-shards', action='store_false', dest='with_shards',
                        help='Disable shard updating')

    return parser


def main(argv):
    """Main function

    @param argv: commandline arguments passed
    """
    parser = get_parser()
    options = parser.parse_args(argv[1:])


    if not options.afe:
        options.afe = [global_config.global_config.get_config_value(
                        'SERVER', 'global_afe_hostname', default='cautotest')]

    with ts_mon_config.SetupTsMonGlobalState('rpc_flight_recorder',
                                             indirect=True):
        flight_recorder = RpcFlightRecorder(options.afe,
                                            with_shards=options.with_shards,
                                            poll_period=options.poll_period)

        flight_recorder.start()
        flight_recorder.join()


if __name__ == '__main__':
    main(sys.argv)
