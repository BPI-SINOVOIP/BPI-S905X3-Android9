#!/usr/bin/env python2
#
# Copyright 2010 Google Inc. All Rights Reserved.
"""Script to checkout the ChromeOS source.

This script sets up the ChromeOS source in the given directory, matching a
particular release of ChromeOS.
"""

from __future__ import print_function

__author__ = ('asharif@google.com (Ahmad Sharif) '
              'llozano@google.com (Luis Lozano) '
              'raymes@google.com (Raymes Khoury) '
              'shenhan@google.com (Han Shen)')

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
  """Build ChromeOS."""
  # Common initializations
  cmd_executer = command_executer.GetCommandExecuter()

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--chromeos_root',
      dest='chromeos_root',
      help='Target directory for ChromeOS installation.')
  parser.add_argument(
      '--clobber_chroot',
      dest='clobber_chroot',
      action='store_true',
      help='Delete the chroot and start fresh',
      default=False)
  parser.add_argument(
      '--clobber_board',
      dest='clobber_board',
      action='store_true',
      help='Delete the board and start fresh',
      default=False)
  parser.add_argument(
      '--rebuild',
      dest='rebuild',
      action='store_true',
      help='Rebuild all board packages except the toolchain.',
      default=False)
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
      '--package', dest='package', help='The package needs to be built')
  parser.add_argument(
      '--label',
      dest='label',
      help='Optional label symlink to point to build dir.')
  parser.add_argument(
      '--dev',
      dest='dev',
      default=False,
      action='store_true',
      help=('Make the final image in dev mode (eg writable, '
            'more space on image). Defaults to False.'))
  parser.add_argument(
      '--debug',
      dest='debug',
      default=False,
      action='store_true',
      help=("Optional. Build chrome browser with \"-g -O0\". "
            "Notice, this also turns on \'--dev\'. "
            'Defaults to False.'))
  parser.add_argument(
      '--env', dest='env', default='', help='Env to pass to build_packages.')
  parser.add_argument(
      '--vanilla',
      dest='vanilla',
      default=False,
      action='store_true',
      help='Use default ChromeOS toolchain.')
  parser.add_argument(
      '--vanilla_image',
      dest='vanilla_image',
      default=False,
      action='store_true',
      help=('Use prebuild packages for building the image. '
            'It also implies the --vanilla option is set.'))

  options = parser.parse_args(argv[1:])

  if options.chromeos_root is None:
    Usage(parser, '--chromeos_root must be set')
  options.chromeos_root = os.path.expanduser(options.chromeos_root)
  scripts_dir = os.path.join(options.chromeos_root, 'src', 'scripts')
  if not os.path.isdir(scripts_dir):
    Usage(parser, '--chromeos_root must be set up first. Use setup_chromeos.py')

  if options.board is None:
    Usage(parser, '--board must be set')

  if options.debug:
    options.dev = True

  build_packages_env = options.env
  if build_packages_env.find('EXTRA_BOARD_FLAGS=') != -1:
    logger.GetLogger().LogFatal(
        ('Passing "EXTRA_BOARD_FLAGS" in "--env" is not supported. '
         'This flags is used internally by this script. '
         'Contact the author for more detail.'))

  if options.rebuild == True:
    build_packages_env += ' EXTRA_BOARD_FLAGS=-e'
    # EXTRA_BOARD_FLAGS=-e should clean up the object files for the chrome
    # browser but it doesn't. So do it here.
    misc.RemoveChromeBrowserObjectFiles(options.chromeos_root, options.board)

  # Build with afdo_use by default.
  # To change the default use --env="USE=-afdo_use".
  build_packages_env = misc.MergeEnvStringWithDict(
      build_packages_env, {'USE': 'chrome_internal afdo_use -cros-debug'})

  build_packages_command = misc.GetBuildPackagesCommand(
      board=options.board, usepkg=options.vanilla_image, debug=options.debug)

  if options.package:
    build_packages_command += ' {0}'.format(options.package)

  build_image_command = misc.GetBuildImageCommand(options.board, options.dev)

  if options.vanilla or options.vanilla_image:
    command = misc.GetSetupBoardCommand(
        options.board,
        usepkg=options.vanilla_image,
        force=options.clobber_board)
    command += '; ' + build_packages_env + ' ' + build_packages_command
    command += '&& ' + build_packages_env + ' ' + build_image_command
    ret = cmd_executer.ChrootRunCommand(options.chromeos_root, command)
    return ret

  # Setup board
  if not os.path.isdir(options.chromeos_root + '/chroot/build/' +
                       options.board) or options.clobber_board:
    # Run build_tc.py from binary package
    ret = cmd_executer.ChrootRunCommand(options.chromeos_root,
                                        misc.GetSetupBoardCommand(
                                            options.board,
                                            force=options.clobber_board))
    logger.GetLogger().LogFatalIf(ret, 'setup_board failed')
  else:
    logger.GetLogger().LogOutput('Did not setup_board '
                                 'because it already exists')

  if options.debug:
    # Perform 2-step build_packages to build a debug chrome browser.

    # Firstly, build everything that chromeos-chrome depends on normally.
    if options.rebuild == True:
      # Give warning about "--rebuild" and "--debug". Under this combination,
      # only dependencies of "chromeos-chrome" get rebuilt.
      logger.GetLogger().LogWarning(
          "\"--rebuild\" does not correctly re-build every package when "
          "\"--debug\" is enabled. ")

      # Replace EXTRA_BOARD_FLAGS=-e with "-e --onlydeps"
      build_packages_env = build_packages_env.replace(
          'EXTRA_BOARD_FLAGS=-e', 'EXTRA_BOARD_FLAGS=\"-e --onlydeps\"')
    else:
      build_packages_env += ' EXTRA_BOARD_FLAGS=--onlydeps'

    ret = cmd_executer.ChrootRunCommand(
        options.chromeos_root, "CFLAGS=\"$(portageq-%s envvar CFLAGS) %s\" "
        "CXXFLAGS=\"$(portageq-%s envvar CXXFLAGS) %s\" "
        "LDFLAGS=\"$(portageq-%s envvar LDFLAGS) %s\" "
        'CHROME_ORIGIN=SERVER_SOURCE '
        '%s '
        '%s --skip_chroot_upgrade'
        'chromeos-chrome' % (options.board, options.cflags, options.board,
                             options.cxxflags, options.board, options.ldflags,
                             build_packages_env, build_packages_command))

    logger.GetLogger().LogFatalIf(\
      ret, 'build_packages failed while trying to build chromeos-chrome deps.')

    # Secondly, build chromeos-chrome using debug mode.
    # Replace '--onlydeps' with '--nodeps'.
    if options.rebuild == True:
      build_packages_env = build_packages_env.replace(
          'EXTRA_BOARD_FLAGS=\"-e --onlydeps\"', 'EXTRA_BOARD_FLAGS=--nodeps')
    else:
      build_packages_env = build_packages_env.replace(
          'EXTRA_BOARD_FLAGS=--onlydeps', 'EXTRA_BOARD_FLAGS=--nodeps')
    ret = cmd_executer.ChrootRunCommand(
        options.chromeos_root, "CFLAGS=\"$(portageq-%s envvar CFLAGS) %s\" "
        "CXXFLAGS=\"$(portageq-%s envvar CXXFLAGS) %s\" "
        "LDFLAGS=\"$(portageq-%s envvar LDFLAGS) %s\" "
        'CHROME_ORIGIN=SERVER_SOURCE BUILDTYPE=Debug '
        '%s '
        '%s --skip_chroot_upgrade'
        'chromeos-chrome' % (options.board, options.cflags, options.board,
                             options.cxxflags, options.board, options.ldflags,
                             build_packages_env, build_packages_command))
    logger.GetLogger().LogFatalIf(
        ret,
        'build_packages failed while trying to build debug chromeos-chrome.')

    # Now, we have built chromeos-chrome and all dependencies.
    # Finally, remove '-e' from EXTRA_BOARD_FLAGS,
    # otherwise, chromeos-chrome gets rebuilt.
    build_packages_env = build_packages_env.replace(\
      'EXTRA_BOARD_FLAGS=--nodeps', '')

    # Up to now, we have a debug built chromos-chrome browser.
    # Fall through to build the rest of the world.

    # Build packages
  ret = cmd_executer.ChrootRunCommand(
      options.chromeos_root, "CFLAGS=\"$(portageq-%s envvar CFLAGS) %s\" "
      "CXXFLAGS=\"$(portageq-%s envvar CXXFLAGS) %s\" "
      "LDFLAGS=\"$(portageq-%s envvar LDFLAGS) %s\" "
      'CHROME_ORIGIN=SERVER_SOURCE '
      '%s '
      '%s --skip_chroot_upgrade' %
      (options.board, options.cflags, options.board, options.cxxflags,
       options.board, options.ldflags, build_packages_env,
       build_packages_command))

  logger.GetLogger().LogFatalIf(ret, 'build_packages failed')
  if options.package:
    return 0
  # Build image
  ret = cmd_executer.ChrootRunCommand(
      options.chromeos_root, build_packages_env + ' ' + build_image_command)

  logger.GetLogger().LogFatalIf(ret, 'build_image failed')

  flags_file_name = 'flags.txt'
  flags_file_path = ('%s/src/build/images/%s/latest/%s' %
                     (options.chromeos_root, options.board, flags_file_name))
  flags_file = open(flags_file_path, 'wb')
  flags_file.write('CFLAGS=%s\n' % options.cflags)
  flags_file.write('CXXFLAGS=%s\n' % options.cxxflags)
  flags_file.write('LDFLAGS=%s\n' % options.ldflags)
  flags_file.close()

  if options.label:
    image_dir_path = ('%s/src/build/images/%s/latest' % (options.chromeos_root,
                                                         options.board))
    real_image_dir_path = os.path.realpath(image_dir_path)
    command = ('ln -sf -T %s %s/%s' %
               (os.path.basename(real_image_dir_path),
                os.path.dirname(real_image_dir_path), options.label))

    ret = cmd_executer.RunCommand(command)
    logger.GetLogger().LogFatalIf(
        ret, 'Failed to apply symlink label %s' % options.label)

  return ret


if __name__ == '__main__':
  retval = Main(sys.argv)
  sys.exit(retval)
