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

import os
import operator
import ntpath


def _SafeStrip(value):
    '''Strip string value if value is not None.

    Args:
        value: string, value to strip

    Returns:
        stripped string; None if input value is None.
    '''
    if value is None:
        return value
    return value.strip()


class BinaryTestCase(object):
    '''A class to represent a binary test case.

    Attributes:
        test_suite: string, test suite name.
        test_name: string, test case name which does not include test suite.
        path: string, absolute test binary path on device.
        tag: string, test tag.
        put_tag_func: function that takes a name and tag to output a combination.
        working_directory: string, working directory to call the command.
        ld_library_path: string, a path for LD_LIBRARY_PATH environment variable.
        profiling_library_path: string, path to lookup and load VTS profiling libraries.
        cmd: string, a shell command to execute the test case. If empty, path will be used.
        envp: string, environment veriable. shoud be in format 'name1=value1 name2=value2...'
              Will be called using 'env <envp> <cmd> <args>'
        args: string, arguments following cmd.
        name_appendix: string, appendix attached to the test name in display,
                       typically contains info of parameters used in the test,
                       e.e. service name used for hal hidl test.
    '''

    def __init__(self,
                 test_suite,
                 test_name,
                 path,
                 tag='',
                 put_tag_func=operator.add,
                 working_directory=None,
                 ld_library_path=None,
                 profiling_library_path=None,
                 cmd='',
                 envp='',
                 args='',
                 name_appendix=''):
        self.test_suite = test_suite
        self.test_name = test_name
        self.path = path
        self.tag = tag
        self.put_tag_func = put_tag_func
        self.working_directory = working_directory
        self.ld_library_path = ld_library_path
        self.profiling_library_path = profiling_library_path
        self.cmd = cmd
        self.envp = envp
        self.args = args
        self.name_appendix = name_appendix

    def __str__(self):
        return self._GenerateDisplayName()

    def _GenerateDisplayName(self):
        '''Get a string of test name for display.

        The display name contains three parts: the original full test name, the
        name appendix which includes more info about the test run, and the
        tag(s) used by the test.
        '''
        return self.put_tag_func(self.full_name + self.name_appendix, self.tag)

    @property
    def name_appendix(self):
        return self._name_appendix

    @name_appendix.setter
    def name_appendix(self, name_appendix):
        self._name_appendix = name_appendix

    @property
    def full_name(self):
        '''Get a string that represents the test.

        Returns:
            A string test name in format '<test suite>.<test name>' if
            test_suite is not empty; '<test name>' otherwise
        '''
        return getattr(self, '_full_name', '{}.{}'.format(
            self.test_suite, self.test_name)
                       if self.test_suite else self.test_name)

    @full_name.setter
    def full_name(self, full_name):
        self._full_name = full_name

    def GetRunCommand(self):
        '''Get the command to run the test.

        Returns:
            String, a command to run the test.
        '''
        working_directory = ('cd %s && ' % self.working_directory
                             if self.working_directory else '')

        envp = 'env %s ' % self.envp if self.envp else ''
        ld_library_path = ('LD_LIBRARY_PATH=%s ' % self.ld_library_path
                           if self.ld_library_path else '')

        if ld_library_path:
            envp = ('{}{}'.format(envp, ld_library_path)
                    if envp else 'env %s ' % ld_library_path)

        args = ' %s' % self.args if self.args else ''

        return '{working_directory}{envp}{cmd}{args}'.format(
            working_directory=working_directory,
            envp=envp,
            cmd=self.cmd,
            args=args)

    @property
    def test_suite(self):
        '''Get test_suite'''
        return self._test_suite

    @test_suite.setter
    def test_suite(self, test_suite):
        '''Set test_suite'''
        self._test_suite = _SafeStrip(test_suite)

    @property
    def test_name(self):
        '''Get test_name'''
        return self._test_name

    @test_name.setter
    def test_name(self, test_name):
        '''Set test_name'''
        self._test_name = _SafeStrip(test_name)

    @property
    def path(self):
        '''Get path'''
        return self._path

    @path.setter
    def path(self, path):
        '''Set path'''
        self._path = _SafeStrip(path)

    @property
    def cmd(self):
        '''Get test command. If command is empty, path is returned.'''
        if not self._cmd:
            return self.path

        return self._cmd

    @cmd.setter
    def cmd(self, cmd):
        '''Set path'''
        self._cmd = _SafeStrip(cmd)

    @property
    def tag(self):
        '''Get tag'''
        return self._tag

    @tag.setter
    def tag(self, tag):
        '''Set tag'''
        self._tag = _SafeStrip(tag)

    @property
    def working_directory(self):
        '''Get working_directory'''
        return self._working_directory

    @working_directory.setter
    def working_directory(self, working_directory):
        '''Set working_directory'''
        self._working_directory = _SafeStrip(working_directory)

    @property
    def ld_library_path(self):
        '''Get ld_library_path'''
        return self._ld_library_path

    @ld_library_path.setter
    def ld_library_path(self, ld_library_path):
        '''Set ld_library_path'''
        self._ld_library_path = _SafeStrip(ld_library_path)

    @property
    def envp(self):
        '''Get envp'''
        return self._envp

    @envp.setter
    def envp(self, envp):
        '''Set env'''
        self._envp = _SafeStrip(envp)

    @property
    def args(self):
        '''Get args'''
        return self._args

    @args.setter
    def args(self, args):
        '''Set args'''
        self._args = _SafeStrip(args)
