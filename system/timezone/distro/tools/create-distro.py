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

"""Generates a time zone distro file"""

import argparse
import os
import shutil
import subprocess
import sys

sys.path.append('%s/external/icu/tools' % os.environ.get('ANDROID_BUILD_TOP'))
import i18nutil


android_build_top = i18nutil.GetAndroidRootOrDie()
android_host_out_dir = i18nutil.GetAndroidHostOutOrDie()
timezone_dir = os.path.realpath('%s/system/timezone' % android_build_top)
i18nutil.CheckDirExists(timezone_dir, 'system/timezone')

def RunCreateTimeZoneDistro(properties_file, distro_output_dir):
  # Build the libraries needed.
  subprocess.check_call(['make', '-C', android_build_top, 'time_zone_distro_tools',
      'time_zone_distro'])

  libs = [ 'time_zone_distro_tools', 'time_zone_distro' ]
  host_java_libs_dir = '%s/../common/obj/JAVA_LIBRARIES' % android_host_out_dir
  classpath_components = []
  for lib in libs:
      classpath_components.append('%s/%s_intermediates/javalib.jar' % (host_java_libs_dir, lib))

  classpath = ':'.join(classpath_components)

  # Run the CreateTimeZoneDistro tool
  subprocess.check_call(['java', '-cp', classpath,
      'com.android.timezone.distro.tools.CreateTimeZoneDistro', properties_file,
      distro_output_dir])


def CreateTimeZoneDistro(iana_version, revision, tzdata_file, icu_file, tzlookup_file, output_dir):
  original_cwd = os.getcwd()

  i18nutil.SwitchToNewTemporaryDirectory()
  working_dir = os.getcwd()

  # Generate the properties file.
  properties_file = '%s/distro.properties' % working_dir
  with open(properties_file, "w") as properties:
    properties.write('rules.version=%s\n' % iana_version)
    properties.write('revision=%s\n' % revision)
    properties.write('tzdata.file=%s\n' % tzdata_file)
    properties.write('icu.file=%s\n' % icu_file)
    properties.write('tzlookup.file=%s\n' % tzlookup_file)

  RunCreateTimeZoneDistro(properties_file, output_dir)

  os.chdir(original_cwd)


def main():
  parser = argparse.ArgumentParser()
  parser.add_argument('-iana_version', required=True,
      help='The IANA time zone rules release version, e.g. 2017b')
  parser.add_argument('-revision', type=int, default=1,
      help='The distro revision for the IANA version, default = 1')
  parser.add_argument('-tzdata', required=True, help='The location of the tzdata file to include')
  parser.add_argument('-icu', required=True,
      help='The location of the ICU overlay .dat file to include')
  parser.add_argument('-tzlookup', required=True,
      help='The location of the tzlookup.xml file to include')
  parser.add_argument('-output', required=True, help='The output directory')
  args = parser.parse_args()

  iana_version = args.iana_version
  revision = args.revision
  tzdata_file = os.path.abspath(args.tzdata)
  icu_file = os.path.abspath(args.icu)
  tzlookup_file = os.path.abspath(args.tzlookup)
  output_dir = os.path.abspath(args.output)

  CreateTimeZoneDistro(
      iana_version=iana_version,
      revision=revision,
      tzdata_file=tzdata_file,
      icu_file=icu_file,
      tzlookup_file=tzlookup_file,
      output_dir=output_dir)

  print 'Distro files created in %s' % output_dir
  sys.exit(0)


if __name__ == '__main__':
  main()
