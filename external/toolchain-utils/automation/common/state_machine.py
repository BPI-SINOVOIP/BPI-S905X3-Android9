# -*- coding: utf-8 -*-
#
# Copyright 2011 Google Inc. All Rights Reserved.
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

from automation.common import events


class BasicStateMachine(object):
  """Generic class for constructing state machines.
  
  Keeps all states and possible transition of a state machine. Ensures that
  transition between two states is always valid. Also stores transition events
  in a timeline object.
  """
  state_machine = {}
  final_states = []

  def __init__(self, initial_state):
    assert initial_state in self.state_machine,\
           'Initial state does not belong to this state machine'

    self._state = initial_state

    self.timeline = events.EventHistory()
    self.timeline.AddEvent(self._state)

  def __str__(self):
    return self._state

  def __eq__(self, value):
    if isinstance(value, BasicStateMachine):
      value = str(value)

    return self._state == value

  def __ne__(self, value):
    return not self == value

  def _TransitionAllowed(self, to_state):
    return to_state in self.state_machine.get(self._state, [])

  def Change(self, new_state):
    assert self._TransitionAllowed(new_state),\
           'Transition from %s to %s not possible' % (self._state, new_state)

    self._state = new_state

    self.timeline.AddEvent(self._state)

    if self._state in self.final_states:
      self.timeline.last.Finish()
