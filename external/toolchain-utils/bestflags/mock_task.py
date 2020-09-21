# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""This module defines the common mock tasks used by various unit tests.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

# Pick an integer at random.
POISONPILL = 975


class MockTask(object):
  """This class emulates an actual task.

  It does not do the actual work, but simply returns the result as given when
  this task is constructed.
  """

  def __init__(self, stage, identifier, cost=0):
    """Set up the results for this task.

    Args:
      stage: the stage of this test is in.
      identifier: the identifier of this task.
      cost: the mock cost of this task.

      The _cost field stored the cost. Once this task is performed, i.e., by
      calling the work method or by setting the result from other task, the
      _cost field will have this cost. The stage field verifies that the module
      being tested and the unitest are in the same stage. If the unitest does
      not care about cost of this task, the cost parameter should be leaved
      blank.
    """

    self._identifier = identifier
    self._cost = cost
    self._stage = stage

    # Indicate that this method has not been performed yet.
    self._performed = False

  def __eq__(self, other):
    if isinstance(other, MockTask):
      return (self._identifier == other.GetIdentifier(self._stage) and
              self._cost == other.GetResult(self._stage))
    return False

  def GetIdentifier(self, stage):
    assert stage == self._stage
    return self._identifier

  def SetResult(self, stage, cost):
    assert stage == self._stage
    self._cost = cost
    self._performed = True

  def Work(self, stage):
    assert stage == self._stage
    self._performed = True

  def GetResult(self, stage):
    assert stage == self._stage
    return self._cost

  def Done(self, stage):
    """Indicates whether the task has been performed."""

    assert stage == self._stage
    return self._performed

  def LogSteeringCost(self):
    pass


class IdentifierMockTask(MockTask):
  """This class defines the mock task that does not consider the cost.

  The task instances will be inserted into a set. Therefore the hash and the
  equal methods are overridden. The unittests that compares identities of the
  tasks for equality can use this mock task instead of the base mock tack.
  """

  def __hash__(self):
    return self._identifier

  def __eq__(self, other):
    if isinstance(other, MockTask):
      return self._identifier == other.GetIdentifier(self._stage)
    return False
