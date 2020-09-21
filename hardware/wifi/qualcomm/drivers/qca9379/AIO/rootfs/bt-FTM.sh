#!/bin/sh
# This script is used to make Btdiag successfully reset(HCI_RESET)
# BT FW during BT FTM test.
# It mainly updates the content of nvm file
#
# Usage:
#     ./<this-script-name> <nvm-file-name>
#     <nvm-file-name>: the nvm binary file to be converted.
#
# After run the script, please use ${1}.ftm to do FTM test

cp "$1" "${1}.ftm"
vi -b "${1}.ftm" <<COMMANDS
:%!xxd
:%s/0000030/0000030/
:.s/82/02/
:%!xxd -r
:wq!
COMMANDS
