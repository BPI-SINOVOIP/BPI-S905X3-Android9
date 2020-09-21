#!/bin/sh
# Copyright (c) 2017 Oracle and/or its affiliates. All Rights Reserved.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 2 of
# the License, or (at your option) any later version.
#
# This program is distributed in the hope that it would be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#
# Author: Alexey Kodanev <alexey.kodanev@oracle.com>

TCID=sctp01
TST_TOTAL=4
TST_CLEANUP="cleanup"
TST_NEEDS_TMPDIR=1

. test_net.sh

cleanup()
{
	tst_rmdir
}

setup()
{
	tst_require_root
	TST_NETLOAD_MAX_SRV_REPLIES=3
}

test_run()
{
	local opts="$@"

	tst_resm TINFO "compare TCP/SCTP performance"

	tst_netload -H $(tst_ipaddr rhost) -T tcp $opts
	local res0="$(cat tst_netload.res)"

	tst_netload -S $(tst_ipaddr) -H $(tst_ipaddr rhost) -T sctp $opts
	local res1="$(cat tst_netload.res)"

	local per=$(( $res0 * 100 / $res1 - 100 ))

	if [ "$per" -gt "100" -o "$per" -lt "-100" ]; then
		tst_resm TFAIL "sctp performance $per %"
	else
		tst_resm TPASS "sctp performance $per % in range -100 ... 100 %"
	fi
}

setup
test_run
test_run -A 65000

tst_exit
