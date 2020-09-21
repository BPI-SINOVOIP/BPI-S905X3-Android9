import heapq
import os
import logging

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.scheduler import drone_task_queue
from autotest_lib.scheduler import drone_utility
from autotest_lib.scheduler import drones
from autotest_lib.scheduler import scheduler_config
from autotest_lib.scheduler import thread_lib

try:
    from chromite.lib import metrics
except ImportError:
    metrics = utils.metrics_mock


# results on drones will be placed under the drone_installation_directory in a
# directory with this name
_DRONE_RESULTS_DIR_SUFFIX = 'results'

WORKING_DIRECTORY = object() # see execute_command()


AUTOSERV_PID_FILE = '.autoserv_execute'
CRASHINFO_PID_FILE = '.collect_crashinfo_execute'
PARSER_PID_FILE = '.parser_execute'
ARCHIVER_PID_FILE = '.archiver_execute'

ALL_PIDFILE_NAMES = (AUTOSERV_PID_FILE, CRASHINFO_PID_FILE, PARSER_PID_FILE,
                     ARCHIVER_PID_FILE)

_THREADED_DRONE_MANAGER = global_config.global_config.get_config_value(
        scheduler_config.CONFIG_SECTION, 'threaded_drone_manager',
        type=bool, default=True)

HOSTS_JOB_SUBDIR = 'hosts/'
PARSE_LOG = '.parse.log'
ENABLE_ARCHIVING =  global_config.global_config.get_config_value(
        scheduler_config.CONFIG_SECTION, 'enable_archiving', type=bool)


class DroneManagerError(Exception):
    pass


class CustomEquals(object):
    def _id(self):
        raise NotImplementedError


    def __eq__(self, other):
        if not isinstance(other, type(self)):
            return NotImplemented
        return self._id() == other._id()


    def __ne__(self, other):
        return not self == other


    def __hash__(self):
        return hash(self._id())


class Process(CustomEquals):
    def __init__(self, hostname, pid, ppid=None):
        self.hostname = hostname
        self.pid = pid
        self.ppid = ppid

    def _id(self):
        return (self.hostname, self.pid)


    def __str__(self):
        return '%s/%s' % (self.hostname, self.pid)


    def __repr__(self):
        return super(Process, self).__repr__() + '<%s>' % self


class PidfileId(CustomEquals):
    def __init__(self, path):
        self.path = path


    def _id(self):
        return self.path


    def __str__(self):
        return str(self.path)


class _PidfileInfo(object):
    age = 0
    num_processes = None


class PidfileContents(object):
    process = None
    exit_status = None
    num_tests_failed = None

    def is_invalid(self):
        return False


    def is_running(self):
        return self.process and not self.exit_status


class InvalidPidfile(object):
    process = None
    exit_status = None
    num_tests_failed = None


    def __init__(self, error):
        self.error = error


    def is_invalid(self):
        return True


    def is_running(self):
        return False


    def __str__(self):
        return self.error


class _DroneHeapWrapper(object):
    """Wrapper to compare drones based on used_capacity().

    These objects can be used to keep a heap of drones by capacity.
    """
    def __init__(self, drone):
        self.drone = drone


    def __cmp__(self, other):
        assert isinstance(other, _DroneHeapWrapper)
        return cmp(self.drone.used_capacity(), other.drone.used_capacity())


class DroneManager(object):
    """
    This class acts as an interface from the scheduler to drones, whether it be
    only a single "drone" for localhost or multiple remote drones.

    All paths going into and out of this class are relative to the full results
    directory, except for those returns by absolute_path().
    """


    # Minimum time to wait before next email
    # about a drone hitting process limit is sent.
    NOTIFY_INTERVAL = 60 * 60 * 24 # one day
    _STATS_KEY = 'drone_manager'



    def __init__(self):
        # absolute path of base results dir
        self._results_dir = None
        # holds Process objects
        self._process_set = set()
        # holds the list of all processes running on all drones
        self._all_processes = {}
        # maps PidfileId to PidfileContents
        self._pidfiles = {}
        # same as _pidfiles
        self._pidfiles_second_read = {}
        # maps PidfileId to _PidfileInfo
        self._registered_pidfile_info = {}
        # used to generate unique temporary paths
        self._temporary_path_counter = 0
        # maps hostname to Drone object
        self._drones = {}
        self._results_drone = None
        # maps results dir to dict mapping file path to contents
        self._attached_files = {}
        # heapq of _DroneHeapWrappers
        self._drone_queue = []
        # A threaded task queue used to refresh drones asynchronously.
        if _THREADED_DRONE_MANAGER:
            self._refresh_task_queue = thread_lib.ThreadedTaskQueue(
                    name='%s.refresh_queue' % self._STATS_KEY)
        else:
            self._refresh_task_queue = drone_task_queue.DroneTaskQueue()


    def initialize(self, base_results_dir, drone_hostnames,
                   results_repository_hostname):
        self._results_dir = base_results_dir

        for hostname in drone_hostnames:
            self._add_drone(hostname)

        if not self._drones:
            # all drones failed to initialize
            raise DroneManagerError('No valid drones found')

        self.refresh_drone_configs()

        logging.info('Using results repository on %s',
                     results_repository_hostname)
        self._results_drone = drones.get_drone(results_repository_hostname)
        results_installation_dir = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION,
                'results_host_installation_directory', default=None)
        if results_installation_dir:
            self._results_drone.set_autotest_install_dir(
                    results_installation_dir)
        # don't initialize() the results drone - we don't want to clear out any
        # directories and we don't need to kill any processes


    def reinitialize_drones(self):
        for drone in self.get_drones():
            with metrics.SecondsTimer(
                    'chromeos/autotest/drone_manager/'
                    'reinitialize_drones_duration',
                    fields={'drone': drone.hostname}):
                drone.call('initialize', self._results_dir)


    def shutdown(self):
        for drone in self.get_drones():
            drone.shutdown()


    def _get_max_pidfile_refreshes(self):
        """
        Normally refresh() is called on every monitor_db.Dispatcher.tick().

        @returns: The number of refresh() calls before we forget a pidfile.
        """
        pidfile_timeout = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'max_pidfile_refreshes',
                type=int, default=2000)
        return pidfile_timeout


    def _add_drone(self, hostname):
        """
        Add drone.

        Catches AutoservRunError if the drone fails initialization and does not
        add it to the list of usable drones.

        @param hostname: Hostname of the drone we are trying to add.
        """
        logging.info('Adding drone %s' % hostname)
        drone = drones.get_drone(hostname)
        if drone:
            try:
                drone.call('initialize', self.absolute_path(''))
            except error.AutoservRunError as e:
                logging.error('Failed to initialize drone %s with error: %s',
                              hostname, e)
                return
            self._drones[drone.hostname] = drone


    def _remove_drone(self, hostname):
        self._drones.pop(hostname, None)


    def refresh_drone_configs(self):
        """
        Reread global config options for all drones.
        """
        # Import server_manager_utils is delayed rather than at the beginning of
        # this module. The reason is that test_that imports drone_manager when
        # importing autoserv_utils. The import is done before test_that setup
        # django (test_that only setup django in setup_local_afe, since it's
        # not needed when test_that runs the test in a lab duts through :lab:
        # option. Therefore, if server_manager_utils is imported at the
        # beginning of this module, test_that will fail since django is not
        # setup yet.
        from autotest_lib.site_utils import server_manager_utils
        config = global_config.global_config
        section = scheduler_config.CONFIG_SECTION
        config.parse_config_file()
        for hostname, drone in self._drones.iteritems():
            if server_manager_utils.use_server_db():
                server = server_manager_utils.get_servers(hostname=hostname)[0]
                attributes = dict([(a.attribute, a.value)
                                   for a in server.attributes.all()])
                drone.enabled = (
                        int(attributes.get('disabled', 0)) == 0)
                drone.max_processes = int(
                        attributes.get(
                            'max_processes',
                            scheduler_config.config.max_processes_per_drone))
                allowed_users = attributes.get('users', None)
            else:
                disabled = config.get_config_value(
                        section, '%s_disabled' % hostname, default='')
                drone.enabled = not bool(disabled)
                drone.max_processes = config.get_config_value(
                        section, '%s_max_processes' % hostname, type=int,
                        default=scheduler_config.config.max_processes_per_drone)

                allowed_users = config.get_config_value(
                        section, '%s_users' % hostname, default=None)
            if allowed_users:
                drone.allowed_users = set(allowed_users.split())
            else:
                drone.allowed_users = None
            logging.info('Drone %s.max_processes: %s', hostname,
                         drone.max_processes)
            logging.info('Drone %s.enabled: %s', hostname, drone.enabled)
            logging.info('Drone %s.allowed_users: %s', hostname,
                         drone.allowed_users)
            logging.info('Drone %s.support_ssp: %s', hostname,
                         drone.support_ssp)

        self._reorder_drone_queue() # max_processes may have changed
        # Clear notification record about reaching max_processes limit.
        self._notify_record = {}


    def get_drones(self):
        return self._drones.itervalues()


    def cleanup_orphaned_containers(self):
        """Queue cleanup_orphaned_containers call at each drone.
        """
        for drone in self._drones.values():
            logging.info('Queue cleanup_orphaned_containers at %s',
                         drone.hostname)
            drone.queue_call('cleanup_orphaned_containers')


    def _get_drone_for_process(self, process):
        return self._drones[process.hostname]


    def _get_drone_for_pidfile_id(self, pidfile_id):
        pidfile_contents = self.get_pidfile_contents(pidfile_id)
        if pidfile_contents.process is None:
          raise DroneManagerError('Fail to get a drone due to empty pidfile')
        return self._get_drone_for_process(pidfile_contents.process)


    def get_drone_for_pidfile_id(self, pidfile_id):
        """Public API for luciferlib.

        @param pidfile_id: PidfileId instance.
        """
        return self._get_drone_for_pidfile_id(pidfile_id)


    def _drop_old_pidfiles(self):
        # use items() since the dict is modified in unregister_pidfile()
        for pidfile_id, info in self._registered_pidfile_info.items():
            if info.age > self._get_max_pidfile_refreshes():
                logging.warning('dropping leaked pidfile %s', pidfile_id)
                self.unregister_pidfile(pidfile_id)
            else:
                info.age += 1


    def _reset(self):
        self._process_set = set()
        self._all_processes = {}
        self._pidfiles = {}
        self._pidfiles_second_read = {}
        self._drone_queue = []


    def _parse_pidfile(self, drone, raw_contents):
        """Parse raw pidfile contents.

        @param drone: The drone on which this pidfile was found.
        @param raw_contents: The raw contents of a pidfile, eg:
            "pid\nexit_staus\nnum_tests_failed\n".
        """
        contents = PidfileContents()
        if not raw_contents:
            return contents
        lines = raw_contents.splitlines()
        if len(lines) > 3:
            return InvalidPidfile('Corrupt pid file (%d lines):\n%s' %
                                  (len(lines), lines))
        try:
            pid = int(lines[0])
            contents.process = Process(drone.hostname, pid)
            # if len(lines) == 2, assume we caught Autoserv between writing
            # exit_status and num_failed_tests, so just ignore it and wait for
            # the next cycle
            if len(lines) == 3:
                contents.exit_status = int(lines[1])
                contents.num_tests_failed = int(lines[2])
        except ValueError, exc:
            return InvalidPidfile('Corrupt pid file: ' + str(exc.args))

        return contents


    def _process_pidfiles(self, drone, pidfiles, store_in_dict):
        for pidfile_path, contents in pidfiles.iteritems():
            pidfile_id = PidfileId(pidfile_path)
            contents = self._parse_pidfile(drone, contents)
            store_in_dict[pidfile_id] = contents


    def _add_process(self, drone, process_info):
        process = Process(drone.hostname, int(process_info['pid']),
                          int(process_info['ppid']))
        self._process_set.add(process)


    def _add_autoserv_process(self, drone, process_info):
        assert process_info['comm'] == 'autoserv'
        # only root autoserv processes have pgid == pid
        if process_info['pgid'] != process_info['pid']:
            return
        self._add_process(drone, process_info)


    def _enqueue_drone(self, drone):
        heapq.heappush(self._drone_queue, _DroneHeapWrapper(drone))


    def _reorder_drone_queue(self):
        heapq.heapify(self._drone_queue)


    def _compute_active_processes(self, drone):
        drone.active_processes = 0
        for pidfile_id, contents in self._pidfiles.iteritems():
            is_running = contents.exit_status is None
            on_this_drone = (contents.process
                             and contents.process.hostname == drone.hostname)
            if is_running and on_this_drone:
                info = self._registered_pidfile_info[pidfile_id]
                if info.num_processes is not None:
                    drone.active_processes += info.num_processes

        metrics.Gauge('chromeos/autotest/drone/active_processes').set(
                drone.active_processes,
                fields={'drone_hostname': drone.hostname})


    def _check_drone_process_limit(self, drone):
        """
        Notify if the number of processes on |drone| is approaching limit.

        @param drone: A Drone object.
        """
        try:
            percent = float(drone.active_processes) / drone.max_processes
        except ZeroDivisionError:
            percent = 100
        metrics.Float('chromeos/autotest/drone/active_process_percentage'
                      ).set(percent, fields={'drone_hostname': drone.hostname})

    def trigger_refresh(self):
        """Triggers a drone manager refresh.

        @raises DroneManagerError: If a drone has un-executed calls.
            Since they will get clobbered when we queue refresh calls.
        """
        self._reset()
        self._drop_old_pidfiles()
        pidfile_paths = [pidfile_id.path
                         for pidfile_id in self._registered_pidfile_info]
        drones = list(self.get_drones())
        for drone in drones:
            calls = drone.get_calls()
            if calls:
                raise DroneManagerError('Drone %s has un-executed calls: %s '
                                        'which might get corrupted through '
                                        'this invocation' %
                                        (drone, [str(call) for call in calls]))
            drone.queue_call('refresh', pidfile_paths)
        logging.info("Invoking drone refresh.")
        with metrics.SecondsTimer(
                'chromeos/autotest/drone_manager/trigger_refresh_duration'):
            self._refresh_task_queue.execute(drones, wait=False)


    def sync_refresh(self):
        """Complete the drone refresh started by trigger_refresh.

        Waits for all drone threads then refreshes internal datastructures
        with drone process information.
        """

        # This gives us a dictionary like what follows:
        # {drone: [{'pidfiles': (raw contents of pidfile paths),
        #           'autoserv_processes': (autoserv process info from ps),
        #           'all_processes': (all process info from ps),
        #           'parse_processes': (parse process infor from ps),
        #           'pidfile_second_read': (pidfile contents, again),}]
        #   drone2: ...}
        # The values of each drone are only a list because this adheres to the
        # drone utility interface (each call is executed and its results are
        # places in a list, but since we never couple the refresh calls with
        # any other call, this list will always contain a single dict).
        with metrics.SecondsTimer(
                'chromeos/autotest/drone_manager/sync_refresh_duration'):
            all_results = self._refresh_task_queue.get_results()
        logging.info("Drones refreshed.")

        # The loop below goes through and parses pidfile contents. Pidfiles
        # are used to track autoserv execution, and will always contain < 3
        # lines of the following: pid, exit code, number of tests. Each pidfile
        # is identified by a PidfileId object, which contains a unique pidfile
        # path (unique because it contains the job id) making it hashable.
        # All pidfiles are stored in the drone managers _pidfiles dict as:
        #   {pidfile_id: pidfile_contents(Process(drone, pid),
        #                                 exit_code, num_tests_failed)}
        # In handle agents, each agent knows its pidfile_id, and uses this
        # to retrieve the refreshed contents of its pidfile via the
        # PidfileRunMonitor (through its tick) before making decisions. If
        # the agent notices that its process has exited, it unregisters the
        # pidfile from the drone_managers._registered_pidfile_info dict
        # through its epilog.
        for drone, results_list in all_results.iteritems():
            results = results_list[0]
            drone_hostname = drone.hostname.replace('.', '_')

            for process_info in results['all_processes']:
                if process_info['comm'] == 'autoserv':
                    self._add_autoserv_process(drone, process_info)
                drone_pid = drone.hostname, int(process_info['pid'])
                self._all_processes[drone_pid] = process_info

            for process_info in results['parse_processes']:
                self._add_process(drone, process_info)

            self._process_pidfiles(drone, results['pidfiles'], self._pidfiles)
            self._process_pidfiles(drone, results['pidfiles_second_read'],
                                   self._pidfiles_second_read)

            self._compute_active_processes(drone)
            if drone.enabled:
                self._enqueue_drone(drone)
                self._check_drone_process_limit(drone)


    def refresh(self):
        """Refresh all drones."""
        with metrics.SecondsTimer(
                'chromeos/autotest/drone_manager/refresh_duration'):
            self.trigger_refresh()
            self.sync_refresh()


    @metrics.SecondsTimerDecorator(
        'chromeos/autotest/drone_manager/execute_actions_duration')
    def execute_actions(self):
        """
        Called at the end of a scheduler cycle to execute all queued actions
        on drones.
        """
        # Invoke calls queued on all drones since the last call to execute
        # and wait for them to return.
        if _THREADED_DRONE_MANAGER:
            thread_lib.ThreadedTaskQueue(
                    name='%s.execute_queue' % self._STATS_KEY).execute(
                            self._drones.values())
        else:
            drone_task_queue.DroneTaskQueue().execute(self._drones.values())

        try:
            self._results_drone.execute_queued_calls()
        except error.AutoservError:
            m = 'chromeos/autotest/errors/results_repository_failed'
            metrics.Counter(m).increment(
                fields={'drone_hostname': self._results_drone.hostname})
            self._results_drone.clear_call_queue()


    def get_orphaned_autoserv_processes(self):
        """
        Returns a set of Process objects for orphaned processes only.
        """
        return set(process for process in self._process_set
                   if process.ppid == 1)


    def kill_process(self, process):
        """
        Kill the given process.
        """
        logging.info('killing %s', process)
        drone = self._get_drone_for_process(process)
        drone.queue_kill_process(process)


    def _ensure_directory_exists(self, path):
        if not os.path.exists(path):
            os.makedirs(path)


    def total_running_processes(self):
        return sum(drone.active_processes for drone in self.get_drones())


    def max_runnable_processes(self, username, drone_hostnames_allowed):
        """
        Return the maximum number of processes that can be run (in a single
        execution) given the current load on drones.
        @param username: login of user to run a process.  may be None.
        @param drone_hostnames_allowed: list of drones that can be used. May be
                                        None
        """
        usable_drone_wrappers = [wrapper for wrapper in self._drone_queue
                                 if wrapper.drone.usable_by(username) and
                                 (drone_hostnames_allowed is None or
                                          wrapper.drone.hostname in
                                                  drone_hostnames_allowed)]
        if not usable_drone_wrappers:
            # all drones disabled or inaccessible
            return 0
        runnable_processes = [
                wrapper.drone.max_processes - wrapper.drone.active_processes
                for wrapper in usable_drone_wrappers]
        return max([0] + runnable_processes)


    def _least_loaded_drone(self, drones):
        return min(drones, key=lambda d: d.used_capacity())


    def pick_drone_to_use(self, num_processes=1, prefer_ssp=False):
        """Return a drone to use.

        Various options can be passed to optimize drone selection.

        num_processes is the number of processes the drone is intended
        to run.

        prefer_ssp indicates whether drones supporting server-side
        packaging should be preferred.  The returned drone is not
        guaranteed to support it.

        This public API is exposed for luciferlib to wrap.

        Returns a drone instance (see drones.py).
        """
        return self._choose_drone_for_execution(
                num_processes=num_processes,
                username=None,  # Always allow all drones
                drone_hostnames_allowed=None,  # Always allow all drones
                require_ssp=prefer_ssp,
        )


    def _choose_drone_for_execution(self, num_processes, username,
                                    drone_hostnames_allowed,
                                    require_ssp=False):
        """Choose a drone to execute command.

        @param num_processes: Number of processes needed for execution.
        @param username: Name of the user to execute the command.
        @param drone_hostnames_allowed: A list of names of drone allowed.
        @param require_ssp: Require server-side packaging to execute the,
                            command, default to False.

        @return: A drone object to be used for execution.
        """
        # cycle through drones is order of increasing used capacity until
        # we find one that can handle these processes
        checked_drones = []
        usable_drones = []
        # Drones do not support server-side packaging, used as backup if no
        # drone is found to run command requires server-side packaging.
        no_ssp_drones = []
        drone_to_use = None
        while self._drone_queue:
            drone = heapq.heappop(self._drone_queue).drone
            checked_drones.append(drone)
            logging.info('Checking drone %s', drone.hostname)
            if not drone.usable_by(username):
                continue

            drone_allowed = (drone_hostnames_allowed is None
                             or drone.hostname in drone_hostnames_allowed)
            if not drone_allowed:
                logging.debug('Drone %s not allowed: ', drone.hostname)
                continue
            if require_ssp and not drone.support_ssp:
                logging.debug('Drone %s does not support server-side '
                              'packaging.', drone.hostname)
                no_ssp_drones.append(drone)
                continue

            usable_drones.append(drone)

            if drone.active_processes + num_processes <= drone.max_processes:
                drone_to_use = drone
                break
            logging.info('Drone %s has %d active + %s requested > %s max',
                         drone.hostname, drone.active_processes, num_processes,
                         drone.max_processes)

        if not drone_to_use and usable_drones:
            # Drones are all over loaded, pick the one with least load.
            drone_summary = ','.join('%s %s/%s' % (drone.hostname,
                                                   drone.active_processes,
                                                   drone.max_processes)
                                     for drone in usable_drones)
            logging.error('No drone has capacity to handle %d processes (%s) '
                          'for user %s', num_processes, drone_summary, username)
            drone_to_use = self._least_loaded_drone(usable_drones)
        elif not drone_to_use and require_ssp and no_ssp_drones:
            # No drone supports server-side packaging, choose the least loaded.
            drone_to_use = self._least_loaded_drone(no_ssp_drones)

        # refill _drone_queue
        for drone in checked_drones:
            self._enqueue_drone(drone)

        return drone_to_use


    def _substitute_working_directory_into_command(self, command,
                                                   working_directory):
        for i, item in enumerate(command):
            if item is WORKING_DIRECTORY:
                command[i] = working_directory


    def execute_command(self, command, working_directory, pidfile_name,
                        num_processes, log_file=None, paired_with_pidfile=None,
                        username=None, drone_hostnames_allowed=None):
        """
        Execute the given command, taken as an argv list.

        @param command: command to execute as a list.  if any item is
                WORKING_DIRECTORY, the absolute path to the working directory
                will be substituted for it.
        @param working_directory: directory in which the pidfile will be written
        @param pidfile_name: name of the pidfile this process will write
        @param num_processes: number of processes to account for from this
                execution
        @param log_file (optional): path (in the results repository) to hold
                command output.
        @param paired_with_pidfile (optional): a PidfileId for an
                already-executed process; the new process will execute on the
                same drone as the previous process.
        @param username (optional): login of the user responsible for this
                process.
        @param drone_hostnames_allowed (optional): hostnames of the drones that
                                                   this command is allowed to
                                                   execute on
        """
        abs_working_directory = self.absolute_path(working_directory)
        if not log_file:
            log_file = self.get_temporary_path('execute')
        log_file = self.absolute_path(log_file)

        self._substitute_working_directory_into_command(command,
                                                        abs_working_directory)

        if paired_with_pidfile:
            drone = self._get_drone_for_pidfile_id(paired_with_pidfile)
        else:
            require_ssp = '--require-ssp' in command
            drone = self._choose_drone_for_execution(
                    num_processes, username, drone_hostnames_allowed,
                    require_ssp=require_ssp)
            # Enable --warn-no-ssp option for autoserv to log a warning and run
            # the command without using server-side packaging.
            if require_ssp and not drone.support_ssp:
                command.append('--warn-no-ssp')

        if not drone:
            raise DroneManagerError('command failed; no drones available: %s'
                                    % command)

        logging.info("command = %s", command)
        logging.info('log file = %s:%s', drone.hostname, log_file)
        self._write_attached_files(working_directory, drone)
        drone.queue_call('execute_command', command, abs_working_directory,
                         log_file, pidfile_name)
        drone.active_processes += num_processes
        self._reorder_drone_queue()

        pidfile_path = os.path.join(abs_working_directory, pidfile_name)
        pidfile_id = PidfileId(pidfile_path)
        self.register_pidfile(pidfile_id)
        self._registered_pidfile_info[pidfile_id].num_processes = num_processes
        return pidfile_id


    def get_pidfile_id_from(self, execution_tag, pidfile_name):
        path = os.path.join(self.absolute_path(execution_tag), pidfile_name)
        return PidfileId(path)


    def register_pidfile(self, pidfile_id):
        """
        Indicate that the DroneManager should look for the given pidfile when
        refreshing.
        """
        if pidfile_id not in self._registered_pidfile_info:
            logging.info('monitoring pidfile %s', pidfile_id)
            self._registered_pidfile_info[pidfile_id] = _PidfileInfo()
        self._reset_pidfile_age(pidfile_id)


    def _reset_pidfile_age(self, pidfile_id):
        if pidfile_id in self._registered_pidfile_info:
            self._registered_pidfile_info[pidfile_id].age = 0


    def unregister_pidfile(self, pidfile_id):
        if pidfile_id in self._registered_pidfile_info:
            logging.info('forgetting pidfile %s', pidfile_id)
            del self._registered_pidfile_info[pidfile_id]


    def declare_process_count(self, pidfile_id, num_processes):
        self._registered_pidfile_info[pidfile_id].num_processes = num_processes


    def get_pidfile_contents(self, pidfile_id, use_second_read=False):
        """
        Retrieve a PidfileContents object for the given pidfile_id.  If
        use_second_read is True, use results that were read after the processes
        were checked, instead of before.
        """
        self._reset_pidfile_age(pidfile_id)
        if use_second_read:
            pidfile_map = self._pidfiles_second_read
        else:
            pidfile_map = self._pidfiles
        return pidfile_map.get(pidfile_id, PidfileContents())


    def is_process_running(self, process):
        """
        Check if the given process is in the running process list.
        """
        if process in self._process_set:
            return True

        drone_pid = process.hostname, process.pid
        if drone_pid in self._all_processes:
            logging.error('Process %s found, but not an autoserv process. '
                    'Is %s', process, self._all_processes[drone_pid])
            return True

        return False


    def get_temporary_path(self, base_name):
        """
        Get a new temporary path guaranteed to be unique across all drones
        for this scheduler execution.
        """
        self._temporary_path_counter += 1
        return os.path.join(drone_utility._TEMPORARY_DIRECTORY,
                            '%s.%s' % (base_name, self._temporary_path_counter))


    def absolute_path(self, path, on_results_repository=False):
        if on_results_repository:
            base_dir = self._results_dir
        else:
            base_dir = os.path.join(drones.AUTOTEST_INSTALL_DIR,
                                    _DRONE_RESULTS_DIR_SUFFIX)
        return os.path.join(base_dir, path)


    def _copy_results_helper(self, process, source_path, destination_path,
                             to_results_repository=False):
        logging.debug('_copy_results_helper. process: %s, source_path: %s, '
                      'destination_path: %s, to_results_repository: %s',
                      process, source_path, destination_path,
                      to_results_repository)
        full_source = self.absolute_path(source_path)
        full_destination = self.absolute_path(
                destination_path, on_results_repository=to_results_repository)
        source_drone = self._get_drone_for_process(process)
        if to_results_repository:
            source_drone.send_file_to(self._results_drone, full_source,
                                      full_destination, can_fail=True)
        else:
            source_drone.queue_call('copy_file_or_directory', full_source,
                                    full_destination)


    def copy_to_results_repository(self, process, source_path,
                                   destination_path=None):
        """
        Copy results from the given process at source_path to destination_path
        in the results repository.

        This will only copy the results back for Special Agent Tasks (Cleanup,
        Verify, Repair) that reside in the hosts/ subdirectory of results if
        the copy_task_results_back flag has been set to True inside
        global_config.ini

        It will also only copy .parse.log files back to the scheduler if the
        copy_parse_log_back flag in global_config.ini has been set to True.
        """
        if not ENABLE_ARCHIVING:
            return
        copy_task_results_back = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'copy_task_results_back',
                type=bool)
        copy_parse_log_back = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'copy_parse_log_back',
                type=bool)
        special_task = source_path.startswith(HOSTS_JOB_SUBDIR)
        parse_log = source_path.endswith(PARSE_LOG)
        if (copy_task_results_back or not special_task) and (
                copy_parse_log_back or not parse_log):
            if destination_path is None:
                destination_path = source_path
            self._copy_results_helper(process, source_path, destination_path,
                                      to_results_repository=True)

    def _copy_to_results_repository(self, process, source_path,
                                   destination_path=None):
        """
        Copy results from the given process at source_path to destination_path
        in the results repository, without special task handling.
        """
        if destination_path is None:
            destination_path = source_path
        self._copy_results_helper(process, source_path, destination_path,
                                  to_results_repository=True)


    def copy_results_on_drone(self, process, source_path, destination_path):
        """
        Copy a results directory from one place to another on the drone.
        """
        self._copy_results_helper(process, source_path, destination_path)


    def _write_attached_files(self, results_dir, drone):
        attached_files = self._attached_files.pop(results_dir, {})
        for file_path, contents in attached_files.iteritems():
            drone.queue_call('write_to_file', self.absolute_path(file_path),
                             contents)


    def attach_file_to_execution(self, results_dir, file_contents,
                                 file_path=None):
        """
        When the process for the results directory is executed, the given file
        contents will be placed in a file on the drone.  Returns the path at
        which the file will be placed.
        """
        if not file_path:
            file_path = self.get_temporary_path('attach')
        files_for_execution = self._attached_files.setdefault(results_dir, {})
        assert file_path not in files_for_execution
        files_for_execution[file_path] = file_contents
        return file_path


    def write_lines_to_file(self, file_path, lines, paired_with_process=None):
        """
        Write the given lines (as a list of strings) to a file.  If
        paired_with_process is given, the file will be written on the drone
        running the given Process.  Otherwise, the file will be written to the
        results repository.
        """
        file_contents = '\n'.join(lines) + '\n'
        if paired_with_process:
            drone = self._get_drone_for_process(paired_with_process)
            on_results_repository = False
        else:
            drone = self._results_drone
            on_results_repository = True
        full_path = self.absolute_path(
                file_path, on_results_repository=on_results_repository)
        drone.queue_call('write_to_file', full_path, file_contents)


_the_instance = None

def instance():
    if _the_instance is None:
        _set_instance(DroneManager())
    return _the_instance


def _set_instance(instance): # usable for testing
    global _the_instance
    _the_instance = instance
