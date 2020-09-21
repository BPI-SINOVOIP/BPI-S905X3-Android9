#!/usr/bin/python

# Script to compare testsuite failures against a list of known-to-fail
# tests.

# Contributed by Diego Novillo <dnovillo@google.com>
# Overhaul by Krystian Baclawski <kbaclawski@google.com>
#
# Copyright (C) 2011 Free Software Foundation, Inc.
#
# This file is part of GCC.
#
# GCC is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3, or (at your option)
# any later version.
#
# GCC is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with GCC; see the file COPYING.  If not, write to
# the Free Software Foundation, 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301, USA.
"""This script provides a coarser XFAILing mechanism that requires no
detailed DejaGNU markings.  This is useful in a variety of scenarios:

- Development branches with many known failures waiting to be fixed.
- Release branches with known failures that are not considered
  important for the particular release criteria used in that branch.

The script must be executed from the toplevel build directory.  When
executed it will:

1) Determine the target built: TARGET
2) Determine the source directory: SRCDIR
3) Look for a failure manifest file in
   <SRCDIR>/contrib/testsuite-management/<TARGET>.xfail
4) Collect all the <tool>.sum files from the build tree.
5) Produce a report stating:
   a) Failures expected in the manifest but not present in the build.
   b) Failures in the build not expected in the manifest.
6) If all the build failures are expected in the manifest, it exits
   with exit code 0.  Otherwise, it exits with error code 1.
"""

import optparse
import logging
import os
import sys

sys.path.append(os.path.dirname(os.path.abspath(__file__)))

from dejagnu.manifest import Manifest
from dejagnu.summary import DejaGnuTestResult
from dejagnu.summary import DejaGnuTestRun

# Pattern for naming manifest files.  The first argument should be
# the toplevel GCC source directory.  The second argument is the
# target triple used during the build.
_MANIFEST_PATH_PATTERN = '%s/contrib/testsuite-management/%s.xfail'


def GetMakefileVars(makefile_path):
  assert os.path.exists(makefile_path)

  with open(makefile_path) as lines:
    kvs = [line.split('=', 1) for line in lines if '=' in line]

    return dict((k.strip(), v.strip()) for k, v in kvs)


def GetSumFiles(build_dir):
  summaries = []

  for root, _, filenames in os.walk(build_dir):
    summaries.extend([os.path.join(root, filename)
                      for filename in filenames if filename.endswith('.sum')])

  return map(os.path.normpath, summaries)


def ValidBuildDirectory(build_dir, target):
  mandatory_paths = [build_dir, os.path.join(build_dir, 'Makefile')]

  extra_paths = [os.path.join(build_dir, target),
                 os.path.join(build_dir, 'build-%s' % target)]

  return (all(map(os.path.exists, mandatory_paths)) and
          any(map(os.path.exists, extra_paths)))


def GetManifestPath(build_dir):
  makefile = GetMakefileVars(os.path.join(build_dir, 'Makefile'))
  srcdir = makefile['srcdir']
  target = makefile['target']

  if not ValidBuildDirectory(build_dir, target):
    target = makefile['target_alias']

  if not ValidBuildDirectory(build_dir, target):
    logging.error('%s is not a valid GCC top level build directory.', build_dir)
    sys.exit(1)

  logging.info('Discovered source directory: "%s"', srcdir)
  logging.info('Discovered build target: "%s"', target)

  return _MANIFEST_PATH_PATTERN % (srcdir, target)


def CompareResults(manifest, actual):
  """Compare sets of results and return two lists:
     - List of results present in MANIFEST but missing from ACTUAL.
     - List of results present in ACTUAL but missing from MANIFEST.
  """
  # Report all the actual results not present in the manifest.
  actual_vs_manifest = actual - manifest

  # Filter out tests marked flaky.
  manifest_without_flaky_tests = set(filter(lambda result: not result.flaky,
                                            manifest))

  # Simlarly for all the tests in the manifest.
  manifest_vs_actual = manifest_without_flaky_tests - actual

  return actual_vs_manifest, manifest_vs_actual


def LogResults(level, results):
  log_fun = getattr(logging, level)

  for num, result in enumerate(sorted(results), start=1):
    log_fun('  %d) %s', num, result)


def CheckExpectedResults(manifest_path, build_dir):
  logging.info('Reading manifest file: "%s"', manifest_path)

  manifest = set(Manifest.FromFile(manifest_path))

  logging.info('Getting actual results from build directory: "%s"',
               os.path.realpath(build_dir))

  summaries = GetSumFiles(build_dir)

  actual = set()

  for summary in summaries:
    test_run = DejaGnuTestRun.FromFile(summary)
    failures = set(Manifest.FromDejaGnuTestRun(test_run))
    actual.update(failures)

  if manifest:
    logging.debug('Tests expected to fail:')
    LogResults('debug', manifest)

  if actual:
    logging.debug('Actual test failures:')
    LogResults('debug', actual)

  actual_vs_manifest, manifest_vs_actual = CompareResults(manifest, actual)

  if actual_vs_manifest:
    logging.info('Build results not in the manifest:')
    LogResults('info', actual_vs_manifest)

  if manifest_vs_actual:
    logging.info('Manifest results not present in the build:')
    LogResults('info', manifest_vs_actual)
    logging.info('NOTE: This is not a failure!  ',
                 'It just means that the manifest expected these tests to '
                 'fail, but they worked in this configuration.')

  if actual_vs_manifest or manifest_vs_actual:
    sys.exit(1)

  logging.info('No unexpected failures.')


def ProduceManifest(manifest_path, build_dir, overwrite):
  if os.path.exists(manifest_path) and not overwrite:
    logging.error('Manifest file "%s" already exists.', manifest_path)
    logging.error('Use --force to overwrite.')
    sys.exit(1)

  testruns = map(DejaGnuTestRun.FromFile, GetSumFiles(build_dir))
  manifests = map(Manifest.FromDejaGnuTestRun, testruns)

  with open(manifest_path, 'w') as manifest_file:
    manifest_strings = [manifest.Generate() for manifest in manifests]
    logging.info('Writing manifest to "%s".', manifest_path)
    manifest_file.write('\n'.join(manifest_strings))


def Main(argv):
  parser = optparse.OptionParser(usage=__doc__)
  parser.add_option(
      '-b',
      '--build_dir',
      dest='build_dir',
      action='store',
      metavar='PATH',
      default=os.getcwd(),
      help='Build directory to check. (default: current directory)')
  parser.add_option('-m',
                    '--manifest',
                    dest='manifest',
                    action='store_true',
                    help='Produce the manifest for the current build.')
  parser.add_option(
      '-f',
      '--force',
      dest='force',
      action='store_true',
      help=('Overwrite an existing manifest file, if user requested creating '
            'new one. (default: False)'))
  parser.add_option('-v',
                    '--verbose',
                    dest='verbose',
                    action='store_true',
                    help='Increase verbosity.')
  options, _ = parser.parse_args(argv[1:])

  if options.verbose:
    logging.root.setLevel(logging.DEBUG)

  manifest_path = GetManifestPath(options.build_dir)

  if options.manifest:
    ProduceManifest(manifest_path, options.build_dir, options.force)
  else:
    CheckExpectedResults(manifest_path, options.build_dir)


if __name__ == '__main__':
  logging.basicConfig(format='%(levelname)s: %(message)s', level=logging.INFO)
  Main(sys.argv)
