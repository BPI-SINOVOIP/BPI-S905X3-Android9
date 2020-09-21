#!/usr/bin/python
#
# Copyright 2011 Google Inc. All Rights Reserved.
#

__author__ = 'kbaclawski@google.com (Krystian Baclawski)'

import cStringIO
import logging
import os
import signal
import socket
import sys
import time
import unittest


def AddScriptDirToPath():
  """Required for remote python script execution."""
  path = os.path.abspath(__file__)

  for _ in range(3):
    path, _ = os.path.split(path)

  if not path in sys.path:
    sys.path.append(path)


AddScriptDirToPath()

from automation.common.command_executer import CommandExecuter


class LoggerMock(object):

  def LogCmd(self, cmd, machine='', user=''):
    if machine:
      logging.info('[%s] Executing: %s', machine, cmd)
    else:
      logging.info('Executing: %s', cmd)

  def LogError(self, msg):
    logging.error(msg)

  def LogWarning(self, msg):
    logging.warning(msg)

  def LogOutput(self, msg):
    logging.info(msg)


class CommandExecuterUnderTest(CommandExecuter):

  def __init__(self):
    CommandExecuter.__init__(self, logger_to_set=LoggerMock())

    # We will record stdout and stderr.
    self._stderr = cStringIO.StringIO()
    self._stdout = cStringIO.StringIO()

  @property
  def stdout(self):
    return self._stdout.getvalue()

  @property
  def stderr(self):
    return self._stderr.getvalue()

  def DataReceivedOnOutput(self, data):
    self._stdout.write(data)

  def DataReceivedOnError(self, data):
    self._stderr.write(data)


class CommandExecuterLocalTests(unittest.TestCase):
  HOSTNAME = None

  def setUp(self):
    self._executer = CommandExecuterUnderTest()

  def tearDown(self):
    pass

  def RunCommand(self, method, **kwargs):
    program = os.path.abspath(sys.argv[0])

    return self._executer.RunCommand('%s runHelper %s' % (program, method),
                                     machine=self.HOSTNAME,
                                     **kwargs)

  def testCommandTimeout(self):
    exit_code = self.RunCommand('SleepForMinute', command_timeout=3)

    self.assertTrue(-exit_code in [signal.SIGTERM, signal.SIGKILL],
                    'Invalid exit code: %d' % exit_code)

  def testCommandTimeoutIfSigTermIgnored(self):
    exit_code = self.RunCommand('IgnoreSigTerm', command_timeout=3)

    self.assertTrue(-exit_code in [signal.SIGTERM, signal.SIGKILL])

  def testCommandSucceeded(self):
    self.assertFalse(self.RunCommand('ReturnTrue'))

  def testCommandFailed(self):
    self.assertTrue(self.RunCommand('ReturnFalse'))

  def testStringOnOutputStream(self):
    self.assertFalse(self.RunCommand('EchoToOutputStream'))
    self.assertEquals(self._executer.stderr, '')
    self.assertEquals(self._executer.stdout, 'test')

  def testStringOnErrorStream(self):
    self.assertFalse(self.RunCommand('EchoToErrorStream'))
    self.assertEquals(self._executer.stderr, 'test')
    self.assertEquals(self._executer.stdout, '')

  def testOutputStreamNonInteractive(self):
    self.assertFalse(
        self.RunCommand('IsOutputStreamInteractive'),
        'stdout stream is a terminal!')

  def testErrorStreamNonInteractive(self):
    self.assertFalse(
        self.RunCommand('IsErrorStreamInteractive'),
        'stderr stream is a terminal!')

  def testAttemptToRead(self):
    self.assertFalse(self.RunCommand('WaitForInput', command_timeout=3))

  def testInterruptedProcess(self):
    self.assertEquals(self.RunCommand('TerminateBySigAbrt'), -signal.SIGABRT)


class CommandExecuterRemoteTests(CommandExecuterLocalTests):
  HOSTNAME = socket.gethostname()

  def testCommandTimeoutIfSigTermIgnored(self):
    exit_code = self.RunCommand('IgnoreSigTerm', command_timeout=6)

    self.assertEquals(exit_code, 255)

    lines = self._executer.stdout.splitlines()
    pid = int(lines[0])

    try:
      with open('/proc/%d/cmdline' % pid) as f:
        cmdline = f.read()
    except IOError:
      cmdline = ''

    self.assertFalse('IgnoreSigTerm' in cmdline, 'Process is still alive.')


class CommandExecuterTestHelpers(object):

  def SleepForMinute(self):
    time.sleep(60)
    return 1

  def ReturnTrue(self):
    return 0

  def ReturnFalse(self):
    return 1

  def EchoToOutputStream(self):
    sys.stdout.write('test')
    return 0

  def EchoToErrorStream(self):
    sys.stderr.write('test')
    return 0

  def IsOutputStreamInteractive(self):
    return sys.stdout.isatty()

  def IsErrorStreamInteractive(self):
    return sys.stderr.isatty()

  def IgnoreSigTerm(self):
    os.write(1, '%d' % os.getpid())
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    time.sleep(30)
    return 0

  def WaitForInput(self):
    try:
      # can only read end-of-file marker
      return os.read(0, 1) != ''
    except OSError:
      # that means that stdin descriptor is closed
      return 0

  def TerminateBySigAbrt(self):
    os.kill(os.getpid(), signal.SIGABRT)
    return 0


if __name__ == '__main__':
  FORMAT = '%(asctime)-15s %(levelname)s %(message)s'
  logging.basicConfig(format=FORMAT, level=logging.DEBUG)

  if len(sys.argv) > 1:
    if sys.argv[1] == 'runHelper':
      helpers = CommandExecuterTestHelpers()
      sys.exit(getattr(helpers, sys.argv[2])())

  unittest.main()
