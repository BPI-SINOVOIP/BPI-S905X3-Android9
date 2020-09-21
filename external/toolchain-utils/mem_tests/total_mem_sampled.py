#! /usr/bin/python
"""Parses the total amount of sampled memory from log files.

This file outputs the total amount of memory that has been sampled by tcmalloc.
The output is of the format:

time in seconds from a base time, amount of memory that has been sampled

"""

import argparse
from cros_utils import compute_total_diff
from datetime import datetime

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

my_file = open(args.filename)
output_file = open('memory_data.csv', 'a')

base_time = datetime(2014, 6, 11, 0, 0)
prev_line = ''
half_entry = (None, None)

for line in my_file:
  if 'heap profile: ' not in line:
    continue
  memory_used = line.strip().split(':')[-1].strip().split(']')[0].strip()
  total_diff = compute_total_diff(line, base_time)
  output_file.write('{0},{1}\n'.format(int(total_diff), memory_used))
