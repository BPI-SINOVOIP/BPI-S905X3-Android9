# Copyright (c) 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


import contextlib
import grp
import httplib
import json
import logging
import os
import random
import re
import time
import traceback
import urllib2

import common
from autotest_lib.client.bin.result_tools import utils as result_utils
from autotest_lib.client.bin.result_tools import utils_lib as result_utils_lib
from autotest_lib.client.bin.result_tools import view as result_view
from autotest_lib.client.common_lib import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import file_utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import host_queue_entry_states
from autotest_lib.client.common_lib import host_states
from autotest_lib.server.cros import provision
from autotest_lib.server.cros.dynamic_suite import constants
from autotest_lib.server.cros.dynamic_suite import job_status

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


CONFIG = global_config.global_config

_SHERIFF_JS = CONFIG.get_config_value('NOTIFICATIONS', 'sheriffs', default='')
_LAB_SHERIFF_JS = CONFIG.get_config_value(
        'NOTIFICATIONS', 'lab_sheriffs', default='')
_CHROMIUM_BUILD_URL = CONFIG.get_config_value(
        'NOTIFICATIONS', 'chromium_build_url', default='')

LAB_GOOD_STATES = ('open', 'throttled')

ENABLE_DRONE_IN_RESTRICTED_SUBNET = CONFIG.get_config_value(
        'CROS', 'enable_drone_in_restricted_subnet', type=bool,
        default=False)

# Wait at most 10 mins for duts to go idle.
IDLE_DUT_WAIT_TIMEOUT = 600

# Mapping between board name and build target. This is for special case handling
# for certain Android board that the board name and build target name does not
# match.
ANDROID_TARGET_TO_BOARD_MAP = {
        'seed_l8150': 'gm4g_sprout',
        'bat_land': 'bat'
        }
ANDROID_BOARD_TO_TARGET_MAP = {
        'gm4g_sprout': 'seed_l8150',
        'bat': 'bat_land'
        }
# Prefix for the metrics name for result size information.
RESULT_METRICS_PREFIX = 'chromeos/autotest/result_collection/'

class TestLabException(Exception):
    """Exception raised when the Test Lab blocks a test or suite."""
    pass


class ParseBuildNameException(Exception):
    """Raised when ParseBuildName() cannot parse a build name."""
    pass


class Singleton(type):
    """Enforce that only one client class is instantiated per process."""
    _instances = {}

    def __call__(cls, *args, **kwargs):
        """Fetch the instance of a class to use for subsequent calls."""
        if cls not in cls._instances:
            cls._instances[cls] = super(Singleton, cls).__call__(
                    *args, **kwargs)
        return cls._instances[cls]

class EmptyAFEHost(object):
    """Object to represent an AFE host object when there is no AFE."""

    def __init__(self):
        """
        We'll be setting the instance attributes as we use them.  Right now
        we only use attributes and labels but as time goes by and other
        attributes are used from an actual AFE Host object (check
        rpc_interfaces.get_hosts()), we'll add them in here so users won't be
        perplexed why their host's afe_host object complains that attribute
        doesn't exist.
        """
        self.attributes = {}
        self.labels = []


def ParseBuildName(name):
    """Format a build name, given board, type, milestone, and manifest num.

    @param name: a build name, e.g. 'x86-alex-release/R20-2015.0.0' or a
                 relative build name, e.g. 'x86-alex-release/LATEST'

    @return board: board the manifest is for, e.g. x86-alex.
    @return type: one of 'release', 'factory', or 'firmware'
    @return milestone: (numeric) milestone the manifest was associated with.
                        Will be None for relative build names.
    @return manifest: manifest number, e.g. '2015.0.0'.
                      Will be None for relative build names.

    """
    match = re.match(r'(trybot-)?(?P<board>[\w-]+?)(?:-chrome)?(?:-chromium)?'
                     r'-(?P<type>\w+)/(R(?P<milestone>\d+)-'
                     r'(?P<manifest>[\d.ab-]+)|LATEST)',
                     name)
    if match and len(match.groups()) >= 5:
        return (match.group('board'), match.group('type'),
                match.group('milestone'), match.group('manifest'))
    raise ParseBuildNameException('%s is a malformed build name.' % name)


def get_labels_from_afe(hostname, label_prefix, afe):
    """Retrieve a host's specific labels from the AFE.

    Looks for the host labels that have the form <label_prefix>:<value>
    and returns the "<value>" part of the label. None is returned
    if there is not a label matching the pattern

    @param hostname: hostname of given DUT.
    @param label_prefix: prefix of label to be matched, e.g., |board:|
    @param afe: afe instance.

    @returns A list of labels that match the prefix or 'None'

    """
    labels = afe.get_labels(name__startswith=label_prefix,
                            host__hostname__in=[hostname])
    if labels:
        return [l.name.split(label_prefix, 1)[1] for l in labels]


def get_label_from_afe(hostname, label_prefix, afe):
    """Retrieve a host's specific label from the AFE.

    Looks for a host label that has the form <label_prefix>:<value>
    and returns the "<value>" part of the label. None is returned
    if there is not a label matching the pattern

    @param hostname: hostname of given DUT.
    @param label_prefix: prefix of label to be matched, e.g., |board:|
    @param afe: afe instance.
    @returns the label that matches the prefix or 'None'

    """
    labels = get_labels_from_afe(hostname, label_prefix, afe)
    if labels and len(labels) == 1:
        return labels[0]


def get_board_from_afe(hostname, afe):
    """Retrieve given host's board from its labels in the AFE.

    Looks for a host label of the form "board:<board>", and
    returns the "<board>" part of the label.  `None` is returned
    if there is not a single, unique label matching the pattern.

    @param hostname: hostname of given DUT.
    @param afe: afe instance.
    @returns board from label, or `None`.

    """
    return get_label_from_afe(hostname, constants.BOARD_PREFIX, afe)


def get_build_from_afe(hostname, afe):
    """Retrieve the current build for given host from the AFE.

    Looks through the host's labels in the AFE to determine its build.

    @param hostname: hostname of given DUT.
    @param afe: afe instance.
    @returns The current build or None if it could not find it or if there
             were multiple build labels assigned to this host.

    """
    for prefix in [provision.CROS_VERSION_PREFIX,
                   provision.ANDROID_BUILD_VERSION_PREFIX]:
        build = get_label_from_afe(hostname, prefix + ':', afe)
        if build:
            return build
    return None


# TODO(fdeng): fix get_sheriffs crbug.com/483254
def get_sheriffs(lab_only=False):
    """
    Polls the javascript file that holds the identity of the sheriff and
    parses it's output to return a list of chromium sheriff email addresses.
    The javascript file can contain the ldap of more than one sheriff, eg:
    document.write('sheriff_one, sheriff_two').

    @param lab_only: if True, only pulls lab sheriff.
    @return: A list of chroium.org sheriff email addresses to cc on the bug.
             An empty list if failed to parse the javascript.
    """
    sheriff_ids = []
    sheriff_js_list = _LAB_SHERIFF_JS.split(',')
    if not lab_only:
        sheriff_js_list.extend(_SHERIFF_JS.split(','))

    for sheriff_js in sheriff_js_list:
        try:
            url_content = utils.urlopen('%s%s'% (
                _CHROMIUM_BUILD_URL, sheriff_js)).read()
        except (ValueError, IOError) as e:
            logging.warning('could not parse sheriff from url %s%s: %s',
                             _CHROMIUM_BUILD_URL, sheriff_js, str(e))
        except (urllib2.URLError, httplib.HTTPException) as e:
            logging.warning('unexpected error reading from url "%s%s": %s',
                             _CHROMIUM_BUILD_URL, sheriff_js, str(e))
        else:
            ldaps = re.search(r"document.write\('(.*)'\)", url_content)
            if not ldaps:
                logging.warning('Could not retrieve sheriff ldaps for: %s',
                                 url_content)
                continue
            sheriff_ids += ['%s@chromium.org' % alias.replace(' ', '')
                            for alias in ldaps.group(1).split(',')]
    return sheriff_ids


def remote_wget(source_url, dest_path, ssh_cmd):
    """wget source_url from localhost to dest_path on remote host using ssh.

    @param source_url: The complete url of the source of the package to send.
    @param dest_path: The path on the remote host's file system where we would
        like to store the package.
    @param ssh_cmd: The ssh command to use in performing the remote wget.
    """
    wget_cmd = ("wget -O - %s | %s 'cat >%s'" %
                (source_url, ssh_cmd, dest_path))
    utils.run(wget_cmd)


_MAX_LAB_STATUS_ATTEMPTS = 5
def _get_lab_status(status_url):
    """Grabs the current lab status and message.

    @returns The JSON object obtained from the given URL.

    """
    retry_waittime = 1
    for _ in range(_MAX_LAB_STATUS_ATTEMPTS):
        try:
            response = urllib2.urlopen(status_url)
        except IOError as e:
            logging.debug('Error occurred when grabbing the lab status: %s.',
                          e)
            time.sleep(retry_waittime)
            continue
        # Check for successful response code.
        if response.getcode() == 200:
            return json.load(response)
        time.sleep(retry_waittime)
    return None


def _decode_lab_status(lab_status, build):
    """Decode lab status, and report exceptions as needed.

    Take a deserialized JSON object from the lab status page, and
    interpret it to determine the actual lab status.  Raise
    exceptions as required to report when the lab is down.

    @param build: build name that we want to check the status of.

    @raises TestLabException Raised if a request to test for the given
                             status and build should be blocked.
    """
    # First check if the lab is up.
    if not lab_status['general_state'] in LAB_GOOD_STATES:
        raise TestLabException('Chromium OS Test Lab is closed: '
                               '%s.' % lab_status['message'])

    # Check if the build we wish to use is disabled.
    # Lab messages should be in the format of:
    #    Lab is 'status' [regex ...] (comment)
    # If the build name matches any regex, it will be blocked.
    build_exceptions = re.search('\[(.*)\]', lab_status['message'])
    if not build_exceptions or not build:
        return
    for build_pattern in build_exceptions.group(1).split():
        if re.match(build_pattern, build):
            raise TestLabException('Chromium OS Test Lab is closed: '
                                   '%s matches %s.' % (
                                           build, build_pattern))
    return


def is_in_lab():
    """Check if current Autotest instance is in lab

    @return: True if the Autotest instance is in lab.
    """
    test_server_name = CONFIG.get_config_value('SERVER', 'hostname')
    return test_server_name.startswith('cautotest')


def check_lab_status(build):
    """Check if the lab status allows us to schedule for a build.

    Checks if the lab is down, or if testing for the requested build
    should be blocked.

    @param build: Name of the build to be scheduled for testing.

    @raises TestLabException Raised if a request to test for the given
                             status and build should be blocked.

    """
    # Ensure we are trying to schedule on the actual lab.
    if not is_in_lab():
        return

    # Download the lab status from its home on the web.
    status_url = CONFIG.get_config_value('CROS', 'lab_status_url')
    json_status = _get_lab_status(status_url)
    if json_status is None:
        # We go ahead and say the lab is open if we can't get the status.
        logging.warning('Could not get a status from %s', status_url)
        return
    _decode_lab_status(json_status, build)


def lock_host_with_labels(afe, lock_manager, labels):
    """Lookup and lock one host that matches the list of input labels.

    @param afe: An instance of the afe class, as defined in server.frontend.
    @param lock_manager: A lock manager capable of locking hosts, eg the
        one defined in server.cros.host_lock_manager.
    @param labels: A list of labels to look for on hosts.

    @return: The hostname of a host matching all labels, and locked through the
        lock_manager. The hostname will be as specified in the database the afe
        object is associated with, i.e if it exists in afe_hosts with a .cros
        suffix, the hostname returned will contain a .cros suffix.

    @raises: error.NoEligibleHostException: If no hosts matching the list of
        input labels are available.
    @raises: error.TestError: If unable to lock a host matching the labels.
    """
    potential_hosts = afe.get_hosts(multiple_labels=labels)
    if not potential_hosts:
        raise error.NoEligibleHostException(
                'No devices found with labels %s.' % labels)

    # This prevents errors where a fault might seem repeatable
    # because we lock, say, the same packet capturer for each test run.
    random.shuffle(potential_hosts)
    for host in potential_hosts:
        if lock_manager.lock([host.hostname]):
            logging.info('Locked device %s with labels %s.',
                         host.hostname, labels)
            return host.hostname
        else:
            logging.info('Unable to lock device %s with labels %s.',
                         host.hostname, labels)

    raise error.TestError('Could not lock a device with labels %s' % labels)


def get_test_views_from_tko(suite_job_id, tko):
    """Get test name and result for given suite job ID.

    @param suite_job_id: ID of suite job.
    @param tko: an instance of TKO as defined in server/frontend.py.
    @return: A dictionary of test status keyed by test name, e.g.,
             {'dummy_Fail.Error': 'ERROR', 'dummy_Fail.NAError': 'TEST_NA'}
    @raise: Exception when there is no test view found.

    """
    views = tko.run('get_detailed_test_views', afe_job_id=suite_job_id)
    relevant_views = filter(job_status.view_is_relevant, views)
    if not relevant_views:
        raise Exception('Failed to retrieve job results.')

    test_views = {}
    for view in relevant_views:
        test_views[view['test_name']] = view['status']

    return test_views


def get_data_key(prefix, suite, build, board):
    """
    Constructs a key string from parameters.

    @param prefix: Prefix for the generating key.
    @param suite: a suite name. e.g., bvt-cq, bvt-inline, dummy
    @param build: The build string. This string should have a consistent
        format eg: x86-mario-release/R26-3570.0.0. If the format of this
        string changes such that we can't determine build_type or branch
        we give up and use the parametes we're sure of instead (suite,
        board). eg:
            1. build = x86-alex-pgo-release/R26-3570.0.0
               branch = 26
               build_type = pgo-release
            2. build = lumpy-paladin/R28-3993.0.0-rc5
               branch = 28
               build_type = paladin
    @param board: The board that this suite ran on.
    @return: The key string used for a dictionary.
    """
    try:
        _board, build_type, branch = ParseBuildName(build)[:3]
    except ParseBuildNameException as e:
        logging.error(str(e))
        branch = 'Unknown'
        build_type = 'Unknown'
    else:
        embedded_str = re.search(r'x86-\w+-(.*)', _board)
        if embedded_str:
            build_type = embedded_str.group(1) + '-' + build_type

    data_key_dict = {
        'prefix': prefix,
        'board': board,
        'branch': branch,
        'build_type': build_type,
        'suite': suite,
    }
    return ('%(prefix)s.%(board)s.%(build_type)s.%(branch)s.%(suite)s'
            % data_key_dict)


def setup_logging(logfile=None, prefix=False):
    """Setup basic logging with all logging info stripped.

    Calls to logging will only show the message. No severity is logged.

    @param logfile: If specified dump output to a file as well.
    @param prefix: Flag for log prefix. Set to True to add prefix to log
        entries to include timestamp and log level. Default is False.
    """
    # Remove all existing handlers. client/common_lib/logging_config adds
    # a StreamHandler to logger when modules are imported, e.g.,
    # autotest_lib.client.bin.utils. A new StreamHandler will be added here to
    # log only messages, not severity.
    logging.getLogger().handlers = []

    if prefix:
        log_format = '%(asctime)s %(levelname)-5s| %(message)s'
    else:
        log_format = '%(message)s'

    screen_handler = logging.StreamHandler()
    screen_handler.setFormatter(logging.Formatter(log_format))
    logging.getLogger().addHandler(screen_handler)
    logging.getLogger().setLevel(logging.INFO)
    if logfile:
        file_handler = logging.FileHandler(logfile)
        file_handler.setFormatter(logging.Formatter(log_format))
        file_handler.setLevel(logging.DEBUG)
        logging.getLogger().addHandler(file_handler)


def is_shard():
    """Determines if this instance is running as a shard.

    Reads the global_config value shard_hostname in the section SHARD.

    @return True, if shard_hostname is set, False otherwise.
    """
    hostname = CONFIG.get_config_value('SHARD', 'shard_hostname', default=None)
    return bool(hostname)


def get_global_afe_hostname():
    """Read the hostname of the global AFE from the global configuration."""
    return CONFIG.get_config_value('SERVER', 'global_afe_hostname')


def is_restricted_user(username):
    """Determines if a user is in a restricted group.

    User in restricted group only have access to master.

    @param username: A string, representing a username.

    @returns: True if the user is in a restricted group.
    """
    if not username:
        return False

    restricted_groups = CONFIG.get_config_value(
            'AUTOTEST_WEB', 'restricted_groups', default='').split(',')
    for group in restricted_groups:
        try:
            if group and username in grp.getgrnam(group).gr_mem:
                return True
        except KeyError as e:
            logging.debug("%s is not a valid group.", group)
    return False


def get_special_task_status(is_complete, success, is_active):
    """Get the status of a special task.

    Emulate a host queue entry status for a special task
    Although SpecialTasks are not HostQueueEntries, it is helpful to
    the user to present similar statuses.

    @param is_complete    Boolean if the task is completed.
    @param success        Boolean if the task succeeded.
    @param is_active      Boolean if the task is active.

    @return The status of a special task.
    """
    if is_complete:
        if success:
            return host_queue_entry_states.Status.COMPLETED
        return host_queue_entry_states.Status.FAILED
    if is_active:
        return host_queue_entry_states.Status.RUNNING
    return host_queue_entry_states.Status.QUEUED


def get_special_task_exec_path(hostname, task_id, task_name, time_requested):
    """Get the execution path of the SpecialTask.

    This method returns different paths depending on where a
    the task ran:
        * Master: hosts/hostname/task_id-task_type
        * Shard: Master_path/time_created
    This is to work around the fact that a shard can fail independent
    of the master, and be replaced by another shard that has the same
    hosts. Without the time_created stamp the logs of the tasks running
    on the second shard will clobber the logs from the first in google
    storage, because task ids are not globally unique.

    @param hostname        Hostname
    @param task_id         Special task id
    @param task_name       Special task name (e.g., Verify, Repair, etc)
    @param time_requested  Special task requested time.

    @return An execution path for the task.
    """
    results_path = 'hosts/%s/%s-%s' % (hostname, task_id, task_name.lower())

    # If we do this on the master it will break backward compatibility,
    # as there are tasks that currently don't have timestamps. If a host
    # or job has been sent to a shard, the rpc for that host/job will
    # be redirected to the shard, so this global_config check will happen
    # on the shard the logs are on.
    if not is_shard():
        return results_path

    # Generate a uid to disambiguate special task result directories
    # in case this shard fails. The simplest uid is the job_id, however
    # in rare cases tasks do not have jobs associated with them (eg:
    # frontend verify), so just use the creation timestamp. The clocks
    # between a shard and master should always be in sync. Any discrepancies
    # will be brought to our attention in the form of job timeouts.
    uid = time_requested.strftime('%Y%d%m%H%M%S')

    # TODO: This is a hack, however it is the easiest way to achieve
    # correctness. There is currently some debate over the future of
    # tasks in our infrastructure and refactoring everything right
    # now isn't worth the time.
    return '%s/%s' % (results_path, uid)


def get_job_tag(id, owner):
    """Returns a string tag for a job.

    @param id    Job id
    @param owner Job owner

    """
    return '%s-%s' % (id, owner)


def get_hqe_exec_path(tag, execution_subdir):
    """Returns a execution path to a HQE's results.

    @param tag               Tag string for a job associated with a HQE.
    @param execution_subdir  Execution sub-directory string of a HQE.

    """
    return os.path.join(tag, execution_subdir)


def is_inside_chroot():
    """Check if the process is running inside chroot.

    This is a wrapper around chromite.lib.cros_build_lib.IsInsideChroot(). The
    method checks if cros_build_lib can be imported first.

    @return: True if the process is running inside chroot or cros_build_lib
             cannot be imported.

    """
    try:
        # TODO(crbug.com/739466) This module import is delayed because it adds
        # 1-2 seconds to the module import time and most users of site_utils
        # don't need it. The correct fix is to break apart site_utils into more
        # meaningful chunks.
        from chromite.lib import cros_build_lib
    except ImportError:
        logging.warn('Unable to import chromite. Can not detect chroot. '
                     'Defaulting to False')
        return False
    return cros_build_lib.IsInsideChroot()


def parse_job_name(name):
    """Parse job name to get information including build, board and suite etc.

    Suite job created by run_suite follows the naming convention of:
    [build]-test_suites/control.[suite]
    For example: lumpy-release/R46-7272.0.0-test_suites/control.bvt
    The naming convention is defined in rpc_interface.create_suite_job.

    Test job created by suite job follows the naming convention of:
    [build]/[suite]/[test name]
    For example: lumpy-release/R46-7272.0.0/bvt/login_LoginSuccess
    The naming convention is defined in
    server/cros/dynamic_suite/tools.create_job_name

    Note that pgo and chrome-perf builds will fail the method. Since lab does
    not run test for these builds, they can be ignored.
    Also, tests for Launch Control builds have different naming convention.
    The build ID will be used as build_version.

    @param name: Name of the job.

    @return: A dictionary containing the test information. The keyvals include:
             build: Name of the build, e.g., lumpy-release/R46-7272.0.0
             build_version: The version of the build, e.g., R46-7272.0.0
             board: Name of the board, e.g., lumpy
             suite: Name of the test suite, e.g., bvt

    """
    info = {}
    suite_job_regex = '([^/]*/[^/]*(?:/\d+)?)-test_suites/control\.(.*)'
    test_job_regex = '([^/]*/[^/]*(?:/\d+)?)/([^/]+)/.*'
    match = re.match(suite_job_regex, name)
    if not match:
        match = re.match(test_job_regex, name)
    if match:
        info['build'] = match.groups()[0]
        info['suite'] = match.groups()[1]
        info['build_version'] = info['build'].split('/')[1]
        try:
            info['board'], _, _, _ = ParseBuildName(info['build'])
        except ParseBuildNameException:
            # Try to parse it as Launch Control build
            # Launch Control builds have name format:
            # branch/build_target-build_type/build_id.
            try:
                _, target, build_id = utils.parse_launch_control_build(
                        info['build'])
                build_target, _ = utils.parse_launch_control_target(target)
                if build_target:
                    info['board'] = build_target
                    info['build_version'] = build_id
            except ValueError:
                pass
    return info


def add_label_detector(label_function_list, label_list=None, label=None):
    """Decorator used to group functions together into the provided list.

    This is a helper function to automatically add label functions that have
    the label decorator.  This is to help populate the class list of label
    functions to be retrieved by the get_labels class method.

    @param label_function_list: List of label detecting functions to add
                                decorated function to.
    @param label_list: List of detectable labels to add detectable labels to.
                       (Default: None)
    @param label: Label string that is detectable by this detection function
                  (Default: None)
    """
    def add_func(func):
        """
        @param func: The function to be added as a detector.
        """
        label_function_list.append(func)
        if label and label_list is not None:
            label_list.append(label)
        return func
    return add_func


def verify_not_root_user():
    """Simple function to error out if running with uid == 0"""
    if os.getuid() == 0:
        raise error.IllegalUser('This script can not be ran as root.')


def get_hostname_from_machine(machine):
    """Lookup hostname from a machine string or dict.

    @returns: Machine hostname in string format.
    """
    hostname, _ = get_host_info_from_machine(machine)
    return hostname


def get_host_info_from_machine(machine):
    """Lookup host information from a machine string or dict.

    @returns: Tuple of (hostname, afe_host)
    """
    if isinstance(machine, dict):
        return (machine['hostname'], machine['afe_host'])
    else:
        return (machine, EmptyAFEHost())


def get_afe_host_from_machine(machine):
    """Return the afe_host from the machine dict if possible.

    @returns: AFE host object.
    """
    _, afe_host = get_host_info_from_machine(machine)
    return afe_host


def get_connection_pool_from_machine(machine):
    """Returns the ssh_multiplex.ConnectionPool from machine if possible."""
    if not isinstance(machine, dict):
        return None
    return machine.get('connection_pool')


def get_creds_abspath(creds_file):
    """Returns the abspath of the credentials file.

    If creds_file is already an absolute path, just return it.
    Otherwise, assume it is located in the creds directory
    specified in global_config and return the absolute path.

    @param: creds_path, a path to the credentials.
    @return: An absolute path to the credentials file.
    """
    if not creds_file:
        return None
    if os.path.isabs(creds_file):
        return creds_file
    creds_dir = CONFIG.get_config_value('SERVER', 'creds_dir', default='')
    if not creds_dir or not os.path.exists(creds_dir):
        creds_dir = common.autotest_dir
    return os.path.join(creds_dir, creds_file)


def machine_is_testbed(machine):
    """Checks if the machine is a testbed.

    The signal we use to determine if the machine is a testbed
    is if the host attributes contain more than 1 serial.

    @param machine: is a list of dicts

    @return: True if the machine is a testbed, False otherwise.
    """
    _, afe_host = get_host_info_from_machine(machine)
    return len(afe_host.attributes.get('serials', '').split(',')) > 1


def SetupTsMonGlobalState(*args, **kwargs):
    """Import-safe wrap around chromite.lib.ts_mon_config's setup function.

    @param *args: Args to pass through.
    @param **kwargs: Kwargs to pass through.
    """
    try:
        # TODO(crbug.com/739466) This module import is delayed because it adds
        # 1-2 seconds to the module import time and most users of site_utils
        # don't need it. The correct fix is to break apart site_utils into more
        # meaningful chunks.
        from chromite.lib import ts_mon_config
    except ImportError:
        logging.warn('Unable to import chromite. Monarch is disabled.')
        return TrivialContextManager()

    try:
        context = ts_mon_config.SetupTsMonGlobalState(*args, **kwargs)
        if hasattr(context, '__exit__'):
            return context
    except Exception as e:
        logging.warning('Caught an exception trying to setup ts_mon, '
                        'monitoring is disabled: %s', e, exc_info=True)
    return TrivialContextManager()


@contextlib.contextmanager
def TrivialContextManager(*args, **kwargs):
    """Context manager that does nothing.

    @param *args: Ignored args
    @param **kwargs: Ignored kwargs.
    """
    yield


def wait_for_idle_duts(duts, afe, max_wait=IDLE_DUT_WAIT_TIMEOUT):
    """Wait for the hosts to all go idle.

    @param duts: List of duts to check for idle state.
    @param afe: afe instance.
    @param max_wait: Max wait time in seconds to wait for duts to be idle.

    @returns Boolean True if all hosts are idle or False if any hosts did not
            go idle within max_wait.
    """
    start_time = time.time()
    # We make a shallow copy since we're going to be modifying active_dut_list.
    active_dut_list = duts[:]
    while active_dut_list:
        # Let's rate-limit how often we hit the AFE.
        time.sleep(1)

        # Check if we've waited too long.
        if (time.time() - start_time) > max_wait:
            return False

        idle_duts = []
        # Get the status for the duts and see if they're in the idle state.
        afe_hosts = afe.get_hosts(active_dut_list)
        idle_duts = [afe_host.hostname for afe_host in afe_hosts
                     if afe_host.status in host_states.IDLE_STATES]

        # Take out idle duts so we don't needlessly check them
        # next time around.
        for idle_dut in idle_duts:
            active_dut_list.remove(idle_dut)

        logging.info('still waiting for following duts to go idle: %s',
                     active_dut_list)
    return True


@contextlib.contextmanager
def lock_duts_and_wait(duts, afe, lock_msg='default lock message',
                       max_wait=IDLE_DUT_WAIT_TIMEOUT):
    """Context manager to lock the duts and wait for them to go idle.

    @param duts: List of duts to lock.
    @param afe: afe instance.
    @param lock_msg: message for afe on locking this host.
    @param max_wait: Max wait time in seconds to wait for duts to be idle.

    @returns Boolean lock_success where True if all duts locked successfully or
             False if we timed out waiting too long for hosts to go idle.
    """
    try:
        locked_duts = []
        duts.sort()
        for dut in duts:
            if afe.lock_host(dut, lock_msg, fail_if_locked=True):
                locked_duts.append(dut)
            else:
                logging.info('%s already locked', dut)
        yield wait_for_idle_duts(locked_duts, afe, max_wait)
    finally:
        afe.unlock_hosts(locked_duts)


def board_labels_allowed(boards):
    """Check if the list of board labels can be set to a single host.

    The only case multiple board labels can be set to a single host is for
    testbed, which may have a list of board labels like
    board:angler-1, board:angler-2, board:angler-3, board:marlin-1'

    @param boards: A list of board labels (may include platform label).

    @returns True if the the list of boards can be set to a single host.
    """
    # Filter out any non-board labels
    boards = [b for b in boards if re.match('board:.*', b)]
    if len(boards) <= 1:
        return True
    for board in boards:
        if not re.match('board:[^-]+-\d+', board):
            return False
    return True


def _get_default_size_info(path):
    """Get the default result size information.

    In case directory summary is failed to build, assume the test result is not
    throttled and all result sizes are the size of existing test results.

    @return: A namedtuple of result size informations, including:
            client_result_collected_KB: The total size (in KB) of test results
                    collected from test device. Set to be the total size of the
                    given path.
            original_result_total_KB: The original size (in KB) of test results
                    before being trimmed. Set to be the total size of the given
                    path.
            result_uploaded_KB: The total size (in KB) of test results to be
                    uploaded. Set to be the total size of the given path.
            result_throttled: True if test results collection is throttled.
                    It's set to False in this default behavior.
    """
    total_size = file_utils.get_directory_size_kibibytes(path);
    return result_utils_lib.ResultSizeInfo(
            client_result_collected_KB=total_size,
            original_result_total_KB=total_size,
            result_uploaded_KB=total_size,
            result_throttled=False)


def _report_result_size_metrics(result_size_info):
    """Report result sizes information to metrics.

    @param result_size_info: A ResultSizeInfo namedtuple containing information
            of test result sizes.
    """
    fields = {'result_throttled' : result_size_info.result_throttled}
    metrics.Counter(RESULT_METRICS_PREFIX + 'client_result_collected_KB',
                    description='The total size (in KB) of test results '
                    'collected from test device. Set to be the total size of '
                    'the given path.'
                    ).increment_by(result_size_info.client_result_collected_KB,
                                   fields=fields)
    metrics.Counter(RESULT_METRICS_PREFIX + 'original_result_total_KB',
                    description='The original size (in KB) of test results '
                    'before being trimmed.'
                    ).increment_by(result_size_info.original_result_total_KB,
                                   fields=fields)
    metrics.Counter(RESULT_METRICS_PREFIX + 'result_uploaded_KB',
                    description='The total size (in KB) of test results to be '
                    'uploaded.'
                    ).increment_by(result_size_info.result_uploaded_KB,
                                   fields=fields)


@metrics.SecondsTimerDecorator(
        'chromeos/autotest/result_collection/collect_result_sizes_duration')
def collect_result_sizes(path, log=logging.debug):
    """Collect the result sizes information and build result summary.

    It first tries to merge directory summaries and calculate the result sizes
    including:
    client_result_collected_KB: The volume in KB that's transfered from the test
            device.
    original_result_total_KB: The volume in KB that's the original size of the
            result files before being trimmed.
    result_uploaded_KB: The volume in KB that will be uploaded.
    result_throttled: Indicating if the result files were throttled.

    If directory summary merging failed for any reason, fall back to use the
    total size of the given result directory.

    @param path: Path of the result directory to get size information.
    @param log: The logging method, default to logging.debug
    @return: A ResultSizeInfo namedtuple containing information of test result
             sizes.
    """
    try:
        client_collected_bytes, summary, files = result_utils.merge_summaries(
                path)
        result_size_info = result_utils_lib.get_result_size_info(
                client_collected_bytes, summary)
        html_file = os.path.join(path, result_view.DEFAULT_RESULT_SUMMARY_NAME)
        result_view.build(client_collected_bytes, summary, html_file)

        # Delete all summary files after final view is built.
        for summary_file in files:
            os.remove(summary_file)
    except:
        log('Failed to calculate result sizes based on directory summaries for '
            'directory %s. Fall back to record the total size.\nException: %s' %
            (path, traceback.format_exc()))
        result_size_info = _get_default_size_info(path)

    _report_result_size_metrics(result_size_info)

    return result_size_info