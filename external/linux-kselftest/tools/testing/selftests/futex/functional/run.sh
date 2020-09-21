#!/bin/sh

###############################################################################
#
#   Copyright © International Business Machines  Corp., 2009
#
#   This program is free software;  you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation; either version 2 of the License, or
#   (at your option) any later version.
#
# DESCRIPTION
#      Run tests in the current directory.
#
# AUTHOR
#      Darren Hart <dvhart@linux.intel.com>
#
# HISTORY
#      2009-Nov-9: Initial version by Darren Hart <dvhart@linux.intel.com>
#      2010-Jan-6: Add futex_wait_uninitialized_heap and futex_wait_private_mapped_file
#                  by KOSAKI Motohiro <kosaki.motohiro@jp.fujitsu.com>
#
###############################################################################

run_test()
{
	$@
	if [ $? -ne 0 ]; then
		rc=1
	fi
}

# Test for a color capable console
if [ -z "$USE_COLOR" ]; then
    tput setf 7 || tput setaf 7
    if [ $? -eq 0 ]; then
        USE_COLOR=1
        tput sgr0
    fi
fi
if [ "$USE_COLOR" -eq 1 ]; then
    COLOR="-c"
fi

rc=0

echo
# requeue pi testing
# without timeouts
run_test ./futex_requeue_pi $COLOR
run_test ./futex_requeue_pi $COLOR -b
run_test ./futex_requeue_pi $COLOR -b -l
run_test ./futex_requeue_pi $COLOR -b -o
run_test ./futex_requeue_pi $COLOR -l
run_test ./futex_requeue_pi $COLOR -o
# with timeouts
run_test ./futex_requeue_pi $COLOR -b -l -t 5000
run_test ./futex_requeue_pi $COLOR -l -t 5000
run_test ./futex_requeue_pi $COLOR -b -l -t 500000
run_test ./futex_requeue_pi $COLOR -l -t 500000
run_test ./futex_requeue_pi $COLOR -b -t 5000
run_test ./futex_requeue_pi $COLOR -t 5000
run_test ./futex_requeue_pi $COLOR -b -t 500000
run_test ./futex_requeue_pi $COLOR -t 500000
run_test ./futex_requeue_pi $COLOR -b -o -t 5000
run_test ./futex_requeue_pi $COLOR -l -t 5000
run_test ./futex_requeue_pi $COLOR -b -o -t 500000
run_test ./futex_requeue_pi $COLOR -l -t 500000
# with long timeout
run_test ./futex_requeue_pi $COLOR -b -l -t 2000000000
run_test ./futex_requeue_pi $COLOR -l -t 2000000000


echo
run_test ./futex_requeue_pi_mismatched_ops $COLOR

echo
run_test ./futex_requeue_pi_signal_restart $COLOR

echo
run_test ./futex_wait_timeout $COLOR

echo
run_test ./futex_wait_wouldblock $COLOR

echo
run_test ./futex_wait_uninitialized_heap $COLOR
run_test ./futex_wait_private_mapped_file $COLOR

exit $rc
