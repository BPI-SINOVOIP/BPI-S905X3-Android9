#!/usr/bin/env bash

cd "`dirname $0`"

die () {
	set +x # Turn off printing commands
	echo ""
	echo " *** fatal error: $*"
	exit 1
}

if [ -z $ANDROID_BUILD_TOP ]; then
  echo "You need to source and lunch before you can use this script"
  exit 1
fi

adb wait-for-device || die

echo "Running scan command test. . ."
sleep 2

adb shell killall wpantund 2> /dev/null

echo "+ adb shell wpantund -I wpan5 -s 'system:ot-ncp\ 1' -o Config:Daemon:ExternalNetifManagement 1 &"
adb shell wpantund -I wpan5 -s 'system:ot-ncp\ 1' -o Config:Daemon:ExternalNetifManagement 1 &
WPANTUND_1_PID=$!
echo "+ adb shell wpantund -I wpan6 -s 'system:ot-ncp\ 2' -o Config:Daemon:ExternalNetifManagement 1 &"
adb shell wpantund -I wpan6 -s 'system:ot-ncp\ 2' -o Config:Daemon:ExternalNetifManagement 1 &
WPANTUND_2_PID=$!
trap "kill -HUP $WPANTUND_1_PID $WPANTUND_2_PID 2> /dev/null" EXIT INT TERM

sleep 2
kill -0 $WPANTUND_1_PID  || die "wpantund failed to start"
kill -0 $WPANTUND_2_PID  || die "wpantund failed to start"
sleep 2

echo "+ adb shell lowpanctl -I wpan5 form blahnet"
adb shell lowpanctl -I wpan5 form blahnet || die
echo "+ adb shell lowpanctl -I wpan5 status"
adb shell lowpanctl -I wpan5 status || die
echo "+ adb shell lowpanctl -I wpan6 scan"
adb shell lowpanctl -I wpan6 scan || die

echo "Finished scan command test."
