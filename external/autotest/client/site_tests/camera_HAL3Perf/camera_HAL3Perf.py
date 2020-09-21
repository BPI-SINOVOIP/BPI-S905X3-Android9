# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""
A test which monitors the camera HAL3 performance metrics:
    1. Time to open device
    2. Time to start preview
    3. Time to capture still image
    4. Shot to shot time
"""

import os, logging
from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import service_stopper

class camera_HAL3Perf(test.test):
    """
    This test monitors several performance metrics reported by the test binary
    arc_camera3_test.
    """

    version = 1
    test_binary = 'arc_camera3_test'
    dep = 'camera_hal3'
    timeout = 60
    test_name = 'camera_HAL3Perf'
    test_log_suffix = 'test.log'
    adapter_service = 'camera-halv3-adapter'

    def _logperf(self, name, key, value, units, higher_is_better=False):
        description = '%s.%s' % (name, key)
        self.output_perf_value(
            description=description, value=value, units=units,
            higher_is_better=higher_is_better)

    def _analyze_log(self, log_file):
        """
        Analyzes the performance metrics extracted from the log file.

        @param log_file: path of the log file. Sample lines of the log file are
            as below.
            Camera: 0
            device_open: 5020 us
            preview_start: 353285 us
            still_image_capture: 308166 us
        """
        try:
            with open(log_file, 'r') as f:
                camera_id = None
                for line in f:
                    key, value = line.split(':')
                    if key == 'Camera':
                        camera_id = value.strip()
                    elif camera_id is not None:
                        self._logperf(self.test_name,
                                      'camera_%s_%s' % (camera_id, key),
                                      value.strip().split(' ')[0],
                                      value.strip().split(' ')[1])
                    else:
                        msg = 'Error in parsing the log file (%s)' % log_file
                        raise error.TestFail(msg)
        except IOError, err:
            msg = 'Error in reading the log file (%s): %s' % (log_file, err)
            raise error.TestFail(msg)

    def setup(self):
        """
        Run common setup steps.
        """
        self.dep_dir = os.path.join(self.autodir, 'deps', self.dep)
        self.job.setup_dep([self.dep])
        logging.debug('mydep is at %s', self.dep_dir)

    def run_once(self):
        """
        Entry point of this test.
        """
        self.job.install_pkg(self.dep, 'dep', self.dep_dir)

        with service_stopper.ServiceStopper([self.adapter_service]):
            binary_path = os.path.join(self.dep_dir, 'bin', self.test_binary)
            test_log_file = os.path.join(self.resultsdir,
                                         '%s_%s' % (self.test_name,
                                                    self.test_log_suffix))
            args = [
                '--gtest_filter=Camera3StillCaptureTest/'
                'Camera3SimpleStillCaptureTest.PerformanceTest/*',
                '--output_log=%s' % test_log_file]
            ret = utils.system(' '.join([binary_path, ' '.join(args)]),
                               timeout=self.timeout, ignore_status=True)
            self._analyze_log(test_log_file)
            if ret != 0:
                msg = 'Failed to execute command: ' + ' '.join([binary_path,
                                                               ' '.join(args)])
                raise error.TestFail(msg)
