# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function

import collections
import contextlib
import logging
import os
import signal
import socket
import sys

import mock
import pytest
import subprocess32

from lucifer import leasing

logger = logging.getLogger(__name__)

# 9999-01-01T00:00:00+00:00
_THE_END = 253370764800


def test_obtain_lease(tmpdir):
    """Test obtain_lease.

    Provides basic test coverage metrics.  The slower subprocess tests
    provide better functional coverage.
    """
    path = _make_lease(tmpdir, 124)
    with leasing.obtain_lease(path):
        pass
    assert not os.path.exists(path)


@pytest.mark.slow
def test_obtain_lease_succesfully_removes_file(tmpdir):
    """Test obtain_lease cleans up lease file if successful."""
    path = _make_lease(tmpdir, 124)
    with _obtain_lease(path) as lease_proc:
        lease_proc.finish()
    assert not os.path.exists(path)


@pytest.mark.slow
def test_obtain_lease_with_error_removes_files(tmpdir):
    """Test obtain_lease removes file if it errors."""
    path = _make_lease(tmpdir, 124)
    with _obtain_lease(path) as lease_proc:
        lease_proc.proc.send_signal(signal.SIGINT)
        lease_proc.proc.wait()
    assert not os.path.exists(path)


@pytest.mark.slow
def test_Lease__expired(tmpdir, end_time):
    """Test get_expired_leases()."""
    _make_lease(tmpdir, 123)
    path = _make_lease(tmpdir, 124)
    with _obtain_lease(path):
        leases = _leases_dict(str(tmpdir))
        assert leases[123].expired()
        assert not leases[124].expired()


def test_unlocked_fresh_leases_are_not_expired(tmpdir):
    """Test get_expired_leases()."""
    path = _make_lease(tmpdir, 123)
    os.utime(path, (_THE_END, _THE_END))
    leases = _leases_dict(str(tmpdir))
    assert not leases[123].expired()


def test_leases_iter_with_sock_files(tmpdir):
    """Test leases_iter() ignores sock files."""
    _make_lease(tmpdir, 123)
    tmpdir.join('124.sock').write('')
    leases = _leases_dict(str(tmpdir))
    assert 124 not in leases


def test_Job_cleanup(tmpdir):
    """Test Job.cleanup()."""
    lease_path = _make_lease(tmpdir, 123)
    tmpdir.join('123.sock').write('')
    sock_path = str(tmpdir.join('123.sock'))
    for job in leasing.leases_iter(str(tmpdir)):
        logger.debug('Cleaning up %r', job)
        job.cleanup()
    assert not os.path.exists(lease_path)
    assert not os.path.exists(sock_path)


def test_Job_cleanup_does_not_raise_on_error(tmpdir):
    """Test Job.cleanup()."""
    lease_path = _make_lease(tmpdir, 123)
    tmpdir.join('123.sock').write('')
    sock_path = str(tmpdir.join('123.sock'))
    for job in leasing.leases_iter(str(tmpdir)):
        os.unlink(lease_path)
        os.unlink(sock_path)
        job.cleanup()


@pytest.mark.slow
def test_Job_abort(tmpdir):
    """Test Job.abort()."""
    _make_lease(tmpdir, 123)
    with _abort_socket(tmpdir, 123) as proc:
        expired = list(leasing.leases_iter(str(tmpdir)))
        assert len(expired) > 0
        for job in expired:
            job.abort()
        proc.wait()
        assert proc.returncode == 0


@pytest.mark.slow
def test_Job_abort_with_closed_socket(tmpdir):
    """Test Job.abort() with closed socket."""
    _make_lease(tmpdir, 123)
    with _abort_socket(tmpdir, 123) as proc:
        proc.terminate()
        proc.wait()
        expired = list(leasing.leases_iter(str(tmpdir)))
        assert len(expired) > 0
        for job in expired:
            with pytest.raises(socket.error):
                job.abort()


@pytest.fixture
def end_time():
    """Mock out time.time to return a time in the future."""
    with mock.patch('time.time', return_value=_THE_END) as t:
        yield t


_LeaseProc = collections.namedtuple('_LeaseProc', 'finish proc')


@contextlib.contextmanager
def _obtain_lease(path):
    """Lock a lease file.

    Yields a _LeaseProc.  finish is a function that can be called to
    finish the process normally.  proc is a Popen instance.

    This uses a slow subprocess; any test that uses this should be
    marked slow.
    """
    with subprocess32.Popen(
            [sys.executable, '-um',
             'lucifer.cmd.test.obtain_lease', path],
            stdin=subprocess32.PIPE,
            stdout=subprocess32.PIPE) as proc:
        # Wait for lock grab.
        proc.stdout.readline()

        def finish():
            """Finish lease process normally."""
            proc.stdin.write('\n')
            # Wait for lease release.
            proc.stdout.readline()
        try:
            yield _LeaseProc(finish, proc)
        finally:
            proc.terminate()


@contextlib.contextmanager
def _abort_socket(tmpdir, job_id):
    """Open a testing abort socket and listener for a job.

    As a context manager, returns the Popen instance for the listener
    process when entering.

    This uses a slow subprocess; any test that uses this should be
    marked slow.
    """
    path = os.path.join(str(tmpdir), '%d.sock' % job_id)
    logger.debug('Making abort socket at %s', path)
    with subprocess32.Popen(
            [sys.executable, '-um',
             'lucifer.cmd.test.abort_socket', path],
            stdout=subprocess32.PIPE) as proc:
        # Wait for socket bind.
        proc.stdout.readline()
        try:
            yield proc
        finally:
            proc.terminate()


def _leases_dict(jobdir):
    """Convenience method for tests."""
    return {lease.id: lease for lease
            in leasing.leases_iter(jobdir)}


def _make_lease(tmpdir, job_id):
    return _make_lease_file(str(tmpdir), job_id)


def _make_lease_file(jobdir, job_id):
    """Make lease file corresponding to a job.

    @param jobdir: job lease file directory
    @param job_id: Job ID
    """
    path = os.path.join(jobdir, str(job_id))
    with open(path, 'w'):
        pass
    return path


class _TestError(Exception):
    """Error for tests."""
