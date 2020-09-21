# pylint: disable-msg=C0111

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
The main job wrapper for the server side.

This is the core infrastructure. Derived from the client side job.py

Copyright Martin J. Bligh, Andy Whitcroft 2007
"""

import errno
import fcntl
import getpass
import itertools
import logging
import os
import pickle
import platform
import re
import select
import shutil
import sys
import tempfile
import time
import traceback
import uuid
import warnings

from autotest_lib.client.bin import sysinfo
from autotest_lib.client.common_lib import base_job
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import logging_manager
from autotest_lib.client.common_lib import packages
from autotest_lib.client.common_lib import utils
from autotest_lib.server import profilers
from autotest_lib.server import site_gtest_runner
from autotest_lib.server import subcommand
from autotest_lib.server import test
from autotest_lib.server import utils as server_utils
from autotest_lib.server.cros.dynamic_suite import frontend_wrappers
from autotest_lib.server.hosts import abstract_ssh
from autotest_lib.server.hosts import afe_store
from autotest_lib.server.hosts import file_store
from autotest_lib.server.hosts import shadowing_store
from autotest_lib.server.hosts import factory as host_factory
from autotest_lib.server.hosts import host_info
from autotest_lib.server.hosts import ssh_multiplex
from autotest_lib.tko import db as tko_db
from autotest_lib.tko import models as tko_models
from autotest_lib.tko import status_lib
from autotest_lib.tko import parser_lib
from autotest_lib.tko import utils as tko_utils

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


INCREMENTAL_TKO_PARSING = global_config.global_config.get_config_value(
        'autoserv', 'incremental_tko_parsing', type=bool, default=False)

def _control_segment_path(name):
    """Get the pathname of the named control segment file."""
    server_dir = os.path.dirname(os.path.abspath(__file__))
    return os.path.join(server_dir, "control_segments", name)


CLIENT_CONTROL_FILENAME = 'control'
SERVER_CONTROL_FILENAME = 'control.srv'
MACHINES_FILENAME = '.machines'

CLIENT_WRAPPER_CONTROL_FILE = _control_segment_path('client_wrapper')
CRASHDUMPS_CONTROL_FILE = _control_segment_path('crashdumps')
CRASHINFO_CONTROL_FILE = _control_segment_path('crashinfo')
INSTALL_CONTROL_FILE = _control_segment_path('install')
CLEANUP_CONTROL_FILE = _control_segment_path('cleanup')
VERIFY_CONTROL_FILE = _control_segment_path('verify')
REPAIR_CONTROL_FILE = _control_segment_path('repair')
PROVISION_CONTROL_FILE = _control_segment_path('provision')
VERIFY_JOB_REPO_URL_CONTROL_FILE = _control_segment_path('verify_job_repo_url')
RESET_CONTROL_FILE = _control_segment_path('reset')
GET_NETWORK_STATS_CONTROL_FILE = _control_segment_path('get_network_stats')


def get_machine_dicts(machine_names, workdir, in_lab, host_attributes=None,
                      connection_pool=None):
    """Converts a list of machine names to list of dicts.

    TODO(crbug.com/678430): This function temporarily has a side effect of
    creating files under workdir for backing a FileStore. This side-effect will
    go away once callers of autoserv start passing in the FileStore.

    @param machine_names: A list of machine names.
    @param workdir: A directory where any on-disk files related to the machines
            may be created.
    @param in_lab: A boolean indicating whether we're running in lab.
    @param host_attributes: Optional list of host attributes to add for each
            host.
    @param connection_pool: ssh_multiplex.ConnectionPool instance to share
            master connections across control scripts.
    @returns: A list of dicts. Each dict has the following keys:
            'hostname': Name of the machine originally in machine_names (str).
            'afe_host': A frontend.Host object for the machine, or a stub if
                    in_lab is false.
            'host_info_store': A host_info.CachingHostInfoStore object to obtain
                    host information. A stub if in_lab is False.
            'connection_pool': ssh_multiplex.ConnectionPool instance to share
                    master ssh connection across control scripts.
    """
    machine_dict_list = []
    for machine in machine_names:
        # See autoserv_parser.parse_args. Only one of in_lab or host_attributes
        # can be provided.
        if not in_lab:
            afe_host = server_utils.EmptyAFEHost()
            host_info_store = host_info.InMemoryHostInfoStore()
            if host_attributes is not None:
                afe_host.attributes.update(host_attributes)
                info = host_info.HostInfo(attributes=host_attributes)
                host_info_store.commit(info)
        elif host_attributes:
            raise error.AutoservError(
                    'in_lab and host_attribute are mutually exclusive. '
                    'Obtained in_lab:%s, host_attributes:%s'
                    % (in_lab, host_attributes))
        else:
            afe_host = _create_afe_host(machine)
            host_info_store = _create_host_info_store(machine, workdir)

        machine_dict_list.append({
                'hostname' : machine,
                'afe_host' : afe_host,
                'host_info_store': host_info_store,
                'connection_pool': connection_pool,
        })

    return machine_dict_list


class status_indenter(base_job.status_indenter):
    """Provide a simple integer-backed status indenter."""
    def __init__(self):
        self._indent = 0


    @property
    def indent(self):
        return self._indent


    def increment(self):
        self._indent += 1


    def decrement(self):
        self._indent -= 1


    def get_context(self):
        """Returns a context object for use by job.get_record_context."""
        class context(object):
            def __init__(self, indenter, indent):
                self._indenter = indenter
                self._indent = indent
            def restore(self):
                self._indenter._indent = self._indent
        return context(self, self._indent)


class server_job_record_hook(object):
    """The job.record hook for server job. Used to inject WARN messages from
    the console or vlm whenever new logs are written, and to echo any logs
    to INFO level logging. Implemented as a class so that it can use state to
    block recursive calls, so that the hook can call job.record itself to
    log WARN messages.

    Depends on job._read_warnings and job._logger.
    """
    def __init__(self, job):
        self._job = job
        self._being_called = False


    def __call__(self, entry):
        """A wrapper around the 'real' record hook, the _hook method, which
        prevents recursion. This isn't making any effort to be threadsafe,
        the intent is to outright block infinite recursion via a
        job.record->_hook->job.record->_hook->job.record... chain."""
        if self._being_called:
            return
        self._being_called = True
        try:
            self._hook(self._job, entry)
        finally:
            self._being_called = False


    @staticmethod
    def _hook(job, entry):
        """The core hook, which can safely call job.record."""
        entries = []
        # poll all our warning loggers for new warnings
        for timestamp, msg in job._read_warnings():
            warning_entry = base_job.status_log_entry(
                'WARN', None, None, msg, {}, timestamp=timestamp)
            entries.append(warning_entry)
            job.record_entry(warning_entry)
        # echo rendered versions of all the status logs to info
        entries.append(entry)
        for entry in entries:
            rendered_entry = job._logger.render_entry(entry)
            logging.info(rendered_entry)
            job._parse_status(rendered_entry)


class server_job(base_job.base_job):
    """The server-side concrete implementation of base_job.

    Optional properties provided by this implementation:
        serverdir

        num_tests_run
        num_tests_failed

        warning_manager
        warning_loggers
    """

    _STATUS_VERSION = 1

    # TODO crbug.com/285395 eliminate ssh_verbosity_flag
    def __init__(self, control, args, resultdir, label, user, machines,
                 client=False, parse_job='',
                 ssh_user=host_factory.DEFAULT_SSH_USER,
                 ssh_port=host_factory.DEFAULT_SSH_PORT,
                 ssh_pass=host_factory.DEFAULT_SSH_PASS,
                 ssh_verbosity_flag=host_factory.DEFAULT_SSH_VERBOSITY,
                 ssh_options=host_factory.DEFAULT_SSH_OPTIONS,
                 test_retry=0, group_name='',
                 tag='', disable_sysinfo=False,
                 control_filename=SERVER_CONTROL_FILENAME,
                 parent_job_id=None, host_attributes=None, in_lab=False):
        """
        Create a server side job object.

        @param control: The pathname of the control file.
        @param args: Passed to the control file.
        @param resultdir: Where to throw the results.
        @param label: Description of the job.
        @param user: Username for the job (email address).
        @param client: True if this is a client-side control file.
        @param parse_job: string, if supplied it is the job execution tag that
                the results will be passed through to the TKO parser with.
        @param ssh_user: The SSH username.  [root]
        @param ssh_port: The SSH port number.  [22]
        @param ssh_pass: The SSH passphrase, if needed.
        @param ssh_verbosity_flag: The SSH verbosity flag, '-v', '-vv',
                '-vvv', or an empty string if not needed.
        @param ssh_options: A string giving additional options that will be
                            included in ssh commands.
        @param test_retry: The number of times to retry a test if the test did
                not complete successfully.
        @param group_name: If supplied, this will be written out as
                host_group_name in the keyvals file for the parser.
        @param tag: The job execution tag from the scheduler.  [optional]
        @param disable_sysinfo: Whether we should disable the sysinfo step of
                tests for a modest shortening of test time.  [optional]
        @param control_filename: The filename where the server control file
                should be written in the results directory.
        @param parent_job_id: Job ID of the parent job. Default to None if the
                job does not have a parent job.
        @param host_attributes: Dict of host attributes passed into autoserv
                                via the command line. If specified here, these
                                attributes will apply to all machines.
        @param in_lab: Boolean that indicates if this is running in the lab
                       environment.
        """
        super(server_job, self).__init__(resultdir=resultdir,
                                         test_retry=test_retry)
        self.test_retry = test_retry
        self.control = control
        self._uncollected_log_file = os.path.join(self.resultdir,
                                                  'uncollected_logs')
        debugdir = os.path.join(self.resultdir, 'debug')
        if not os.path.exists(debugdir):
            os.mkdir(debugdir)

        if user:
            self.user = user
        else:
            self.user = getpass.getuser()

        self.args = args
        self.label = label
        self.machines = machines
        self._client = client
        self.warning_loggers = set()
        self.warning_manager = warning_manager()
        self._ssh_user = ssh_user
        self._ssh_port = ssh_port
        self._ssh_pass = ssh_pass
        self._ssh_verbosity_flag = ssh_verbosity_flag
        self._ssh_options = ssh_options
        self.tag = tag
        self.hosts = set()
        self.drop_caches = False
        self.drop_caches_between_iterations = False
        self._control_filename = control_filename
        self._disable_sysinfo = disable_sysinfo

        self.logging = logging_manager.get_logging_manager(
                manage_stdout_and_stderr=True, redirect_fds=True)
        subcommand.logging_manager_object = self.logging

        self.sysinfo = sysinfo.sysinfo(self.resultdir)
        self.profilers = profilers.profilers(self)

        job_data = {'label' : label, 'user' : user,
                    'hostname' : ','.join(machines),
                    'drone' : platform.node(),
                    'status_version' : str(self._STATUS_VERSION),
                    'job_started' : str(int(time.time()))}
        # Save parent job id to keyvals, so parser can retrieve the info and
        # write to tko_jobs record.
        if parent_job_id:
            job_data['parent_job_id'] = parent_job_id
        if group_name:
            job_data['host_group_name'] = group_name

        # only write these keyvals out on the first job in a resultdir
        if 'job_started' not in utils.read_keyval(self.resultdir):
            job_data.update(self._get_job_data())
            utils.write_keyval(self.resultdir, job_data)

        self._parse_job = parse_job
        self._using_parser = (INCREMENTAL_TKO_PARSING and self._parse_job
                              and len(machines) <= 1)
        self.pkgmgr = packages.PackageManager(
            self.autodir, run_function_dargs={'timeout':600})
        self.num_tests_run = 0
        self.num_tests_failed = 0

        self._register_subcommand_hooks()

        # set up the status logger
        self._indenter = status_indenter()
        self._logger = base_job.status_logger(
            self, self._indenter, 'status.log', 'status.log',
            record_hook=server_job_record_hook(self))

        # Initialize a flag to indicate DUT failure during the test, e.g.,
        # unexpected reboot.
        self.failed_with_device_error = False

        self._connection_pool = ssh_multiplex.ConnectionPool()

        self.parent_job_id = parent_job_id
        self.in_lab = in_lab
        self.machine_dict_list = get_machine_dicts(
                self.machines, self.resultdir, self.in_lab, host_attributes,
                self._connection_pool)

        # TODO(jrbarnette) The harness attribute is only relevant to
        # client jobs, but it's required to be present, or we will fail
        # server job unit tests.  Yes, really.
        #
        # TODO(jrbarnette) The utility of the 'harness' attribute even
        # to client jobs is suspect.  Probably, we should remove it.
        self.harness = None

        if control:
            parsed_control = control_data.parse_control(
                    control, raise_warnings=False)
            self.fast = parsed_control.fast
            self.max_result_size_KB = parsed_control.max_result_size_KB
        else:
            self.fast = False
            # Set the maximum result size to be the default specified in
            # global config, if the job has no control file associated.
            self.max_result_size_KB = control_data.DEFAULT_MAX_RESULT_SIZE_KB


    @classmethod
    def _find_base_directories(cls):
        """
        Determine locations of autodir, clientdir and serverdir. Assumes
        that this file is located within serverdir and uses __file__ along
        with relative paths to resolve the location.
        """
        serverdir = os.path.abspath(os.path.dirname(__file__))
        autodir = os.path.normpath(os.path.join(serverdir, '..'))
        clientdir = os.path.join(autodir, 'client')
        return autodir, clientdir, serverdir


    def _find_resultdir(self, resultdir, *args, **dargs):
        """
        Determine the location of resultdir. For server jobs we expect one to
        always be explicitly passed in to __init__, so just return that.
        """
        if resultdir:
            return os.path.normpath(resultdir)
        else:
            return None


    def _get_status_logger(self):
        """Return a reference to the status logger."""
        return self._logger


    @staticmethod
    def _load_control_file(path):
        f = open(path)
        try:
            control_file = f.read()
        finally:
            f.close()
        return re.sub('\r', '', control_file)


    def _register_subcommand_hooks(self):
        """
        Register some hooks into the subcommand modules that allow us
        to properly clean up self.hosts created in forked subprocesses.
        """
        def on_fork(cmd):
            self._existing_hosts_on_fork = set(self.hosts)
        def on_join(cmd):
            new_hosts = self.hosts - self._existing_hosts_on_fork
            for host in new_hosts:
                host.close()
        subcommand.subcommand.register_fork_hook(on_fork)
        subcommand.subcommand.register_join_hook(on_join)


    def init_parser(self):
        """
        Start the continuous parsing of self.resultdir. This sets up
        the database connection and inserts the basic job object into
        the database if necessary.
        """
        if self.fast and not self._using_parser:
            self.parser = parser_lib.parser(self._STATUS_VERSION)
            self.job_model = self.parser.make_job(self.resultdir)
            self.parser.start(self.job_model)
            return

        if not self._using_parser:
            return

        # redirect parser debugging to .parse.log
        parse_log = os.path.join(self.resultdir, '.parse.log')
        parse_log = open(parse_log, 'w', 0)
        tko_utils.redirect_parser_debugging(parse_log)
        # create a job model object and set up the db
        self.results_db = tko_db.db(autocommit=True)
        self.parser = parser_lib.parser(self._STATUS_VERSION)
        self.job_model = self.parser.make_job(self.resultdir)
        self.parser.start(self.job_model)
        # check if a job already exists in the db and insert it if
        # it does not
        job_idx = self.results_db.find_job(self._parse_job)
        if job_idx is None:
            self.results_db.insert_job(self._parse_job, self.job_model,
                                       self.parent_job_id)
        else:
            machine_idx = self.results_db.lookup_machine(self.job_model.machine)
            self.job_model.index = job_idx
            self.job_model.machine_idx = machine_idx


    def cleanup_parser(self):
        """
        This should be called after the server job is finished
        to carry out any remaining cleanup (e.g. flushing any
        remaining test results to the results db)
        """
        if self.fast and not self._using_parser:
            final_tests = self.parser.end()
            for test in final_tests:
                if status_lib.is_worse_than_or_equal_to(test.status, 'FAIL'):
                    self.num_tests_failed += 1
            return

        if not self._using_parser:
            return

        final_tests = self.parser.end()
        for test in final_tests:
            self.__insert_test(test)
        self._using_parser = False

    # TODO crbug.com/285395 add a kwargs parameter.
    def _make_namespace(self):
        """Create a namespace dictionary to be passed along to control file.

        Creates a namespace argument populated with standard values:
        machines, job, ssh_user, ssh_port, ssh_pass, ssh_verbosity_flag,
        and ssh_options.
        """
        namespace = {'machines' : self.machine_dict_list,
                     'job' : self,
                     'ssh_user' : self._ssh_user,
                     'ssh_port' : self._ssh_port,
                     'ssh_pass' : self._ssh_pass,
                     'ssh_verbosity_flag' : self._ssh_verbosity_flag,
                     'ssh_options' : self._ssh_options}
        return namespace


    def cleanup(self, labels):
        """Cleanup machines.

        @param labels: Comma separated job labels, will be used to
                       determine special task actions.
        """
        if not self.machines:
            raise error.AutoservError('No machines specified to cleanup')
        if self.resultdir:
            os.chdir(self.resultdir)

        namespace = self._make_namespace()
        namespace.update({'job_labels': labels, 'args': ''})
        self._execute_code(CLEANUP_CONTROL_FILE, namespace, protect=False)


    def verify(self, labels):
        """Verify machines are all ssh-able.

        @param labels: Comma separated job labels, will be used to
                       determine special task actions.
        """
        if not self.machines:
            raise error.AutoservError('No machines specified to verify')
        if self.resultdir:
            os.chdir(self.resultdir)

        namespace = self._make_namespace()
        namespace.update({'job_labels': labels, 'args': ''})
        self._execute_code(VERIFY_CONTROL_FILE, namespace, protect=False)


    def reset(self, labels):
        """Reset machines by first cleanup then verify each machine.

        @param labels: Comma separated job labels, will be used to
                       determine special task actions.
        """
        if not self.machines:
            raise error.AutoservError('No machines specified to reset.')
        if self.resultdir:
            os.chdir(self.resultdir)

        namespace = self._make_namespace()
        namespace.update({'job_labels': labels, 'args': ''})
        self._execute_code(RESET_CONTROL_FILE, namespace, protect=False)


    def repair(self, labels):
        """Repair machines.

        @param labels: Comma separated job labels, will be used to
                       determine special task actions.
        """
        if not self.machines:
            raise error.AutoservError('No machines specified to repair')
        if self.resultdir:
            os.chdir(self.resultdir)

        namespace = self._make_namespace()
        namespace.update({'job_labels': labels, 'args': ''})
        self._execute_code(REPAIR_CONTROL_FILE, namespace, protect=False)


    def provision(self, labels):
        """
        Provision all hosts to match |labels|.

        @param labels: A comma seperated string of labels to provision the
                       host to.

        """
        control = self._load_control_file(PROVISION_CONTROL_FILE)
        self.run(control=control, job_labels=labels)


    def precheck(self):
        """
        perform any additional checks in derived classes.
        """
        pass


    def enable_external_logging(self):
        """
        Start or restart external logging mechanism.
        """
        pass


    def disable_external_logging(self):
        """
        Pause or stop external logging mechanism.
        """
        pass


    def use_external_logging(self):
        """
        Return True if external logging should be used.
        """
        return False


    def _make_parallel_wrapper(self, function, machines, log):
        """Wrap function as appropriate for calling by parallel_simple."""
        # machines could be a list of dictionaries, e.g.,
        # [{'host_attributes': {}, 'hostname': '100.96.51.226'}]
        # The dictionary is generated in server_job.__init__, refer to
        # variable machine_dict_list, then passed in with namespace, see method
        # server_job._make_namespace.
        # To compare the machinese to self.machines, which is a list of machine
        # hostname, we need to convert machines back to a list of hostnames.
        # Note that the order of hostnames doesn't matter, as is_forking will be
        # True if there are more than one machine.
        if (machines and isinstance(machines, list)
            and isinstance(machines[0], dict)):
            machines = [m['hostname'] for m in machines]
        is_forking = not (len(machines) == 1 and self.machines == machines)
        if self._parse_job and is_forking and log:
            def wrapper(machine):
                hostname = server_utils.get_hostname_from_machine(machine)
                self._parse_job += "/" + hostname
                self._using_parser = INCREMENTAL_TKO_PARSING
                self.machines = [machine]
                self.push_execution_context(hostname)
                os.chdir(self.resultdir)
                utils.write_keyval(self.resultdir, {"hostname": hostname})
                self.init_parser()
                result = function(machine)
                self.cleanup_parser()
                return result
        elif len(machines) > 1 and log:
            def wrapper(machine):
                hostname = server_utils.get_hostname_from_machine(machine)
                self.push_execution_context(hostname)
                os.chdir(self.resultdir)
                machine_data = {'hostname' : hostname,
                                'status_version' : str(self._STATUS_VERSION)}
                utils.write_keyval(self.resultdir, machine_data)
                result = function(machine)
                return result
        else:
            wrapper = function
        return wrapper


    def parallel_simple(self, function, machines, log=True, timeout=None,
                        return_results=False):
        """
        Run 'function' using parallel_simple, with an extra wrapper to handle
        the necessary setup for continuous parsing, if possible. If continuous
        parsing is already properly initialized then this should just work.

        @param function: A callable to run in parallel given each machine.
        @param machines: A list of machine names to be passed one per subcommand
                invocation of function.
        @param log: If True, output will be written to output in a subdirectory
                named after each machine.
        @param timeout: Seconds after which the function call should timeout.
        @param return_results: If True instead of an AutoServError being raised
                on any error a list of the results|exceptions from the function
                called on each arg is returned.  [default: False]

        @raises error.AutotestError: If any of the functions failed.
        """
        wrapper = self._make_parallel_wrapper(function, machines, log)
        return subcommand.parallel_simple(
                wrapper, machines,
                subdir_name_constructor=server_utils.get_hostname_from_machine,
                log=log, timeout=timeout, return_results=return_results)


    def parallel_on_machines(self, function, machines, timeout=None):
        """
        @param function: Called in parallel with one machine as its argument.
        @param machines: A list of machines to call function(machine) on.
        @param timeout: Seconds after which the function call should timeout.

        @returns A list of machines on which function(machine) returned
                without raising an exception.
        """
        results = self.parallel_simple(function, machines, timeout=timeout,
                                       return_results=True)
        success_machines = []
        for result, machine in itertools.izip(results, machines):
            if not isinstance(result, Exception):
                success_machines.append(machine)
        return success_machines


    def record_skipped_test(self, skipped_test, message=None):
        """Insert a failure record into status.log for this test."""
        msg = message
        if msg is None:
            msg = 'No valid machines found for test %s.' % skipped_test
        logging.info(msg)
        self.record('START', None, skipped_test.test_name)
        self.record('INFO', None, skipped_test.test_name, msg)
        self.record('END TEST_NA', None, skipped_test.test_name, msg)


    def _has_failed_tests(self):
        """Parse status log for failed tests.

        This checks the current working directory and is intended only for use
        by the run() method.

        @return boolean
        """
        path = os.getcwd()

        # TODO(ayatane): Copied from tko/parse.py.  Needs extensive refactor to
        # make code reuse plausible.
        job_keyval = tko_models.job.read_keyval(path)
        status_version = job_keyval.get("status_version", 0)

        # parse out the job
        parser = parser_lib.parser(status_version)
        job = parser.make_job(path)
        status_log = os.path.join(path, "status.log")
        if not os.path.exists(status_log):
            status_log = os.path.join(path, "status")
        if not os.path.exists(status_log):
            logging.warning("! Unable to parse job, no status file")
            return True

        # parse the status logs
        status_lines = open(status_log).readlines()
        parser.start(job)
        tests = parser.end(status_lines)

        # parser.end can return the same object multiple times, so filter out
        # dups
        job.tests = []
        already_added = set()
        for test in tests:
            if test not in already_added:
                already_added.add(test)
                job.tests.append(test)

        failed = False
        for test in job.tests:
            # The current job is still running and shouldn't count as failed.
            # The parser will fail to parse the exit status of the job since it
            # hasn't exited yet (this running right now is the job).
            failed = failed or (test.status != 'GOOD'
                                and not _is_current_server_job(test))
        return failed


    def _collect_crashes(self, namespace, collect_crashinfo):
        """Collect crashes.

        @param namespace: namespace dict.
        @param collect_crashinfo: whether to collect crashinfo in addition to
                dumps
        """
        if collect_crashinfo:
            # includes crashdumps
            crash_control_file = CRASHINFO_CONTROL_FILE
        else:
            crash_control_file = CRASHDUMPS_CONTROL_FILE
        self._execute_code(crash_control_file, namespace)


    _USE_TEMP_DIR = object()
    def run(self, install_before=False, install_after=False,
            collect_crashdumps=True, namespace={}, control=None,
            control_file_dir=None, verify_job_repo_url=False,
            only_collect_crashinfo=False, skip_crash_collection=False,
            job_labels='', use_packaging=True):
        # for a normal job, make sure the uncollected logs file exists
        # for a crashinfo-only run it should already exist, bail out otherwise
        created_uncollected_logs = False
        logging.info("I am PID %s", os.getpid())
        if self.resultdir and not os.path.exists(self._uncollected_log_file):
            if only_collect_crashinfo:
                # if this is a crashinfo-only run, and there were no existing
                # uncollected logs, just bail out early
                logging.info("No existing uncollected logs, "
                             "skipping crashinfo collection")
                return
            else:
                log_file = open(self._uncollected_log_file, "w")
                pickle.dump([], log_file)
                log_file.close()
                created_uncollected_logs = True

        # use a copy so changes don't affect the original dictionary
        namespace = namespace.copy()
        machines = self.machines
        if control is None:
            if self.control is None:
                control = ''
            else:
                control = self._load_control_file(self.control)
        if control_file_dir is None:
            control_file_dir = self.resultdir

        self.aborted = False
        namespace.update(self._make_namespace())
        namespace.update({
                'args': self.args,
                'job_labels': job_labels,
                'gtest_runner': site_gtest_runner.gtest_runner(),
        })
        test_start_time = int(time.time())

        if self.resultdir:
            os.chdir(self.resultdir)
            # touch status.log so that the parser knows a job is running here
            open(self.get_status_log_path(), 'a').close()
            self.enable_external_logging()

        collect_crashinfo = True
        temp_control_file_dir = None
        try:
            try:
                if not self.fast:
                    with metrics.SecondsTimer(
                            'chromeos/autotest/job/get_network_stats',
                            fields = {'stage': 'start'}):
                        namespace['network_stats_label'] = 'at-start'
                        self._execute_code(GET_NETWORK_STATS_CONTROL_FILE,
                                           namespace)

                if install_before and machines:
                    self._execute_code(INSTALL_CONTROL_FILE, namespace)

                if only_collect_crashinfo:
                    return

                # If the verify_job_repo_url option is set but we're unable
                # to actually verify that the job_repo_url contains the autotest
                # package, this job will fail.
                if verify_job_repo_url:
                    self._execute_code(VERIFY_JOB_REPO_URL_CONTROL_FILE,
                                       namespace)
                else:
                    logging.warning('Not checking if job_repo_url contains '
                                    'autotest packages on %s', machines)

                # determine the dir to write the control files to
                cfd_specified = (control_file_dir
                                 and control_file_dir is not self._USE_TEMP_DIR)
                if cfd_specified:
                    temp_control_file_dir = None
                else:
                    temp_control_file_dir = tempfile.mkdtemp(
                        suffix='temp_control_file_dir')
                    control_file_dir = temp_control_file_dir
                server_control_file = os.path.join(control_file_dir,
                                                   self._control_filename)
                client_control_file = os.path.join(control_file_dir,
                                                   CLIENT_CONTROL_FILENAME)
                if self._client:
                    namespace['control'] = control
                    utils.open_write_close(client_control_file, control)
                    shutil.copyfile(CLIENT_WRAPPER_CONTROL_FILE,
                                    server_control_file)
                else:
                    utils.open_write_close(server_control_file, control)

                logging.info("Processing control file")
                namespace['use_packaging'] = use_packaging
                self._execute_code(server_control_file, namespace)
                logging.info("Finished processing control file")

                # If no device error occured, no need to collect crashinfo.
                collect_crashinfo = self.failed_with_device_error
            except Exception as e:
                try:
                    # Add num_tests_failed if any extra exceptions are raised
                    # outside _execute_code().
                    self.num_tests_failed += 1
                    logging.exception(
                            'Exception escaped control file, job aborting:')
                    reason = re.sub(base_job.status_log_entry.BAD_CHAR_REGEX,
                                    ' ', str(e))
                    self.record('INFO', None, None, str(e),
                                {'job_abort_reason': reason})
                except:
                    pass # don't let logging exceptions here interfere
                raise
        finally:
            if temp_control_file_dir:
                # Clean up temp directory used for copies of the control files
                try:
                    shutil.rmtree(temp_control_file_dir)
                except Exception as e:
                    logging.warning('Could not remove temp directory %s: %s',
                                 temp_control_file_dir, e)

            if machines and (collect_crashdumps or collect_crashinfo):
                if skip_crash_collection or self.fast:
                    logging.info('Skipping crash dump/info collection '
                                 'as requested.')
                else:
                    with metrics.SecondsTimer(
                            'chromeos/autotest/job/collect_crashinfo'):
                        namespace['test_start_time'] = test_start_time
                        # Remove crash files for passing tests.
                        # TODO(ayatane): Tests that create crash files should be
                        # reported.
                        namespace['has_failed_tests'] = self._has_failed_tests()
                        self._collect_crashes(namespace, collect_crashinfo)
            self.disable_external_logging()
            if self._uncollected_log_file and created_uncollected_logs:
                os.remove(self._uncollected_log_file)
            if install_after and machines:
                self._execute_code(INSTALL_CONTROL_FILE, namespace)

            if not self.fast:
                with metrics.SecondsTimer(
                        'chromeos/autotest/job/get_network_stats',
                        fields = {'stage': 'end'}):
                    namespace['network_stats_label'] = 'at-end'
                    self._execute_code(GET_NETWORK_STATS_CONTROL_FILE,
                                       namespace)


    def run_test(self, url, *args, **dargs):
        """
        Summon a test object and run it.

        tag
                tag to add to testname
        url
                url of the test to run
        """
        if self._disable_sysinfo:
            dargs['disable_sysinfo'] = True

        group, testname = self.pkgmgr.get_package_name(url, 'test')
        testname, subdir, tag = self._build_tagged_test_name(testname, dargs)
        outputdir = self._make_test_outputdir(subdir)

        def group_func():
            try:
                test.runtest(self, url, tag, args, dargs)
            except error.TestBaseException as e:
                self.record(e.exit_status, subdir, testname, str(e))
                raise
            except Exception as e:
                info = str(e) + "\n" + traceback.format_exc()
                self.record('FAIL', subdir, testname, info)
                raise
            else:
                self.record('GOOD', subdir, testname, 'completed successfully')

        try:
            result = self._run_group(testname, subdir, group_func)
        except error.TestBaseException as e:
            return False
        else:
            return True


    def _run_group(self, name, subdir, function, *args, **dargs):
        """Underlying method for running something inside of a group."""
        result, exc_info = None, None
        try:
            self.record('START', subdir, name)
            result = function(*args, **dargs)
        except error.TestBaseException as e:
            self.record("END %s" % e.exit_status, subdir, name)
            raise
        except Exception as e:
            err_msg = str(e) + '\n'
            err_msg += traceback.format_exc()
            self.record('END ABORT', subdir, name, err_msg)
            raise error.JobError(name + ' failed\n' + traceback.format_exc())
        else:
            self.record('END GOOD', subdir, name)

        return result


    def run_group(self, function, *args, **dargs):
        """\
        @param function: subroutine to run
        @returns: (result, exc_info). When the call succeeds, result contains
                the return value of |function| and exc_info is None. If
                |function| raises an exception, exc_info contains the tuple
                returned by sys.exc_info(), and result is None.
        """

        name = function.__name__
        # Allow the tag for the group to be specified.
        tag = dargs.pop('tag', None)
        if tag:
            name = tag

        try:
            result = self._run_group(name, None, function, *args, **dargs)[0]
        except error.TestBaseException:
            return None, sys.exc_info()
        return result, None


    def run_op(self, op, op_func, get_kernel_func):
        """\
        A specialization of run_group meant specifically for handling
        management operation. Includes support for capturing the kernel version
        after the operation.

        Args:
           op: name of the operation.
           op_func: a function that carries out the operation (reboot, suspend)
           get_kernel_func: a function that returns a string
                            representing the kernel version.
        """
        try:
            self.record('START', None, op)
            op_func()
        except Exception as e:
            err_msg = str(e) + '\n' + traceback.format_exc()
            self.record('END FAIL', None, op, err_msg)
            raise
        else:
            kernel = get_kernel_func()
            self.record('END GOOD', None, op,
                        optional_fields={"kernel": kernel})


    def run_control(self, path):
        """Execute a control file found at path (relative to the autotest
        path). Intended for executing a control file within a control file,
        not for running the top-level job control file."""
        path = os.path.join(self.autodir, path)
        control_file = self._load_control_file(path)
        self.run(control=control_file, control_file_dir=self._USE_TEMP_DIR)


    def add_sysinfo_command(self, command, logfile=None, on_every_test=False):
        self._add_sysinfo_loggable(sysinfo.command(command, logf=logfile),
                                   on_every_test)


    def add_sysinfo_logfile(self, file, on_every_test=False):
        self._add_sysinfo_loggable(sysinfo.logfile(file), on_every_test)


    def _add_sysinfo_loggable(self, loggable, on_every_test):
        if on_every_test:
            self.sysinfo.test_loggables.add(loggable)
        else:
            self.sysinfo.boot_loggables.add(loggable)


    def _read_warnings(self):
        """Poll all the warning loggers and extract any new warnings that have
        been logged. If the warnings belong to a category that is currently
        disabled, this method will discard them and they will no longer be
        retrievable.

        Returns a list of (timestamp, message) tuples, where timestamp is an
        integer epoch timestamp."""
        warnings = []
        while True:
            # pull in a line of output from every logger that has
            # output ready to be read
            loggers, _, _ = select.select(self.warning_loggers, [], [], 0)
            closed_loggers = set()
            for logger in loggers:
                line = logger.readline()
                # record any broken pipes (aka line == empty)
                if len(line) == 0:
                    closed_loggers.add(logger)
                    continue
                # parse out the warning
                timestamp, msgtype, msg = line.split('\t', 2)
                timestamp = int(timestamp)
                # if the warning is valid, add it to the results
                if self.warning_manager.is_valid(timestamp, msgtype):
                    warnings.append((timestamp, msg.strip()))

            # stop listening to loggers that are closed
            self.warning_loggers -= closed_loggers

            # stop if none of the loggers have any output left
            if not loggers:
                break

        # sort into timestamp order
        warnings.sort()
        return warnings


    def _unique_subdirectory(self, base_subdirectory_name):
        """Compute a unique results subdirectory based on the given name.

        Appends base_subdirectory_name with a number as necessary to find a
        directory name that doesn't already exist.
        """
        subdirectory = base_subdirectory_name
        counter = 1
        while os.path.exists(os.path.join(self.resultdir, subdirectory)):
            subdirectory = base_subdirectory_name + '.' + str(counter)
            counter += 1
        return subdirectory


    def get_record_context(self):
        """Returns an object representing the current job.record context.

        The object returned is an opaque object with a 0-arg restore method
        which can be called to restore the job.record context (i.e. indentation)
        to the current level. The intention is that it should be used when
        something external which generate job.record calls (e.g. an autotest
        client) can fail catastrophically and the server job record state
        needs to be reset to its original "known good" state.

        @return: A context object with a 0-arg restore() method."""
        return self._indenter.get_context()


    def record_summary(self, status_code, test_name, reason='', attributes=None,
                       distinguishing_attributes=(), child_test_ids=None):
        """Record a summary test result.

        @param status_code: status code string, see
                common_lib.log.is_valid_status()
        @param test_name: name of the test
        @param reason: (optional) string providing detailed reason for test
                outcome
        @param attributes: (optional) dict of string keyvals to associate with
                this result
        @param distinguishing_attributes: (optional) list of attribute names
                that should be used to distinguish identically-named test
                results.  These attributes should be present in the attributes
                parameter.  This is used to generate user-friendly subdirectory
                names.
        @param child_test_ids: (optional) list of test indices for test results
                used in generating this result.
        """
        subdirectory_name_parts = [test_name]
        for attribute in distinguishing_attributes:
            assert attributes
            assert attribute in attributes, '%s not in %s' % (attribute,
                                                              attributes)
            subdirectory_name_parts.append(attributes[attribute])
        base_subdirectory_name = '.'.join(subdirectory_name_parts)

        subdirectory = self._unique_subdirectory(base_subdirectory_name)
        subdirectory_path = os.path.join(self.resultdir, subdirectory)
        os.mkdir(subdirectory_path)

        self.record(status_code, subdirectory, test_name,
                    status=reason, optional_fields={'is_summary': True})

        if attributes:
            utils.write_keyval(subdirectory_path, attributes)

        if child_test_ids:
            ids_string = ','.join(str(test_id) for test_id in child_test_ids)
            summary_data = {'child_test_ids': ids_string}
            utils.write_keyval(os.path.join(subdirectory_path, 'summary_data'),
                               summary_data)


    def disable_warnings(self, warning_type):
        self.warning_manager.disable_warnings(warning_type)
        self.record("INFO", None, None,
                    "disabling %s warnings" % warning_type,
                    {"warnings.disable": warning_type})


    def enable_warnings(self, warning_type):
        self.warning_manager.enable_warnings(warning_type)
        self.record("INFO", None, None,
                    "enabling %s warnings" % warning_type,
                    {"warnings.enable": warning_type})


    def get_status_log_path(self, subdir=None):
        """Return the path to the job status log.

        @param subdir - Optional paramter indicating that you want the path
            to a subdirectory status log.

        @returns The path where the status log should be.
        """
        if self.resultdir:
            if subdir:
                return os.path.join(self.resultdir, subdir, "status.log")
            else:
                return os.path.join(self.resultdir, "status.log")
        else:
            return None


    def _update_uncollected_logs_list(self, update_func):
        """Updates the uncollected logs list in a multi-process safe manner.

        @param update_func - a function that updates the list of uncollected
            logs. Should take one parameter, the list to be updated.
        """
        # Skip log collection if file _uncollected_log_file does not exist.
        if not (self._uncollected_log_file and
                os.path.exists(self._uncollected_log_file)):
            return
        if self._uncollected_log_file:
            log_file = open(self._uncollected_log_file, "r+")
            fcntl.flock(log_file, fcntl.LOCK_EX)
        try:
            uncollected_logs = pickle.load(log_file)
            update_func(uncollected_logs)
            log_file.seek(0)
            log_file.truncate()
            pickle.dump(uncollected_logs, log_file)
            log_file.flush()
        finally:
            fcntl.flock(log_file, fcntl.LOCK_UN)
            log_file.close()


    def add_client_log(self, hostname, remote_path, local_path):
        """Adds a new set of client logs to the list of uncollected logs,
        to allow for future log recovery.

        @param host - the hostname of the machine holding the logs
        @param remote_path - the directory on the remote machine holding logs
        @param local_path - the local directory to copy the logs into
        """
        def update_func(logs_list):
            logs_list.append((hostname, remote_path, local_path))
        self._update_uncollected_logs_list(update_func)


    def remove_client_log(self, hostname, remote_path, local_path):
        """Removes a set of client logs from the list of uncollected logs,
        to allow for future log recovery.

        @param host - the hostname of the machine holding the logs
        @param remote_path - the directory on the remote machine holding logs
        @param local_path - the local directory to copy the logs into
        """
        def update_func(logs_list):
            logs_list.remove((hostname, remote_path, local_path))
        self._update_uncollected_logs_list(update_func)


    def get_client_logs(self):
        """Retrieves the list of uncollected logs, if it exists.

        @returns A list of (host, remote_path, local_path) tuples. Returns
                 an empty list if no uncollected logs file exists.
        """
        log_exists = (self._uncollected_log_file and
                      os.path.exists(self._uncollected_log_file))
        if log_exists:
            return pickle.load(open(self._uncollected_log_file))
        else:
            return []


    def _fill_server_control_namespace(self, namespace, protect=True):
        """
        Prepare a namespace to be used when executing server control files.

        This sets up the control file API by importing modules and making them
        available under the appropriate names within namespace.

        For use by _execute_code().

        Args:
          namespace: The namespace dictionary to fill in.
          protect: Boolean.  If True (the default) any operation that would
              clobber an existing entry in namespace will cause an error.
        Raises:
          error.AutoservError: When a name would be clobbered by import.
        """
        def _import_names(module_name, names=()):
            """
            Import a module and assign named attributes into namespace.

            Args:
                module_name: The string module name.
                names: A limiting list of names to import from module_name.  If
                    empty (the default), all names are imported from the module
                    similar to a "from foo.bar import *" statement.
            Raises:
                error.AutoservError: When a name being imported would clobber
                    a name already in namespace.
            """
            module = __import__(module_name, {}, {}, names)

            # No names supplied?  Import * from the lowest level module.
            # (Ugh, why do I have to implement this part myself?)
            if not names:
                for submodule_name in module_name.split('.')[1:]:
                    module = getattr(module, submodule_name)
                if hasattr(module, '__all__'):
                    names = getattr(module, '__all__')
                else:
                    names = dir(module)

            # Install each name into namespace, checking to make sure it
            # doesn't override anything that already exists.
            for name in names:
                # Check for conflicts to help prevent future problems.
                if name in namespace and protect:
                    if namespace[name] is not getattr(module, name):
                        raise error.AutoservError('importing name '
                                '%s from %s %r would override %r' %
                                (name, module_name, getattr(module, name),
                                 namespace[name]))
                    else:
                        # Encourage cleanliness and the use of __all__ for a
                        # more concrete API with less surprises on '*' imports.
                        warnings.warn('%s (%r) being imported from %s for use '
                                      'in server control files is not the '
                                      'first occurrence of that import.' %
                                      (name, namespace[name], module_name))

                namespace[name] = getattr(module, name)


        # This is the equivalent of prepending a bunch of import statements to
        # the front of the control script.
        namespace.update(os=os, sys=sys, logging=logging)
        _import_names('autotest_lib.server',
                ('hosts', 'autotest', 'standalone_profiler'))
        _import_names('autotest_lib.server.subcommand',
                      ('parallel', 'parallel_simple', 'subcommand'))
        _import_names('autotest_lib.server.utils',
                      ('run', 'get_tmp_dir', 'sh_escape', 'parse_machine'))
        _import_names('autotest_lib.client.common_lib.error')
        _import_names('autotest_lib.client.common_lib.barrier', ('barrier',))

        # Inject ourself as the job object into other classes within the API.
        # (Yuck, this injection is a gross thing be part of a public API. -gps)
        #
        # XXX Autotest does not appear to use .job.  Who does?
        namespace['autotest'].Autotest.job = self
        # server.hosts.base_classes.Host uses .job.
        namespace['hosts'].Host.job = self
        namespace['hosts'].TestBed.job = self
        namespace['hosts'].factory.ssh_user = self._ssh_user
        namespace['hosts'].factory.ssh_port = self._ssh_port
        namespace['hosts'].factory.ssh_pass = self._ssh_pass
        namespace['hosts'].factory.ssh_verbosity_flag = (
                self._ssh_verbosity_flag)
        namespace['hosts'].factory.ssh_options = self._ssh_options


    def _execute_code(self, code_file, namespace, protect=True):
        """
        Execute code using a copy of namespace as a server control script.

        Unless protect_namespace is explicitly set to False, the dict will not
        be modified.

        Args:
          code_file: The filename of the control file to execute.
          namespace: A dict containing names to make available during execution.
          protect: Boolean.  If True (the default) a copy of the namespace dict
              is used during execution to prevent the code from modifying its
              contents outside of this function.  If False the raw dict is
              passed in and modifications will be allowed.
        """
        if protect:
            namespace = namespace.copy()
        self._fill_server_control_namespace(namespace, protect=protect)
        # TODO: Simplify and get rid of the special cases for only 1 machine.
        if len(self.machines) > 1:
            machines_text = '\n'.join(self.machines) + '\n'
            # Only rewrite the file if it does not match our machine list.
            try:
                machines_f = open(MACHINES_FILENAME, 'r')
                existing_machines_text = machines_f.read()
                machines_f.close()
            except EnvironmentError:
                existing_machines_text = None
            if machines_text != existing_machines_text:
                utils.open_write_close(MACHINES_FILENAME, machines_text)
        execfile(code_file, namespace, namespace)


    def _parse_status(self, new_line):
        if self.fast and not self._using_parser:
            logging.info('Parsing lines in fast mode')
            new_tests = self.parser.process_lines([new_line])
            for test in new_tests:
                if status_lib.is_worse_than_or_equal_to(test.status, 'FAIL'):
                    self.num_tests_failed += 1

        if not self._using_parser:
            return

        new_tests = self.parser.process_lines([new_line])
        for test in new_tests:
            self.__insert_test(test)


    def __insert_test(self, test):
        """
        An internal method to insert a new test result into the
        database. This method will not raise an exception, even if an
        error occurs during the insert, to avoid failing a test
        simply because of unexpected database issues."""
        self.num_tests_run += 1
        if status_lib.is_worse_than_or_equal_to(test.status, 'FAIL'):
            self.num_tests_failed += 1
        try:
            self.results_db.insert_test(self.job_model, test)
        except Exception:
            msg = ("WARNING: An unexpected error occured while "
                   "inserting test results into the database. "
                   "Ignoring error.\n" + traceback.format_exc())
            print >> sys.stderr, msg


    def preprocess_client_state(self):
        """
        Produce a state file for initializing the state of a client job.

        Creates a new client state file with all the current server state, as
        well as some pre-set client state.

        @returns The path of the file the state was written into.
        """
        # initialize the sysinfo state
        self._state.set('client', 'sysinfo', self.sysinfo.serialize())

        # dump the state out to a tempfile
        fd, file_path = tempfile.mkstemp(dir=self.tmpdir)
        os.close(fd)

        # write_to_file doesn't need locking, we exclusively own file_path
        self._state.write_to_file(file_path)
        return file_path


    def postprocess_client_state(self, state_path):
        """
        Update the state of this job with the state from a client job.

        Updates the state of the server side of a job with the final state
        of a client job that was run. Updates the non-client-specific state,
        pulls in some specific bits from the client-specific state, and then
        discards the rest. Removes the state file afterwards

        @param state_file A path to the state file from the client.
        """
        # update the on-disk state
        try:
            self._state.read_from_file(state_path)
            os.remove(state_path)
        except OSError, e:
            # ignore file-not-found errors
            if e.errno != errno.ENOENT:
                raise
            else:
                logging.debug('Client state file %s not found', state_path)

        # update the sysinfo state
        if self._state.has('client', 'sysinfo'):
            self.sysinfo.deserialize(self._state.get('client', 'sysinfo'))

        # drop all the client-specific state
        self._state.discard_namespace('client')


    def clear_all_known_hosts(self):
        """Clears known hosts files for all AbstractSSHHosts."""
        for host in self.hosts:
            if isinstance(host, abstract_ssh.AbstractSSHHost):
                host.clear_known_hosts()


    def close(self):
        """Closes this job's operation."""

        # Use shallow copy, because host.close() internally discards itself.
        for host in list(self.hosts):
            host.close()
        assert not self.hosts
        self._connection_pool.shutdown()


    def _get_job_data(self):
        """Add custom data to the job keyval info.

        When multiple machines are used in a job, change the hostname to
        the platform of the first machine instead of machine1,machine2,...  This
        makes the job reports easier to read and keeps the tko_machines table from
        growing too large.

        Returns:
            keyval dictionary with new hostname value, or empty dictionary.
        """
        job_data = {}
        # Only modify hostname on multimachine jobs. Assume all host have the same
        # platform.
        if len(self.machines) > 1:
            # Search through machines for first machine with a platform.
            for host in self.machines:
                keyval_path = os.path.join(self.resultdir, 'host_keyvals', host)
                keyvals = utils.read_keyval(keyval_path)
                host_plat = keyvals.get('platform', None)
                if not host_plat:
                    continue
                job_data['hostname'] = host_plat
                break
        return job_data


class warning_manager(object):
    """Class for controlling warning logs. Manages the enabling and disabling
    of warnings."""
    def __init__(self):
        # a map of warning types to a list of disabled time intervals
        self.disabled_warnings = {}


    def is_valid(self, timestamp, warning_type):
        """Indicates if a warning (based on the time it occured and its type)
        is a valid warning. A warning is considered "invalid" if this type of
        warning was marked as "disabled" at the time the warning occured."""
        disabled_intervals = self.disabled_warnings.get(warning_type, [])
        for start, end in disabled_intervals:
            if timestamp >= start and (end is None or timestamp < end):
                return False
        return True


    def disable_warnings(self, warning_type, current_time_func=time.time):
        """As of now, disables all further warnings of this type."""
        intervals = self.disabled_warnings.setdefault(warning_type, [])
        if not intervals or intervals[-1][1] is not None:
            intervals.append((int(current_time_func()), None))


    def enable_warnings(self, warning_type, current_time_func=time.time):
        """As of now, enables all further warnings of this type."""
        intervals = self.disabled_warnings.get(warning_type, [])
        if intervals and intervals[-1][1] is None:
            intervals[-1] = (intervals[-1][0], int(current_time_func()))


def _is_current_server_job(test):
    """Return True if parsed test is the currently running job.

    @param test: test instance from tko parser.
    """
    return test.testname == 'SERVER_JOB'


def _create_afe_host(hostname):
    """Create an afe_host object backed by the AFE.

    @param hostname: Name of the host for which we want the Host object.
    @returns: An object of type frontend.AFE
    """
    afe = frontend_wrappers.RetryingAFE(timeout_min=5, delay_sec=10)
    hosts = afe.get_hosts(hostname=hostname)
    if not hosts:
        raise error.AutoservError('No hosts named %s found' % hostname)

    return hosts[0]


def _create_host_info_store(hostname, workdir):
    """Create a CachingHostInfo store backed by the AFE.

    @param hostname: Name of the host for which we want the store.
    @param workdir: A directory where any on-disk files related to the machines
            may be created.
    @returns: An object of type shadowing_store.ShadowingStore
    """
    primary_store = afe_store.AfeStore(hostname)
    try:
        primary_store.get(force_refresh=True)
    except host_info.StoreError:
        raise error.AutoservError('Could not obtain HostInfo for hostname %s' %
                                  hostname)
    backing_file_path = _file_store_unique_file_path(workdir)
    logging.info('Shadowing AFE store with a FileStore at %s',
                 backing_file_path)
    shadow_store = file_store.FileStore(backing_file_path)
    return shadowing_store.ShadowingStore(primary_store, shadow_store)


def _file_store_unique_file_path(workdir):
    """Returns a unique filepath for the on-disk FileStore.

    Also makes sure that the workdir exists.

    @param: Top level working directory.
    """
    store_dir = os.path.join(workdir, 'host_info_store')
    _make_dirs_if_needed(store_dir)
    file_path = os.path.join(store_dir, 'store_%s' % uuid.uuid4())
    return file_path


def _make_dirs_if_needed(path):
    """os.makedirs, but ignores failure because the leaf directory exists"""
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise
