# Copyright 2011 Google Inc. All Rights Reserved.
# Author: kbaclawski@google.com (Krystian Baclawski)
#

from contextlib import contextmanager
import glob
from itertools import chain
import logging
import optparse
import os.path
import sys

from manifest import Manifest
import report
from summary import DejaGnuTestRun


def ExpandGlobExprList(paths):
  """Returns an iterator that goes over expanded glob paths."""
  return chain.from_iterable(map(glob.glob, paths))


@contextmanager
def OptionChecker(parser):
  """Provides scoped environment for command line option checking."""
  try:
    yield
  except SystemExit as ex:
    parser.print_help()
    print ''
    sys.exit('ERROR: %s' % str(ex))


def ManifestCommand(argv):
  parser = optparse.OptionParser(
      description=
      ('Read in one or more DejaGNU summary files (.sum), parse their '
       'content and generate manifest files.  Manifest files store a list '
       'of failed tests that should be ignored.  Generated files are '
       'stored in current directory under following name: '
       '${tool}-${board}.xfail (e.g. "gcc-unix.xfail").'),
      usage='Usage: %prog manifest [file.sum] (file2.sum ...)')

  _, args = parser.parse_args(argv[2:])

  with OptionChecker(parser):
    if not args:
      sys.exit('At least one *.sum file required.')

  for filename in chain.from_iterable(map(glob.glob, args)):
    test_run = DejaGnuTestRun.FromFile(filename)

    manifest = Manifest.FromDejaGnuTestRun(test_run)
    manifest_filename = '%s-%s.xfail' % (test_run.tool, test_run.board)

    with open(manifest_filename, 'w') as manifest_file:
      manifest_file.write(manifest.Generate())

      logging.info('Wrote manifest to "%s" file.', manifest_filename)


def ReportCommand(argv):
  parser = optparse.OptionParser(
      description=
      ('Read in one or more DejaGNU summary files (.sum), parse their '
       'content and generate a single report file in selected format '
       '(currently only HTML).'),
      usage=('Usage: %prog report (-m manifest.xfail) [-o report.html] '
             '[file.sum (file2.sum ...)'))
  parser.add_option(
      '-o',
      dest='output',
      type='string',
      default=None,
      help=('Suppress failures for test listed in provided manifest files. '
            '(use -m for each manifest file you want to read)'))
  parser.add_option(
      '-m',
      dest='manifests',
      type='string',
      action='append',
      default=None,
      help=('Suppress failures for test listed in provided manifest files. '
            '(use -m for each manifest file you want to read)'))

  opts, args = parser.parse_args(argv[2:])

  with OptionChecker(parser):
    if not args:
      sys.exit('At least one *.sum file required.')

    if not opts.output:
      sys.exit('Please provide name for report file.')

  manifests = []

  for filename in ExpandGlobExprList(opts.manifests or []):
    logging.info('Using "%s" manifest.', filename)
    manifests.append(Manifest.FromFile(filename))

  test_runs = [DejaGnuTestRun.FromFile(filename)
               for filename in chain.from_iterable(map(glob.glob, args))]

  html = report.Generate(test_runs, manifests)

  if html:
    with open(opts.output, 'w') as html_file:
      html_file.write(html)
      logging.info('Wrote report to "%s" file.', opts.output)
  else:
    sys.exit(1)


def HelpCommand(argv):
  sys.exit('\n'.join([
      'Usage: %s command [options]' % os.path.basename(argv[
          0]), '', 'Commands:',
      '  manifest - manage files containing a list of suppressed test failures',
      '  report   - generate report file for selected test runs'
  ]))


def Main(argv):
  try:
    cmd_name = argv[1]
  except IndexError:
    cmd_name = None

  cmd_map = {'manifest': ManifestCommand, 'report': ReportCommand}
  cmd_map.get(cmd_name, HelpCommand)(argv)


if __name__ == '__main__':
  FORMAT = '%(asctime)-15s %(levelname)s %(message)s'
  logging.basicConfig(format=FORMAT, level=logging.INFO)

  Main(sys.argv)
