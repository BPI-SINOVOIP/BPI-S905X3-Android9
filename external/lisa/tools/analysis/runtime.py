#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
#
# Copyright (C) 2017, ARM Limited, Google, and contributors.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from time import gmtime, strftime
from collections import namedtuple
import trappy, os, sys
import numpy as np
import pandas as pd
import argparse

class RunData:
    def __init__(self, pid, comm, time):
        self.pid = pid
        self.comm = comm
        self.last_start_time = time
        self.total_time = np.float64(0.0)
        self.start_time = -1
        self.end_time = -1
        self.running = 1
        self.maxrun = -1

parser = argparse.ArgumentParser(description='Analyze runnable times')

parser.add_argument('--trace', dest='trace_file', action='store', required=True,
                    help='trace file')

parser.add_argument('--rt', dest='rt', action='store_true', default=False,
                    help='only consider RT tasks')

parser.add_argument('--normalize', dest='normalize', action='store_true', default=False,
                    help='normalize time')

parser.add_argument('--rows', dest='nrows', action='store', default=20, type=int,
                    help='normalize time')

parser.add_argument('--total', dest='lat_total', action='store_true', default=False,
                    help='sort by total runnable time')

parser.add_argument('--start-time', dest='start_time', action='store', default=0, type=float,
                    help='trace window start time')

parser.add_argument('--end-time', dest='end_time', action='store', default=None, type=float,
                    help='trace window end time')

args = parser.parse_args()

path_to_html = args.trace_file
rtonly = args.rt
nrows = args.nrows

# Hash table for runtimes
runpids = {}

# Debugging aids for debugging within the callbacks
dpid = -1       # Debug all pids
debugg = False  # Global debug switch
printrows = False   # Debug aid to print all switch and wake events in a time range

switch_events = []

starttime = None
endtime = None

def switch_cb(data):
    global starttime, endtime, dpid

    prevpid = data['prev_pid']
    nextpid = data['next_pid']
    time = data['Index']

    if not starttime:
        starttime = time
    endtime = time

    debug = (dpid == prevpid or dpid == nextpid or dpid == -1) and debugg

    if debug:
        print "{}: {} {} -> {} {}".format(time, prevpid, data['prev_comm'], nextpid, data['next_comm'])

    if prevpid != 0:
       # prev pid processing (switch out)
        if runpids.has_key(prevpid):
            rp = runpids[prevpid]
            if rp.running == 1:
                rp.running = 0
                runtime = time - rp.last_start_time
                if runtime > rp.maxrun:
                    rp.maxrun = runtime
                    rp.start_time = rp.last_start_time
                    rp.end_time = time
                rp.total_time += runtime
                if debug and dpid == prevpid: print 'adding to total time {}, new total {}'.format(runtime, rp.total_time)

            else:
                print 'switch out seen while no switch in {}'.format(prevpid)
        else:
            print 'switch out seen while no switch in {}'.format(prevpid)

    if nextpid == 0:
        return

    # nextpid processing  (switch in)
    if not runpids.has_key(nextpid):
        # print 'creating data for nextpid {}'.format(nextpid)
        rp = RunData(nextpid, data['next_comm'], time)
        runpids[nextpid] = rp
        return

    rp = runpids[nextpid]
    if rp.running == 1:
        print 'switch in seen for already running task {}'.format(nextpid)
        return

    rp.running = 1
    rp.last_start_time = time


if args.normalize:
    kwargs = { 'window': (args.start_time, args.end_time) }
else:
    kwargs = { 'abs_window': (args.start_time, args.end_time) }

systrace_obj = trappy.SysTrace(name="systrace", path=path_to_html, \
        scope="sched", events=["sched_switch"], normalize_time=args.normalize, **kwargs)

ncpus = systrace_obj.sched_switch.data_frame["__cpu"].max() + 1
print 'cpus found: {}\n'.format(ncpus)

systrace_obj.apply_callbacks({ "sched_switch": switch_cb })

## Print results
testtime = (endtime - starttime) * ncpus              # for 4 cpus
print "total test time (scaled by number of CPUs): {} secs".format(testtime)

# Print the results: PID, latency, start, end, sort
result = sorted(runpids.items(), key=lambda x: x[1].total_time, reverse=True)
print "PID".ljust(10) + "\t" + "name".ljust(15) + "\t" + "max run (secs)".ljust(15) + \
      "\t" + "start time".ljust(15) + "\t" + "end time".ljust(15) + "\t" + "total runtime".ljust(15) + "\t" + "percent cpu".ljust(15) \
      + "\t" + "totalpc"

totalpc = np.float64(0.0)
for r in result[:nrows]:
	rd = r[1] # RunData named tuple
	if rd.pid != r[0]:
		raise RuntimeError("BUG: pid inconsitency found") # Sanity check
        start = rd.start_time
        end = rd.end_time
        cpupc = (rd.total_time / testtime) * 100
        totalpc += cpupc

	print str(r[0]).ljust(10) + "\t" + str(rd.comm).ljust(15) + "\t" + \
		  str(rd.maxrun).ljust(15)[:15] + "\t" + str(start).ljust(15)[:15] + \
		  "\t" + str(end).ljust(15)[:15] + "\t" + str(rd.total_time).ljust(15) + \
                    "\t" + str(cpupc) \
                    + "\t" + str(totalpc)

#############################################################
## Debugging aids to print all events in a given time range #
#############################################################
def print_event_rows(events, start, end):
	print "time".ljust(20) + "\t" + "event".ljust(10) + "\tpid" + "\tprevpid" + "\tnextpid"
	for e in events:
		if e.time < start or e.time > end:
			continue
		if e.event == "switch":
			nextpid =  e.data['next_pid']
			prevpid = e.data['prev_pid']
			pid = -1
		elif e.event == "wake":
			pid = e.data['pid']
			nextpid = -1
			prevpid = -1
		else:
			raise RuntimeError("unknown event seen")
		print str(e.time).ljust(20)[:20] + "\t" + e.event.ljust(10) + "\t" + str(pid) + "\t" + str(prevpid) + "\t" + str(nextpid)

if printrows:
    rows = switch_events + wake_events
    rows =  sorted(rows, key=lambda r: r.time)
    print_event_rows(rows, 1.1, 1.2)
