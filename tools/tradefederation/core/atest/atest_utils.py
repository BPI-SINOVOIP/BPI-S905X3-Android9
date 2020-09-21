# Copyright 2017, The Android Open Source Project
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

"""
Utility functions for atest.
"""

import itertools
import logging
import os
import re
import subprocess
import sys
import urllib2

import constants

_MAKE_CMD = '%s/build/soong/soong_ui.bash' % os.environ.get(
    constants.ANDROID_BUILD_TOP)
_BUILD_CMD = [_MAKE_CMD, '--make-mode']
_BASH_RESET_CODE = '\033[0m\n'
# Arbitrary number to limit stdout for failed runs in _run_limited_output.
# Reason for its use is that the make command itself has its own carriage
# return output mechanism that when collected line by line causes the streaming
# full_output list to be extremely large.
_FAILED_OUTPUT_LINE_LIMIT = 100
# Regular expression to match the start of a ninja compile:
# ex: [ 99% 39710/39711]
_BUILD_COMPILE_STATUS = re.compile(r'\[\s*(\d{1,3}%\s+)?\d+/\d+\]')
_BUILD_FAILURE = 'FAILED: '


def _capture_fail_section(full_log):
    """Return the error message from the build output.

    Args:
        full_log: List of strings representing full output of build.

    Returns:
        capture_output: List of strings that are build errors.
    """
    am_capturing = False
    capture_output = []
    for line in full_log:
        if am_capturing and _BUILD_COMPILE_STATUS.match(line):
            break
        if am_capturing or line.startswith(_BUILD_FAILURE):
            capture_output.append(line)
            am_capturing = True
            continue
    return capture_output


def _run_limited_output(cmd, env_vars=None):
    """Runs a given command and streams the output on a single line in stdout.

    Args:
        cmd: A list of strings representing the command to run.
        env_vars: Optional arg. Dict of env vars to set during build.

    Raises:
        subprocess.CalledProcessError: When the command exits with a non-0
            exitcode.
    """
    # Send stderr to stdout so we only have to deal with a single pipe.
    proc = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, env=env_vars)
    sys.stdout.write('\n')
    # Determine the width of the terminal. We'll need to clear this many
    # characters when carriage returning.
    _, term_width = os.popen('stty size', 'r').read().split()
    term_width = int(term_width)
    white_space = " " * int(term_width)
    full_output = []
    while proc.poll() is None:
        line = proc.stdout.readline()
        # Readline will often return empty strings.
        if not line:
            continue
        full_output.append(line)
        # Trim the line to the width of the terminal.
        # Note: Does not handle terminal resizing, which is probably not worth
        #       checking the width every loop.
        if len(line) >= term_width:
            line = line[:term_width - 1]
        # Clear the last line we outputted.
        sys.stdout.write('\r%s\r' % white_space)
        sys.stdout.write('%s' % line.strip())
        sys.stdout.flush()
    # Reset stdout (on bash) to remove any custom formatting and newline.
    sys.stdout.write(_BASH_RESET_CODE)
    sys.stdout.flush()
    # Wait for the Popen to finish completely before checking the returncode.
    proc.wait()
    if proc.returncode != 0:
        # Parse out the build error to output.
        output = _capture_fail_section(full_output)
        if not output:
            output = full_output
        if len(output) >= _FAILED_OUTPUT_LINE_LIMIT:
            output = output[-_FAILED_OUTPUT_LINE_LIMIT:]
        output = 'Output (may be trimmed):\n%s' % ''.join(output)
        raise subprocess.CalledProcessError(proc.returncode, cmd, output)


def build(build_targets, verbose=False, env_vars=None):
    """Shell out and make build_targets.

    Args:
        build_targets: A set of strings of build targets to make.
        verbose: Optional arg. If True output is streamed to the console.
                 If False, only the last line of the build output is outputted.
        env_vars: Optional arg. Dict of env vars to set during build.

    Returns:
        Boolean of whether build command was successful, True if nothing to
        build.
    """
    if not build_targets:
        logging.debug('No build targets, skipping build.')
        return True
    full_env_vars = os.environ.copy()
    if env_vars:
        full_env_vars.update(env_vars)
    logging.info('Building targets: %s', ' '.join(build_targets))
    cmd = _BUILD_CMD + list(build_targets)
    logging.debug('Executing command: %s', cmd)
    try:
        if verbose:
            subprocess.check_call(cmd, stderr=subprocess.STDOUT,
                                  env=full_env_vars)
        else:
            # TODO: Save output to a log file.
            _run_limited_output(cmd, env_vars=full_env_vars)
        logging.info('Build successful')
        return True
    except subprocess.CalledProcessError as err:
        logging.error('Error building: %s', build_targets)
        if err.output:
            logging.error(err.output)
        return False


def _can_upload_to_result_server():
    """Return Boolean if we can talk to result server."""
    # TODO: Also check if we have a slow connection to result server.
    if constants.RESULT_SERVER:
        try:
            urllib2.urlopen(constants.RESULT_SERVER,
                            timeout=constants.RESULT_SERVER_TIMEOUT).close()
            return True
        # pylint: disable=broad-except
        except Exception as err:
            logging.debug('Talking to result server raised exception: %s', err)
    return False


def get_result_server_args():
    """Return list of args for communication with result server."""
    if _can_upload_to_result_server():
        return constants.RESULT_SERVER_ARGS
    return []


def sort_and_group(iterable, key):
    """Sort and group helper function."""
    return itertools.groupby(sorted(iterable, key=key), key=key)
