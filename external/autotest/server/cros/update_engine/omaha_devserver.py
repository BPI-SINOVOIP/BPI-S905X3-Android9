# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import json
import logging
import socket
import time
import urllib2
import urlparse

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.server import hosts


def snippet(text):
    """Returns the text with start/end snip markers around it.

    @param text: The snippet text.

    @return The text with start/end snip markers around it.
    """
    snip = '---8<---' * 10
    start = '-- START -'
    end = '-- END -'
    return ('%s%s\n%s\n%s%s' %
            (start, snip[len(start):], text, end, snip[len(end):]))


class OmahaDevserverFailedToStart(error.TestError):
    """Raised when a omaha devserver fails to start."""


class OmahaDevserver(object):
    """Spawns a test-private devserver instance."""
    # How long to wait for a devserver to start.
    _WAIT_FOR_DEVSERVER_STARTED_SECONDS = 30

    # How long to sleep (seconds) between checks to see if a devserver is up.
    _WAIT_SLEEP_INTERVAL = 1

    # Max devserver execution time (seconds); used with timeout(1) to ensure we
    # don't have defunct instances hogging the system.
    _DEVSERVER_TIMELIMIT_SECONDS = 12 * 60 * 60


    def __init__(self, omaha_host, update_payload_staged_url, max_updates=1,
                 critical_update=True):
        """Starts a private devserver instance, operating at Omaha capacity.

        @param omaha_host: host address where the devserver is spawned.
        @param update_payload_staged_url: URL to provision for update requests.
        @param max_updates: int number of updates this devserver will handle.
                            This is passed to src/platform/dev/devserver.py.
        @param critical_update: Whether to set a deadline in responses.
        """
        self._devserver_dir = '/home/chromeos-test/chromiumos/src/platform/dev'

        if not update_payload_staged_url:
            raise error.TestError('Missing update payload url')

        self._critical_update = critical_update
        self._max_updates = max_updates
        self._omaha_host = omaha_host
        self._devserver_pid = 0
        self._devserver_port = 0  # Determined later from devserver portfile.
        self._update_payload_staged_url = update_payload_staged_url

        self._devserver_ssh = hosts.SSHHost(self._omaha_host,
                                            user='chromeos-test')

        # Temporary files for various devserver outputs.
        self._devserver_logfile = None
        self._devserver_stdoutfile = None
        self._devserver_portfile = None
        self._devserver_pidfile = None
        self._devserver_static_dir = None


    def _cleanup_devserver_files(self):
        """Cleans up the temporary devserver files."""
        for filename in (self._devserver_logfile, self._devserver_stdoutfile,
                         self._devserver_portfile, self._devserver_pidfile):
            if filename:
                self._devserver_ssh.run('rm -f %s' % filename,
                                        ignore_status=True)

        if self._devserver_static_dir:
            self._devserver_ssh.run('rm -rf %s' % self._devserver_static_dir,
                                    ignore_status=True)


    def _create_tempfile_on_devserver(self, label, dir=False):
        """Creates a temporary file/dir on the devserver and returns its path.

        @param label: Identifier for the file context (string, no whitespaces).
        @param dir: If True, create a directory instead of a file.

        @raises test.TestError: If we failed to invoke mktemp on the server.
        @raises OmahaDevserverFailedToStart: If tempfile creation failed.
        """
        remote_cmd = 'mktemp --tmpdir devserver-%s.XXXXXX' % label
        if dir:
            remote_cmd += ' --directory'

        logging.info(remote_cmd)

        try:
            result = self._devserver_ssh.run(remote_cmd, ignore_status=True,
                                             ssh_failure_retry_ok=True)
        except error.AutoservRunError as e:
            self._log_and_raise_remote_ssh_error(e)
        if result.exit_status != 0:
            logging.info(result)
            raise OmahaDevserverFailedToStart(
                    'Could not create a temporary %s file on the devserver, '
                    'error output: "%s"' % (label, result.stderr))
        return result.stdout.strip()


    @staticmethod
    def _log_and_raise_remote_ssh_error(e):
        """Logs failure to ssh remote, then raises a TestError."""
        logging.debug('Failed to ssh into the devserver: %s', e)
        logging.error('If you are running this locally it means you did not '
                      'configure ssh correctly.')
        raise error.TestError('Failed to ssh into the devserver: %s' % e)


    def _read_int_from_devserver_file(self, filename):
        """Reads and returns an integer value from a file on the devserver."""
        return int(self._get_devserver_file_content(filename).strip())


    def _wait_for_devserver_to_start(self):
        """Waits until the devserver starts within the time limit.

        Infers and sets the devserver PID and serving port.

        Raises:
            OmahaDevserverFailedToStart: If the time limit is reached and we
                                         cannot connect to the devserver.
        """
        # Compute the overall timeout.
        deadline = time.time() + self._WAIT_FOR_DEVSERVER_STARTED_SECONDS

        # First, wait for port file to be filled and determine the server port.
        logging.warning('Waiting for devserver to start up.')
        while time.time() < deadline:
            try:
                self._devserver_pid = self._read_int_from_devserver_file(
                        self._devserver_pidfile)
                self._devserver_port = self._read_int_from_devserver_file(
                        self._devserver_portfile)
                logging.info('Devserver pid is %d, serving on port %d',
                             self._devserver_pid, self._devserver_port)
                break
            except Exception:  # Couldn't read file or corrupt content.
                time.sleep(self._WAIT_SLEEP_INTERVAL)
        else:
            try:
                self._devserver_ssh.run_output('uptime',
                                               ssh_failure_retry_ok=True)
            except error.AutoservRunError as e:
                logging.debug('Failed to run uptime on the devserver: %s', e)
            raise OmahaDevserverFailedToStart(
                    'The test failed to find the pid/port of the omaha '
                    'devserver after %d seconds. Check the dumped devserver '
                    'logs and devserver load for more information.' %
                    self._WAIT_FOR_DEVSERVER_STARTED_SECONDS)

        # Check that the server is reponsding to network requests.
        logging.warning('Waiting for devserver to accept network requests.')
        url = 'http://%s' % self.get_netloc()
        while time.time() < deadline:
            if dev_server.ImageServer.devserver_healthy(url, timeout_min=0.1):
                break

            # TODO(milleral): Refactor once crbug.com/221626 is resolved.
            time.sleep(self._WAIT_SLEEP_INTERVAL)
        else:
            raise OmahaDevserverFailedToStart(
                    'The test failed to establish a connection to the omaha '
                    'devserver it set up on port %d. Check the dumped '
                    'devserver logs for more information.' %
                    self._devserver_port)


    def start_devserver(self):
        """Starts the devserver and confirms it is up.

        Raises:
            test.TestError: If we failed to spawn the remote devserver.
            OmahaDevserverFailedToStart: If the time limit is reached and we
                                         cannot connect to the devserver.
        """
        update_payload_url_base, update_payload_path = self._split_url(
                self._update_payload_staged_url)

        # Allocate temporary files for various server outputs.
        self._devserver_logfile = self._create_tempfile_on_devserver('log')
        self._devserver_stdoutfile = self._create_tempfile_on_devserver(
                'stdout')
        self._devserver_portfile = self._create_tempfile_on_devserver('port')
        self._devserver_pidfile = self._create_tempfile_on_devserver('pid')
        self._devserver_static_dir = self._create_tempfile_on_devserver(
                'static', dir=True)

        # Invoke the Omaha/devserver on the remote server. Will attempt to kill
        # it with a SIGTERM after a predetermined timeout has elapsed, followed
        # by SIGKILL if not dead within 30 seconds from the former signal.
        cmdlist = [
                'timeout', '-s', 'TERM', '-k', '30',
                str(self._DEVSERVER_TIMELIMIT_SECONDS),
                '%s/devserver.py' % self._devserver_dir,
                '--payload=%s' % update_payload_path,
                '--port=0',
                '--pidfile=%s' % self._devserver_pidfile,
                '--portfile=%s' % self._devserver_portfile,
                '--logfile=%s' % self._devserver_logfile,
                '--remote_payload',
                '--urlbase=%s' % update_payload_url_base,
                '--max_updates=%s' % self._max_updates,
                '--host_log',
                '--static_dir=%s' % self._devserver_static_dir
        ]

        if self._critical_update:
            cmdlist.append('--critical_update')

        remote_cmd = '( %s ) </dev/null >%s 2>&1 &' % (
                ' '.join(cmdlist), self._devserver_stdoutfile)

        logging.info('Starting devserver with %r', remote_cmd)
        try:
            self._devserver_ssh.run_output(remote_cmd,
                                           ssh_failure_retry_ok=True)
        except error.AutoservRunError as e:
            self._log_and_raise_remote_ssh_error(e)

        try:
            self._wait_for_devserver_to_start()
        except OmahaDevserverFailedToStart:
            self._kill_remote_process()
            self._dump_devserver_log()
            self._cleanup_devserver_files()
            raise


    def _kill_remote_process(self):
        """Kills the devserver and verifies it's down; clears the remote pid."""
        def devserver_down():
            """Ensure that the devserver process is down."""
            return not self._remote_process_alive()

        if devserver_down():
            return

        for signal in 'SIGTERM', 'SIGKILL':
            remote_cmd = 'kill -s %s %s' % (signal, self._devserver_pid)
            self._devserver_ssh.run(remote_cmd, ssh_failure_retry_ok=True)
            try:
                client_utils.poll_for_condition(
                        devserver_down, sleep_interval=1, desc='devserver down')
                break
            except client_utils.TimeoutError:
                logging.warning('Could not kill devserver with %s.', signal)
        else:
            logging.warning('Failed to kill devserver, giving up.')

        self._devserver_pid = None


    def _remote_process_alive(self):
        """Tests whether the remote devserver process is running."""
        if not self._devserver_pid:
            return False
        remote_cmd = 'test -e /proc/%s' % self._devserver_pid
        result = self._devserver_ssh.run(remote_cmd, ignore_status=True)
        return result.exit_status == 0


    def get_netloc(self):
        """Returns the netloc (host:port) of the devserver."""
        if not (self._devserver_pid and self._devserver_port):
            raise error.TestError('No running omaha/devserver')

        return '%s:%s' % (self._omaha_host, self._devserver_port)


    def get_update_url(self):
        """Returns the update_url you can use to update via this server."""
        return urlparse.urlunsplit(('http', self.get_netloc(), '/update',
                                    '', ''))


    def _get_devserver_file_content(self, filename):
        """Returns the content of a file on the devserver."""
        return self._devserver_ssh.run_output('cat %s' % filename,
                                              stdout_tee=None,
                                              ssh_failure_retry_ok=True)


    def _get_devserver_log(self):
        """Obtain the devserver output."""
        return self._get_devserver_file_content(self._devserver_logfile)


    def _get_devserver_stdout(self):
        """Obtain the devserver output in stdout and stderr."""
        return self._get_devserver_file_content(self._devserver_stdoutfile)


    def get_hostlog(self, ip, wait_for_reboot_events=False):
        """Get the update events json (aka hostlog).

        @param ip: IP of the DUT to get update info for.
        @param wait_for_reboot_events: True if we expect the reboot events.

        @return the json dump of the update events for the given IP.
        """
        omaha_hostlog_url = urlparse.urlunsplit(
            ['http', self.get_netloc(), '/api/hostlog',
             'ip=' + ip, ''])

        # 4 rootfs and 1 post reboot
        expected_events_count = 5

        while True:
            try:
                conn = urllib2.urlopen(omaha_hostlog_url)
            except urllib2.URLError, e:
                logging.warning('Failed to read event log url: %s', e)
                return None
            except socket.timeout, e:
                logging.warning('Timed out reading event log url: %s', e)
                return None

            event_log_resp = conn.read()
            conn.close()
            hostlog = json.loads(event_log_resp)
            if wait_for_reboot_events and len(hostlog) < expected_events_count:
                time.sleep(5)
                continue
            else:
                return hostlog


    def _dump_devserver_log(self, logging_level=logging.ERROR):
        """Dump the devserver log to the autotest log.

        @param logging_level: logging level (from logging) to log the output.
        """
        logging.log(logging_level, "Devserver stdout and stderr:\n" +
                    snippet(self._get_devserver_stdout()))
        logging.log(logging_level, "Devserver log file:\n" +
                    snippet(self._get_devserver_log()))


    @staticmethod
    def _split_url(url):
        """Splits a URL into the URL base and path."""
        split_url = urlparse.urlsplit(url)
        url_base = urlparse.urlunsplit(
                (split_url.scheme, split_url.netloc, '', '', ''))
        url_path = split_url.path
        return url_base, url_path.lstrip('/')


    def stop_devserver(self):
        """Kill remote process and wait for it to die, dump its output."""
        if not self._devserver_pid:
            logging.error('No running omaha/devserver.')
            return

        logging.info('Killing omaha/devserver')
        self._kill_remote_process()
        logging.debug('Final devserver log before killing')
        self._dump_devserver_log(logging.DEBUG)
        self._cleanup_devserver_files()
