#!/usr/bin/env python3

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

# Import the python3 compatible bytes()
from builtins import bytes

import mock
import os
import sys
import unittest

from acts.libs.proc import job

if os.name == 'posix' and sys.version_info[0] < 3:
    import subprocess32 as subprocess
else:
    import subprocess


class FakePopen(object):
    """A fake version of the object returned from subprocess.Popen()."""

    def __init__(self,
                 stdout=None,
                 stderr=None,
                 returncode=0,
                 will_timeout=False):
        self.returncode = returncode
        self._stdout = bytes(stdout,
                             'utf-8') if stdout is not None else bytes()
        self._stderr = bytes(stderr,
                             'utf-8') if stderr is not None else bytes()
        self._will_timeout = will_timeout

    def communicate(self, timeout=None):
        if self._will_timeout:
            raise subprocess.TimeoutExpired(
                -1, 'Timed out according to test logic')
        return self._stdout, self._stderr

    def kill(self):
        pass

    def wait(self):
        pass


class JobTestCases(unittest.TestCase):
    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(stdout='TEST\n'))
    def test_run_success(self, popen):
        """Test running a simple shell command."""
        result = job.run('echo TEST')
        self.assertTrue(result.stdout.startswith('TEST'))

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(stderr='TEST\n'))
    def test_run_stderr(self, popen):
        """Test that we can read process stderr."""
        result = job.run('echo TEST 1>&2')
        self.assertEqual(len(result.stdout), 0)
        self.assertTrue(result.stderr.startswith('TEST'))
        self.assertFalse(result.stdout)

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(returncode=1))
    def test_run_error(self, popen):
        """Test that we raise on non-zero exit statuses."""
        self.assertRaises(job.Error, job.run, 'exit 1')

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(returncode=1))
    def test_run_with_ignored_error(self, popen):
        """Test that we can ignore exit status on request."""
        result = job.run('exit 1', ignore_status=True)
        self.assertEqual(result.exit_status, 1)

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(will_timeout=True))
    def test_run_timeout(self, popen):
        """Test that we correctly implement command timeouts."""
        self.assertRaises(job.Error, job.run, 'sleep 5', timeout=0.1)

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(stdout='TEST\n'))
    def test_run_no_shell(self, popen):
        """Test that we handle running without a wrapping shell."""
        result = job.run(['echo', 'TEST'])
        self.assertTrue(result.stdout.startswith('TEST'))

    @mock.patch(
        'acts.libs.proc.job.subprocess.Popen',
        return_value=FakePopen(stdout='TEST\n'))
    def test_job_env(self, popen):
        """Test that we can set environment variables correctly."""
        test_env = {'MYTESTVAR': '20'}
        result = job.run('printenv', env=test_env.copy())
        popen.assert_called_once()
        _, kwargs = popen.call_args
        self.assertTrue('env' in kwargs)
        self.assertEqual(kwargs['env'], test_env)


if __name__ == '__main__':
    unittest.main()
