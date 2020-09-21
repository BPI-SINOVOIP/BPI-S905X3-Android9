# -*- coding: utf-8 -*-
#
# Copyright 2011 Google Inc. All Rights Reserved.
#
"""Tools for recording and reporting timeline of abstract events.

You can store any events provided that they can be stringified.
"""

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import collections
import datetime
import time


class _EventRecord(object):
  """Internal class.  Attaches extra information to an event."""

  def __init__(self, event, time_started=None, time_elapsed=None):
    self._event = event
    self._time_started = time_started or time.time()
    self._time_elapsed = None

    if time_elapsed:
      self.time_elapsed = time_elapsed

  @property
  def event(self):
    return self._event

  @property
  def time_started(self):
    return self._time_started

  def _TimeElapsedGet(self):
    if self.has_finished:
      time_elapsed = self._time_elapsed
    else:
      time_elapsed = time.time() - self._time_started

    return datetime.timedelta(seconds=time_elapsed)

  def _TimeElapsedSet(self, time_elapsed):
    if isinstance(time_elapsed, datetime.timedelta):
      self._time_elapsed = time_elapsed.seconds
    else:
      self._time_elapsed = time_elapsed

  time_elapsed = property(_TimeElapsedGet, _TimeElapsedSet)

  @property
  def has_finished(self):
    return self._time_elapsed is not None

  def GetTimeStartedFormatted(self):
    return time.strftime('%m/%d/%Y %H:%M:%S', time.gmtime(self._time_started))

  def GetTimeElapsedRounded(self):
    return datetime.timedelta(seconds=int(self.time_elapsed.seconds))

  def Finish(self):
    if not self.has_finished:
      self._time_elapsed = time.time() - self._time_started


class _Transition(collections.namedtuple('_Transition', ('from_', 'to_'))):
  """Internal class.  Represents transition point between events / states."""

  def __str__(self):
    return '%s => %s' % (self.from_, self.to_)


class EventHistory(collections.Sequence):
  """Records events and provides human readable events timeline."""

  def __init__(self, records=None):
    self._records = records or []

  def __len__(self):
    return len(self._records)

  def __iter__(self):
    return iter(self._records)

  def __getitem__(self, index):
    return self._records[index]

  @property
  def last(self):
    if self._records:
      return self._records[-1]

  def AddEvent(self, event):
    if self.last:
      self.last.Finish()

    evrec = _EventRecord(event)
    self._records.append(evrec)
    return evrec

  def GetTotalTime(self):
    if self._records:
      total_time_elapsed = sum(evrec.time_elapsed.seconds
                               for evrec in self._records)

      return datetime.timedelta(seconds=int(total_time_elapsed))

  def GetTransitionEventHistory(self):
    records = []

    if self._records:
      for num, next_evrec in enumerate(self._records[1:], start=1):
        evrec = self._records[num - 1]

        records.append(_EventRecord(
            _Transition(evrec.event, next_evrec.event), evrec.time_started,
            evrec.time_elapsed))

      if not self.last.has_finished:
        records.append(_EventRecord(
            _Transition(self.last.event,
                        'NOW'), self.last.time_started, self.last.time_elapsed))

    return EventHistory(records)

  @staticmethod
  def _GetReport(history, report_name):
    report = [report_name]

    for num, evrec in enumerate(history, start=1):
      time_elapsed = str(evrec.GetTimeElapsedRounded())

      if not evrec.has_finished:
        time_elapsed.append(' (not finished)')

      report.append('%d) %s: %s: %s' % (num, evrec.GetTimeStartedFormatted(),
                                        evrec.event, time_elapsed))

    report.append('Total Time: %s' % history.GetTotalTime())

    return '\n'.join(report)

  def GetEventReport(self):
    return EventHistory._GetReport(self, 'Timeline of events:')

  def GetTransitionEventReport(self):
    return EventHistory._GetReport(self.GetTransitionEventHistory(),
                                   'Timeline of transition events:')
