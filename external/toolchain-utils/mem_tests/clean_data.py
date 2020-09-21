#! /usr/bin/python
"""Cleans output from other scripts to eliminate duplicates.

When frequently sampling data, we see that records occasionally will contain
the same timestamp (due to perf recording twice in the same second).

This removes all of the duplicate timestamps for every record. Order with
respect to timestamps is not preserved. Also, the assumption is that the log
file is a csv with the first value in each row being the time in seconds from a
standard time.

"""

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('filename')
args = parser.parse_args()

my_file = open(args.filename)
output_file = open('clean2.csv', 'a')
dictionary = dict()

for line in my_file:
  new_time = int(line.split(',')[0])
  dictionary[new_time] = line

for key in dictionary.keys():
  output_file.write(dictionary[key])
