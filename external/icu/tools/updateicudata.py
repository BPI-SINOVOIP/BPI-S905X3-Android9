#!/usr/bin/python -B

"""Regenerates (just) ICU data files used in the Android system image."""

import os
import sys

import i18nutil
import icuutil


# Run with no arguments from any directory, with no special setup required.
def main():
  i18nutil.SwitchToNewTemporaryDirectory()
  icu_build_dir = '%s/icu' % os.getcwd()

  icu_dir = icuutil.icuDir()
  print 'Found icu in %s ...' % icu_dir

  icuutil.PrepareIcuBuild(icu_build_dir)

  icuutil.MakeAndCopyIcuDataFiles(icu_build_dir)

  print 'Look in %s for new data files' % icu_dir
  sys.exit(0)

if __name__ == '__main__':
  main()
