# Copyright 2012 Google Inc. All Rights Reserved.
#
"""Tools for recording and reporting timeline of benchmark_run."""

from __future__ import print_function

__author__ = 'yunlian@google.com (Yunlian Jiang)'

import time


class Event(object):
  """One event on the timeline."""

  def __init__(self, name='', cur_time=0):
    self.name = name
    self.timestamp = cur_time


class Timeline(object):
  """Use a dict to store the timeline."""

  def __init__(self):
    self.events = []

  def Record(self, event):
    for e in self.events:
      assert e.name != event, ('The event {0} is already recorded.'
                               .format(event))
    cur_event = Event(name=event, cur_time=time.time())
    self.events.append(cur_event)

  def GetEvents(self):
    return ([e.name for e in self.events])

  def GetEventDict(self):
    tl = {}
    for e in self.events:
      tl[e.name] = e.timestamp
    return tl

  def GetEventTime(self, event):
    for e in self.events:
      if e.name == event:
        return e.timestamp
    raise IndexError, 'The event {0} is not recorded'.format(event)

  def GetLastEventTime(self):
    return self.events[-1].timestamp

  def GetLastEvent(self):
    return self.events[-1].name
