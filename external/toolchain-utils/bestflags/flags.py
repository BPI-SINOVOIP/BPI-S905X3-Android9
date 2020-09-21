# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Manage bundles of flags used for the optimizing of ChromeOS.

Part of the Chrome build flags optimization.

The content of this module is adapted from the Trakhelp JVM project. This module
contains the basic Class Flag and the Class FlagSet. The core abstractions are:

The class Flag, which takes a domain specific language describing how to fill
the flags with values.

The class FlagSet, which contains a number of flags and can create new FlagSets
by mixing with other FlagSets.

The Flag DSL works by replacing value ranges in [x-y] with numbers in the range
x through y.

Examples:
  "foo[0-9]bar" will expand to e.g. "foo5bar".
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import random
import re

#
# This matches a [...] group in the internal representation for a flag
# specification, and is used in "filling out" flags - placing values inside
# the flag_spec.  The internal flag_spec format is like "foo[0]", with
# values filled out like 5; this would be transformed by
# FormattedForUse() into "foo5".
_FLAG_FILLOUT_VALUE_RE = re.compile(r'\[([^\]]*)\]')

# This matches a numeric flag flag=[start-end].
rx = re.compile(r'\[(?P<start>\d+)-(?P<end>\d+)\]')


# Search the numeric flag pattern.
def Search(spec):
  return rx.search(spec)


class NoSuchFileError(Exception):
  """Define an Exception class for user providing invalid input file."""
  pass


def ReadConf(file_name):
  """Parse the configuration file.

  The configuration contains one flag specification in each line.

  Args:
    file_name: The name of the configuration file.

  Returns:
    A list of specs in the configuration file.

  Raises:
    NoSuchFileError: The caller should provide a valid configuration file.
  """

  with open(file_name, 'r') as input_file:
    lines = input_file.readlines()

    return sorted([line.strip() for line in lines if line.strip()])

  raise NoSuchFileError()


class Flag(object):
  """A class representing a particular command line flag argument.

  The Flag consists of two parts: The spec and the value.
  The spec is a definition of the following form: a string with escaped
  sequences of the form [<start>-<end>] where start and end is an positive
  integer for a fillable value.

  An example of a spec is "foo[0-9]".
  There are two kinds of flags, boolean flag and numeric flags. Boolean flags
  can either be turned on or off, which numeric flags can have different
  positive integer values. For example, -finline-limit=[1-1000] is a numeric
  flag and -ftree-vectorize is a boolean flag.

  A (boolean/numeric) flag is not turned on if it is not selected in the
  FlagSet.
  """

  def __init__(self, spec, value=-1):
    self._spec = spec

    # If the value is not specified, generate a random value to use.
    if value == -1:
      # If creating a boolean flag, the value will be 0.
      value = 0

      # Parse the spec's expression for the flag value's numeric range.
      numeric_flag_match = Search(spec)

      # If this is a numeric flag, a value is chosen within start and end, start
      # inclusive and end exclusive.
      if numeric_flag_match:
        start = int(numeric_flag_match.group('start'))
        end = int(numeric_flag_match.group('end'))

        assert start < end
        value = random.randint(start, end)

    self._value = value

  def __eq__(self, other):
    if isinstance(other, Flag):
      return self._spec == other.GetSpec() and self._value == other.GetValue()
    return False

  def __hash__(self):
    return hash(self._spec) + self._value

  def GetValue(self):
    """Get the value for this flag.

    Returns:
     The value.
    """

    return self._value

  def GetSpec(self):
    """Get the spec for this flag.

    Returns:
     The spec.
    """

    return self._spec

  def FormattedForUse(self):
    """Calculate the combination of flag_spec and values.

    For e.g. the flag_spec 'foo[0-9]' and the value equals to 5, this will
    return 'foo5'. The filled out version of the flag is the text string you use
    when you actually want to pass the flag to some binary.

    Returns:
      A string that represent the filled out flag, e.g. the flag with the
      FlagSpec '-X[0-9]Y' and value equals to 5 would return '-X5Y'.
    """

    return _FLAG_FILLOUT_VALUE_RE.sub(str(self._value), self._spec)


class FlagSet(object):
  """A dictionary of Flag objects.

  The flags dictionary stores the spec and flag pair.
  """

  def __init__(self, flag_array):
    # Store the flags as a dictionary mapping of spec -> flag object
    self._flags = dict([(flag.GetSpec(), flag) for flag in flag_array])

  def __eq__(self, other):
    return isinstance(other, FlagSet) and self._flags == other.GetFlags()

  def __hash__(self):
    return sum([hash(flag) for flag in self._flags.values()])

  def __getitem__(self, flag_spec):
    """Get flag with a particular flag_spec.

    Args:
      flag_spec: The flag_spec to find.

    Returns:
      A flag.
    """

    return self._flags[flag_spec]

  def __contains__(self, flag_spec):
    return self._flags.has_key(flag_spec)

  def GetFlags(self):
    return self._flags

  def FormattedForUse(self):
    """Format this for use in an application.

    Returns:
      A list of flags, sorted alphabetically and filled in with the values
      for each flag.
    """

    return sorted([f.FormattedForUse() for f in self._flags.values()])
