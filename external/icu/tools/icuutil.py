#!/usr/bin/python -B

# Copyright 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Utility methods associated with ICU source and builds."""

import glob
import os
import shutil
import subprocess
import sys

import i18nutil
import ziputil

def icuDir():
  """Returns the location of ICU in the Android source tree."""
  android_build_top = i18nutil.GetAndroidRootOrDie()
  icu_dir = os.path.realpath('%s/external/icu' % android_build_top)
  i18nutil.CheckDirExists(icu_dir, 'external/icu')
  return icu_dir


def icu4cDir():
  """Returns the location of ICU4C in the Android source tree."""
  icu4c_dir = os.path.realpath('%s/icu4c/source' % icuDir())
  i18nutil.CheckDirExists(icu4c_dir, 'external/icu/icu4c/source')
  return icu4c_dir


def icu4jDir():
  """Returns the location of ICU4J in the Android source tree."""
  icu4j_dir = os.path.realpath('%s/icu4j' % icuDir())
  i18nutil.CheckDirExists(icu4j_dir, 'external/icu/icu4j')
  return icu4j_dir


def datFile(icu_build_dir):
  """Returns the location of the ICU .dat file in the specified ICU build dir."""
  dat_file_pattern = '%s/data/out/tmp/icudt??l.dat' % icu_build_dir
  dat_files = glob.glob(dat_file_pattern)
  if len(dat_files) != 1:
    print 'ERROR: Unexpectedly found %d .dat files (%s). Halting.' % (len(datfiles), datfiles)
    sys.exit(1)
  dat_file = dat_files[0]
  return dat_file


def PrepareIcuBuild(icu_build_dir):
  """Sets up an ICU build in the specified (non-existent) directory.

  Creates the directory and runs "runConfigureICU Linux"
  """
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Create a directory to run 'make' from.
  os.mkdir(icu_build_dir)
  os.chdir(icu_build_dir)

  # Build the ICU tools.
  print 'Configuring ICU tools...'
  subprocess.check_call(['%s/runConfigureICU' % icu4cDir(), 'Linux'])

  os.chdir(original_working_dir)


def MakeTzDataFiles(icu_build_dir, iana_tar_file):
  """Builds and runs the ICU tools in ${icu_Build_dir}/tools/tzcode.

  The tools are run against the specified IANA tzdata .tar.gz.
  The resulting zoneinfo64.txt is copied into the src directories.
  """
  tzcode_working_dir = '%s/tools/tzcode' % icu_build_dir

  # Fix missing files.
  # The tz2icu tool only picks up icuregions and icuzones if they are in the CWD
  for icu_data_file in [ 'icuregions', 'icuzones']:
    icu_data_file_source = '%s/tools/tzcode/%s' % (icu4cDir(), icu_data_file)
    icu_data_file_symlink = '%s/%s' % (tzcode_working_dir, icu_data_file)
    os.symlink(icu_data_file_source, icu_data_file_symlink)

  iana_tar_filename = os.path.basename(iana_tar_file)
  working_iana_tar_file = '%s/%s' % (tzcode_working_dir, iana_tar_filename)
  shutil.copyfile(iana_tar_file, working_iana_tar_file)

  print 'Making ICU tz data files...'
  # The Makefile assumes the existence of the bin directory.
  os.mkdir('%s/bin' % icu_build_dir)

  subprocess.check_call(['make', '-C', tzcode_working_dir])

  # Copy the source file to its ultimate destination.
  zoneinfo_file = '%s/zoneinfo64.txt' % tzcode_working_dir
  icu_txt_data_dir = '%s/data/misc' % icu4cDir()
  print 'Copying zoneinfo64.txt to %s ...' % icu_txt_data_dir
  shutil.copy(zoneinfo_file, icu_txt_data_dir)


def MakeAndCopyIcuDataFiles(icu_build_dir):
  """Builds the ICU .dat and .jar files using the current src data.

  The files are copied back into the expected locations in the src tree.
  """
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Regenerate the .dat file.
  os.chdir(icu_build_dir)
  subprocess.check_call(['make', 'INCLUDE_UNI_CORE_DATA=1', '-j32'])

  # Copy the .dat file to its ultimate destination.
  icu_dat_data_dir = '%s/stubdata' % icu4cDir()
  dat_file = datFile(icu_build_dir)

  print 'Copying %s to %s ...' % (dat_file, icu_dat_data_dir)
  shutil.copy(dat_file, icu_dat_data_dir)

  # Generate the ICU4J .jar files
  os.chdir('%s/data' % icu_build_dir)
  subprocess.check_call(['make', 'icu4j-data'])

  # Copy the ICU4J .jar files to their ultimate destination.
  icu_jar_data_dir = '%s/main/shared/data' % icu4jDir()
  jarfiles = glob.glob('out/icu4j/*.jar')
  if len(jarfiles) != 2:
    print 'ERROR: Unexpectedly found %d .jar files (%s). Halting.' % (len(jarfiles), jarfiles)
    sys.exit(1)
  for jarfile in jarfiles:
    icu_jarfile = os.path.join(icu_jar_data_dir, os.path.basename(jarfile))
    if ziputil.ZipCompare(jarfile, icu_jarfile):
      print 'Ignoring %s which is identical to %s ...' % (jarfile, icu_jarfile)
    else:
      print 'Copying %s to %s ...' % (jarfile, icu_jar_data_dir)
      shutil.copy(jarfile, icu_jar_data_dir)

  # Switch back to the original working cwd.
  os.chdir(original_working_dir)


def MakeAndCopyOverlayTzIcuData(icu_build_dir, dest_file):
  """Makes a .dat file containing just time-zone data.

  The overlay file can be used as an overlay of a full ICU .dat file
  to provide newer time zone data. Some strings like translated
  time zone names will be missing, but rules will be correct."""
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Regenerate the .res files.
  os.chdir(icu_build_dir)
  subprocess.check_call(['make', 'INCLUDE_UNI_CORE_DATA=1', '-j32'])

  # The list of ICU resources needed for time zone data overlays.
  tz_res_names = [
          'metaZones.res',
          'timezoneTypes.res',
          'windowsZones.res',
          'zoneinfo64.res',
  ]

  dat_file = datFile(icu_build_dir)
  icu_package_dat = os.path.basename(dat_file)
  if not icu_package_dat.endswith('.dat'):
      print '%s does not end with .dat' % icu_package_dat
      sys.exit(1)
  icu_package = icu_package_dat[:-4]

  # Create a staging directory to hold the files to go into the overlay .dat
  res_staging_dir = '%s/overlay_res' % icu_build_dir
  os.mkdir(res_staging_dir)

  # Copy all the .res files we need from, e.g. ./data/out/build/icudt55l, to the staging directory
  res_src_dir = '%s/data/out/build/%s' % (icu_build_dir, icu_package)
  for tz_res_name in tz_res_names:
    shutil.copy('%s/%s' % (res_src_dir, tz_res_name), res_staging_dir)

  # Create a .lst file to pass to pkgdata.
  tz_files_file = '%s/tzdata.lst' % res_staging_dir
  with open(tz_files_file, "a") as tz_files:
    for tz_res_name in tz_res_names:
      tz_files.write('%s\n' % tz_res_name)

  icu_lib_dir = '%s/lib' % icu_build_dir
  pkg_data_bin = '%s/bin/pkgdata' % icu_build_dir

  # Run pkgdata to create a .dat file.
  icu_env = os.environ.copy()
  icu_env["LD_LIBRARY_PATH"] = icu_lib_dir

  # pkgdata treats the .lst file it is given as relative to CWD, and the path also affects the
  # resource names in the .dat file produced so we change the CWD.
  os.chdir(res_staging_dir)

  # -F : force rebuilding all data
  # -m common : create a .dat
  # -v : verbose
  # -T . : use "." as a temp dir
  # -d . : use "." as the dest dir
  # -p <name> : Set the "data name"
  p = subprocess.Popen(
      [pkg_data_bin, '-F', '-m', 'common', '-v', '-T', '.', '-d', '.', '-p',
          icu_package, tz_files_file],
      env=icu_env)
  p.wait()
  if p.returncode != 0:
    print 'pkgdata failed with status code: %s' % p.returncode

  # Copy the .dat to the chosen place / name.
  generated_dat_file = '%s/%s' % (res_staging_dir, icu_package_dat)
  shutil.copyfile(generated_dat_file, dest_file)
  print 'ICU overlay .dat can be found here: %s' % dest_file

  # Switch back to the original working cwd.
  os.chdir(original_working_dir)

