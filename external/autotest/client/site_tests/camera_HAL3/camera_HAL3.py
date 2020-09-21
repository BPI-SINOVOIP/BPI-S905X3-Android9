# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A test which verifies the camera function with HAL3 interface."""

import os, logging
import xml.etree.ElementTree
from autotest_lib.client.bin import test, utils
from autotest_lib.client.cros import service_stopper
from sets import Set

class camera_HAL3(test.test):
    """
    This test is a wrapper of the test binary arc_camera3_test.
    """

    version = 1
    test_binary = 'arc_camera3_test'
    dep = 'camera_hal3'
    adapter_service = 'camera-halv3-adapter'
    timeout = 600
    media_profiles_path = os.path.join('vendor', 'etc', 'media_profiles.xml')
    tablet_board_list = ['scarlet']

    def setup(self):
        """
        Run common setup steps.
        """
        self.dep_dir = os.path.join(self.autodir, 'deps', self.dep)
        self.job.setup_dep([self.dep])
        logging.debug('mydep is at %s' % self.dep_dir)

    def run_once(self):
        """
        Entry point of this test.
        """
        self.job.install_pkg(self.dep, 'dep', self.dep_dir)

        with service_stopper.ServiceStopper([self.adapter_service]):
            cmd = [ os.path.join(self.dep_dir, 'bin', self.test_binary) ]
            xml_content = utils.system_output(
                ' '.join(['android-sh', '-c', '\"cat',
                          self.media_profiles_path + '\"']))
            root = xml.etree.ElementTree.fromstring(xml_content)
            recording_params = Set()
            for camcorder_profiles in root.findall('CamcorderProfiles'):
                for encoder_profile in camcorder_profiles.findall('EncoderProfile'):
                    video = encoder_profile.find('Video')
                    recording_params.add('%s:%s:%s:%s' % (
                        camcorder_profiles.get('cameraId'), video.get('width'),
                        video.get('height'), video.get('frameRate')))
            if recording_params:
                cmd.append('--recording_params=' + ','.join(recording_params))
            if utils.get_current_board() in self.tablet_board_list:
                cmd.append('--gtest_filter=-*SensorOrientationTest/*')

            utils.system(' '.join(cmd), timeout=self.timeout)
