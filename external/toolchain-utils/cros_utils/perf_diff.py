#!/usr/bin/env python2
# Copyright 2012 Google Inc. All Rights Reserved.
"""One-line documentation for perf_diff module.

A detailed description of perf_diff.
"""

from __future__ import print_function

__author__ = 'asharif@google.com (Ahmad Sharif)'

import argparse
import re
import sys

import misc
import tabulator

ROWS_TO_SHOW = 'Rows_to_show_in_the_perf_table'
TOTAL_EVENTS = 'Total_events_of_this_profile'


def GetPerfDictFromReport(report_file):
  output = {}
  perf_report = PerfReport(report_file)
  for k, v in perf_report.sections.items():
    if k not in output:
      output[k] = {}
    output[k][ROWS_TO_SHOW] = 0
    output[k][TOTAL_EVENTS] = 0
    for function in v.functions:
      out_key = '%s' % (function.name)
      output[k][out_key] = function.count
      output[k][TOTAL_EVENTS] += function.count
      if function.percent > 1:
        output[k][ROWS_TO_SHOW] += 1
  return output


def _SortDictionaryByValue(d):
  l = [(k, v) for (k, v) in d.iteritems()]

  def GetFloat(x):
    if misc.IsFloat(x):
      return float(x)
    else:
      return x

  sorted_l = sorted(l, key=lambda x: GetFloat(x[1]))
  sorted_l.reverse()
  return [f[0] for f in sorted_l]


class Tabulator(object):
  """Make tables."""

  def __init__(self, all_dicts):
    self._all_dicts = all_dicts

  def PrintTable(self):
    for dicts in self._all_dicts:
      self.PrintTableHelper(dicts)

  def PrintTableHelper(self, dicts):
    """Transfrom dicts to tables."""
    fields = {}
    for d in dicts:
      for f in d.keys():
        if f not in fields:
          fields[f] = d[f]
        else:
          fields[f] = max(fields[f], d[f])
    table = []
    header = ['name']
    for i in range(len(dicts)):
      header.append(i)

    table.append(header)

    sorted_fields = _SortDictionaryByValue(fields)

    for f in sorted_fields:
      row = [f]
      for d in dicts:
        if f in d:
          row.append(d[f])
        else:
          row.append('0')
      table.append(row)

    print(tabulator.GetSimpleTable(table))


class Function(object):
  """Function for formatting."""

  def __init__(self):
    self.count = 0
    self.name = ''
    self.percent = 0


class Section(object):
  """Section formatting."""

  def __init__(self, contents):
    self.name = ''
    self.raw_contents = contents
    self._ParseSection()

  def _ParseSection(self):
    matches = re.findall(r'Events: (\w+)\s+(.*)', self.raw_contents)
    assert len(matches) <= 1, 'More than one event found in 1 section'
    if not matches:
      return
    match = matches[0]
    self.name = match[1]
    self.count = misc.UnitToNumber(match[0])

    self.functions = []
    for line in self.raw_contents.splitlines():
      if not line.strip():
        continue
      if '%' not in line:
        continue
      if not line.startswith('#'):
        fields = [f for f in line.split(' ') if f]
        function = Function()
        function.percent = float(fields[0].strip('%'))
        function.count = int(fields[1])
        function.name = ' '.join(fields[2:])
        self.functions.append(function)


class PerfReport(object):
  """Get report from raw report."""

  def __init__(self, perf_file):
    self.perf_file = perf_file
    self._ReadFile()
    self.sections = {}
    self.metadata = {}
    self._section_contents = []
    self._section_header = ''
    self._SplitSections()
    self._ParseSections()
    self._ParseSectionHeader()

  def _ParseSectionHeader(self):
    """Parse a header of a perf report file."""
    # The "captured on" field is inaccurate - this actually refers to when the
    # report was generated, not when the data was captured.
    for line in self._section_header.splitlines():
      line = line[2:]
      if ':' in line:
        key, val = line.strip().split(':', 1)
        key = key.strip()
        val = val.strip()
        self.metadata[key] = val

  def _ReadFile(self):
    self._perf_contents = open(self.perf_file).read()

  def _ParseSections(self):
    self.event_counts = {}
    self.sections = {}
    for section_content in self._section_contents:
      section = Section(section_content)
      section.name = self._GetHumanReadableName(section.name)
      self.sections[section.name] = section

  # TODO(asharif): Do this better.
  def _GetHumanReadableName(self, section_name):
    if not 'raw' in section_name:
      return section_name
    raw_number = section_name.strip().split(' ')[-1]
    for line in self._section_header.splitlines():
      if raw_number in line:
        name = line.strip().split(' ')[5]
        return name

  def _SplitSections(self):
    self._section_contents = []
    indices = [m.start() for m in re.finditer('# Events:', self._perf_contents)]
    indices.append(len(self._perf_contents))
    for i in range(len(indices) - 1):
      section_content = self._perf_contents[indices[i]:indices[i + 1]]
      self._section_contents.append(section_content)
    self._section_header = ''
    if indices:
      self._section_header = self._perf_contents[0:indices[0]]


class PerfDiffer(object):
  """Perf differ class."""

  def __init__(self, reports, num_symbols, common_only):
    self._reports = reports
    self._num_symbols = num_symbols
    self._common_only = common_only
    self._common_function_names = {}

  def DoDiff(self):
    """The function that does the diff."""
    section_names = self._FindAllSections()

    filename_dicts = []
    summary_dicts = []
    for report in self._reports:
      d = {}
      filename_dicts.append({'file': report.perf_file})
      for section_name in section_names:
        if section_name in report.sections:
          d[section_name] = report.sections[section_name].count
      summary_dicts.append(d)

    all_dicts = [filename_dicts, summary_dicts]

    for section_name in section_names:
      function_names = self._GetTopFunctions(section_name, self._num_symbols)
      self._FindCommonFunctions(section_name)
      dicts = []
      for report in self._reports:
        d = {}
        if section_name in report.sections:
          section = report.sections[section_name]

          # Get a common scaling factor for this report.
          common_scaling_factor = self._GetCommonScalingFactor(section)

          for function in section.functions:
            if function.name in function_names:
              key = '%s %s' % (section.name, function.name)
              d[key] = function.count
              # Compute a factor to scale the function count by in common_only
              # mode.
              if self._common_only and (
                  function.name in self._common_function_names[section.name]):
                d[key + ' scaled'] = common_scaling_factor * function.count
        dicts.append(d)

      all_dicts.append(dicts)

    mytabulator = Tabulator(all_dicts)
    mytabulator.PrintTable()

  def _FindAllSections(self):
    sections = {}
    for report in self._reports:
      for section in report.sections.values():
        if section.name not in sections:
          sections[section.name] = section.count
        else:
          sections[section.name] = max(sections[section.name], section.count)
    return _SortDictionaryByValue(sections)

  def _GetCommonScalingFactor(self, section):
    unique_count = self._GetCount(
        section, lambda x: x in self._common_function_names[section.name])
    return 100.0 / unique_count

  def _GetCount(self, section, filter_fun=None):
    total_count = 0
    for function in section.functions:
      if not filter_fun or filter_fun(function.name):
        total_count += int(function.count)
    return total_count

  def _FindCommonFunctions(self, section_name):
    function_names_list = []
    for report in self._reports:
      if section_name in report.sections:
        section = report.sections[section_name]
        function_names = [f.name for f in section.functions]
        function_names_list.append(function_names)

    self._common_function_names[section_name] = (
        reduce(set.intersection, map(set, function_names_list)))

  def _GetTopFunctions(self, section_name, num_functions):
    all_functions = {}
    for report in self._reports:
      if section_name in report.sections:
        section = report.sections[section_name]
        for f in section.functions[:num_functions]:
          if f.name in all_functions:
            all_functions[f.name] = max(all_functions[f.name], f.count)
          else:
            all_functions[f.name] = f.count
    # FIXME(asharif): Don't really need to sort these...
    return _SortDictionaryByValue(all_functions)

  def _GetFunctionsDict(self, section, function_names):
    d = {}
    for function in section.functions:
      if function.name in function_names:
        d[function.name] = function.count
    return d


def Main(argv):
  """The entry of the main."""
  parser = argparse.ArgumentParser()
  parser.add_argument('-n',
                      '--num_symbols',
                      dest='num_symbols',
                      default='5',
                      help='The number of symbols to show.')
  parser.add_argument('-c',
                      '--common_only',
                      dest='common_only',
                      action='store_true',
                      default=False,
                      help='Diff common symbols only.')

  options, args = parser.parse_known_args(argv)

  try:
    reports = []
    for report in args[1:]:
      report = PerfReport(report)
      reports.append(report)
    pd = PerfDiffer(reports, int(options.num_symbols), options.common_only)
    pd.DoDiff()
  finally:
    pass

  return 0


if __name__ == '__main__':
  sys.exit(Main(sys.argv))
