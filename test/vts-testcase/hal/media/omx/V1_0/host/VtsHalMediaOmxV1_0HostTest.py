#!/usr/bin/env python
#
# Copyright (C) 2017 The Android Open Source Project
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

import copy
import logging
import time

from vts.runners.host import asserts
from vts.runners.host import keys
from vts.runners.host import test_runner
from vts.testcases.template.hal_hidl_gtest import hal_hidl_gtest


class VtsHalMediaOmxV1_0Host(hal_hidl_gtest.HidlHalGTest):
    """Host test class to run the Media_Omx HAL."""

    COMPONENT_TEST = "ComponentHidlTest"
    AUDIO_ENC_TEST = "AudioEncHidlTest"
    AUDIO_DEC_TEST = "AudioDecHidlTest"
    VIDEO_ENC_TEST = "VideoEncHidlTest"
    VIDEO_DEC_TEST = "VideoDecHidlTest"

    # Roles we want to test.
    whitelist_roles = ["audio_encoder.aac",
                       "audio_encoder.amrnb",
                       "audio_encoder.amrwb",
                       "audio_encoder.flac",
                       "audio_decoder.aac",
                       "audio_decoder.amrnb",
                       "audio_decoder.amrwb",
                       "audio_decoder.flac",
                       "audio_decoder.g711alaw",
                       "audio_decoder.g711mlaw",
                       "audio_decoder.gsm",
                       "audio_decoder.mp3",
                       "audio_decoder.opus",
                       "audio_decoder.raw",
                       "audio_decoder.vorbis",
                       "video_encoder.avc",
                       "video_encoder.h263",
                       "video_encoder.mpeg4",
                       "video_encoder.vp8",
                       "video_encoder.vp9",
                       "video_decoder.avc",
                       "video_decoder.h263",
                       "video_decoder.hevc",
                       "video_decoder.mpeg4",
                       "video_decoder.vp8",
                       "video_decoder.vp9"]

    def CreateTestCases(self):
        """Get all registered test components and create test case objects."""
        # Init the IOmx hal.
        self._dut.hal.InitHidlHal(
            target_type="media_omx",
            target_basepaths=self._dut.libPaths,
            target_version=1.0,
            target_package="android.hardware.media.omx",
            target_component_name="IOmx",
            bits=64 if self._dut.is64Bit else 32)

        # Call listNodes to get all registered components.
        self.vtypes = self._dut.hal.media_omx.GetHidlTypeInterface("types")
        status, nodeList = self._dut.hal.media_omx.listNodes()
        asserts.assertEqual(self.vtypes.Status.OK, status)

        self.components = {}
        for node in nodeList:
            self.components[node['mName']] = node['mRoles']

        super(VtsHalMediaOmxV1_0Host, self).CreateTestCases()

    # @Override
    def CreateTestCase(self, path, tag=''):
        """Create a list of VtsHalMediaOmxV1_0testCase objects.

        For each target side gtest test case, create a set of new test cases
        argumented with different component and role values.

        Args:
            path: string, absolute path of a gtest binary on device
            tag: string, a tag that will be appended to the end of test name

        Returns:
            A list of VtsHalMediaOmxV1_0TestCase objects
        """
        gtest_cases = super(VtsHalMediaOmxV1_0Host, self).CreateTestCase(path,
                                                                         tag)
        test_cases = []

        for gtest_case in gtest_cases:
            test_suite = gtest_case.full_name
            for component, roles in self.components.iteritems():
                for role in roles:
                    if not self.COMPONENT_TEST in test_suite and not role in self.whitelist_roles:
                        continue
                    if self.AUDIO_ENC_TEST in test_suite and not "audio_encoder" in role:
                        continue
                    if self.AUDIO_DEC_TEST in test_suite and not "audio_decoder" in role:
                        continue
                    if self.VIDEO_ENC_TEST in test_suite and not "video_encoder" in role:
                        continue
                    if self.VIDEO_DEC_TEST in test_suite and not "video_decoder" in role:
                        continue

                    test_case = copy.copy(gtest_case)
                    test_case.args += " -C " + component
                    test_case.args += " -R " + role
                    test_case.name_appendix = '_' + component + '_' + role + test_case.name_appendix
                    test_cases.append(test_case)
        logging.info("num of test_testcases: %s", len(test_cases))
        return test_cases


if __name__ == "__main__":
    test_runner.main()
