# Copyright 2011 Google Inc. All Rights Reserved.
# Author: kbaclawski@google.com (Krystian Baclawski)
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

from collections import namedtuple
from cStringIO import StringIO
import logging

from summary import DejaGnuTestResult


class Manifest(namedtuple('Manifest', 'tool board results')):
  """Stores a list of unsuccessful tests.

  Any line that starts with '#@' marker carries auxiliary data in form of a
  key-value pair, for example:

  #@ tool: *
  #@ board: unix

  So far tool and board parameters are recognized.  Their value can contain
  arbitrary glob expression.  Based on aforementioned parameters given manifest
  will be applied for all test results, but only in selected test runs.  Note
  that all parameters are optional.  Their default value is '*' (i.e. for all
  tools/boards).

  The meaning of lines above is as follows: corresponding test results to follow
  should only be suppressed if test run was performed on "unix" board.

  The summary line used to build the test result should have this format:

  attrlist | UNRESOLVED: gcc.dg/unroll_1.c (test for excess errors)
  ^^^^^^^^   ^^^^^^^^^^  ^^^^^^^^^^^^^^^^^ ^^^^^^^^^^^^^^^^^^^^^^^^
  optional   result      name              variant
  attributes
  """
  SUPPRESSIBLE_RESULTS = ['FAIL', 'UNRESOLVED', 'XPASS', 'ERROR']

  @classmethod
  def FromDejaGnuTestRun(cls, test_run):
    results = [result
               for result in test_run.results
               if result.result in cls.SUPPRESSIBLE_RESULTS]

    return cls(test_run.tool, test_run.board, results)

  @classmethod
  def FromFile(cls, filename):
    """Creates manifest instance from a file in format described above."""
    params = {}
    results = []

    with open(filename, 'r') as manifest_file:
      for line in manifest_file:
        if line.startswith('#@'):
          # parse a line with a parameter
          try:
            key, value = line[2:].split(':', 1)
          except ValueError:
            logging.warning('Malformed parameter line: "%s".', line)
          else:
            params[key.strip()] = value.strip()
        else:
          # remove comment
          try:
            line, _ = line.split('#', 1)
          except ValueError:
            pass

          line = line.strip()

          if line:
            # parse a line with a test result
            result = DejaGnuTestResult.FromLine(line)

            if result:
              results.append(result)
            else:
              logging.warning('Malformed test result line: "%s".', line)

    tool = params.get('tool', '*')
    board = params.get('board', '*')

    return cls(tool, board, results)

  def Generate(self):
    """Dumps manifest to string."""
    text = StringIO()

    for name in ['tool', 'board']:
      text.write('#@ {0}: {1}\n'.format(name, getattr(self, name)))

    text.write('\n')

    for result in sorted(self.results, key=lambda r: r.result):
      text.write('{0}\n'.format(result))

    return text.getvalue()

  def __iter__(self):
    return iter(self.results)
