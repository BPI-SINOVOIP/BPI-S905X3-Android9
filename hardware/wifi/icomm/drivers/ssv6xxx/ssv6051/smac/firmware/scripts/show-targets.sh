##############################################################################
#
#
#
##############################################################################
#!/bin/sh

die() { echo "$@" 1>&2; exit; }

CONFIG_DIR=$1
CFG_FILES=$(find $CONFIG_DIR -name *.cfg)
DEF_CFG="$CONFIG_DIR/defconfig"

# make sure the 'defconfig' exists 
[ -f "$DEF_CFG" ] || die "'$(basename $DEF_CFG)' not found !!"

prj_def=$(cat $DEF_CFG | grep -e '^CONFIG_TARGET[[:blank:]]*=' | sed 's/=/ /g' | awk '{ print $2 }' )
hci_def=$(cat $DEF_CFG | grep -e '^CONFIG_HCI[[:blank:]]*=' | sed 's/=/ /g' | awk '{ print $2 }' )

echo $CONFIG_DIR
echo
echo "Configuration File List:"
for cfg_file in $CFG_FILES
do	
	cfgfile=$(basename $cfg_file)
	#echo "  [ ] $cfgfile"
	prj_name=$(cat $cfg_file | grep -e '^CONFIG_TARGET[[:blank:]]*=' | sed 's/=/ /g' | awk '{ print $2 }' )
	hci_name=$(cat $cfg_file | grep -e '^CONFIG_HCI[[:blank:]]*=' | sed 's/=/ /g' | awk '{ print $2 }' )
	
	if [ "$prj_def" = "$prj_name" ] && [ "$hci_def" = "$hci_name" ]; then
		echo "  [*] $prj_name-$hci_name ($cfgfile)"
	else
		echo "  [ ] $prj_name-$hci_name ($cfgfile)"	
	fi 

done
echo



