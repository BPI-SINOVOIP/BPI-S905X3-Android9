#!/usr/bin/python
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Script to archive old Autotest results to Google Storage.

Uses gsutil to archive files to the configured Google Storage bucket.
Upon successful copy, the local results directory is deleted.
"""

import abc
import datetime
import errno
import glob
import gzip
import logging
import logging.handlers
import os
import re
import shutil
import socket
import stat
import subprocess
import sys
import tarfile
import tempfile
import time

from optparse import OptionParser

import common
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.site_utils import job_directories
# For unittest, the cloud_console.proto is not compiled yet.
try:
    from autotest_lib.site_utils import cloud_console_client
except ImportError:
    cloud_console_client = None
from autotest_lib.tko import models
from autotest_lib.utils import labellib
from autotest_lib.utils import gslib
from chromite.lib import timeout_util

# Autotest requires the psutil module from site-packages, so it must be imported
# after "import common".
try:
    # Does not exist, nor is needed, on moblab.
    import psutil
except ImportError:
    psutil = None

from chromite.lib import parallel
try:
    from chromite.lib import metrics
    from chromite.lib import ts_mon_config
except ImportError:
    metrics = utils.metrics_mock
    ts_mon_config = utils.metrics_mock


GS_OFFLOADING_ENABLED = global_config.global_config.get_config_value(
        'CROS', 'gs_offloading_enabled', type=bool, default=True)

# Nice setting for process, the higher the number the lower the priority.
NICENESS = 10

# Maximum number of seconds to allow for offloading a single
# directory.
OFFLOAD_TIMEOUT_SECS = 60 * 60

# Sleep time per loop.
SLEEP_TIME_SECS = 5

# Minimum number of seconds between e-mail reports.
REPORT_INTERVAL_SECS = 60 * 60

# Location of Autotest results on disk.
RESULTS_DIR = '/usr/local/autotest/results'
FAILED_OFFLOADS_FILE = os.path.join(RESULTS_DIR, 'FAILED_OFFLOADS')

FAILED_OFFLOADS_FILE_HEADER = '''
This is the list of gs_offloader failed jobs.
Last offloader attempt at %s failed to offload %d files.
Check http://go/cros-triage-gsoffloader to triage the issue


First failure       Count   Directory name
=================== ======  ==============================
'''
# --+----1----+----  ----+  ----+----1----+----2----+----3

FAILED_OFFLOADS_LINE_FORMAT = '%19s  %5d  %-1s\n'
FAILED_OFFLOADS_TIME_FORMAT = '%Y-%m-%d %H:%M:%S'

USE_RSYNC_ENABLED = global_config.global_config.get_config_value(
        'CROS', 'gs_offloader_use_rsync', type=bool, default=False)

LIMIT_FILE_COUNT = global_config.global_config.get_config_value(
        'CROS', 'gs_offloader_limit_file_count', type=bool, default=False)

# Use multiprocessing for gsutil uploading.
GS_OFFLOADER_MULTIPROCESSING = global_config.global_config.get_config_value(
        'CROS', 'gs_offloader_multiprocessing', type=bool, default=False)

D = '[0-9][0-9]'
TIMESTAMP_PATTERN = '%s%s.%s.%s_%s.%s.%s' % (D, D, D, D, D, D, D)
CTS_RESULT_PATTERN = 'testResult.xml'
CTS_V2_RESULT_PATTERN = 'test_result.xml'
# Google Storage bucket URI to store results in.
DEFAULT_CTS_RESULTS_GSURI = global_config.global_config.get_config_value(
        'CROS', 'cts_results_server', default='')
DEFAULT_CTS_APFE_GSURI = global_config.global_config.get_config_value(
        'CROS', 'cts_apfe_server', default='')

# metadata type
GS_OFFLOADER_SUCCESS_TYPE = 'gs_offloader_success'
GS_OFFLOADER_FAILURE_TYPE = 'gs_offloader_failure'

# Autotest test to collect list of CTS tests
TEST_LIST_COLLECTOR = 'tradefed-run-collect-tests-only'

def _get_metrics_fields(dir_entry):
    """Get metrics fields for the given test result directory, including board
    and milestone.

    @param dir_entry: Directory entry to offload.
    @return A dictionary for the metrics data to be uploaded.
    """
    fields = {'board': 'unknown',
              'milestone': 'unknown'}
    if dir_entry:
        # There could be multiple hosts in the job directory, use the first one
        # available.
        for host in glob.glob(os.path.join(dir_entry, '*')):
            try:
                keyval = models.test.parse_job_keyval(host)
            except ValueError:
                continue
            build = keyval.get('build')
            if build:
                try:
                    cros_version = labellib.parse_cros_version(build)
                    fields['board'] = cros_version.board
                    fields['milestone'] = cros_version.milestone
                    break
                except ValueError:
                    # Ignore version parsing error so it won't crash
                    # gs_offloader.
                    pass

    return fields;


def _get_es_metadata(dir_entry):
    """Get ES metadata for the given test result directory.

    @param dir_entry: Directory entry to offload.
    @return A dictionary for the metadata to be uploaded.
    """
    fields = _get_metrics_fields(dir_entry)
    fields['hostname'] = socket.gethostname()
    # Include more data about the test job in metadata.
    if dir_entry:
        fields['dir_entry'] = dir_entry
        fields['job_id'] = job_directories.get_job_id_or_task_id(dir_entry)

    return fields


def _get_cmd_list(multiprocessing, dir_entry, gs_path):
    """Return the command to offload a specified directory.

    @param multiprocessing: True to turn on -m option for gsutil.
    @param dir_entry: Directory entry/path that which we need a cmd_list
                      to offload.
    @param gs_path: Location in google storage where we will
                    offload the directory.

    @return A command list to be executed by Popen.
    """
    cmd = ['gsutil']
    if multiprocessing:
        cmd.append('-m')
    if USE_RSYNC_ENABLED:
        cmd.append('rsync')
        target = os.path.join(gs_path, os.path.basename(dir_entry))
    else:
        cmd.append('cp')
        target = gs_path
    cmd += ['-eR', dir_entry, target]
    return cmd


def sanitize_dir(dirpath):
    """Sanitize directory for gs upload.

    Symlinks and FIFOS are converted to regular files to fix bugs.

    @param dirpath: Directory entry to be sanitized.
    """
    if not os.path.exists(dirpath):
        return
    _escape_rename(dirpath)
    _escape_rename_dir_contents(dirpath)
    _sanitize_fifos(dirpath)
    _sanitize_symlinks(dirpath)


def _escape_rename_dir_contents(dirpath):
    """Recursively rename directory to escape filenames for gs upload.

    @param dirpath: Directory path string.
    """
    for filename in os.listdir(dirpath):
        path = os.path.join(dirpath, filename)
        _escape_rename(path)
    for filename in os.listdir(dirpath):
        path = os.path.join(dirpath, filename)
        if os.path.isdir(path):
            _escape_rename_dir_contents(path)


def _escape_rename(path):
    """Rename file to escape filenames for gs upload.

    @param path: File path string.
    """
    dirpath, filename = os.path.split(path)
    sanitized_filename = gslib.escape(filename)
    sanitized_path = os.path.join(dirpath, sanitized_filename)
    os.rename(path, sanitized_path)


def _sanitize_fifos(dirpath):
    """Convert fifos to regular files (fixes crbug.com/684122).

    @param dirpath: Directory path string.
    """
    for root, _, files in os.walk(dirpath):
        for filename in files:
            path = os.path.join(root, filename)
            file_stat = os.lstat(path)
            if stat.S_ISFIFO(file_stat.st_mode):
                _replace_fifo_with_file(path)


def _replace_fifo_with_file(path):
    """Replace a fifo with a normal file.

    @param path: Fifo path string.
    """
    logging.debug('Removing fifo %s', path)
    os.remove(path)
    logging.debug('Creating marker %s', path)
    with open(path, 'w') as f:
        f.write('<FIFO>')


def _sanitize_symlinks(dirpath):
    """Convert Symlinks to regular files (fixes crbug.com/692788).

    @param dirpath: Directory path string.
    """
    for root, _, files in os.walk(dirpath):
        for filename in files:
            path = os.path.join(root, filename)
            file_stat = os.lstat(path)
            if stat.S_ISLNK(file_stat.st_mode):
                _replace_symlink_with_file(path)


def _replace_symlink_with_file(path):
    """Replace a symlink with a normal file.

    @param path: Symlink path string.
    """
    target = os.readlink(path)
    logging.debug('Removing symlink %s', path)
    os.remove(path)
    logging.debug('Creating marker %s', path)
    with open(path, 'w') as f:
        f.write('<symlink to %s>' % target)


# Maximum number of files in the folder.
_MAX_FILE_COUNT = 500
_FOLDERS_NEVER_ZIP = ['debug', 'ssp_logs', 'autoupdate_logs']


def _get_zippable_folders(dir_entry):
    folders_list = []
    for folder in os.listdir(dir_entry):
        folder_path = os.path.join(dir_entry, folder)
        if (not os.path.isfile(folder_path) and
                not folder in _FOLDERS_NEVER_ZIP):
            folders_list.append(folder_path)
    return folders_list


def limit_file_count(dir_entry):
    """Limit the number of files in given directory.

    The method checks the total number of files in the given directory.
    If the number is greater than _MAX_FILE_COUNT, the method will
    compress each folder in the given directory, except folders in
    _FOLDERS_NEVER_ZIP.

    @param dir_entry: Directory entry to be checked.
    """
    try:
        count = _count_files(dir_entry)
    except ValueError:
        logging.warning('Fail to get the file count in folder %s.', dir_entry)
        return
    if count < _MAX_FILE_COUNT:
        return

    # For test job, zip folders in a second level, e.g. 123-debug/host1.
    # This is to allow autoserv debug folder still be accessible.
    # For special task, it does not need to dig one level deeper.
    is_special_task = re.match(job_directories.SPECIAL_TASK_PATTERN,
                               dir_entry)

    folders = _get_zippable_folders(dir_entry)
    if not is_special_task:
        subfolders = []
        for folder in folders:
            subfolders.extend(_get_zippable_folders(folder))
        folders = subfolders

    for folder in folders:
        _make_into_tarball(folder)


def _count_files(dirpath):
    """Count the number of files in a directory recursively.

    @param dirpath: Directory path string.
    """
    return sum(len(files) for _path, _dirs, files in os.walk(dirpath))


def _make_into_tarball(dirpath):
    """Make directory into tarball.

    @param dirpath: Directory path string.
    """
    tarpath = '%s.tgz' % dirpath
    with tarfile.open(tarpath, 'w:gz') as tar:
        tar.add(dirpath, arcname=os.path.basename(dirpath))
    shutil.rmtree(dirpath)


def correct_results_folder_permission(dir_entry):
    """Make sure the results folder has the right permission settings.

    For tests running with server-side packaging, the results folder has
    the owner of root. This must be changed to the user running the
    autoserv process, so parsing job can access the results folder.

    @param dir_entry: Path to the results folder.
    """
    if not dir_entry:
        return

    logging.info('Trying to correct file permission of %s.', dir_entry)
    try:
        owner = '%s:%s' % (os.getuid(), os.getgid())
        subprocess.check_call(
                ['sudo', '-n', 'chown', '-R', owner, dir_entry])
        subprocess.check_call(['chmod', '-R', 'u+r', dir_entry])
        subprocess.check_call(
                ['find', dir_entry, '-type', 'd',
                 '-exec', 'chmod', 'u+x', '{}', ';'])
    except subprocess.CalledProcessError as e:
        logging.error('Failed to modify permission for %s: %s',
                      dir_entry, e)


def _upload_cts_testresult(dir_entry, multiprocessing):
    """Upload test results to separate gs buckets.

    Upload testResult.xml.gz/test_result.xml.gz file to cts_results_bucket.
    Upload timestamp.zip to cts_apfe_bucket.

    @param dir_entry: Path to the results folder.
    @param multiprocessing: True to turn on -m option for gsutil.
    """
    for host in glob.glob(os.path.join(dir_entry, '*')):
        cts_path = os.path.join(host, 'cheets_CTS.*', 'results', '*',
                                TIMESTAMP_PATTERN)
        cts_v2_path = os.path.join(host, 'cheets_CTS_*', 'results', '*',
                                   TIMESTAMP_PATTERN)
        gts_v2_path = os.path.join(host, 'cheets_GTS.*', 'results', '*',
                                   TIMESTAMP_PATTERN)
        for result_path, result_pattern in [(cts_path, CTS_RESULT_PATTERN),
                            (cts_v2_path, CTS_V2_RESULT_PATTERN),
                            (gts_v2_path, CTS_V2_RESULT_PATTERN)]:
            for path in glob.glob(result_path):
                try:
                    _upload_files(host, path, result_pattern, multiprocessing)
                except Exception as e:
                    logging.error('ERROR uploading test results %s to GS: %s',
                                  path, e)


def _is_valid_result(build, result_pattern, suite):
    """Check if the result should be uploaded to CTS/GTS buckets.

    @param build: Builder name.
    @param result_pattern: XML result file pattern.
    @param suite: Test suite name.

    @returns: Bool flag indicating whether a valid result.
    """
    if build is None or suite is None:
        return False

    # Not valid if it's not a release build.
    if not re.match(r'(?!trybot-).*-release/.*', build):
        return False

    # Not valid if it's cts result but not 'arc-cts*' or 'test_that_wrapper'
    # suite.
    result_patterns = [CTS_RESULT_PATTERN, CTS_V2_RESULT_PATTERN]
    if result_pattern in result_patterns and not (
            suite.startswith('arc-cts') or suite.startswith('arc-gts') or
            suite.startswith('test_that_wrapper')):
        return False

    return True


def _is_test_collector(package):
    """Returns true if the test run is just to collect list of CTS tests.

    @param package: Autotest package name. e.g. cheets_CTS_N.CtsGraphicsTestCase

    @return Bool flag indicating a test package is CTS list generator or not.
    """
    return TEST_LIST_COLLECTOR in package


def _upload_files(host, path, result_pattern, multiprocessing):
    keyval = models.test.parse_job_keyval(host)
    build = keyval.get('build')
    suite = keyval.get('suite')

    if not _is_valid_result(build, result_pattern, suite):
        # No need to upload current folder, return.
        return

    parent_job_id = str(keyval['parent_job_id'])

    folders = path.split(os.sep)
    job_id = folders[-6]
    package = folders[-4]
    timestamp = folders[-1]

    # Results produced by CTS test list collector are dummy results.
    # They don't need to be copied to APFE bucket which is mainly being used for
    # CTS APFE submission.
    if not _is_test_collector(package):
        # Path: bucket/build/parent_job_id/cheets_CTS.*/job_id_timestamp/
        # or bucket/build/parent_job_id/cheets_GTS.*/job_id_timestamp/
        cts_apfe_gs_path = os.path.join(
                DEFAULT_CTS_APFE_GSURI, build, parent_job_id,
                package, job_id + '_' + timestamp) + '/'

        for zip_file in glob.glob(os.path.join('%s.zip' % path)):
            utils.run(' '.join(_get_cmd_list(
                    multiprocessing, zip_file, cts_apfe_gs_path)))
            logging.debug('Upload %s to %s ', zip_file, cts_apfe_gs_path)
    else:
        logging.debug('%s is a CTS Test collector Autotest test run.', package)
        logging.debug('Skipping CTS results upload to APFE gs:// bucket.')

    # Path: bucket/cheets_CTS.*/job_id_timestamp/
    # or bucket/cheets_GTS.*/job_id_timestamp/
    test_result_gs_path = os.path.join(
            DEFAULT_CTS_RESULTS_GSURI, package,
            job_id + '_' + timestamp) + '/'

    for test_result_file in glob.glob(os.path.join(path, result_pattern)):
        # gzip test_result_file(testResult.xml/test_result.xml)

        test_result_file_gz =  '%s.gz' % test_result_file
        with open(test_result_file, 'r') as f_in, (
                gzip.open(test_result_file_gz, 'w')) as f_out:
            shutil.copyfileobj(f_in, f_out)
        utils.run(' '.join(_get_cmd_list(
                multiprocessing, test_result_file_gz, test_result_gs_path)))
        logging.debug('Zip and upload %s to %s',
                      test_result_file_gz, test_result_gs_path)
        # Remove test_result_file_gz(testResult.xml.gz/test_result.xml.gz)
        os.remove(test_result_file_gz)


def _emit_gs_returncode_metric(returncode):
    """Increment the gs_returncode counter based on |returncode|."""
    m_gs_returncode = 'chromeos/autotest/gs_offloader/gs_returncode'
    rcode = int(returncode)
    if rcode < 0 or rcode > 255:
        rcode = -1
    metrics.Counter(m_gs_returncode).increment(fields={'return_code': rcode})


def _handle_dir_os_error(dir_entry, fix_permission=False):
    """Try to fix the result directory's permission issue if needed.

    @param dir_entry: Directory entry to offload.
    @param fix_permission: True to change the directory's owner to the same one
            running gs_offloader.
    """
    if fix_permission:
        correct_results_folder_permission(dir_entry)
    m_permission_error = ('chromeos/autotest/errors/gs_offloader/'
                          'wrong_permissions_count')
    metrics_fields = _get_metrics_fields(dir_entry)
    metrics.Counter(m_permission_error).increment(fields=metrics_fields)


class BaseGSOffloader(object):

    """Google Storage offloader interface."""

    __metaclass__ = abc.ABCMeta

    def offload(self, dir_entry, dest_path, job_complete_time):
        """Safely offload a directory entry to Google Storage.

        This method is responsible for copying the contents of
        `dir_entry` to Google storage at `dest_path`.

        When successful, the method must delete all of `dir_entry`.
        On failure, `dir_entry` should be left undisturbed, in order
        to allow for retry.

        Errors are conveyed simply and solely by two methods:
          * At the time of failure, write enough information to the log
            to allow later debug, if necessary.
          * Don't delete the content.

        In order to guarantee robustness, this method must not raise any
        exceptions.

        @param dir_entry: Directory entry to offload.
        @param dest_path: Location in google storage where we will
                          offload the directory.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        """
        try:
            self._full_offload(dir_entry, dest_path, job_complete_time)
        except Exception as e:
            logging.debug('Exception in offload for %s', dir_entry)
            logging.debug('Ignoring this error: %s', str(e))

    @abc.abstractmethod
    def _full_offload(self, dir_entry, dest_path, job_complete_time):
        """Offload a directory entry to Google Storage.

        This method implements the actual offload behavior of its
        subclass.  To guarantee effective debug, this method should
        catch all exceptions, and perform any reasonable diagnosis
        or other handling.

        @param dir_entry: Directory entry to offload.
        @param dest_path: Location in google storage where we will
                          offload the directory.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        """


class GSOffloader(BaseGSOffloader):
    """Google Storage Offloader."""

    def __init__(self, gs_uri, multiprocessing, delete_age,
            console_client=None):
        """Returns the offload directory function for the given gs_uri

        @param gs_uri: Google storage bucket uri to offload to.
        @param multiprocessing: True to turn on -m option for gsutil.
        @param console_client: The cloud console client. If None,
          cloud console APIs are  not called.
        """
        self._gs_uri = gs_uri
        self._multiprocessing = multiprocessing
        self._delete_age = delete_age
        self._console_client = console_client

    @metrics.SecondsTimerDecorator(
            'chromeos/autotest/gs_offloader/job_offload_duration')
    def _full_offload(self, dir_entry, dest_path, job_complete_time):
        """Offload the specified directory entry to Google storage.

        @param dir_entry: Directory entry to offload.
        @param dest_path: Location in google storage where we will
                          offload the directory.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        """
        with tempfile.TemporaryFile('w+') as stdout_file, \
             tempfile.TemporaryFile('w+') as stderr_file:
            try:
                try:
                    self._try_offload(dir_entry, dest_path, stdout_file,
                                      stderr_file)
                except OSError as e:
                    # Correct file permission error of the directory, then raise
                    # the exception so gs_offloader can retry later.
                    _handle_dir_os_error(dir_entry, e.errno==errno.EACCES)
                    # Try again after the permission issue is fixed.
                    self._try_offload(dir_entry, dest_path, stdout_file,
                                      stderr_file)
            except _OffloadError as e:
                metrics_fields = _get_metrics_fields(dir_entry)
                m_any_error = 'chromeos/autotest/errors/gs_offloader/any_error'
                metrics.Counter(m_any_error).increment(fields=metrics_fields)

                # Rewind the log files for stdout and stderr and log
                # their contents.
                stdout_file.seek(0)
                stderr_file.seek(0)
                stderr_content = stderr_file.read()
                logging.warning('Error occurred when offloading %s:', dir_entry)
                logging.warning('Stdout:\n%s \nStderr:\n%s', stdout_file.read(),
                                stderr_content)

                # Some result files may have wrong file permission. Try
                # to correct such error so later try can success.
                # TODO(dshi): The code is added to correct result files
                # with wrong file permission caused by bug 511778. After
                # this code is pushed to lab and run for a while to
                # clean up these files, following code and function
                # correct_results_folder_permission can be deleted.
                if 'CommandException: Error opening file' in stderr_content:
                    correct_results_folder_permission(dir_entry)
            else:
                self._prune(dir_entry, job_complete_time)

    def _try_offload(self, dir_entry, dest_path,
                 stdout_file, stderr_file):
        """Offload the specified directory entry to Google storage.

        @param dir_entry: Directory entry to offload.
        @param dest_path: Location in google storage where we will
                          offload the directory.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        @param stdout_file: Log file.
        @param stderr_file: Log file.
        """
        if _is_uploaded(dir_entry):
            return
        start_time = time.time()
        metrics_fields = _get_metrics_fields(dir_entry)
        es_metadata = _get_es_metadata(dir_entry)
        error_obj = _OffloadError(start_time, es_metadata)
        try:
            sanitize_dir(dir_entry)
            if DEFAULT_CTS_RESULTS_GSURI:
                _upload_cts_testresult(dir_entry, self._multiprocessing)

            if LIMIT_FILE_COUNT:
                limit_file_count(dir_entry)
            es_metadata['size_kb'] = file_utils.get_directory_size_kibibytes(dir_entry)

            process = None
            with timeout_util.Timeout(OFFLOAD_TIMEOUT_SECS):
                gs_path = '%s%s' % (self._gs_uri, dest_path)
                process = subprocess.Popen(
                        _get_cmd_list(self._multiprocessing, dir_entry, gs_path),
                        stdout=stdout_file, stderr=stderr_file)
                process.wait()

            _emit_gs_returncode_metric(process.returncode)
            if process.returncode != 0:
                raise error_obj
            _emit_offload_metrics(dir_entry)

            if self._console_client:
                gcs_uri = os.path.join(gs_path,
                        os.path.basename(dir_entry))
                if not self._console_client.send_test_job_offloaded_message(
                        gcs_uri):
                    raise error_obj

            _mark_uploaded(dir_entry)
        except timeout_util.TimeoutError:
            m_timeout = 'chromeos/autotest/errors/gs_offloader/timed_out_count'
            metrics.Counter(m_timeout).increment(fields=metrics_fields)
            # If we finished the call to Popen(), we may need to
            # terminate the child process.  We don't bother calling
            # process.poll(); that inherently races because the child
            # can die any time it wants.
            if process:
                try:
                    process.terminate()
                except OSError:
                    # We don't expect any error other than "No such
                    # process".
                    pass
            logging.error('Offloading %s timed out after waiting %d '
                          'seconds.', dir_entry, OFFLOAD_TIMEOUT_SECS)
            raise error_obj

    def _prune(self, dir_entry, job_complete_time):
        """Prune directory if it is uploaded and expired.

        @param dir_entry: Directory entry to offload.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        """
        if not (_is_uploaded(dir_entry)
                and job_directories.is_job_expired(self._delete_age,
                                                   job_complete_time)):
            return
        try:
            shutil.rmtree(dir_entry)
        except OSError as e:
            # The wrong file permission can lead call `shutil.rmtree(dir_entry)`
            # to raise OSError with message 'Permission denied'. Details can be
            # found in crbug.com/536151
            _handle_dir_os_error(dir_entry, e.errno==errno.EACCES)
            # Try again after the permission issue is fixed.
            shutil.rmtree(dir_entry)


class _OffloadError(Exception):
    """Google Storage offload failed."""

    def __init__(self, start_time, es_metadata):
        super(_OffloadError, self).__init__(start_time, es_metadata)
        self.start_time = start_time
        self.es_metadata = es_metadata



class FakeGSOffloader(BaseGSOffloader):

    """Fake Google Storage Offloader that only deletes directories."""

    def _full_offload(self, dir_entry, dest_path, job_complete_time):
        """Pretend to offload a directory and delete it.

        @param dir_entry: Directory entry to offload.
        @param dest_path: Location in google storage where we will
                          offload the directory.
        @param job_complete_time: The complete time of the job from the AFE
                                  database.
        """
        shutil.rmtree(dir_entry)


def _is_expired(job, age_limit):
    """Return whether job directory is expired for uploading

    @param job: _JobDirectory instance.
    @param age_limit:  Minimum age in days at which a job may be offloaded.
    """
    job_timestamp = job.get_timestamp_if_finished()
    if not job_timestamp:
        return False
    return job_directories.is_job_expired(age_limit, job_timestamp)


def _emit_offload_metrics(dirpath):
    """Emit gs offload metrics.

    @param dirpath: Offloaded directory path.
    """
    dir_size = file_utils.get_directory_size_kibibytes(dirpath)
    metrics_fields = _get_metrics_fields(dirpath)

    m_offload_count = (
            'chromeos/autotest/gs_offloader/jobs_offloaded')
    metrics.Counter(m_offload_count).increment(
            fields=metrics_fields)
    m_offload_size = ('chromeos/autotest/gs_offloader/'
                      'kilobytes_transferred')
    metrics.Counter(m_offload_size).increment_by(
            dir_size, fields=metrics_fields)


def _is_uploaded(dirpath):
    """Return whether directory has been uploaded.

    @param dirpath: Directory path string.
    """
    return os.path.isfile(_get_uploaded_marker_file(dirpath))


def _mark_uploaded(dirpath):
    """Mark directory as uploaded.

    @param dirpath: Directory path string.
    """
    with open(_get_uploaded_marker_file(dirpath), 'a'):
        pass


def _get_uploaded_marker_file(dirpath):
    """Return path to upload marker file for directory.

    @param dirpath: Directory path string.
    """
    return '%s/.GS_UPLOADED' % (dirpath,)


def _format_job_for_failure_reporting(job):
    """Formats a _JobDirectory for reporting / logging.

    @param job: The _JobDirectory to format.
    """
    d = datetime.datetime.fromtimestamp(job.first_offload_start)
    data = (d.strftime(FAILED_OFFLOADS_TIME_FORMAT),
            job.offload_count,
            job.dirname)
    return FAILED_OFFLOADS_LINE_FORMAT % data


def wait_for_gs_write_access(gs_uri):
    """Verify and wait until we have write access to Google Storage.

    @param gs_uri: The Google Storage URI we are trying to offload to.
    """
    # TODO (sbasi) Try to use the gsutil command to check write access.
    # Ensure we have write access to gs_uri.
    dummy_file = tempfile.NamedTemporaryFile()
    test_cmd = _get_cmd_list(False, dummy_file.name, gs_uri)
    while True:
        try:
            subprocess.check_call(test_cmd)
            subprocess.check_call(
                    ['gsutil', 'rm',
                     os.path.join(gs_uri,
                                  os.path.basename(dummy_file.name))])
            break
        except subprocess.CalledProcessError:
            logging.debug('Unable to offload to %s, sleeping.', gs_uri)
            time.sleep(120)


class Offloader(object):
    """State of the offload process.

    Contains the following member fields:
      * _gs_offloader:  _BaseGSOffloader to use to offload a job directory.
      * _jobdir_classes:  List of classes of job directory to be
        offloaded.
      * _processes:  Maximum number of outstanding offload processes
        to allow during an offload cycle.
      * _age_limit:  Minimum age in days at which a job may be
        offloaded.
      * _open_jobs: a dictionary mapping directory paths to Job
        objects.
    """

    def __init__(self, options):
        self._upload_age_limit = options.age_to_upload
        self._delete_age_limit = options.age_to_delete
        if options.delete_only:
            self._gs_offloader = FakeGSOffloader()
        else:
            self.gs_uri = utils.get_offload_gsuri()
            logging.debug('Offloading to: %s', self.gs_uri)
            multiprocessing = False
            if options.multiprocessing:
                multiprocessing = True
            elif options.multiprocessing is None:
                multiprocessing = GS_OFFLOADER_MULTIPROCESSING
            logging.info(
                    'Offloader multiprocessing is set to:%r', multiprocessing)
            console_client = None
            if (cloud_console_client and
                    cloud_console_client.is_cloud_notification_enabled()):
                console_client = cloud_console_client.PubSubBasedClient()
            self._gs_offloader = GSOffloader(
                    self.gs_uri, multiprocessing, self._delete_age_limit,
                    console_client)
        classlist = []
        if options.process_hosts_only or options.process_all:
            classlist.append(job_directories.SpecialJobDirectory)
        if not options.process_hosts_only:
            classlist.append(job_directories.RegularJobDirectory)
        self._jobdir_classes = classlist
        assert self._jobdir_classes
        self._processes = options.parallelism
        self._open_jobs = {}
        self._pusub_topic = None
        self._offload_count_limit = 3


    def _add_new_jobs(self):
        """Find new job directories that need offloading.

        Go through the file system looking for valid job directories
        that are currently not in `self._open_jobs`, and add them in.

        """
        new_job_count = 0
        for cls in self._jobdir_classes:
            for resultsdir in cls.get_job_directories():
                if (
                        resultsdir in self._open_jobs
                        or _is_uploaded(resultsdir)):
                    continue
                self._open_jobs[resultsdir] = cls(resultsdir)
                new_job_count += 1
        logging.debug('Start of offload cycle - found %d new jobs',
                      new_job_count)


    def _remove_offloaded_jobs(self):
        """Removed offloaded jobs from `self._open_jobs`."""
        removed_job_count = 0
        for jobkey, job in self._open_jobs.items():
            if (
                    not os.path.exists(job.dirname)
                    or _is_uploaded(job.dirname)):
                del self._open_jobs[jobkey]
                removed_job_count += 1
        logging.debug('End of offload cycle - cleared %d new jobs, '
                      'carrying %d open jobs',
                      removed_job_count, len(self._open_jobs))


    def _report_failed_jobs(self):
        """Report status after attempting offload.

        This function processes all jobs in `self._open_jobs`, assuming
        an attempt has just been made to offload all of them.

        If any jobs have reportable errors, and we haven't generated
        an e-mail report in the last `REPORT_INTERVAL_SECS` seconds,
        send new e-mail describing the failures.

        """
        failed_jobs = [j for j in self._open_jobs.values() if
                       j.first_offload_start]
        self._report_failed_jobs_count(failed_jobs)
        self._log_failed_jobs_locally(failed_jobs)


    def offload_once(self):
        """Perform one offload cycle.

        Find all job directories for new jobs that we haven't seen
        before.  Then, attempt to offload the directories for any
        jobs that have finished running.  Offload of multiple jobs
        is done in parallel, up to `self._processes` at a time.

        After we've tried uploading all directories, go through the list
        checking the status of all uploaded directories.  If necessary,
        report failures via e-mail.

        """
        self._add_new_jobs()
        self._report_current_jobs_count()
        with parallel.BackgroundTaskRunner(
                self._gs_offloader.offload, processes=self._processes) as queue:
            for job in self._open_jobs.values():
                _enqueue_offload(job, queue, self._upload_age_limit)
        self._give_up_on_jobs_over_limit()
        self._remove_offloaded_jobs()
        self._report_failed_jobs()


    def _give_up_on_jobs_over_limit(self):
        """Give up on jobs that have gone over the offload limit.

        We mark them as uploaded as we won't try to offload them any more.
        """
        for job in self._open_jobs.values():
            if job.offload_count >= self._offload_count_limit:
                _mark_uploaded(job.dirname)


    def _log_failed_jobs_locally(self, failed_jobs,
                                 log_file=FAILED_OFFLOADS_FILE):
        """Updates a local file listing all the failed jobs.

        The dropped file can be used by the developers to list jobs that we have
        failed to upload.

        @param failed_jobs: A list of failed _JobDirectory objects.
        @param log_file: The file to log the failed jobs to.
        """
        now = datetime.datetime.now()
        now_str = now.strftime(FAILED_OFFLOADS_TIME_FORMAT)
        formatted_jobs = [_format_job_for_failure_reporting(job)
                            for job in failed_jobs]
        formatted_jobs.sort()

        with open(log_file, 'w') as logfile:
            logfile.write(FAILED_OFFLOADS_FILE_HEADER %
                          (now_str, len(failed_jobs)))
            logfile.writelines(formatted_jobs)


    def _report_current_jobs_count(self):
        """Report the number of outstanding jobs to monarch."""
        metrics.Gauge('chromeos/autotest/gs_offloader/current_jobs_count').set(
                len(self._open_jobs))


    def _report_failed_jobs_count(self, failed_jobs):
        """Report the number of outstanding failed offload jobs to monarch.

        @param: List of failed jobs.
        """
        metrics.Gauge('chromeos/autotest/gs_offloader/failed_jobs_count').set(
                len(failed_jobs))


def _enqueue_offload(job, queue, age_limit):
    """Enqueue the job for offload, if it's eligible.

    The job is eligible for offloading if the database has marked
    it finished, and the job is older than the `age_limit`
    parameter.

    If the job is eligible, offload processing is requested by
    passing the `queue` parameter's `put()` method a sequence with
    the job's `dirname` attribute and its directory name.

    @param job       _JobDirectory instance to offload.
    @param queue     If the job should be offloaded, put the offload
                     parameters into this queue for processing.
    @param age_limit Minimum age for a job to be offloaded.  A value
                     of 0 means that the job will be offloaded as
                     soon as it is finished.

    """
    if not job.offload_count:
        if not _is_expired(job, age_limit):
            return
        job.first_offload_start = time.time()
    job.offload_count += 1
    if job.process_gs_instructions():
        timestamp = job.get_timestamp_if_finished()
        queue.put([job.dirname, os.path.dirname(job.dirname), timestamp])


def parse_options():
    """Parse the args passed into gs_offloader."""
    defaults = 'Defaults:\n  Destination: %s\n  Results Path: %s' % (
            utils.DEFAULT_OFFLOAD_GSURI, RESULTS_DIR)
    usage = 'usage: %prog [options]\n' + defaults
    parser = OptionParser(usage)
    parser.add_option('-a', '--all', dest='process_all',
                      action='store_true',
                      help='Offload all files in the results directory.')
    parser.add_option('-s', '--hosts', dest='process_hosts_only',
                      action='store_true',
                      help='Offload only the special tasks result files '
                      'located in the results/hosts subdirectory')
    parser.add_option('-p', '--parallelism', dest='parallelism',
                      type='int', default=1,
                      help='Number of parallel workers to use.')
    parser.add_option('-o', '--delete_only', dest='delete_only',
                      action='store_true',
                      help='GS Offloader will only the delete the '
                      'directories and will not offload them to google '
                      'storage. NOTE: If global_config variable '
                      'CROS.gs_offloading_enabled is False, --delete_only '
                      'is automatically True.',
                      default=not GS_OFFLOADING_ENABLED)
    parser.add_option('-d', '--days_old', dest='days_old',
                      help='Minimum job age in days before a result can be '
                      'offloaded.', type='int', default=0)
    parser.add_option('-l', '--log_size', dest='log_size',
                      help='Limit the offloader logs to a specified '
                      'number of Mega Bytes.', type='int', default=0)
    parser.add_option('-m', dest='multiprocessing', action='store_true',
                      help='Turn on -m option for gsutil. If not set, the '
                      'global config setting gs_offloader_multiprocessing '
                      'under CROS section is applied.')
    parser.add_option('-i', '--offload_once', dest='offload_once',
                      action='store_true',
                      help='Upload all available results and then exit.')
    parser.add_option('-y', '--normal_priority', dest='normal_priority',
                      action='store_true',
                      help='Upload using normal process priority.')
    parser.add_option('-u', '--age_to_upload', dest='age_to_upload',
                      help='Minimum job age in days before a result can be '
                      'offloaded, but not removed from local storage',
                      type='int', default=None)
    parser.add_option('-n', '--age_to_delete', dest='age_to_delete',
                      help='Minimum job age in days before a result can be '
                      'removed from local storage',
                      type='int', default=None)
    parser.add_option(
            '--metrics-file',
            help='If provided, drop metrics to this local file instead of '
                 'reporting to ts_mon',
            type=str,
            default=None,
    )

    options = parser.parse_args()[0]
    if options.process_all and options.process_hosts_only:
        parser.print_help()
        print ('Cannot process all files and only the hosts '
               'subdirectory. Please remove an argument.')
        sys.exit(1)

    if options.days_old and (options.age_to_upload or options.age_to_delete):
        parser.print_help()
        print('Use the days_old option or the age_to_* options but not both')
        sys.exit(1)

    if options.age_to_upload == None:
        options.age_to_upload = options.days_old
    if options.age_to_delete == None:
        options.age_to_delete = options.days_old

    return options


def main():
    """Main method of gs_offloader."""
    options = parse_options()

    if options.process_all:
        offloader_type = 'all'
    elif options.process_hosts_only:
        offloader_type = 'hosts'
    else:
        offloader_type = 'jobs'

    _setup_logging(options, offloader_type)

    # Nice our process (carried to subprocesses) so we don't overload
    # the system.
    if not options.normal_priority:
        logging.debug('Set process to nice value: %d', NICENESS)
        os.nice(NICENESS)
    if psutil:
        proc = psutil.Process()
        logging.debug('Set process to ionice IDLE')
        proc.ionice(psutil.IOPRIO_CLASS_IDLE)

    # os.listdir returns relative paths, so change to where we need to
    # be to avoid an os.path.join on each loop.
    logging.debug('Offloading Autotest results in %s', RESULTS_DIR)
    os.chdir(RESULTS_DIR)

    service_name = 'gs_offloader(%s)' % offloader_type
    with ts_mon_config.SetupTsMonGlobalState(service_name, indirect=True,
                                             short_lived=False,
                                             debug_file=options.metrics_file):
        with metrics.SuccessCounter('chromeos/autotest/gs_offloader/exit'):
            offloader = Offloader(options)
            if not options.delete_only:
                wait_for_gs_write_access(offloader.gs_uri)
            while True:
                offloader.offload_once()
                if options.offload_once:
                    break
                time.sleep(SLEEP_TIME_SECS)


_LOG_LOCATION = '/usr/local/autotest/logs/'
_LOG_FILENAME_FORMAT = 'gs_offloader_%s_log_%s.txt'
_LOG_TIMESTAMP_FORMAT = '%Y%m%d_%H%M%S'
_LOGGING_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'


def _setup_logging(options, offloader_type):
    """Set up logging.

    @param options: Parsed options.
    @param offloader_type: Type of offloader action as string.
    """
    log_filename = _get_log_filename(options, offloader_type)
    log_formatter = logging.Formatter(_LOGGING_FORMAT)
    # Replace the default logging handler with a RotatingFileHandler. If
    # options.log_size is 0, the file size will not be limited. Keeps
    # one backup just in case.
    handler = logging.handlers.RotatingFileHandler(
            log_filename, maxBytes=1024 * options.log_size, backupCount=1)
    handler.setFormatter(log_formatter)
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    logger.addHandler(handler)


def _get_log_filename(options, offloader_type):
    """Get log filename.

    @param options: Parsed options.
    @param offloader_type: Type of offloader action as string.
    """
    if options.log_size > 0:
        log_timestamp = ''
    else:
        log_timestamp = time.strftime(_LOG_TIMESTAMP_FORMAT)
    log_basename = _LOG_FILENAME_FORMAT % (offloader_type, log_timestamp)
    return os.path.join(_LOG_LOCATION, log_basename)


if __name__ == '__main__':
    main()
