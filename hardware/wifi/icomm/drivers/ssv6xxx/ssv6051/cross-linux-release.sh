#!/bin/bash
prompt="Pick the target platform:"
chip_options=("x1000" \
              "t10" \
              "sc6138")
PLATFORM=""

select opt in "${chip_options[@]}" "Quit"; do 
    case "$REPLY" in

    1 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    2 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;
    3 ) echo "${chip_options[$REPLY-1]} is option";PLATFORM=${chip_options[$REPLY-1]};break;;

    $(( ${#chip_options[@]}+1 )) ) echo "Goodbye!"; break;;
    *) echo "Invalid option. Try another one.";continue;;
    esac
done

if [ "$PLATFORM" != "" ]; then
./ver_info.pl include/ssv_version.h

if [ $? -eq 0 ]; then
    echo "Please check SVN first !!"
else
cp platforms/platform-config.mak .
cp platforms/$PLATFORM.cfg .
cp platforms/$PLATFORM-generic-wlan.c .
cp platforms/$PLATFORM-wifi.cfg .
cp Makefile.cross_linux Makefile
sed -i 's,PLATFORMS =,PLATFORMS = '"$PLATFORM"',g' Makefile
make clean

# Remove garbage
svn rm wpa_supplicant.conf
svn rm unload.sh
svn rm sta.cfg
svn rm ssvcfg.sh
svn rm rules.mak
svn rm remove_old_driver.sh
svn rm load.sh
svn rm launch_sta_ap.sh
svn rm launch_ap_sta.sh
svn rm config.mak
svn rm cli
svn rm clean_log.sh
svn rm ap_shutdown.sh
svn rm ap_launch.sh
svn rm ap_check.sh
svn rm ap.cfg
svn rm build.sh
svn rm android-build.sh
svn rm platforms 

echo "Done ko!"
fi
else
echo "Fail!"
fi

