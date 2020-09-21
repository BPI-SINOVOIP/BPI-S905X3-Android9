# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import re
import shutil
from autotest_lib.client.common_lib import utils as client_utils
from autotest_lib.client.common_lib.cros import dev_server
from autotest_lib.client.common_lib.cros import retry
from autotest_lib.client.cros import constants
from autotest_lib.server.cros.dynamic_suite.constants import JOB_BUILD_KEY
from autotest_lib.server.crashcollect import collect_log_file
from autotest_lib.server import utils

try:
    from chromite.lib import metrics
except ImportError:
    metrics = client_utils.metrics_mock


def generate_minidump_stacktrace(minidump_path):
    """
    Generates a stacktrace for the specified minidump.

    This function expects the debug symbols to reside under:
        /build/<board>/usr/lib/debug

    @param minidump_path: absolute path to minidump to by symbolicated.
    @raise client_utils.error.CmdError if minidump_stackwalk return code != 0.
    """
    symbol_dir = '%s/../../../lib/debug' % utils.get_server_dir()
    logging.info('symbol_dir: %s', symbol_dir)
    client_utils.run('minidump_stackwalk "%s" "%s" > "%s.txt"' %
                     (minidump_path, symbol_dir, minidump_path))


def _resolve_crashserver():
    """
    Attempts to find a devserver / crashserver that has capacity to
    symbolicate a crashdump.

    @raises DevServerException if no server with capacity could be found.
    @returns Hostname of resolved server, if found.
    """
    crashserver_name = dev_server.get_least_loaded_devserver(
            devserver_type=dev_server.CrashServer)
    if not crashserver_name:
        metrics.Counter('chromeos/autotest/crashcollect/could_not_resolve'
                        ).increment()
        raise dev_server.DevServerException(
                'No crash server has the capacity to symbolicate the dump.')
    else:
        metrics.Counter('chromeos/autotest/crashcollect/resolved'
                        ).increment(fields={'crash_server': crashserver_name})
    return crashserver_name


def _symbolicate_minidump_with_devserver(minidump_path, resultdir,
                                        crashserver_name):
    """
    Generates a stack trace for the specified minidump by consulting devserver.

    This function assumes the debug symbols have been staged on the devserver.

    @param minidump_path: absolute path to minidump to by symbolicated.
    @param resultdir: server job's result directory.
    @param crashserver_name: Name of crashserver to attempt to symbolicate with.
    @raise DevServerException upon failure, HTTP or otherwise.
    """
    # First, look up what build we tested.  If we can't find this, we can't
    # get the right debug symbols, so we might as well give up right now.
    keyvals = client_utils.read_keyval(resultdir)
    if JOB_BUILD_KEY not in keyvals:
        raise dev_server.DevServerException(
            'Cannot determine build being tested.')

    devserver = dev_server.CrashServer(crashserver_name)

    with metrics.SecondsTimer(
            'chromeos/autotest/crashcollect/symbolicate_duration',
            fields={'crash_server': crashserver_name}):
        trace_text = devserver.symbolicate_dump(minidump_path,
                                                keyvals[JOB_BUILD_KEY])

    if not trace_text:
        raise dev_server.DevServerException('Unknown error!!')
    with open(minidump_path + '.txt', 'w') as trace_file:
        trace_file.write(trace_text)

def generate_stacktrace_for_file(minidump, host_resultdir):
    """
    Tries to generate a stack trace for the file located at |minidump|.
    @param minidump: path to minidump file to generate the stacktrace for.
    @param host_resultdir: server job's result directory.
    """
    # First, try to symbolicate locally.
    try:
        logging.info('Trying to generate stack trace locally for %s', minidump)
        generate_minidump_stacktrace(minidump)
        logging.info('Generated stack trace for dump %s', minidump)
        return
    except client_utils.error.CmdError as err:
        logging.info('Failed to generate stack trace locally for '
                     'dump %s (rc=%d):\n%r',
                     minidump, err.result_obj.exit_status, err)

    # If that did not succeed, try to symbolicate using the dev server.
    try:
        logging.info('Generating stack trace using devserver for %s', minidump)
        crashserver_name = _resolve_crashserver()
        args = (minidump, host_resultdir, crashserver_name)
        is_timeout, _ = retry.timeout(_symbolicate_minidump_with_devserver,
                                      args=args,
                                      timeout_sec=600)
        if is_timeout:
            logging.info('Generating stack trace timed out for dump %s',
                         minidump)
            metrics.Counter(
                    'chromeos/autotest/crashcollect/symbolicate_timed_out'
            ).increment(fields={'crash_server': crashserver_name})
        else:
            logging.info('Generated stack trace for dump %s', minidump)
            return
    except dev_server.DevServerException as e:
        logging.info('Failed to generate stack trace on devserver for dump '
                     '%s:\n%r', minidump, e)

    # Symbolicating failed.
    logging.warning('Failed to generate stack trace for %s (see info logs)',
                    minidump)

def find_and_generate_minidump_stacktraces(host_resultdir):
    """
    Finds all minidump files and generates a stack trace for each.

    Enumerates all files under the test results directory (recursively)
    and generates a stack trace file for the minidumps.  Minidump files are
    identified as files with .dmp extension.  The stack trace filename is
    composed by appending the .txt extension to the minidump filename.

    @param host_resultdir: Directory to walk looking for dmp files.

    @returns The list of all found minidump files. Each dump may or may not have
             been symbolized.
    """
    minidumps = []
    for file in _find_crashdumps(host_resultdir):
        generate_stacktrace_for_file(file, host_resultdir)
        minidumps.append(file)
    return minidumps


def _find_crashdumps(host_resultdir):
    """Find crashdumps.

    @param host_resultdir The result directory for this host for this test run.
    """
    for dir, subdirs, files in os.walk(host_resultdir):
        for file in files:
            if file.endswith('.dmp'):
                yield os.path.join(dir, file)


def _find_orphaned_crashdumps(host):
    """Return file paths of crashdumps on host.

    @param host A host object of the device.
    """
    return host.list_files_glob(os.path.join(constants.CRASH_DIR, '*'))


def report_crashdumps(host):
    """Report on crashdumps for host.

    This is run when no tests failed.  We don't process crashdumps in this
    case because of devserver load, but they should still be reported.

    @param host A host object of the device we're to pull crashes from.
    """
    for crashfile in _find_orphaned_crashdumps(host):
        logging.warning('Host crashdump exists: %s', crashfile)
        host.job.record('INFO', None, None,
                        'Host crashdump exists: %s' % (crashfile,))

    host_resultdir = _get_host_resultdir(host)
    for crashfile in _find_crashdumps(host_resultdir):
        logging.warning('Local crashdump exists: %s', crashfile)
        host.job.record('INFO', None, None,
                        'Local crashdump exists: %s' % (crashfile,))


def fetch_orphaned_crashdumps(host, infodir):
    """
    Copy all of the crashes in the crash directory over to the results folder.

    @param host A host object of the device we're to pull crashes from.
    @param infodir The directory to fetch crashdumps into.
    @return The list of minidumps that we pulled back from the host.
    """
    if not os.path.exists(infodir):
        os.mkdir(infodir)
    orphans = []

    if not host.check_cached_up_status():
        logging.warning('Host %s did not answer to ping, skip fetching '
                        'orphaned crashdumps.', host.hostname)
        return orphans

    try:
        for file in _find_orphaned_crashdumps(host):
            logging.info('Collecting %s...', file)
            collect_log_file(host, file, infodir, clean=True)
            orphans.append(file)
    except Exception as e:
        logging.warning('Collection of orphaned crash dumps failed %s', e)
    finally:
        # Delete infodir if we have no orphans
        if not orphans:
            logging.info('There are no orphaned crashes; deleting %s', infodir)
            os.rmdir(infodir)
    return orphans


def _copy_to_debug_dir(host_resultdir, filename):
    """
    Copies a file to the debug dir under host_resultdir.

    @param host_resultdir The result directory for this host for this test run.
    @param filename The full path of the file to copy to the debug folder.
    """
    debugdir = os.path.join(host_resultdir, 'debug')
    src = filename
    dst = os.path.join(debugdir, os.path.basename(filename))

    try:
        shutil.copyfile(src, dst)
        logging.info('Copied %s to %s', src, dst)
    except IOError:
        logging.warning('Failed to copy %s to %s', src, dst)


def _get_host_resultdir(host):
    """Get resultdir for host.

    @param host A host object of the device we're to pull crashes from.
    """
    return getattr(getattr(host, 'job', None), 'resultdir', None)


def get_host_infodir(host):
    """Get infodir for host.

    @param host A host object of the device we're to pull crashes from.
    """
    host_resultdir = _get_host_resultdir(host)
    return os.path.join(host_resultdir, 'crashinfo.%s' % host.hostname)


def get_site_crashdumps(host, test_start_time):
    """
    Copy all of the crashdumps from a host to the results directory.

    @param host The host object from which to pull crashes
    @param test_start_time When the test we just ran started.
    @return A list of all the minidumps
    """
    host_resultdir = _get_host_resultdir(host)
    infodir = get_host_infodir(host)

    orphans = fetch_orphaned_crashdumps(host, infodir)
    minidumps = find_and_generate_minidump_stacktraces(host_resultdir)

    # Record all crashdumps in status.log of the job:
    # - If one server job runs several client jobs we will only record
    # crashdumps in the status.log of the high level server job.
    # - We will record these crashdumps whether or not we successfully
    # symbolicate them.
    if host.job and minidumps or orphans:
        host.job.record('INFO', None, None, 'Start crashcollection record')
        for minidump in minidumps:
            host.job.record('INFO', None, 'New Crash Dump', minidump)
        for orphan in orphans:
            host.job.record('INFO', None, 'Orphaned Crash Dump', orphan)
        host.job.record('INFO', None, None, 'End crashcollection record')

    orphans.extend(minidumps)

    for minidump in orphans:
        report_bug_from_crash(host, minidump)

    # We copy Chrome crash information to the debug dir to assist debugging.
    # Since orphans occurred on a previous run, they are most likely not
    # relevant to the current failure, so we don't copy them.
    for minidump in minidumps:
        minidump_no_ext = os.path.splitext(minidump)[0]
        _copy_to_debug_dir(host_resultdir, minidump_no_ext + '.dmp.txt')
        _copy_to_debug_dir(host_resultdir, minidump_no_ext + '.log')

    return orphans


def find_package_of(host, exec_name):
    """
    Find the package that an executable came from.

    @param host A host object that has the executable.
    @param exec_name Name of or path to executable.
    @return The name of the package that installed the executable.
    """
    # Run "portageq owners" on "host" to determine which package owns
    # "exec_name."  Portageq queue output consists of package names followed
    # tab-prefixed path names.  For example, owners of "python:"
    #
    # sys-devel/gdb-7.7.1-r2
    #         /usr/share/gdb/python
    # chromeos-base/dev-install-0.0.1-r711
    #         /usr/bin/python
    # dev-lang/python-2.7.3-r7
    #         /etc/env.d/python
    #
    # This gets piped into "xargs stat" to annotate each line with
    # information about the path, so we later can consider only packages
    # with executable files.  After annotation the above looks like:
    #
    # stat: cannot stat '@@@ sys-devel/gdb-7.7.1-r2 @@@': ...
    # stat: cannot stat '/usr/share/gdb/python': ...
    # stat: cannot stat '@@@ chromeos-base/dev-install-0.0.1-r711 @@@': ...
    # 755 -rwxr-xr-x /usr/bin/python
    # stat: cannot stat '@@@ dev-lang/python-2.7.3-r7 @@@': ...
    # 755 drwxr-xr-x /etc/env.d/python
    #
    # Package names are surrounded by "@@@" to facilitate parsing.  Lines
    # starting with an octal number were successfully annotated, because
    # the path existed on "host."
    # The above is then parsed to find packages which contain executable files
    # (not directories), in this case "chromeos-base/dev-install-0.0.1-r711."
    #
    # TODO(milleral): portageq can show scary looking error messages
    # in the debug logs via stderr. We only look at stdout, so those
    # get filtered, but it would be good to silence them.
    cmd = ('portageq owners / ' + exec_name +
            r'| sed -e "s/^[^\t].*/@@@ & @@@/" -e "s/^\t//"'
            r'| tr \\n \\0'
            ' | xargs -0 -r stat -L -c "%a %A %n" 2>&1')
    portageq = host.run(cmd, ignore_status=True)

    # Parse into a set of names of packages containing an executable file.
    packages = set()
    pkg = ''
    pkg_re = re.compile('@@@ (.*) @@@')
    path_re = re.compile('^([0-7]{3,}) (.)')
    for line in portageq.stdout.splitlines():
        match = pkg_re.search(line)
        if match:
            pkg = match.group(1)
            continue
        match = path_re.match(line)
        if match:
            isexec = int(match.group(1), 8) & 0o111
            isfile = match.group(2) == '-'
            if pkg and isexec and isfile:
                packages.add(pkg)

    # If exactly one package found it must be the one we want, return it.
    if len(packages) == 1:
        return packages.pop()

    # TODO(milleral): Decide if it really is an error if not exactly one
    # package is found.
    # It is highly questionable as to if this should be left in the
    # production version of this code or not.
    if len(packages) == 0:
        logging.warning('find_package_of() found no packages for "%s"',
                        exec_name)
    else:
        logging.warning('find_package_of() found multiple packages for "%s": '
                        '%s', exec_name, ', '.join(packages))
    return ''


def report_bug_from_crash(host, minidump_path):
    """
    Given a host to query and a minidump, file a bug about the crash.

    @param host A host object that is where the dump came from
    @param minidump_path The path to the dump file that should be reported.
    """
    # TODO(milleral): Once this has actually been tested, remove the
    # try/except. In the meantime, let's make sure nothing dies because of
    # the fact that this code isn't very heavily tested.
    try:
        meta_path = os.path.splitext(minidump_path)[0] + '.meta'
        with open(meta_path, 'r') as f:
            for line in f.readlines():
                parts = line.split('=')
                if parts[0] == 'exec_name':
                    package = find_package_of(host, parts[1].strip())
                    if not package:
                        package = '<unknown package>'
                    logging.info('Would report crash on %s.', package)
                    break
    except Exception as e:
        logging.warning('Crash detection failed with: %s', e)
