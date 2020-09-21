#! /usr/bin/python
"""Groups memory by allocation sizes.

Takes a log entry and sorts sorts everything into groups based on what size
chunks the memory has been allocated in. groups is an array that contains the
divisions (in bytes).

The output format is:

timestamp, percent of memory in chunks < groups[0], percent between groups[0]
and groups[1], etc.

"""

import argparse
from cros_utils import compute_total_diff
from datetime import datetime

pretty_print = True

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

my_file = open(args.filename)
output_file = open('groups.csv', 'a')

# The cutoffs for each group in the output (in bytes)
groups = [1024, 8192, 65536, 524288, 4194304]

base_time = datetime(2014, 6, 11, 0, 0)
prev_line = ''
half_entry = (None, None)

for line in my_file:
  if 'heap profile:' in line:
    if half_entry[0] is not None:
      group_totals = half_entry[1]
      total = sum(group_totals) * 1.0
      to_join = [half_entry[0]] + [value / total for value in group_totals]
      to_output = ','.join([str(elem) for elem in to_join])
      output_file.write(to_output)
    total_diff = compute_total_diff(line, base_time)
    half_entry = (total_diff, [0] * (len(groups) + 1))
  if '] @ ' in line and 'heap profile:' not in line:
    mem_samples = line.strip().split('[')[0]
    num_samples, total_mem = map(int, mem_samples.strip().split(':'))
    mem_per_sample = total_mem // num_samples
    group_totals = half_entry[1]
    for cutoff_index in range(len(groups)):
      if mem_per_sample <= groups[cutoff_index]:
        group_totals[cutoff_index] += total_mem
        break
    if mem_per_sample > groups[-1]:
      group_totals[-1] += total_mem
