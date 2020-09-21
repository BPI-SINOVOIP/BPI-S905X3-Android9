# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""MachineImageManager allocates images to duts."""


class MachineImageManager(object):
  """Management of allocating images to duts.

    * Data structure we have -

      duts_ - list of duts, for each duts, we assume the following 2 properties
      exist - label_ (the current label the duts_ carries or None, if it has an
      alien image) and name (a string)

      labels_ - a list of labels, for each label, we assume these properties
      exist - remote (a set/vector/list of dut names (not dut object), to each
      of which this image is compatible), remote could be none, which means
      universal compatible.

      label_duts_ - for each label, we maintain a list of duts, onto which the
      label is imaged. Note this is an array of lists. Each element of each list
      is an integer which is dut oridnal. We access this array using label
      ordinal.

      allocate_log_ - a list of allocation record. For example, if we allocate
      l1 to d1, then l2 to d2, then allocate_log_ would be [(1, 1), (2, 2)].
      This is used for debug/log, etc. All tuples in the list are integer pairs
      (label_ordinal, dut_ordinal).

      n_duts_ - number of duts.

      n_labels_ - number of labels.

      dut_name_ordinal_ - mapping from dut name (a string) to an integer,
      starting from 0. So that duts_[dut_name_ordinal_[a_dut.name]]= a_dut.

    * Problem abstraction -

      Assume we have the following matrix - label X machine (row X col). A 'X'
      in (i, j) in the matrix means machine and lable is not compatible, or that
      we cannot image li to Mj.

           M1   M2   M3
      L1    X

      L2             X

      L3    X    X

      Now that we'll try to find a way to fill Ys in the matrix so that -

        a) - each row at least get a Y, this ensures that each label get imaged
        at least once, an apparent prerequiste.

        b) - each column get at most N Ys. This make sure we can successfully
        finish all tests by re-image each machine at most N times. That being
        said, we could *OPTIONALLY* reimage some machines more than N times to
        *accelerate* the test speed.

      How to choose initial N for b) -
      If number of duts (nd) is equal to or more than that of labels (nl), we
      start from N == 1. Else we start from N = nl - nd + 1.

      We will begin the search with pre-defined N, if we fail to find such a
      solution for such N, we increase N by 1 and continue the search till we
      get N == nl, at this case we fails.

      Such a solution ensures minimal number of reimages.

    * Solution representation

      The solution will be placed inside the matrix, like below

           M1   M2   M3   M4
      L1    X    X    Y

      L2    Y         X

      L3    X    Y    X

    * Allocation algorithm

      When Mj asks for a image, we check column j, pick the first cell that
      contains a 'Y', and mark the cell '_'. If no such 'Y' exists (like M4 in
      the above solution matrix), we just pick an image that the minimal reimage
      number.

      After allocate for M3
           M1   M2   M3   M4
      L1    X    X    _

      L2    Y         X

      L3    X    Y    X

      After allocate for M4
           M1   M2   M3   M4
      L1    X    X    _

      L2    Y         X    _

      L3    X    Y    X

      After allocate for M2
           M1   M2   M3   M4
      L1    X    X    _

      L2    Y         X    _

      L3    X    _    X

      After allocate for M1
           M1   M2   M3   M4
      L1    X    X    _

      L2    _         X    _

      L3    X    _    X

      After allocate for M2
           M1   M2   M3   M4
      L1    X    X    _

      L2    _    _    X    _

      L3    X    _    X

      If we try to allocate for M1 or M2 or M3 again, we get None.

    * Special / common case to handle seperately

      We have only 1 dut or if we have only 1 label, that's simple enough.
  """

  def __init__(self, labels, duts):
    self.labels_ = labels
    self.duts_ = duts
    self.n_labels_ = len(labels)
    self.n_duts_ = len(duts)
    self.dut_name_ordinal_ = dict()
    for idx, dut in enumerate(self.duts_):
      self.dut_name_ordinal_[dut.name] = idx

    # Generate initial matrix containg 'X' or ' '.
    self.matrix_ = [['X' if (l.remote and len(l.remote)) else ' ' \
                     for _ in range(self.n_duts_)] for l in self.labels_]
    for ol, l in enumerate(self.labels_):
      if l.remote:
        for r in l.remote:
          self.matrix_[ol][self.dut_name_ordinal_[r]] = ' '

    self.label_duts_ = [[] for _ in range(self.n_labels_)]
    self.allocate_log_ = []

  def compute_initial_allocation(self):
    """Compute the initial label-dut allocation.

    This method finds the most efficient way that every label gets imaged at
    least once.

    Returns:
      False, only if not all labels could be imaged to a certain machine,
      otherwise True.
    """

    if self.n_duts_ == 1:
      for i, v in self.matrix_vertical_generator(0):
        if v != 'X':
          self.matrix_[i][0] = 'Y'
      return

    if self.n_labels_ == 1:
      for j, v in self.matrix_horizontal_generator(0):
        if v != 'X':
          self.matrix_[0][j] = 'Y'
      return

    if self.n_duts_ >= self.n_labels_:
      n = 1
    else:
      n = self.n_labels_ - self.n_duts_ + 1
    while n <= self.n_labels_:
      if self._compute_initial_allocation_internal(0, n):
        break
      n += 1

    return n <= self.n_labels_

  def _record_allocate_log(self, label_i, dut_j):
    self.allocate_log_.append((label_i, dut_j))
    self.label_duts_[label_i].append(dut_j)

  def allocate(self, dut, schedv2=None):
    """Allocate a label for dut.

    Args:
      dut: the dut that asks for a new image.
      schedv2: the scheduling instance, we need the benchmark run
               information with schedv2 for a better allocation.

    Returns:
      a label to image onto the dut or None if no more available images for
      the dut.
    """
    j = self.dut_name_ordinal_[dut.name]
    # 'can_' prefix means candidate label's.
    can_reimage_number = 999
    can_i = 999
    can_label = None
    can_pending_br_num = 0
    for i, v in self.matrix_vertical_generator(j):
      label = self.labels_[i]

      # 2 optimizations here regarding allocating label to dut.
      # Note schedv2 might be None in case we do not need this
      # optimization or we are in testing mode.
      if schedv2 is not None:
        pending_br_num = len(schedv2.get_label_map()[label])
        if pending_br_num == 0:
          # (A) - we have finished all br of this label,
          # apparently, we do not want to reimaeg dut to
          # this label.
          continue
      else:
        # In case we do not have a schedv2 instance, mark
        # pending_br_num as 0, so pending_br_num >=
        # can_pending_br_num is always True.
        pending_br_num = 0

      # For this time being, I just comment this out until we have a
      # better estimation how long each benchmarkrun takes.
      # if (pending_br_num <= 5 and
      #     len(self.label_duts_[i]) >= 1):
      #     # (B) this is heuristic - if there are just a few test cases
      #     # (say <5) left undone for this label, and there is at least
      #     # 1 other machine working on this lable, we probably not want
      #     # to bother to reimage this dut to help with these 5 test
      #     # cases
      #     continue

      if v == 'Y':
        self.matrix_[i][j] = '_'
        self._record_allocate_log(i, j)
        return label
      if v == ' ':
        label_reimage_number = len(self.label_duts_[i])
        if ((can_label is None) or
            (label_reimage_number < can_reimage_number or
             (label_reimage_number == can_reimage_number and
              pending_br_num >= can_pending_br_num))):
          can_reimage_number = label_reimage_number
          can_i = i
          can_label = label
          can_pending_br_num = pending_br_num

    # All labels are marked either '_' (already taken) or 'X' (not
    # compatible), so return None to notify machine thread to quit.
    if can_label is None:
      return None

    # At this point, we don't find any 'Y' for the machine, so we go the
    # 'min' approach.
    self.matrix_[can_i][j] = '_'
    self._record_allocate_log(can_i, j)
    return can_label

  def matrix_vertical_generator(self, col):
    """Iterate matrix vertically at column 'col'.

    Yield row number i and value at matrix_[i][col].
    """
    for i, _ in enumerate(self.labels_):
      yield i, self.matrix_[i][col]

  def matrix_horizontal_generator(self, row):
    """Iterate matrix horizontally at row 'row'.

    Yield col number j and value at matrix_[row][j].
    """
    for j, _ in enumerate(self.duts_):
      yield j, self.matrix_[row][j]

  def _compute_initial_allocation_internal(self, level, N):
    """Search matrix for d with N."""

    if level == self.n_labels_:
      return True

    for j, v in self.matrix_horizontal_generator(level):
      if v == ' ':
        # Before we put a 'Y', we check how many Y column 'j' has.
        # Note y[0] is row idx, y[1] is the cell value.
        ny = reduce(lambda x, y: x + 1 if (y[1] == 'Y') else x,
                    self.matrix_vertical_generator(j), 0)
        if ny < N:
          self.matrix_[level][j] = 'Y'
          if self._compute_initial_allocation_internal(level + 1, N):
            return True
          self.matrix_[level][j] = ' '

    return False
