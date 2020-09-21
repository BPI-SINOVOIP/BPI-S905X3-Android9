# Copyright 2011 Google Inc. All Rights Reserved.
"""Module to print help message."""

from __future__ import print_function

import sys
import textwrap
from settings_factory import BenchmarkSettings
from settings_factory import GlobalSettings
from settings_factory import LabelSettings


class Help(object):
  """The help class."""

  def GetUsage(self):
    return """%s [OPTIONS] EXPERIMENT_FILE""" % (sys.argv[0])

  def _WrapLine(self, line):
    return '\n'.join(textwrap.wrap(line, 80))

  def _GetFieldDescriptions(self, fields):
    res = ''
    for field_name in fields:
      field = fields[field_name]
      res += 'Field:\t\t%s\n' % field.name
      res += self._WrapLine('Description:\t%s' % field.description) + '\n'
      res += 'Type:\t\t%s\n' % type(field).__name__.replace('Field', '')
      res += 'Required:\t%s\n' % field.required
      if field.default:
        res += 'Default:\t%s\n' % field.default
      res += '\n'
    return res

  def GetHelp(self):
    global_fields = self._GetFieldDescriptions(GlobalSettings('').fields)
    benchmark_fields = self._GetFieldDescriptions(BenchmarkSettings('').fields)
    label_fields = self._GetFieldDescriptions(LabelSettings('').fields)

    return """%s is a script for running performance experiments on
ChromeOS. It allows one to run ChromeOS Autotest benchmarks over
several images and compare the results to determine whether there
is a performance difference.

Comparing several images using %s is referred to as running an
"experiment". An "experiment file" is a configuration file which holds
all the information that describes the experiment and how it should be
run. An example of a simple experiment file is below:

--------------------------------- test.exp ---------------------------------
name: my_experiment
board: x86-alex
remote: chromeos2-row1-rack4-host7.cros 172.18.122.132

benchmark: page_cycler_v2.morejs {
  suite: telemetry_Crosperf
  iterations: 3
}

my_first_image {
  chromeos_image: /usr/local/chromeos-1/chromiumos_image.bin
}

my_second_image {
  chromeos_image:  /usr/local/chromeos-2/chromiumos_image.bin
}
----------------------------------------------------------------------------

This experiment file names the experiment "my_experiment". It will be
run on the board x86-alex. Benchmarks will be run using two remote
devices, one is a device specified by a hostname and the other is a
device specified by it's IP address. Benchmarks will be run in
parallel across these devices.  There is currently no way to specify
which benchmark will run on each device.

We define one "benchmark" that will be run, page_cycler_v2.morejs. This
benchmark has two "fields", one which specifies that this benchmark is
part of the telemetry_Crosperf suite (this is the common way to run
most Telemetry benchmarks), and the other which specifies how many
iterations it will run for.

We specify one or more "labels" or images which will be compared. The
page_cycler_v2.morejs benchmark will be run on each of these images 3
times and a result table will be output which compares them for all
the images specified.

The full list of fields that can be specified in the experiment file
are as follows:
=================
Global Fields
=================
%s
=================
Benchmark Fields
=================
%s
=================
Label Fields
=================
%s

Note that global fields are overidden by label or benchmark fields, if
they can be specified in both places. Fields that are specified as
arguments override fields specified in experiment files.

%s is invoked by passing it a path to an experiment file,
as well as any options (in addition to those specified in the
experiment file).  Crosperf runs the experiment and caches the results
(or reads the previously cached experiment results out of the cache),
generates and displays a report based on the run, and emails the
report to the user.  If the results were all read out of the cache,
then by default no email is generated.
""" % (sys.argv[0], sys.argv[0], global_fields, benchmark_fields, label_fields,
       sys.argv[0])
