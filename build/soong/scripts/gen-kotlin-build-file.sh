#!/bin/bash -e

# Copyright 2018 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# Generates kotlinc module xml file to standard output based on rsp files

if [ -z "$1" ]; then
  echo "usage: $0 <classpath> <outDir> <rspFiles>..." >&2
  exit 1
fi

# Classpath variable has a tendency to be prefixed by "-classpath", remove it.
if [[ $1 == "-classpath" ]]; then
  shift
fi;

classpath=$1
out_dir=$2
shift 2

# Path in the build file are relative to the build file, we need to make them absolute.
prefix=`pwd`

# Print preamble
echo "<modules><module name=\"name\" type=\"java-production\" outputDir=\"${out_dir}\">"

# Print classpath entries
for file in $(echo $classpath | tr ":" "\n"); do
  echo "  <classpath path=\"${prefix}/${file}\"/>"
done

# For each rsp file, print source entries
while (( "$#" )); do
  for file in $(cat $1); do
    if [[ $file == *.java ]]; then
      echo "  <javaSourceRoots path=\"${prefix}/${file}\"/>"
    elif [[ $file == *.kt ]]; then
      echo "  <sources path=\"${prefix}/${file}\"/>"
    else
      echo "Unknown source file type ${file}"
      exit 1
    fi
  done

  shift
done

echo "</module></modules>"
