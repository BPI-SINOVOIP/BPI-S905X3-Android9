#! /usr/bin/python
"""Parses the actual memory usage from TCMalloc.

This goes through logs that have the actual allocated memory (not sampled) in
the logs. The output is of the form of:

time (in seconds from some base time), amount of memory allocated by the
application

"""

import argparse
from cros_utils import compute_total_diff
from datetime import datetime

pretty_print = True

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

my_file = open(args.filename)
output_file = open('raw_memory_data.csv', 'a')

base_time = datetime(2014, 6, 11, 0, 0)
prev_line = ''
half_entry = (None, None)

for line in my_file:
  if 'Output Heap Stats:' in line:
    total_diff = compute_total_diff(line, base_time)
    half_entry = (total_diff, None)
  if 'Bytes in use by application' in line:
    total_diff = half_entry[0]
    memory_used = int(line.strip().split()[1])
    half_entry = (None, None)
    output_file.write('{0},{1}\n'.format(total_diff, memory_used))
