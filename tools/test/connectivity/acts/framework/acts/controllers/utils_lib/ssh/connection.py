# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import collections
import os
import re
import shutil
import tempfile
import threading
import time
import uuid

from acts import logger
from acts.controllers.utils_lib import host_utils
from acts.controllers.utils_lib.ssh import formatter
from acts.libs.proc import job


class Error(Exception):
    """An error occurred during an ssh operation."""


class CommandError(Exception):
    """An error occurred with the command.

    Attributes:
        result: The results of the ssh command that had the error.
    """

    def __init__(self, result):
        """
        Args:
            result: The result of the ssh command that created the problem.
        """
        self.result = result

    def __str__(self):
        return 'cmd: %s\nstdout: %s\nstderr: %s' % (self.result.command,
                                                    self.result.stdout,
                                                    self.result.stderr)


_Tunnel = collections.namedtuple('_Tunnel',
                                 ['local_port', 'remote_port', 'proc'])


class SshConnection(object):
    """Provides a connection to a remote machine through ssh.

    Provides the ability to connect to a remote machine and execute a command
    on it. The connection will try to establish a persistent connection When
    a command is run. If the persistent connection fails it will attempt
    to connect normally.
    """

    @property
    def socket_path(self):
        """Returns: The os path to the master socket file."""
        return os.path.join(self._master_ssh_tempdir, 'socket')

    def __init__(self, settings):
        """
        Args:
            settings: The ssh settings to use for this connection.
            formatter: The object that will handle formatting ssh command
                       for use with the background job.
        """
        self._settings = settings
        self._formatter = formatter.SshFormatter()
        self._lock = threading.Lock()
        self._master_ssh_proc = None
        self._master_ssh_tempdir = None
        self._tunnels = list()

        def log_line(msg):
            return '[SshConnection | %s] %s' % (self._settings.hostname, msg)

        self.log = logger.create_logger(log_line)

    def __del__(self):
        self.close()

    def setup_master_ssh(self, timeout_seconds=5):
        """Sets up the master ssh connection.

        Sets up the initial master ssh connection if it has not already been
        started.

        Args:
            timeout_seconds: The time to wait for the master ssh connection to
            be made.

        Raises:
            Error: When setting up the master ssh connection fails.
        """
        with self._lock:
            if self._master_ssh_proc is not None:
                socket_path = self.socket_path
                if (not os.path.exists(socket_path) or
                        self._master_ssh_proc.poll() is not None):
                    self.log.debug('Master ssh connection to %s is down.',
                                   self._settings.hostname)
                    self._cleanup_master_ssh()

            if self._master_ssh_proc is None:
                # Create a shared socket in a temp location.
                self._master_ssh_tempdir = tempfile.mkdtemp(
                    prefix='ssh-master')

                # Setup flags and options for running the master ssh
                # -N: Do not execute a remote command.
                # ControlMaster: Spawn a master connection.
                # ControlPath: The master connection socket path.
                extra_flags = {'-N': None}
                extra_options = {
                    'ControlMaster': True,
                    'ControlPath': self.socket_path,
                    'BatchMode': True
                }

                # Construct the command and start it.
                master_cmd = self._formatter.format_ssh_local_command(
                    self._settings,
                    extra_flags=extra_flags,
                    extra_options=extra_options)
                self.log.info('Starting master ssh connection.')
                self._master_ssh_proc = job.run_async(master_cmd)

                end_time = time.time() + timeout_seconds

                while time.time() < end_time:
                    if os.path.exists(self.socket_path):
                        break
                    time.sleep(.2)
                else:
                    self._cleanup_master_ssh()
                    raise Error('Master ssh connection timed out.')

    def run(self,
            command,
            timeout=3600,
            ignore_status=False,
            env=None,
            io_encoding='utf-8',
            attempts=2):
        """Runs a remote command over ssh.

        Will ssh to a remote host and run a command. This method will
        block until the remote command is finished.

        Args:
            command: The command to execute over ssh. Can be either a string
                     or a list.
            timeout: number seconds to wait for command to finish.
            ignore_status: bool True to ignore the exit code of the remote
                           subprocess.  Note that if you do ignore status codes,
                           you should handle non-zero exit codes explicitly.
            env: dict environment variables to setup on the remote host.
            io_encoding: str unicode encoding of command output.
            attempts: Number of attempts before giving up on command failures.

        Returns:
            A job.Result containing the results of the ssh command.

        Raises:
            job.TimeoutError: When the remote command took to long to execute.
            Error: When the ssh connection failed to be created.
            CommandError: Ssh worked, but the command had an error executing.
        """
        if attempts == 0:
            return None
        if env is None:
            env = {}

        try:
            self.setup_master_ssh(self._settings.connect_timeout)
        except Error:
            self.log.warning('Failed to create master ssh connection, using '
                             'normal ssh connection.')

        extra_options = {'BatchMode': True}
        if self._master_ssh_proc:
            extra_options['ControlPath'] = self.socket_path

        identifier = str(uuid.uuid4())
        full_command = 'echo "CONNECTED: %s"; %s' % (identifier, command)

        terminal_command = self._formatter.format_command(
            full_command, env, self._settings, extra_options=extra_options)

        dns_retry_count = 2
        while True:
            result = job.run(
                terminal_command, ignore_status=True, timeout=timeout)
            output = result.stdout

            # Check for a connected message to prevent false negatives.
            valid_connection = re.search(
                '^CONNECTED: %s' % identifier, output, flags=re.MULTILINE)
            if valid_connection:
                # Remove the first line that contains the connect message.
                line_index = output.find('\n')
                real_output = output[line_index + 1:].encode(result._encoding)

                result = job.Result(
                    command=result.command,
                    stdout=real_output,
                    stderr=result._raw_stderr,
                    exit_status=result.exit_status,
                    duration=result.duration,
                    did_timeout=result.did_timeout,
                    encoding=result._encoding)
                if result.exit_status and not ignore_status:
                    raise job.Error(result)
                return result

            error_string = result.stderr

            had_dns_failure = (result.exit_status == 255 and re.search(
                r'^ssh: .*: Name or service not known',
                error_string,
                flags=re.MULTILINE))
            if had_dns_failure:
                dns_retry_count -= 1
                if not dns_retry_count:
                    raise Error('DNS failed to find host.', result)
                self.log.debug('Failed to connect to host, retrying...')
            else:
                break

        had_timeout = re.search(
            r'^ssh: connect to host .* port .*: '
            r'Connection timed out\r$',
            error_string,
            flags=re.MULTILINE)
        if had_timeout:
            raise Error('Ssh timed out.', result)

        permission_denied = 'Permission denied' in error_string
        if permission_denied:
            raise Error('Permission denied.', result)

        unknown_host = re.search(
            r'ssh: Could not resolve hostname .*: '
            r'Name or service not known',
            error_string,
            flags=re.MULTILINE)
        if unknown_host:
            raise Error('Unknown host.', result)

        self.log.error('An unknown error has occurred. Job result: %s' % result)
        ping_output = job.run(
            'ping %s -c 3 -w 1' % self._settings.hostname, ignore_status=True)
        self.log.error('Ping result: %s' % ping_output)
        if attempts > 1:
            self._cleanup_master_ssh()
            self.run(command, timeout, ignore_status, env, io_encoding,
                     attempts - 1)
        raise Error('The job failed for unknown reasons.', result)

    def run_async(self, command, env=None):
        """Starts up a background command over ssh.

        Will ssh to a remote host and startup a command. This method will
        block until there is confirmation that the remote command has started.

        Args:
            command: The command to execute over ssh. Can be either a string
                     or a list.
            env: A dictonary of environment variables to setup on the remote
                 host.

        Returns:
            The result of the command to launch the background job.

        Raises:
            CmdTimeoutError: When the remote command took to long to execute.
            SshTimeoutError: When the connection took to long to established.
            SshPermissionDeniedError: When permission is not allowed on the
                                      remote host.
        """
        command = '(%s) < /dev/null > /dev/null 2>&1 & echo -n $!' % command
        result = self.run(command, env=env)
        return result

    def close(self):
        """Clean up open connections to remote host."""
        self._cleanup_master_ssh()
        while self._tunnels:
            self.close_ssh_tunnel(self._tunnels[0].local_port)

    def _cleanup_master_ssh(self):
        """
        Release all resources (process, temporary directory) used by an active
        master SSH connection.
        """
        # If a master SSH connection is running, kill it.
        if self._master_ssh_proc is not None:
            self.log.debug('Nuking master_ssh_job.')
            self._master_ssh_proc.kill()
            self._master_ssh_proc.wait()
            self._master_ssh_proc = None

        # Remove the temporary directory for the master SSH socket.
        if self._master_ssh_tempdir is not None:
            self.log.debug('Cleaning master_ssh_tempdir.')
            shutil.rmtree(self._master_ssh_tempdir)
            self._master_ssh_tempdir = None

    def create_ssh_tunnel(self, port, local_port=None):
        """Create an ssh tunnel from local_port to port.

        This securely forwards traffic from local_port on this machine to the
        remote SSH host at port.

        Args:
            port: remote port on the host.
            local_port: local forwarding port, or None to pick an available
                        port.

        Returns:
            the created tunnel process.
        """
        if local_port is None:
            local_port = host_utils.get_available_host_port()
        else:
            for tunnel in self._tunnels:
                if tunnel.remote_port == port:
                    return tunnel.local_port

        extra_flags = {
            '-n': None,  # Read from /dev/null for stdin
            '-N': None,  # Do not execute a remote command
            '-q': None,  # Suppress warnings and diagnostic commands
            '-L': '%d:localhost:%d' % (local_port, port),
        }
        extra_options = dict()
        if self._master_ssh_proc:
            extra_options['ControlPath'] = self.socket_path
        tunnel_cmd = self._formatter.format_ssh_local_command(
            self._settings,
            extra_flags=extra_flags,
            extra_options=extra_options)
        self.log.debug('Full tunnel command: %s', tunnel_cmd)
        # Exec the ssh process directly so that when we deliver signals, we
        # deliver them straight to the child process.
        tunnel_proc = job.run_async(tunnel_cmd)
        self.log.debug('Started ssh tunnel, local = %d'
                       ' remote = %d, pid = %d', local_port, port,
                       tunnel_proc.pid)
        self._tunnels.append(_Tunnel(local_port, port, tunnel_proc))
        return local_port

    def close_ssh_tunnel(self, local_port):
        """Close a previously created ssh tunnel of a TCP port.

        Args:
            local_port: int port on localhost previously forwarded to the remote
                        host.

        Returns:
            integer port number this port was forwarded to on the remote host or
            None if no tunnel was found.
        """
        idx = None
        for i, tunnel in enumerate(self._tunnels):
            if tunnel.local_port == local_port:
                idx = i
                break
        if idx is not None:
            tunnel = self._tunnels.pop(idx)
            tunnel.proc.kill()
            tunnel.proc.wait()
            return tunnel.remote_port
        return None

    def send_file(self, local_path, remote_path):
        """Send a file from the local host to the remote host.

        Args:
            local_path: string path of file to send on local host.
            remote_path: string path to copy file to on remote host.
        """
        # TODO: This may belong somewhere else: b/32572515
        user_host = self._formatter.format_host_name(self._settings)
        job.run('scp %s %s:%s' % (local_path, user_host, remote_path))

    def find_free_port(self, interface_name='localhost'):
        """Find a unused port on the remote host.

        Note that this method is inherently racy, since it is impossible
        to promise that the remote port will remain free.

        Args:
            interface_name: string name of interface to check whether a
                            port is used against.

        Returns:
            integer port number on remote interface that was free.
        """
        # TODO: This may belong somewhere else: b/3257251
        free_port_cmd = (
                            'python -c "import socket; s=socket.socket(); '
                            's.bind((\'%s\', 0)); print(s.getsockname()[1]); s.close()"'
                        ) % interface_name
        port = int(self.run(free_port_cmd).stdout)
        # Yield to the os to ensure the port gets cleaned up.
        time.sleep(0.001)
        return port
