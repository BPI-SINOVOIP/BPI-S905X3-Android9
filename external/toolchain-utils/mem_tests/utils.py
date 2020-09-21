"""Utility functions for the memory tests.
"""

from datetime import datetime


def compute_total_diff(line, base_time):
  """
    Computes the difference in time the line was recorded from the base time.

    An example of a line is:
    [4688:4688:0701/010151:ERROR:perf_provider_chromeos.cc(228)]...

    Here, the month is 07, the day is 01 and the time is 01:01:51.

    line- the line that contains the time the record was taken
    base_time- the base time to measure our timestamp from
    """
  date = line.strip().split(':')[2].split('/')
  timestamp = datetime(2014, int(date[0][0:2]), int(date[0][2:4]),
                       int(date[1][0:2]), int(date[1][2:4]), int(date[1][4:6]))
  return (timestamp - base_time).total_seconds()
