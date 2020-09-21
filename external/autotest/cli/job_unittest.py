#!/usr/bin/python -u
#
# Copyright 2008 Google Inc. All Rights Reserved.


"""Tests for job."""

# pylint: disable=missing-docstring

import copy, getpass, unittest, sys

import common
from autotest_lib.cli import cli_mock, job
from autotest_lib.client.common_lib.test_utils import mock
from autotest_lib.client.common_lib import control_data
from autotest_lib.client.common_lib import priorities

CLIENT = control_data.CONTROL_TYPE_NAMES.CLIENT
SERVER = control_data.CONTROL_TYPE_NAMES.SERVER

class job_unittest(cli_mock.cli_unittest):
    def setUp(self):
        super(job_unittest, self).setUp()
        self.values = copy.deepcopy(self.values_template)

    results = [{u'status_counts': {u'Aborted': 1},
                u'control_file':
                u"job.run_test('sleeptest')\n",
                u'name': u'test_job0',
                u'control_type': SERVER,
                u'priority':
                priorities.Priority.DEFAULT,
                u'owner': u'user0',
                u'created_on':
                u'2008-07-08 17:45:44',
                u'synch_count': 2,
                u'id': 180},
               {u'status_counts': {u'Queued': 1},
                u'control_file':
                u"job.run_test('sleeptest')\n",
                u'name': u'test_job1',
                u'control_type': CLIENT,
                u'priority':
                priorities.Priority.DEFAULT,
                u'owner': u'user0',
                u'created_on':
                u'2008-07-08 12:17:47',
                u'synch_count': 1,
                u'id': 338}]


    values_template = [{u'id': 180,          # Valid job
                        u'priority': priorities.Priority.DEFAULT,
                        u'name': u'test_job0',
                        u'owner': u'Cringer',
                        u'invalid': False,
                        u'created_on': u'2008-07-02 13:02:40',
                        u'control_type': SERVER,
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 2},
                       {u'id': 338,          # Valid job
                        u'priority': priorities.Priority.DEFAULT,
                        u'name': u'test_job1',
                        u'owner': u'Fisto',
                        u'invalid': False,
                        u'created_on': u'2008-07-06 14:05:33',
                        u'control_type': CLIENT,
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 1},
                       {u'id': 339,          # Valid job
                        u'priority': priorities.Priority.DEFAULT,
                        u'name': u'test_job2',
                        u'owner': u'Roboto',
                        u'invalid': False,
                        u'created_on': u'2008-07-07 15:33:18',
                        u'control_type': SERVER,
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 1},
                       {u'id': 340,          # Invalid job priority
                        u'priority': priorities.Priority.DEFAULT,
                        u'name': u'test_job3',
                        u'owner': u'Panthor',
                        u'invalid': True,
                        u'created_on': u'2008-07-04 00:00:01',
                        u'control_type': SERVER,
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 2},
                       {u'id': 350,          # Invalid job created_on
                        u'priority': priorities.Priority.DEFAULT,
                        u'name': u'test_job4',
                        u'owner': u'Icer',
                        u'invalid': True,
                        u'created_on': u'Today',
                        u'control_type': CLIENT,
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 1},
                       {u'id': 420,          # Invalid job control_type
                        u'priority': 'Urgent',
                        u'name': u'test_job5',
                        u'owner': u'Spikor',
                        u'invalid': True,
                        u'created_on': u'2012-08-08 18:54:37',
                        u'control_type': u'Child',
                        u'status_counts': {u'Queued': 1},
                        u'synch_count': 2}]


class job_list_unittest(job_unittest):
    def test_job_list_jobs(self):
        self.god.stub_function(getpass, 'getuser')
        getpass.getuser.expect_call().and_return('user0')
        self.run_cmd(argv=['atest', 'job', 'list'],
                     rpcs=[('get_jobs_summary', {'owner': 'user0',
                                                 'running': None},
                            True, self.values)],
                     out_words_ok=['test_job0', 'test_job1', 'test_job2'],
                     out_words_no=['Uber', 'Today', 'Child'])


    def test_job_list_jobs_only_user(self):
        values = [item for item in self.values if item['owner'] == 'Cringer']
        self.run_cmd(argv=['atest', 'job', 'list', '-u', 'Cringer'],
                     rpcs=[('get_jobs_summary', {'owner': 'Cringer',
                                                 'running': None},
                            True, values)],
                     out_words_ok=['Cringer'],
                     out_words_no=['Fisto', 'Roboto', 'Panthor', 'Icer',
                                   'Spikor'])


    def test_job_list_jobs_all(self):
        self.run_cmd(argv=['atest', 'job', 'list', '--all'],
                     rpcs=[('get_jobs_summary', {'running': None},
                            True, self.values)],
                     out_words_ok=['Fisto', 'Roboto', 'Panthor',
                                   'Icer', 'Spikor', 'Cringer'],
                     out_words_no=['Created', 'Priority'])


    def test_job_list_jobs_id(self):
        self.run_cmd(argv=['atest', 'job', 'list', '5964'],
                     rpcs=[('get_jobs_summary', {'id__in': ['5964'],
                                                 'running': None},
                            True,
                            [{u'status_counts': {u'Completed': 1},
                              u'control_file': u'kernel = \'8210088647656509311.kernel-smp-2.6.18-220.5.x86_64.rpm\'\ndef step_init():\n    job.next_step([step_test])\n\ndef step_test():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "Autotest Team"\n    NAME = "Sleeptest"\n    TIME = "SHORT"\n    TEST_CATEGORY = "Functional"\n    TEST_CLASS = "General"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    This test simply sleeps for 1 second by default.  It\'s a good way to test\n    profilers and double check that autotest is working.\n    The seconds argument can also be modified to make the machine sleep for as\n    long as needed.\n    """\n    \n    job.run_test(\'sleeptest\',                             seconds = 1)',
                              u'name': u'mytest',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-28 12:42:52',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 5964}])],
                     out_words_ok=['user0', 'Completed', '1', '5964'],
                     out_words_no=['sleeptest', 'Priority', CLIENT, '2008'])


    def test_job_list_jobs_id_verbose(self):
        self.run_cmd(argv=['atest', 'job', 'list', '5964', '-v'],
                     rpcs=[('get_jobs_summary', {'id__in': ['5964'],
                                                 'running': None},
                            True,
                            [{u'status_counts': {u'Completed': 1},
                              u'control_file': u'kernel = \'8210088647656509311.kernel-smp-2.6.18-220.5.x86_64.rpm\'\ndef step_init():\n    job.next_step([step_test])\n\ndef step_test():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "Autotest Team"\n    NAME = "Sleeptest"\n    TIME = "SHORT"\n    TEST_CATEGORY = "Functional"\n    TEST_CLASS = "General"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    This test simply sleeps for 1 second by default.  It\'s a good way to test\n    profilers and double check that autotest is working.\n    The seconds argument can also be modified to make the machine sleep for as\n    long as needed.\n    """\n    \n    job.run_test(\'sleeptest\',                             seconds = 1)',
                              u'name': u'mytest',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-28 12:42:52',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 5964}])],
                     out_words_ok=['user0', 'Completed', '1', '5964',
                                   CLIENT, '2008', 'Priority'],
                     out_words_no=['sleeptest'])


    def test_job_list_jobs_name(self):
        self.run_cmd(argv=['atest', 'job', 'list', 'myt*'],
                     rpcs=[('get_jobs_summary', {'name__startswith': 'myt',
                                                 'running': None},
                            True,
                            [{u'status_counts': {u'Completed': 1},
                              u'control_file': u'kernel = \'8210088647656509311.kernel-smp-2.6.18-220.5.x86_64.rpm\'\ndef step_init():\n    job.next_step([step_test])\n\ndef step_test():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "Autotest Team"\n    NAME = "Sleeptest"\n    TIME = "SHORT"\n    TEST_CATEGORY = "Functional"\n    TEST_CLASS = "General"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    This test simply sleeps for 1 second by default.  It\'s a good way to test\n    profilers and double check that autotest is working.\n    The seconds argument can also be modified to make the machine sleep for as\n    long as needed.\n    """\n    \n    job.run_test(\'sleeptest\',                             seconds = 1)',
                              u'name': u'mytest',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-28 12:42:52',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 5964}])],
                     out_words_ok=['user0', 'Completed', '1', '5964'],
                     out_words_no=['sleeptest', 'Priority', CLIENT, '2008'])


    def test_job_list_jobs_all_verbose(self):
        self.run_cmd(argv=['atest', 'job', 'list', '--all', '--verbose'],
                     rpcs=[('get_jobs_summary', {'running': None},
                            True, self.values)],
                     out_words_ok=['Fisto', 'Spikor', 'Cringer', 'Priority',
                                   'Created'])


class job_list_jobs_all_and_user_unittest(cli_mock.cli_unittest):
    def test_job_list_jobs_all_and_user(self):
        testjob = job.job_list()
        sys.argv = ['atest', 'job', 'list', '-a', '-u', 'user0']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(cli_mock.ExitException))
        self.assertRaises(cli_mock.ExitException, testjob.parse)
        self.god.unmock_io()
        self.god.check_playback()


class job_stat_unittest(job_unittest):
    def test_job_stat_job(self):
        results = copy.deepcopy(self.results)
        self.run_cmd(argv=['atest', 'job', 'stat', '180'],
                     rpcs=[('get_jobs_summary', {'id__in': ['180']}, True,
                            [results[0]]),
                           ('get_host_queue_entries', {'job__in': ['180']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Repair Failed',
                                        u'locked': False,
                                        u'hostname': u'host0',
                                        u'invalid': True,
                                        u'id': 4432},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'synchronizing': 0,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 180},
                              u'active': 0,
                              u'id': 101084}])],
                     out_words_ok=['test_job0', 'host0', 'Failed',
                                   'Aborted'])



    def test_job_stat_list_unassigned_host(self):
        self.run_cmd(argv=['atest', 'job', 'stat', '6761',
                           '--list-hosts'],
                     rpcs=[('get_jobs_summary', {'id__in': ['6761']}, True,
                            [{u'status_counts': {u'Queued': 1},
                              u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                              u'name': u'test_on_meta_hosts',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-30 22:15:43',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 6761}]),
                           ('get_host_queue_entries', {'job__in': ['6761']},
                            True,
                            [{u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': u'Xeon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166} ])],
                     err_words_ok=['unassigned', 'meta-hosts'],
                     out_words_no=['Xeon'])


    def test_job_stat_list_hosts(self):
        self.run_cmd(argv=['atest', 'job', 'stat', '6761',
                           '--list-hosts'],
                     rpcs=[('get_jobs_summary', {'id__in': ['6761']}, True,
                            [{u'status_counts': {u'Queued': 1},
                              u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                              u'name': u'test_on_meta_hosts',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-30 22:15:43',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 6761}]),
                           ('get_host_queue_entries', {'job__in': ['6761']},
                            True,
                            [{u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': {u'status': u'Running',
                                        u'lock_time': None,
                                        u'hostname': u'host41',
                                        u'locked': False,
                                        u'locked_by': None,
                                        u'invalid': False,
                                        u'id': 4833,
                                        u'protection': u'Repair filesystem only'},
                              u'priority': 1,
                              u'meta_host': u'Xeon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166},
                            {u'status': u'Running',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': {u'status': u'Running',
                                        u'lock_time': None,
                                        u'hostname': u'host42',
                                        u'locked': False,
                                        u'locked_by': None,
                                        u'invalid': False,
                                        u'id': 4833,
                                        u'protection': u'Repair filesystem only'},
                              u'priority': 1,
                              u'meta_host': u'Xeon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166} ])],
                     out_words_ok=['host41', 'host42'],
                     out_words_no=['Xeon', 'Running', 'Queued'],
                     err_words_no=['unassigned'])


    def test_job_stat_list_hosts_status(self):
        self.run_cmd(argv=['atest', 'job', 'stat', '6761',
                           '--list-hosts-status', 'Running,Queued'],
                     rpcs=[('get_jobs_summary', {'id__in': ['6761']}, True,
                            [{u'status_counts': {u'Queued': 1, u'Running': 1},
                              u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                              u'name': u'test',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-30 22:15:43',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 6761}]),
                           ('get_host_queue_entries', {'job__in': ['6761']},
                            True,
                            [{u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': {u'status': u'Queued',
                                        u'lock_time': None,
                                        u'hostname': u'host41',
                                        u'locked': False,
                                        u'locked_by': None,
                                        u'invalid': False,
                                        u'id': 4833,
                                        u'protection': u'Repair filesystem only'},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166},
                            {u'status': u'Running',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': {u'status': u'Running',
                                        u'lock_time': None,
                                        u'hostname': u'host42',
                                        u'locked': False,
                                        u'locked_by': None,
                                        u'invalid': False,
                                        u'id': 4833,
                                        u'protection': u'Repair filesystem only'},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166} ])],
                     out_words_ok=['Queued', 'Running', 'host41', 'host42'],
                     out_words_no=['Xeon'],
                     err_words_no=['unassigned'])


    def test_job_stat_job_multiple_hosts(self):
        self.run_cmd(argv=['atest', 'job', 'stat', '6761'],
                     rpcs=[('get_jobs_summary', {'id__in': ['6761']}, True,
                            [{u'status_counts': {u'Running': 1,
                                                 u'Queued': 4},
                              u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                              u'name': u'test_on_meta_hosts',
                              u'control_type': CLIENT,
                              u'run_verify': 1,
                              u'priority': priorities.Priority.DEFAULT,
                              u'owner': u'user0',
                              u'created_on': u'2008-07-30 22:15:43',
                              u'timeout': 144,
                              u'synch_count': 1,
                              u'id': 6761}]),
                           ('get_host_queue_entries', {'job__in': ['6761']},
                            True,
                            [{u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': u'Xeon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193166},
                             {u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': u'Xeon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193167},
                             {u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': u'Athlon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193168},
                             {u'status': u'Queued',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': u'x286',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 0,
                              u'id': 193169},
                             {u'status': u'Running',
                              u'complete': 0,
                              u'deleted': 0,
                              u'host': {u'status': u'Running',
                                        u'lock_time': None,
                                        u'hostname': u'host42',
                                        u'locked': False,
                                        u'locked_by': None,
                                        u'invalid': False,
                                        u'id': 4833,
                                        u'protection': u'Repair filesystem only'},
                              u'priority': 1,
                              u'meta_host': u'Athlon',
                              u'job': {u'control_file': u'def step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "mbligh@google.com (Martin Bligh)"\n    NAME = "Kernbench"\n    TIME = "SHORT"\n    TEST_CLASS = "Kernel"\n    TEST_CATEGORY = "Benchmark"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    A standard CPU benchmark. Runs a kernel compile and measures the performance.\n    """\n    \n    job.run_test(\'kernbench\')',
                                       u'name': u'test_on_meta_hosts',
                                       u'control_type': CLIENT,
                                       u'run_verify': 1,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-07-30 22:15:43',
                                       u'timeout': 144,
                                       u'synch_count': 1,
                                       u'id': 6761},
                              u'active': 1,
                              u'id': 193170} ])],
                     out_words_ok=['test_on_meta_hosts',
                                   'host42', 'Queued', 'Running'],
                     out_words_no=['Athlon', 'Xeon', 'x286'])


    def test_job_stat_job_no_host_in_qes(self):
        results = copy.deepcopy(self.results)
        self.run_cmd(argv=['atest', 'job', 'stat', '180'],
                     rpcs=[('get_jobs_summary', {'id__in': ['180']}, True,
                            [results[0]]),
                           ('get_host_queue_entries', {'job__in': ['180']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': None,
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 180},
                              u'active': 0,
                              u'id': 101084}])],
                     err_words_ok=['unassigned', 'meta-hosts'])


    def test_job_stat_multi_jobs(self):
        results = copy.deepcopy(self.results)
        self.run_cmd(argv=['atest', 'job', 'stat', '180', '338'],
                     rpcs=[('get_jobs_summary', {'id__in': ['180', '338']},
                            True, results),
                           ('get_host_queue_entries',
                            {'job__in': ['180', '338']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Repair Failed',
                                        u'locked': False,
                                        u'hostname': u'host0',
                                        u'invalid': True,
                                        u'id': 4432},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 180},
                              u'active': 0,
                              u'id': 101084},
                             {u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Repair Failed',
                                        u'locked': False,
                                        u'hostname': u'host10',
                                        u'invalid': True,
                                        u'id': 4432},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 338},
                              u'active': 0,
                              u'id': 101084}])],
                     out_words_ok=['test_job0', 'test_job1'])


    def test_job_stat_multi_jobs_name_id(self):
        self.run_cmd(argv=['atest', 'job', 'stat', 'mytest', '180'],
                     rpcs=[('get_jobs_summary', {'id__in': ['180']},
                            True,
                            [{u'status_counts': {u'Aborted': 1},
                             u'control_file':
                             u"job.run_test('sleeptest')\n",
                             u'name': u'job0',
                             u'control_type': SERVER,
                             u'priority':
                             priorities.Priority.DEFAULT,
                             u'owner': u'user0',
                             u'created_on':
                             u'2008-07-08 17:45:44',
                             u'synch_count': 2,
                             u'id': 180}]),
                           ('get_jobs_summary', {'name__in': ['mytest']},
                            True,
                            [{u'status_counts': {u'Queued': 1},
                             u'control_file':
                             u"job.run_test('sleeptest')\n",
                             u'name': u'mytest',
                             u'control_type': CLIENT,
                             u'priority':
                             priorities.Priority.DEFAULT,
                             u'owner': u'user0',
                             u'created_on': u'2008-07-08 12:17:47',
                             u'synch_count': 1,
                             u'id': 338}]),
                           ('get_host_queue_entries',
                            {'job__in': ['180']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Repair Failed',
                                        u'locked': False,
                                        u'hostname': u'host0',
                                        u'invalid': True,
                                        u'id': 4432},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'synchronizing': 0,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 180},
                              u'active': 0,
                              u'id': 101084}]),
                           ('get_host_queue_entries',
                            {'job__name__in': ['mytest']},
                            True,
                            [{u'status': u'Failed',
                              u'complete': 1,
                              u'host': {u'status': u'Repair Failed',
                                        u'locked': False,
                                        u'hostname': u'host10',
                                        u'invalid': True,
                                        u'id': 4432},
                              u'priority': 1,
                              u'meta_host': None,
                              u'job': {u'control_file': u"def run(machine):\n\thost = hosts.create_host(machine)\n\tat = autotest.Autotest(host)\n\tat.run_test('sleeptest')\n\nparallel_simple(run, machines)",
                                       u'name': u'test_sleep',
                                       u'control_type': SERVER,
                                       u'synchronizing': 0,
                                       u'priority': priorities.Priority.DEFAULT,
                                       u'owner': u'user0',
                                       u'created_on': u'2008-03-18 11:27:29',
                                       u'synch_count': 1,
                                       u'id': 338},
                              u'active': 0,
                              u'id': 101084}])],
                     out_words_ok=['job0', 'mytest', 'Aborted', 'Queued',
                                   'Failed', str(priorities.Priority.DEFAULT)])


class job_create_unittest(cli_mock.cli_unittest):
    ctrl_file = '\ndef step_init():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "Autotest Team"\n    NAME = "Sleeptest"\n  TIME =\n    "SHORT"\n    TEST_CATEGORY = "Functional"\n    TEST_CLASS = "General"\n\n    TEST_TYPE = "client"\n \n    DOC = """\n    This test simply sleeps for 1\n    second by default.  It\'s a good way to test\n    profilers and double check\n    that autotest is working.\n The seconds argument can also be modified to\n    make the machine sleep for as\n    long as needed.\n    """\n   \n\n    job.run_test(\'sleeptest\', seconds = 1)'

    kernel_ctrl_file = 'kernel = \'kernel\'\ndef step_init():\n    job.next_step([step_test])\n\ndef step_test():\n    job.next_step(\'step0\')\n\ndef step0():\n    AUTHOR = "Autotest Team"\n    NAME = "Sleeptest"\n    TIME = "SHORT"\n    TEST_CATEGORY = "Functional"\n    TEST_CLASS = "General"\n    TEST_TYPE = "client"\n    \n    DOC = """\n    This test simply sleeps for 1 second by default.  It\'s a good way to test\n    profilers and double check that autotest is working.\n    The seconds argument can also be modified to make the machine sleep for as\n    long as needed.\n    """\n    \n    job.run_test(\'sleeptest\', seconds = 1)'

    trivial_ctrl_file = 'print "Hello"\n'

    data = {'priority': priorities.Priority.DEFAULT, 'control_file': ctrl_file,
            'hosts': ['host0'],
            'name': 'test_job0', 'control_type': CLIENT, 'email_list': '',
            'meta_hosts': [], 'synch_count': 1, 'dependencies': [],
            'require_ssp': False}


    def test_execute_create_job(self):
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '-m', 'host0'],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : []}),
                           ('create_job', self.data, True, 180)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])


    def test_execute_create_job_with_control(self):
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0'],
                     rpcs=[('create_job', self.data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_control_and_email(self):
        data = self.data.copy()
        data['email_list'] = 'em'
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-e', 'em'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_control_and_dependencies(self):
        data = self.data.copy()
        data['dependencies'] = ['dep1', 'dep2']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-d', 'dep1, dep2 '],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_control_and_comma_dependencies(self):
        data = self.data.copy()
        data['dependencies'] = ['dep2,False', 'dep1,True']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-d',
                           'dep1\,True, dep2\,False '],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_synch_count(self):
        data = self.data.copy()
        data['synch_count'] = 2
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-y', '2'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_test_and_dependencies(self):
        data = self.data.copy()
        data['dependencies'] = ['dep1', 'dep2', 'dep3']
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '-m', 'host0', '-d', 'dep1, dep2 '],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : ['dep3']}),
                           ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])


    def test_execute_create_job_with_test_and_comma_dependencies(self):
        data = self.data.copy()
        data['dependencies'] = ['dep1,True', 'dep2,False', 'dep3,123']
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '-m', 'host0', '-d',
                           'dep1\,True dep2\,False '],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : ['dep3,123']}),
                           ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])


    def test_execute_create_job_no_args(self):
        testjob = job.job_create()
        sys.argv = ['atest', 'job', 'create']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(cli_mock.ExitException))
        self.assertRaises(cli_mock.ExitException, testjob.parse)
        self.god.unmock_io()
        self.god.check_playback()


    def test_execute_create_job_no_hosts(self):
        testjob = job.job_create()
        file_temp = cli_mock.create_file(self.ctrl_file)
        sys.argv = ['atest', '-f', file_temp.name, 'test_job0']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(cli_mock.ExitException))
        self.assertRaises(cli_mock.ExitException, testjob.parse)
        self.god.unmock_io()
        self.god.check_playback()
        file_temp.clean()


    def test_execute_create_job_cfile_and_tests(self):
        testjob = job.job_create()
        sys.argv = ['atest', 'job', 'create', '-t', 'sleeptest', '-f',
                    'control_file', 'test_job0', '-m', 'host0']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(cli_mock.ExitException))
        self.assertRaises(cli_mock.ExitException, testjob.parse)
        self.god.unmock_io()
        self.god.check_playback()


    def test_execute_create_job_bad_cfile(self):
        testjob = job.job_create()
        sys.argv = ['atest', 'job', 'create', '-f', 'control_file',
                    'test_job0', '-m', 'host0']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(IOError))
        self.assertRaises(IOError, testjob.parse)
        self.god.unmock_io()


    def test_execute_create_job_bad_priority(self):
        testjob = job.job_create()
        sys.argv = ['atest', 'job', 'create', '-t', 'sleeptest', '-p', 'Uber',
                    '-m', 'host0', 'test_job0']
        self.god.mock_io()
        (sys.exit.expect_call(mock.anything_comparator())
         .and_raises(cli_mock.ExitException))
        self.assertRaises(cli_mock.ExitException, testjob.parse)
        self.god.unmock_io()
        self.god.check_playback()


    def test_execute_create_job_with_mfile(self):
        data = self.data.copy()
        data['hosts'] = ['host3', 'host2', 'host1', 'host0']
        ctemp = cli_mock.create_file(self.ctrl_file)
        file_temp = cli_mock.create_file('host0\nhost1\nhost2\nhost3')
        self.run_cmd(argv=['atest', 'job', 'create', '--mlist', file_temp.name,
                           '-f', ctemp.name, 'test_job0'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'])
        ctemp.clean()
        file_temp.clean()


    def test_execute_create_job_with_timeout(self):
        data = self.data.copy()
        data['timeout_mins'] = '13320'
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-o', '13320'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],)
        file_temp.clean()


    def test_execute_create_job_with_max_runtime(self):
        data = self.data.copy()
        data['max_runtime_mins'] = '13320'
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '--max_runtime',
                           '13320'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],)
        file_temp.clean()



    def test_execute_create_job_with_noverify(self):
        data = self.data.copy()
        data['run_verify'] = False
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-n'],
                     rpcs=[('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],)
        file_temp.clean()


    def test_execute_create_job_oth(self):
        data = self.data.copy()
        data['hosts'] = []
        data['one_time_hosts'] = ['host0']
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '--one-time-hosts', 'host0'],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : []}),
                           ('create_job', data, True, 180)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])


    def test_execute_create_job_multi_oth(self):
        data = self.data.copy()
        data['hosts'] = []
        data['one_time_hosts'] = ['host1', 'host0']
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '--one-time-hosts', 'host0,host1'],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : []}),
                           ('create_job', data, True, 180)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])


    def test_execute_create_job_oth_exists(self):
        data = self.data.copy()
        data['hosts'] = []
        data['one_time_hosts'] = ['host0']
        self.run_cmd(argv=['atest', 'job', 'create', '-t', 'sleeptest',
                           'test_job0', '--one-time-hosts', 'host0'],
                     rpcs=[('generate_control_file',
                            {'tests': ['sleeptest']},
                            True,
                            {'control_file' : self.ctrl_file,
                             'synch_count' : 1,
                             'is_server' : False,
                             'dependencies' : []}),
                           ('create_job', data, False,
                            '''ValidationError: {'hostname': 'host0 '''
                            '''already exists in the autotest DB.  '''
                            '''Select it rather than entering it as '''
                            '''a one time host.'}''')],
                     out_words_no=['test_job0', 'Created'],
                     err_words_ok=['failed', 'already exists'])


    def test_execute_create_job_with_control_and_labels(self):
        data = self.data.copy()
        data['hosts'] = ['host0', 'host1', 'host2']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0', '-b', 'label1,label2'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label1',
                            'label2']}, True,
                            [{u'status': u'Running', u'lock_time': None,
                              u'hostname': u'host1', u'locked': False,
                              u'locked_by': None, u'invalid': False, u'id': 42,
                              u'labels': [u'label1'], u'platform':
                              u'Warp18_Diskfull', u'protection':
                              u'Repair software only', u'dirty': True},
                             {u'status': u'Running', u'lock_time': None,
                              u'hostname': u'host2', u'locked': False,
                              u'locked_by': None, u'invalid': False, u'id': 43,
                              u'labels': [u'label2'], u'platform':
                              u'Warp18_Diskfull', u'protection':
                              u'Repair software only', u'dirty': True}]),
                            ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_label_and_duplicate_hosts(self):
        data = self.data.copy()
        data['hosts'] = ['host1', 'host0']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0,host1', '-b', 'label1'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label1']}, True,
                            [{u'status': u'Running', u'lock_time': None,
                              u'hostname': u'host1', u'locked': False,
                              u'locked_by': None, u'invalid': False, u'id': 42,
                              u'labels': [u'label1'], u'platform':
                              u'Warp18_Diskfull', u'protection':
                              u'Repair software only', u'dirty': True}]),
                            ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_label_commas_and_duplicate_hosts(self):
        data = self.data.copy()
        data['hosts'] = ['host1', 'host0']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0,host1', '-b',
                           'label1,label\\,2'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label1',
                            'label,2']}, True,
                            [{u'status': u'Running', u'lock_time': None,
                              u'hostname': u'host1', u'locked': False,
                              u'locked_by': None, u'invalid': False, u'id': 42,
                              u'labels': [u'label1', u'label,2'], u'platform':
                              u'Warp18_Diskfull', u'protection':
                              u'Repair software only', u'dirty': True}]),
                            ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def test_execute_create_job_with_label_escaping_and_duplicate_hosts(self):
        data = self.data.copy()
        data['hosts'] = ['host1', 'host0']
        file_temp = cli_mock.create_file(self.ctrl_file)
        self.run_cmd(argv=['atest', 'job', 'create', '-f', file_temp.name,
                           'test_job0', '-m', 'host0,host1', '-b',
                           'label1,label\\,2\\\\,label3'],
                     rpcs=[('get_hosts', {'multiple_labels': ['label,2\\',
                            'label1', 'label3']}, True,
                            [{u'status': u'Running', u'lock_time': None,
                              u'hostname': u'host1', u'locked': False,
                              u'locked_by': None, u'invalid': False, u'id': 42,
                              u'labels': [u'label1', u'label,2\\', u'label3'],
                              u'platform': u'Warp18_Diskfull', u'protection':
                              u'Repair software only', u'dirty': True}]),
                            ('create_job', data, True, 42)],
                     out_words_ok=['test_job0', 'Created'],
                     out_words_no=['Uploading', 'Done'])
        file_temp.clean()


    def _test_parse_hosts(self, args, exp_hosts=[], exp_meta_hosts=[]):
        testjob = job.job_create_or_clone()
        (hosts, meta_hosts) = testjob._parse_hosts(args)
        self.assertEqualNoOrder(hosts, exp_hosts)
        self.assertEqualNoOrder(meta_hosts, exp_meta_hosts)


    def test_parse_hosts_regular(self):
        self._test_parse_hosts(['host0'], ['host0'])


    def test_parse_hosts_regulars(self):
        self._test_parse_hosts(['host0', 'host1'], ['host0', 'host1'])


    def test_parse_hosts_meta_one(self):
        self._test_parse_hosts(['*meta0'], [], ['meta0'])


    def test_parse_hosts_meta_five(self):
        self._test_parse_hosts(['5*meta0'], [], ['meta0']*5)


    def test_parse_hosts_metas_five(self):
        self._test_parse_hosts(['5*meta0', '2*meta1'], [],
                               ['meta0']*5 + ['meta1']*2)


    def test_parse_hosts_mix(self):
        self._test_parse_hosts(['5*meta0', 'host0', '2*meta1', 'host1',
                                '*meta2'], ['host0', 'host1'],
                               ['meta0']*5 + ['meta1']*2 + ['meta2'])


class job_clone_unittest(cli_mock.cli_unittest):
    job_data = {'control_file': u'NAME = \'Server Sleeptest\'\nAUTHOR = \'mbligh@google.com (Martin Bligh)\'\nTIME = \'SHORT\'\nTEST_CLASS = \'Software\'\nTEST_CATEGORY = \'Functional\'\nTEST_TYPE = \'server\'\nEXPERIMENTAL = \'False\'\n\nDOC = """\nruns sleep for one second on the list of machines.\n"""\n\ndef run(machine):\n    host = hosts.create_host(machine)\n    job.run_test(\'sleeptest\')\n\njob.parallel_simple(run, machines)\n',
                    'control_type': SERVER,
                    'dependencies': [],
                    'email_list': u'',
                    'max_runtime_mins': 28800,
                    'parse_failed_repair': True,
                    'priority': priorities.Priority.DEFAULT,
                    'reboot_after': u'Always',
                    'reboot_before': u'If dirty',
                    'run_verify': True,
                    'synch_count': 1,
                    'timeout_mins': 480}

    local_hosts = [{u'acls': [u'acl0'],
                    u'attributes': {},
                    u'dirty': False,
                    u'hostname': u'host0',
                    u'id': 8,
                    u'invalid': False,
                    u'labels': [u'label0', u'label1'],
                    u'lock_time': None,
                    u'locked': False,
                    u'locked_by': None,
                    u'other_labels': u'label0, label1',
                    u'platform': u'plat0',
                    u'protection': u'Repair software only',
                    u'status': u'Ready'},
                   {u'acls': [u'acl0'],
                    u'attributes': {},
                    u'dirty': False,
                    u'hostname': u'host1',
                    u'id': 9,
                    u'invalid': False,
                    u'labels': [u'label0', u'label1'],
                    u'lock_time': None,
                    u'locked': False,
                    u'locked_by': None,
                    u'other_labels': u'label0, label1',
                    u'platform': u'plat0',
                    u'protection': u'Repair software only',
                    u'status': u'Ready'}]


    def setUp(self):
        super(job_clone_unittest, self).setUp()
        self.job_data_clone_info = copy.deepcopy(self.job_data)
        self.job_data_clone_info['created_on'] = '2009-07-23 16:21:29'
        self.job_data_clone_info['name'] = 'testing_clone'
        self.job_data_clone_info['id'] = 42
        self.job_data_clone_info['owner'] = 'user0'

        self.job_data_cloned = copy.deepcopy(self.job_data)
        self.job_data_cloned['name'] = 'cloned'
        self.job_data_cloned['hosts'] = [u'host0']
        self.job_data_cloned['meta_hosts'] = []


    def test_backward_compat(self):
        self.run_cmd(argv=['atest', 'job', 'create', '--clone', '42',
                           '-r', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': True},
                            True,
                            {u'hosts': [{u'acls': [u'acl0'],
                                         u'attributes': {},
                                         u'dirty': False,
                                         u'hostname': u'host0',
                                         u'id': 4378,
                                         u'invalid': False,
                                         u'labels': [u'label0', u'label1'],
                                         u'lock_time': None,
                                         u'locked': False,
                                         u'locked_by': None,
                                         u'other_labels': u'label0, label1',
                                         u'platform': u'plat0',
                                         u'protection': u'Repair software only',
                                         u'status': u'Ready'}],
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


    def test_clone_reuse_hosts(self):
        self.job_data_cloned['hosts'] = [u'host0', 'host1']
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '-r', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': True},
                            True,
                            {u'hosts': self.local_hosts,
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


    def test_clone_reuse_metahosts(self):
        self.job_data_cloned['hosts'] = []
        self.job_data_cloned['meta_hosts'] = ['type1']*4 +  ['type0']
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '-r', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': True},
                            True,
                            {u'hosts': [],
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {u'type0': 1,
                                                   u'type1': 4}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


    def test_clone_reuse_both(self):
        self.job_data_cloned['hosts'] = [u'host0', 'host1']
        self.job_data_cloned['meta_hosts'] = ['type1']*4 +  ['type0']
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '-r', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': True},
                            True,
                            {
                             u'hosts': self.local_hosts,
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {u'type0': 1,
                                                   u'type1': 4}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


    def test_clone_no_hosts(self):
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42', 'cloned'],
                     exit_code=1,
                     out_words_ok=['usage'],
                     err_words_ok=['machine'])


    def test_clone_reuse_and_hosts(self):
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '-r', '-m', 'host5', 'cloned'],
                     exit_code=1,
                     out_words_ok=['usage'],
                     err_words_ok=['specify'])


    def test_clone_new_multiple_hosts(self):
        self.job_data_cloned['hosts'] = [u'host5', 'host4', 'host3']
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '-m', 'host5,host4,host3', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': False},
                            True,
                            {u'hosts': self.local_hosts,
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


    def test_clone_oth(self):
        self.job_data_cloned['hosts'] = []
        self.job_data_cloned['one_time_hosts'] = [u'host5']
        self.run_cmd(argv=['atest', 'job', 'clone', '--id', '42',
                           '--one-time-hosts', 'host5', 'cloned'],
                     rpcs=[('get_info_for_clone', {'id': '42',
                                                   'preserve_metahosts': False},
                            True,
                            {u'hosts': self.local_hosts,
                             u'job': self.job_data_clone_info,
                             u'meta_host_counts': {}}),
                           ('create_job', self.job_data_cloned, True, 43)],
                     out_words_ok=['Created job', '43'])


class job_abort_unittest(cli_mock.cli_unittest):
    results = [{u'status_counts': {u'Aborted': 1}, u'control_file':
                u"job.run_test('sleeptest')\n", u'name': u'test_job0',
                u'control_type': SERVER, u'priority':
                priorities.Priority.DEFAULT, u'owner': u'user0', u'created_on':
                u'2008-07-08 17:45:44', u'synch_count': 2, u'id': 180}]

    def test_execute_job_abort(self):
        self.run_cmd(argv=['atest', 'job', 'abort', '180'],
                     rpcs=[('abort_host_queue_entries',
                            {'job__id__in': ['180']}, True, None)],
                     out_words_ok=['Aborting', '180'])


if __name__ == '__main__':
    unittest.main()
