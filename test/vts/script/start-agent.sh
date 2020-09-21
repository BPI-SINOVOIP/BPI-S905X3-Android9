#!/bin/bash
adb shell chmod 755 /data/local/tmp/32/vts_shell_driver32
adb shell chmod 755 /data/local/tmp/64/vts_shell_driver64
adb shell killall vts_hal_driver32 > /dev/null 2&>1
adb shell killall vts_hal_driver64 > /dev/null 2&>1
adb shell killall vts_shell_driver32 > /dev/null 2&>1
adb shell killall vts_shell_driver64 > /dev/null 2&>1
adb shell rm -f /data/local/tmp/vts_driver_*
adb shell rm -f /data/local/tmp/vts_agent_callback*
adb shell LD_LIBRARY_PATH=/data/local/tmp/64 \
/data/local/tmp/64/vts_hal_agent64 \
--hal_driver_path_32=/data/local/tmp/32/vts_hal_driver32 \
--hal_driver_path_64=/data/local/tmp/64/vts_hal_driver64 \
--spec_dir=/data/local/tmp/spec \
--shell_driver_path_32=/data/local/tmp/32/vts_shell_driver32 \
--shell_driver_path_64=/data/local/tmp/64/vts_shell_driver64
# to run using nohup
# adb shell LD_LIBRARY_PATH=/data/local/tmp nohup /data/local/tmp/vts_hal_agent
# /data/local/tmp/vts_hal_driver32 /data/local/tmp/vts_hal_driver64 /data/local/tmp/spec
# ASAN_OPTIONS=coverage=1 for ASAN
