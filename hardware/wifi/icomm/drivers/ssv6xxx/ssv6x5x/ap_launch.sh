#! /bin/bash

BLUE='\e[1;34m'
GREEN='\e[1;32m'
CYAN='\e[1;36m'
RED='\e[1;31m'
PURPLE='\e[1;35m'
YELLOW='\e[1;33m'
# No Color
NC='\e[0m'


HOSTPAD_DIR=../hostapd/hostapd

./ap_shutdown.sh

dir=$(pwd)
chmod 777 -R $HOSTPAD_DIR
cd $HOSTPAD_DIR

PID=$!
wait $PID
sleep 2


cd $dir
echo -e "${YELLOW}Load wireless driver...${NC}"
./load.sh
PID=$!
wait $PID

echo -e "${YELLOW}Detect wireless device...${NC}"
sleep 2
#get driver mac address
macAddr=$(cat sta.cfg| grep "hw_mac"|sed 1q |tr -d ' '|tail -c 18)
echo "device macaddr=[$macAddr]" 

#get driver name(wlan??)
#devName=$(ifconfig | grep $macAddr)
devName=$(ifconfig -a | grep -i $macAddr)
devName=`echo $devName| cut -d ' ' -f 1`

ifconfig $devName > /dev/null

if [ $? -ne 0 ]; then
    echo -e "${RED}Device $devName does not exist.${NC}"
    exit 1;
else
    echo -e "${YELLOW}Device is $devName.${NC}"
fi

echo -e "${YELLOW}Config wireless AP...${NC}"
#rm -rf load_dhcp.sh
#rm -rf hostapd.conf
#relpace wlan@@ to real device name
cp script/template/load_dhcp.sh load_dhcp.sh
#cp script/template/hostapd.conf hostapd.conf
awk 'NF' script/template/hostapd.conf | grep -v '#' > hostapd.conf
awk 'NF' ap.cfg | grep -v '#' >> hostapd.conf

sed -i "s/wlan@@/$devName/" load_dhcp.sh
sed -i "s/wlan@@/$devName/" hostapd.conf

chmod 777 load_dhcp.sh

#move to right position
#mv load_dhcp.sh $HOSTPAD_DIR
#mv hostapd.conf $HOSTPAD_DIR/hostapd/

dhcp_config_file="/etc/default/isc-dhcp-server"
dhcp_config=$(grep "$devName" $dhcp_config_file)
if [ "$dhcp_config" != "$devName" ]; then
	echo -en "${YELLOW}Config $dhcp_config_file.....${NC}"
	
	rm -rf tmp
	sed '/INTERFACE/d' /etc/default/isc-dhcp-server >>tmp
	echo "INTERFACES=\"$devName\"" >>tmp	
	rm -rf $dhcp_config_file	
	mv tmp /etc/default/isc-dhcp-server
	
	echo -e "${YELLOW}OK${NC}"
fi
	
	
dir=$(pwd)
echo -e "${YELLOW}Wireless Done. ${NC}"
trap handle_stop INT

function version_great() { test "$(printf '%s\n' "$@" | sort -V | head -n 1)" != "$1"; }
nmcli_version=$(nmcli -v | cut -d ' ' -f 4)
chk_nmcli_version=0.9.8.999

function handle_stop() {
#    popd
    if version_great $nmcli_version $chk_nmcli_version; then
        nmcli radio wifi on
    else
        nmcli nm wifi on
    fi
    
    echo -e "${YELLOW}Shutting down AP.${NC}"
    ./ap_shutdown.sh
}
        
if version_great $nmcli_version $chk_nmcli_version; then
    nmcli radio wifi off
else
    nmcli nm wifi off
fi

sudo rfkill unblock wlan

#pushd $HOSTPAD_DIR
#. ./load_ap.sh
#$HOSTPAD_DIR/load_dhcp.sh &
./load_dhcp.sh &
PID=$!
wait $PID

echo -e "${YELLOW}Load AP...${NC}"
echo -e "${GREEN}Launch hostapd.${NC}"
#run hostapd2.0
$HOSTPAD_DIR/hostapd -t hostapd.conf
#hostapd -t hostapd.conf
