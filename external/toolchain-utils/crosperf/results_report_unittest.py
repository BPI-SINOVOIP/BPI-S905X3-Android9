#!/usr/bin/env python2
#
# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Unittest for the results reporter."""

from __future__ import division
from __future__ import print_function

from StringIO import StringIO

import collections
import mock
import os
import test_flag
import unittest

from benchmark_run import MockBenchmarkRun
from cros_utils import logger
from experiment_factory import ExperimentFactory
from experiment_file import ExperimentFile
from machine_manager import MockCrosMachine
from machine_manager import MockMachineManager
from results_cache import MockResult
from results_report import BenchmarkResults
from results_report import HTMLResultsReport
from results_report import JSONResultsReport
from results_report import ParseChromeosImage
from results_report import ParseStandardPerfReport
from results_report import TextResultsReport


class FreeFunctionsTest(unittest.TestCase):
  """Tests for any free functions in results_report."""

  def testParseChromeosImage(self):
    # N.B. the cases with blank versions aren't explicitly supported by
    # ParseChromeosImage. I'm not sure if they need to be supported, but the
    # goal of this was to capture existing functionality as much as possible.
    base_case = '/my/chroot/src/build/images/x86-generic/R01-1.0.date-time' \
        '/chromiumos_test_image.bin'
    self.assertEqual(ParseChromeosImage(base_case), ('R01-1.0', base_case))

    dir_base_case = os.path.dirname(base_case)
    self.assertEqual(ParseChromeosImage(dir_base_case), ('', dir_base_case))

    buildbot_case = '/my/chroot/chroot/tmp/buildbot-build/R02-1.0.date-time' \
        '/chromiumos_test_image.bin'
    buildbot_img = buildbot_case.split('/chroot/tmp')[1]

    self.assertEqual(
        ParseChromeosImage(buildbot_case), ('R02-1.0', buildbot_img))
    self.assertEqual(
        ParseChromeosImage(os.path.dirname(buildbot_case)),
        ('', os.path.dirname(buildbot_img)))

    # Ensure we don't act completely insanely given a few mildly insane paths.
    fun_case = '/chromiumos_test_image.bin'
    self.assertEqual(ParseChromeosImage(fun_case), ('', fun_case))

    fun_case2 = 'chromiumos_test_image.bin'
    self.assertEqual(ParseChromeosImage(fun_case2), ('', fun_case2))


# There are many ways for this to be done better, but the linter complains
# about all of them (that I can think of, at least).
_fake_path_number = [0]


def FakePath(ext):
  """Makes a unique path that shouldn't exist on the host system.

  Each call returns a different path, so if said path finds its way into an
  error message, it may be easier to track it to its source.
  """
  _fake_path_number[0] += 1
  prefix = '/tmp/should/not/exist/%d/' % (_fake_path_number[0],)
  return os.path.join(prefix, ext)


def MakeMockExperiment(compiler='gcc'):
  """Mocks an experiment using the given compiler."""
  mock_experiment_file = StringIO("""
      board: x86-alex
      remote: 127.0.0.1
      perf_args: record -a -e cycles
      benchmark: PageCycler {
        iterations: 3
      }

      image1 {
        chromeos_image: %s
      }

      image2 {
        remote: 127.0.0.2
        chromeos_image: %s
      }
      """ % (FakePath('cros_image1.bin'), FakePath('cros_image2.bin')))
  efile = ExperimentFile(mock_experiment_file)
  experiment = ExperimentFactory().GetExperiment(efile,
                                                 FakePath('working_directory'),
                                                 FakePath('log_dir'))
  for label in experiment.labels:
    label.compiler = compiler
  return experiment


def _InjectSuccesses(experiment, how_many, keyvals, for_benchmark=0,
                     label=None):
  """Injects successful experiment runs (for each label) into the experiment."""
  # Defensive copy of keyvals, so if it's modified, we'll know.
  keyvals = dict(keyvals)
  num_configs = len(experiment.benchmarks) * len(experiment.labels)
  num_runs = len(experiment.benchmark_runs) // num_configs

  # TODO(gbiv): Centralize the mocking of these, maybe? (It's also done in
  # benchmark_run_unittest)
  bench = experiment.benchmarks[for_benchmark]
  cache_conditions = []
  log_level = 'average'
  share_cache = ''
  locks_dir = ''
  log = logger.GetLogger()
  machine_manager = MockMachineManager(
      FakePath('chromeos_root'), 0, log_level, locks_dir)
  machine_manager.AddMachine('testing_machine')
  machine = next(m for m in machine_manager.GetMachines()
                 if m.name == 'testing_machine')
  for label in experiment.labels:

    def MakeSuccessfulRun(n):
      run = MockBenchmarkRun('mock_success%d' % (n,), bench, label,
                             1 + n + num_runs, cache_conditions,
                             machine_manager, log, log_level, share_cache)
      mock_result = MockResult(log, label, log_level, machine)
      mock_result.keyvals = keyvals
      run.result = mock_result
      return run

    experiment.benchmark_runs.extend(
        MakeSuccessfulRun(n) for n in xrange(how_many))
  return experiment


class TextResultsReportTest(unittest.TestCase):
  """Tests that the output of a text report contains the things we pass in.

  At the moment, this doesn't care deeply about the format in which said
  things are displayed. It just cares that they're present.
  """

  def _checkReport(self, email):
    num_success = 2
    success_keyvals = {'retval': 0, 'machine': 'some bot', 'a_float': 3.96}
    experiment = _InjectSuccesses(MakeMockExperiment(), num_success,
                                  success_keyvals)
    text_report = TextResultsReport.FromExperiment(experiment, email=email) \
                                   .GetReport()
    self.assertIn(str(success_keyvals['a_float']), text_report)
    self.assertIn(success_keyvals['machine'], text_report)
    self.assertIn(MockCrosMachine.CPUINFO_STRING, text_report)
    return text_report

  def testOutput(self):
    email_report = self._checkReport(email=True)
    text_report = self._checkReport(email=False)

    # Ensure that the reports somehow different. Otherwise, having the
    # distinction is useless.
    self.assertNotEqual(email_report, text_report)


class HTMLResultsReportTest(unittest.TestCase):
  """Tests that the output of a HTML report contains the things we pass in.

  At the moment, this doesn't care deeply about the format in which said
  things are displayed. It just cares that they're present.
  """

  _TestOutput = collections.namedtuple('TestOutput', [
      'summary_table', 'perf_html', 'chart_js', 'charts', 'full_table',
      'experiment_file'
  ])

  @staticmethod
  def _GetTestOutput(perf_table, chart_js, summary_table, print_table,
                     chart_divs, full_table, experiment_file):
    # N.B. Currently we don't check chart_js; it's just passed through because
    # cros lint complains otherwise.
    summary_table = print_table(summary_table, 'HTML')
    perf_html = print_table(perf_table, 'HTML')
    full_table = print_table(full_table, 'HTML')
    return HTMLResultsReportTest._TestOutput(
        summary_table=summary_table,
        perf_html=perf_html,
        chart_js=chart_js,
        charts=chart_divs,
        full_table=full_table,
        experiment_file=experiment_file)

  def _GetOutput(self, experiment=None, benchmark_results=None):
    with mock.patch('results_report_templates.GenerateHTMLPage') as standin:
      if experiment is not None:
        HTMLResultsReport.FromExperiment(experiment).GetReport()
      else:
        HTMLResultsReport(benchmark_results).GetReport()
      mod_mock = standin
    self.assertEquals(mod_mock.call_count, 1)
    # call_args[0] is positional args, call_args[1] is kwargs.
    self.assertEquals(mod_mock.call_args[0], tuple())
    fmt_args = mod_mock.call_args[1]
    return self._GetTestOutput(**fmt_args)

  def testNoSuccessOutput(self):
    output = self._GetOutput(MakeMockExperiment())
    self.assertIn('no result', output.summary_table)
    self.assertIn('no result', output.full_table)
    self.assertEqual(output.charts, '')
    self.assertNotEqual(output.experiment_file, '')

  def testSuccessfulOutput(self):
    num_success = 2
    success_keyvals = {'retval': 0, 'a_float': 3.96}
    output = self._GetOutput(
        _InjectSuccesses(MakeMockExperiment(), num_success, success_keyvals))

    self.assertNotIn('no result', output.summary_table)
    #self.assertIn(success_keyvals['machine'], output.summary_table)
    self.assertIn('a_float', output.summary_table)
    self.assertIn(str(success_keyvals['a_float']), output.summary_table)
    self.assertIn('a_float', output.full_table)
    # The _ in a_float is filtered out when we're generating HTML.
    self.assertIn('afloat', output.charts)
    # And make sure we have our experiment file...
    self.assertNotEqual(output.experiment_file, '')

  def testBenchmarkResultFailure(self):
    labels = ['label1']
    benchmark_names_and_iterations = [('bench1', 1)]
    benchmark_keyvals = {'bench1': [[]]}
    results = BenchmarkResults(labels, benchmark_names_and_iterations,
                               benchmark_keyvals)
    output = self._GetOutput(benchmark_results=results)
    self.assertIn('no result', output.summary_table)
    self.assertEqual(output.charts, '')
    self.assertEqual(output.experiment_file, '')

  def testBenchmarkResultSuccess(self):
    labels = ['label1']
    benchmark_names_and_iterations = [('bench1', 1)]
    benchmark_keyvals = {'bench1': [[{'retval': 1, 'foo': 2.0}]]}
    results = BenchmarkResults(labels, benchmark_names_and_iterations,
                               benchmark_keyvals)
    output = self._GetOutput(benchmark_results=results)
    self.assertNotIn('no result', output.summary_table)
    self.assertIn('bench1', output.summary_table)
    self.assertIn('bench1', output.full_table)
    self.assertNotEqual(output.charts, '')
    self.assertEqual(output.experiment_file, '')


class JSONResultsReportTest(unittest.TestCase):
  """Tests JSONResultsReport."""

  REQUIRED_REPORT_KEYS = ('date', 'time', 'label', 'test_name', 'pass')
  EXPERIMENT_REPORT_KEYS = ('board', 'chromeos_image', 'chromeos_version',
                            'chrome_version', 'compiler')

  @staticmethod
  def _GetRequiredKeys(is_experiment):
    required_keys = JSONResultsReportTest.REQUIRED_REPORT_KEYS
    if is_experiment:
      required_keys += JSONResultsReportTest.EXPERIMENT_REPORT_KEYS
    return required_keys

  def _CheckRequiredKeys(self, test_output, is_experiment):
    required_keys = self._GetRequiredKeys(is_experiment)
    for output in test_output:
      for key in required_keys:
        self.assertIn(key, output)

  def testAllFailedJSONReportOutput(self):
    experiment = MakeMockExperiment()
    results = JSONResultsReport.FromExperiment(experiment).GetReportObject()
    self._CheckRequiredKeys(results, is_experiment=True)
    # Nothing succeeded; we don't send anything more than what's required.
    required_keys = self._GetRequiredKeys(is_experiment=True)
    for result in results:
      self.assertItemsEqual(result.iterkeys(), required_keys)

  def testJSONReportOutputWithSuccesses(self):
    success_keyvals = {
        'retval': 0,
        'a_float': '2.3',
        'many_floats': [['1.0', '2.0'], ['3.0']],
        'machine': "i'm a pirate"
    }

    # 2 is arbitrary.
    num_success = 2
    experiment = _InjectSuccesses(MakeMockExperiment(), num_success,
                                  success_keyvals)
    results = JSONResultsReport.FromExperiment(experiment).GetReportObject()
    self._CheckRequiredKeys(results, is_experiment=True)

    num_passes = num_success * len(experiment.labels)
    non_failures = [r for r in results if r['pass']]
    self.assertEqual(num_passes, len(non_failures))

    # TODO(gbiv): ...Is the 3.0 *actually* meant to be dropped?
    expected_detailed = {'a_float': 2.3, 'many_floats': [1.0, 2.0]}
    for pass_ in non_failures:
      self.assertIn('detailed_results', pass_)
      self.assertDictEqual(expected_detailed, pass_['detailed_results'])
      self.assertIn('machine', pass_)
      self.assertEqual(success_keyvals['machine'], pass_['machine'])

  def testFailedJSONReportOutputWithoutExperiment(self):
    labels = ['label1']
    benchmark_names_and_iterations = [('bench1', 1), ('bench2', 2),
                                      ('bench3', 1), ('bench4', 0)]
    benchmark_keyvals = {
        'bench1': [[{
            'retval': 1,
            'foo': 2.0
        }]],
        'bench2': [[{
            'retval': 1,
            'foo': 4.0
        }, {
            'retval': -1,
            'bar': 999
        }]],
        # lack of retval is considered a failure.
        'bench3': [[{}]],
        'bench4': [[]]
    }
    bench_results = BenchmarkResults(labels, benchmark_names_and_iterations,
                                     benchmark_keyvals)
    results = JSONResultsReport(bench_results).GetReportObject()
    self._CheckRequiredKeys(results, is_experiment=False)
    self.assertFalse(any(r['pass'] for r in results))

  def testJSONGetReportObeysJSONSettings(self):
    labels = ['label1']
    benchmark_names_and_iterations = [('bench1', 1)]
    # These can be anything, really. So long as they're distinctive.
    separators = (',\t\n\t', ':\t\n\t')
    benchmark_keyvals = {'bench1': [[{'retval': 0, 'foo': 2.0}]]}
    bench_results = BenchmarkResults(labels, benchmark_names_and_iterations,
                                     benchmark_keyvals)
    reporter = JSONResultsReport(
        bench_results, json_args={'separators': separators})
    result_str = reporter.GetReport()
    self.assertIn(separators[0], result_str)
    self.assertIn(separators[1], result_str)

  def testSuccessfulJSONReportOutputWithoutExperiment(self):
    labels = ['label1']
    benchmark_names_and_iterations = [('bench1', 1), ('bench2', 2)]
    benchmark_keyvals = {
        'bench1': [[{
            'retval': 0,
            'foo': 2.0
        }]],
        'bench2': [[{
            'retval': 0,
            'foo': 4.0
        }, {
            'retval': 0,
            'bar': 999
        }]]
    }
    bench_results = BenchmarkResults(labels, benchmark_names_and_iterations,
                                     benchmark_keyvals)
    results = JSONResultsReport(bench_results).GetReportObject()
    self._CheckRequiredKeys(results, is_experiment=False)
    self.assertTrue(all(r['pass'] for r in results))
    # Enforce that the results have *some* deterministic order.
    keyfn = lambda r: (r['test_name'], r['detailed_results'].get('foo', 5.0))
    sorted_results = sorted(results, key=keyfn)
    detailed_results = [r['detailed_results'] for r in sorted_results]
    bench1, bench2_foo, bench2_bar = detailed_results
    self.assertEqual(bench1['foo'], 2.0)
    self.assertEqual(bench2_foo['foo'], 4.0)
    self.assertEqual(bench2_bar['bar'], 999)
    self.assertNotIn('bar', bench1)
    self.assertNotIn('bar', bench2_foo)
    self.assertNotIn('foo', bench2_bar)


class PerfReportParserTest(unittest.TestCase):
  """Tests for the perf report parser in results_report."""

  @staticmethod
  def _ReadRealPerfReport():
    my_dir = os.path.dirname(os.path.realpath(__file__))
    with open(os.path.join(my_dir, 'perf_files/perf.data.report.0')) as f:
      return f.read()

  def testParserParsesRealWorldPerfReport(self):
    report = ParseStandardPerfReport(self._ReadRealPerfReport())
    self.assertItemsEqual(['cycles', 'instructions'], report.keys())

    # Arbitrarily selected known percentages from the perf report.
    known_cycles_percentages = {
        '0xffffffffa4a1f1c9': 0.66,
        '0x0000115bb7ba9b54': 0.47,
        '0x0000000000082e08': 0.00,
        '0xffffffffa4a13e63': 0.00,
    }
    report_cycles = report['cycles']
    self.assertEqual(len(report_cycles), 214)
    for k, v in known_cycles_percentages.iteritems():
      self.assertIn(k, report_cycles)
      self.assertEqual(v, report_cycles[k])

    known_instrunctions_percentages = {
        '0x0000115bb6c35d7a': 1.65,
        '0x0000115bb7ba9b54': 0.67,
        '0x0000000000024f56': 0.00,
        '0xffffffffa4a0ee03': 0.00,
    }
    report_instructions = report['instructions']
    self.assertEqual(len(report_instructions), 492)
    for k, v in known_instrunctions_percentages.iteritems():
      self.assertIn(k, report_instructions)
      self.assertEqual(v, report_instructions[k])


if __name__ == '__main__':
  test_flag.SetTestMode(True)
  unittest.main()
