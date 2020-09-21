#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import itertools

from vts.runners.host import const


class CheckSetupCleanup(object):
    """An abstract class for defining environment setup job.

    Usually such a job contains check -> setup -> cleanup workflow

    Attributes:
        to_check: bool, whether or not to check the defined environment
                  requirement. Default: True
        to_setup: bool, whether or not to setup the defined environment
                  requirement. Default: False
        to_cleanup: bool, whether or not to cleanup the defined environment
                    requirement if it is set up by this class. Default: False
        context: ShellEnvironment object that provides AddCleanupJobs and
                 ExecuteShellCommand method
        note: string, used by GetNote to generate error message if fail.
    """

    def __init__(self):
        self.to_check = True
        self.to_setup = False
        self.to_cleanup = False
        self.context = None
        self.note = None

    @property
    def note(self):
        return self._note

    @note.setter
    def note(self, note):
        self._note = note

    @property
    def context(self):
        return self._context

    @context.setter
    def context(self, context):
        self._context = context

    def Execute(self):
        """Execute the check, setup, and cleanup.

        Will execute setup and cleanup only if the boolean switches for them
        are True. It will NOT execute cleanup if check function passes.

           Return:
               tuple(bool, string), a tuple of True and empty string if success,
                                    a tuple of False and error message if fail.
        """

        if self.context is None:
            self.note = "Error: Environment definition context not set"
            return False

        if not self.InternalCall(self.ValidateInputs):
            return False

        check_result = False
        if self.to_check:
            check_result = self.InternalCall(self.Check)

        if check_result or not self.to_setup:
            return check_result

        if self.to_cleanup:
            self.context.AddCleanupJob(self.InternalCall, self.Cleanup)

        return self.InternalCall(self.Setup)

    def __str__(self):
        return ("Shell Environment Check Definition Class:{cls} "
                "Variables: {props}").format(
                    cls=self.__class__.__name__, props=vars(self))

    def GetNote(self):
        """Get a string note as error message. Can be override by sub-class"""
        if not self.note:
            self.note = "Shell environment definition unsatisfied"
        return "{}\nat: {}".format(self.note, self)

    def InternalCall(self, method):
        """Internal method to call sub class inherited methods.

        It call the function, check results, and put failure note if not set
        """
        self.note = None
        success = method()
        if not success and not self.note:
            self.note = ("Shell environment definition unsatisfied: step [%s] "
                         "failed.") % method.__name__
        return success

    def ValidateInputs(self):
        """Validate input parameters. Can be override by sub-class

        Return:
            tuple(bool, string), a tuple of True and empty string if pass,
                                 a tuple of False and error message if fail.
        """
        return True

    def ToListLike(self, obj):
        """Convert single item to list of single item.

        If input is already a list like object, the same object will
        be returned.
        This method is for the convenience of writing child class.

        Arguments:
            obj: any object
        """
        if not self.IsListLike(obj):
            return [obj]
        else:
            return obj

    def IsListLike(self, obj):
        """Checks whether a object is list-like.

        This method is for the convenience of writing child class.
        It will check for existence of __iter__ and __getitem__
        in attributes of the object.
        String is not considered list-like, tuple is.

        Arguments:
            obj: any object
        """
        return hasattr(obj, '__iter__') and hasattr(obj, '__getitem__')

    def NormalizeInputLists(self, *inputs):
        """Normalize inputs to lists of same length.

        This method is for the convenience of writing child class.
        If there are lists in inputs, they should all be of same length;
        otherwise, None is returned.
        If there are lists and single items in inputs, single items are
        duplicated to a list of other list's length.
        If there are only single items in inputs, they all get converted to
        single item lists.

        Arguments:
            inputs: any inputs

        Return:
            a generator of normalized inputs
        """
        # If any of inputs is None or empty, inputs are considered not valid
        # Definition classes wish to take None input should not use this method
        if not all(inputs):
            return None

        lists = filter(self.IsListLike, inputs)
        if not lists:
            # All inputs are single items. Convert them to lists
            return ([i] for i in inputs)

        lengths = set(map(len, lists))
        if len(lengths) != 1:
            # lists in inputs have different lengths, cannot normalize
            return None
        length = lengths.pop()

        return (i if self.IsListLike(i) else list(itertools.repeat(i, length))
                for i in inputs)

    def Check(self):
        """Check function for the class.

        Used to check environment. Can be override by sub-class
        """
        self.note = "Check step undefined."
        return False

    def Setup(self):
        """Check function for the class.

        Used to setup environment if check fail. Can be override by sub-class
        """
        self.note = "Setup step undefined."
        return False

    def Cleanup(self):
        """Check function for the class.

        Used to cleanup setup if check fail. Can be override by sub-class
        """
        self.note = "Cleanup step undefined."
        return False

    @property
    def shell(self):
        """returns an object that can execute a shell command"""
        return self.context.shell

    def ExecuteShellCommand(self, cmd):
        """Execute a shell command or a list of shell commands.

        Args:
            command: str, the command to execute; Or
                     list of str, a list of commands to execute

        return:
            A dictionary containing results in format:
                {const.STDOUT: [stdouts],
                 const.STDERR: [stderrs],
                 const.EXIT_CODE: [exit_codes]}.
        """
        return self.shell.Execute(cmd)
