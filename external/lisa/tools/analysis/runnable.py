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

from collections import namedtuple
import trappy
import argparse

EventData = namedtuple('EventData', ['time', 'event', 'data'])
LatData = namedtuple('LatData', ['pid', 'switch_data', 'wake_data', 'last_wake_data', 'running', 'wake_pend', 'latency', 'lat_total'])

# path_to_html = "/home/joelaf/repo/catapult/systrace/trace_sf-dispsync_rt_pull_fail_1504ms.html"
# path_to_html   = "/home/joelaf/Downloads/trace_sf_late_wakeup_ipi.html"
# path_to_html = "/home/joelaf/repo/lisa/results/ChromeMonkey/trace.html"

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

# "/home/joelaf/repo/lisa-aosp/results/camera-trace-manual/trace.html"
# path_to_html = "/home/joelaf/repo/catapult/systrace/trace_sf-dispsync_rt_pull_fail_1504ms.html"
# path_to_html   = "/home/joelaf/Downloads/trace_sf_late_wakeup_ipi.html"
# path_to_html = "/home/joelaf/repo/lisa/results/ChromeMonkey/trace.html"


# Hash table of pid -> LatData named tuple
latpids = {}

# Debugging aids for debugging within the callbacks
dpid = 21047
debugg = False

normalize = False   # Normalize with the base timestamp
printrows = False   # Debug aid to print all switch and wake events in a time range

switch_events = []
wake_events = []
basetime = None

def switch_cb(data):
    event = "switch"
    prevpid = data['prev_pid']
    nextpid = data['next_pid']
    time = data['Index']

    # print str(time) + ": " + str(data)

    global basetime, switch_events
    # basetime = time if (normalize and not basetime) else basetime
    # time = time - basetime if normalize else time

    e = EventData(time, event, data)
    if printrows:
        switch_events.append(e)

    debug = False # debugg and (prevpid == dpid or nextpid == dpid)
    if debug: print ("\nProcessing switch nextpid=" + str(nextpid) + " prevpid=" + str(prevpid) + \
                     " time=" + str(time))

    # prev pid processing (switch out)
    if latpids.has_key(prevpid):
        if latpids[prevpid].running == 1:
            latpids[prevpid] = latpids[prevpid]._replace(running=0)
        if latpids[prevpid].wake_pend == 1:
            print "Impossible: switch out during wake_pend " + str(e)
            latpids[prevpid] = latpids[prevpid]._replace(wake_pend=0)
            return
            # raise RuntimeError("error")

    if not latpids.has_key(nextpid):
        return

    # nextpid processing  (switch in)
    pid = nextpid
    if latpids[pid].running == 1:
        print "INFO: previous pid switch-out not seen for an event, ignoring\n" + str(e)
        return
    latpids[pid] = latpids[pid]._replace(running=1)

    # Ignore latency calc for next-switch events for which wake never seen
    # They are still valid in this scenario because of preemption
    if latpids[pid].wake_pend == 0:
        if debug: print "wake_pend = 0, doing nothing"
        return

    if debug: print "recording"
    # Measure latency
    lat = time - latpids[pid].last_wake_data.time
    total = latpids[pid].lat_total + lat
    latpids[pid] = latpids[pid]._replace(lat_total=total)

    if lat > latpids[pid].latency:
        latpids[pid] = LatData(pid, switch_data = e,
                               wake_data = latpids[pid].last_wake_data,
                               last_wake_data=None, latency=lat, lat_total=latpids[pid].lat_total, running=1, wake_pend=0)
        return
    latpids[pid] = latpids[pid]._replace(running=1, wake_pend=0)

def wake_cb(data):
    event = "wake"
    pid = data['pid']
    time = data['Index']
    debug = False # debugg and (pid == dpid)

    global basetime, wake_events
    # basetime = time if (normalize and not basetime) else basetime
    # time = time - basetime if normalize else time

    e = EventData(time, event, data)
    wake_events.append(e)

    if rtonly and data["prio"] > 99:
        return

    if debug: print "\nProcessing wake pid=" + str(pid) + " time=" + str(time)
    if  not latpids.has_key(pid):
        latpids[pid] = LatData(pid, switch_data=None, wake_data=None,
                last_wake_data = e, running=0, latency=-1, lat_total=0, wake_pend=1)
        if debug: print "new wakeup"
        return

    if latpids[pid].running == 1 or latpids[pid].wake_pend == 1:
        if debug: print "already running or wake_pend"
        # Task already running or a wakeup->switch pending, ignore
        return

    if debug: print "recording wake"
    latpids[pid] = latpids[pid]._replace(last_wake_data = e, wake_pend=1)


if args.normalize:
    kwargs = { 'window': (args.start_time, args.end_time) }
else:
    kwargs = { 'abs_window': (args.start_time, args.end_time) }

systrace_obj = trappy.SysTrace(name="systrace", path=path_to_html, \
        scope="sched", events=["sched_switch", "sched_wakeup", "sched_waking"], normalize_time=args.normalize, **kwargs)

systrace_obj.apply_callbacks({ "sched_switch": switch_cb, "sched_wakeup": wake_cb, \
                          "sched_waking": wake_cb })

# Print the results: PID, latency, start, end, sort
if args.lat_total:
    result = sorted(latpids.items(), key=lambda x: x[1].lat_total, reverse=True)
else:
    result = sorted(latpids.items(), key=lambda x: x[1].latency, reverse=True)

print "PID".ljust(10) + "\t" + "name".ljust(20) + "\t" + "latency (secs)".ljust(20) + \
      "\t" + "start time".ljust(20) + "\t" + "end time".ljust(20) + "\t" + "total (secs)".ljust(20)
for r in result[:nrows]:
	l = r[1] # LatData named tuple
	if l.pid != r[0]:
		raise RuntimeError("BUG: pid inconsitency found") # Sanity check
        wake_time   = l.wake_data.time
        switch_time = l.switch_data.time
        total = l.lat_total

	print str(r[0]).ljust(10) + "\t" + str(l.wake_data.data['comm']).ljust(20) + "\t" + \
		  str(l.latency).ljust(20)[:20] + "\t" + str(wake_time).ljust(20)[:20] + \
		  "\t" + str(switch_time).ljust(20)[:20] + "\t" + str(total).ljust(20)[:20]

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
