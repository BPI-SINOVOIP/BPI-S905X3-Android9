# Copyright Martin J. Bligh, Google Inc 2008
# Released under the GPL v2

"""
This class allows you to communicate with the frontend to submit jobs etc
It is designed for writing more sophisiticated server-side control files that
can recursively add and manage other jobs.

We turn the JSON dictionaries into real objects that are more idiomatic

For docs, see:
    http://www.chromium.org/chromium-os/testing/afe-rpc-infrastructure
    http://docs.djangoproject.com/en/dev/ref/models/querysets/#queryset-api
"""

#pylint: disable=missing-docstring

import getpass
import os
import re

import common

from autotest_lib.frontend.afe import rpc_client_lib
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import host_states
from autotest_lib.client.common_lib import priorities
from autotest_lib.client.common_lib import utils
from autotest_lib.tko import db

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock

try:
    from autotest_lib.server.site_common import site_utils as server_utils
except:
    from autotest_lib.server import utils as server_utils
form_ntuples_from_machines = server_utils.form_ntuples_from_machines

GLOBAL_CONFIG = global_config.global_config
DEFAULT_SERVER = 'autotest'


def dump_object(header, obj):
    """
    Standard way to print out the frontend objects (eg job, host, acl, label)
    in a human-readable fashion for debugging
    """
    result = header + '\n'
    for key in obj.hash:
        if key == 'afe' or key == 'hash':
            continue
        result += '%20s: %s\n' % (key, obj.hash[key])
    return result


class RpcClient(object):
    """
    Abstract RPC class for communicating with the autotest frontend
    Inherited for both TKO and AFE uses.

    All the constructors go in the afe / tko class.
    Manipulating methods go in the object classes themselves
    """
    def __init__(self, path, user, server, print_log, debug, reply_debug):
        """
        Create a cached instance of a connection to the frontend

            user: username to connect as
            server: frontend server to connect to
            print_log: pring a logging message to stdout on every operation
            debug: print out all RPC traffic
        """
        if not user and utils.is_in_container():
            user = GLOBAL_CONFIG.get_config_value('SSP', 'user', default=None)
        if not user:
            user = getpass.getuser()
        if not server:
            if 'AUTOTEST_WEB' in os.environ:
                server = os.environ['AUTOTEST_WEB']
            else:
                server = GLOBAL_CONFIG.get_config_value('SERVER', 'hostname',
                                                        default=DEFAULT_SERVER)
        self.server = server
        self.user = user
        self.print_log = print_log
        self.debug = debug
        self.reply_debug = reply_debug
        headers = {'AUTHORIZATION': self.user}
        rpc_server = rpc_client_lib.add_protocol(server) + path
        if debug:
            print 'SERVER: %s' % rpc_server
            print 'HEADERS: %s' % headers
        self.proxy = rpc_client_lib.get_proxy(rpc_server, headers=headers)


    def run(self, call, **dargs):
        """
        Make a RPC call to the AFE server
        """
        rpc_call = getattr(self.proxy, call)
        if self.debug:
            print 'DEBUG: %s %s' % (call, dargs)
        try:
            result = utils.strip_unicode(rpc_call(**dargs))
            if self.reply_debug:
                print result
            return result
        except Exception:
            raise


    def log(self, message):
        if self.print_log:
            print message


class TKO(RpcClient):
    def __init__(self, user=None, server=None, print_log=True, debug=False,
                 reply_debug=False):
        super(TKO, self).__init__(path='/new_tko/server/noauth/rpc/',
                                  user=user,
                                  server=server,
                                  print_log=print_log,
                                  debug=debug,
                                  reply_debug=reply_debug)
        self._db = None


    @metrics.SecondsTimerDecorator(
            'chromeos/autotest/tko/get_job_status_duration')
    def get_job_test_statuses_from_db(self, job_id):
        """Get job test statuses from the database.

        Retrieve a set of fields from a job that reflect the status of each test
        run within a job.
        fields retrieved: status, test_name, reason, test_started_time,
                          test_finished_time, afe_job_id, job_owner, hostname.

        @param job_id: The afe job id to look up.
        @returns a TestStatus object of the resulting information.
        """
        if self._db is None:
            self._db = db.db()
        fields = ['status', 'test_name', 'subdir', 'reason',
                  'test_started_time', 'test_finished_time', 'afe_job_id',
                  'job_owner', 'hostname', 'job_tag']
        table = 'tko_test_view_2'
        where = 'job_tag like "%s-%%"' % job_id
        test_status = []
        # Run commit before we query to ensure that we are pulling the latest
        # results.
        self._db.commit()
        for entry in self._db.select(','.join(fields), table, (where, None)):
            status_dict = {}
            for key,value in zip(fields, entry):
                # All callers expect values to be a str object.
                status_dict[key] = str(value)
            # id is used by TestStatus to uniquely identify each Test Status
            # obj.
            status_dict['id'] = [status_dict['reason'], status_dict['hostname'],
                                 status_dict['test_name']]
            test_status.append(status_dict)

        return [TestStatus(self, e) for e in test_status]


    def get_status_counts(self, job, **data):
        entries = self.run('get_status_counts',
                           group_by=['hostname', 'test_name', 'reason'],
                           job_tag__startswith='%s-' % job, **data)
        return [TestStatus(self, e) for e in entries['groups']]


class _StableVersionMap(object):
    """
    A mapping from board names to strings naming software versions.

    The mapping is meant to allow finding a nominally "stable" version
    of software associated with a given board.  The mapping identifies
    specific versions of software that should be installed during
    operations such as repair.

    Conceptually, there are multiple version maps, each handling
    different types of image.  For instance, a single board may have
    both a stable OS image (e.g. for CrOS), and a separate stable
    firmware image.

    Each different type of image requires a certain amount of special
    handling, implemented by a subclass of `StableVersionMap`.  The
    subclasses take care of pre-processing of arguments, delegating
    actual RPC calls to this superclass.

    @property _afe      AFE object through which to make the actual RPC
                        calls.
    @property _android  Value of the `android` parameter to be passed
                        when calling the `get_stable_version` RPC.
    """

    # DEFAULT_BOARD - The stable_version RPC API recognizes this special
    # name as a mapping to use when no specific mapping for a board is
    # present.  This default mapping is only allowed for CrOS image
    # types; other image type subclasses exclude it.
    #
    # TODO(jrbarnette):  This value is copied from
    # site_utils.stable_version_utils, because if we import that
    # module here, it breaks unit tests.  Something about the Django
    # setup...
    DEFAULT_BOARD = 'DEFAULT'


    def __init__(self, afe, android):
        self._afe = afe
        self._android = android


    def get_all_versions(self):
        """
        Get all mappings in the stable versions table.

        Extracts the full content of the `stable_version` table
        in the AFE database, and returns it as a dictionary
        mapping board names to version strings.

        @return A dictionary mapping board names to version strings.
        """
        return self._afe.run('get_all_stable_versions')


    def get_version(self, board):
        """
        Get the mapping of one board in the stable versions table.

        Look up and return the version mapped to the given board in the
        `stable_versions` table in the AFE database.

        @param board  The board to be looked up.

        @return The version mapped for the given board.
        """
        return self._afe.run('get_stable_version',
                             board=board, android=self._android)


    def set_version(self, board, version):
        """
        Change the mapping of one board in the stable versions table.

        Set the mapping in the `stable_versions` table in the AFE
        database for the given board to the given version.

        @param board    The board to be updated.
        @param version  The new version to be assigned to the board.
        """
        self._afe.run('set_stable_version',
                      version=version, board=board)


    def delete_version(self, board):
        """
        Remove the mapping of one board in the stable versions table.

        Remove the mapping in the `stable_versions` table in the AFE
        database for the given board.

        @param board    The board to be updated.
        """
        self._afe.run('delete_stable_version', board=board)


class _OSVersionMap(_StableVersionMap):
    """
    Abstract stable version mapping for full OS images of various types.
    """

    def get_all_versions(self):
        # TODO(jrbarnette):  We exclude non-OS (i.e. firmware) version
        # mappings, but the returned dict doesn't distinguish CrOS
        # boards from Android boards; both will be present, and the
        # subclass can't distinguish them.
        #
        # Ultimately, the right fix is to move knowledge of image type
        # over to the RPC server side.
        #
        versions = super(_OSVersionMap, self).get_all_versions()
        for board in versions.keys():
            if '/' in board:
                del versions[board]
        return versions


class _CrosVersionMap(_OSVersionMap):
    """
    Stable version mapping for Chrome OS release images.

    This class manages a mapping of Chrome OS board names to known-good
    release (or canary) images.  The images selected can be installed on
    DUTs during repair tasks, as a way of getting a DUT into a known
    working state.
    """

    def __init__(self, afe):
        super(_CrosVersionMap, self).__init__(afe, False)

    @staticmethod
    def format_image_name(board, version):
        """
        Return an image name for a given `board` and `version`.

        This formats `board` and `version` into a string identifying an
        image file.  The string represents part of a URL for access to
        the image.

        The returned image name is typically of a form like
        "falco-release/R55-8872.44.0".
        """
        build_pattern = GLOBAL_CONFIG.get_config_value(
                'CROS', 'stable_build_pattern')
        return build_pattern % (board, version)

    def get_image_name(self, board):
        """
        Return the full image name of the stable version for `board`.

        This finds the stable version for `board`, and returns a string
        identifying the associated image as for `format_image_name()`,
        above.

        @return A string identifying the image file for the stable
                image for `board`.
        """
        return self.format_image_name(board, self.get_version(board))


class _AndroidVersionMap(_OSVersionMap):
    """
    Stable version mapping for Android release images.

    This class manages a mapping of Android/Brillo board names to
    known-good images.
    """

    def __init__(self, afe):
        super(_AndroidVersionMap, self).__init__(afe, True)


    def get_all_versions(self):
        versions = super(_AndroidVersionMap, self).get_all_versions()
        del versions[self.DEFAULT_BOARD]
        return versions


class _SuffixHackVersionMap(_StableVersionMap):
    """
    Abstract super class for mappings using a pseudo-board name.

    For non-OS image type mappings, we look them up in the
    `stable_versions` table by constructing a "pseudo-board" from the
    real board name plus a suffix string that identifies the image type.
    So, for instance the name "lulu/firmware" is used to look up the
    FAFT firmware version for lulu boards.
    """

    # _SUFFIX - The suffix used in constructing the "pseudo-board"
    # lookup key.  Each subclass must define this value for itself.
    #
    _SUFFIX = None

    def __init__(self, afe):
        super(_SuffixHackVersionMap, self).__init__(afe, False)


    def get_all_versions(self):
        # Get all the mappings from the AFE, extract just the mappings
        # with our suffix, and replace the pseudo-board name keys with
        # the real board names.
        #
        all_versions = super(
                _SuffixHackVersionMap, self).get_all_versions()
        return {
            board[0 : -len(self._SUFFIX)]: all_versions[board]
                for board in all_versions.keys()
                    if board.endswith(self._SUFFIX)
        }


    def get_version(self, board):
        board += self._SUFFIX
        return super(_SuffixHackVersionMap, self).get_version(board)


    def set_version(self, board, version):
        board += self._SUFFIX
        super(_SuffixHackVersionMap, self).set_version(board, version)


    def delete_version(self, board):
        board += self._SUFFIX
        super(_SuffixHackVersionMap, self).delete_version(board)


class _FAFTVersionMap(_SuffixHackVersionMap):
    """
    Stable version mapping for firmware versions used in FAFT repair.

    When DUTs used for FAFT fail repair, stable firmware may need to be
    flashed directly from original tarballs.  The FAFT firmware version
    mapping finds the appropriate tarball for a given board.
    """

    _SUFFIX = '/firmware'

    def get_version(self, board):
        # If there's no mapping for `board`, the lookup will return the
        # default CrOS version mapping.  To eliminate that case, we
        # require a '/' character in the version, since CrOS versions
        # won't match that.
        #
        # TODO(jrbarnette):  This is, of course, a hack.  Ultimately,
        # the right fix is to move handling to the RPC server side.
        #
        version = super(_FAFTVersionMap, self).get_version(board)
        return version if '/' in version else None


class _FirmwareVersionMap(_SuffixHackVersionMap):
    """
    Stable version mapping for firmware supplied in Chrome OS images.

    A Chrome OS image bundles a version of the firmware that the
    device should update to when the OS version is installed during
    AU.

    Test images suppress the firmware update during AU.  Instead, during
    repair and verify we check installed firmware on a DUT, compare it
    against the stable version mapping for the board, and update when
    the DUT is out-of-date.
    """

    _SUFFIX = '/rwfw'

    def get_version(self, board):
        # If there's no mapping for `board`, the lookup will return the
        # default CrOS version mapping.  To eliminate that case, we
        # require the version start with "Google_", since CrOS versions
        # won't match that.
        #
        # TODO(jrbarnette):  This is, of course, a hack.  Ultimately,
        # the right fix is to move handling to the RPC server side.
        #
        version = super(_FirmwareVersionMap, self).get_version(board)
        return version if version.startswith('Google_') else None


class AFE(RpcClient):

    # Known image types for stable version mapping objects.
    # CROS_IMAGE_TYPE - Mappings for Chrome OS images.
    # FAFT_IMAGE_TYPE - Mappings for Firmware images for FAFT repair.
    # FIRMWARE_IMAGE_TYPE - Mappings for released RW Firmware images.
    # ANDROID_IMAGE_TYPE - Mappings for Android images.
    #
    CROS_IMAGE_TYPE = 'cros'
    FAFT_IMAGE_TYPE = 'faft'
    FIRMWARE_IMAGE_TYPE = 'firmware'
    ANDROID_IMAGE_TYPE = 'android'

    _IMAGE_MAPPING_CLASSES = {
        CROS_IMAGE_TYPE: _CrosVersionMap,
        FAFT_IMAGE_TYPE: _FAFTVersionMap,
        FIRMWARE_IMAGE_TYPE: _FirmwareVersionMap,
        ANDROID_IMAGE_TYPE: _AndroidVersionMap
    }


    def __init__(self, user=None, server=None, print_log=True, debug=False,
                 reply_debug=False, job=None):
        self.job = job
        super(AFE, self).__init__(path='/afe/server/noauth/rpc/',
                                  user=user,
                                  server=server,
                                  print_log=print_log,
                                  debug=debug,
                                  reply_debug=reply_debug)


    def get_stable_version_map(self, image_type):
        """
        Return a stable version mapping for the given image type.

        @return An object mapping board names to version strings for
                software of the given image type.
        """
        return self._IMAGE_MAPPING_CLASSES[image_type](self)


    def host_statuses(self, live=None):
        dead_statuses = ['Repair Failed', 'Repairing']
        statuses = self.run('get_static_data')['host_statuses']
        if live == True:
            return list(set(statuses) - set(dead_statuses))
        if live == False:
            return dead_statuses
        else:
            return statuses


    @staticmethod
    def _dict_for_host_query(hostnames=(), status=None, label=None):
        query_args = {}
        if hostnames:
            query_args['hostname__in'] = hostnames
        if status:
            query_args['status'] = status
        if label:
            query_args['labels__name'] = label
        return query_args


    def get_hosts(self, hostnames=(), status=None, label=None, **dargs):
        query_args = dict(dargs)
        query_args.update(self._dict_for_host_query(hostnames=hostnames,
                                                    status=status,
                                                    label=label))
        hosts = self.run('get_hosts', **query_args)
        return [Host(self, h) for h in hosts]


    def get_hostnames(self, status=None, label=None, **dargs):
        """Like get_hosts() but returns hostnames instead of Host objects."""
        # This implementation can be replaced with a more efficient one
        # that does not query for entire host objects in the future.
        return [host_obj.hostname for host_obj in
                self.get_hosts(status=status, label=label, **dargs)]


    def reverify_hosts(self, hostnames=(), status=None, label=None):
        query_args = dict(locked=False,
                          aclgroup__users__login=self.user)
        query_args.update(self._dict_for_host_query(hostnames=hostnames,
                                                    status=status,
                                                    label=label))
        return self.run('reverify_hosts', **query_args)


    def repair_hosts(self, hostnames=(), status=None, label=None):
        query_args = dict(locked=False,
                          aclgroup__users__login=self.user)
        query_args.update(self._dict_for_host_query(hostnames=hostnames,
                                                    status=status,
                                                    label=label))
        return self.run('repair_hosts', **query_args)


    def create_host(self, hostname, **dargs):
        id = self.run('add_host', hostname=hostname, **dargs)
        return self.get_hosts(id=id)[0]


    def get_host_attribute(self, attr, **dargs):
        host_attrs = self.run('get_host_attribute', attribute=attr, **dargs)
        return [HostAttribute(self, a) for a in host_attrs]


    def set_host_attribute(self, attr, val, **dargs):
        self.run('set_host_attribute', attribute=attr, value=val, **dargs)


    def get_labels(self, **dargs):
        labels = self.run('get_labels', **dargs)
        return [Label(self, l) for l in labels]


    def create_label(self, name, **dargs):
        id = self.run('add_label', name=name, **dargs)
        return self.get_labels(id=id)[0]


    def get_acls(self, **dargs):
        acls = self.run('get_acl_groups', **dargs)
        return [Acl(self, a) for a in acls]


    def create_acl(self, name, **dargs):
        id = self.run('add_acl_group', name=name, **dargs)
        return self.get_acls(id=id)[0]


    def get_users(self, **dargs):
        users = self.run('get_users', **dargs)
        return [User(self, u) for u in users]


    def generate_control_file(self, tests, **dargs):
        ret = self.run('generate_control_file', tests=tests, **dargs)
        return ControlFile(self, ret)


    def get_jobs(self, summary=False, **dargs):
        if summary:
            jobs_data = self.run('get_jobs_summary', **dargs)
        else:
            jobs_data = self.run('get_jobs', **dargs)
        jobs = []
        for j in jobs_data:
            job = Job(self, j)
            # Set up some extra information defaults
            job.testname = re.sub('\s.*', '', job.name) # arbitrary default
            job.platform_results = {}
            job.platform_reasons = {}
            jobs.append(job)
        return jobs


    def get_host_queue_entries(self, **kwargs):
        """Find JobStatus objects matching some constraints.

        @param **kwargs: Arguments to pass to the RPC
        """
        entries = self.run('get_host_queue_entries', **kwargs)
        return self._entries_to_statuses(entries)


    def get_host_queue_entries_by_insert_time(self, **kwargs):
        """Like get_host_queue_entries, but using the insert index table.

        @param **kwargs: Arguments to pass to the RPC
        """
        entries = self.run('get_host_queue_entries_by_insert_time', **kwargs)
        return self._entries_to_statuses(entries)


    def _entries_to_statuses(self, entries):
        """Converts HQEs to JobStatuses

        Sadly, get_host_queue_entries doesn't return platforms, we have
        to get those back from an explicit get_hosts queury, then patch
        the new host objects back into the host list.

        :param entries: A list of HQEs from get_host_queue_entries or
          get_host_queue_entries_by_insert_time.
        """
        job_statuses = [JobStatus(self, e) for e in entries]
        hostnames = [s.host.hostname for s in job_statuses if s.host]
        hosts = {}
        for host in self.get_hosts(hostname__in=hostnames):
            hosts[host.hostname] = host
        for status in job_statuses:
            if status.host:
                status.host = hosts.get(status.host.hostname)
        # filter job statuses that have either host or meta_host
        return [status for status in job_statuses if (status.host or
                                                      status.meta_host)]


    def get_special_tasks(self, **data):
        tasks = self.run('get_special_tasks', **data)
        return [SpecialTask(self, t) for t in tasks]


    def get_host_special_tasks(self, host_id, **data):
        tasks = self.run('get_host_special_tasks',
                         host_id=host_id, **data)
        return [SpecialTask(self, t) for t in tasks]


    def get_host_status_task(self, host_id, end_time):
        task = self.run('get_host_status_task',
                        host_id=host_id, end_time=end_time)
        return SpecialTask(self, task) if task else None


    def get_host_diagnosis_interval(self, host_id, end_time, success):
        return self.run('get_host_diagnosis_interval',
                        host_id=host_id, end_time=end_time,
                        success=success)


    def create_job(self, control_file, name=' ',
                   priority=priorities.Priority.DEFAULT,
                   control_type=control_data.CONTROL_TYPE_NAMES.CLIENT,
                   **dargs):
        id = self.run('create_job', name=name, priority=priority,
                 control_file=control_file, control_type=control_type, **dargs)
        return self.get_jobs(id=id)[0]


    def abort_jobs(self, jobs):
        """Abort a list of jobs.

        Already completed jobs will not be affected.

        @param jobs: List of job ids to abort.
        """
        for job in jobs:
            self.run('abort_host_queue_entries', job_id=job)


    def get_hosts_by_attribute(self, attribute, value):
        """
        Get the list of hosts that share the same host attribute value.

        @param attribute: String of the host attribute to check.
        @param value: String of the value that is shared between hosts.

        @returns List of hostnames that all have the same host attribute and
                 value.
        """
        return self.run('get_hosts_by_attribute',
                        attribute=attribute, value=value)


    def lock_host(self, host, lock_reason, fail_if_locked=False):
        """
        Lock the given host with the given lock reason.

        Locking a host that's already locked using the 'modify_hosts' rpc
        will raise an exception. That's why fail_if_locked exists so the
        caller can determine if the lock succeeded or failed.  This will
        save every caller from wrapping lock_host in a try-except.

        @param host: hostname of host to lock.
        @param lock_reason: Reason for locking host.
        @param fail_if_locked: Return False if host is already locked.

        @returns Boolean, True if lock was successful, False otherwise.
        """
        try:
            self.run('modify_hosts',
                     host_filter_data={'hostname': host},
                     update_data={'locked': True,
                                  'lock_reason': lock_reason})
        except Exception:
            return not fail_if_locked
        return True


    def unlock_hosts(self, locked_hosts):
        """
        Unlock the hosts.

        Unlocking a host that's already unlocked will do nothing so we don't
        need any special try-except clause here.

        @param locked_hosts: List of hostnames of hosts to unlock.
        """
        self.run('modify_hosts',
                 host_filter_data={'hostname__in': locked_hosts},
                 update_data={'locked': False,
                              'lock_reason': ''})


class TestResults(object):
    """
    Container class used to hold the results of the tests for a job
    """
    def __init__(self):
        self.good = []
        self.fail = []
        self.pending = []


    def add(self, result):
        if result.complete_count > result.pass_count:
            self.fail.append(result)
        elif result.incomplete_count > 0:
            self.pending.append(result)
        else:
            self.good.append(result)


class RpcObject(object):
    """
    Generic object used to construct python objects from rpc calls
    """
    def __init__(self, afe, hash):
        self.afe = afe
        self.hash = hash
        self.__dict__.update(hash)


    def __str__(self):
        return dump_object(self.__repr__(), self)


class ControlFile(RpcObject):
    """
    AFE control file object

    Fields: synch_count, dependencies, control_file, is_server
    """
    def __repr__(self):
        return 'CONTROL FILE: %s' % self.control_file


class Label(RpcObject):
    """
    AFE label object

    Fields:
        name, invalid, platform, kernel_config, id, only_if_needed
    """
    def __repr__(self):
        return 'LABEL: %s' % self.name


    def add_hosts(self, hosts):
        # We must use the label's name instead of the id because label ids are
        # not consistent across master-shard.
        return self.afe.run('label_add_hosts', id=self.name, hosts=hosts)


    def remove_hosts(self, hosts):
        # We must use the label's name instead of the id because label ids are
        # not consistent across master-shard.
        return self.afe.run('label_remove_hosts', id=self.name, hosts=hosts)


class Acl(RpcObject):
    """
    AFE acl object

    Fields:
        users, hosts, description, name, id
    """
    def __repr__(self):
        return 'ACL: %s' % self.name


    def add_hosts(self, hosts):
        self.afe.log('Adding hosts %s to ACL %s' % (hosts, self.name))
        return self.afe.run('acl_group_add_hosts', self.id, hosts)


    def remove_hosts(self, hosts):
        self.afe.log('Removing hosts %s from ACL %s' % (hosts, self.name))
        return self.afe.run('acl_group_remove_hosts', self.id, hosts)


    def add_users(self, users):
        self.afe.log('Adding users %s to ACL %s' % (users, self.name))
        return self.afe.run('acl_group_add_users', id=self.name, users=users)


class Job(RpcObject):
    """
    AFE job object

    Fields:
        name, control_file, control_type, synch_count, reboot_before,
        run_verify, priority, email_list, created_on, dependencies,
        timeout, owner, reboot_after, id
    """
    def __repr__(self):
        return 'JOB: %s' % self.id


class JobStatus(RpcObject):
    """
    AFE job_status object

    Fields:
        status, complete, deleted, meta_host, host, active, execution_subdir, id
    """
    def __init__(self, afe, hash):
        super(JobStatus, self).__init__(afe, hash)
        self.job = Job(afe, self.job)
        if getattr(self, 'host'):
            self.host = Host(afe, self.host)


    def __repr__(self):
        if self.host and self.host.hostname:
            hostname = self.host.hostname
        else:
            hostname = 'None'
        return 'JOB STATUS: %s-%s' % (self.job.id, hostname)


class SpecialTask(RpcObject):
    """
    AFE special task object
    """
    def __init__(self, afe, hash):
        super(SpecialTask, self).__init__(afe, hash)
        self.host = Host(afe, self.host)


    def __repr__(self):
        return 'SPECIAL TASK: %s' % self.id


class Host(RpcObject):
    """
    AFE host object

    Fields:
        status, lock_time, locked_by, locked, hostname, invalid,
        labels, platform, protection, dirty, id
    """
    def __repr__(self):
        return 'HOST OBJECT: %s' % self.hostname


    def show(self):
        labels = list(set(self.labels) - set([self.platform]))
        print '%-6s %-7s %-7s %-16s %s' % (self.hostname, self.status,
                                           self.locked, self.platform,
                                           ', '.join(labels))


    def delete(self):
        return self.afe.run('delete_host', id=self.id)


    def modify(self, **dargs):
        return self.afe.run('modify_host', id=self.id, **dargs)


    def get_acls(self):
        return self.afe.get_acls(hosts__hostname=self.hostname)


    def add_acl(self, acl_name):
        self.afe.log('Adding ACL %s to host %s' % (acl_name, self.hostname))
        return self.afe.run('acl_group_add_hosts', id=acl_name,
                            hosts=[self.hostname])


    def remove_acl(self, acl_name):
        self.afe.log('Removing ACL %s from host %s' % (acl_name, self.hostname))
        return self.afe.run('acl_group_remove_hosts', id=acl_name,
                            hosts=[self.hostname])


    def get_labels(self):
        return self.afe.get_labels(host__hostname__in=[self.hostname])


    def add_labels(self, labels):
        self.afe.log('Adding labels %s to host %s' % (labels, self.hostname))
        return self.afe.run('host_add_labels', id=self.id, labels=labels)


    def remove_labels(self, labels):
        self.afe.log('Removing labels %s from host %s' % (labels,self.hostname))
        return self.afe.run('host_remove_labels', id=self.id, labels=labels)


    def is_available(self):
        """Check whether DUT host is available.

        @return: bool
        """
        return not (self.locked
                    or self.status in host_states.UNAVAILABLE_STATES)


class User(RpcObject):
    def __repr__(self):
        return 'USER: %s' % self.login


class TestStatus(RpcObject):
    """
    TKO test status object

    Fields:
        test_idx, hostname, testname, id
        complete_count, incomplete_count, group_count, pass_count
    """
    def __repr__(self):
        return 'TEST STATUS: %s' % self.id


class HostAttribute(RpcObject):
    """
    AFE host attribute object

    Fields:
        id, host, attribute, value
    """
    def __repr__(self):
        return 'HOST ATTRIBUTE %d' % self.id
