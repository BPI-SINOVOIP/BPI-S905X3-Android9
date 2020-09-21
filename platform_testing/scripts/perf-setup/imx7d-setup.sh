if [[ "`id -u`" -ne "0" ]]; then
  echo "WARNING: running as non-root, proceeding anyways..."
fi

stop thermal-engine
stop mpdecision
stop perfd

# Set CPU cores frequency to 1200 MHz
cpubase=/sys/devices/system/cpu
gov=cpufreq/scaling_governor

cpu=0
S=1200000
while [ $((cpu < 2)) -eq 1 ]; do
    echo 1 > $cpubase/cpu${cpu}/online
    echo userspace > $cpubase/cpu${cpu}/$gov
    echo $S > $cpubase/cpu${cpu}/cpufreq/scaling_max_freq
    echo $S > $cpubase/cpu${cpu}/cpufreq/scaling_min_freq
    echo $S > $cpubase/cpu${cpu}/cpufreq/scaling_setspeed
    cpu=$(($cpu + 1))
done
