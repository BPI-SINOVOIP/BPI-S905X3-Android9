# -*- coding:utf-8 -*-
# Copyright 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Various utility functions."""

from __future__ import print_function

import errno
import functools
import os
import signal
import subprocess
import sys
import tempfile
import time

_path = os.path.realpath(__file__ + '/../..')
if sys.path[0] != _path:
    sys.path.insert(0, _path)
del _path

# pylint: disable=wrong-import-position
import rh.shell
import rh.signals


class CommandResult(object):
    """An object to store various attributes of a child process."""

    def __init__(self, cmd=None, error=None, output=None, returncode=None):
        self.cmd = cmd
        self.error = error
        self.output = output
        self.returncode = returncode

    @property
    def cmdstr(self):
        """Return self.cmd as a nicely formatted string (useful for logs)."""
        return rh.shell.cmd_to_str(self.cmd)


class RunCommandError(Exception):
    """Error caught in RunCommand() method."""

    def __init__(self, msg, result, exception=None):
        self.msg, self.result, self.exception = msg, result, exception
        if exception is not None and not isinstance(exception, Exception):
            raise ValueError('exception must be an exception instance; got %r'
                             % (exception,))
        Exception.__init__(self, msg)
        self.args = (msg, result, exception)

    def stringify(self, error=True, output=True):
        """Custom method for controlling what is included in stringifying this.

        Each individual argument is the literal name of an attribute
        on the result object; if False, that value is ignored for adding
        to this string content.  If true, it'll be incorporated.

        Args:
          error: See comment about individual arguments above.
          output: See comment about individual arguments above.
        """
        items = [
            'return code: %s; command: %s' % (
                self.result.returncode, self.result.cmdstr),
        ]
        if error and self.result.error:
            items.append(self.result.error)
        if output and self.result.output:
            items.append(self.result.output)
        if self.msg:
            items.append(self.msg)
        return '\n'.join(items)

    def __str__(self):
        # __str__ needs to return ascii, thus force a conversion to be safe.
        return self.stringify().decode('utf-8', 'replace').encode(
            'ascii', 'xmlcharrefreplace')

    def __eq__(self, other):
        return (type(self) == type(other) and
                self.args == other.args)

    def __ne__(self, other):
        return not self.__eq__(other)


class TerminateRunCommandError(RunCommandError):
    """We were signaled to shutdown while running a command.

    Client code shouldn't generally know, nor care about this class.  It's
    used internally to suppress retry attempts when we're signaled to die.
    """


def sudo_run_command(cmd, user='root', **kwargs):
    """Run a command via sudo.

    Client code must use this rather than coming up with their own RunCommand
    invocation that jams sudo in- this function is used to enforce certain
    rules in our code about sudo usage, and as a potential auditing point.

    Args:
      cmd: The command to run.  See RunCommand for rules of this argument-
          SudoRunCommand purely prefixes it with sudo.
      user: The user to run the command as.
      kwargs: See RunCommand options, it's a direct pass thru to it.
          Note that this supports a 'strict' keyword that defaults to True.
          If set to False, it'll suppress strict sudo behavior.

    Returns:
      See RunCommand documentation.

    Raises:
      This function may immediately raise RunCommandError if we're operating
      in a strict sudo context and the API is being misused.
      Barring that, see RunCommand's documentation- it can raise the same things
      RunCommand does.
    """
    sudo_cmd = ['sudo']

    if user == 'root' and os.geteuid() == 0:
        return run_command(cmd, **kwargs)

    if user != 'root':
        sudo_cmd += ['-u', user]

    # Pass these values down into the sudo environment, since sudo will
    # just strip them normally.
    extra_env = kwargs.pop('extra_env', None)
    extra_env = {} if extra_env is None else extra_env.copy()

    sudo_cmd.extend('%s=%s' % (k, v) for k, v in extra_env.iteritems())

    # Finally, block people from passing options to sudo.
    sudo_cmd.append('--')

    if isinstance(cmd, basestring):
        # We need to handle shell ourselves so the order is correct:
        #  $ sudo [sudo args] -- bash -c '[shell command]'
        # If we let RunCommand take care of it, we'd end up with:
        #  $ bash -c 'sudo [sudo args] -- [shell command]'
        shell = kwargs.pop('shell', False)
        if not shell:
            raise Exception('Cannot run a string command without a shell')
        sudo_cmd.extend(['/bin/bash', '-c', cmd])
    else:
        sudo_cmd.extend(cmd)

    return run_command(sudo_cmd, **kwargs)


def _kill_child_process(proc, int_timeout, kill_timeout, cmd, original_handler,
                        signum, frame):
    """Used as a signal handler by RunCommand.

    This is internal to Runcommand.  No other code should use this.
    """
    if signum:
        # If we've been invoked because of a signal, ignore delivery of that
        # signal from this point forward.  The invoking context of this func
        # restores signal delivery to what it was prior; we suppress future
        # delivery till then since this code handles SIGINT/SIGTERM fully
        # including delivering the signal to the original handler on the way
        # out.
        signal.signal(signum, signal.SIG_IGN)

    # Do not trust Popen's returncode alone; we can be invoked from contexts
    # where the Popen instance was created, but no process was generated.
    if proc.returncode is None and proc.pid is not None:
        try:
            while proc.poll() is None and int_timeout >= 0:
                time.sleep(0.1)
                int_timeout -= 0.1

            proc.terminate()
            while proc.poll() is None and kill_timeout >= 0:
                time.sleep(0.1)
                kill_timeout -= 0.1

            if proc.poll() is None:
                # Still doesn't want to die.  Too bad, so sad, time to die.
                proc.kill()
        except EnvironmentError as e:
            print('Ignoring unhandled exception in _kill_child_process: %s' % e,
                  file=sys.stderr)

        # Ensure our child process has been reaped.
        proc.wait()

    if not rh.signals.relay_signal(original_handler, signum, frame):
        # Mock up our own, matching exit code for signaling.
        cmd_result = CommandResult(cmd=cmd, returncode=signum << 8)
        raise TerminateRunCommandError('Received signal %i' % signum,
                                       cmd_result)


class _Popen(subprocess.Popen):
    """subprocess.Popen derivative customized for our usage.

    Specifically, we fix terminate/send_signal/kill to work if the child process
    was a setuid binary; on vanilla kernels, the parent can wax the child
    regardless, on goobuntu this apparently isn't allowed, thus we fall back
    to the sudo machinery we have.

    While we're overriding send_signal, we also suppress ESRCH being raised
    if the process has exited, and suppress signaling all together if the
    process has knowingly been waitpid'd already.
    """

    def send_signal(self, signum):
        if self.returncode is not None:
            # The original implementation in Popen allows signaling whatever
            # process now occupies this pid, even if the Popen object had
            # waitpid'd.  Since we can escalate to sudo kill, we do not want
            # to allow that.  Fixing this addresses that angle, and makes the
            # API less sucky in the process.
            return

        try:
            os.kill(self.pid, signum)
        except EnvironmentError as e:
            if e.errno == errno.EPERM:
                # Kill returns either 0 (signal delivered), or 1 (signal wasn't
                # delivered).  This isn't particularly informative, but we still
                # need that info to decide what to do, thus error_code_ok=True.
                ret = sudo_run_command(['kill', '-%i' % signum, str(self.pid)],
                                       redirect_stdout=True,
                                       redirect_stderr=True, error_code_ok=True)
                if ret.returncode == 1:
                    # The kill binary doesn't distinguish between permission
                    # denied and the pid is missing.  Denied can only occur
                    # under weird grsec/selinux policies.  We ignore that
                    # potential and just assume the pid was already dead and
                    # try to reap it.
                    self.poll()
            elif e.errno == errno.ESRCH:
                # Since we know the process is dead, reap it now.
                # Normally Popen would throw this error- we suppress it since
                # frankly that's a misfeature and we're already overriding
                # this method.
                self.poll()
            else:
                raise


# pylint: disable=redefined-builtin
def run_command(cmd, error_message=None, redirect_stdout=False,
                redirect_stderr=False, cwd=None, input=None,
                shell=False, env=None, extra_env=None, ignore_sigint=False,
                combine_stdout_stderr=False, log_stdout_to_file=None,
                error_code_ok=False, int_timeout=1, kill_timeout=1,
                stdout_to_pipe=False, capture_output=False,
                quiet=False, close_fds=True):
    """Runs a command.

    Args:
      cmd: cmd to run.  Should be input to subprocess.Popen.  If a string, shell
          must be true.  Otherwise the command must be an array of arguments,
          and shell must be false.
      error_message: Prints out this message when an error occurs.
      redirect_stdout: Returns the stdout.
      redirect_stderr: Holds stderr output until input is communicated.
      cwd: The working directory to run this cmd.
      input: The data to pipe into this command through stdin.  If a file object
          or file descriptor, stdin will be connected directly to that.
      shell: Controls whether we add a shell as a command interpreter.  See cmd
          since it has to agree as to the type.
      env: If non-None, this is the environment for the new process.
      extra_env: If set, this is added to the environment for the new process.
          This dictionary is not used to clear any entries though.
      ignore_sigint: If True, we'll ignore signal.SIGINT before calling the
          child.  This is the desired behavior if we know our child will handle
          Ctrl-C.  If we don't do this, I think we and the child will both get
          Ctrl-C at the same time, which means we'll forcefully kill the child.
      combine_stdout_stderr: Combines stdout and stderr streams into stdout.
      log_stdout_to_file: If set, redirects stdout to file specified by this
          path.  If |combine_stdout_stderr| is set to True, then stderr will
          also be logged to the specified file.
      error_code_ok: Does not raise an exception when command returns a non-zero
          exit code.  Instead, returns the CommandResult object containing the
          exit code.
      int_timeout: If we're interrupted, how long (in seconds) should we give
          the invoked process to clean up before we send a SIGTERM.
      kill_timeout: If we're interrupted, how long (in seconds) should we give
          the invoked process to shutdown from a SIGTERM before we SIGKILL it.
      stdout_to_pipe: Redirect stdout to pipe.
      capture_output: Set |redirect_stdout| and |redirect_stderr| to True.
      quiet: Set |stdout_to_pipe| and |combine_stdout_stderr| to True.
      close_fds: Whether to close all fds before running |cmd|.

    Returns:
      A CommandResult object.

    Raises:
      RunCommandError: Raises exception on error with optional error_message.
    """
    if capture_output:
        redirect_stdout, redirect_stderr = True, True

    if quiet:
        stdout_to_pipe, combine_stdout_stderr = True, True

    # Set default for variables.
    stdout = None
    stderr = None
    stdin = None
    cmd_result = CommandResult()

    # Force the timeout to float; in the process, if it's not convertible,
    # a self-explanatory exception will be thrown.
    kill_timeout = float(kill_timeout)

    def _get_tempfile():
        try:
            return tempfile.TemporaryFile(bufsize=0)
        except EnvironmentError as e:
            if e.errno != errno.ENOENT:
                raise
            # This can occur if we were pointed at a specific location for our
            # TMP, but that location has since been deleted.  Suppress that
            # issue in this particular case since our usage gurantees deletion,
            # and since this is primarily triggered during hard cgroups
            # shutdown.
            return tempfile.TemporaryFile(bufsize=0, dir='/tmp')

    # Modify defaults based on parameters.
    # Note that tempfiles must be unbuffered else attempts to read
    # what a separate process did to that file can result in a bad
    # view of the file.
    # The Popen API accepts either an int or a file handle for stdout/stderr.
    # pylint: disable=redefined-variable-type
    if log_stdout_to_file:
        stdout = open(log_stdout_to_file, 'w+')
    elif stdout_to_pipe:
        stdout = subprocess.PIPE
    elif redirect_stdout:
        stdout = _get_tempfile()

    if combine_stdout_stderr:
        stderr = subprocess.STDOUT
    elif redirect_stderr:
        stderr = _get_tempfile()
    # pylint: enable=redefined-variable-type

    # If subprocesses have direct access to stdout or stderr, they can bypass
    # our buffers, so we need to flush to ensure that output is not interleaved.
    if stdout is None or stderr is None:
        sys.stdout.flush()
        sys.stderr.flush()

    # If input is a string, we'll create a pipe and send it through that.
    # Otherwise we assume it's a file object that can be read from directly.
    if isinstance(input, basestring):
        stdin = subprocess.PIPE
    elif input is not None:
        stdin = input
        input = None

    if isinstance(cmd, basestring):
        if not shell:
            raise Exception('Cannot run a string command without a shell')
        cmd = ['/bin/bash', '-c', cmd]
        shell = False
    elif shell:
        raise Exception('Cannot run an array command with a shell')

    # If we are using enter_chroot we need to use enterchroot pass env through
    # to the final command.
    env = env.copy() if env is not None else os.environ.copy()
    env.update(extra_env if extra_env else {})

    cmd_result.cmd = cmd

    proc = None
    # Verify that the signals modules is actually usable, and won't segfault
    # upon invocation of getsignal.  See signals.SignalModuleUsable for the
    # details and upstream python bug.
    use_signals = rh.signals.signal_module_usable()
    try:
        proc = _Popen(cmd, cwd=cwd, stdin=stdin, stdout=stdout,
                      stderr=stderr, shell=False, env=env,
                      close_fds=close_fds)

        if use_signals:
            old_sigint = signal.getsignal(signal.SIGINT)
            if ignore_sigint:
                handler = signal.SIG_IGN
            else:
                handler = functools.partial(
                    _kill_child_process, proc, int_timeout, kill_timeout, cmd,
                    old_sigint)
            signal.signal(signal.SIGINT, handler)

            old_sigterm = signal.getsignal(signal.SIGTERM)
            handler = functools.partial(_kill_child_process, proc, int_timeout,
                                        kill_timeout, cmd, old_sigterm)
            signal.signal(signal.SIGTERM, handler)

        try:
            (cmd_result.output, cmd_result.error) = proc.communicate(input)
        finally:
            if use_signals:
                signal.signal(signal.SIGINT, old_sigint)
                signal.signal(signal.SIGTERM, old_sigterm)

            if stdout and not log_stdout_to_file and not stdout_to_pipe:
                # The linter is confused by how stdout is a file & an int.
                # pylint: disable=maybe-no-member,no-member
                stdout.seek(0)
                cmd_result.output = stdout.read()
                stdout.close()

            if stderr and stderr != subprocess.STDOUT:
                # The linter is confused by how stderr is a file & an int.
                # pylint: disable=maybe-no-member,no-member
                stderr.seek(0)
                cmd_result.error = stderr.read()
                stderr.close()

        cmd_result.returncode = proc.returncode

        if not error_code_ok and proc.returncode:
            msg = 'cwd=%s' % cwd
            if extra_env:
                msg += ', extra env=%s' % extra_env
            if error_message:
                msg += '\n%s' % error_message
            raise RunCommandError(msg, cmd_result)
    except OSError as e:
        estr = str(e)
        if e.errno == errno.EACCES:
            estr += '; does the program need `chmod a+x`?'
        if error_code_ok:
            cmd_result = CommandResult(cmd=cmd, error=estr, returncode=255)
        else:
            raise RunCommandError(estr, CommandResult(cmd=cmd), exception=e)
    finally:
        if proc is not None:
            # Ensure the process is dead.
            _kill_child_process(proc, int_timeout, kill_timeout, cmd, None,
                                None, None)

    return cmd_result
# pylint: enable=redefined-builtin


def collection(classname, **kwargs):
    """Create a new class with mutable named members.

    This is like collections.namedtuple, but mutable.  Also similar to the
    python 3.3 types.SimpleNamespace.

    Example:
      # Declare default values for this new class.
      Foo = collection('Foo', a=0, b=10)
      # Create a new class but set b to 4.
      foo = Foo(b=4)
      # Print out a (will be the default 0) and b (will be 4).
      print('a = %i, b = %i' % (foo.a, foo.b))
    """

    def sn_init(self, **kwargs):
        """The new class's __init__ function."""
        # First verify the kwargs don't have excess settings.
        valid_keys = set(self.__slots__[1:])
        these_keys = set(kwargs.keys())
        invalid_keys = these_keys - valid_keys
        if invalid_keys:
            raise TypeError('invalid keyword arguments for this object: %r' %
                            invalid_keys)

        # Now initialize this object.
        for k in valid_keys:
            setattr(self, k, kwargs.get(k, self.__defaults__[k]))

    def sn_repr(self):
        """The new class's __repr__ function."""
        return '%s(%s)' % (classname, ', '.join(
            '%s=%r' % (k, getattr(self, k)) for k in self.__slots__[1:]))

    # Give the new class a unique name and then generate the code for it.
    classname = 'Collection_%s' % classname
    expr = '\n'.join((
        'class %(classname)s(object):',
        '  __slots__ = ["__defaults__", "%(slots)s"]',
        '  __defaults__ = {}',
    )) % {
        'classname': classname,
        'slots': '", "'.join(sorted(str(k) for k in kwargs)),
    }

    # Create the class in a local namespace as exec requires.
    namespace = {}
    exec expr in namespace  # pylint: disable=exec-used
    new_class = namespace[classname]

    # Bind the helpers.
    new_class.__defaults__ = kwargs.copy()
    new_class.__init__ = sn_init
    new_class.__repr__ = sn_repr

    return new_class
