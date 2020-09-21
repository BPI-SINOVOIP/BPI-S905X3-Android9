#!/usr/bin/env python2
"""Verify that a ChromeOS sub-tree was built with a particular compiler"""

from __future__ import print_function

import argparse
import fnmatch
import os
import sys

from cros_utils import command_executer

COMPILERS = ['gcc', 'clang']

COMPILER_STRINGS = {'gcc': 'GNU C', 'clang': 'clang version'}

ERR_NO_DEBUG_INFO = 1024


def UsageError(parser, message):
  """Output error message and help/usage info."""

  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def CreateTmpDwarfFile(filename, dwarf_file, cmd_executer):
  """Create temporary dwarfdump file, to be parsed later."""

  cmd = ('readelf --debug-dump=info %s | grep -A5 DW_AT_producer > %s' %
         (filename, dwarf_file))
  retval = cmd_executer.RunCommand(cmd, print_to_console=False)
  return retval


def FindAllFiles(root_dir):
  """Create a list of all the *.debug and *.dwp files to be checked."""

  file_list = []
  tmp_list = [
      os.path.join(dirpath, f)
      for dirpath, _, files in os.walk(root_dir)
      for f in fnmatch.filter(files, '*.debug')
  ]
  for f in tmp_list:
    if 'build-id' not in f:
      file_list.append(f)
  tmp_list = [
      os.path.join(dirpath, f)
      for dirpath, _, files in os.walk(root_dir)
      for f in fnmatch.filter(files, '*.dwp')
  ]
  file_list += tmp_list
  return file_list


def VerifyArgs(compiler, filename, tmp_dir, root_dir, options, parser):
  """Verify that the option values and combinations are valid."""

  if options.filename and options.all_files:
    UsageError(parser, 'Cannot use both --file and --all_files.')
  if options.filename and options.root_dir:
    UsageError(parser, 'Cannot use both --file and --root_dir.')
  if options.all_files and not options.root_dir:
    UsageError(parser, 'Missing --root_dir option.')
  if options.root_dir and not options.all_files:
    UsageError(parser, 'Missing --all_files option.')
  if not options.filename and not options.all_files:
    UsageError(parser, 'Must specify either --file or --all_files.')

  # Verify that the values specified are valid.
  if filename:
    if not os.path.exists(filename):
      UsageError(parser, 'Cannot find %s' % filename)
  compiler = options.compiler.lower()
  if compiler not in COMPILERS:
    UsageError(parser, '%s is not a valid compiler (gcc or clang).' % compiler)
  if root_dir and not os.path.exists(root_dir):
    UsageError(parser, '%s does not exist.' % root_dir)
  if not os.path.exists(tmp_dir):
    os.makedirs(tmp_dir)


def CheckFile(filename, compiler, tmp_dir, options, cmd_executer):
  """Verify the information in a single file."""

  print('Checking %s' % filename)
  # Verify that file contains debug information.
  cmd = 'readelf -SW %s | grep debug_info' % filename
  retval = cmd_executer.RunCommand(cmd, print_to_console=False)
  if retval != 0:
    print('No debug info in this file. Unable to verify compiler.')
    # There's no debug info in this file, so skip it.
    return ERR_NO_DEBUG_INFO

  tmp_name = os.path.basename(filename) + '.dwarf'
  dwarf_file = os.path.join(tmp_dir, tmp_name)
  status = CreateTmpDwarfFile(filename, dwarf_file, cmd_executer)

  if status != 0:
    print('Unable to create dwarf file for %s (status: %d).' % (filename,
                                                                status))
    return status

  comp_str = COMPILER_STRINGS[compiler]

  retval = 0
  with open(dwarf_file, 'r') as in_file:
    lines = in_file.readlines()
    looking_for_name = False
    for line in lines:
      if 'DW_AT_producer' in line:
        looking_for_name = False
        if 'GNU AS' in line:
          continue
        if comp_str not in line:
          looking_for_name = True
          retval = 1
      elif looking_for_name:
        if 'DW_AT_name' in line:
          words = line.split(':')
          bad_file = words[-1]
          print('FAIL:  %s was not compiled with %s.' % (bad_file.rstrip(),
                                                         compiler))
          looking_for_name = False
        elif 'DW_TAG_' in line:
          looking_for_name = False

  if not options.keep_file:
    os.remove(dwarf_file)

  return retval


def Main(argv):

  cmd_executer = command_executer.GetCommandExecuter()
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--file', dest='filename', help='Name of file to be verified.')
  parser.add_argument(
      '--compiler',
      dest='compiler',
      required=True,
      help='Desired compiler (gcc or clang)')
  parser.add_argument(
      '--tmp_dir',
      dest='tmp_dir',
      help='Directory in which to put dwarf dump file.'
      ' Defaults to /tmp')
  parser.add_argument(
      '--keep_file',
      dest='keep_file',
      default=False,
      action='store_true',
      help='Do not delete dwarf file when done.')
  parser.add_argument(
      '--all_files',
      dest='all_files',
      default=False,
      action='store_true',
      help='Find and check ALL .debug/.dwp files '
      'in subtree.  Must be used with --root_dir '
      '(and NOT with --file).')
  parser.add_argument(
      '--root_dir',
      dest='root_dir',
      help='Root of subtree in which to look for '
      'files.  Must be used with --all_files, and'
      ' not with --file.')

  options = parser.parse_args(argv)

  compiler = options.compiler
  filename = None
  if options.filename:
    filename = os.path.realpath(os.path.expanduser(options.filename))
  tmp_dir = '/tmp'
  if options.tmp_dir:
    tmp_dir = os.path.realpath(os.path.expanduser(options.tmp_dir))
  root_dir = None
  if options.root_dir:
    root_dir = os.path.realpath(os.path.expanduser(options.root_dir))

  VerifyArgs(compiler, filename, tmp_dir, root_dir, options, parser)

  file_list = []
  if filename:
    file_list.append(filename)
  else:
    file_list = FindAllFiles(root_dir)

  bad_files = []
  unknown_files = []
  all_pass = True
  for f in file_list:
    result = CheckFile(f, compiler, tmp_dir, options, cmd_executer)
    if result == ERR_NO_DEBUG_INFO:
      unknown_files.append(f)
      all_pass = False
    elif result != 0:
      bad_files.append(f)
      all_pass = False

  if all_pass:
    print('\n\nSUCCESS:  All compilation units were compiled with %s.\n' %
          compiler)
    return 0
  else:
    if len(bad_files) == 0:
      print(
          '\n\n*Mostly* SUCCESS: All files that could be checked were compiled '
          'with %s.' % compiler)
      print(
          '\n\nUnable to verify the following files (no debug info in them):\n')
      for f in unknown_files:
        print(f)
    else:
      print('\n\nFAILED:  The following files were not compiled with %s:\n' %
            compiler)
      for f in bad_files:
        print(f)
      if len(unknown_files) > 0:
        print('\n\nUnable to verify the following files (no debug info in '
              'them):\n')
        for f in unknown_files:
          print(f)
    return 1


if __name__ == '__main__':
  sys.exit(Main(sys.argv[1:]))
