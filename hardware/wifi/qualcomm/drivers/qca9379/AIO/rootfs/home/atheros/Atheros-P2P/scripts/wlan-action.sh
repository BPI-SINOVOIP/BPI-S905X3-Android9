#!/bin/sh

IFNAME=$1
CMD=$2
P2P_SCRIPT_PATH=/home/atheros/Atheros-P2P/scripts

kill_daemon() {
    NAME=$1
    PLIST=`ps | grep $NAME | cut -f2 -d" "`
    if [ ${#PLIST} -eq 0 ]; then
        PLIST=`ps | grep $NAME | cut -f3 -d" "`
    fi
    PID=`echo $PLIST | cut -f1 -d" "`
    if [ $PID -gt 0 ]; then
        if ps $PID | grep -q $NAME; then
            kill $PID
        fi
    fi
}



if [ "$CMD" = "CONNECTED" ]; then
        killall udhcpc
	udhcpc -i $IFNAME -s ${P2P_SCRIPT_PATH}/udhcpc-p2p.script &
fi

if [ "$CMD" = "DISCONNECTED" ]; then
	killall udhcpc 
	ifconfig $IFNAME 0.0.0.0
fi
