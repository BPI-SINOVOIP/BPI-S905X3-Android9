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

import logging
import os
import shlex
import sys
import time

if os.name == 'posix' and sys.version_info[0] < 3:
    import subprocess32 as subprocess
    DEVNULL = open(os.devnull, 'wb')
else:
    import subprocess
    # Only exists in python3.3
    from subprocess import DEVNULL


class Error(Exception):
    """Indicates that a command failed, is fatal to the test unless caught."""

    def __init__(self, result):
        super(Error, self).__init__(result)
        self.result = result


class TimeoutError(Error):
    """Thrown when a BackgroundJob times out on wait."""


class Result(object):
    """Command execution result.

    Contains information on subprocess execution after it has exited.

    Attributes:
        command: An array containing the command and all arguments that
                 was executed.
        exit_status: Integer exit code of the process.
        stdout_raw: The raw bytes output from standard out.
        stderr_raw: The raw bytes output from standard error
        duration: How long the process ran for.
        did_timeout: True if the program timed out and was killed.
    """

    @property
    def stdout(self):
        """String representation of standard output."""
        if not self._stdout_str:
            self._stdout_str = self._raw_stdout.decode(encoding=self._encoding)
            self._stdout_str = self._stdout_str.strip()
        return self._stdout_str

    @property
    def stderr(self):
        """String representation of standard error."""
        if not self._stderr_str:
            self._stderr_str = self._raw_stderr.decode(encoding=self._encoding)
            self._stderr_str = self._stderr_str.strip()
        return self._stderr_str

    def __init__(self,
                 command=[],
                 stdout=bytes(),
                 stderr=bytes(),
                 exit_status=None,
                 duration=0,
                 did_timeout=False,
                 encoding='utf-8'):
        """
        Args:
            command: The command that was run. This will be a list containing
                     the executed command and all args.
            stdout: The raw bytes that standard output gave.
            stderr: The raw bytes that standard error gave.
            exit_status: The exit status of the command.
            duration: How long the command ran.
            did_timeout: True if the command timed out.
            encoding: The encoding standard that the program uses.
        """
        self.command = command
        self.exit_status = exit_status
        self._raw_stdout = stdout
        self._raw_stderr = stderr
        self._stdout_str = None
        self._stderr_str = None
        self._encoding = encoding
        self.duration = duration
        self.did_timeout = did_timeout

    def __repr__(self):
        return ('job.Result(command=%r, stdout=%r, stderr=%r, exit_status=%r, '
                'duration=%r, did_timeout=%r, encoding=%r)') % (
                    self.command, self._raw_stdout, self._raw_stderr,
                    self.exit_status, self.duration, self.did_timeout,
                    self._encoding)


def run(command,
        timeout=60,
        ignore_status=False,
        env=None,
        io_encoding='utf-8'):
    """Execute a command in a subproccess and return its output.

    Commands can be either shell commands (given as strings) or the
    path and arguments to an executable (given as a list).  This function
    will block until the subprocess finishes or times out.

    Args:
        command: The command to execute. Can be either a string or a list.
        timeout: number seconds to wait for command to finish.
        ignore_status: bool True to ignore the exit code of the remote
                       subprocess.  Note that if you do ignore status codes,
                       you should handle non-zero exit codes explicitly.
        env: dict enviroment variables to setup on the remote host.
        io_encoding: str unicode encoding of command output.

    Returns:
        A job.Result containing the results of the ssh command.

    Raises:
        job.TimeoutError: When the remote command took to long to execute.
        Error: When the ssh connection failed to be created.
        CommandError: Ssh worked, but the command had an error executing.
    """
    start_time = time.time()
    proc = subprocess.Popen(
        command,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        shell=not isinstance(command, list))
    # Wait on the process terminating
    timed_out = False
    out = bytes()
    err = bytes()
    try:
        (out, err) = proc.communicate(timeout=timeout)
    except subprocess.TimeoutExpired:
        timed_out = True
        proc.kill()
        proc.wait()

    result = Result(
        command=command,
        stdout=out,
        stderr=err,
        exit_status=proc.returncode,
        duration=time.time() - start_time,
        encoding=io_encoding,
        did_timeout=timed_out)
    logging.debug(result)

    if timed_out:
        logging.error("Command %s with %s timeout setting timed out", command,
                      timeout)
        raise TimeoutError(result)

    if not ignore_status and proc.returncode != 0:
        raise Error(result)

    return result


def run_async(command, env=None):
    """Execute a command in a subproccess asynchronously.

    It is the callers responsibility to kill/wait on the resulting
    subprocess.Popen object.

    Commands can be either shell commands (given as strings) or the
    path and arguments to an executable (given as a list).  This function
    will not block.

    Args:
        command: The command to execute. Can be either a string or a list.
        env: dict enviroment variables to setup on the remote host.

    Returns:
        A subprocess.Popen object representing the created subprocess.

    """
    proc = subprocess.Popen(
        command,
        env=env,
        preexec_fn=os.setpgrp,
        shell=not isinstance(command, list),
        stdout=DEVNULL,
        stderr=subprocess.STDOUT)
    logging.debug("command %s started with pid %s", command, proc.pid)
    return proc

