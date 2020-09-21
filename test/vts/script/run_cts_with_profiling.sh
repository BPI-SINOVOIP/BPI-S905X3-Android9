#!/bin/bash
#
# Copyright 2016 The Android Open Source Project
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

serial_no=$1
if [ -z "$serial_no" ]
then
  echo "Must provide serial number of the testing device."
  exit
fi

local_trace_dir=$2
if [ -z "$local_trace_dir" ]
then
  local_trace_dir=/usr/local/backup/cts-traces
fi

test_list=$3
if [ -z "$test_list" ]
then
  test_list=${ANDROID_BUILD_TOP}/test/vts/script/cts_test_list.txt
fi

# allow write to /vendor partition
adb -s $serial_no root
adb -s $serial_no disable-verity
adb -s $serial_no reboot
adb -s $serial_no wait-for-device
adb -s $serial_no root
adb -s $serial_no remount
adb -s $serial_no shell setenforce 0
adb -s $serial_no shell chmod 777 -R data/local/tmp

# push profiler libs
adb -s $serial_no push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib64/*-vts.profiler.so vendor/lib64/
adb -s $serial_no push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib/*-vts.profiler.so vendor/lib/
adb -s $serial_no push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib64/libvts_* vendor/lib64/
adb -s $serial_no push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/lib/libvts_* vendor/lib/

# push vts_profiling_configure
adb -s $serial_no push ${ANDROID_BUILD_TOP}/out/host/linux-x86/vts/android-vts/testcases/DATA/bin/vts_profiling_configure /data/local/tmp/

# get cts testcases
tests=()
while read -r test
do
  tests+=($test)
done < "$test_list"

# run cts testcases
for i in ${tests[@]}
do
  echo Running $i
  adb -s $serial_no shell rm /data/local/tmp/*.vts.trace
  adb -s $serial_no shell ./data/local/tmp/vts_profiling_configure enable /vendor/lib/ /vendor/lib64/
  cts-tradefed run commandAndExit cts -s $serial_no --primary-abi-only --skip-device-info \
  --skip-system-status-check com.android.compatibility.common.tradefed.targetprep.NetworkConnectivityChecker \
  --skip-system-status-check com.android.tradefed.suite.checker.KeyguardStatusChecker -m $i
  # In case device restart during the test run.
  adb -s $serial_no root
  adb -s $serial_no shell setenforce 0
  adb -s $serial_no shell ls /data/local/tmp/*.vts.trace > temp
  trace_path=$local_trace_dir/$i
  rm -rf $trace_path
  mkdir -p $trace_path
  while read -r trace
  do
    adb -s $serial_no pull $trace $trace_path
  done < "temp"
done

echo "done"
