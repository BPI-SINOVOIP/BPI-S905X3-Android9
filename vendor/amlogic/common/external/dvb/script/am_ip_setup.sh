#!/bin/sh

#设定IP地址
#命令: am_ip_setup.sh dynamic
#    设定主机的IP地址和DNS为通过DHCP自动获得
#命令: am_ip_setup.sh static IPADDR NETMASK GATEWAY DNS1 [DNS2]
#    静态设定主机的IP地址,子网掩码,网关，DNS服务器
usage()
{
	CMD=`basename $0`
	echo usage: $CMD dynamic MAC
	echo        $CMD static MAC IPADDR NETMASK GATEWAY DNS1 [DNS2]
}

DHCPC="udhcpc"
IFCONFIG="ifconfig"
ROUTE="route"
RESOLVCONF="/etc/resolv.conf"

#DHCPC="echo udhcpc"
#IFCONFIG="echo ifconfig"
#ROUTE="echo route"

#DHCPC="dhclient"
#IFCONFIG="ifconfig"
#ROUTE="route"

ETH="eth0"

PROT=$1

#Kill all DHCPC
RET=0
while [ $RET = 0 ]; do
	killall -9 ${DHCPC}
	RET=$?
done

MAC=$2

if [ "x$MAC" = "x" ]; then
	exit 0
fi

${IFCONFIG} ${ETH} down
${IFCONFIG} ${ETH} hw ether ${MAC}

if [ "x$PROT" = "xstatic" ]; then
	#Static IP
	IPADDR=$3
	NETMASK=$4
	GWADDR=$5
	DNS1=$6
	DNS2=$7

	if [ "x$IPADDR" = "x" -o "x$NETMASK" = "x" ]; then
		exit 0
	fi
	
	${IFCONFIG} ${ETH} down
	${IFCONFIG} ${ETH} ${IPADDR} netmask ${NETMASK} up

	if [ "x$GWADDR" != "x" ]; then
		${ROUTE} add default gw ${GWADDR} ${ETH}
	fi

	if [ "x$DNS1" != "x" ]; then
		echo nameserver ${DNS1} > ${RESOLVCONF}
	fi

	if [ "x$DNS2" != "x" ]; then
		echo nameserver ${DNS2} >> ${RESOLVCONF}
	fi
elif [ "x$PROT" = "xdynamic" ]; then
	#DHCP
	${DHCPC} &
else
	usage
	exit 1
fi
