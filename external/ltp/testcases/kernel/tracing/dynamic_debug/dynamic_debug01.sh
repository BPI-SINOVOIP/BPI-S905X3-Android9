#!/bin/sh

# Copyright (C) 2017 Red Hat, Inc.
#
# This program is free software;  you can redistribute it and#or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#
#
# Test description: Test functionality of dynamic debug feature by enabling
#                   and disabling traces with available flags. Check that these
#                   settings don't cause issues by searching dmesg.
#
#                   This test handles changes of dynamic debug interface from
#                   commits 5ca7d2a6 (dynamic_debug: describe_flags with
#                   '=[pmflt_]*') and 8ba6ebf5 (Dynamic debug: Add more flags)
#
# Usage
# ./dynamic_debug01.sh

TST_TESTFUNC=ddebug_test
TST_NEEDS_CMDS="awk /bin/echo"
TST_NEEDS_TMPDIR=1
TST_NEEDS_ROOT=1
TST_SETUP=setup
TST_CLEANUP=cleanup

. tst_test.sh


DEBUGFS_WAS_MOUNTED=0
DEBUGFS_PATH=""
DEBUGFS_CONTROL=""
DYNDEBUG_STATEMENTS="./debug_statements"
EMPTY_FLAG="-"
NEW_INTERFACE=0


mount_debugfs()
{
	if grep -q debugfs /proc/mounts ; then
		DEBUGFS_WAS_MOUNTED=1
		DEBUGFS_PATH=$(awk '/debugfs/{print $2}' /proc/mounts)
		tst_res TINFO "debugfs already mounted at $DEBUGFS_PATH"
	else
		if ! grep -q debugfs /proc/filesystems ; then
			tst_res TCONF "debugfs not supported"
		fi
		DEBUGFS_PATH="./tst_debug"
		mkdir "$DEBUGFS_PATH"
		if mount -t debugfs xxx "$DEBUGFS_PATH" ; then
			tst_res TINFO "debugfs mounted at $DEBUGFS_PATH"
		else
			tst_res TFAIL "Unable to mount debugfs"
		fi
	fi
}

setup()
{
	if tst_kvcmp -lt 2.6.30 ; then
		tst_brk TCONF "Dynamic debug is available since version 2.6.30"
	fi

	mount_debugfs
	if [ ! -d "$DEBUGFS_PATH/dynamic_debug" ] ; then
		tst_brk TBROK "Unable to find $DEBUGFS_PATH/dynamic_debug"
	fi
	DEBUGFS_CONTROL="$DEBUGFS_PATH/dynamic_debug/control"
	if [ ! -e "$DEBUGFS_CONTROL" ] ; then
		tst_brk TBROK "Unable to find $DEBUGFS_CONTROL"
	fi

	# Both patches with changes were backported to RHEL6 kernel 2.6.32-547
	if tst_kvcmp -ge '3.4 RHEL6:2.6.32-547' ; then
		NEW_INTERFACE=1
		EMPTY_FLAG="=_"
	fi

	grep -v "^#" "$DEBUGFS_CONTROL" > "$DYNDEBUG_STATEMENTS"
}

do_flag()
{
	local FLAG="$1"
	local OPTION_TO_SET="$2"
	local OPTION_VALUE="$3"

	if ! echo "$OPTION_TO_SET $OPTION_VALUE $FLAG" > \
		"$DEBUGFS_CONTROL" ; then
		tst_res TFAIL "Setting '$OPTION_TO_SET $OPTION_VALUE " \
			"$FLAG' failed with $?!"
	fi
}

do_all_flags()
{
	OPTION="$1"
	ALL_INPUTS="$2"

	for INPUT_LINE in $ALL_INPUTS; do
		do_flag "+p" "$OPTION" "$INPUT_LINE"
		if tst_kvcmp -ge 3.2 || [ $NEW_INTERFACE -eq 1 ] ; then
			do_flag "+flmt" "$OPTION" "$INPUT_LINE"
			do_flag "-flmt" "$OPTION" "$INPUT_LINE"
		fi
		do_flag "-p" "$OPTION" "$INPUT_LINE"
	done

	if awk -v emp="$EMPTY_FLAG" '$3 != emp' "$DEBUGFS_CONTROL" \
		| grep -v -q "^#" ; then
		tst_res TFAIL "Failed to remove all set flags"
	fi
}

ddebug_test()
{
	dmesg > ./dmesg.old

	DD_FUNCS=$(awk -F " |]" '{print $3}' "$DYNDEBUG_STATEMENTS" \
		| sort | uniq)
	DD_FILES=$(awk -F " |:" '{print $1}' "$DYNDEBUG_STATEMENTS" \
		| sort | uniq)
	DD_LINES=$(awk -F " |:" '{print $2}' "$DYNDEBUG_STATEMENTS" \
		| sort | uniq)
	DD_MODULES=$(awk -F [][] '{print $2}' "$DYNDEBUG_STATEMENTS" \
		| sort | uniq)

	do_all_flags "func" "$DD_FUNCS"
	do_all_flags "file" "$DD_FILES"
	do_all_flags "line" "$DD_LINES"
	do_all_flags "module" "$DD_MODULES"

	dmesg > ./dmesg.new
	sed -i -e 1,`wc -l < ./dmesg.old`d ./dmesg.new
	if grep -q -e "Kernel panic" -e "Oops" -e "general protection fault" \
		-e "general protection handler: wrong gs" -e "\(XEN\) Panic" \
		-e "fault" -e "warn" -e "BUG" ./dmesg.new ; then
		tst_res TFAIL "Issues found in dmesg!"
	else
		tst_res TPASS "Dynamic debug OK"
	fi
}

cleanup()
{
	FLAGS_SET=$(awk -v emp="$EMPTY_FLAG" '$3 != emp' $DYNDEBUG_STATEMENTS)
	if [ "$FLAGS_SET" ] ; then
		FLAG_PREFIX=$([ $NEW_INTERFACE -eq 1 ] && echo "" || echo "+")
		/bin/echo "$FLAGS_SET" | while read -r FLAG_LINE ; do
			/bin/echo -n "$FLAG_LINE" \
				| awk -v prf="$FLAG_PREFIX" -F " |:" \
				'{print "file "$1" line "$2" "prf $4}' \
				> "$DEBUGFS_CONTROL"
		done
	fi
	if [ $DEBUGFS_WAS_MOUNTED -eq 0 ] ; then
		tst_umount "$DEBUGFS_PATH"
	fi
}

tst_run
