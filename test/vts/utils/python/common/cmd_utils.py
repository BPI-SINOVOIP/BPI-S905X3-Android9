#
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
import subprocess
import threading

from vts.runners.host import utils

STDOUT = 'stdouts'
STDERR = 'stderrs'
EXIT_CODE = 'return_codes'


def _ExecuteOneShellCommandWithTimeout(cmd, timeout):
    """Executes a command with timeout.

    If the process times out, this function terminates it and continues
    waiting.

    Args:
        proc: Popen object, the process to wait for.
        timeout: float, timeout in seconds.

    Returns:
        tuple(string, string, int) which are stdout, stderr and return code.
    """
    # On Windows, subprocess.Popen(shell=True) starts two processes, cmd.exe
    # and the command. The Popen object represents the cmd.exe process, so
    # calling Popen.kill() does not terminate the command.
    # This function uses process group to ensure command termination.
    proc = utils.start_standing_subprocess(cmd)
    result = []

    def WaitForProcess():
        out, err = proc.communicate()
        result.append((out, err, proc.returncode))

    wait_thread = threading.Thread(target=WaitForProcess)
    wait_thread.daemon = True
    wait_thread.start()
    try:
        wait_thread.join(timeout)
    finally:
        if proc.poll() is None:
            utils.kill_process_group(proc)
    wait_thread.join()

    if len(result) != 1:
        logging.error("Unexpected command result: %s", result)
        return "", "", proc.returncode
    return result[0]


def RunCommand(command):
    """Runs a unix command and stashes the result.

    Args:
        command: the command to run.

    Returns:
        code of the subprocess.
    """
    proc = subprocess.Popen(
        command, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    (stdout, stderr) = proc.communicate()
    if proc.returncode != 0:
        logging.error('Fail to execute command: %s  '
                      '(stdout: %s\n  stderr: %s\n)' % (command, stdout,
                                                        stderr))
    return proc.returncode


def ExecuteOneShellCommand(cmd, timeout=None):
    """Executes one shell command and returns (stdout, stderr, exit_code).

    Args:
        cmd: string, a shell command.
        timeout: float, timeout in seconds.

    Returns:
        tuple(string, string, int), containing stdout, stderr, exit_code of
        the shell command.
        If timeout, exit_code is -15 on Unix; -1073741510 on Windows.
    """
    if timeout is None:
        p = subprocess.Popen(
            str(cmd), shell=True,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        return (stdout, stderr, p.returncode)
    else:
        return _ExecuteOneShellCommandWithTimeout(str(cmd), timeout)


def ExecuteShellCommand(cmd):
    """Execute one shell cmd or a list of shell commands.

    Args:
        cmd: string or a list of strings, shell command(s)

    Returns:
        dict{int->string}, containing stdout, stderr, exit_code of the shell command(s)
    """
    if not isinstance(cmd, list):
        cmd = [cmd]

    results = [ExecuteOneShellCommand(command) for command in cmd]
    stdout, stderr, exit_code = zip(*results)
    return {STDOUT: stdout, STDERR: stderr, EXIT_CODE: exit_code}
