#!/usr/bin/env python3

import argparse
import datetime
import json
import matplotlib.pyplot as plt
import sys

class UidSnapshot(object):
    def __init__(self, activity):
        self.uid = activity['uid']
        self.foregroundWrittenBytes = activity['foregroundWrittenBytes']
        self.foregroundFsyncCalls = activity['foregroundFsyncCalls']
        self.backgroundFsyncCalls = activity['backgroundFsyncCalls']
        self.backgroundWrittenBytes = activity['backgroundWrittenBytes']
        self.appPackages = activity['appPackages']
        self.runtimeMs = activity['runtimeMs']
        self.totalWrittenBytes = self.foregroundWrittenBytes + self.backgroundWrittenBytes
        self.totalFsyncCalls = self.backgroundFsyncCalls + self.foregroundFsyncCalls
        if self.appPackages is None: self.appPackages = []

class Snapshot(object):
    def __init__(self, activity, uptime):
        self.uptime = uptime
        self.uids = {}
        self.foregroundWrittenBytes = 0
        self.foregroundFsyncCalls = 0
        self.backgroundFsyncCalls = 0
        self.backgroundWrittenBytes = 0
        self.totalWrittenBytes = 0
        self.totalFsyncCalls = 0
        for entry in activity:
            uid = entry['uid']
            snapshot = UidSnapshot(entry)
            self.foregroundWrittenBytes += snapshot.foregroundWrittenBytes
            self.foregroundFsyncCalls += snapshot.foregroundFsyncCalls
            self.backgroundFsyncCalls += snapshot.backgroundFsyncCalls
            self.backgroundWrittenBytes += snapshot.backgroundWrittenBytes
            self.totalWrittenBytes += snapshot.totalWrittenBytes
            self.totalFsyncCalls += snapshot.totalFsyncCalls
            self.uids[uid] = snapshot

class Document(object):
    def __init__(self, f):
        self.snapshots = []
        uptimes = [0, 0]
        for line in f:
            line = json.loads(line)
            if line['type'] != 'snapshot': continue
            activity = line['activity']
            uptime = line['uptime']
            if uptime < uptimes[0]: uptimes[0] = uptime
            if uptime > uptimes[1]: uptimes[1] = uptime
            self.snapshots.append(Snapshot(activity, uptime))
        self.runtime = datetime.timedelta(milliseconds=uptimes[1]-uptimes[0])

def merge(l1, l2):
    s1 = set(l1)
    s2 = set(l2)
    return list(s1 | s2)


thresholds = [
    (1024 * 1024 * 1024 * 1024, "TB"),
    (1024 * 1024 * 1024, "GB"),
    (1024 * 1024, "MB"),
    (1024, "KB"),
    (1, "bytes")
]
def prettyPrintBytes(n):
    for t in thresholds:
        if n >= t[0]:
            return "%.1f %s" % (n / (t[0] + 0.0), t[1])
    return "0 bytes"

# knowledge extracted from android_filesystem_config.h
wellKnownUids = {
    0 :         ["linux kernel"],
    1010 :      ["wifi"],
    1013 :      ["mediaserver"],
    1017 :      ["keystore"],
    1019 :      ["DRM server"],
    1021 :      ["GPS"],
    1023 :      ["media storage write access"],
    1036 :      ["logd"],
    1040 :      ["mediaextractor"],
    1041 :      ["audioserver"],
    1046 :      ["mediacodec"],
    1047 :      ["cameraserver"],
    1053 :      ["webview zygote"],
    1054 :      ["vehicle hal"],
    1058 :      ["tombstoned"],
    1066 :      ["statsd"],
    1067 :      ["incidentd"],
    9999 :      ["nobody"],
}

class UserActivity(object):
    def __init__(self, uid):
        self.uid = uid
        self.snapshots = []
        self.appPackages = wellKnownUids.get(uid, [])
        self.foregroundWrittenBytes = 0
        self.foregroundFsyncCalls = 0
        self.backgroundFsyncCalls = 0
        self.backgroundWrittenBytes = 0
        self.totalWrittenBytes = 0
        self.totalFsyncCalls = 0

    def addSnapshot(self, snapshot):
        assert snapshot.uid == self.uid
        self.snapshots.append(snapshot)
        self.foregroundWrittenBytes += snapshot.foregroundWrittenBytes
        self.foregroundFsyncCalls += snapshot.foregroundFsyncCalls
        self.backgroundFsyncCalls += snapshot.backgroundFsyncCalls
        self.backgroundWrittenBytes += snapshot.backgroundWrittenBytes
        self.totalWrittenBytes += snapshot.totalWrittenBytes
        self.totalFsyncCalls += snapshot.totalFsyncCalls
        self.appPackages = merge(self.appPackages, snapshot.appPackages)

    def plot(self, foreground=True, background=True, total=True):
        plt.figure()
        plt.title("I/O activity for UID %s" % (self.uid))
        X = range(0,len(self.snapshots))
        minY = 0
        maxY = 0
        if foreground:
            Y = [s.foregroundWrittenBytes for s in self.snapshots]
            if any([y > 0 for y in Y]):
                plt.plot(X, Y, 'b-')
                minY = min(minY, min(Y))
                maxY = max(maxY, max(Y))
        if background:
            Y = [s.backgroundWrittenBytes for s in self.snapshots]
            if any([y > 0 for y in Y]):
                plt.plot(X, Y, 'g-')
                minY = min(minY, min(Y))
                maxY = max(maxY, max(Y))
        if total:
            Y = [s.totalWrittenBytes for s in self.snapshots]
            if any([y > 0 for y in Y]):
                plt.plot(X, Y, 'r-')
                minY = min(minY, min(Y))
                maxY = max(maxY, max(Y))

        i = int((maxY - minY) / 5)
        Yt = list(range(minY, maxY, i))
        Yl = [prettyPrintBytes(y) for y in Yt]
        plt.yticks(Yt, Yl)
        Xt = list(range(0, len(X)))
        plt.xticks(Xt)

class SystemActivity(object):
    def __init__(self):
        self.uids = {}
        self.snapshots = []
        self.foregroundWrittenBytes = 0
        self.foregroundFsyncCalls = 0
        self.backgroundFsyncCalls = 0
        self.backgroundWrittenBytes = 0
        self.totalWrittenBytes = 0
        self.totalFsyncCalls = 0

    def addSnapshot(self, snapshot):
        self.snapshots.append(snapshot)
        self.foregroundWrittenBytes += snapshot.foregroundWrittenBytes
        self.foregroundFsyncCalls += snapshot.foregroundFsyncCalls
        self.backgroundFsyncCalls += snapshot.backgroundFsyncCalls
        self.backgroundWrittenBytes += snapshot.backgroundWrittenBytes
        self.totalWrittenBytes += snapshot.totalWrittenBytes
        self.totalFsyncCalls += snapshot.totalFsyncCalls
        for uid in snapshot.uids:
            if uid not in self.uids: self.uids[uid] = UserActivity(uid)
            self.uids[uid].addSnapshot(snapshot.uids[uid])

    def loadDocument(self, doc):
        for snapshot in doc.snapshots:
            self.addSnapshot(snapshot)

    def sorted(self, f):
        return sorted(self.uids.values(), key=f, reverse=True)

    def pie(self):
        plt.figure()
        plt.title("Total disk writes per UID")
        A = [(K, self.uids[K].totalWrittenBytes) for K in self.uids]
        A = filter(lambda i: i[1] > 0, A)
        A = list(sorted(A, key=lambda i: i[1], reverse=True))
        X = [i[1] for i in A]
        L = [i[0] for i in A]
        plt.pie(X, labels=L, counterclock=False, startangle=90)

parser = argparse.ArgumentParser("Process FlashApp logs into reports")
parser.add_argument("filename")
parser.add_argument("--reportuid", action="append", default=[])
parser.add_argument("--plotuid", action="append", default=[])
parser.add_argument("--totalpie", action="store_true", default=False)

args = parser.parse_args()

class UidFilter(object):
    def __call__(self, uid):
        return False

class UidFilterAcceptAll(UidFilter):
    def __call__(self, uid):
        return True

class UidFilterAcceptSome(UidFilter):
    def __init__(self, uids):
        self.uids = uids

    def __call__(self, uid):
        return uid in self.uids

uidset = set()
plotfilter = None
for uid in args.plotuid:
    if uid == "all":
        plotfilter = UidFilterAcceptAll()
        break
    else:
        uidset.add(int(uid))
if plotfilter is None: plotfilter = UidFilterAcceptSome(uidset)

uidset = set()
reportfilter = None
for uid in args.reportuid:
    if uid == "all":
        reportfilter = UidFilterAcceptAll()
        break
    else:
        uidset.add(int(uid))
if reportfilter is None:
    if len(uidset) == 0:
        reportfilter = UidFilterAcceptAll()
    else:
        reportfilter = UidFilterAcceptSome(uidset)

document = Document(open(args.filename))
print("System runtime: %s\n" % (document.runtime))
system = SystemActivity()
system.loadDocument(document)

print("Total bytes written: %s (of which %s in foreground and %s in background)\n" % (
        prettyPrintBytes(system.totalWrittenBytes),
        prettyPrintBytes(system.foregroundWrittenBytes),
        prettyPrintBytes(system.backgroundWrittenBytes)))

writemost = filter(lambda ua: ua.totalWrittenBytes > 0, system.sorted(lambda ua: ua.totalWrittenBytes))
for entry in writemost:
    if reportfilter(entry.uid):
        print("user id %d (%s) wrote %s (of which %s in foreground and %s in background)" % (
            entry.uid,
            ','.join(entry.appPackages),
            prettyPrintBytes(entry.totalWrittenBytes),
            prettyPrintBytes(entry.foregroundWrittenBytes),
            prettyPrintBytes(entry.backgroundWrittenBytes)))
    if plotfilter(entry.uid):
        entry.plot()
        plt.show()

if args.totalpie:
    system.pie()
    plt.show()

