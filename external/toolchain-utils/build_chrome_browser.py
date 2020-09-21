#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to checkout the ChromeOS source.

This script sets up the ChromeOS source in the given directory, matching a
particular release of ChromeOS.
"""

from __future__ import print_function

__author__ = 'raymes@google.com (Raymes Khoury)'

import argparse
import os
import sys

from cros_utils import command_executer
from cros_utils import logger
from cros_utils import misc


def Usage(parser, message):
  print('ERROR: %s' % message)
  parser.print_help()
  sys.exit(0)


def Main(argv):
  """Build Chrome browser."""

  cmd_executer = command_executer.GetCommandExecuter()

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      help='Target directory for ChromeOS installation.')
  parser.add_argument('--version', dest='version')
  parser.add_argument(
      '--clean',
      dest='clean',
      default=False,
      action='store_true',
      help=('Clean the /var/cache/chromeos-chrome/'
            'chrome-src/src/out_$board dir'))
  parser.add_argument(
      '--env', dest='env', default='', help='Use the following env')
  parser.add_argument(
      '--ebuild_version',
      dest='ebuild_version',
      help='Use this ebuild instead of the default one.')
  parser.add_argument(
      '--cflags',
      dest='cflags',
      default='',
      help='CFLAGS for the ChromeOS packages')
  parser.add_argument(
      '--cxxflags',
      dest='cxxflags',
      default='',
      help='CXXFLAGS for the ChromeOS packages')
  parser.add_argument(
      '--ldflags',
      dest='ldflags',
      default='',
      help='LDFLAGS for the ChromeOS packages')
  parser.add_argument(
      '--board', dest='board', help='ChromeOS target board, e.g. x86-generic')
  parser.add_argument(
      '--no_build_image',
      dest='no_build_image',
      default=False,
      action='store_true',
      help=('Skip build image after building browser.'
            'Defaults to False.'))
  parser.add_argument(
      '--label',
      dest='label',
      help='Optional label to apply to the ChromeOS image.')
  parser.add_argument(
      '--build_image_args',
      default='',
      dest='build_image_args',
      help='Optional arguments to build_image.')
  parser.add_argument(
      '--cros_workon',
      dest='cros_workon',
      help='Build using external source tree.')
  parser.add_argument(
      '--dev',
      dest='dev',
      default=False,
      action='store_true',
      help=('Build a dev (eg. writable/large) image. '
            'Defaults to False.'))
  parser.add_argument(
      '--debug',
      dest='debug',
      default=False,
      action='store_true',
      help=('Build chrome browser using debug mode. '
            'This option implies --dev. Defaults to false.'))
  parser.add_argument(
      '--verbose',
      dest='verbose',
      default=False,
      action='store_true',
      help='Build with verbose information.')

  options = parser.parse_args(argv)

  if options.chromeos_root is None:
    Usage(parser, '--chromeos_root must be set')

  if options.board is None:
    Usage(parser, '--board must be set')

  if options.version is None:
    logger.GetLogger().LogOutput('No Chrome version given so '
                                 'using the default checked in version.')
    chrome_version = ''
  else:
    chrome_version = 'CHROME_VERSION=%s' % options.version

  if options.dev and options.no_build_image:
    logger.GetLogger().LogOutput(
        "\"--dev\" is meaningless if \"--no_build_image\" is given.")

  if options.debug:
    options.dev = True

  options.chromeos_root = misc.CanonicalizePath(options.chromeos_root)

  unmask_env = 'ACCEPT_KEYWORDS=~*'
  if options.ebuild_version:
    ebuild_version = '=%s' % options.ebuild_version
    options.env = '%s %s' % (options.env, unmask_env)
  else:
    ebuild_version = 'chromeos-chrome'

  if options.cros_workon and not (
      os.path.isdir(options.cros_workon) and os.path.exists(
          os.path.join(options.cros_workon, 'src/chromeos/BUILD.gn'))):
    Usage(parser, '--cros_workon must be a valid chromium browser checkout.')

  if options.verbose:
    options.env = misc.MergeEnvStringWithDict(
        options.env, {'USE': 'chrome_internal verbose'})
  else:
    options.env = misc.MergeEnvStringWithDict(options.env,
                                              {'USE': 'chrome_internal'})
  if options.debug:
    options.env = misc.MergeEnvStringWithDict(options.env,
                                              {'BUILDTYPE': 'Debug'})

  if options.clean:
    misc.RemoveChromeBrowserObjectFiles(options.chromeos_root, options.board)

  chrome_origin = 'SERVER_SOURCE'
  if options.cros_workon:
    chrome_origin = 'LOCAL_SOURCE'
    command = 'cros_workon --board={0} start chromeos-chrome'.format(
        options.board)
    ret = cmd_executer.ChrootRunCommandWOutput(options.chromeos_root, command)

    # cros_workon start returns non-zero if chromeos-chrome is already a
    # cros_workon package.
    if ret[0] and ret[2].find(
        'WARNING : Already working on chromeos-base/chromeos-chrome') == -1:
      logger.GetLogger().LogFatal('cros_workon chromeos-chrome failed.')

    # Return value is non-zero means we do find the "Already working on..."
    # message, keep the information, so later on we do not revert the
    # cros_workon status.
    cros_workon_keep = (ret[0] != 0)

  # Emerge the browser
  emerge_browser_command = ('CHROME_ORIGIN={0} {1} '
                            'CFLAGS="$(portageq-{2} envvar CFLAGS) {3}" '
                            'LDFLAGS="$(portageq-{2} envvar LDFLAGS) {4}" '
                            'CXXFLAGS="$(portageq-{2} envvar CXXFLAGS) {5}" '
                            '{6} emerge-{2} --buildpkg {7}').format(
                                chrome_origin, chrome_version, options.board,
                                options.cflags, options.ldflags,
                                options.cxxflags, options.env, ebuild_version)

  cros_sdk_options = ''
  if options.cros_workon:
    cros_sdk_options = '--chrome_root={0}'.format(options.cros_workon)

  ret = cmd_executer.ChrootRunCommand(
      options.chromeos_root,
      emerge_browser_command,
      cros_sdk_options=cros_sdk_options)

  logger.GetLogger().LogFatalIf(ret, 'build_packages failed')

  if options.cros_workon and not cros_workon_keep:
    command = 'cros_workon --board={0} stop chromeos-chrome'.format(
        options.board)
    ret = cmd_executer.ChrootRunCommand(options.chromeos_root, command)
    # cros_workon failed, not a fatal one, just report it.
    if ret:
      print('cros_workon stop chromeos-chrome failed.')

  if options.no_build_image:
    return ret

  # Finally build the image
  ret = cmd_executer.ChrootRunCommand(options.chromeos_root,
                                      '{0} {1} {2} {3}'.format(
                                          unmask_env, options.env,
                                          misc.GetBuildImageCommand(
                                              options.board, dev=options.dev),
                                          options.build_image_args))

  logger.GetLogger().LogFatalIf(ret, 'build_image failed')

  flags_file_name = 'chrome_flags.txt'
  flags_file_path = '{0}/src/build/images/{1}/latest/{2}'.format(
      options.chromeos_root, options.board, flags_file_name)
  flags_file = open(flags_file_path, 'wb')
  flags_file.write('CFLAGS={0}\n'.format(options.cflags))
  flags_file.write('CXXFLAGS={0}\n'.format(options.cxxflags))
  flags_file.write('LDFLAGS={0}\n'.format(options.ldflags))
  flags_file.close()

  if options.label:
    image_dir_path = '{0}/src/build/images/{1}/latest'.format(
        options.chromeos_root, options.board)
    real_image_dir_path = os.path.realpath(image_dir_path)
    command = 'ln -sf -T {0} {1}/{2}'.format(
        os.path.basename(real_image_dir_path),\
        os.path.dirname(real_image_dir_path),\
        options.label)

    ret = cmd_executer.RunCommand(command)
    logger.GetLogger().LogFatalIf(
        ret, 'Failed to apply symlink label %s' % options.label)

  return ret


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
