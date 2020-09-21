"""Test to ensure we're not touching /dev/ptmx when running commands."""

from __future__ import print_function

import os
import subprocess
import tempfile
import time
import unittest
from cros_utils import command_executer


class NoPsuedoTerminalTest(unittest.TestCase):
  """Test to ensure we're not touching /dev/ptmx when running commands."""

  _strace_process = None
  STRACE_TIMEOUT = 10

  def _AttachStraceToSelf(self, output_file):
    """Attaches strace to the current process."""
    args = ['strace', '-o', output_file, '-p', str(os.getpid())]
    print(args)
    self._strace_process = subprocess.Popen(args)
    # Wait until we see some activity.
    start_time = time.time()
    while time.time() - start_time < self.STRACE_TIMEOUT:
      if os.path.isfile(output_file) and open(output_file).read(1):
        return True
      time.sleep(1)
    return False

  def _KillStraceProcess(self):
    """Kills strace that was started by _AttachStraceToSelf()."""
    self._strace_process.terminate()
    self._strace_process.wait()
    return True

  def testNoPseudoTerminalWhenRunningCommand(self):
    """Test to make sure we're not touching /dev/ptmx when running commands."""
    temp_file = tempfile.mktemp()
    self.assertTrue(self._AttachStraceToSelf(temp_file))

    ce = command_executer.GetCommandExecuter()
    ce.RunCommand('echo')

    self.assertTrue(self._KillStraceProcess())

    strace_contents = open(temp_file).read()
    self.assertFalse('/dev/ptmx' in strace_contents)


if __name__ == '__main__':
  unittest.main()
