# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""An example main file running the algorithms.

Part of the Chrome build flags optimization.

An example use of the framework. It parses the input json configuration file.
Then it initiates the variables of the generation. Finally, it sets up the
processes for different modules and runs the experiment.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import json
import multiprocessing
from optparse import OptionParser
import sys

import flags
from genetic_algorithm import GAGeneration
from pipeline_process import PipelineProcess
import pipeline_worker
from steering import Steering
from task import BUILD_STAGE
from task import Task
from task import TEST_STAGE
import testing_batch

parser = OptionParser()

parser.add_option('-f',
                  '--file',
                  dest='filename',
                  help='configuration file FILE input',
                  metavar='FILE')

# The meta data for the genetic algorithm.
BUILD_CMD = 'BUILD_CMD'
TEST_CMD = 'TEST_CMD'
OUTPUT = 'OUTPUT'
DEFAULT_OUTPUT = 'output'
CONF = 'CONF'
DEFAULT_CONF = 'conf'
NUM_BUILDER = 'NUM_BUILDER'
DEFAULT_NUM_BUILDER = 1
NUM_TESTER = 'NUM_TESTER'
DEFAULT_NUM_TESTER = 1
STOP_THRESHOLD = 'STOP_THRESHOLD'
DEFAULT_STOP_THRESHOLD = 1
NUM_CHROMOSOMES = 'NUM_CHROMOSOMES'
DEFAULT_NUM_CHROMOSOMES = 20
NUM_TRIALS = 'NUM_TRIALS'
DEFAULT_NUM_TRIALS = 20
MUTATION_RATE = 'MUTATION_RATE'
DEFAULT_MUTATION_RATE = 0.01


def _ProcessGA(meta_data):
  """Set up the meta data for the genetic algorithm.

  Args:
    meta_data: the meta data for the genetic algorithm.
  """
  assert BUILD_CMD in meta_data
  build_cmd = meta_data[BUILD_CMD]

  assert TEST_CMD in meta_data
  test_cmd = meta_data[TEST_CMD]

  if OUTPUT not in meta_data:
    output_file = DEFAULT_OUTPUT
  else:
    output_file = meta_data[OUTPUT]

  if CONF not in meta_data:
    conf_file = DEFAULT_CONF
  else:
    conf_file = meta_data[CONF]

  if NUM_BUILDER not in meta_data:
    num_builders = DEFAULT_NUM_BUILDER
  else:
    num_builders = meta_data[NUM_BUILDER]

  if NUM_TESTER not in meta_data:
    num_testers = DEFAULT_NUM_TESTER
  else:
    num_testers = meta_data[NUM_TESTER]

  if STOP_THRESHOLD not in meta_data:
    stop_threshold = DEFAULT_STOP_THRESHOLD
  else:
    stop_threshold = meta_data[STOP_THRESHOLD]

  if NUM_CHROMOSOMES not in meta_data:
    num_chromosomes = DEFAULT_NUM_CHROMOSOMES
  else:
    num_chromosomes = meta_data[NUM_CHROMOSOMES]

  if NUM_TRIALS not in meta_data:
    num_trials = DEFAULT_NUM_TRIALS
  else:
    num_trials = meta_data[NUM_TRIALS]

  if MUTATION_RATE not in meta_data:
    mutation_rate = DEFAULT_MUTATION_RATE
  else:
    mutation_rate = meta_data[MUTATION_RATE]

  specs = flags.ReadConf(conf_file)

  # Initiate the build/test command and the log directory.
  Task.InitLogCommand(build_cmd, test_cmd, output_file)

  # Initiate the build/test command and the log directory.
  GAGeneration.InitMetaData(stop_threshold, num_chromosomes, num_trials, specs,
                            mutation_rate)

  # Generate the initial generations.
  generation_tasks = testing_batch.GenerateRandomGATasks(specs, num_chromosomes,
                                                         num_trials)
  generations = [GAGeneration(generation_tasks, set([]), 0)]

  # Execute the experiment.
  _StartExperiment(num_builders, num_testers, generations)


def _ParseJson(file_name):
  """Parse the input json file.

  Parse the input json file and call the proper function to perform the
  algorithms.

  Args:
    file_name: the input json file name.
  """

  experiments = json.load(open(file_name))

  for experiment in experiments:
    if experiment == 'GA':
      # An GA experiment
      _ProcessGA(experiments[experiment])


def _StartExperiment(num_builders, num_testers, generations):
  """Set up the experiment environment and execute the framework.

  Args:
    num_builders: number of concurrent builders.
    num_testers: number of concurrent testers.
    generations: the initial generation for the framework.
  """

  manager = multiprocessing.Manager()

  # The queue between the steering algorithm and the builder.
  steering_build = manager.Queue()
  # The queue between the builder and the tester.
  build_test = manager.Queue()
  # The queue between the tester and the steering algorithm.
  test_steering = manager.Queue()

  # Set up the processes for the builder, tester and steering algorithm module.
  build_process = PipelineProcess(num_builders, 'builder', {}, BUILD_STAGE,
                                  steering_build, pipeline_worker.Helper,
                                  pipeline_worker.Worker, build_test)

  test_process = PipelineProcess(num_testers, 'tester', {}, TEST_STAGE,
                                 build_test, pipeline_worker.Helper,
                                 pipeline_worker.Worker, test_steering)

  steer_process = multiprocessing.Process(
      target=Steering,
      args=(set([]), generations, test_steering, steering_build))

  # Start the processes.
  build_process.start()
  test_process.start()
  steer_process.start()

  # Wait for the processes to finish.
  build_process.join()
  test_process.join()
  steer_process.join()


def main(argv):
  (options, _) = parser.parse_args(argv)
  assert options.filename
  _ParseJson(options.filename)


if __name__ == '__main__':
  main(sys.argv)
