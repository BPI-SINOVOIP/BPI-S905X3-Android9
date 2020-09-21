# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""Helper functions to parse result collected from device"""

from __future__ import print_function
from fix_skia_results import _TransformBenchmarks

import json

def normalize(bench, dict_list):
  bench_base = {
      'Panorama': 1,
      'Dex2oat': 1,
      'Hwui': 10000,
      'Skia': 1,
      'Synthmark': 1,
      'Binder': 0.001
  }
  result_dict = dict_list[0]
  for key in result_dict:
    result_dict[key] = result_dict[key] / bench_base[bench]
  return [result_dict]


# Functions to parse benchmark result for data collection.
def parse_Panorama(bench, fin):
  result_dict = {}
  for line in fin:
    words = line.split()
    if 'elapsed' in words:
      #TODO: Need to restructure the embedded word counts.
      result_dict['total_time_s'] = float(words[3])
      result_dict['retval'] = 0
      return normalize(bench, [result_dict])
  raise ValueError('You passed the right type of thing, '
                   'but it didn\'t have the expected contents.')


def parse_Synthmark(bench, fin):
  result_dict = {}
  accum = 0
  cnt = 0
  for line in fin:
    words = line.split()
    if 'normalized' in words:
      #TODO: Need to restructure the embedded word counts.
      accum += float(words[-1])
      cnt += 1
  if accum != 0:
    result_dict['total_voices'] = accum / cnt
    result_dict['retval'] = 0
    return normalize(bench, [result_dict])
  raise ValueError('You passed the right type of thing, '
                   'but it didn\'t have the expected contents.')


def parse_Binder(bench, fin):
  result_dict = {}
  accum = 0
  cnt = 0
  for line in fin:
    words = line.split()
    for word in words:
      if 'average' in word:
        #TODO: Need to restructure the embedded word counts.
        accum += float(word[8:-2])
        cnt += 1
  if accum != 0:
    result_dict['avg_time_ms'] = accum / cnt
    result_dict['retval'] = 0
    return normalize(bench, [result_dict])
  raise ValueError('You passed the right type of thing, '
                   'but it didn\'t have the expected contents.')


def parse_Dex2oat(bench, fin):
  result_dict = {}
  cnt = 0
  for line in fin:
    words = line.split()
    if 'elapsed' in words:
      cnt += 1
      #TODO: Need to restructure the embedded word counts.
      if cnt == 1:
        # First 'elapsed' time is for microbench 'Chrome'
        result_dict['chrome_s'] = float(words[3])
      elif cnt == 2:
        # Second 'elapsed' time is for microbench 'Camera'
        result_dict['camera_s'] = float(words[3])

        result_dict['retval'] = 0
        # Two results found, return
        return normalize(bench, [result_dict])
  raise ValueError('You passed the right type of thing, '
                   'but it didn\'t have the expected contents.')


def parse_Hwui(bench, fin):
  result_dict = {}
  for line in fin:
    words = line.split()
    if 'elapsed' in words:
      #TODO: Need to restructure the embedded word counts.
      result_dict['total_time_s'] = float(words[3])
      result_dict['retval'] = 0
      return normalize(bench, [result_dict])
  raise ValueError('You passed the right type of thing, '
                   'but it didn\'t have the expected contents.')


def parse_Skia(bench, fin):
  obj = json.load(fin)
  return normalize(bench, _TransformBenchmarks(obj))
