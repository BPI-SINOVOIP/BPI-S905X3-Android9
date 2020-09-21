# Copyright 2016 - The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import shellescape
import signal
import time

from acts.controllers.utils_lib.ssh import connection
from acts.libs.proc import job


class ShellCommand(object):
    """Wraps basic commands that tend to be tied very closely to a shell.

    This class is a wrapper for running basic shell commands through
    any object that has a run command. Basic shell functionality for managing
    the system, programs, and files in wrapped within this class.

    Note: At the moment this only works with the ssh runner.
    """

    def __init__(self, runner, working_dir=None):
        """Creates a new shell command invoker.

        Args:
            runner: The object that will run the shell commands.
            working_dir: The directory that all commands should work in,
                         if none then the runners enviroment default is used.
        """
        self._runner = runner
        self._working_dir = working_dir

    def run(self, command, timeout=3600):
        """Runs a generic command through the runner.

        Takes the command and prepares it to be run in the target shell using
        this objects settings.

        Args:
            command: The command to run.
            timeout: How long to wait for the command (in seconds).

        Returns:
            A CmdResult object containing the results of the shell command.

        Raises:
            job.Error: When the command executed but had an error.
        """
        if self._working_dir:
            command_str = 'cd %s; %s' % (self._working_dir, command)
        else:
            command_str = command

        return self._runner.run(command_str, timeout=timeout)

    def is_alive(self, identifier):
        """Checks to see if a program is alive.

        Checks to see if a program is alive on the shells enviroment. This can
        be used to check on generic programs, or a specific program using
        a pid.

        Args:
            identifier: string or int, Used to identify the program to check.
                        if given an int then it is assumed to be a pid. If
                        given a string then it will be used as a search key
                        to compare on the running processes.
        Returns:
            True if a process was found running, false otherwise.
        """
        try:
            if isinstance(identifier, str):
                self.run('ps aux | grep -v grep | grep %s' % identifier)
            elif isinstance(identifier, int):
                self.signal(identifier, 0)
            else:
                raise ValueError('Bad type was given for identifier')

            return True
        except job.Error:
            return False

    def get_pids(self, identifier):
        """Gets the pids of a program.

        Searches for a program with a specific name and grabs the pids for all
        programs that match.

        Args:
            identifier: A search term that identifies the program.

        Returns: An array of all pids that matched the identifier, or None
                  if no pids were found.
        """
        try:
            result = self.run('ps aux | grep -v grep | grep %s' % identifier)
        except job.Error:
            raise StopIteration

        lines = result.stdout.splitlines()

        # The expected output of the above command is like so:
        # bob    14349  0.0  0.0  34788  5552 pts/2    Ss   Oct10   0:03 bash
        # bob    52967  0.0  0.0  34972  5152 pts/4    Ss   Oct10   0:00 bash
        # Where the format is:
        # USER    PID  ...
        for line in lines:
            pieces = line.split()
            yield int(pieces[1])

    def search_file(self, search_string, file_name):
        """Searches through a file for a string.

        Args:
            search_string: The string or pattern to look for.
            file_name: The name of the file to search.

        Returns:
            True if the string or pattern was found, False otherwise.
        """
        try:
            self.run('grep %s %s' % (shellescape.quote(search_string),
                                     file_name))
            return True
        except job.Error:
            return False

    def read_file(self, file_name):
        """Reads a file through the shell.

        Args:
            file_name: The name of the file to read.

        Returns:
            A string of the files contents.
        """
        return self.run('cat %s' % file_name).stdout

    def write_file(self, file_name, data):
        """Writes a block of data to a file through the shell.

        Args:
            file_name: The name of the file to write to.
            data: The string of data to write.
        """
        return self.run('echo %s > %s' % (shellescape.quote(data), file_name))

    def append_file(self, file_name, data):
        """Appends a block of data to a file through the shell.

        Args:
            file_name: The name of the file to write to.
            data: The string of data to write.
        """
        return self.run('echo %s >> %s' % (shellescape.quote(data), file_name))

    def touch_file(self, file_name):
        """Creates a file through the shell.

        Args:
            file_name: The name of the file to create.
        """
        self.write_file(file_name, '')

    def delete_file(self, file_name):
        """Deletes a file through the shell.

        Args:
            file_name: The name of the file to delete.
        """
        try:
            self.run('rm %s' % file_name)
        except job.Error as e:
            if 'No such file or directory' in e.result.stderr:
                return

            raise

    def kill(self, identifier, timeout=10):
        """Kills a program or group of programs through the shell.

        Kills all programs that match an identifier through the shell. This
        will send an increasing queue of kill signals to all programs
        that match the identifier until either all are dead or the timeout
        finishes.

        Programs are guaranteed to be killed after running this command.

        Args:
            identifier: A string used to identify the program.
            timeout: The time to wait for all programs to die. Each signal will
                     take an equal portion of this time.
        """
        if isinstance(identifier, int):
            pids = [identifier]
        else:
            pids = list(self.get_pids(identifier))

        signal_queue = [signal.SIGINT, signal.SIGTERM, signal.SIGKILL]

        signal_duration = timeout / len(signal_queue)
        for sig in signal_queue:
            for pid in pids:
                try:
                    self.signal(pid, sig)
                except job.Error:
                    pass

            start_time = time.time()
            while pids and time.time() - start_time < signal_duration:
                time.sleep(0.1)
                pids = [pid for pid in pids if self.is_alive(pid)]

            if not pids:
                break

    def signal(self, pid, sig):
        """Sends a specific signal to a program.

        Args:
            pid: The process id of the program to kill.
            sig: The signal to send.

        Raises:
            job.Error: Raised when the signal fail to reach
                       the specified program.
        """
        self.run('kill -%d %d' % (sig, pid))
