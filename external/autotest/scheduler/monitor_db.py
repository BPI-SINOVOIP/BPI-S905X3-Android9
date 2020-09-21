#!/usr/bin/python

#pylint: disable=C0111

"""
Autotest scheduler
"""

import datetime
import functools
import gc
import logging
import optparse
import os
import signal
import sys
import time

import common
from autotest_lib.frontend import setup_django_environment

import django.db

from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib import utils
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import agent_task, drone_manager
from autotest_lib.scheduler import email_manager, gc_stats, host_scheduler
from autotest_lib.scheduler import luciferlib
from autotest_lib.scheduler import monitor_db_cleanup, prejob_task
from autotest_lib.scheduler import postjob_task
from autotest_lib.scheduler import query_managers
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.scheduler import scheduler_models
from autotest_lib.scheduler import scheduler_config
from autotest_lib.server import autoserv_utils
from autotest_lib.server import system_utils
from autotest_lib.server import utils as server_utils
from autotest_lib.site_utils import server_manager_utils

try:
    from chromite.lib import metrics
    from chromite.lib import ts_mon_config
except ImportError:
    metrics = utils.metrics_mock
    ts_mon_config = utils.metrics_mock


PID_FILE_PREFIX = 'monitor_db'

RESULTS_DIR = '.'
AUTOTEST_PATH = os.path.join(os.path.dirname(__file__), '..')

if os.environ.has_key('AUTOTEST_DIR'):
    AUTOTEST_PATH = os.environ['AUTOTEST_DIR']
AUTOTEST_SERVER_DIR = os.path.join(AUTOTEST_PATH, 'server')
AUTOTEST_TKO_DIR = os.path.join(AUTOTEST_PATH, 'tko')

if AUTOTEST_SERVER_DIR not in sys.path:
    sys.path.insert(0, AUTOTEST_SERVER_DIR)

# error message to leave in results dir when an autoserv process disappears
# mysteriously
_LOST_PROCESS_ERROR = """\
Autoserv failed abnormally during execution for this job, probably due to a
system error on the Autotest server.  Full results may not be available.  Sorry.
"""

_db_manager = None
_db = None
_shutdown = False

# These 2 globals are replaced for testing
_autoserv_directory = autoserv_utils.autoserv_directory
_autoserv_path = autoserv_utils.autoserv_path
_testing_mode = False
_drone_manager = None


def _verify_default_drone_set_exists():
    if (models.DroneSet.drone_sets_enabled() and
            not models.DroneSet.default_drone_set_name()):
        raise scheduler_lib.SchedulerError(
                'Drone sets are enabled, but no default is set')


def _sanity_check():
    """Make sure the configs are consistent before starting the scheduler"""
    _verify_default_drone_set_exists()


def main():
    try:
        try:
            main_without_exception_handling()
        except SystemExit:
            raise
        except:
            logging.exception('Exception escaping in monitor_db')
            raise
    finally:
        utils.delete_pid_file_if_exists(PID_FILE_PREFIX)


def main_without_exception_handling():
    scheduler_lib.setup_logging(
            os.environ.get('AUTOTEST_SCHEDULER_LOG_DIR', None),
            os.environ.get('AUTOTEST_SCHEDULER_LOG_NAME', None))
    usage = 'usage: %prog [options] results_dir'
    parser = optparse.OptionParser(usage)
    parser.add_option('--recover-hosts', help='Try to recover dead hosts',
                      action='store_true')
    parser.add_option('--test', help='Indicate that scheduler is under ' +
                      'test and should use dummy autoserv and no parsing',
                      action='store_true')
    parser.add_option(
            '--metrics-file',
            help='If provided, drop metrics to this local file instead of '
                 'reporting to ts_mon',
            type=str,
            default=None,
    )
    parser.add_option(
            '--lifetime-hours',
            type=float,
            default=None,
            help='If provided, number of hours the scheduler should run for. '
                 'At the expiry of this time, the process will exit '
                 'gracefully.',
    )
    parser.add_option('--production',
                      help=('Indicate that scheduler is running in production '
                            'environment and it can use database that is not '
                            'hosted in localhost. If it is set to False, '
                            'scheduler will fail if database is not in '
                            'localhost.'),
                      action='store_true', default=False)
    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.print_usage()
        return

    scheduler_lib.check_production_settings(options)

    scheduler_enabled = global_config.global_config.get_config_value(
        scheduler_config.CONFIG_SECTION, 'enable_scheduler', type=bool)

    if not scheduler_enabled:
        logging.error("Scheduler not enabled, set enable_scheduler to true in "
                      "the global_config's SCHEDULER section to enable it. "
                      "Exiting.")
        sys.exit(1)

    global RESULTS_DIR
    RESULTS_DIR = args[0]

    # Change the cwd while running to avoid issues incase we were launched from
    # somewhere odd (such as a random NFS home directory of the person running
    # sudo to launch us as the appropriate user).
    os.chdir(RESULTS_DIR)

    # This is helpful for debugging why stuff a scheduler launches is
    # misbehaving.
    logging.info('os.environ: %s', os.environ)

    if options.test:
        global _autoserv_path
        _autoserv_path = 'autoserv_dummy'
        global _testing_mode
        _testing_mode = True

    with ts_mon_config.SetupTsMonGlobalState('autotest_scheduler',
                                             indirect=True,
                                             debug_file=options.metrics_file):
      try:
          metrics.Counter('chromeos/autotest/scheduler/start').increment()
          process_start_time = time.time()
          initialize()
          dispatcher = Dispatcher()
          dispatcher.initialize(recover_hosts=options.recover_hosts)
          minimum_tick_sec = global_config.global_config.get_config_value(
                  scheduler_config.CONFIG_SECTION, 'minimum_tick_sec', type=float)

          while not _shutdown:
              if _lifetime_expired(options.lifetime_hours, process_start_time):
                  break

              start = time.time()
              dispatcher.tick()
              curr_tick_sec = time.time() - start
              if minimum_tick_sec > curr_tick_sec:
                  time.sleep(minimum_tick_sec - curr_tick_sec)
              else:
                  time.sleep(0.0001)
      except server_manager_utils.ServerActionError as e:
          # This error is expected when the server is not in primary status
          # for scheduler role. Thus do not send email for it.
          logging.exception(e)
      except Exception:
          logging.exception('Uncaught exception, terminating monitor_db.')
          metrics.Counter('chromeos/autotest/scheduler/uncaught_exception'
                          ).increment()

    email_manager.manager.send_queued_emails()
    _drone_manager.shutdown()
    _db_manager.disconnect()


def handle_signal(signum, frame):
    global _shutdown
    _shutdown = True
    logging.info("Shutdown request received.")


def _lifetime_expired(lifetime_hours, process_start_time):
    """Returns True if we've expired the process lifetime, False otherwise.

    Also sets the global _shutdown so that any background processes also take
    the cue to exit.
    """
    if lifetime_hours is None:
        return False
    if time.time() - process_start_time > lifetime_hours * 3600:
        logging.info('Process lifetime %0.3f hours exceeded. Shutting down.',
                     lifetime_hours)
        global _shutdown
        _shutdown = True
        return True
    return False


def initialize():
    logging.info("%s> dispatcher starting", time.strftime("%X %x"))
    logging.info("My PID is %d", os.getpid())

    if utils.program_is_alive(PID_FILE_PREFIX):
        logging.critical("monitor_db already running, aborting!")
        sys.exit(1)
    utils.write_pid(PID_FILE_PREFIX)

    if _testing_mode:
        global_config.global_config.override_config_value(
            scheduler_lib.DB_CONFIG_SECTION, 'database',
            'stresstest_autotest_web')

    # If server database is enabled, check if the server has role `scheduler`.
    # If the server does not have scheduler role, exception will be raised and
    # scheduler will not continue to run.
    if server_manager_utils.use_server_db():
        server_manager_utils.confirm_server_has_role(hostname='localhost',
                                                     role='scheduler')

    os.environ['PATH'] = AUTOTEST_SERVER_DIR + ':' + os.environ['PATH']
    global _db_manager
    _db_manager = scheduler_lib.ConnectionManager()
    global _db
    _db = _db_manager.get_connection()
    logging.info("Setting signal handler")
    signal.signal(signal.SIGINT, handle_signal)
    signal.signal(signal.SIGTERM, handle_signal)

    initialize_globals()
    scheduler_models.initialize()

    drone_list = system_utils.get_drones()
    results_host = global_config.global_config.get_config_value(
        scheduler_config.CONFIG_SECTION, 'results_host', default='localhost')
    _drone_manager.initialize(RESULTS_DIR, drone_list, results_host)

    logging.info("Connected! Running...")


def initialize_globals():
    global _drone_manager
    _drone_manager = drone_manager.instance()


def _autoserv_command_line(machines, extra_args, job=None, queue_entry=None,
                           verbose=True):
    """
    @returns The autoserv command line as a list of executable + parameters.

    @param machines - string - A machine or comma separated list of machines
            for the (-m) flag.
    @param extra_args - list - Additional arguments to pass to autoserv.
    @param job - Job object - If supplied, -u owner, -l name, --test-retry,
            and client -c or server -s parameters will be added.
    @param queue_entry - A HostQueueEntry object - If supplied and no Job
            object was supplied, this will be used to lookup the Job object.
    """
    command = autoserv_utils.autoserv_run_job_command(_autoserv_directory,
            machines, results_directory=drone_manager.WORKING_DIRECTORY,
            extra_args=extra_args, job=job, queue_entry=queue_entry,
            verbose=verbose, in_lab=True)
    return command

def _calls_log_tick_msg(func):
    """Used to trace functions called by Dispatcher.tick."""
    @functools.wraps(func)
    def wrapper(self, *args, **kwargs):
        self._log_tick_msg('Starting %s' % func.__name__)
        return func(self, *args, **kwargs)

    return wrapper


class Dispatcher(object):


    def __init__(self):
        self._agents = []
        self._last_clean_time = time.time()
        user_cleanup_time = scheduler_config.config.clean_interval_minutes
        self._periodic_cleanup = monitor_db_cleanup.UserCleanup(
                _db, user_cleanup_time)
        self._24hr_upkeep = monitor_db_cleanup.TwentyFourHourUpkeep(
                _db, _drone_manager)
        self._host_agents = {}
        self._queue_entry_agents = {}
        self._tick_count = 0
        self._last_garbage_stats_time = time.time()
        self._seconds_between_garbage_stats = 60 * (
                global_config.global_config.get_config_value(
                        scheduler_config.CONFIG_SECTION,
                        'gc_stats_interval_mins', type=int, default=6*60))
        self._tick_debug = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'tick_debug', type=bool,
                default=False)
        self._extra_debugging = global_config.global_config.get_config_value(
                scheduler_config.CONFIG_SECTION, 'extra_debugging', type=bool,
                default=False)
        self._inline_host_acquisition = (
                global_config.global_config.get_config_value(
                        scheduler_config.CONFIG_SECTION,
                        'inline_host_acquisition', type=bool, default=True))

        # If _inline_host_acquisition is set the scheduler will acquire and
        # release hosts against jobs inline, with the tick. Otherwise the
        # scheduler will only focus on jobs that already have hosts, and
        # will not explicitly unlease a host when a job finishes using it.
        self._job_query_manager = query_managers.AFEJobQueryManager()
        self._host_scheduler = (host_scheduler.BaseHostScheduler()
                                if self._inline_host_acquisition else
                                host_scheduler.DummyHostScheduler())


    def initialize(self, recover_hosts=True):
        self._periodic_cleanup.initialize()
        self._24hr_upkeep.initialize()
        # Execute all actions queued in the cleanup tasks. Scheduler tick will
        # run a refresh task first. If there is any action in the queue, refresh
        # will raise an exception.
        _drone_manager.execute_actions()

        # always recover processes
        self._recover_processes()

        if recover_hosts:
            self._recover_hosts()


    # TODO(pprabhu) Drop this metric once tick_times has been verified.
    @metrics.SecondsTimerDecorator(
            'chromeos/autotest/scheduler/tick_durations/tick')
    def tick(self):
        """
        This is an altered version of tick() where we keep track of when each
        major step begins so we can try to figure out where we are using most
        of the tick time.
        """
        with metrics.RuntimeBreakdownTimer(
            'chromeos/autotest/scheduler/tick_times') as breakdown_timer:
            self._log_tick_msg('New tick')
            system_utils.DroneCache.refresh()

            with breakdown_timer.Step('garbage_collection'):
                self._garbage_collection()
            with breakdown_timer.Step('trigger_refresh'):
                self._log_tick_msg('Starting _drone_manager.trigger_refresh')
                _drone_manager.trigger_refresh()
            with breakdown_timer.Step('schedule_running_host_queue_entries'):
                self._schedule_running_host_queue_entries()
            with breakdown_timer.Step('schedule_special_tasks'):
                self._schedule_special_tasks()
            with breakdown_timer.Step('schedule_new_jobs'):
                self._schedule_new_jobs()
            with breakdown_timer.Step('gather_tick_metrics'):
                self._gather_tick_metrics()
            with breakdown_timer.Step('sync_refresh'):
                self._log_tick_msg('Starting _drone_manager.sync_refresh')
                _drone_manager.sync_refresh()
            if luciferlib.is_lucifer_enabled():
                with breakdown_timer.Step('send_to_lucifer'):
                    self._send_to_lucifer()
            # _run_cleanup must be called between drone_manager.sync_refresh,
            # and drone_manager.execute_actions, as sync_refresh will clear the
            # calls queued in drones. Therefore, any action that calls
            # drone.queue_call to add calls to the drone._calls, should be after
            # drone refresh is completed and before
            # drone_manager.execute_actions at the end of the tick.
            with breakdown_timer.Step('run_cleanup'):
                self._run_cleanup()
            with breakdown_timer.Step('find_aborting'):
                self._find_aborting()
            with breakdown_timer.Step('find_aborted_special_tasks'):
                self._find_aborted_special_tasks()
            with breakdown_timer.Step('handle_agents'):
                self._handle_agents()
            with breakdown_timer.Step('host_scheduler_tick'):
                self._log_tick_msg('Starting _host_scheduler.tick')
                self._host_scheduler.tick()
            with breakdown_timer.Step('drones_execute_actions'):
                self._log_tick_msg('Starting _drone_manager.execute_actions')
                _drone_manager.execute_actions()
            with breakdown_timer.Step('send_queued_emails'):
                self._log_tick_msg(
                    'Starting email_manager.manager.send_queued_emails')
                email_manager.manager.send_queued_emails()
            with breakdown_timer.Step('db_reset_queries'):
                self._log_tick_msg('Starting django.db.reset_queries')
                django.db.reset_queries()

            self._tick_count += 1
            metrics.Counter('chromeos/autotest/scheduler/tick').increment()


    @_calls_log_tick_msg
    def _run_cleanup(self):
        self._periodic_cleanup.run_cleanup_maybe()
        self._24hr_upkeep.run_cleanup_maybe()


    @_calls_log_tick_msg
    def _garbage_collection(self):
        threshold_time = time.time() - self._seconds_between_garbage_stats
        if threshold_time < self._last_garbage_stats_time:
            # Don't generate these reports very often.
            return

        self._last_garbage_stats_time = time.time()
        # Force a full level 0 collection (because we can, it doesn't hurt
        # at this interval).
        gc.collect()
        logging.info('Logging garbage collector stats on tick %d.',
                     self._tick_count)
        gc_stats._log_garbage_collector_stats()


    def _gather_tick_metrics(self):
        """Gather metrics during tick, after all tasks have been scheduled."""
        metrics.Gauge(
            'chromeos/autotest/scheduler/agent_count'
        ).set(len(self._agents))


    def _register_agent_for_ids(self, agent_dict, object_ids, agent):
        for object_id in object_ids:
            agent_dict.setdefault(object_id, set()).add(agent)


    def _unregister_agent_for_ids(self, agent_dict, object_ids, agent):
        for object_id in object_ids:
            assert object_id in agent_dict
            agent_dict[object_id].remove(agent)
            # If an ID has no more active agent associated, there is no need to
            # keep it in the dictionary. Otherwise, scheduler will keep an
            # unnecessarily big dictionary until being restarted.
            if not agent_dict[object_id]:
                agent_dict.pop(object_id)


    def add_agent_task(self, agent_task):
        """
        Creates and adds an agent to the dispatchers list.

        In creating the agent we also pass on all the queue_entry_ids and
        host_ids from the special agent task. For every agent we create, we
        add it to 1. a dict against the queue_entry_ids given to it 2. A dict
        against the host_ids given to it. So theoritically, a host can have any
        number of agents associated with it, and each of them can have any
        special agent task, though in practice we never see > 1 agent/task per
        host at any time.

        @param agent_task: A SpecialTask for the agent to manage.
        """
        # These are owned by lucifer; don't manage these tasks.
        if (luciferlib.is_enabled_for('GATHERING')
            and (isinstance(agent_task, postjob_task.GatherLogsTask)
                 # TODO(crbug.com/811877): Don't skip split HQE parsing.
                 or (isinstance(agent_task, postjob_task.FinalReparseTask)
                     and not luciferlib.is_split_job(
                             agent_task.queue_entries[0].id)))):
            return
        if luciferlib.is_enabled_for('STARTING'):
            # TODO(crbug.com/810141): Transition code.  After running at
            # STARTING for a while, these tasks should no longer exist.
            if (isinstance(agent_task, postjob_task.GatherLogsTask)
                # TODO(crbug.com/811877): Don't skip split HQE parsing.
                or (isinstance(agent_task, postjob_task.FinalReparseTask)
                    and not luciferlib.is_split_job(
                            agent_task.queue_entries[0].id))):
                return
            # If this AgentTask is already started (i.e., recovered from
            # the scheduler running previously not at STARTING lucifer
            # level), we want to use the AgentTask to run the test to
            # completion.
            if (isinstance(agent_task, postjob_task.AbstractQueueTask)
                and not agent_task.started):
                return

        agent = Agent(agent_task)
        self._agents.append(agent)
        agent.dispatcher = self
        self._register_agent_for_ids(self._host_agents, agent.host_ids, agent)
        self._register_agent_for_ids(self._queue_entry_agents,
                                     agent.queue_entry_ids, agent)


    def get_agents_for_entry(self, queue_entry):
        """
        Find agents corresponding to the specified queue_entry.
        """
        return list(self._queue_entry_agents.get(queue_entry.id, set()))


    def host_has_agent(self, host):
        """
        Determine if there is currently an Agent present using this host.
        """
        return bool(self._host_agents.get(host.id, None))


    def remove_agent(self, agent):
        self._agents.remove(agent)
        self._unregister_agent_for_ids(self._host_agents, agent.host_ids,
                                       agent)
        self._unregister_agent_for_ids(self._queue_entry_agents,
                                       agent.queue_entry_ids, agent)


    def _host_has_scheduled_special_task(self, host):
        return bool(models.SpecialTask.objects.filter(host__id=host.id,
                                                      is_active=False,
                                                      is_complete=False))


    def _recover_processes(self):
        agent_tasks = self._create_recovery_agent_tasks()
        self._register_pidfiles(agent_tasks)
        _drone_manager.refresh()
        self._recover_tasks(agent_tasks)
        self._recover_pending_entries()
        self._check_for_unrecovered_verifying_entries()
        self._reverify_remaining_hosts()
        # reinitialize drones after killing orphaned processes, since they can
        # leave around files when they die
        _drone_manager.execute_actions()
        _drone_manager.reinitialize_drones()


    def _create_recovery_agent_tasks(self):
        return (self._get_queue_entry_agent_tasks()
                + self._get_special_task_agent_tasks(is_active=True))


    def _get_queue_entry_agent_tasks(self):
        """
        Get agent tasks for all hqe in the specified states.

        Loosely this translates to taking a hqe in one of the specified states,
        say parsing, and getting an AgentTask for it, like the FinalReparseTask,
        through _get_agent_task_for_queue_entry. Each queue entry can only have
        one agent task at a time, but there might be multiple queue entries in
        the group.

        @return: A list of AgentTasks.
        """
        # host queue entry statuses handled directly by AgentTasks
        # (Verifying is handled through SpecialTasks, so is not
        # listed here)
        statuses = (models.HostQueueEntry.Status.STARTING,
                    models.HostQueueEntry.Status.RUNNING,
                    models.HostQueueEntry.Status.GATHERING,
                    models.HostQueueEntry.Status.PARSING)
        status_list = ','.join("'%s'" % status for status in statuses)
        queue_entries = scheduler_models.HostQueueEntry.fetch(
                where='status IN (%s)' % status_list)

        agent_tasks = []
        used_queue_entries = set()
        hqe_count_by_status = {}
        for entry in queue_entries:
            try:
                hqe_count_by_status[entry.status] = (
                    hqe_count_by_status.get(entry.status, 0) + 1)
                if self.get_agents_for_entry(entry):
                    # already being handled
                    continue
                if entry in used_queue_entries:
                    # already picked up by a synchronous job
                    continue
                try:
                    agent_task = self._get_agent_task_for_queue_entry(entry)
                except scheduler_lib.SchedulerError:
                    # Probably being handled by lucifer crbug.com/809773
                    continue
                agent_tasks.append(agent_task)
                used_queue_entries.update(agent_task.queue_entries)
            except scheduler_lib.MalformedRecordError as e:
                logging.exception('Skipping agent task for a malformed hqe.')
                # TODO(akeshet): figure out a way to safely permanently discard
                # this errant HQE. It appears that calling entry.abort() is not
                # sufficient, as that already makes some assumptions about
                # record sanity that may be violated. See crbug.com/739530 for
                # context.
                m = 'chromeos/autotest/scheduler/skipped_malformed_hqe'
                metrics.Counter(m).increment()

        for status, count in hqe_count_by_status.iteritems():
            metrics.Gauge(
                'chromeos/autotest/scheduler/active_host_queue_entries'
            ).set(count, fields={'status': status})

        return agent_tasks


    def _get_special_task_agent_tasks(self, is_active=False):
        special_tasks = models.SpecialTask.objects.filter(
                is_active=is_active, is_complete=False)
        agent_tasks = []
        for task in special_tasks:
          try:
              agent_tasks.append(self._get_agent_task_for_special_task(task))
          except scheduler_lib.MalformedRecordError as e:
              logging.exception('Skipping agent task for malformed special '
                                'task.')
              m = 'chromeos/autotest/scheduler/skipped_malformed_special_task'
              metrics.Counter(m).increment()
        return agent_tasks


    def _get_agent_task_for_queue_entry(self, queue_entry):
        """
        Construct an AgentTask instance for the given active HostQueueEntry.

        @param queue_entry: a HostQueueEntry
        @return: an AgentTask to run the queue entry
        """
        task_entries = queue_entry.job.get_group_entries(queue_entry)
        self._check_for_duplicate_host_entries(task_entries)

        if queue_entry.status in (models.HostQueueEntry.Status.STARTING,
                                  models.HostQueueEntry.Status.RUNNING):
            if queue_entry.is_hostless():
                return HostlessQueueTask(queue_entry=queue_entry)
            return QueueTask(queue_entries=task_entries)
        if queue_entry.status == models.HostQueueEntry.Status.GATHERING:
            return postjob_task.GatherLogsTask(queue_entries=task_entries)
        if queue_entry.status == models.HostQueueEntry.Status.PARSING:
            return postjob_task.FinalReparseTask(queue_entries=task_entries)

        raise scheduler_lib.MalformedRecordError(
                '_get_agent_task_for_queue_entry got entry with '
                'invalid status %s: %s' % (queue_entry.status, queue_entry))


    def _check_for_duplicate_host_entries(self, task_entries):
        non_host_statuses = {models.HostQueueEntry.Status.PARSING}
        for task_entry in task_entries:
            using_host = (task_entry.host is not None
                          and task_entry.status not in non_host_statuses)
            if using_host:
                self._assert_host_has_no_agent(task_entry)


    def _assert_host_has_no_agent(self, entry):
        """
        @param entry: a HostQueueEntry or a SpecialTask
        """
        if self.host_has_agent(entry.host):
            agent = tuple(self._host_agents.get(entry.host.id))[0]
            raise scheduler_lib.MalformedRecordError(
                    'While scheduling %s, host %s already has a host agent %s'
                    % (entry, entry.host, agent.task))


    def _get_agent_task_for_special_task(self, special_task):
        """
        Construct an AgentTask class to run the given SpecialTask and add it
        to this dispatcher.

        A special task is created through schedule_special_tasks, but only if
        the host doesn't already have an agent. This happens through
        add_agent_task. All special agent tasks are given a host on creation,
        and a Null hqe. To create a SpecialAgentTask object, you need a
        models.SpecialTask. If the SpecialTask used to create a SpecialAgentTask
        object contains a hqe it's passed on to the special agent task, which
        creates a HostQueueEntry and saves it as it's queue_entry.

        @param special_task: a models.SpecialTask instance
        @returns an AgentTask to run this SpecialTask
        """
        self._assert_host_has_no_agent(special_task)

        special_agent_task_classes = (prejob_task.CleanupTask,
                                      prejob_task.VerifyTask,
                                      prejob_task.RepairTask,
                                      prejob_task.ResetTask,
                                      prejob_task.ProvisionTask)

        for agent_task_class in special_agent_task_classes:
            if agent_task_class.TASK_TYPE == special_task.task:
                return agent_task_class(task=special_task)

        raise scheduler_lib.MalformedRecordError(
                'No AgentTask class for task', str(special_task))


    def _register_pidfiles(self, agent_tasks):
        for agent_task in agent_tasks:
            agent_task.register_necessary_pidfiles()


    def _recover_tasks(self, agent_tasks):
        orphans = _drone_manager.get_orphaned_autoserv_processes()

        for agent_task in agent_tasks:
            agent_task.recover()
            if agent_task.monitor and agent_task.monitor.has_process():
                orphans.discard(agent_task.monitor.get_process())
            self.add_agent_task(agent_task)

        self._check_for_remaining_orphan_processes(orphans)


    def _get_unassigned_entries(self, status):
        for entry in scheduler_models.HostQueueEntry.fetch(where="status = '%s'"
                                                           % status):
            if entry.status == status and not self.get_agents_for_entry(entry):
                # The status can change during iteration, e.g., if job.run()
                # sets a group of queue entries to Starting
                yield entry


    def _check_for_remaining_orphan_processes(self, orphans):
        m = 'chromeos/autotest/errors/unrecovered_orphan_processes'
        metrics.Gauge(m).set(len(orphans))

        if not orphans:
            return
        subject = 'Unrecovered orphan autoserv processes remain'
        message = '\n'.join(str(process) for process in orphans)
        die_on_orphans = global_config.global_config.get_config_value(
            scheduler_config.CONFIG_SECTION, 'die_on_orphans', type=bool)

        if die_on_orphans:
            raise RuntimeError(subject + '\n' + message)


    def _recover_pending_entries(self):
        for entry in self._get_unassigned_entries(
                models.HostQueueEntry.Status.PENDING):
            logging.info('Recovering Pending entry %s', entry)
            try:
                entry.on_pending()
            except scheduler_lib.MalformedRecordError as e:
                logging.exception(
                        'Skipping agent task for malformed special task.')
                m = 'chromeos/autotest/scheduler/skipped_malformed_special_task'
                metrics.Counter(m).increment()


    def _check_for_unrecovered_verifying_entries(self):
        # Verify is replaced by Reset.
        queue_entries = scheduler_models.HostQueueEntry.fetch(
                where='status = "%s"' % models.HostQueueEntry.Status.RESETTING)
        for queue_entry in queue_entries:
            special_tasks = models.SpecialTask.objects.filter(
                    task__in=(models.SpecialTask.Task.CLEANUP,
                              models.SpecialTask.Task.VERIFY,
                              models.SpecialTask.Task.RESET),
                    queue_entry__id=queue_entry.id,
                    is_complete=False)
            if special_tasks.count() == 0:
                logging.error('Unrecovered Resetting host queue entry: %s. '
                              'Setting status to Queued.', str(queue_entry))
                # Essentially this host queue entry was set to be Verifying
                # however no special task exists for entry. This occurs if the
                # scheduler dies between changing the status and creating the
                # special task. By setting it to queued, the job can restart
                # from the beginning and proceed correctly. This is much more
                # preferable than having monitor_db not launching.
                queue_entry.set_status('Queued')


    @_calls_log_tick_msg
    def _schedule_special_tasks(self):
        """
        Execute queued SpecialTasks that are ready to run on idle hosts.

        Special tasks include PreJobTasks like verify, reset and cleanup.
        They are created through _schedule_new_jobs and associated with a hqe
        This method translates SpecialTasks to the appropriate AgentTask and
        adds them to the dispatchers agents list, so _handle_agents can execute
        them.
        """
        # When the host scheduler is responsible for acquisition we only want
        # to run tasks with leased hosts. All hqe tasks will already have
        # leased hosts, and we don't want to run frontend tasks till the host
        # scheduler has vetted the assignment. Note that this doesn't include
        # frontend tasks with hosts leased by other active hqes.
        for task in self._job_query_manager.get_prioritized_special_tasks(
                only_tasks_with_leased_hosts=not self._inline_host_acquisition):
            if self.host_has_agent(task.host):
                continue
            try:
                self.add_agent_task(self._get_agent_task_for_special_task(task))
            except scheduler_lib.MalformedRecordError:
                logging.exception('Skipping schedule for malformed '
                                  'special task.')
                m = 'chromeos/autotest/scheduler/skipped_schedule_special_task'
                metrics.Counter(m).increment()


    def _reverify_remaining_hosts(self):
        # recover active hosts that have not yet been recovered, although this
        # should never happen
        message = ('Recovering active host %s - this probably indicates a '
                   'scheduler bug')
        self._reverify_hosts_where(
                "status IN ('Repairing', 'Verifying', 'Cleaning', 'Provisioning')",
                print_message=message)


    DEFAULT_REQUESTED_BY_USER_ID = 1


    def _reverify_hosts_where(self, where,
                              print_message='Reverifying host %s'):
        full_where='locked = 0 AND invalid = 0 AND ' + where
        for host in scheduler_models.Host.fetch(where=full_where):
            if self.host_has_agent(host):
                # host has already been recovered in some way
                continue
            if self._host_has_scheduled_special_task(host):
                # host will have a special task scheduled on the next cycle
                continue
            if print_message:
                logging.error(print_message, host.hostname)
            try:
                user = models.User.objects.get(login='autotest_system')
            except models.User.DoesNotExist:
                user = models.User.objects.get(
                        id=self.DEFAULT_REQUESTED_BY_USER_ID)
            models.SpecialTask.objects.create(
                    task=models.SpecialTask.Task.RESET,
                    host=models.Host.objects.get(id=host.id),
                    requested_by=user)


    def _recover_hosts(self):
        # recover "Repair Failed" hosts
        message = 'Reverifying dead host %s'
        self._reverify_hosts_where("status = 'Repair Failed'",
                                   print_message=message)


    def _refresh_pending_queue_entries(self):
        """
        Lookup the pending HostQueueEntries and call our HostScheduler
        refresh() method given that list.  Return the list.

        @returns A list of pending HostQueueEntries sorted in priority order.
        """
        queue_entries = self._job_query_manager.get_pending_queue_entries(
                only_hostless=not self._inline_host_acquisition)
        if not queue_entries:
            return []
        return queue_entries


    def _schedule_hostless_job(self, queue_entry):
        """Schedule a hostless (suite) job.

        @param queue_entry: The queue_entry representing the hostless job.
        """
        if not luciferlib.is_enabled_for('STARTING'):
            self.add_agent_task(HostlessQueueTask(queue_entry))

        # Need to set execution_subdir before setting the status:
        # After a restart of the scheduler, agents will be restored for HQEs in
        # Starting, Running, Gathering, Parsing or Archiving. To do this, the
        # execution_subdir is needed. Therefore it must be set before entering
        # one of these states.
        # Otherwise, if the scheduler was interrupted between setting the status
        # and the execution_subdir, upon it's restart restoring agents would
        # fail.
        # Is there a way to get a status in one of these states without going
        # through this code? Following cases are possible:
        # - If it's aborted before being started:
        #     active bit will be 0, so there's nothing to parse, it will just be
        #     set to completed by _find_aborting. Critical statuses are skipped.
        # - If it's aborted or it fails after being started:
        #     It was started, so this code was executed.
        queue_entry.update_field('execution_subdir', 'hostless')
        queue_entry.set_status(models.HostQueueEntry.Status.STARTING)


    def _schedule_host_job(self, host, queue_entry):
        """Schedules a job on the given host.

        1. Assign the host to the hqe, if it isn't already assigned.
        2. Create a SpecialAgentTask for the hqe.
        3. Activate the hqe.

        @param queue_entry: The job to schedule.
        @param host: The host to schedule the job on.
        """
        if self.host_has_agent(host):
            host_agent_task = list(self._host_agents.get(host.id))[0].task
        else:
            self._host_scheduler.schedule_host_job(host, queue_entry)


    @_calls_log_tick_msg
    def _schedule_new_jobs(self):
        """
        Find any new HQEs and call schedule_pre_job_tasks for it.

        This involves setting the status of the HQE and creating a row in the
        db corresponding the the special task, through
        scheduler_models._queue_special_task. The new db row is then added as
        an agent to the dispatcher through _schedule_special_tasks and
        scheduled for execution on the drone through _handle_agents.
        """
        queue_entries = self._refresh_pending_queue_entries()

        key = 'scheduler.jobs_per_tick'
        new_hostless_jobs = 0
        new_jobs_with_hosts = 0
        new_jobs_need_hosts = 0
        host_jobs = []
        logging.debug('Processing %d queue_entries', len(queue_entries))

        for queue_entry in queue_entries:
            if queue_entry.is_hostless():
                self._schedule_hostless_job(queue_entry)
                new_hostless_jobs = new_hostless_jobs + 1
            else:
                host_jobs.append(queue_entry)
                new_jobs_need_hosts = new_jobs_need_hosts + 1

        metrics.Counter(
            'chromeos/autotest/scheduler/scheduled_jobs_hostless'
        ).increment_by(new_hostless_jobs)

        if not host_jobs:
            return

        if not self._inline_host_acquisition:
          # In this case, host_scheduler is responsible for scheduling
          # host_jobs. Scheduling the jobs ourselves can lead to DB corruption
          # since host_scheduler assumes it is the single process scheduling
          # host jobs.
          metrics.Gauge(
              'chromeos/autotest/errors/scheduler/unexpected_host_jobs').set(
                  len(host_jobs))
          return

        jobs_with_hosts = self._host_scheduler.find_hosts_for_jobs(host_jobs)
        for host_assignment in jobs_with_hosts:
            self._schedule_host_job(host_assignment.host, host_assignment.job)
            new_jobs_with_hosts = new_jobs_with_hosts + 1

        metrics.Counter(
            'chromeos/autotest/scheduler/scheduled_jobs_with_hosts'
        ).increment_by(new_jobs_with_hosts)


    @_calls_log_tick_msg
    def _send_to_lucifer(self):
        """
        Hand off ownership of a job to lucifer component.
        """
        if luciferlib.is_enabled_for('starting'):
            self._send_starting_to_lucifer()
        # TODO(crbug.com/810141): Older states need to be supported when
        # STARTING is toggled; some jobs may be in an intermediate state
        # at that moment.
        self._send_gathering_to_lucifer()
        self._send_parsing_to_lucifer()


    # TODO(crbug.com/748234): This is temporary to enable toggling
    # lucifer rollouts with an option.
    def _send_starting_to_lucifer(self):
        Status = models.HostQueueEntry.Status
        queue_entries_qs = (models.HostQueueEntry.objects
                            .filter(status=Status.STARTING))
        for queue_entry in queue_entries_qs:
            if self.get_agents_for_entry(queue_entry):
                continue
            job = queue_entry.job
            if luciferlib.is_lucifer_owned(job):
                continue
            drone = luciferlib.spawn_starting_job_handler(
                    manager=_drone_manager,
                    job=job)
            models.JobHandoff.objects.create(job=job, drone=drone.hostname())


    # TODO(crbug.com/748234): This is temporary to enable toggling
    # lucifer rollouts with an option.
    def _send_gathering_to_lucifer(self):
        Status = models.HostQueueEntry.Status
        queue_entries_qs = (models.HostQueueEntry.objects
                            .filter(status=Status.GATHERING))
        for queue_entry in queue_entries_qs:
            # If this HQE already has an agent, let monitor_db continue
            # owning it.
            if self.get_agents_for_entry(queue_entry):
                continue

            job = queue_entry.job
            if luciferlib.is_lucifer_owned(job):
                continue
            task = postjob_task.PostJobTask(
                    [queue_entry], log_file_name='/dev/null')
            pidfile_id = task._autoserv_monitor.pidfile_id
            autoserv_exit = task._autoserv_monitor.exit_code()
            try:
                drone = luciferlib.spawn_gathering_job_handler(
                        manager=_drone_manager,
                        job=job,
                        autoserv_exit=autoserv_exit,
                        pidfile_id=pidfile_id)
                models.JobHandoff.objects.create(job=job,
                                                 drone=drone.hostname())
            except drone_manager.DroneManagerError as e:
                logging.warning(
                    'Fail to get drone for job %s, skipping lucifer. Error: %s',
                    job.id, e)


    # TODO(crbug.com/748234): This is temporary to enable toggling
    # lucifer rollouts with an option.
    def _send_parsing_to_lucifer(self):
        Status = models.HostQueueEntry.Status
        queue_entries_qs = (models.HostQueueEntry.objects
                            .filter(status=Status.PARSING))
        for queue_entry in queue_entries_qs:
            # If this HQE already has an agent, let monitor_db continue
            # owning it.
            if self.get_agents_for_entry(queue_entry):
                continue
            job = queue_entry.job
            if luciferlib.is_lucifer_owned(job):
                continue
            # TODO(crbug.com/811877): Ignore split HQEs.
            if luciferlib.is_split_job(queue_entry.id):
                continue
            task = postjob_task.PostJobTask(
                    [queue_entry], log_file_name='/dev/null')
            pidfile_id = task._autoserv_monitor.pidfile_id
            autoserv_exit = task._autoserv_monitor.exit_code()
            try:
                drone = luciferlib.spawn_parsing_job_handler(
                        manager=_drone_manager,
                        job=job,
                        autoserv_exit=autoserv_exit,
                        pidfile_id=pidfile_id)
                models.JobHandoff.objects.create(job=job,
                                                 drone=drone.hostname())
            except drone_manager.DroneManagerError as e:
                logging.warning(
                    'Fail to get drone for job %s, skipping lucifer. Error: %s',
                    job.id, e)



    @_calls_log_tick_msg
    def _schedule_running_host_queue_entries(self):
        """
        Adds agents to the dispatcher.

        Any AgentTask, like the QueueTask, is wrapped in an Agent. The
        QueueTask for example, will have a job with a control file, and
        the agent will have methods that poll, abort and check if the queue
        task is finished. The dispatcher runs the agent_task, as well as
        other agents in it's _agents member, through _handle_agents, by
        calling the Agents tick().

        This method creates an agent for each HQE in one of (starting, running,
        gathering, parsing) states, and adds it to the dispatcher so
        it is handled by _handle_agents.
        """
        for agent_task in self._get_queue_entry_agent_tasks():
            self.add_agent_task(agent_task)


    @_calls_log_tick_msg
    def _find_aborting(self):
        """
        Looks through the afe_host_queue_entries for an aborted entry.

        The aborted bit is set on an HQE in many ways, the most common
        being when a user requests an abort through the frontend, which
        results in an rpc from the afe to abort_host_queue_entries.
        """
        jobs_to_stop = set()
        for entry in scheduler_models.HostQueueEntry.fetch(
                where='aborted=1 and complete=0'):

            # If the job is running on a shard, let the shard handle aborting
            # it and sync back the right status.
            if entry.job.shard_id is not None and not server_utils.is_shard():
                logging.info('Waiting for shard %s to abort hqe %s',
                        entry.job.shard_id, entry)
                continue

            logging.info('Aborting %s', entry)

            # The task would have started off with both is_complete and
            # is_active = False. Aborted tasks are neither active nor complete.
            # For all currently active tasks this will happen through the agent,
            # but we need to manually update the special tasks that haven't
            # started yet, because they don't have agents.
            models.SpecialTask.objects.filter(is_active=False,
                queue_entry_id=entry.id).update(is_complete=True)

            for agent in self.get_agents_for_entry(entry):
                agent.abort()
            entry.abort(self)
            jobs_to_stop.add(entry.job)
        logging.debug('Aborting %d jobs this tick.', len(jobs_to_stop))
        for job in jobs_to_stop:
            job.stop_if_necessary()


    @_calls_log_tick_msg
    def _find_aborted_special_tasks(self):
        """
        Find SpecialTasks that have been marked for abortion.

        Poll the database looking for SpecialTasks that are active
        and have been marked for abortion, then abort them.
        """

        # The completed and active bits are very important when it comes
        # to scheduler correctness. The active bit is set through the prolog
        # of a special task, and reset through the cleanup method of the
        # SpecialAgentTask. The cleanup is called both through the abort and
        # epilog. The complete bit is set in several places, and in general
        # a hanging job will have is_active=1 is_complete=0, while a special
        # task which completed will have is_active=0 is_complete=1. To check
        # aborts we directly check active because the complete bit is set in
        # several places, including the epilog of agent tasks.
        aborted_tasks = models.SpecialTask.objects.filter(is_active=True,
                                                          is_aborted=True)
        for task in aborted_tasks:
            # There are 2 ways to get the agent associated with a task,
            # through the host and through the hqe. A special task
            # always needs a host, but doesn't always need a hqe.
            for agent in self._host_agents.get(task.host.id, []):
                if isinstance(agent.task, agent_task.SpecialAgentTask):

                    # The epilog preforms critical actions such as
                    # queueing the next SpecialTask, requeuing the
                    # hqe etc, however it doesn't actually kill the
                    # monitor process and set the 'done' bit. Epilogs
                    # assume that the job failed, and that the monitor
                    # process has already written an exit code. The
                    # done bit is a necessary condition for
                    # _handle_agents to schedule any more special
                    # tasks against the host, and it must be set
                    # in addition to is_active, is_complete and success.
                    agent.task.epilog()
                    agent.task.abort()


    def _can_start_agent(self, agent, have_reached_limit):
        # always allow zero-process agents to run
        if agent.task.num_processes == 0:
            return True
        # don't allow any nonzero-process agents to run after we've reached a
        # limit (this avoids starvation of many-process agents)
        if have_reached_limit:
            return False
        # total process throttling
        max_runnable_processes = _drone_manager.max_runnable_processes(
                agent.task.owner_username,
                agent.task.get_drone_hostnames_allowed())
        if agent.task.num_processes > max_runnable_processes:
            return False
        return True


    @_calls_log_tick_msg
    def _handle_agents(self):
        """
        Handles agents of the dispatcher.

        Appropriate Agents are added to the dispatcher through
        _schedule_running_host_queue_entries. These agents each
        have a task. This method runs the agents task through
        agent.tick() leading to:
            agent.start
                prolog -> AgentTasks prolog
                          For each queue entry:
                            sets host status/status to Running
                            set started_on in afe_host_queue_entries
                run    -> AgentTasks run
                          Creates PidfileRunMonitor
                          Queues the autoserv command line for this AgentTask
                          via the drone manager. These commands are executed
                          through the drone managers execute actions.
                poll   -> AgentTasks/BaseAgentTask poll
                          checks the monitors exit_code.
                          Executes epilog if task is finished.
                          Executes AgentTasks _finish_task
                finish_task is usually responsible for setting the status
                of the HQE/host, and updating it's active and complete fileds.

            agent.is_done
                Removed the agent from the dispatchers _agents queue.
                Is_done checks the finished bit on the agent, that is
                set based on the Agents task. During the agents poll
                we check to see if the monitor process has exited in
                it's finish method, and set the success member of the
                task based on this exit code.
        """
        num_started_this_tick = 0
        num_finished_this_tick = 0
        have_reached_limit = False
        # iterate over copy, so we can remove agents during iteration
        logging.debug('Handling %d Agents', len(self._agents))
        for agent in list(self._agents):
            self._log_extra_msg('Processing Agent with Host Ids: %s and '
                                'queue_entry ids:%s' % (agent.host_ids,
                                agent.queue_entry_ids))
            if not agent.started:
                if not self._can_start_agent(agent, have_reached_limit):
                    have_reached_limit = True
                    logging.debug('Reached Limit of allowed running Agents.')
                    continue
                num_started_this_tick += agent.task.num_processes
                self._log_extra_msg('Starting Agent')
            agent.tick()
            self._log_extra_msg('Agent tick completed.')
            if agent.is_done():
                num_finished_this_tick += agent.task.num_processes
                self._log_extra_msg("Agent finished")
                self.remove_agent(agent)

        metrics.Counter(
            'chromeos/autotest/scheduler/agent_processes_started'
        ).increment_by(num_started_this_tick)
        metrics.Counter(
            'chromeos/autotest/scheduler/agent_processes_finished'
        ).increment_by(num_finished_this_tick)
        num_agent_processes = _drone_manager.total_running_processes()
        metrics.Gauge(
            'chromeos/autotest/scheduler/agent_processes'
        ).set(num_agent_processes)
        logging.info('%d running processes. %d added this tick.',
                     num_agent_processes, num_started_this_tick)


    def _log_tick_msg(self, msg):
        if self._tick_debug:
            logging.debug(msg)


    def _log_extra_msg(self, msg):
        if self._extra_debugging:
            logging.debug(msg)


class Agent(object):
    """
    An agent for use by the Dispatcher class to perform a task.  An agent wraps
    around an AgentTask mainly to associate the AgentTask with the queue_entry
    and host ids.

    The following methods are required on all task objects:
        poll() - Called periodically to let the task check its status and
                update its internal state.  If the task succeeded.
        is_done() - Returns True if the task is finished.
        abort() - Called when an abort has been requested.  The task must
                set its aborted attribute to True if it actually aborted.

    The following attributes are required on all task objects:
        aborted - bool, True if this task was aborted.
        success - bool, True if this task succeeded.
        queue_entry_ids - A sequence of HostQueueEntry ids this task handles.
        host_ids - A sequence of Host ids this task represents.
    """


    def __init__(self, task):
        """
        @param task: An instance of an AgentTask.
        """
        self.task = task

        # This is filled in by Dispatcher.add_agent()
        self.dispatcher = None

        self.queue_entry_ids = task.queue_entry_ids
        self.host_ids = task.host_ids

        self.started = False
        self.finished = False


    def tick(self):
        self.started = True
        if not self.finished:
            self.task.poll()
            if self.task.is_done():
                self.finished = True


    def is_done(self):
        return self.finished


    def abort(self):
        if self.task:
            self.task.abort()
            if self.task.aborted:
                # tasks can choose to ignore aborts
                self.finished = True


class AbstractQueueTask(agent_task.AgentTask, agent_task.TaskWithJobKeyvals):
    """
    Common functionality for QueueTask and HostlessQueueTask
    """
    def __init__(self, queue_entries):
        super(AbstractQueueTask, self).__init__()
        self.job = queue_entries[0].job
        self.queue_entries = queue_entries


    def _keyval_path(self):
        return os.path.join(self._working_directory(), self._KEYVAL_FILE)


    def _write_control_file(self, execution_path):
        control_path = _drone_manager.attach_file_to_execution(
                execution_path, self.job.control_file)
        return control_path


    # TODO: Refactor into autoserv_utils. crbug.com/243090
    def _command_line(self):
        execution_path = self.queue_entries[0].execution_path()
        control_path = self._write_control_file(execution_path)
        hostnames = ','.join(entry.host.hostname
                             for entry in self.queue_entries
                             if not entry.is_hostless())

        execution_tag = self.queue_entries[0].execution_tag()
        params = _autoserv_command_line(
            hostnames,
            ['-P', execution_tag, '-n',
             _drone_manager.absolute_path(control_path)],
            job=self.job, verbose=False)

        return params


    @property
    def num_processes(self):
        return len(self.queue_entries)


    @property
    def owner_username(self):
        return self.job.owner


    def _working_directory(self):
        return self._get_consistent_execution_path(self.queue_entries)


    def prolog(self):
        queued_key, queued_time = self._job_queued_keyval(self.job)
        keyval_dict = self.job.keyval_dict()
        keyval_dict[queued_key] = queued_time
        self._write_keyvals_before_job(keyval_dict)
        for queue_entry in self.queue_entries:
            queue_entry.set_status(models.HostQueueEntry.Status.RUNNING)
            queue_entry.set_started_on_now()


    def _write_lost_process_error_file(self):
        error_file_path = os.path.join(self._working_directory(), 'job_failure')
        _drone_manager.write_lines_to_file(error_file_path,
                                           [_LOST_PROCESS_ERROR])


    def _finish_task(self):
        if not self.monitor:
            return

        self._write_job_finished()

        if self.monitor.lost_process:
            self._write_lost_process_error_file()


    def _write_status_comment(self, comment):
        _drone_manager.write_lines_to_file(
            os.path.join(self._working_directory(), 'status.log'),
            ['INFO\t----\t----\t' + comment],
            paired_with_process=self.monitor.get_process())


    def _log_abort(self):
        if not self.monitor or not self.monitor.has_process():
            return

        # build up sets of all the aborted_by and aborted_on values
        aborted_by, aborted_on = set(), set()
        for queue_entry in self.queue_entries:
            if queue_entry.aborted_by:
                aborted_by.add(queue_entry.aborted_by)
                t = int(time.mktime(queue_entry.aborted_on.timetuple()))
                aborted_on.add(t)

        # extract some actual, unique aborted by value and write it out
        # TODO(showard): this conditional is now obsolete, we just need to leave
        # it in temporarily for backwards compatibility over upgrades.  delete
        # soon.
        assert len(aborted_by) <= 1
        if len(aborted_by) == 1:
            aborted_by_value = aborted_by.pop()
            aborted_on_value = max(aborted_on)
        else:
            aborted_by_value = 'autotest_system'
            aborted_on_value = int(time.time())

        self._write_keyval_after_job("aborted_by", aborted_by_value)
        self._write_keyval_after_job("aborted_on", aborted_on_value)

        aborted_on_string = str(datetime.datetime.fromtimestamp(
            aborted_on_value))
        self._write_status_comment('Job aborted by %s on %s' %
                                   (aborted_by_value, aborted_on_string))


    def abort(self):
        super(AbstractQueueTask, self).abort()
        self._log_abort()
        self._finish_task()


    def epilog(self):
        super(AbstractQueueTask, self).epilog()
        self._finish_task()


class QueueTask(AbstractQueueTask):
    def __init__(self, queue_entries):
        super(QueueTask, self).__init__(queue_entries)
        self._set_ids(queue_entries=queue_entries)
        self._enable_ssp_container = (
                global_config.global_config.get_config_value(
                        'AUTOSERV', 'enable_ssp_container', type=bool,
                        default=True))


    def prolog(self):
        self._check_queue_entry_statuses(
                self.queue_entries,
                allowed_hqe_statuses=(models.HostQueueEntry.Status.STARTING,
                                      models.HostQueueEntry.Status.RUNNING),
                allowed_host_statuses=(models.Host.Status.PENDING,
                                       models.Host.Status.RUNNING))

        super(QueueTask, self).prolog()

        for queue_entry in self.queue_entries:
            self._write_host_keyvals(queue_entry.host)
            queue_entry.host.set_status(models.Host.Status.RUNNING)
            queue_entry.host.update_field('dirty', 1)


    def _finish_task(self):
        super(QueueTask, self)._finish_task()

        for queue_entry in self.queue_entries:
            queue_entry.set_status(models.HostQueueEntry.Status.GATHERING)
            queue_entry.host.set_status(models.Host.Status.RUNNING)


    def _command_line(self):
        invocation = super(QueueTask, self)._command_line()
        # Check if server-side packaging is needed.
        if (self._enable_ssp_container and
            self.job.control_type == control_data.CONTROL_TYPE.SERVER and
            self.job.require_ssp != False):
            invocation += ['--require-ssp']
            keyval_dict = self.job.keyval_dict()
            test_source_build = keyval_dict.get('test_source_build', None)
            if test_source_build:
                invocation += ['--test_source_build', test_source_build]
        if self.job.parent_job_id:
            invocation += ['--parent_job_id', str(self.job.parent_job_id)]
        return invocation + ['--verify_job_repo_url']


class HostlessQueueTask(AbstractQueueTask):
    def __init__(self, queue_entry):
        super(HostlessQueueTask, self).__init__([queue_entry])
        self.queue_entry_ids = [queue_entry.id]


    def prolog(self):
        super(HostlessQueueTask, self).prolog()


    def _finish_task(self):
        super(HostlessQueueTask, self)._finish_task()

        # When a job is added to database, its initial status is always
        # Starting. In a scheduler tick, scheduler finds all jobs in Starting
        # status, check if any of them can be started. If scheduler hits some
        # limit, e.g., max_hostless_jobs_per_drone, scheduler will
        # leave these jobs in Starting status. Otherwise, the jobs'
        # status will be changed to Running, and an autoserv process
        # will be started in drone for each of these jobs.
        # If the entry is still in status Starting, the process has not started
        # yet. Therefore, there is no need to parse and collect log. Without
        # this check, exception will be raised by scheduler as execution_subdir
        # for this queue entry does not have a value yet.
        hqe = self.queue_entries[0]
        if hqe.status != models.HostQueueEntry.Status.STARTING:
            hqe.set_status(models.HostQueueEntry.Status.PARSING)


if __name__ == '__main__':
    main()
