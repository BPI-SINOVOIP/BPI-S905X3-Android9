# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A module to handle the report format."""
from __future__ import print_function

import datetime
import functools
import itertools
import json
import os
import re

from cros_utils.tabulator import AmeanResult
from cros_utils.tabulator import Cell
from cros_utils.tabulator import CoeffVarFormat
from cros_utils.tabulator import CoeffVarResult
from cros_utils.tabulator import Column
from cros_utils.tabulator import Format
from cros_utils.tabulator import GmeanRatioResult
from cros_utils.tabulator import LiteralResult
from cros_utils.tabulator import MaxResult
from cros_utils.tabulator import MinResult
from cros_utils.tabulator import PValueFormat
from cros_utils.tabulator import PValueResult
from cros_utils.tabulator import RatioFormat
from cros_utils.tabulator import RawResult
from cros_utils.tabulator import StdResult
from cros_utils.tabulator import TableFormatter
from cros_utils.tabulator import TableGenerator
from cros_utils.tabulator import TablePrinter
from update_telemetry_defaults import TelemetryDefaults

from column_chart import ColumnChart
from results_organizer import OrganizeResults

import results_report_templates as templates


def ParseChromeosImage(chromeos_image):
  """Parse the chromeos_image string for the image and version.

  The chromeos_image string will probably be in one of two formats:
  1: <path-to-chroot>/src/build/images/<board>/<ChromeOS-version>.<datetime>/ \
     chromiumos_test_image.bin
  2: <path-to-chroot>/chroot/tmp/<buildbot-build>/<ChromeOS-version>/ \
      chromiumos_test_image.bin

  We parse these strings to find the 'chromeos_version' to store in the
  json archive (without the .datatime bit in the first case); and also
  the 'chromeos_image', which would be all of the first case, but only the
  part after '/chroot/tmp' in the second case.

  Args:
      chromeos_image: string containing the path to the chromeos_image that
      crosperf used for the test.

  Returns:
      version, image: The results of parsing the input string, as explained
      above.
  """
  # Find the Chromeos Version, e.g. R45-2345.0.0.....
  # chromeos_image should have been something like:
  # <path>/<board-trybot-release>/<chromeos-version>/chromiumos_test_image.bin"
  if chromeos_image.endswith('/chromiumos_test_image.bin'):
    full_version = chromeos_image.split('/')[-2]
    # Strip the date and time off of local builds (which have the format
    # "R43-2345.0.0.date-and-time").
    version, _ = os.path.splitext(full_version)
  else:
    version = ''

  # Find the chromeos image.  If it's somewhere in .../chroot/tmp/..., then
  # it's an official image that got downloaded, so chop off the download path
  # to make the official image name more clear.
  official_image_path = '/chroot/tmp'
  if official_image_path in chromeos_image:
    image = chromeos_image.split(official_image_path, 1)[1]
  else:
    image = chromeos_image
  return version, image


def _AppendUntilLengthIs(gen, the_list, target_len):
  """Appends to `list` until `list` is `target_len` elements long.

  Uses `gen` to generate elements.
  """
  the_list.extend(gen() for _ in xrange(target_len - len(the_list)))
  return the_list


def _FilterPerfReport(event_threshold, report):
  """Filters out entries with `< event_threshold` percent in a perf report."""

  def filter_dict(m):
    return {
        fn_name: pct
        for fn_name, pct in m.iteritems() if pct >= event_threshold
    }

  return {event: filter_dict(m) for event, m in report.iteritems()}


class _PerfTable(object):
  """Generates dicts from a perf table.

  Dicts look like:
  {'benchmark_name': {'perf_event_name': [LabelData]}}
  where LabelData is a list of perf dicts, each perf dict coming from the same
  label.
  Each perf dict looks like {'function_name': 0.10, ...} (where 0.10 is the
  percentage of time spent in function_name).
  """

  def __init__(self,
               benchmark_names_and_iterations,
               label_names,
               read_perf_report,
               event_threshold=None):
    """Constructor.

    read_perf_report is a function that takes a label name, benchmark name, and
    benchmark iteration, and returns a dictionary describing the perf output for
    that given run.
    """
    self.event_threshold = event_threshold
    self._label_indices = {name: i for i, name in enumerate(label_names)}
    self.perf_data = {}
    for label in label_names:
      for bench_name, bench_iterations in benchmark_names_and_iterations:
        for i in xrange(bench_iterations):
          report = read_perf_report(label, bench_name, i)
          self._ProcessPerfReport(report, label, bench_name, i)

  def _ProcessPerfReport(self, perf_report, label, benchmark_name, iteration):
    """Add the data from one run to the dict."""
    perf_of_run = perf_report
    if self.event_threshold is not None:
      perf_of_run = _FilterPerfReport(self.event_threshold, perf_report)
    if benchmark_name not in self.perf_data:
      self.perf_data[benchmark_name] = {event: [] for event in perf_of_run}
    ben_data = self.perf_data[benchmark_name]
    label_index = self._label_indices[label]
    for event in ben_data:
      _AppendUntilLengthIs(list, ben_data[event], label_index + 1)
      data_for_label = ben_data[event][label_index]
      _AppendUntilLengthIs(dict, data_for_label, iteration + 1)
      data_for_label[iteration] = perf_of_run[event] if perf_of_run else {}


def _GetResultsTableHeader(ben_name, iterations):
  benchmark_info = ('Benchmark:  {0};  Iterations: {1}'.format(
      ben_name, iterations))
  cell = Cell()
  cell.string_value = benchmark_info
  cell.header = True
  return [[cell]]


def _ParseColumn(columns, iteration):
  new_column = []
  for column in columns:
    if column.result.__class__.__name__ != 'RawResult':
      new_column.append(column)
    else:
      new_column.extend(
          Column(LiteralResult(i), Format(), str(i + 1))
          for i in xrange(iteration))
  return new_column


def _GetTables(benchmark_results, columns, table_type):
  iter_counts = benchmark_results.iter_counts
  result = benchmark_results.run_keyvals
  tables = []
  for bench_name, runs in result.iteritems():
    iterations = iter_counts[bench_name]
    ben_table = _GetResultsTableHeader(bench_name, iterations)

    all_runs_empty = all(not dict for label in runs for dict in label)
    if all_runs_empty:
      cell = Cell()
      cell.string_value = ('This benchmark contains no result.'
                           ' Is the benchmark name valid?')
      cell_table = [[cell]]
    else:
      table = TableGenerator(runs, benchmark_results.label_names).GetTable()
      parsed_columns = _ParseColumn(columns, iterations)
      tf = TableFormatter(table, parsed_columns)
      cell_table = tf.GetCellTable(table_type)
    tables.append(ben_table)
    tables.append(cell_table)
  return tables


def _GetPerfTables(benchmark_results, columns, table_type):
  p_table = _PerfTable(benchmark_results.benchmark_names_and_iterations,
                       benchmark_results.label_names,
                       benchmark_results.read_perf_report)

  tables = []
  for benchmark in p_table.perf_data:
    iterations = benchmark_results.iter_counts[benchmark]
    ben_table = _GetResultsTableHeader(benchmark, iterations)
    tables.append(ben_table)
    benchmark_data = p_table.perf_data[benchmark]
    table = []
    for event in benchmark_data:
      tg = TableGenerator(
          benchmark_data[event],
          benchmark_results.label_names,
          sort=TableGenerator.SORT_BY_VALUES_DESC)
      table = tg.GetTable(ResultsReport.PERF_ROWS)
      parsed_columns = _ParseColumn(columns, iterations)
      tf = TableFormatter(table, parsed_columns)
      tf.GenerateCellTable(table_type)
      tf.AddColumnName()
      tf.AddLabelName()
      tf.AddHeader(str(event))
      table = tf.GetCellTable(table_type, headers=False)
      tables.append(table)
  return tables


class ResultsReport(object):
  """Class to handle the report format."""
  MAX_COLOR_CODE = 255
  PERF_ROWS = 5

  def __init__(self, results):
    self.benchmark_results = results

  def _GetTablesWithColumns(self, columns, table_type, perf):
    get_tables = _GetPerfTables if perf else _GetTables
    return get_tables(self.benchmark_results, columns, table_type)

  def GetFullTables(self, perf=False):
    columns = [
        Column(RawResult(), Format()), Column(MinResult(), Format()), Column(
            MaxResult(), Format()), Column(AmeanResult(), Format()), Column(
                StdResult(), Format(), 'StdDev'),
        Column(CoeffVarResult(), CoeffVarFormat(), 'StdDev/Mean'), Column(
            GmeanRatioResult(), RatioFormat(), 'GmeanSpeedup'), Column(
                PValueResult(), PValueFormat(), 'p-value')
    ]
    return self._GetTablesWithColumns(columns, 'full', perf)

  def GetSummaryTables(self, perf=False):
    columns = [
        Column(AmeanResult(), Format()), Column(StdResult(), Format(),
                                                'StdDev'),
        Column(CoeffVarResult(), CoeffVarFormat(), 'StdDev/Mean'), Column(
            GmeanRatioResult(), RatioFormat(), 'GmeanSpeedup'), Column(
                PValueResult(), PValueFormat(), 'p-value')
    ]
    return self._GetTablesWithColumns(columns, 'summary', perf)


def _PrintTable(tables, out_to):
  # tables may be None.
  if not tables:
    return ''

  if out_to == 'HTML':
    out_type = TablePrinter.HTML
  elif out_to == 'PLAIN':
    out_type = TablePrinter.PLAIN
  elif out_to == 'CONSOLE':
    out_type = TablePrinter.CONSOLE
  elif out_to == 'TSV':
    out_type = TablePrinter.TSV
  elif out_to == 'EMAIL':
    out_type = TablePrinter.EMAIL
  else:
    raise ValueError('Invalid out_to value: %s' % (out_to,))

  printers = (TablePrinter(table, out_type) for table in tables)
  return ''.join(printer.Print() for printer in printers)


class TextResultsReport(ResultsReport):
  """Class to generate text result report."""

  H1_STR = '==========================================='
  H2_STR = '-------------------------------------------'

  def __init__(self, results, email=False, experiment=None):
    super(TextResultsReport, self).__init__(results)
    self.email = email
    self.experiment = experiment

  @staticmethod
  def _MakeTitle(title):
    header_line = TextResultsReport.H1_STR
    # '' at the end gives one newline.
    return '\n'.join([header_line, title, header_line, ''])

  @staticmethod
  def _MakeSection(title, body):
    header_line = TextResultsReport.H2_STR
    # '\n' at the end gives us two newlines.
    return '\n'.join([header_line, title, header_line, body, '\n'])

  @staticmethod
  def FromExperiment(experiment, email=False):
    results = BenchmarkResults.FromExperiment(experiment)
    return TextResultsReport(results, email, experiment)

  def GetStatusTable(self):
    """Generate the status table by the tabulator."""
    table = [['', '']]
    columns = [
        Column(LiteralResult(iteration=0), Format(), 'Status'), Column(
            LiteralResult(iteration=1), Format(), 'Failing Reason')
    ]

    for benchmark_run in self.experiment.benchmark_runs:
      status = [
          benchmark_run.name,
          [benchmark_run.timeline.GetLastEvent(), benchmark_run.failure_reason]
      ]
      table.append(status)
    cell_table = TableFormatter(table, columns).GetCellTable('status')
    return [cell_table]

  def GetReport(self):
    """Generate the report for email and console."""
    output_type = 'EMAIL' if self.email else 'CONSOLE'
    experiment = self.experiment

    sections = []
    if experiment is not None:
      title_contents = "Results report for '%s'" % (experiment.name,)
    else:
      title_contents = 'Results report'
    sections.append(self._MakeTitle(title_contents))

    summary_table = _PrintTable(self.GetSummaryTables(perf=False), output_type)
    sections.append(self._MakeSection('Summary', summary_table))

    if experiment is not None:
      table = _PrintTable(self.GetStatusTable(), output_type)
      sections.append(self._MakeSection('Benchmark Run Status', table))

    perf_table = _PrintTable(self.GetSummaryTables(perf=True), output_type)
    if perf_table:
      sections.append(self._MakeSection('Perf Data', perf_table))

    if experiment is not None:
      experiment_file = experiment.experiment_file
      sections.append(self._MakeSection('Experiment File', experiment_file))

      cpu_info = experiment.machine_manager.GetAllCPUInfo(experiment.labels)
      sections.append(self._MakeSection('CPUInfo', cpu_info))

    return '\n'.join(sections)


def _GetHTMLCharts(label_names, test_results):
  charts = []
  for item, runs in test_results.iteritems():
    # Fun fact: label_names is actually *entirely* useless as a param, since we
    # never add headers. We still need to pass it anyway.
    table = TableGenerator(runs, label_names).GetTable()
    columns = [
        Column(AmeanResult(), Format()), Column(MinResult(), Format()), Column(
            MaxResult(), Format())
    ]
    tf = TableFormatter(table, columns)
    data_table = tf.GetCellTable('full', headers=False)

    for cur_row_data in data_table:
      test_key = cur_row_data[0].string_value
      title = '{0}: {1}'.format(item, test_key.replace('/', ''))
      chart = ColumnChart(title, 300, 200)
      chart.AddColumn('Label', 'string')
      chart.AddColumn('Average', 'number')
      chart.AddColumn('Min', 'number')
      chart.AddColumn('Max', 'number')
      chart.AddSeries('Min', 'line', 'black')
      chart.AddSeries('Max', 'line', 'black')
      cur_index = 1
      for label in label_names:
        chart.AddRow([
            label, cur_row_data[cur_index].value,
            cur_row_data[cur_index + 1].value, cur_row_data[cur_index + 2].value
        ])
        if isinstance(cur_row_data[cur_index].value, str):
          chart = None
          break
        cur_index += 3
      if chart:
        charts.append(chart)
  return charts


class HTMLResultsReport(ResultsReport):
  """Class to generate html result report."""

  def __init__(self, benchmark_results, experiment=None):
    super(HTMLResultsReport, self).__init__(benchmark_results)
    self.experiment = experiment

  @staticmethod
  def FromExperiment(experiment):
    return HTMLResultsReport(
        BenchmarkResults.FromExperiment(experiment), experiment=experiment)

  def GetReport(self):
    label_names = self.benchmark_results.label_names
    test_results = self.benchmark_results.run_keyvals
    charts = _GetHTMLCharts(label_names, test_results)
    chart_javascript = ''.join(chart.GetJavascript() for chart in charts)
    chart_divs = ''.join(chart.GetDiv() for chart in charts)

    summary_table = self.GetSummaryTables()
    full_table = self.GetFullTables()
    perf_table = self.GetSummaryTables(perf=True)
    experiment_file = ''
    if self.experiment is not None:
      experiment_file = self.experiment.experiment_file
    # Use kwargs for sanity, and so that testing is a bit easier.
    return templates.GenerateHTMLPage(
        perf_table=perf_table,
        chart_js=chart_javascript,
        summary_table=summary_table,
        print_table=_PrintTable,
        chart_divs=chart_divs,
        full_table=full_table,
        experiment_file=experiment_file)


def ParseStandardPerfReport(report_data):
  """Parses the output of `perf report`.

  It'll parse the following:
  {{garbage}}
  # Samples: 1234M of event 'foo'

  1.23% command shared_object location function::name

  1.22% command shared_object location function2::name

  # Samples: 999K of event 'bar'

  0.23% command shared_object location function3::name
  {{etc.}}

  Into:
    {'foo': {'function::name': 1.23, 'function2::name': 1.22},
     'bar': {'function3::name': 0.23, etc.}}
  """
  # This function fails silently on its if it's handed a string (as opposed to a
  # list of lines). So, auto-split if we do happen to get a string.
  if isinstance(report_data, basestring):
    report_data = report_data.splitlines()

  # Samples: N{K,M,G} of event 'event-name'
  samples_regex = re.compile(r"#\s+Samples: \d+\S? of event '([^']+)'")

  # We expect lines like:
  # N.NN%  command  samples  shared_object  [location] symbol
  #
  # Note that we're looking at stripped lines, so there is no space at the
  # start.
  perf_regex = re.compile(r'^(\d+(?:.\d*)?)%'  # N.NN%
                          r'\s*\d+'  # samples count (ignored)
                          r'\s*\S+'  # command (ignored)
                          r'\s*\S+'  # shared_object (ignored)
                          r'\s*\[.\]'  # location (ignored)
                          r'\s*(\S.+)'  # function
                         )

  stripped_lines = (l.strip() for l in report_data)
  nonempty_lines = (l for l in stripped_lines if l)
  # Ignore all lines before we see samples_regex
  interesting_lines = itertools.dropwhile(lambda x: not samples_regex.match(x),
                                          nonempty_lines)

  first_sample_line = next(interesting_lines, None)
  # Went through the entire file without finding a 'samples' header. Quit.
  if first_sample_line is None:
    return {}

  sample_name = samples_regex.match(first_sample_line).group(1)
  current_result = {}
  results = {sample_name: current_result}
  for line in interesting_lines:
    samples_match = samples_regex.match(line)
    if samples_match:
      sample_name = samples_match.group(1)
      current_result = {}
      results[sample_name] = current_result
      continue

    match = perf_regex.match(line)
    if not match:
      continue
    percentage_str, func_name = match.groups()
    try:
      percentage = float(percentage_str)
    except ValueError:
      # Couldn't parse it; try to be "resilient".
      continue
    current_result[func_name] = percentage
  return results


def _ReadExperimentPerfReport(results_directory, label_name, benchmark_name,
                              benchmark_iteration):
  """Reads a perf report for the given benchmark. Returns {} on failure.

  The result should be a map of maps; it should look like:
  {perf_event_name: {function_name: pct_time_spent}}, e.g.
  {'cpu_cycles': {'_malloc': 10.0, '_free': 0.3, ...}}
  """
  raw_dir_name = label_name + benchmark_name + str(benchmark_iteration + 1)
  dir_name = ''.join(c for c in raw_dir_name if c.isalnum())
  file_name = os.path.join(results_directory, dir_name, 'perf.data.report.0')
  try:
    with open(file_name) as in_file:
      return ParseStandardPerfReport(in_file)
  except IOError:
    # Yes, we swallow any IO-related errors.
    return {}


# Split out so that testing (specifically: mocking) is easier
def _ExperimentToKeyvals(experiment, for_json_report):
  """Converts an experiment to keyvals."""
  return OrganizeResults(
      experiment.benchmark_runs, experiment.labels, json_report=for_json_report)


class BenchmarkResults(object):
  """The minimum set of fields that any ResultsReport will take."""

  def __init__(self,
               label_names,
               benchmark_names_and_iterations,
               run_keyvals,
               read_perf_report=None):
    if read_perf_report is None:

      def _NoPerfReport(*_args, **_kwargs):
        return {}

      read_perf_report = _NoPerfReport

    self.label_names = label_names
    self.benchmark_names_and_iterations = benchmark_names_and_iterations
    self.iter_counts = dict(benchmark_names_and_iterations)
    self.run_keyvals = run_keyvals
    self.read_perf_report = read_perf_report

  @staticmethod
  def FromExperiment(experiment, for_json_report=False):
    label_names = [label.name for label in experiment.labels]
    benchmark_names_and_iterations = [(benchmark.name, benchmark.iterations)
                                      for benchmark in experiment.benchmarks]
    run_keyvals = _ExperimentToKeyvals(experiment, for_json_report)
    read_perf_report = functools.partial(_ReadExperimentPerfReport,
                                         experiment.results_directory)
    return BenchmarkResults(label_names, benchmark_names_and_iterations,
                            run_keyvals, read_perf_report)


def _GetElemByName(name, from_list):
  """Gets an element from the given list by its name field.

  Raises an error if it doesn't find exactly one match.
  """
  elems = [e for e in from_list if e.name == name]
  if len(elems) != 1:
    raise ValueError('Expected 1 item named %s, found %d' % (name, len(elems)))
  return elems[0]


def _Unlist(l):
  """If l is a list, extracts the first element of l. Otherwise, returns l."""
  return l[0] if isinstance(l, list) else l


class JSONResultsReport(ResultsReport):
  """Class that generates JSON reports for experiments."""

  def __init__(self,
               benchmark_results,
               date=None,
               time=None,
               experiment=None,
               json_args=None):
    """Construct a JSONResultsReport.

    json_args is the dict of arguments we pass to json.dumps in GetReport().
    """
    super(JSONResultsReport, self).__init__(benchmark_results)

    defaults = TelemetryDefaults()
    defaults.ReadDefaultsFile()
    summary_field_defaults = defaults.GetDefault()
    if summary_field_defaults is None:
      summary_field_defaults = {}
    self.summary_field_defaults = summary_field_defaults

    if json_args is None:
      json_args = {}
    self.json_args = json_args

    self.experiment = experiment
    if not date:
      timestamp = datetime.datetime.strftime(datetime.datetime.now(),
                                             '%Y-%m-%d %H:%M:%S')
      date, time = timestamp.split(' ')
    self.date = date
    self.time = time

  @staticmethod
  def FromExperiment(experiment, date=None, time=None, json_args=None):
    benchmark_results = BenchmarkResults.FromExperiment(
        experiment, for_json_report=True)
    return JSONResultsReport(benchmark_results, date, time, experiment,
                             json_args)

  def GetReportObjectIgnoringExperiment(self):
    """Gets the JSON report object specifically for the output data.

    Ignores any experiment-specific fields (e.g. board, machine checksum, ...).
    """
    benchmark_results = self.benchmark_results
    label_names = benchmark_results.label_names
    summary_field_defaults = self.summary_field_defaults
    final_results = []
    for test, test_results in benchmark_results.run_keyvals.iteritems():
      for label_name, label_results in zip(label_names, test_results):
        for iter_results in label_results:
          passed = iter_results.get('retval') == 0
          json_results = {
              'date': self.date,
              'time': self.time,
              'label': label_name,
              'test_name': test,
              'pass': passed,
          }
          final_results.append(json_results)

          if not passed:
            continue

          # Get overall results.
          summary_fields = summary_field_defaults.get(test)
          if summary_fields is not None:
            value = []
            json_results['overall_result'] = value
            for f in summary_fields:
              v = iter_results.get(f)
              if v is None:
                continue
              # New telemetry results format: sometimes we get a list of lists
              # now.
              v = _Unlist(_Unlist(v))
              value.append((f, float(v)))

          # Get detailed results.
          detail_results = {}
          json_results['detailed_results'] = detail_results
          for k, v in iter_results.iteritems():
            if k == 'retval' or k == 'PASS' or k == ['PASS'] or v == 'PASS':
              continue

            v = _Unlist(v)
            if 'machine' in k:
              json_results[k] = v
            elif v is not None:
              if isinstance(v, list):
                detail_results[k] = [float(d) for d in v]
              else:
                detail_results[k] = float(v)
    return final_results

  def GetReportObject(self):
    """Generate the JSON report, returning it as a python object."""
    report_list = self.GetReportObjectIgnoringExperiment()
    if self.experiment is not None:
      self._AddExperimentSpecificFields(report_list)
    return report_list

  def _AddExperimentSpecificFields(self, report_list):
    """Add experiment-specific data to the JSON report."""
    board = self.experiment.labels[0].board
    manager = self.experiment.machine_manager
    for report in report_list:
      label_name = report['label']
      label = _GetElemByName(label_name, self.experiment.labels)

      img_path = os.path.realpath(os.path.expanduser(label.chromeos_image))
      ver, img = ParseChromeosImage(img_path)

      report.update({
          'board': board,
          'chromeos_image': img,
          'chromeos_version': ver,
          'chrome_version': label.chrome_version,
          'compiler': label.compiler
      })

      if not report['pass']:
        continue
      if 'machine_checksum' not in report:
        report['machine_checksum'] = manager.machine_checksum[label_name]
      if 'machine_string' not in report:
        report['machine_string'] = manager.machine_checksum_string[label_name]

  def GetReport(self):
    """Dump the results of self.GetReportObject() to a string as JSON."""
    # This exists for consistency with the other GetReport methods.
    # Specifically, they all return strings, so it's a bit awkward if the JSON
    # results reporter returns an object.
    return json.dumps(self.GetReportObject(), **self.json_args)
