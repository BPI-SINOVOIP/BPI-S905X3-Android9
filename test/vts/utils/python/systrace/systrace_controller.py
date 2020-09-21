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

import os
import tempfile
import shutil
import subprocess
import logging

PATH_SYSTRACE_SCRIPT = os.path.join('tools/external/chromium-trace',
                                    'systrace.py')
EXPECTED_START_STDOUT = 'Starting tracing'


class SystraceController(object):
    '''A util to start/stop systrace through shell command.

    Attributes:
        _android_vts_path: string, path to android-vts
        _path_output: string, systrace temporally output path
        _path_systrace_script: string, path to systrace controller python script
        _device_serial: string, device serial string
        _subprocess: subprocess.Popen, a subprocess objects of systrace shell command
        is_valid: boolean, whether the current environment setting for
                  systrace is valid
        process_name: string, process name to trace. The value can be empty.
    '''

    def __init__(self, android_vts_path, device_serial, process_name=''):
        self._android_vts_path = android_vts_path
        self._path_output = None
        self._subprocess = None
        self._device_serial = device_serial
        if not device_serial:
            logging.warning(
                'Device serial is not provided for systrace. '
                'Tool will not start if multiple devices are connected.')
        self.process_name = process_name
        self._path_systrace_script = os.path.join(android_vts_path,
                                                  PATH_SYSTRACE_SCRIPT)
        self.is_valid = os.path.exists(self._path_systrace_script)
        if not self.is_valid:
            logging.error('invalid systrace script path: %s',
                          self._path_systrace_script)

    @property
    def is_valid(self):
        ''''returns whether the current environment setting is valid'''
        return self._is_valid

    @is_valid.setter
    def is_valid(self, is_valid):
        ''''Set valid status'''
        self._is_valid = is_valid

    @property
    def process_name(self):
        ''''returns process name'''
        return self._process_name

    @process_name.setter
    def process_name(self, process_name):
        ''''Set process name'''
        self._process_name = process_name

    @property
    def has_output(self):
        ''''returns whether output file exists and not empty.

        Returns:
            False if output path is not specified, or output file doesn't exist, or output
            file size is zero; True otherwise.
        '''
        if not self._path_output:
            logging.warning('systrace output path is empty.')
            return False

        try:
            if os.path.getsize(self._path_output) == 0:
                logging.warning('systrace output file is empty.')
                return False
        except OSError:
            logging.info('systrace output file does not exist.')
            return False
        return True

    def Start(self):
        '''Start systrace process.

        Use shell command to start a python systrace script

        Returns:
            True if successfully started systrace; False otherwise.
        '''
        self._subprocess = None
        self._path_output = None

        if not self.is_valid:
            logging.error(
                'Cannot start systrace: configuration is not correct for %s.',
                self.process_name)
            return False

        # TODO: check target device for compatibility (e.g. has systrace hooks)
        process_name_arg = ''
        if self.process_name:
            process_name_arg = '-a %s' % self.process_name

        device_serial_arg = ''
        if self._device_serial:
            device_serial_arg = '--serial=%s' % self._device_serial

        tmp_dir = tempfile.mkdtemp()
        tmp_filename = self.process_name if self.process_name else 'systrace'
        self._path_output = str(os.path.join(tmp_dir, tmp_filename + '.html'))

        cmd = ('python -u {script} hal sched '
               '{process_name_arg} {serial} -o {output}').format(
                   script=self._path_systrace_script,
                   process_name_arg=process_name_arg,
                   serial=device_serial_arg,
                   output=self._path_output)
        process = subprocess.Popen(
            str(cmd),
            shell=True,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE)

        line = ''
        success = False
        while process.poll() is None:
            line += process.stdout.read(1)

            if not line:
                break
            elif EXPECTED_START_STDOUT in line:
                success = True
                break

        if not success:
            logging.error('Failed to start systrace on process %s',
                          self.process_name)
            stdout, stderr = process.communicate()
            logging.error('stdout: %s', line + stdout)
            logging.error('stderr: %s', stderr)
            logging.error('ret_code: %s', process.returncode)
            return False

        self._subprocess = process
        logging.info('Systrace started for %s', self.process_name)
        return True

    def Stop(self):
        '''Stop systrace process.

        Returns:
            True if successfully stopped systrace or systrace already stopped;
            False otherwise.
        '''
        if not self.is_valid:
            logging.warn(
                'Cannot stop systrace: systrace was not started for %s.',
                self.process_name)
            return False

        if not self._subprocess:
            logging.info('Systrace already stopped.')
            return True

        # Press enter to stop systrace script
        self._subprocess.stdin.write('\n')
        self._subprocess.stdin.flush()
        # Wait for output to be written down
        # TODO: use subprocess.TimeoutExpired after upgrading to python >3.3
        out, err = self._subprocess.communicate()
        logging.info('Systrace stopped for %s', self.process_name)
        logging.info('Systrace stdout: %s', out)
        logging.info('Systrace stderr: %s', err)

        self._subprocess = None

        return True

    def ReadLastOutput(self):
        '''Read systrace output html.

        Returns:
            string, data of systrace html output. None if failed to read.
        '''
        if not self.is_valid or not self._subprocess:
            logging.warn(
                'Cannot read output: systrace was not started for %s.',
                self.process_name)
            return None

        if not self.has_output:
            logging.error(
                'systrace did not started/ended correctly. Output is empty.')
            return False

        try:
            with open(self._path_output, 'r') as f:
                data = f.read()
                logging.info('Systrace output length for %s: %s', process_name,
                             len(data))
                return data
        except Exception as e:
            logging.error('Cannot read output: file open failed, %s', e)
            return None

    def SaveLastOutput(self, report_path=None):
        if not report_path:
            logging.error('report path supplied is None')
            return False
        report_path = str(report_path)

        if not self.has_output:
            logging.error(
                'systrace did not started/ended correctly. Output is empty.')
            return False

        parent_dir = os.path.dirname(report_path)
        if not os.path.exists(parent_dir):
            try:
                os.makedirs(parent_dir)
            except Exception as e:
                logging.error('error happened while creating directory: %s', e)
                return False

        try:
            shutil.copy(self._path_output, report_path)
        except Exception as e:  # TODO(yuexima): more specific error catch
            logging.error('failed to copy output to report path: %s', e)
            return False

        return True

    def ClearLastOutput(self):
        '''Clear systrace output html.

        Since output are created in temp directories, this step is optional.

        Returns:
            True if successfully deleted temp output file; False otherwise.
        '''

        if self._path_output:
            try:
                shutil.rmtree(os.path.basename(self._path_output))
            except Exception as e:
                logging.error('failed to remove systrace output file. %s', e)
                return False
            finally:
                self._path_output = None

        return True
