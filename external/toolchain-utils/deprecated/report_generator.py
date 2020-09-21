#!/usr/bin/python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to  compare a baseline results file to a new results file."""

from __future__ import print_function

__author__ = 'raymes@google.com (Raymes Khoury)'

import sys
from cros_utils import logger
from cros_utils import html_tools

PASS = 'pass'
FAIL = 'fail'
NOT_EXECUTED = 'not executed'


class ResultsReport(object):
  """Class for holding report results."""

  def __init__(self, report, num_tests_executed, num_passes, num_failures,
               num_regressions):
    self.report = report
    self.num_tests_executed = num_tests_executed
    self.num_passes = num_passes
    self.num_failures = num_failures
    self.num_regressions = num_regressions

  def GetReport(self):
    return self.report

  def GetNumExecuted(self):
    return self.num_tests_executed

  def GetNumPasses(self):
    return self.num_passes

  def GetNumFailures(self):
    return self.num_failures

  def GetNumRegressions(self):
    return self.num_regressions

  def GetSummary(self):
    summary = 'Tests executed: %s\n' % str(self.num_tests_executed)
    summary += 'Tests Passing: %s\n' % str(self.num_passes)
    summary += 'Tests Failing: %s\n' % str(self.num_failures)
    summary += 'Regressions: %s\n' % str(self.num_regressions)
    return summary


def Usage():
  print('Usage: %s baseline_results new_results' % sys.argv[0])
  sys.exit(1)


def ParseResults(results_filename):
  results = []
  try:
    results_file = open(results_filename, 'rb')
    for line in results_file:
      if line.strip() != '':
        results.append(line.strip().split('\t'))
    results_file.close()
  except IOError:
    logger.GetLogger().LogWarning('Could not open results file: ' +
                                  results_filename)
  return results


def GenerateResultsReport(baseline_file, new_result_file):
  baseline_results = ParseResults(baseline_file)
  new_results = ParseResults(new_result_file)

  test_status = {}

  for new_result in new_results:
    new_test_name = new_result[0]
    new_test_result = new_result[1]
    test_status[new_test_name] = (new_test_result, NOT_EXECUTED)

  for baseline_result in baseline_results:
    baseline_test_name = baseline_result[0]
    baseline_test_result = baseline_result[1]
    if baseline_test_name in test_status:
      new_test_result = test_status[baseline_test_name][0]
      test_status[baseline_test_name] = (new_test_result, baseline_test_result)
    else:
      test_status[baseline_test_name] = (NOT_EXECUTED, baseline_test_result)

  regressions = []
  for result in test_status.keys():
    if test_status[result][0] != test_status[result][1]:
      regressions.append(result)

  num_tests_executed = len(new_results)
  num_regressions = len(regressions)
  num_passes = 0
  num_failures = 0
  for result in new_results:
    if result[1] == PASS:
      num_passes += 1
    else:
      num_failures += 1

  report = html_tools.GetPageHeader('Test Summary')
  report += html_tools.GetHeader('Test Summary')
  report += html_tools.GetListHeader()
  report += html_tools.GetListItem('Tests executed: ' + str(num_tests_executed))
  report += html_tools.GetListItem('Passes: ' + str(num_passes))
  report += html_tools.GetListItem('Failures: ' + str(num_failures))
  report += html_tools.GetListItem('Regressions: ' + str(num_regressions))
  report += html_tools.GetListFooter()
  report += html_tools.GetHeader('Regressions', 2)
  report += html_tools.GetTableHeader(['Test name', 'Expected result',
                                       'Actual result'])

  for regression in regressions:
    report += html_tools.GetTableRow([regression[:150], test_status[regression][
        1], test_status[regression][0]])
    report += '\n'
  report += html_tools.GetTableFooter()
  report += html_tools.GetHeader('All Tests', 2)
  report += html_tools.GetTableHeader(['Test name', 'Expected result',
                                       'Actual result'])
  for result in test_status.keys():
    report += html_tools.GetTableRow([result[:150], test_status[result][1],
                                      test_status[result][0]])
    report += '\n'
  report += html_tools.GetTableFooter()
  report += html_tools.GetFooter()
  return ResultsReport(report, num_tests_executed, num_passes, num_failures,
                       num_regressions)


def Main(argv):
  if len(argv) < 2:
    Usage()

  print(GenerateResultsReport(argv[1], argv[2])[0])


if __name__ == '__main__':
  Main(sys.argv)
