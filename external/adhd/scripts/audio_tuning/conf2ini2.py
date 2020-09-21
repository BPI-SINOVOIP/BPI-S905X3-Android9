#!/usr/bin/python
#
# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# convert audio.conf from the audio tuning UI to dsp.ini which can be
# accepted by cras eq/drc plugin.

import json
import sys
import fnmatch

biquad_type_name = [
    "none",
    "lowpass",
    "highpass",
    "bandpass",
    "lowshelf",
    "highshelf",
    "peaking",
    "notch",
    "allpass"
    ]

header = """\
[output_source]
library=builtin
label=source
purpose=playback
disable=(not (equal? dsp_name "speaker_eq"))
output_0={src:0}
output_1={src:1}

[output_sink]
library=builtin
label=sink
purpose=playback
input_0={dst:0}
input_1={dst:1}"""

drc_header = """\
[drc]
library=builtin
label=drc
input_0={%s:0}
input_1={%s:1}
output_2={%s:0}
output_3={%s:1}
input_4=%-7d   ; emphasis_disabled"""

drc_param = """\
input_%d=%-7g   ; f
input_%d=%-7g   ; enable
input_%d=%-7g   ; threshold
input_%d=%-7g   ; knee
input_%d=%-7g   ; ratio
input_%d=%-7g   ; attack
input_%d=%-7g   ; release
input_%d=%-7g   ; boost"""

eq_header = """\
[eq2]
library=builtin
label=eq2
input_0={%s:0}
input_1={%s:1}
output_2={%s:0}
output_3={%s:1}"""

eq_param = """\
input_%d=%-7d ; %s
input_%d=%-7g ; freq
input_%d=%-7g ; Q
input_%d=%-7g ; gain"""

def is_true(d, pattern):
  for k in d:
    if fnmatch.fnmatch(k, pattern) and d[k]:
        return True
  return False

def intermediate_name(index):
  return 'intermediate' + ('' if index == 1 else str(index))

def main():
  f = open(sys.argv[1])
  d = json.loads(f.read())
  print header

  has_drc = is_true(d, 'global.enable_drc') and is_true(d, 'drc.*.enable')
  has_eq = is_true(d, 'global.enable_eq') and is_true(d, 'eq.*.*.enable')

  stages = []
  if has_drc:
    stages.append(print_drc)
  if has_eq:
    stages.append(print_eq)

  if is_true(d, 'global.enable_swap') and len(stages) >= 2:
    stages[0], stages[1] = stages[1], stages[0]

  for i in range(len(stages)):
    print
    src = 'src' if i == 0 else intermediate_name(i)
    dst = 'dst' if i == len(stages) - 1 else intermediate_name(i + 1)
    stages[i](d, src, dst)

def print_drc(d, src, dst):
  print drc_header % (src, src, dst, dst, int(d['drc.emphasis_disabled']))
  n = 5
  for i in range(3):
    prefix = 'drc.%d.' % i
    f = d[prefix + 'f']
    enable = int(d[prefix + 'enable'])
    threshold = d[prefix + 'threshold']
    knee = d[prefix + 'knee']
    ratio = d[prefix + 'ratio']
    attack = d[prefix + 'attack']
    release = d[prefix + 'release']
    boost = d[prefix + 'boost']

    print drc_param % (n, f,
                       n+1, enable,
                       n+2, threshold,
                       n+3, knee,
                       n+4, ratio,
                       n+5, attack,
                       n+6, release,
                       n+7, boost)
    n += 8

# Returns two sorted lists, each one contains the enabled eq index for a channel
def enabled_eq(d):
    eeq = [[], []]
    for k in d:
      s = k.split('.')
      if s[0] == 'eq' and s[3] == 'enable' and d[k]:
        ch_index = int(s[1])
        eq_num = int(s[2])
        eeq[ch_index].append(eq_num)
    return sorted(eeq[0]), sorted(eeq[1])

def print_eq(d, src, dst):
  print eq_header % (src, src, dst, dst)
  eeq = enabled_eq(d)
  eeqn = max(len(eeq[0]), len(eeq[1]))
  n = 4  # the first input index
  for i in range(0, eeqn):
    for ch in (0, 1):
      if i < len(eeq[ch]):
        prefix = 'eq.%d.%d.' % (ch, eeq[ch][i])
        type_name = d[prefix + 'type']
        type_index = biquad_type_name.index(type_name)
        f = d[prefix + 'freq']
        q = d[prefix + 'q']
        g = d[prefix + 'gain']
      else:
        type_name = 'none';
        type_index = 0;
        f = q = g = 0
      print eq_param % (n, type_index, type_name,
                        n+1, f, n+2, q, n+3, g)
      n += 4

main()
