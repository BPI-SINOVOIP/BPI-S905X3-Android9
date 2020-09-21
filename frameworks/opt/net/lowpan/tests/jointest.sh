#!/usr/bin/env bash

cd "`dirname $0`"

if [ "shell" = "$1" ]
then
	WANTS_SHELL=1
fi

possibly_enter_shell() {
	if [ "$WANTS_SHELL" = "1" ]
	then
		echo " *** Entering adb shell:"
		adb shell
	fi
}

die () {
	set +x # Turn off printing commands
	echo ""
	echo " *** fatal error: $*"
	possibly_enter_shell
	exit 1
}

if [ -z $ANDROID_BUILD_TOP ]; then
  echo "You need to source and lunch before you can use this script"
  exit 1
fi

adb wait-for-device || die

echo "Running join command test. . ."
sleep 2

adb shell killall wpantund 2> /dev/null

adb shell wpantund -I wpan5 -s 'system:ot-ncp\ 1' -o Config:Daemon:ExternalNetifManagement 1 &
WPANTUND_1_PID=$!
adb shell wpantund -I wpan6 -s 'system:ot-ncp\ 2' -o Config:Daemon:ExternalNetifManagement 1 &
WPANTUND_2_PID=$!
trap "kill -HUP $WPANTUND_1_PID $WPANTUND_2_PID 2> /dev/null" EXIT INT TERM

sleep 2

kill -0 $WPANTUND_1_PID  || die "wpantund failed to start"
kill -0 $WPANTUND_2_PID  || die "wpantund failed to start"

sleep 2

echo "+ adb shell lowpanctl -I wpan5 status"
adb shell lowpanctl -I wpan5 status || die
echo "+ adb shell lowpanctl -I wpan5 form blahnet --panid 1234 --xpanid 0011223344556677 --channel 11"
adb shell lowpanctl -I wpan5 form blahnet --panid 1234 --xpanid 0011223344556677 --channel 11 || die
echo "+ adb shell lowpanctl -I wpan5 status"
adb shell lowpanctl -I wpan5 status || die
echo "+ adb shell lowpanctl -I wpan5 show-credential"
adb shell lowpanctl -I wpan5 show-credential || die

CREDENTIAL=`adb shell lowpanctl -I wpan5 show-credential -r` || die

echo "+ adb shell lowpanctl -I wpan6 status"
adb shell lowpanctl -I wpan6 status || die
echo "+ adb shell lowpanctl -I wpan6 scan"
adb shell lowpanctl -I wpan6 scan || die
echo "+ adb shell lowpanctl -I wpan6 join blahnet --panid 1234 --xpanid 0011223344556677 --channel 11 --master-key ${CREDENTIAL}"
adb shell lowpanctl -I wpan6 join blahnet --panid 1234 --xpanid 0011223344556677 --channel 11 --master-key ${CREDENTIAL} || die

sleep 2

echo "+ adb shell lowpanctl -I wpan6 status"
adb shell lowpanctl -I wpan6 status || die

WPAN5_LL_ADDR=`adb shell lowpanctl -I wpan5 status | grep fe80:: | sed -e 's:^[^a-f:0-9]*\([a-f:0-9]*\)/.*:\1:i'`
WPAN6_LL_ADDR=`adb shell lowpanctl -I wpan6 status | grep fe80:: | sed -e 's:^[^a-f:0-9]*\([a-f:0-9]*\)/.*:\1:i'`

echo "+ ping6 -c 4 -w 6 ${WPAN5_LL_ADDR}%wpan6"
adb shell ping6 -c 4 -w 6 ${WPAN5_LL_ADDR}%wpan6 || die
echo "+ ping6 -c 4 -w 6 ${WPAN6_LL_ADDR}%wpan5"
adb shell ping6 -c 4 -w 6 ${WPAN6_LL_ADDR}%wpan5 || die

possibly_enter_shell

echo "Finished join command test."
