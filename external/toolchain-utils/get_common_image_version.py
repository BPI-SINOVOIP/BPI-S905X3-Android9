#!/usr/bin/env python2
#
# Copyright 2013 Google Inc. All Rights Reserved.
"""Script to find list of common images (first beta releases) in Chromeos.

Display information about stable ChromeOS/Chrome versions to be used
by the team developers. The purpose is to increase team productivity
by using stable (known and tested) ChromeOS/Chrome versions instead of
using randomly selected versions. Currently we define as a "stable"
version the first Beta release in a particular release cycle.
"""

from __future__ import print_function

__author__ = 'llozano@google.com (Luis Lozano)'

import argparse
import pickle
import re
import sys
import urllib

VERSIONS_HISTORY_URL = 'http://cros-omahaproxy.appspot.com/history'


def DisplayBetas(betas):
  print('List of betas from %s' % VERSIONS_HISTORY_URL)
  for beta in betas:
    print('  Release', beta['chrome_major_version'], beta)
  return


def FindAllBetas(all_versions):
  """Get ChromeOS first betas from History URL."""

  all_betas = []
  prev_beta = {}
  for line in all_versions:
    match_obj = re.match(
        r'(?P<date>.*),(?P<chromeos_version>.*),'
        r'(?P<chrome_major_version>\d*).(?P<chrome_minor_version>.*),'
        r'(?P<chrome_appid>.*),beta-channel,,Samsung Chromebook Series 5 550',
        line)
    if match_obj:
      if prev_beta:
        if (prev_beta['chrome_major_version'] !=
            match_obj.group('chrome_major_version')):
          all_betas.append(prev_beta)
      prev_beta = match_obj.groupdict()
  if prev_beta:
    all_betas.append(prev_beta)
  return all_betas


def SerializeBetas(all_betas, serialize_file):
  with open(serialize_file, 'wb') as f:
    pickle.dump(all_betas, f)
    print('Serialized list of betas into', serialize_file)
  return


def Main(argv):
  """Get ChromeOS first betas list from history URL."""

  parser = argparse.ArgumentParser()
  parser.add_argument(
      '--serialize',
      dest='serialize',
      default=None,
      help='Save list of common images into the specified '
      'file.')
  options = parser.parse_args(argv)

  try:
    opener = urllib.URLopener()
    all_versions = opener.open(VERSIONS_HISTORY_URL)
  except IOError as ioe:
    print('Cannot open', VERSIONS_HISTORY_URL)
    print(ioe)
    return 1

  all_betas = FindAllBetas(all_versions)
  DisplayBetas(all_betas)
  if options.serialize:
    SerializeBetas(all_betas, options.serialize)
  all_versions.close()

  return 0


if __name__ == '__main__':
  retval = Main(sys.argv[1:])
  sys.exit(retval)
