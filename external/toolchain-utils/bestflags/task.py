# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""A reproducing entity.

Part of the Chrome build flags optimization.

The Task class is used by different modules. Each module fills in the
corresponding information into a Task instance. Class Task contains the bit set
representing the flags selection. The builder module is responsible for filling
the image and the checksum field of a Task. The executor module will put the
execution output to the execution field.
"""

__author__ = 'yuhenglong@google.com (Yuheng Long)'

import os
import subprocess
import sys
from uuid import uuid4

BUILD_STAGE = 1
TEST_STAGE = 2

# Message indicating that the build or test failed.
ERROR_STRING = 'error'

# The maximum number of tries a build can have. Some compilations may fail due
# to unexpected environment circumstance. This variable defines how many tries
# the build should attempt before giving up.
BUILD_TRIES = 3

# The maximum number of tries a test can have. Some tests may fail due to
# unexpected environment circumstance. This variable defines how many tries the
# test should attempt before giving up.
TEST_TRIES = 3


# Create the file/directory if it does not already exist.
def _CreateDirectory(file_name):
  directory = os.path.dirname(file_name)
  if not os.path.exists(directory):
    os.makedirs(directory)


class Task(object):
  """A single reproducing entity.

  A single test of performance with a particular set of flags. It records the
  flag set, the image, the check sum of the image and the cost.
  """

  # The command that will be used in the build stage to compile the tasks.
  BUILD_COMMAND = None
  # The command that will be used in the test stage to test the tasks.
  TEST_COMMAND = None
  # The directory to log the compilation and test results.
  LOG_DIRECTORY = None

  @staticmethod
  def InitLogCommand(build_command, test_command, log_directory):
    """Set up the build and test command for the task and the log directory.

    This framework is generic. It lets the client specify application specific
    compile and test methods by passing different build_command and
    test_command.

    Args:
      build_command: The command that will be used in the build stage to compile
        this task.
      test_command: The command that will be used in the test stage to test this
        task.
      log_directory: The directory to log the compilation and test results.
    """

    Task.BUILD_COMMAND = build_command
    Task.TEST_COMMAND = test_command
    Task.LOG_DIRECTORY = log_directory

  def __init__(self, flag_set):
    """Set up the optimization flag selection for this task.

    Args:
      flag_set: The optimization flag set that is encapsulated by this task.
    """

    self._flag_set = flag_set

    # A unique identifier that distinguishes this task from other tasks.
    self._task_identifier = uuid4()

    self._log_path = (Task.LOG_DIRECTORY, self._task_identifier)

    # Initiate the hash value. The hash value is used so as not to recompute it
    # every time the hash method is called.
    self._hash_value = None

    # Indicate that the task has not been compiled/tested.
    self._build_cost = None
    self._exe_cost = None
    self._checksum = None
    self._image = None
    self._file_length = None
    self._text_length = None

  def __eq__(self, other):
    """Test whether two tasks are equal.

    Two tasks are equal if their flag_set are equal.

    Args:
      other: The other task with which this task is tested equality.
    Returns:
      True if the encapsulated flag sets are equal.
    """
    if isinstance(other, Task):
      return self.GetFlags() == other.GetFlags()
    return False

  def __hash__(self):
    if self._hash_value is None:
      # Cache the hash value of the flags, so as not to recompute them.
      self._hash_value = hash(self._flag_set)
    return self._hash_value

  def GetIdentifier(self, stage):
    """Get the identifier of the task in the stage.

    The flag set uniquely identifies a task in the build stage. The checksum of
    the image of the task uniquely identifies the task in the test stage.

    Args:
      stage: The stage (build/test) in which this method is called.
    Returns:
      Return the flag set in build stage and return the checksum in test stage.
    """

    # Define the dictionary for different stage function lookup.
    get_identifier_functions = {BUILD_STAGE: self.FormattedFlags,
                                TEST_STAGE: self.__GetCheckSum}

    assert stage in get_identifier_functions
    return get_identifier_functions[stage]()

  def GetResult(self, stage):
    """Get the performance results of the task in the stage.

    Args:
      stage: The stage (build/test) in which this method is called.
    Returns:
      Performance results.
    """

    # Define the dictionary for different stage function lookup.
    get_result_functions = {BUILD_STAGE: self.__GetBuildResult,
                            TEST_STAGE: self.GetTestResult}

    assert stage in get_result_functions

    return get_result_functions[stage]()

  def SetResult(self, stage, result):
    """Set the performance results of the task in the stage.

    This method is called by the pipeling_worker to set the results for
    duplicated tasks.

    Args:
      stage: The stage (build/test) in which this method is called.
      result: The performance results of the stage.
    """

    # Define the dictionary for different stage function lookup.
    set_result_functions = {BUILD_STAGE: self.__SetBuildResult,
                            TEST_STAGE: self.__SetTestResult}

    assert stage in set_result_functions

    set_result_functions[stage](result)

  def Done(self, stage):
    """Check whether the stage is done.

    Args:
      stage: The stage to be checked, build or test.
    Returns:
      True if the stage is done.
    """

    # Define the dictionary for different result string lookup.
    done_string = {BUILD_STAGE: self._build_cost, TEST_STAGE: self._exe_cost}

    assert stage in done_string

    return done_string[stage] is not None

  def Work(self, stage):
    """Perform the task.

    Args:
      stage: The stage in which the task is performed, compile or test.
    """

    # Define the dictionary for different stage function lookup.
    work_functions = {BUILD_STAGE: self.__Compile, TEST_STAGE: self.__Test}

    assert stage in work_functions

    work_functions[stage]()

  def FormattedFlags(self):
    """Format the optimization flag set of this task.

    Returns:
      The formatted optimization flag set that is encapsulated by this task.
    """
    return str(self._flag_set.FormattedForUse())

  def GetFlags(self):
    """Get the optimization flag set of this task.

    Returns:
      The optimization flag set that is encapsulated by this task.
    """

    return self._flag_set

  def __GetCheckSum(self):
    """Get the compilation image checksum of this task.

    Returns:
      The compilation image checksum of this task.
    """

    # The checksum should be computed before this method is called.
    assert self._checksum is not None
    return self._checksum

  def __Compile(self):
    """Run a compile.

    This method compile an image using the present flags, get the image,
    test the existent of the image and gathers monitoring information, and sets
    the internal cost (fitness) for this set of flags.
    """

    # Format the flags as a string as input to compile command. The unique
    # identifier is passed to the compile command. If concurrent processes are
    # used to compile different tasks, these processes can use the identifier to
    # write to different file.
    flags = self._flag_set.FormattedForUse()
    command = '%s %s %s' % (Task.BUILD_COMMAND, ' '.join(flags),
                            self._task_identifier)

    # Try BUILD_TRIES number of times before confirming that the build fails.
    for _ in range(BUILD_TRIES):
      try:
        # Execute the command and get the execution status/results.
        p = subprocess.Popen(command.split(),
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()

        if out:
          out = out.strip()
          if out != ERROR_STRING:
            # Each build results contains the checksum of the result image, the
            # performance cost of the build, the compilation image, the length
            # of the build, and the length of the text section of the build.
            (checksum, cost, image, file_length, text_length) = out.split()
            # Build successfully.
            break

        # Build failed.
        cost = ERROR_STRING
      except _:
        # If there is exception getting the cost information of the build, the
        # build failed.
        cost = ERROR_STRING

    # Convert the build cost from String to integer. The build cost is used to
    # compare a task with another task. Set the build cost of the failing task
    # to the max integer. The for loop will keep trying until either there is a
    # success or BUILD_TRIES number of tries have been conducted.
    self._build_cost = sys.maxint if cost == ERROR_STRING else float(cost)

    self._checksum = checksum
    self._file_length = file_length
    self._text_length = text_length
    self._image = image

    self.__LogBuildCost(err)

  def __Test(self):
    """__Test the task against benchmark(s) using the input test command."""

    # Ensure that the task is compiled before being tested.
    assert self._image is not None

    # If the task does not compile, no need to test.
    if self._image == ERROR_STRING:
      self._exe_cost = ERROR_STRING
      return

    # The unique identifier is passed to the test command. If concurrent
    # processes are used to compile different tasks, these processes can use the
    # identifier to write to different file.
    command = '%s %s %s' % (Task.TEST_COMMAND, self._image,
                            self._task_identifier)

    # Try TEST_TRIES number of times before confirming that the build fails.
    for _ in range(TEST_TRIES):
      try:
        p = subprocess.Popen(command.split(),
                             stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        (out, err) = p.communicate()

        if out:
          out = out.strip()
          if out != ERROR_STRING:
            # The test results contains the performance cost of the test.
            cost = out
            # Test successfully.
            break

        # Test failed.
        cost = ERROR_STRING
      except _:
        # If there is exception getting the cost information of the test, the
        # test failed. The for loop will keep trying until either there is a
        # success or TEST_TRIES number of tries have been conducted.
        cost = ERROR_STRING

    self._exe_cost = sys.maxint if (cost == ERROR_STRING) else float(cost)

    self.__LogTestCost(err)

  def __SetBuildResult(self, (checksum, build_cost, image, file_length,
                              text_length)):
    self._checksum = checksum
    self._build_cost = build_cost
    self._image = image
    self._file_length = file_length
    self._text_length = text_length

  def __GetBuildResult(self):
    return (self._checksum, self._build_cost, self._image, self._file_length,
            self._text_length)

  def GetTestResult(self):
    return self._exe_cost

  def __SetTestResult(self, exe_cost):
    self._exe_cost = exe_cost

  def LogSteeringCost(self):
    """Log the performance results for the task.

    This method is called by the steering stage and this method writes the
    results out to a file. The results include the build and the test results.
    """

    steering_log = '%s/%s/steering.txt' % self._log_path

    _CreateDirectory(steering_log)

    with open(steering_log, 'w') as out_file:
      # Include the build and the test results.
      steering_result = (self._flag_set, self._checksum, self._build_cost,
                         self._image, self._file_length, self._text_length,
                         self._exe_cost)

      # Write out the result in the comma-separated format (CSV).
      out_file.write('%s,%s,%s,%s,%s,%s,%s\n' % steering_result)

  def __LogBuildCost(self, log):
    """Log the build results for the task.

    The build results include the compilation time of the build, the result
    image, the checksum, the file length and the text length of the image.
    The file length of the image includes the length of the file of the image.
    The text length only includes the length of the text section of the image.

    Args:
      log: The build log of this task.
    """

    build_result_log = '%s/%s/build.txt' % self._log_path

    _CreateDirectory(build_result_log)

    with open(build_result_log, 'w') as out_file:
      build_result = (self._flag_set, self._build_cost, self._image,
                      self._checksum, self._file_length, self._text_length)

      # Write out the result in the comma-separated format (CSV).
      out_file.write('%s,%s,%s,%s,%s,%s\n' % build_result)

    # The build information about running the build.
    build_run_log = '%s/%s/build_log.txt' % self._log_path
    _CreateDirectory(build_run_log)

    with open(build_run_log, 'w') as out_log_file:
      # Write out the execution information.
      out_log_file.write('%s' % log)

  def __LogTestCost(self, log):
    """Log the test results for the task.

    The test results include the runtime execution time of the test.

    Args:
      log: The test log of this task.
    """

    test_log = '%s/%s/test.txt' % self._log_path

    _CreateDirectory(test_log)

    with open(test_log, 'w') as out_file:
      test_result = (self._flag_set, self._checksum, self._exe_cost)

      # Write out the result in the comma-separated format (CSV).
      out_file.write('%s,%s,%s\n' % test_result)

    # The execution information about running the test.
    test_run_log = '%s/%s/test_log.txt' % self._log_path

    _CreateDirectory(test_run_log)

    with open(test_run_log, 'w') as out_log_file:
      # Append the test log information.
      out_log_file.write('%s' % log)

  def IsImproved(self, other):
    """Compare the current task with another task.

    Args:
      other: The other task against which the current task is compared.

    Returns:
      True if this task has improvement upon the other task.
    """

    # The execution costs must have been initiated.
    assert self._exe_cost is not None
    assert other.GetTestResult() is not None

    return self._exe_cost < other.GetTestResult()
