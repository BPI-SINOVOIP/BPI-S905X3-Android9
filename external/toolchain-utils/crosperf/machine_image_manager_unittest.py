#!/usr/bin/env python2

# Copyright 2015 Google Inc. All Rights Reserved.
"""Unit tests for the MachineImageManager class."""

from __future__ import print_function

import random
import unittest

from machine_image_manager import MachineImageManager


class MockLabel(object):
  """Class for generating a mock Label."""

  def __init__(self, name, remotes=None):
    self.name = name
    self.remote = remotes

  def __hash__(self):
    """Provide hash function for label.

       This is required because Label object is used inside a dict as key.
    """
    return hash(self.name)

  def __eq__(self, other):
    """Provide eq function for label.

       This is required because Label object is used inside a dict as key.
    """
    return isinstance(other, MockLabel) and other.name == self.name


class MockDut(object):
  """Class for creating a mock Device-Under-Test (DUT)."""

  def __init__(self, name, label=None):
    self.name = name
    self.label_ = label


class MachineImageManagerTester(unittest.TestCase):
  """Class for testing MachineImageManager."""

  def gen_duts_by_name(self, *names):
    duts = []
    for n in names:
      duts.append(MockDut(n))
    return duts

  def print_matrix(self, matrix):
    # pylint: disable=expression-not-assigned
    for r in matrix:
      for v in r:
        print('{} '.format('.' if v == ' ' else v)),
      print('')

  def create_labels_and_duts_from_pattern(self, pattern):
    labels = []
    duts = []
    for i, r in enumerate(pattern):
      l = MockLabel('l{}'.format(i), [])
      for j, v in enumerate(r.split()):
        if v == '.':
          l.remote.append('m{}'.format(j))
        if i == 0:
          duts.append(MockDut('m{}'.format(j)))
      labels.append(l)
    return labels, duts

  def check_matrix_against_pattern(self, matrix, pattern):
    for i, s in enumerate(pattern):
      for j, v in enumerate(s.split()):
        self.assertTrue(v == '.' and matrix[i][j] == ' ' or v == matrix[i][j])

  def pattern_based_test(self, inp, output):
    labels, duts = self.create_labels_and_duts_from_pattern(inp)
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.compute_initial_allocation())
    self.check_matrix_against_pattern(mim.matrix_, output)
    return mim

  def test_single_dut(self):
    labels = [MockLabel('l1'), MockLabel('l2'), MockLabel('l3')]
    dut = MockDut('m1')
    mim = MachineImageManager(labels, [dut])
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [['Y'], ['Y'], ['Y']])

  def test_single_label(self):
    labels = [MockLabel('l1')]
    duts = self.gen_duts_by_name('m1', 'm2', 'm3')
    mim = MachineImageManager(labels, duts)
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [['Y', 'Y', 'Y']])

  def test_case1(self):
    labels = [
        MockLabel('l1', ['m1', 'm2']), MockLabel('l2', ['m2', 'm3']), MockLabel(
            'l3', ['m1'])
    ]
    duts = [MockDut('m1'), MockDut('m2'), MockDut('m3')]
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.matrix_ == [[' ', ' ', 'X'], ['X', ' ', ' '],
                                    [' ', 'X', 'X']])
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [[' ', 'Y', 'X'], ['X', ' ', 'Y'],
                                    ['Y', 'X', 'X']])

  def test_case2(self):
    labels = [
        MockLabel('l1', ['m1', 'm2']), MockLabel('l2', ['m2', 'm3']), MockLabel(
            'l3', ['m1'])
    ]
    duts = [MockDut('m1'), MockDut('m2'), MockDut('m3')]
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.matrix_ == [[' ', ' ', 'X'], ['X', ' ', ' '],
                                    [' ', 'X', 'X']])
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [[' ', 'Y', 'X'], ['X', ' ', 'Y'],
                                    ['Y', 'X', 'X']])

  def test_case3(self):
    labels = [
        MockLabel('l1', ['m1', 'm2']), MockLabel('l2', ['m2', 'm3']), MockLabel(
            'l3', ['m1'])
    ]
    duts = [MockDut('m1', labels[0]), MockDut('m2'), MockDut('m3')]
    mim = MachineImageManager(labels, duts)
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [[' ', 'Y', 'X'], ['X', ' ', 'Y'],
                                    ['Y', 'X', 'X']])

  def test_case4(self):
    labels = [
        MockLabel('l1', ['m1', 'm2']), MockLabel('l2', ['m2', 'm3']), MockLabel(
            'l3', ['m1'])
    ]
    duts = [MockDut('m1'), MockDut('m2', labels[0]), MockDut('m3')]
    mim = MachineImageManager(labels, duts)
    mim.compute_initial_allocation()
    self.assertTrue(mim.matrix_ == [[' ', 'Y', 'X'], ['X', ' ', 'Y'],
                                    ['Y', 'X', 'X']])

  def test_case5(self):
    labels = [
        MockLabel('l1', ['m3']), MockLabel('l2', ['m3']), MockLabel(
            'l3', ['m1'])
    ]
    duts = self.gen_duts_by_name('m1', 'm2', 'm3')
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.compute_initial_allocation())
    self.assertTrue(mim.matrix_ == [['X', 'X', 'Y'], ['X', 'X', 'Y'],
                                    ['Y', 'X', 'X']])

  def test_2x2_with_allocation(self):
    labels = [MockLabel('l0'), MockLabel('l1')]
    duts = [MockDut('m0'), MockDut('m1')]
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.compute_initial_allocation())
    self.assertTrue(mim.allocate(duts[0]) == labels[0])
    self.assertTrue(mim.allocate(duts[0]) == labels[1])
    self.assertTrue(mim.allocate(duts[0]) is None)
    self.assertTrue(mim.matrix_[0][0] == '_')
    self.assertTrue(mim.matrix_[1][0] == '_')
    self.assertTrue(mim.allocate(duts[1]) == labels[1])

  def test_10x10_general(self):
    """Gen 10x10 matrix."""
    n = 10
    labels = []
    duts = []
    for i in range(n):
      labels.append(MockLabel('l{}'.format(i)))
      duts.append(MockDut('m{}'.format(i)))
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.compute_initial_allocation())
    for i in range(n):
      for j in range(n):
        if i == j:
          self.assertTrue(mim.matrix_[i][j] == 'Y')
        else:
          self.assertTrue(mim.matrix_[i][j] == ' ')
    self.assertTrue(mim.allocate(duts[3]).name == 'l3')

  def test_random_generated(self):
    n = 10
    labels = []
    duts = []
    for i in range(10):
      # generate 3-5 machines that is compatible with this label
      l = MockLabel('l{}'.format(i), [])
      r = random.random()
      for _ in range(4):
        t = int(r * 10) % n
        r *= 10
        l.remote.append('m{}'.format(t))
      labels.append(l)
      duts.append(MockDut('m{}'.format(i)))
    mim = MachineImageManager(labels, duts)
    self.assertTrue(mim.compute_initial_allocation())

  def test_10x10_fully_random(self):
    inp = [
        'X  .  .  .  X  X  .  X  X  .', 'X  X  .  X  .  X  .  X  X  .',
        'X  X  X  .  .  X  .  X  .  X', 'X  .  X  X  .  .  X  X  .  X',
        'X  X  X  X  .  .  .  X  .  .', 'X  X  .  X  .  X  .  .  X  .',
        '.  X  .  X  .  X  X  X  .  .', '.  X  .  X  X  .  X  X  .  .',
        'X  X  .  .  .  X  X  X  .  .', '.  X  X  X  X  .  .  .  .  X'
    ]
    output = [
        'X  Y  .  .  X  X  .  X  X  .', 'X  X  Y  X  .  X  .  X  X  .',
        'X  X  X  Y  .  X  .  X  .  X', 'X  .  X  X  Y  .  X  X  .  X',
        'X  X  X  X  .  Y  .  X  .  .', 'X  X  .  X  .  X  Y  .  X  .',
        'Y  X  .  X  .  X  X  X  .  .', '.  X  .  X  X  .  X  X  Y  .',
        'X  X  .  .  .  X  X  X  .  Y', '.  X  X  X  X  .  .  Y  .  X'
    ]
    self.pattern_based_test(inp, output)

  def test_10x10_fully_random2(self):
    inp = [
        'X  .  X  .  .  X  .  X  X  X', 'X  X  X  X  X  X  .  .  X  .',
        'X  .  X  X  X  X  X  .  .  X', 'X  X  X  .  X  .  X  X  .  .',
        '.  X  .  X  .  X  X  X  X  X', 'X  X  X  X  X  X  X  .  .  X',
        'X  .  X  X  X  X  X  .  .  X', 'X  X  X  .  X  X  X  X  .  .',
        'X  X  X  .  .  .  X  X  X  X', '.  X  X  .  X  X  X  .  X  X'
    ]
    output = [
        'X  .  X  Y  .  X  .  X  X  X', 'X  X  X  X  X  X  Y  .  X  .',
        'X  Y  X  X  X  X  X  .  .  X', 'X  X  X  .  X  Y  X  X  .  .',
        '.  X  Y  X  .  X  X  X  X  X', 'X  X  X  X  X  X  X  Y  .  X',
        'X  .  X  X  X  X  X  .  Y  X', 'X  X  X  .  X  X  X  X  .  Y',
        'X  X  X  .  Y  .  X  X  X  X', 'Y  X  X  .  X  X  X  .  X  X'
    ]
    self.pattern_based_test(inp, output)

  def test_3x4_with_allocation(self):
    inp = ['X  X  .  .', '.  .  X  .', 'X  .  X  .']
    output = ['X  X  Y  .', 'Y  .  X  .', 'X  Y  X  .']
    mim = self.pattern_based_test(inp, output)
    self.assertTrue(mim.allocate(mim.duts_[2]) == mim.labels_[0])
    self.assertTrue(mim.allocate(mim.duts_[3]) == mim.labels_[2])
    self.assertTrue(mim.allocate(mim.duts_[0]) == mim.labels_[1])
    self.assertTrue(mim.allocate(mim.duts_[1]) == mim.labels_[2])
    self.assertTrue(mim.allocate(mim.duts_[3]) == mim.labels_[1])
    self.assertTrue(mim.allocate(mim.duts_[3]) == mim.labels_[0])
    self.assertTrue(mim.allocate(mim.duts_[3]) is None)
    self.assertTrue(mim.allocate(mim.duts_[2]) is None)
    self.assertTrue(mim.allocate(mim.duts_[1]) == mim.labels_[1])
    self.assertTrue(mim.allocate(mim.duts_[1]) == None)
    self.assertTrue(mim.allocate(mim.duts_[0]) == None)
    self.assertTrue(mim.label_duts_[0] == [2, 3])
    self.assertTrue(mim.label_duts_[1] == [0, 3, 1])
    self.assertTrue(mim.label_duts_[2] == [3, 1])
    self.assertTrue(mim.allocate_log_ == [(0, 2), (2, 3), (1, 0), (2, 1),
                                          (1, 3), (0, 3), (1, 1)])

  def test_cornercase_1(self):
    """This corner case is brought up by Caroline.

        The description is -

        If you have multiple labels and multiple machines, (so we don't
        automatically fall into the 1 dut or 1 label case), but all of the
        labels specify the same 1 remote, then instead of assigning the same
        machine to all the labels, your algorithm fails to assign any...

        So first step is to create an initial matrix like below, l0, l1 and l2
        all specify the same 1 remote - m0.

             m0    m1    m2
        l0   .     X     X

        l1   .     X     X

        l2   .     X     X

        The search process will be like this -
        a) try to find a solution with at most 1 'Y's per column (but ensure at
        least 1 Y per row), fail
        b) try to find a solution with at most 2 'Y's per column (but ensure at
        least 1 Y per row), fail
        c) try to find a solution with at most 3 'Y's per column (but ensure at
        least 1 Y per row), succeed, so we end up having this solution

            m0    m1    m2
        l0   Y     X     X

        l1   Y     X     X

        l2   Y     X     X
    """

    inp = ['.  X  X', '.  X  X', '.  X  X']
    output = ['Y  X  X', 'Y  X  X', 'Y  X  X']
    mim = self.pattern_based_test(inp, output)
    self.assertTrue(mim.allocate(mim.duts_[1]) is None)
    self.assertTrue(mim.allocate(mim.duts_[2]) is None)
    self.assertTrue(mim.allocate(mim.duts_[0]) == mim.labels_[0])
    self.assertTrue(mim.allocate(mim.duts_[0]) == mim.labels_[1])
    self.assertTrue(mim.allocate(mim.duts_[0]) == mim.labels_[2])
    self.assertTrue(mim.allocate(mim.duts_[0]) is None)


if __name__ == '__main__':
  unittest.main()
