#!/system/bin/sh
#
# Copyright (C) 2018 The Android Open Source Project
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

# performance test setup for 2018 devices

stop thermal-engine
stop perfd
stop vendor.thermal-engine
stop vendor.perfd

cpubase=/sys/devices/system/cpu
gov=cpufreq/scaling_governor

cpu=4
top=8

# Enable the gold cores at max frequency.
# 825600 902400 979200 1056000 1209600 1286400 1363200 1459200 1536000
# 1612800 1689600 1766400 1843200 1920000 1996800 2092800 2169600
# 2246400 2323200 2400000 2476800 2553600 2649600
while [ $((cpu < $top)) -eq 1 ]; do
  echo 1 > $cpubase/cpu${cpu}/online
  echo performance > $cpubase/cpu${cpu}/$gov
  S=`cat $cpubase/cpu${cpu}/cpufreq/scaling_cur_freq`
  echo "setting cpu $cpu to max at $S kHz"
  cpu=$(($cpu + 1))
done

cpu=0
top=4

# Disable the silver cores.
while [ $((cpu < $top)) -eq 1 ]; do
  echo "disable cpu $cpu"
  echo 0 > $cpubase/cpu${cpu}/online
  cpu=$(($cpu + 1))
done

echo "setting GPU bus split"
echo 0 > /sys/class/kgsl/kgsl-3d0/bus_split
echo "setting GPU force clocks"
echo 1 > /sys/class/kgsl/kgsl-3d0/force_clk_on
echo "setting GPU idle timer"
echo 10000 > /sys/class/kgsl/kgsl-3d0/idle_timer

# 0 381 572 762 1144 1571 2086 2597 2929 3879 4943 5931 6881
echo performance > /sys/class/devfreq/soc:qcom,gpubw/governor
echo -n "setting GPU bus frequency to max at "
cat /sys/class/devfreq/soc:qcom,gpubw/cur_freq

# 257000000 342000000 414000000 520000000 596000000 675000000 710000000
echo performance > /sys/class/kgsl/kgsl-3d0/devfreq/governor
echo -n "setting GPU frequency to max at "
cat /sys/class/kgsl/kgsl-3d0/devfreq/cur_freq
