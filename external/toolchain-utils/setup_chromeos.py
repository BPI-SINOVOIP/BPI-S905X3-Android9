#!/usr/bin/env python2
#
# Copyright 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Script to checkout the ChromeOS source.

This script sets up the ChromeOS source in the given directory, matching a
particular release of ChromeOS.
"""

from __future__ import print_function

__author__ = 'raymes@google.com (Raymes Khoury)'

from datetime import datetime

import argparse
import os
import pickle
import sys
import tempfile
import time
from cros_utils import command_executer
from cros_utils import logger
from cros_utils import manifest_versions

GCLIENT_FILE = """solutions = [
  { "name"        : "CHROME_DEPS",
    "url"         :
    "svn://svn.chromium.org/chrome-internal/trunk/tools/buildspec/releases/%s",
    "custom_deps" : {
      "src/third_party/WebKit/LayoutTests": None,
      "src-pdf": None,
      "src/pdf": None,
    },
    "safesync_url": "",
   },
]
"""

# List of stable versions used for common team image
# Sheriff must update this list when a new common version becomes available
COMMON_VERSIONS = '/home/mobiletc-prebuild/common_images/common_list.txt'


def Usage(parser):
  parser.print_help()
  sys.exit(0)


# Get version spec file, either from "paladin" or "buildspec" directory.
def GetVersionSpecFile(version, versions_git):
  temp = tempfile.mkdtemp()
  commands = ['cd {0}'.format(temp), \
              'git clone {0} versions'.format(versions_git)]
  cmd_executer = command_executer.GetCommandExecuter()
  ret = cmd_executer.RunCommands(commands)
  err_msg = None
  if ret:
    err_msg = 'Failed to checkout versions_git - {0}'.format(versions_git)
    ret = None
  else:
    v, m = version.split('.', 1)
    paladin_spec = 'paladin/buildspecs/{0}/{1}.xml'.format(v, m)
    generic_spec = 'buildspecs/{0}/{1}.xml'.format(v, m)
    paladin_path = '{0}/versions/{1}'.format(temp, paladin_spec)
    generic_path = '{0}/versions/{1}'.format(temp, generic_spec)
    if os.path.exists(paladin_path):
      ret = paladin_spec
    elif os.path.exists(generic_path):
      ret = generic_spec
    else:
      err_msg = 'No spec found for version {0}'.format(version)
      ret = None
    # Fall through to clean up.

  commands = ['rm -rf {0}'.format(temp)]
  cmd_executer.RunCommands(commands)
  if err_msg:
    logger.GetLogger().LogFatal(err_msg)
  return ret


def TimeToCommonVersion(timestamp):
  """Convert timestamp to common image version."""
  tdt = datetime.fromtimestamp(float(timestamp))
  with open(COMMON_VERSIONS, 'r') as f:
    common_list = pickle.load(f)
    for sv in common_list:
      sdt = datetime.strptime(sv['date'], '%Y-%m-%d %H:%M:%S.%f')
      if tdt >= sdt:
        return '%s.%s' % (sv['chrome_major_version'], sv['chromeos_version'])
    # should never reach here
    logger.GetLogger().LogFatal('No common version for timestamp')
  return None


def Main(argv):
  """Checkout the ChromeOS source."""
  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--dir',
      dest='directory',
      help='Target directory for ChromeOS installation.')
  parser.add_argument(
      '--version',
      dest='version',
      default='latest_lkgm',
      help="""ChromeOS version. Can be:
(1) A release version in the format: 'X.X.X.X'
(2) 'top' for top of trunk
(3) 'latest_lkgm' for the latest lkgm version
(4) 'lkgm' for the lkgm release before timestamp
(5) 'latest_common' for the latest team common stable version
(6) 'common' for the team common stable version before timestamp
Default is 'latest_lkgm'.""")
  parser.add_argument(
      '--timestamp',
      dest='timestamp',
      default=None,
      help='Timestamps in epoch format. It will check out the'
      'latest LKGM or the latest COMMON version of ChromeOS'
      ' before the timestamp. Use in combination with'
      ' --version=latest or --version=common. Use '
      '"date -d <date string> +%s" to find epoch time')
  parser.add_argument(
      '--minilayout',
      dest='minilayout',
      default=False,
      action='store_true',
      help='Whether to checkout the minilayout (smaller '
      'checkout).')
  parser.add_argument(
      '--jobs', '-j', dest='jobs', help='Number of repo sync threads to use.')
  parser.add_argument(
      '--public',
      '-p',
      dest='public',
      default=False,
      action='store_true',
      help='Use the public checkout instead of the private '
      'one.')

  options = parser.parse_args(argv)

  if not options.version:
    parser.print_help()
    logger.GetLogger().LogFatal('No version specified.')
  else:
    version = options.version.strip()

  if not options.timestamp:
    timestamp = ''
  else:
    timestamp = options.timestamp.strip()
    if version not in ('lkgm', 'common'):
      parser.print_help()
      logger.GetLogger().LogFatal('timestamp option only applies for '
                                  "versions \"lkgm\" or \"common\"")

  if not options.directory:
    parser.print_help()
    logger.GetLogger().LogFatal('No directory specified.')

  directory = options.directory.strip()

  if options.public:
    manifest_repo = 'https://chromium.googlesource.com/chromiumos/manifest.git'
    versions_repo = ('https://chromium.googlesource.com/'
                     'chromiumos/manifest-versions.git')
  else:
    manifest_repo = ('https://chrome-internal.googlesource.com/chromeos/'
                     'manifest-internal.git')
    versions_repo = ('https://chrome-internal.googlesource.com/chromeos/'
                     'manifest-versions.git')

  if version == 'top':
    init = 'repo init -u %s' % manifest_repo
  elif version == 'latest_lkgm':
    manifests = manifest_versions.ManifestVersions()
    version = manifests.TimeToVersionChromeOS(time.mktime(time.gmtime()))
    version, manifest = version.split('.', 1)
    logger.GetLogger().LogOutput('found version %s.%s for latest LKGM' %
                                 (version, manifest))
    init = ('repo init -u %s -m paladin/buildspecs/%s/%s.xml' %
            (versions_repo, version, manifest))
    del manifests
  elif version == 'lkgm':
    if not timestamp:
      parser.print_help()
      logger.GetLogger().LogFatal('No timestamp specified for version=lkgm')
    manifests = manifest_versions.ManifestVersions()
    version = manifests.TimeToVersion(timestamp)
    version, manifest = version.split('.', 1)
    logger.GetLogger().LogOutput(
        'found version %s.%s for LKGM at timestamp %s' % (version, manifest,
                                                          timestamp))
    init = ('repo init -u %s -m paladin/buildspecs/%s/%s.xml' %
            (versions_repo, version, manifest))
    del manifests
  elif version == 'latest_common':
    version = TimeToCommonVersion(time.mktime(time.gmtime()))
    version, manifest = version.split('.', 1)
    logger.GetLogger().LogOutput('found version %s.%s for latest Common image' %
                                 (version, manifest))
    init = ('repo init -u %s -m buildspecs/%s/%s.xml' % (versions_repo, version,
                                                         manifest))
  elif version == 'common':
    if not timestamp:
      parser.print_help()
      logger.GetLogger().LogFatal('No timestamp specified for version=lkgm')
    version = TimeToCommonVersion(timestamp)
    version, manifest = version.split('.', 1)
    logger.GetLogger().LogOutput('found version %s.%s for latest common image '
                                 'at timestamp %s' % (version, manifest,
                                                      timestamp))
    init = ('repo init -u %s -m buildspecs/%s/%s.xml' % (versions_repo, version,
                                                         manifest))
  else:
    # user specified a specific version number
    version_spec_file = GetVersionSpecFile(version, versions_repo)
    if not version_spec_file:
      return 1
    init = 'repo init -u %s -m %s' % (versions_repo, version_spec_file)

  if options.minilayout:
    init += ' -g minilayout'

  init += ' --repo-url=https://chromium.googlesource.com/external/repo.git'

  # crosbug#31837 - "Sources need to be world-readable to properly
  # function inside the chroot"
  sync = 'umask 022 && repo sync'
  if options.jobs:
    sync += ' -j %s' % options.jobs

  commands = ['mkdir -p %s' % directory, 'cd %s' % directory, init, sync]
  cmd_executer = command_executer.GetCommandExecuter()
  ret = cmd_executer.RunCommands(commands)
  if ret:
    return ret

  return cmd_executer.RunCommand(
      'git ls-remote '
      'https://chrome-internal.googlesource.com/chrome/src-internal.git '
      '> /dev/null')


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
