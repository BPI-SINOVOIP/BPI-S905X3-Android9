# Copyright 2011 Google Inc. All Rights Reserved.
# Author: kbaclawski@google.com (Krystian Baclawski)
#

from collections import defaultdict
from collections import namedtuple
from datetime import datetime
from fnmatch import fnmatch
from itertools import groupby
import logging
import os.path
import re


class DejaGnuTestResult(namedtuple('Result', 'name variant result flaky')):
  """Stores the result of a single test case."""

  # avoid adding __dict__ to the class
  __slots__ = ()

  LINE_RE = re.compile(r'([A-Z]+):\s+([\w/+.-]+)(.*)')

  @classmethod
  def FromLine(cls, line):
    """Alternate constructor which takes a string and parses it."""
    try:
      attrs, line = line.split('|', 1)

      if attrs.strip() != 'flaky':
        return None

      line = line.strip()
      flaky = True
    except ValueError:
      flaky = False

    fields = cls.LINE_RE.match(line.strip())

    if fields:
      result, path, variant = fields.groups()

      # some of the tests are generated in build dir and are issued from there,
      # because every test run is performed in randomly named tmp directory we
      # need to remove random part
      try:
        # assume that 2nd field is a test path
        path_parts = path.split('/')

        index = path_parts.index('testsuite')
        path = '/'.join(path_parts[index + 1:])
      except ValueError:
        path = '/'.join(path_parts)

      # Remove junk from test description.
      variant = variant.strip(', ')

      substitutions = [
          # remove include paths - they contain name of tmp directory
          ('-I\S+', ''),
          # compress white spaces
          ('\s+', ' ')
      ]

      for pattern, replacement in substitutions:
        variant = re.sub(pattern, replacement, variant)

      # Some tests separate last component of path by space, so actual filename
      # ends up in description instead of path part. Correct that.
      try:
        first, rest = variant.split(' ', 1)
      except ValueError:
        pass
      else:
        if first.endswith('.o'):
          path = os.path.join(path, first)
          variant = rest

      # DejaGNU framework errors don't contain path part at all, so description
      # part has to be reconstructed.
      if not any(os.path.basename(path).endswith('.%s' % suffix)
                 for suffix in ['h', 'c', 'C', 'S', 'H', 'cc', 'i', 'o']):
        variant = '%s %s' % (path, variant)
        path = ''

      # Some tests are picked up from current directory (presumably DejaGNU
      # generates some test files). Remove the prefix for these files.
      if path.startswith('./'):
        path = path[2:]

      return cls(path, variant or '', result, flaky=flaky)

  def __str__(self):
    """Returns string representation of a test result."""
    if self.flaky:
      fmt = 'flaky | '
    else:
      fmt = ''
    fmt += '{2}: {0}'
    if self.variant:
      fmt += ' {1}'
    return fmt.format(*self)


class DejaGnuTestRun(object):
  """Container for test results that were a part of single test run.

  The class stores also metadata related to the test run.

  Attributes:
    board: Name of DejaGNU board, which was used to run the tests.
    date: The date when the test run was started.
    target: Target triple.
    host: Host triple.
    tool: The tool that was tested (e.g. gcc, binutils, g++, etc.)
    results: a list of DejaGnuTestResult objects.
  """

  __slots__ = ('board', 'date', 'target', 'host', 'tool', 'results')

  def __init__(self, **kwargs):
    assert all(name in self.__slots__ for name in kwargs)

    self.results = set()
    self.date = kwargs.get('date', datetime.now())

    for name in ('board', 'target', 'tool', 'host'):
      setattr(self, name, kwargs.get(name, 'unknown'))

  @classmethod
  def FromFile(cls, filename):
    """Alternate constructor - reads a DejaGNU output file."""
    test_run = cls()
    test_run.FromDejaGnuOutput(filename)
    test_run.CleanUpTestResults()
    return test_run

  @property
  def summary(self):
    """Returns a summary as {ResultType -> Count} dictionary."""
    summary = defaultdict(int)

    for r in self.results:
      summary[r.result] += 1

    return summary

  def _ParseBoard(self, fields):
    self.board = fields.group(1).strip()

  def _ParseDate(self, fields):
    self.date = datetime.strptime(fields.group(2).strip(), '%a %b %d %X %Y')

  def _ParseTarget(self, fields):
    self.target = fields.group(2).strip()

  def _ParseHost(self, fields):
    self.host = fields.group(2).strip()

  def _ParseTool(self, fields):
    self.tool = fields.group(1).strip()

  def FromDejaGnuOutput(self, filename):
    """Read in and parse DejaGNU output file."""

    logging.info('Reading "%s" DejaGNU output file.', filename)

    with open(filename, 'r') as report:
      lines = [line.strip() for line in report.readlines() if line.strip()]

    parsers = ((re.compile(r'Running target (.*)'), self._ParseBoard),
               (re.compile(r'Test Run By (.*) on (.*)'), self._ParseDate),
               (re.compile(r'=== (.*) tests ==='), self._ParseTool),
               (re.compile(r'Target(\s+)is (.*)'), self._ParseTarget),
               (re.compile(r'Host(\s+)is (.*)'), self._ParseHost))

    for line in lines:
      result = DejaGnuTestResult.FromLine(line)

      if result:
        self.results.add(result)
      else:
        for regexp, parser in parsers:
          fields = regexp.match(line)
          if fields:
            parser(fields)
            break

    logging.debug('DejaGNU output file parsed successfully.')
    logging.debug(self)

  def CleanUpTestResults(self):
    """Remove certain test results considered to be spurious.

    1) Large number of test reported as UNSUPPORTED are also marked as
       UNRESOLVED. If that's the case remove latter result.
    2) If a test is performed on compiler output and for some reason compiler
       fails, we don't want to report all failures that depend on the former.
    """
    name_key = lambda v: v.name
    results_by_name = sorted(self.results, key=name_key)

    for name, res_iter in groupby(results_by_name, key=name_key):
      results = set(res_iter)

      # If DejaGnu was unable to compile a test it will create following result:
      failed = DejaGnuTestResult(name, '(test for excess errors)', 'FAIL',
                                 False)

      # If a test compilation failed, remove all results that are dependent.
      if failed in results:
        dependants = set(filter(lambda r: r.result != 'FAIL', results))

        self.results -= dependants

        for res in dependants:
          logging.info('Removed {%s} dependance.', res)

      # Remove all UNRESOLVED results that were also marked as UNSUPPORTED.
      unresolved = [res._replace(result='UNRESOLVED')
                    for res in results if res.result == 'UNSUPPORTED']

      for res in unresolved:
        if res in self.results:
          self.results.remove(res)
          logging.info('Removed {%s} duplicate.', res)

  def _IsApplicable(self, manifest):
    """Checks if test results need to be reconsidered based on the manifest."""
    check_list = [(self.tool, manifest.tool), (self.board, manifest.board)]

    return all(fnmatch(text, pattern) for text, pattern in check_list)

  def SuppressTestResults(self, manifests):
    """Suppresses all test results listed in manifests."""

    # Get a set of tests results that are going to be suppressed if they fail.
    manifest_results = set()

    for manifest in filter(self._IsApplicable, manifests):
      manifest_results |= set(manifest.results)

    suppressed_results = self.results & manifest_results

    for result in sorted(suppressed_results):
      logging.debug('Result suppressed for {%s}.', result)

      new_result = '!' + result.result

      # Mark result suppression as applied.
      manifest_results.remove(result)

      # Rewrite test result.
      self.results.remove(result)
      self.results.add(result._replace(result=new_result))

    for result in sorted(manifest_results):
      logging.warning('Result {%s} listed in manifest but not suppressed.',
                      result)

  def __str__(self):
    return '{0}, {1} @{2} on {3}'.format(self.target, self.tool, self.board,
                                         self.date)
