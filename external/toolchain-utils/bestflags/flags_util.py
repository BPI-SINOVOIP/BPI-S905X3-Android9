# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Utility functions to explore the neighbor flags.

Part of the Chrome build flags optimization.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import flags
from flags import Flag


def ClimbNext(flags_dict, climb_spec):
  """Get the flags that are different from |flags_dict| by |climb_spec|.

  Given a set of flags, |flags_dict|, return a new set of flags that are
  adjacent along the flag spec |climb_spec|.

  An example flags_dict is {foo=[1-9]:foo=5, bar=[1-5]:bar=2} and climb_spec is
  bar=[1-5]. This method changes the flag that contains the spec bar=[1-5]. The
  results are its neighbors dictionaries, i.e., {foo=[1-9]:foo=5,
  bar=[1-5]:bar=1} and {foo=[1-9]:foo=5, bar=[1-5]:bar=3}.

  Args:
    flags_dict: The dictionary containing the original flags whose neighbors are
      to be explored.
    climb_spec: The spec in the flags_dict is to be changed. The spec is a
      definition in the little language, a string with escaped sequences of the
      form [<start>-<end>] where start and end is an positive integer for a
      fillable value. An example of a spec is "foo[0-9]".

  Returns:
    List of dictionaries of neighbor flags.
  """

  # This method searches for a pattern [start-end] in the spec. If the spec
  # contains this pattern, it is a numeric flag. Otherwise it is a boolean flag.
  # For example, -finline-limit=[1-1000] is a numeric flag and -falign-jumps is
  # a boolean flag.
  numeric_flag_match = flags.Search(climb_spec)

  # If the flags do not contain the spec.
  if climb_spec not in flags_dict:
    results = flags_dict.copy()

    if numeric_flag_match:
      # Numeric flags.
      results[climb_spec] = Flag(climb_spec,
                                 int(numeric_flag_match.group('start')))
    else:
      # Boolean flags.
      results[climb_spec] = Flag(climb_spec)

    return [results]

  # The flags contain the spec.
  if not numeric_flag_match:
    # Boolean flags.
    results = flags_dict.copy()

    # Turn off the flag. A flag is turned off if it is not presented in the
    # flags_dict.
    del results[climb_spec]
    return [results]

  # Numeric flags.
  flag = flags_dict[climb_spec]

  # The value of the flag having spec.
  value = flag.GetValue()
  results = []

  if value + 1 < int(numeric_flag_match.group('end')):
    # If the value is not the end value, explore the value that is 1 larger than
    # the current value.
    neighbor = flags_dict.copy()
    neighbor[climb_spec] = Flag(climb_spec, value + 1)
    results.append(neighbor)

  if value > int(numeric_flag_match.group('start')):
    # If the value is not the start value, explore the value that is 1 lesser
    # than the current value.
    neighbor = flags_dict.copy()
    neighbor[climb_spec] = Flag(climb_spec, value - 1)
    results.append(neighbor)
  else:
    # Delete the value, i.e., turn off the flag. A flag is turned off if it is
    # not presented in the flags_dict.
    neighbor = flags_dict.copy()
    del neighbor[climb_spec]
    results.append(neighbor)

  return results
