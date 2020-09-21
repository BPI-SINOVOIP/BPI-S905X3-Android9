#
# Copyright 2007 Google Inc. Released under the GPL v2

"""
This module defines the SSHHost class.

Implementation details:
You should import the "hosts" package instead of importing each type of host.

        SSHHost: a remote machine with a ssh access
"""

import inspect
import logging
import re
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import pxssh
from autotest_lib.server import utils
from autotest_lib.server.hosts import abstract_ssh

# In case cros_host is being ran via SSP on an older Moblab version with an
# older chromite version.
try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


class SSHHost(abstract_ssh.AbstractSSHHost):
    """
    This class represents a remote machine controlled through an ssh
    session on which you can run programs.

    It is not the machine autoserv is running on. The machine must be
    configured for password-less login, for example through public key
    authentication.

    It includes support for controlling the machine through a serial
    console on which you can run programs. If such a serial console is
    set up on the machine then capabilities such as hard reset and
    boot strap monitoring are available. If the machine does not have a
    serial console available then ordinary SSH-based commands will
    still be available, but attempts to use extensions such as
    console logging or hard reset will fail silently.

    Implementation details:
    This is a leaf class in an abstract class hierarchy, it must
    implement the unimplemented methods in parent classes.
    """

    def _initialize(self, hostname, *args, **dargs):
        """
        Construct a SSHHost object

        Args:
                hostname: network hostname or address of remote machine
        """
        super(SSHHost, self)._initialize(hostname=hostname, *args, **dargs)
        self.setup_ssh()


    def ssh_command(self, connect_timeout=30, options='', alive_interval=300,
                    alive_count_max=3, connection_attempts=1):
        """
        Construct an ssh command with proper args for this host.

        @param connect_timeout: connection timeout (in seconds)
        @param options: SSH options
        @param alive_interval: SSH Alive interval.
        @param alive_count_max: SSH AliveCountMax.
        @param connection_attempts: SSH ConnectionAttempts
        """
        options = " ".join([options, self._master_ssh.ssh_option])
        base_cmd = self.make_ssh_command(user=self.user, port=self.port,
                                         opts=options,
                                         hosts_file=self.known_hosts_file,
                                         connect_timeout=connect_timeout,
                                         alive_interval=alive_interval,
                                         alive_count_max=alive_count_max,
                                         connection_attempts=connection_attempts)
        return "%s %s" % (base_cmd, self.hostname)

    def _get_server_stack_state(self, lowest_frames=0, highest_frames=None):
        """ Get the server stack frame status.
        @param lowest_frames: the lowest frames to start printing.
        @param highest_frames: the highest frames to print.
                        (None means no restriction)
        """
        stack_frames = inspect.stack()
        stack = ''
        for frame in stack_frames[lowest_frames:highest_frames]:
            function_name = inspect.getframeinfo(frame[0]).function
            stack = '%s|%s' % (function_name, stack)
        del stack_frames
        return stack[:-1] # Delete the last '|' character

    def _verbose_logger_command(self, command):
        """
        Prepend the command for the client with information about the ssh
        command to be executed and the server stack state.

        @param command: the ssh command to be executed.
        """
        # The last 3 frames on the stack are boring. Print 6-3=3 stack frames.
        stack = self._get_server_stack_state(lowest_frames=3, highest_frames=6)
        # If "logger" executable exists on the DUT use it to respew |command|.
        # Then regardless of "logger" run |command| as usual.
        command = ('if type "logger" > /dev/null 2>&1; then'
                   ' logger -tag "autotest" "server[stack::%s] -> ssh_run(%s)";'
                   'fi; '
                   '%s' % (stack, utils.sh_escape(command), command))
        return command


    def _run(self, command, timeout, ignore_status,
             stdout, stderr, connect_timeout, env, options, stdin, args,
             ignore_timeout, ssh_failure_retry_ok):
        """Helper function for run()."""
        ssh_cmd = self.ssh_command(connect_timeout, options)
        if not env.strip():
            env = ""
        else:
            env = "export %s;" % env
        for arg in args:
            command += ' "%s"' % utils.sh_escape(arg)
        full_cmd = '%s "%s %s"' % (ssh_cmd, env, utils.sh_escape(command))

        # TODO(jrbarnette):  crbug.com/484726 - When we're in an SSP
        # container, sometimes shortly after reboot we will see DNS
        # resolution errors on ssh commands; the problem never
        # occurs more than once in a row.  This especially affects
        # the autoupdate_Rollback test, but other cases have been
        # affected, too.
        #
        # We work around it by detecting the first DNS resolution error
        # and retrying exactly one time.
        dns_error_retry_count = 1

        def counters_inc(counter_name, failure_name):
            """Helper function to increment metrics counters.
            @param counter_name: string indicating which counter to use
            @param failure_name: string indentifying an error, or 'success'
            """
            if counter_name == 'call':
                # ssh_counter records the outcome of each ssh invocation
                # inside _run(), including exceptions.
                ssh_counter = metrics.Counter('chromeos/autotest/ssh/calls')
                fields = {'error' : failure_name or 'success',
                          'attempt' : ssh_call_count}
                ssh_counter.increment(fields=fields)

            if counter_name == 'run':
                # run_counter records each call to _run() with its result
                # and how many tries were made.  Calls are recorded when
                # _run() exits (including exiting with an exception)
                run_counter = metrics.Counter('chromeos/autotest/ssh/runs')
                fields = {'error' : failure_name or 'success',
                          'attempt' : ssh_call_count}
                run_counter.increment(fields=fields)

        # If ssh_failure_retry_ok is True, retry twice on timeouts and generic
        # error 255: if a simple retry doesn't work, kill the ssh master
        # connection and try again.  (Note that either error could come from
        # the command running in the DUT, in which case the retry may be
        # useless but, in theory, also harmless.)
        if ssh_failure_retry_ok:
            # Ignore ssh command timeout, even though it could be a timeout due
            # to the command executing in the remote host.  Note that passing
            # ignore_timeout = True makes utils.run() return None on timeouts
            # (and only on timeouts).
            original_ignore_timeout = ignore_timeout
            ignore_timeout = True
            ssh_failure_retry_count = 2
        else:
            ssh_failure_retry_count = 0

        ssh_call_count = 0

        while True:
            try:
                # Increment call count first, in case utils.run() throws an
                # exception.
                ssh_call_count += 1
                result = utils.run(full_cmd, timeout, True, stdout, stderr,
                                   verbose=False, stdin=stdin,
                                   stderr_is_expected=ignore_status,
                                   ignore_timeout=ignore_timeout)
            except Exception as e:
                # No retries on exception.
                counters_inc('call', 'exception')
                counters_inc('run', 'exception')
                raise e

            failure_name = None

            if result:
                if result.exit_status == 255:
                    if re.search(r'^ssh: .*: Name or service not known',
                                 result.stderr):
                        failure_name = 'dns_failure'
                    else:
                        failure_name = 'error_255'
                elif result.exit_status > 0:
                    failure_name = 'nonzero_status'
            else:
                # result == None
                failure_name = 'timeout'

            # Record the outcome of the ssh invocation.
            counters_inc('call', failure_name)

            if failure_name:
                # There was a failure: decide whether to retry.
                if failure_name == 'dns_failure':
                    if dns_error_retry_count > 0:
                        logging.debug('retrying ssh because of DNS failure')
                        dns_error_retry_count -= 1
                        continue
                else:
                    if ssh_failure_retry_count == 2:
                        logging.debug('retrying ssh command after %s',
                                       failure_name)
                        ssh_failure_retry_count -= 1
                        continue
                    elif ssh_failure_retry_count == 1:
                        # After two failures, restart the master connection
                        # before the final try.
                        logging.debug('retry 2: restarting master connection')
                        self.restart_master_ssh()
                        # Last retry: reinstate timeout behavior.
                        ignore_timeout = original_ignore_timeout
                        ssh_failure_retry_count -= 1
                        continue

            # No retry conditions occurred.  Exit the loop.
            break

        # The outcomes of ssh invocations have been recorded.  Now record
        # the outcome of this function.

        if ignore_timeout and not result:
            counters_inc('run', 'ignored_timeout')
            return None

        # The error messages will show up in band (indistinguishable
        # from stuff sent through the SSH connection), so we have the
        # remote computer echo the message "Connected." before running
        # any command.  Since the following 2 errors have to do with
        # connecting, it's safe to do these checks.
        if result.exit_status == 255:
            if re.search(r'^ssh: connect to host .* port .*: '
                         r'Connection timed out\r$', result.stderr):
                counters_inc('run', 'final_timeout')
                raise error.AutoservSSHTimeout("ssh timed out", result)
            if "Permission denied." in result.stderr:
                msg = "ssh permission denied"
                counters_inc('run', 'final_eperm')
                raise error.AutoservSshPermissionDeniedError(msg, result)

        if not ignore_status and result.exit_status > 0:
            counters_inc('run', 'final_run_error')
            raise error.AutoservRunError("command execution error", result)

        counters_inc('run', failure_name)
        return result


    def run_very_slowly(self, command, timeout=3600, ignore_status=False,
            stdout_tee=utils.TEE_TO_LOGS, stderr_tee=utils.TEE_TO_LOGS,
            connect_timeout=30, options='', stdin=None, verbose=True, args=(),
            ignore_timeout=False, ssh_failure_retry_ok=False):
        """
        Run a command on the remote host.
        This RPC call has an overhead of minimum 40ms and up to 400ms on
        servers (crbug.com/734887). Each time a run_very_slowly is added for
        every job - a server core dies in the lab.
        @see common_lib.hosts.host.run()

        @param timeout: command execution timeout
        @param connect_timeout: ssh connection timeout (in seconds)
        @param options: string with additional ssh command options
        @param verbose: log the commands
        @param ignore_timeout: bool True if SSH command timeouts should be
                ignored.  Will return None on command timeout.
        @param ssh_failure_retry_ok: True if the command may be retried on
                probable ssh failure (error 255 or timeout).  When true,
                the command may be executed up to three times, the second
                time after restarting the ssh master connection.  Use only for
                commands that are idempotent, because when a "probable
                ssh failure" occurs, we cannot tell if the command executed
                or not.

        @raises AutoservRunError: if the command failed
        @raises AutoservSSHTimeout: ssh connection has timed out
        """
        with metrics.SecondsTimer('chromeos/autotest/ssh/master_ssh_time',
                                  scale=0.001):
            if verbose:
                stack = self._get_server_stack_state(lowest_frames=1,
                                                     highest_frames=7)
                logging.debug("Running (ssh) '%s' from '%s'", command, stack)
                command = self._verbose_logger_command(command)

            # Start a master SSH connection if necessary.
            self.start_master_ssh()

            env = " ".join("=".join(pair) for pair in self.env.iteritems())
            try:
                return self._run(command, timeout, ignore_status,
                                 stdout_tee, stderr_tee, connect_timeout, env,
                                 options, stdin, args, ignore_timeout,
                                 ssh_failure_retry_ok)
            except error.CmdError, cmderr:
                # We get a CmdError here only if there is timeout of that
                # command. Catch that and stuff it into AutoservRunError and
                # raise it.
                timeout_message = str('Timeout encountered: %s' %
                                      cmderr.args[0])
                raise error.AutoservRunError(timeout_message, cmderr.args[1])


    def run(self, *args, **kwargs):
        return self.run_very_slowly(*args, **kwargs)


    def run_background(self, command, verbose=True):
        """Start a command on the host in the background.

        The command is started on the host in the background, and
        this method call returns immediately without waiting for the
        command's completion.  The PID of the process on the host is
        returned as a string.

        The command may redirect its stdin, stdout, or stderr as
        necessary.  Without redirection, all input and output will
        use /dev/null.

        @param command The command to run in the background
        @param verbose As for `self.run()`

        @return Returns the PID of the remote background process
                as a string.
        """
        # Redirection here isn't merely hygienic; it's a functional
        # requirement.  sshd won't terminate until stdin, stdout,
        # and stderr are all closed.
        #
        # The subshell is needed to do the right thing in case the
        # passed in command has its own I/O redirections.
        cmd_fmt = '( %s ) </dev/null >/dev/null 2>&1 & echo -n $!'
        return self.run(cmd_fmt % command, verbose=verbose).stdout


    def run_short(self, command, **kwargs):
        """
        Calls the run() command with a short default timeout.

        Takes the same arguments as does run(),
        with the exception of the timeout argument which
        here is fixed at 60 seconds.
        It returns the result of run.

        @param command: the command line string

        """
        return self.run(command, timeout=60, **kwargs)


    def run_grep(self, command, timeout=30, ignore_status=False,
                 stdout_ok_regexp=None, stdout_err_regexp=None,
                 stderr_ok_regexp=None, stderr_err_regexp=None,
                 connect_timeout=30):
        """
        Run a command on the remote host and look for regexp
        in stdout or stderr to determine if the command was
        successul or not.


        @param command: the command line string
        @param timeout: time limit in seconds before attempting to
                        kill the running process. The run() function
                        will take a few seconds longer than 'timeout'
                        to complete if it has to kill the process.
        @param ignore_status: do not raise an exception, no matter
                              what the exit code of the command is.
        @param stdout_ok_regexp: regexp that should be in stdout
                                 if the command was successul.
        @param stdout_err_regexp: regexp that should be in stdout
                                  if the command failed.
        @param stderr_ok_regexp: regexp that should be in stderr
                                 if the command was successul.
        @param stderr_err_regexp: regexp that should be in stderr
                                 if the command failed.
        @param connect_timeout: connection timeout (in seconds)

        Returns:
                if the command was successul, raises an exception
                otherwise.

        Raises:
                AutoservRunError:
                - the exit code of the command execution was not 0.
                - If stderr_err_regexp is found in stderr,
                - If stdout_err_regexp is found in stdout,
                - If stderr_ok_regexp is not found in stderr.
                - If stdout_ok_regexp is not found in stdout,
        """

        # We ignore the status, because we will handle it at the end.
        result = self.run(command, timeout, ignore_status=True,
                          connect_timeout=connect_timeout)

        # Look for the patterns, in order
        for (regexp, stream) in ((stderr_err_regexp, result.stderr),
                                 (stdout_err_regexp, result.stdout)):
            if regexp and stream:
                err_re = re.compile (regexp)
                if err_re.search(stream):
                    raise error.AutoservRunError(
                        '%s failed, found error pattern: "%s"' % (command,
                                                                regexp), result)

        for (regexp, stream) in ((stderr_ok_regexp, result.stderr),
                                 (stdout_ok_regexp, result.stdout)):
            if regexp and stream:
                ok_re = re.compile (regexp)
                if ok_re.search(stream):
                    if ok_re.search(stream):
                        return

        if not ignore_status and result.exit_status > 0:
            raise error.AutoservRunError("command execution error", result)


    def setup_ssh_key(self):
        """Setup SSH Key"""
        logging.debug('Performing SSH key setup on %s as %s.',
                      self.host_port, self.user)

        try:
            host = pxssh.pxssh()
            host.login(self.hostname, self.user, self.password,
                        port=self.port)
            public_key = utils.get_public_key()

            host.sendline('mkdir -p ~/.ssh')
            host.prompt()
            host.sendline('chmod 700 ~/.ssh')
            host.prompt()
            host.sendline("echo '%s' >> ~/.ssh/authorized_keys; " %
                            public_key)
            host.prompt()
            host.sendline('chmod 600 ~/.ssh/authorized_keys')
            host.prompt()
            host.logout()

            logging.debug('SSH key setup complete.')

        except:
            logging.debug('SSH key setup has failed.')
            try:
                host.logout()
            except:
                pass


    def setup_ssh(self):
        """Setup SSH"""
        if self.password:
            try:
                self.ssh_ping()
            except error.AutoservSshPingHostError:
                self.setup_ssh_key()
