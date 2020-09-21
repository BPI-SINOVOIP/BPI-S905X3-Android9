# Copyright 2018 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import contextlib
import logging
import os
import random
import re

from autotest_lib.client.bin import utils as client_utils
from autotest_lib.client.common_lib import utils as common_utils
from autotest_lib.client.common_lib import error
from autotest_lib.server import utils
from autotest_lib.server.cros import lockfile


@contextlib.contextmanager
def lock(filename):
    """Prevents other autotest/tradefed instances from accessing cache.

    @param filename: The file to be locked.
    """
    filelock = lockfile.FileLock(filename)
    # It is tempting just to call filelock.acquire(3600). But the implementation
    # has very poor temporal granularity (timeout/10), which is unsuitable for
    # our needs. See /usr/lib64/python2.7/site-packages/lockfile/
    attempts = 0
    while not filelock.i_am_locking():
        try:
            attempts += 1
            logging.info('Waiting for cache lock...')
            # We must not use a random integer as the filelock implementations
            # may underflow an integer division.
            filelock.acquire(random.uniform(0.0, pow(2.0, attempts)))
        except (lockfile.AlreadyLocked, lockfile.LockTimeout):
            # Our goal is to wait long enough to be sure something very bad
            # happened to the locking thread. 11 attempts is between 15 and
            # 30 minutes.
            if attempts > 11:
                # Normally we should aqcuire the lock immediately. Once we
                # wait on the order of 10 minutes either the dev server IO is
                # overloaded or a lock didn't get cleaned up. Take one for the
                # team, break the lock and report a failure. This should fix
                # the lock for following tests. If the failure affects more than
                # one job look for a deadlock or dev server overload.
                logging.error('Permanent lock failure. Trying to break lock.')
                # TODO(ihf): Think how to do this cleaner without having a
                # recursive lock breaking problem. We may have to kill every
                # job that is currently waiting. The main goal though really is
                # to have a cache that does not corrupt. And cache updates
                # only happen once a month or so, everything else are reads.
                filelock.break_lock()
                raise error.TestFail('Error: permanent cache lock failure.')
        else:
            logging.info('Acquired cache lock after %d attempts.', attempts)
    try:
        yield
    finally:
        filelock.release()
        logging.info('Released cache lock.')


@contextlib.contextmanager
def adb_keepalive(target, extra_paths):
    """A context manager that keeps the adb connection alive.

    AdbKeepalive will spin off a new process that will continuously poll for
    adb's connected state, and will attempt to reconnect if it ever goes down.
    This is the only way we can currently recover safely from (intentional)
    reboots.

    @param target: the hostname and port of the DUT.
    @param extra_paths: any additional components to the PATH environment
                        variable.
    """
    from autotest_lib.client.common_lib.cros import adb_keepalive as module
    # |__file__| returns the absolute path of the compiled bytecode of the
    # module. We want to run the original .py file, so we need to change the
    # extension back.
    script_filename = module.__file__.replace('.pyc', '.py')
    job = common_utils.BgJob(
        [script_filename, target],
        nickname='adb_keepalive',
        stderr_level=logging.DEBUG,
        stdout_tee=common_utils.TEE_TO_LOGS,
        stderr_tee=common_utils.TEE_TO_LOGS,
        extra_paths=extra_paths)

    try:
        yield
    finally:
        # The adb_keepalive.py script runs forever until SIGTERM is sent.
        common_utils.nuke_subprocess(job.sp)
        common_utils.join_bg_jobs([job])


@contextlib.contextmanager
def pushd(d):
    """Defines pushd.
    @param d: the directory to change to.
    """
    current = os.getcwd()
    os.chdir(d)
    try:
        yield
    finally:
        os.chdir(current)


def parse_tradefed_result(result, waivers=None):
    """Check the result from the tradefed output.

    @param result: The result stdout string from the tradefed command.
    @param waivers: a set() of tests which are permitted to fail.
    @return 5-tuple (tests, passed, failed, notexecuted, waived)
    """
    # Regular expressions for start/end messages of each test-run chunk.
    abi_re = r'arm\S*|x86\S*'
    # TODO(kinaba): use the current running module name.
    module_re = r'\S+'
    start_re = re.compile(r'(?:Start|Continu)ing (%s) %s with'
                          r' (\d+(?:,\d+)?) test' % (abi_re, module_re))
    end_re = re.compile(r'(%s) %s (?:complet|fail)ed in .*\.'
                        r' (\d+) passed, (\d+) failed, (\d+) not executed' %
                        (abi_re, module_re))

    # Records the result per each ABI.
    total_test = dict()
    total_pass = dict()
    total_fail = dict()
    last_notexec = dict()

    # ABI and the test count for the current chunk.
    abi = None
    ntest = None
    prev_npass = prev_nfail = prev_nnotexec = None

    for line in result.splitlines():
        # Beginning of a chunk of tests.
        match = start_re.search(line)
        if match:
            if abi:
                raise error.TestFail('Error: Unexpected test start: ' + line)
            abi = match.group(1)
            ntest = int(match.group(2).replace(',', ''))
            prev_npass = prev_nfail = prev_nnotexec = None
        else:
            # End of the current chunk.
            match = end_re.search(line)
            if not match:
                continue

            npass, nfail, nnotexec = map(int, match.group(2, 3, 4))
            if abi != match.group(1):
                # When the last case crashed during teardown, tradefed emits two
                # end-messages with possibly increased fail count. Ignore it.
                if (prev_npass == npass and
                    (prev_nfail == nfail or prev_nfail == nfail - 1) and
                        prev_nnotexec == nnotexec):
                    continue
                raise error.TestFail('Error: Unexpected test end: ' + line)
            prev_npass, prev_nfail, prev_nnotexec = npass, nfail, nnotexec

            # When the test crashes too ofen, tradefed seems to finish the
            # iteration by running "0 tests, 0 passed, ...". Do not count
            # that in.
            if ntest > 0:
                total_test[abi] = (
                    total_test.get(abi, 0) + ntest - last_notexec.get(abi, 0))
                total_pass[abi] = total_pass.get(abi, 0) + npass
                total_fail[abi] = total_fail.get(abi, 0) + nfail
                last_notexec[abi] = nnotexec
            abi = None

    if abi:
        # When tradefed crashes badly, it may exit without printing the counts
        # from the last chunk. Regard them as not executed and retry (rather
        # than aborting the test cycle at this point.)
        if ntest > 0:
            total_test[abi] = (
                total_test.get(abi, 0) + ntest - last_notexec.get(abi, 0))
            last_notexec[abi] = ntest
        logging.warning('No result reported for the last chunk. ' +
                        'Assuming all not executed.')

    # TODO(rohitbm): make failure parsing more robust by extracting the list
    # of failing tests instead of searching in the result blob. As well as
    # only parse for waivers for the running ABI.
    waived = 0
    if waivers:
        abis = total_test.keys()
        for testname in waivers:
            # TODO(dhaddock): Find a more robust way to apply waivers.
            fail_count = (
                result.count(testname + ' FAIL') +
                result.count(testname + ' fail'))
            if fail_count:
                if fail_count > len(abis):
                    # This should be an error.TestFail, but unfortunately
                    # tradefed has a bug that emits "fail" twice when a
                    # test failed during teardown. It will anyway causes
                    # a test count inconsistency and visible on the dashboard.
                    logging.error('Found %d failures for %s '
                                  'but there are only %d abis: %s', fail_count,
                                  testname, len(abis), abis)
                waived += fail_count
                logging.info('Waived failure for %s %d time(s)', testname,
                             fail_count)
    counts = tuple(
        sum(count_per_abi.values())
        for count_per_abi in (total_test, total_pass, total_fail,
                              last_notexec)) + (waived,)
    msg = (
        'tests=%d, passed=%d, failed=%d, not_executed=%d, waived=%d' % counts)
    logging.info(msg)
    if counts[2] - waived < 0:
        raise error.TestFail('Error: Internal waiver bookkeeping has '
                             'become inconsistent (%s)' % msg)
    return counts


def select_32bit_java():
    """Switches to 32 bit java if installed (like in lab lxc images) to save
    about 30-40% server/shard memory during the run."""
    if utils.is_in_container() and not client_utils.is_moblab():
        java = '/usr/lib/jvm/java-8-openjdk-i386'
        if os.path.exists(java):
            logging.info('Found 32 bit java, switching to use it.')
            os.environ['JAVA_HOME'] = java
            os.environ['PATH'] = (
                os.path.join(java, 'bin') + os.pathsep + os.environ['PATH'])
