#!/bin/bash
#
# Copyright (C) 2017 The Android Open Source Project
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
#
# usage: usbtuner-test.sh <test_case> [channel]
#
# To test repeated channel change, run:
#
# ./usbtuner-test.sh <1 or 3>
#
# To test watching a fixed channel, run:
#
# ./usbtuner-test.sh 2
#
# Case 2 uses the last-viewed channel by TV app. Give a channel number
# as a 2nd parameter if you want to use the channel for testing, like below:
#
# ./usbtuner-test.sh 2 6-1
#
# The script assumes that:
#   1) Browsing by keydown event circulates among the USB input channels only
#   2) When started, TV app should tune to one of the channels provided by the USB input
#
# The test result is logged in the doc: https://goo.gl/MsPBf7

function start_tv {
  disable_analytics_report
  adb shell am force-stop com.android.tv
  adb shell am start -n com.android.tv/.MainActivity > /dev/null
  sleep 5
}

function log_begin {
  adb shell dumpsys meminfo -d --package com.android.tv.tuner > meminfo-begin.txt
}

function tune {
  adb shell input text $1
  adb shell input keyevent KEYCODE_DPAD_CENTER
  sleep 5  # Wait enough for tuning
}

function browse {
  for i in {1..50}; do
    adb shell input keyevent DPAD_DOWN
    sleep 10  # Tune and watch the channel for a while
  done;
}

function browse_heavily {
  for i in {1..60}; do
    echo "$(date '+%x %X') ======== Test #$i of 60 ========"
    clear_logcat
    for j in {1..60}; do
      adb shell input keyevent DPAD_DOWN
      sleep $(( $RANDOM % 3 ))  # Sleep for 0 - 2 seconds
    done;
    measure_tuning_time
  done;
}

function clear_logcat {
  adb logcat -c
}

function measure_tuning_time {
  timeout 1 adb logcat -s TvInputSessionImpl | awk -f $(dirname $0)/measure-tuning-time.awk
}

function log_end {
  adb shell dumpsys meminfo -d --package com.android.tv.tuner > meminfo-end.txt
}

function stop_tv {
  # Stop TV by running other app (Settings)
  adb shell am start com.android.tv.settings/com.android.tv.settings.MainSettings
  restore_analytics_setting
}

function output {
  echo "Cut and paste this"
  sed -n 33,46p meminfo-begin.txt | cut -f 2 -d ":" -s | awk '{print $1}'
  sed -n 33,46p meminfo-end.txt | cut -f 2 -d ":" -s | awk '{print $1}'
}

function disable_analytics_report {
  tracker=$(adb shell getprop tv_use_tracker | tr -d '[[:space:]]')
  adb shell setprop tv_use_tracker false
}

function restore_analytics_setting {
  if [ "${tracker}" == "" ]; then
    adb shell setprop tv_use_tracker ""
  else
    adb shell setprop tv_use_tracker ${tracker}
  fi
}

function control_c {
  restore_analytics_setting
  echo "Exiting..."
  exit 1
}

# Entry point

trap control_c SIGINT

case "$1" in
  1)
     echo "Runing test 1"
     start_tv
     log_begin
     clear_logcat
     browse  # Repeat channel change for about 10 minutes
     measure_tuning_time
     log_end
     stop_tv
     output
     ;;
  2)
     echo "Runing test 2"
     start_tv
     log_begin
     if [ "$2" != "" ]; then
       tune $2
     fi
     sleep 600  # 10 minutes
     log_end
     stop_tv
     output
     ;;
  3)
     echo "Runing test 3"
     start_tv
     log_begin
     browse_heavily  # Repeat channel change for about 3 hours
     log_end
     stop_tv
     output
     ;;
  *)
     echo "usage: usbtuner-test.sh <1|2|3> [channel]"
     exit 1
     ;;
esac

