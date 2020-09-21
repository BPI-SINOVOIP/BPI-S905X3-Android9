#!/usr/bin/python

import gc
import time
import unittest

import common
from autotest_lib.frontend import setup_django_environment
from autotest_lib.frontend.afe import frontend_test_utils
from autotest_lib.client.common_lib import global_config
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.database import database_connection
from autotest_lib.frontend.afe import models
from autotest_lib.scheduler import agent_task
from autotest_lib.scheduler import monitor_db, drone_manager
from autotest_lib.scheduler import pidfile_monitor
from autotest_lib.scheduler import scheduler_config, gc_stats
from autotest_lib.scheduler import scheduler_lib
from autotest_lib.scheduler import scheduler_models

_DEBUG = False


class DummyAgentTask(object):
    num_processes = 1
    owner_username = 'my_user'

    def get_drone_hostnames_allowed(self):
        return None


class DummyAgent(object):
    started = False
    _is_done = False
    host_ids = ()
    hostnames = {}
    queue_entry_ids = ()

    def __init__(self):
        self.task = DummyAgentTask()


    def tick(self):
        self.started = True


    def is_done(self):
        return self._is_done


    def set_done(self, done):
        self._is_done = done


class IsRow(mock.argument_comparator):
    def __init__(self, row_id):
        self.row_id = row_id


    def is_satisfied_by(self, parameter):
        return list(parameter)[0] == self.row_id


    def __str__(self):
        return 'row with id %s' % self.row_id


class IsAgentWithTask(mock.argument_comparator):
    def __init__(self, task):
        self._task = task


    def is_satisfied_by(self, parameter):
        if not isinstance(parameter, monitor_db.Agent):
            return False
        tasks = list(parameter.queue.queue)
        if len(tasks) != 1:
            return False
        return tasks[0] == self._task


def _set_host_and_qe_ids(agent_or_task, id_list=None):
    if id_list is None:
        id_list = []
    agent_or_task.host_ids = agent_or_task.queue_entry_ids = id_list
    agent_or_task.hostnames = dict((host_id, '192.168.1.1')
                                   for host_id in id_list)


class BaseSchedulerTest(unittest.TestCase,
                        frontend_test_utils.FrontendTestMixin):
    _config_section = 'AUTOTEST_WEB'

    def _do_query(self, sql):
        self._database.execute(sql)


    def _set_monitor_stubs(self):
        self.mock_config = global_config.FakeGlobalConfig()
        self.god.stub_with(global_config, 'global_config', self.mock_config)

        # Clear the instance cache as this is a brand new database.
        scheduler_models.DBObject._clear_instance_cache()

        self._database = (
            database_connection.TranslatingDatabase.get_test_database(
                translators=scheduler_lib._DB_TRANSLATORS))
        self._database.connect(db_type='django')
        self._database.debug = _DEBUG

        connection_manager = scheduler_lib.ConnectionManager(autocommit=False)
        self.god.stub_with(connection_manager, 'db_connection', self._database)
        self.god.stub_with(monitor_db, '_db_manager', connection_manager)
        self.god.stub_with(monitor_db, '_db', self._database)

        self.god.stub_with(monitor_db.Dispatcher,
                           '_get_pending_queue_entries',
                           self._get_pending_hqes)
        self.god.stub_with(scheduler_models, '_db', self._database)
        self.god.stub_with(drone_manager.instance(), '_results_dir',
                           '/test/path')
        self.god.stub_with(drone_manager.instance(), '_temporary_directory',
                           '/test/path/tmp')
        self.god.stub_with(drone_manager.instance(), 'initialize',
                           lambda *args: None)
        self.god.stub_with(drone_manager.instance(), 'execute_actions',
                           lambda *args: None)

        monitor_db.initialize_globals()
        scheduler_models.initialize_globals()


    def setUp(self):
        self._frontend_common_setup()
        self._set_monitor_stubs()
        self._set_global_config_values()
        self._dispatcher = monitor_db.Dispatcher()


    def tearDown(self):
        self._database.disconnect()
        self._frontend_common_teardown()


    def _set_global_config_values(self):
        """Set global_config values to suit unittest needs."""
        self.mock_config.set_config_value(
                'SCHEDULER', 'inline_host_acquisition', True)


    def _update_hqe(self, set, where=''):
        query = 'UPDATE afe_host_queue_entries SET ' + set
        if where:
            query += ' WHERE ' + where
        self._do_query(query)


    def _get_pending_hqes(self):
        query_string=('afe_jobs.priority DESC, '
                      'ifnull(nullif(host_id, NULL), host_id) DESC, '
                      'ifnull(nullif(meta_host, NULL), meta_host) DESC, '
                      'job_id')
        return list(scheduler_models.HostQueueEntry.fetch(
            joins='INNER JOIN afe_jobs ON (job_id=afe_jobs.id)',
            where='NOT complete AND NOT active AND status="Queued"',
            order_by=query_string))


class DispatcherSchedulingTest(BaseSchedulerTest):
    _jobs_scheduled = []


    def tearDown(self):
        super(DispatcherSchedulingTest, self).tearDown()


    def _set_monitor_stubs(self):
        super(DispatcherSchedulingTest, self)._set_monitor_stubs()

        def hqe__do_schedule_pre_job_tasks_stub(queue_entry):
            """Called by HostQueueEntry.run()."""
            self._record_job_scheduled(queue_entry.job.id, queue_entry.host.id)
            queue_entry.set_status('Starting')

        self.god.stub_with(scheduler_models.HostQueueEntry,
                           '_do_schedule_pre_job_tasks',
                           hqe__do_schedule_pre_job_tasks_stub)


    def _record_job_scheduled(self, job_id, host_id):
        record = (job_id, host_id)
        self.assert_(record not in self._jobs_scheduled,
                     'Job %d scheduled on host %d twice' %
                     (job_id, host_id))
        self._jobs_scheduled.append(record)


    def _assert_job_scheduled_on(self, job_id, host_id):
        record = (job_id, host_id)
        self.assert_(record in self._jobs_scheduled,
                     'Job %d not scheduled on host %d as expected\n'
                     'Jobs scheduled: %s' %
                     (job_id, host_id, self._jobs_scheduled))
        self._jobs_scheduled.remove(record)


    def _assert_job_scheduled_on_number_of(self, job_id, host_ids, number):
        """Assert job was scheduled on exactly number hosts out of a set."""
        found = []
        for host_id in host_ids:
            record = (job_id, host_id)
            if record in self._jobs_scheduled:
                found.append(record)
                self._jobs_scheduled.remove(record)
        if len(found) < number:
            self.fail('Job %d scheduled on fewer than %d hosts in %s.\n'
                      'Jobs scheduled: %s' % (job_id, number, host_ids, found))
        elif len(found) > number:
            self.fail('Job %d scheduled on more than %d hosts in %s.\n'
                      'Jobs scheduled: %s' % (job_id, number, host_ids, found))


    def _check_for_extra_schedulings(self):
        if len(self._jobs_scheduled) != 0:
            self.fail('Extra jobs scheduled: ' +
                      str(self._jobs_scheduled))


    def _convert_jobs_to_metahosts(self, *job_ids):
        sql_tuple = '(' + ','.join(str(i) for i in job_ids) + ')'
        self._do_query('UPDATE afe_host_queue_entries SET '
                       'meta_host=host_id, host_id=NULL '
                       'WHERE job_id IN ' + sql_tuple)


    def _lock_host(self, host_id):
        self._do_query('UPDATE afe_hosts SET locked=1 WHERE id=' +
                       str(host_id))


    def setUp(self):
        super(DispatcherSchedulingTest, self).setUp()
        self._jobs_scheduled = []


    def _run_scheduler(self):
        self._dispatcher._host_scheduler.tick()
        for _ in xrange(2): # metahost scheduling can take two ticks
            self._dispatcher._schedule_new_jobs()


    def _test_basic_scheduling_helper(self, use_metahosts):
        'Basic nonmetahost scheduling'
        self._create_job_simple([1], use_metahosts)
        self._create_job_simple([2], use_metahosts)
        self._run_scheduler()
        self._assert_job_scheduled_on(1, 1)
        self._assert_job_scheduled_on(2, 2)
        self._check_for_extra_schedulings()


    def _test_priorities_helper(self, use_metahosts):
        'Test prioritization ordering'
        self._create_job_simple([1], use_metahosts)
        self._create_job_simple([2], use_metahosts)
        self._create_job_simple([1,2], use_metahosts)
        self._create_job_simple([1], use_metahosts, priority=1)
        self._run_scheduler()
        self._assert_job_scheduled_on(4, 1) # higher priority
        self._assert_job_scheduled_on(2, 2) # earlier job over later
        self._check_for_extra_schedulings()


    def _test_hosts_ready_helper(self, use_metahosts):
        """
        Only hosts that are status=Ready, unlocked and not invalid get
        scheduled.
        """
        self._create_job_simple([1], use_metahosts)
        self._do_query('UPDATE afe_hosts SET status="Running" WHERE id=1')
        self._run_scheduler()
        self._check_for_extra_schedulings()

        self._do_query('UPDATE afe_hosts SET status="Ready", locked=1 '
                       'WHERE id=1')
        self._run_scheduler()
        self._check_for_extra_schedulings()

        self._do_query('UPDATE afe_hosts SET locked=0, invalid=1 '
                       'WHERE id=1')
        self._run_scheduler()
        if not use_metahosts:
            self._assert_job_scheduled_on(1, 1)
        self._check_for_extra_schedulings()


    def _test_hosts_idle_helper(self, use_metahosts):
        'Only idle hosts get scheduled'
        self._create_job(hosts=[1], active=True)
        self._create_job_simple([1], use_metahosts)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def _test_obey_ACLs_helper(self, use_metahosts):
        self._do_query('DELETE FROM afe_acl_groups_hosts WHERE host_id=1')
        self._create_job_simple([1], use_metahosts)
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_basic_scheduling(self):
        self._test_basic_scheduling_helper(False)


    def test_priorities(self):
        self._test_priorities_helper(False)


    def test_hosts_ready(self):
        self._test_hosts_ready_helper(False)


    def test_hosts_idle(self):
        self._test_hosts_idle_helper(False)


    def test_obey_ACLs(self):
        self._test_obey_ACLs_helper(False)


    def test_one_time_hosts_ignore_ACLs(self):
        self._do_query('DELETE FROM afe_acl_groups_hosts WHERE host_id=1')
        self._do_query('UPDATE afe_hosts SET invalid=1 WHERE id=1')
        self._create_job_simple([1])
        self._run_scheduler()
        self._assert_job_scheduled_on(1, 1)
        self._check_for_extra_schedulings()


    def test_non_metahost_on_invalid_host(self):
        """
        Non-metahost entries can get scheduled on invalid hosts (this is how
        one-time hosts work).
        """
        self._do_query('UPDATE afe_hosts SET invalid=1')
        self._test_basic_scheduling_helper(False)


    def test_metahost_scheduling(self):
        """
        Basic metahost scheduling
        """
        self._test_basic_scheduling_helper(True)


    def test_metahost_priorities(self):
        self._test_priorities_helper(True)


    def test_metahost_hosts_ready(self):
        self._test_hosts_ready_helper(True)


    def test_metahost_hosts_idle(self):
        self._test_hosts_idle_helper(True)


    def test_metahost_obey_ACLs(self):
        self._test_obey_ACLs_helper(True)


    def test_nonmetahost_over_metahost(self):
        """
        Non-metahost entries should take priority over metahost entries
        for the same host
        """
        self._create_job(metahosts=[1])
        self._create_job(hosts=[1])
        self._run_scheduler()
        self._assert_job_scheduled_on(2, 1)
        self._check_for_extra_schedulings()


    def test_no_execution_subdir_not_found(self):
        """Reproduce bug crosbug.com/334353 and recover from it."""

        self.mock_config.set_config_value('SCHEDULER', 'drones', 'localhost')

        job = self._create_job(hostless=True)

        # Ensure execution_subdir is set before status
        original_set_status = scheduler_models.HostQueueEntry.set_status
        def fake_set_status(hqe, *args, **kwargs):
            self.assertEqual(hqe.execution_subdir, 'hostless')
            original_set_status(hqe, *args, **kwargs)

        self.god.stub_with(scheduler_models.HostQueueEntry, 'set_status',
                           fake_set_status)

        self._dispatcher._schedule_new_jobs()

        hqe = job.hostqueueentry_set.all()[0]
        self.assertEqual(models.HostQueueEntry.Status.STARTING, hqe.status)
        self.assertEqual('hostless', hqe.execution_subdir)


    def test_only_schedule_queued_entries(self):
        self._create_job(metahosts=[1])
        self._update_hqe(set='active=1, host_id=2')
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_no_ready_hosts(self):
        self._create_job(hosts=[1])
        self._do_query('UPDATE afe_hosts SET status="Repair Failed"')
        self._run_scheduler()
        self._check_for_extra_schedulings()


    def test_garbage_collection(self):
        self.god.stub_with(self._dispatcher, '_seconds_between_garbage_stats',
                           999999)
        self.god.stub_function(gc, 'collect')
        self.god.stub_function(gc_stats, '_log_garbage_collector_stats')
        gc.collect.expect_call().and_return(0)
        gc_stats._log_garbage_collector_stats.expect_call()
        # Force a garbage collection run
        self._dispatcher._last_garbage_stats_time = 0
        self._dispatcher._garbage_collection()
        # The previous call should have reset the time, it won't do anything
        # the second time.  If it does, we'll get an unexpected call.
        self._dispatcher._garbage_collection()


class DispatcherThrottlingTest(BaseSchedulerTest):
    """
    Test that the dispatcher throttles:
     * total number of running processes
     * number of processes started per cycle
    """
    _MAX_RUNNING = 3
    _MAX_STARTED = 2

    def setUp(self):
        super(DispatcherThrottlingTest, self).setUp()
        scheduler_config.config.max_processes_per_drone = self._MAX_RUNNING

        def fake_max_runnable_processes(fake_self, username,
                                        drone_hostnames_allowed):
            running = sum(agent.task.num_processes
                          for agent in self._agents
                          if agent.started and not agent.is_done())
            return self._MAX_RUNNING - running
        self.god.stub_with(drone_manager.DroneManager, 'max_runnable_processes',
                           fake_max_runnable_processes)


    def _setup_some_agents(self, num_agents):
        self._agents = [DummyAgent() for i in xrange(num_agents)]
        self._dispatcher._agents = list(self._agents)


    def _run_a_few_ticks(self):
        for i in xrange(4):
            self._dispatcher._handle_agents()


    def _assert_agents_started(self, indexes, is_started=True):
        for i in indexes:
            self.assert_(self._agents[i].started == is_started,
                         'Agent %d %sstarted' %
                         (i, is_started and 'not ' or ''))


    def _assert_agents_not_started(self, indexes):
        self._assert_agents_started(indexes, False)


    def test_throttle_total(self):
        self._setup_some_agents(4)
        self._run_a_few_ticks()
        self._assert_agents_started([0, 1, 2])
        self._assert_agents_not_started([3])


    def test_throttle_with_synchronous(self):
        self._setup_some_agents(2)
        self._agents[0].task.num_processes = 3
        self._run_a_few_ticks()
        self._assert_agents_started([0])
        self._assert_agents_not_started([1])


    def test_large_agent_starvation(self):
        """
        Ensure large agents don't get starved by lower-priority agents.
        """
        self._setup_some_agents(3)
        self._agents[1].task.num_processes = 3
        self._run_a_few_ticks()
        self._assert_agents_started([0])
        self._assert_agents_not_started([1, 2])

        self._agents[0].set_done(True)
        self._run_a_few_ticks()
        self._assert_agents_started([1])
        self._assert_agents_not_started([2])


    def test_zero_process_agent(self):
        self._setup_some_agents(5)
        self._agents[4].task.num_processes = 0
        self._run_a_few_ticks()
        self._assert_agents_started([0, 1, 2, 4])
        self._assert_agents_not_started([3])


class PidfileRunMonitorTest(unittest.TestCase):
    execution_tag = 'test_tag'
    pid = 12345
    process = drone_manager.Process('myhost', pid)
    num_tests_failed = 1

    def setUp(self):
        self.god = mock.mock_god()
        self.mock_drone_manager = self.god.create_mock_class(
            drone_manager.DroneManager, 'drone_manager')
        self.god.stub_with(drone_manager, '_the_instance',
                           self.mock_drone_manager)
        self.god.stub_with(pidfile_monitor, '_get_pidfile_timeout_secs',
                           self._mock_get_pidfile_timeout_secs)

        self.pidfile_id = object()

        (self.mock_drone_manager.get_pidfile_id_from
             .expect_call(self.execution_tag,
                          pidfile_name=drone_manager.AUTOSERV_PID_FILE)
             .and_return(self.pidfile_id))

        self.monitor = pidfile_monitor.PidfileRunMonitor()
        self.monitor.attach_to_existing_process(self.execution_tag)

    def tearDown(self):
        self.god.unstub_all()


    def _mock_get_pidfile_timeout_secs(self):
        return 300


    def setup_pidfile(self, pid=None, exit_code=None, tests_failed=None,
                      use_second_read=False):
        contents = drone_manager.PidfileContents()
        if pid is not None:
            contents.process = drone_manager.Process('myhost', pid)
        contents.exit_status = exit_code
        contents.num_tests_failed = tests_failed
        self.mock_drone_manager.get_pidfile_contents.expect_call(
            self.pidfile_id, use_second_read=use_second_read).and_return(
            contents)


    def set_not_yet_run(self):
        self.setup_pidfile()


    def set_empty_pidfile(self):
        self.setup_pidfile()


    def set_running(self, use_second_read=False):
        self.setup_pidfile(self.pid, use_second_read=use_second_read)


    def set_complete(self, error_code, use_second_read=False):
        self.setup_pidfile(self.pid, error_code, self.num_tests_failed,
                           use_second_read=use_second_read)


    def _check_monitor(self, expected_pid, expected_exit_status,
                       expected_num_tests_failed):
        if expected_pid is None:
            self.assertEquals(self.monitor._state.process, None)
        else:
            self.assertEquals(self.monitor._state.process.pid, expected_pid)
        self.assertEquals(self.monitor._state.exit_status, expected_exit_status)
        self.assertEquals(self.monitor._state.num_tests_failed,
                          expected_num_tests_failed)


        self.god.check_playback()


    def _test_read_pidfile_helper(self, expected_pid, expected_exit_status,
                                  expected_num_tests_failed):
        self.monitor._read_pidfile()
        self._check_monitor(expected_pid, expected_exit_status,
                            expected_num_tests_failed)


    def _get_expected_tests_failed(self, expected_exit_status):
        if expected_exit_status is None:
            expected_tests_failed = None
        else:
            expected_tests_failed = self.num_tests_failed
        return expected_tests_failed


    def test_read_pidfile(self):
        self.set_not_yet_run()
        self._test_read_pidfile_helper(None, None, None)

        self.set_empty_pidfile()
        self._test_read_pidfile_helper(None, None, None)

        self.set_running()
        self._test_read_pidfile_helper(self.pid, None, None)

        self.set_complete(123)
        self._test_read_pidfile_helper(self.pid, 123, self.num_tests_failed)


    def test_read_pidfile_error(self):
        self.mock_drone_manager.get_pidfile_contents.expect_call(
            self.pidfile_id, use_second_read=False).and_return(
            drone_manager.InvalidPidfile('error'))
        self.assertRaises(pidfile_monitor.PidfileRunMonitor._PidfileException,
                          self.monitor._read_pidfile)
        self.god.check_playback()


    def setup_is_running(self, is_running):
        self.mock_drone_manager.is_process_running.expect_call(
            self.process).and_return(is_running)


    def _test_get_pidfile_info_helper(self, expected_pid, expected_exit_status,
                                      expected_num_tests_failed):
        self.monitor._get_pidfile_info()
        self._check_monitor(expected_pid, expected_exit_status,
                            expected_num_tests_failed)


    def test_get_pidfile_info(self):
        """
        normal cases for get_pidfile_info
        """
        # running
        self.set_running()
        self.setup_is_running(True)
        self._test_get_pidfile_info_helper(self.pid, None, None)

        # exited during check
        self.set_running()
        self.setup_is_running(False)
        self.set_complete(123, use_second_read=True) # pidfile gets read again
        self._test_get_pidfile_info_helper(self.pid, 123, self.num_tests_failed)

        # completed
        self.set_complete(123)
        self._test_get_pidfile_info_helper(self.pid, 123, self.num_tests_failed)


    def test_get_pidfile_info_running_no_proc(self):
        """
        pidfile shows process running, but no proc exists
        """
        # running but no proc
        self.set_running()
        self.setup_is_running(False)
        self.set_running(use_second_read=True)
        self._test_get_pidfile_info_helper(self.pid, 1, 0)
        self.assertTrue(self.monitor.lost_process)


    def test_get_pidfile_info_not_yet_run(self):
        """
        pidfile hasn't been written yet
        """
        self.set_not_yet_run()
        self._test_get_pidfile_info_helper(None, None, None)


    def test_process_failed_to_write_pidfile(self):
        self.set_not_yet_run()
        self.monitor._start_time = (time.time() -
                                    pidfile_monitor._get_pidfile_timeout_secs() - 1)
        self._test_get_pidfile_info_helper(None, 1, 0)
        self.assertTrue(self.monitor.lost_process)


class AgentTest(unittest.TestCase):
    def setUp(self):
        self.god = mock.mock_god()
        self._dispatcher = self.god.create_mock_class(monitor_db.Dispatcher,
                                                      'dispatcher')


    def tearDown(self):
        self.god.unstub_all()


    def _create_mock_task(self, name):
        task = self.god.create_mock_class(agent_task.AgentTask, name)
        task.num_processes = 1
        _set_host_and_qe_ids(task)
        return task

    def _create_agent(self, task):
        agent = monitor_db.Agent(task)
        agent.dispatcher = self._dispatcher
        return agent


    def _finish_agent(self, agent):
        while not agent.is_done():
            agent.tick()


    def test_agent_abort(self):
        task = self._create_mock_task('task')
        task.poll.expect_call()
        task.is_done.expect_call().and_return(False)
        task.abort.expect_call()
        task.aborted = True

        agent = self._create_agent(task)
        agent.tick()
        agent.abort()
        self._finish_agent(agent)
        self.god.check_playback()


    def _test_agent_abort_before_started_helper(self, ignore_abort=False):
        task = self._create_mock_task('task')
        task.abort.expect_call()
        if ignore_abort:
            task.aborted = False
            task.poll.expect_call()
            task.is_done.expect_call().and_return(True)
            task.success = True
        else:
            task.aborted = True

        agent = self._create_agent(task)
        agent.abort()
        self._finish_agent(agent)
        self.god.check_playback()


    def test_agent_abort_before_started(self):
        self._test_agent_abort_before_started_helper()
        self._test_agent_abort_before_started_helper(True)


class JobSchedulingTest(BaseSchedulerTest):
    def _test_run_helper(self, expect_agent=True, expect_starting=False,
                         expect_pending=False):
        if expect_starting:
            expected_status = models.HostQueueEntry.Status.STARTING
        elif expect_pending:
            expected_status = models.HostQueueEntry.Status.PENDING
        else:
            expected_status = models.HostQueueEntry.Status.VERIFYING
        job = scheduler_models.Job.fetch('id = 1')[0]
        queue_entry = scheduler_models.HostQueueEntry.fetch('id = 1')[0]
        assert queue_entry.job is job
        job.run_if_ready(queue_entry)

        self.god.check_playback()

        self._dispatcher._schedule_running_host_queue_entries()
        agent = self._dispatcher._agents[0]

        actual_status = models.HostQueueEntry.smart_get(1).status
        self.assertEquals(expected_status, actual_status)

        if not expect_agent:
            self.assertEquals(agent, None)
            return

        self.assert_(isinstance(agent, monitor_db.Agent))
        self.assert_(agent.task)
        return agent.task


    def test_run_synchronous_ready(self):
        self._create_job(hosts=[1, 2], synchronous=True)
        self._update_hqe("status='Pending', execution_subdir=''")

        queue_task = self._test_run_helper(expect_starting=True)

        self.assert_(isinstance(queue_task, monitor_db.QueueTask))
        self.assertEquals(queue_task.job.id, 1)
        hqe_ids = [hqe.id for hqe in queue_task.queue_entries]
        self.assertEquals(hqe_ids, [1, 2])


    def test_schedule_running_host_queue_entries_fail(self):
        self._create_job(hosts=[2])
        self._update_hqe("status='%s', execution_subdir=''" %
                         models.HostQueueEntry.Status.PENDING)
        job = scheduler_models.Job.fetch('id = 1')[0]
        queue_entry = scheduler_models.HostQueueEntry.fetch('id = 1')[0]
        assert queue_entry.job is job
        job.run_if_ready(queue_entry)
        self.assertEqual(queue_entry.status,
                         models.HostQueueEntry.Status.STARTING)
        self.assert_(queue_entry.execution_subdir)
        self.god.check_playback()

        class dummy_test_agent(object):
            task = 'dummy_test_agent'
        self._dispatcher._register_agent_for_ids(
                self._dispatcher._host_agents, [queue_entry.host.id],
                dummy_test_agent)

        # Attempted to schedule on a host that already has an agent.
        # Verify that it doesn't raise any error.
        self._dispatcher._schedule_running_host_queue_entries()


    def test_schedule_hostless_job(self):
        job = self._create_job(hostless=True)
        self.assertEqual(1, job.hostqueueentry_set.count())
        hqe_query = scheduler_models.HostQueueEntry.fetch(
                'id = %s' % job.hostqueueentry_set.all()[0].id)
        self.assertEqual(1, len(hqe_query))
        hqe = hqe_query[0]

        self.assertEqual(models.HostQueueEntry.Status.QUEUED, hqe.status)
        self.assertEqual(0, len(self._dispatcher._agents))

        self._dispatcher._schedule_new_jobs()

        self.assertEqual(models.HostQueueEntry.Status.STARTING, hqe.status)
        self.assertEqual(1, len(self._dispatcher._agents))

        self._dispatcher._schedule_new_jobs()

        # No change to previously schedule hostless job, and no additional agent
        self.assertEqual(models.HostQueueEntry.Status.STARTING, hqe.status)
        self.assertEqual(1, len(self._dispatcher._agents))


class TopLevelFunctionsTest(unittest.TestCase):
    def setUp(self):
        self.god = mock.mock_god()


    def tearDown(self):
        self.god.unstub_all()


    def test_autoserv_command_line(self):
        machines = 'abcd12,efgh34'
        extra_args = ['-Z', 'hello']
        expected_command_line_base = set((monitor_db._autoserv_path, '-p',
                                          '-m', machines, '-r',
                                          '--lab', 'True',
                                          drone_manager.WORKING_DIRECTORY))

        expected_command_line = expected_command_line_base.union(
                ['--verbose']).union(extra_args)
        command_line = set(
                monitor_db._autoserv_command_line(machines, extra_args))
        self.assertEqual(expected_command_line, command_line)

        class FakeJob(object):
            owner = 'Bob'
            name = 'fake job name'
            test_retry = 0
            id = 1337

        class FakeHQE(object):
            job = FakeJob

        expected_command_line = expected_command_line_base.union(
                ['-u', FakeJob.owner, '-l', FakeJob.name])
        command_line = set(monitor_db._autoserv_command_line(
                machines, extra_args=[], queue_entry=FakeHQE, verbose=False))
        self.assertEqual(expected_command_line, command_line)


class AgentTaskTest(unittest.TestCase,
                    frontend_test_utils.FrontendTestMixin):
    def setUp(self):
        self._frontend_common_setup()


    def tearDown(self):
        self._frontend_common_teardown()


    def _setup_drones(self):
        self.god.stub_function(models.DroneSet, 'drone_sets_enabled')
        models.DroneSet.drone_sets_enabled.expect_call().and_return(True)

        drones = []
        for x in xrange(4):
            drones.append(models.Drone.objects.create(hostname=str(x)))

        drone_set_1 = models.DroneSet.objects.create(name='1')
        drone_set_1.drones.add(*drones[0:2])
        drone_set_2 = models.DroneSet.objects.create(name='2')
        drone_set_2.drones.add(*drones[2:4])
        drone_set_3 = models.DroneSet.objects.create(name='3')

        job_1 = self._create_job_simple([self.hosts[0].id],
                                        drone_set=drone_set_1)
        job_2 = self._create_job_simple([self.hosts[0].id],
                                        drone_set=drone_set_2)
        job_3 = self._create_job_simple([self.hosts[0].id],
                                        drone_set=drone_set_3)

        job_4 = self._create_job_simple([self.hosts[0].id])
        job_4.drone_set = None
        job_4.save()

        hqe_1 = job_1.hostqueueentry_set.all()[0]
        hqe_2 = job_2.hostqueueentry_set.all()[0]
        hqe_3 = job_3.hostqueueentry_set.all()[0]
        hqe_4 = job_4.hostqueueentry_set.all()[0]

        return (hqe_1, hqe_2, hqe_3, hqe_4), agent_task.AgentTask()


    def test_get_drone_hostnames_allowed_no_drones_in_set(self):
        hqes, task = self._setup_drones()
        task.queue_entry_ids = (hqes[2].id,)
        self.assertEqual(set(), task.get_drone_hostnames_allowed())
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_no_drone_set(self):
        hqes, task = self._setup_drones()
        hqe = hqes[3]
        task.queue_entry_ids = (hqe.id,)

        result = object()

        self.god.stub_function(task, '_user_or_global_default_drone_set')
        task._user_or_global_default_drone_set.expect_call(
                hqe.job, hqe.job.user()).and_return(result)

        self.assertEqual(result, task.get_drone_hostnames_allowed())
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_success(self):
        hqes, task = self._setup_drones()
        task.queue_entry_ids = (hqes[0].id,)
        self.assertEqual(set(('0','1')), task.get_drone_hostnames_allowed([]))
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_multiple_jobs(self):
        hqes, task = self._setup_drones()
        task.queue_entry_ids = (hqes[0].id, hqes[1].id)
        self.assertRaises(AssertionError,
                          task.get_drone_hostnames_allowed)
        self.god.check_playback()


    def test_get_drone_hostnames_allowed_no_hqe(self):
        class MockSpecialTask(object):
            requested_by = object()

        class MockSpecialAgentTask(agent_task.SpecialAgentTask):
            task = MockSpecialTask()
            queue_entry_ids = []
            def __init__(self, *args, **kwargs):
                super(agent_task.SpecialAgentTask, self).__init__()

        task = MockSpecialAgentTask()
        self.god.stub_function(models.DroneSet, 'drone_sets_enabled')
        self.god.stub_function(task, '_user_or_global_default_drone_set')

        result = object()
        models.DroneSet.drone_sets_enabled.expect_call().and_return(True)
        task._user_or_global_default_drone_set.expect_call(
                task.task, MockSpecialTask.requested_by).and_return(result)

        self.assertEqual(result, task.get_drone_hostnames_allowed())
        self.god.check_playback()


    def _setup_test_user_or_global_default_drone_set(self):
        result = object()
        class MockDroneSet(object):
            def get_drone_hostnames(self):
                return result

        self.god.stub_function(models.DroneSet, 'get_default')
        models.DroneSet.get_default.expect_call().and_return(MockDroneSet())
        return result


    def test_user_or_global_default_drone_set(self):
        expected = object()
        class MockDroneSet(object):
            def get_drone_hostnames(self):
                return expected
        class MockUser(object):
            drone_set = MockDroneSet()

        self._setup_test_user_or_global_default_drone_set()

        actual = agent_task.AgentTask()._user_or_global_default_drone_set(
                None, MockUser())

        self.assertEqual(expected, actual)
        self.god.check_playback()


    def test_user_or_global_default_drone_set_no_user(self):
        expected = self._setup_test_user_or_global_default_drone_set()
        actual = agent_task.AgentTask()._user_or_global_default_drone_set(
                None, None)

        self.assertEqual(expected, actual)
        self.god.check_playback()


    def test_user_or_global_default_drone_set_no_user_drone_set(self):
        class MockUser(object):
            drone_set = None
            login = None

        expected = self._setup_test_user_or_global_default_drone_set()
        actual = agent_task.AgentTask()._user_or_global_default_drone_set(
                None, MockUser())

        self.assertEqual(expected, actual)
        self.god.check_playback()


    def test_abort_HostlessQueueTask(self):
        hqe = self.god.create_mock_class(scheduler_models.HostQueueEntry,
                                         'HostQueueEntry')
        # If hqe is still in STARTING status, aborting the task should finish
        # without changing hqe's status.
        hqe.status = models.HostQueueEntry.Status.STARTING
        hqe.job = None
        hqe.id = 0
        task = monitor_db.HostlessQueueTask(hqe)
        task.abort()

        # If hqe is in RUNNING status, aborting the task should change hqe's
        # status to Parsing, so FinalReparseTask can be scheduled.
        hqe.set_status.expect_call('Parsing')
        hqe.status = models.HostQueueEntry.Status.RUNNING
        hqe.job = None
        hqe.id = 0
        task = monitor_db.HostlessQueueTask(hqe)
        task.abort()


if __name__ == '__main__':
    unittest.main()
