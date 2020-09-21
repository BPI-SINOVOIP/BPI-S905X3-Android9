#! /bin/bash -u
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# This script first collects the addresses of the instructions being tracked by
# the profile. After that, it calculates the offset of the addresses compared
# to the base address and then gets the number of execution times for each
# address. After that, it draws the heat map and the time map. A heap map shows
# the overall hotness of instructions being executed while the time map shows the
# hotness of instruction at different time.

# binary : the name of the binary
# profile : output of 'perf report -D'
# loading_address  : the loading address of the binary
# page_size : the size to be displayed, usually 4096(byte).

if [[ $# -ne 4 ]]; then
     echo 'Illegal number of parameters' exit 1
fi

binary=$1
profile=$2
loading_address=$3
page_size=$4

# size of binary supported.
binary_maximum=1000000000

if ! [[ -e $profile ]] ; then
     echo "error: profile does not exist" >&2; exit 1
fi

re='^[0-9]+$'
if ! [[ $page_size =~ $re ]] ; then
     echo "error: page_size is not a number" >&2; exit 1
fi

function test {
    "$@"
    local status=$?
    if [ $status -ne 0 ]; then
        echo "error with $1" >&2
    fi
    return $status
}

HEAT_PNG="heat_map.png"
TIMELINE_PNG="timeline.png"

test  grep -A 2 PERF_RECORD_SAMPLE $profile | grep -A 1 -B 1 "thread: $binary" | \
grep -B 2 "dso.*$binary$" | awk -v base=$loading_address \
     "BEGIN { count=0; } /PERF_RECORD_SAMPLE/ {addr = strtonum(\$8) - strtonum(base); \
     if (addr < $binary_maximum) count++; \
     if (addr < $binary_maximum) print \$7,count,int(addr/$page_size)*$page_size}" >  out.txt


test  awk '{print $3}' out.txt | sort -n | uniq -c > inst-histo.txt

# generate inst heat map
echo "
set terminal png size 600,450
set xlabel \"Instruction Virtual Address (MB)\"
set ylabel \"Sample Occurance\"
set grid

set output \"${HEAT_PNG}\"
set title \"Instruction Heat Map\"

plot 'inst-histo.txt' using (\$2/1024/1024):1 with impulses notitle
" | test gnuplot

# generate instruction page access timeline
num=$(awk 'END {print NR+1}' out.txt)

echo "
set terminal png size 600,450
set xlabel \"time (sec)\"
set ylabel \"Instruction Virtual Address (MB)\"

set output \"${TIMELINE_PNG}\"
set title \"instruction page accessd timeline\"

plot 'out.txt' using (\$0/$num*10):(\$3/1024/1024) with dots notitle
" | test gnuplot
