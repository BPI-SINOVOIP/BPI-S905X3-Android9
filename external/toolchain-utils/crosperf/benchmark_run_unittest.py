#!/usr/bin/env python2

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Testing of benchmark_run."""

from __future__ import print_function

import mock
import unittest
import inspect

from cros_utils import logger

import benchmark_run

from suite_runner import MockSuiteRunner
from suite_runner import SuiteRunner
from label import MockLabel
from benchmark import Benchmark
from machine_manager import MockMachineManager
from machine_manager import MachineManager
from machine_manager import MockCrosMachine
from results_cache import MockResultsCache
from results_cache import CacheConditions
from results_cache import Result
from results_cache import ResultsCache


class BenchmarkRunTest(unittest.TestCase):
  """Unit tests for the BenchmarkRun class and all of its methods."""

  def setUp(self):
    self.status = []
    self.called_ReadCache = None
    self.log_error = []
    self.log_output = []
    self.err_msg = None
    self.test_benchmark = Benchmark(
        'page_cycler.netsim.top_10',  # name
        'page_cycler.netsim.top_10',  # test_name
        '',  # test_args
        1,  # iterations
        False,  # rm_chroot_tmp
        '',  # perf_args
        suite='telemetry_Crosperf')  # suite

    self.test_label = MockLabel(
        'test1',
        'image1',
        'autotest_dir',
        '/tmp/test_benchmark_run',
        'x86-alex',
        'chromeos2-row1-rack4-host9.cros',
        image_args='',
        cache_dir='',
        cache_only=False,
        log_level='average',
        compiler='gcc')

    self.test_cache_conditions = [
        CacheConditions.CACHE_FILE_EXISTS, CacheConditions.CHECKSUMS_MATCH
    ]

    self.mock_logger = logger.GetLogger(log_dir='', mock=True)

    self.mock_machine_manager = mock.Mock(spec=MachineManager)

  def testDryRun(self):
    my_label = MockLabel(
        'test1',
        'image1',
        'autotest_dir',
        '/tmp/test_benchmark_run',
        'x86-alex',
        'chromeos2-row1-rack4-host9.cros',
        image_args='',
        cache_dir='',
        cache_only=False,
        log_level='average',
        compiler='gcc')

    logging_level = 'average'
    m = MockMachineManager('/tmp/chromeos_root', 0, logging_level, '')
    m.AddMachine('chromeos2-row1-rack4-host9.cros')
    bench = Benchmark(
        'page_cycler.netsim.top_10',  # name
        'page_cycler.netsim.top_10',  # test_name
        '',  # test_args
        1,  # iterations
        False,  # rm_chroot_tmp
        '',  # perf_args
        suite='telemetry_Crosperf')  # suite
    b = benchmark_run.MockBenchmarkRun('test run', bench, my_label, 1, [], m,
                                       logger.GetLogger(), logging_level, '')
    b.cache = MockResultsCache()
    b.suite_runner = MockSuiteRunner()
    b.start()

    # Make sure the arguments to BenchmarkRun.__init__ have not changed
    # since the last time this test was updated:
    args_list = [
        'self', 'name', 'benchmark', 'label', 'iteration', 'cache_conditions',
        'machine_manager', 'logger_to_use', 'log_level', 'share_cache'
    ]
    arg_spec = inspect.getargspec(benchmark_run.BenchmarkRun.__init__)
    self.assertEqual(len(arg_spec.args), len(args_list))
    self.assertEqual(arg_spec.args, args_list)

  def test_init(self):
    # Nothing really worth testing here; just field assignments.
    pass

  def test_read_cache(self):
    # Nothing really worth testing here, either.
    pass

  def test_run(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    def MockLogOutput(msg, print_to_console=False):
      'Helper function for test_run.'
      del print_to_console
      self.log_output.append(msg)

    def MockLogError(msg, print_to_console=False):
      'Helper function for test_run.'
      del print_to_console
      self.log_error.append(msg)

    def MockRecordStatus(msg):
      'Helper function for test_run.'
      self.status.append(msg)

    def FakeReadCache():
      'Helper function for test_run.'
      br.cache = mock.Mock(spec=ResultsCache)
      self.called_ReadCache = True
      return 0

    def FakeReadCacheSucceed():
      'Helper function for test_run.'
      br.cache = mock.Mock(spec=ResultsCache)
      br.result = mock.Mock(spec=Result)
      br.result.out = 'result.out stuff'
      br.result.err = 'result.err stuff'
      br.result.retval = 0
      self.called_ReadCache = True
      return 0

    def FakeReadCacheException():
      'Helper function for test_run.'
      raise RuntimeError('This is an exception test; it is supposed to happen')

    def FakeAcquireMachine():
      'Helper function for test_run.'
      mock_machine = MockCrosMachine('chromeos1-row3-rack5-host7.cros',
                                     'chromeos', 'average')
      return mock_machine

    def FakeRunTest(_machine):
      'Helper function for test_run.'
      mock_result = mock.Mock(spec=Result)
      mock_result.retval = 0
      return mock_result

    def FakeRunTestFail(_machine):
      'Helper function for test_run.'
      mock_result = mock.Mock(spec=Result)
      mock_result.retval = 1
      return mock_result

    def ResetTestValues():
      'Helper function for test_run.'
      self.log_output = []
      self.log_error = []
      self.status = []
      br.result = None
      self.called_ReadCache = False

    # Assign all the fake functions to the appropriate objects.
    br.logger().LogOutput = MockLogOutput
    br.logger().LogError = MockLogError
    br.timeline.Record = MockRecordStatus
    br.ReadCache = FakeReadCache
    br.RunTest = FakeRunTest
    br.AcquireMachine = FakeAcquireMachine

    # First test:  No cache hit, all goes well.
    ResetTestValues()
    br.run()
    self.assertTrue(self.called_ReadCache)
    self.assertEqual(self.log_output, [
        'test_run: No cache hit.',
        'Releasing machine: chromeos1-row3-rack5-host7.cros',
        'Released machine: chromeos1-row3-rack5-host7.cros'
    ])
    self.assertEqual(len(self.log_error), 0)
    self.assertEqual(self.status, ['WAITING', 'SUCCEEDED'])

    # Second test: No cached result found; test run was "terminated" for some
    # reason.
    ResetTestValues()
    br.terminated = True
    br.run()
    self.assertTrue(self.called_ReadCache)
    self.assertEqual(self.log_output, [
        'test_run: No cache hit.',
        'Releasing machine: chromeos1-row3-rack5-host7.cros',
        'Released machine: chromeos1-row3-rack5-host7.cros'
    ])
    self.assertEqual(len(self.log_error), 0)
    self.assertEqual(self.status, ['WAITING'])

    # Third test.  No cached result found; RunTest failed for some reason.
    ResetTestValues()
    br.terminated = False
    br.RunTest = FakeRunTestFail
    br.run()
    self.assertTrue(self.called_ReadCache)
    self.assertEqual(self.log_output, [
        'test_run: No cache hit.',
        'Releasing machine: chromeos1-row3-rack5-host7.cros',
        'Released machine: chromeos1-row3-rack5-host7.cros'
    ])
    self.assertEqual(len(self.log_error), 0)
    self.assertEqual(self.status, ['WAITING', 'FAILED'])

    # Fourth test: ReadCache found a cached result.
    ResetTestValues()
    br.RunTest = FakeRunTest
    br.ReadCache = FakeReadCacheSucceed
    br.run()
    self.assertTrue(self.called_ReadCache)
    self.assertEqual(self.log_output, [
        'test_run: Cache hit.', 'result.out stuff',
        'Releasing machine: chromeos1-row3-rack5-host7.cros',
        'Released machine: chromeos1-row3-rack5-host7.cros'
    ])
    self.assertEqual(self.log_error, ['result.err stuff'])
    self.assertEqual(self.status, ['SUCCEEDED'])

    # Fifth test: ReadCache generates an exception; does the try/finally block
    # work?
    ResetTestValues()
    br.ReadCache = FakeReadCacheException
    br.machine = FakeAcquireMachine()
    br.run()
    self.assertEqual(self.log_error, [
        "Benchmark run: 'test_run' failed: This is an exception test; it is "
        'supposed to happen'
    ])
    self.assertEqual(self.status, ['FAILED'])

  def test_terminate_pass(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    def GetLastEventPassed():
      'Helper function for test_terminate_pass'
      return benchmark_run.STATUS_SUCCEEDED

    def RecordStub(status):
      'Helper function for test_terminate_pass'
      self.status = status

    self.status = benchmark_run.STATUS_SUCCEEDED
    self.assertFalse(br.terminated)
    self.assertFalse(br.suite_runner.CommandTerminator().IsTerminated())

    br.timeline.GetLastEvent = GetLastEventPassed
    br.timeline.Record = RecordStub

    br.Terminate()

    self.assertTrue(br.terminated)
    self.assertTrue(br.suite_runner.CommandTerminator().IsTerminated())
    self.assertEqual(self.status, benchmark_run.STATUS_FAILED)

  def test_terminate_fail(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    def GetLastEventFailed():
      'Helper function for test_terminate_fail'
      return benchmark_run.STATUS_FAILED

    def RecordStub(status):
      'Helper function for test_terminate_fail'
      self.status = status

    self.status = benchmark_run.STATUS_SUCCEEDED
    self.assertFalse(br.terminated)
    self.assertFalse(br.suite_runner.CommandTerminator().IsTerminated())

    br.timeline.GetLastEvent = GetLastEventFailed
    br.timeline.Record = RecordStub

    br.Terminate()

    self.assertTrue(br.terminated)
    self.assertTrue(br.suite_runner.CommandTerminator().IsTerminated())
    self.assertEqual(self.status, benchmark_run.STATUS_SUCCEEDED)

  def test_acquire_machine(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    br.terminated = True
    self.assertRaises(Exception, br.AcquireMachine)

    br.terminated = False
    mock_machine = MockCrosMachine('chromeos1-row3-rack5-host7.cros',
                                   'chromeos', 'average')
    self.mock_machine_manager.AcquireMachine.return_value = mock_machine

    machine = br.AcquireMachine()
    self.assertEqual(machine.name, 'chromeos1-row3-rack5-host7.cros')

  def test_get_extra_autotest_args(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    def MockLogError(err_msg):
      'Helper function for test_get_extra_autotest_args'
      self.err_msg = err_msg

    self.mock_logger.LogError = MockLogError

    result = br.GetExtraAutotestArgs()
    self.assertEqual(result, '')

    self.test_benchmark.perf_args = 'record -e cycles'
    result = br.GetExtraAutotestArgs()
    self.assertEqual(
        result,
        "--profiler=custom_perf --profiler_args='perf_options=\"record -a -e "
        "cycles\"'")

    self.test_benchmark.suite = 'telemetry'
    result = br.GetExtraAutotestArgs()
    self.assertEqual(result, '')
    self.assertEqual(self.err_msg, 'Telemetry does not support profiler.')

    self.test_benchmark.perf_args = 'record -e cycles'
    self.test_benchmark.suite = 'test_that'
    result = br.GetExtraAutotestArgs()
    self.assertEqual(result, '')
    self.assertEqual(self.err_msg, 'test_that does not support profiler.')

    self.test_benchmark.perf_args = 'junk args'
    self.test_benchmark.suite = 'telemetry_Crosperf'
    self.assertRaises(Exception, br.GetExtraAutotestArgs)

  @mock.patch.object(SuiteRunner, 'Run')
  @mock.patch.object(Result, 'CreateFromRun')
  def test_run_test(self, mock_result, mock_runner):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    self.status = []

    def MockRecord(status):
      self.status.append(status)

    br.timeline.Record = MockRecord
    mock_machine = MockCrosMachine('chromeos1-row3-rack5-host7.cros',
                                   'chromeos', 'average')
    mock_runner.return_value = [0, "{'Score':100}", '']

    br.RunTest(mock_machine)

    self.assertTrue(br.run_completed)
    self.assertEqual(self.status, [
        benchmark_run.STATUS_IMAGING, benchmark_run.STATUS_RUNNING
    ])

    self.assertEqual(br.machine_manager.ImageMachine.call_count, 1)
    br.machine_manager.ImageMachine.assert_called_with(mock_machine,
                                                       self.test_label)
    self.assertEqual(mock_runner.call_count, 1)
    mock_runner.assert_called_with(mock_machine.name, br.label, br.benchmark,
                                   '', br.profiler_args)

    self.assertEqual(mock_result.call_count, 1)
    mock_result.assert_called_with(
        self.mock_logger, 'average', self.test_label, None, "{'Score':100}", '',
        0, 'page_cycler.netsim.top_10', 'telemetry_Crosperf')

  def test_set_cache_conditions(self):
    br = benchmark_run.BenchmarkRun(
        'test_run', self.test_benchmark, self.test_label, 1,
        self.test_cache_conditions, self.mock_machine_manager, self.mock_logger,
        'average', '')

    phony_cache_conditions = [123, 456, True, False]

    self.assertEqual(br.cache_conditions, self.test_cache_conditions)

    br.SetCacheConditions(phony_cache_conditions)
    self.assertEqual(br.cache_conditions, phony_cache_conditions)

    br.SetCacheConditions(self.test_cache_conditions)
    self.assertEqual(br.cache_conditions, self.test_cache_conditions)


if __name__ == '__main__':
  unittest.main()
