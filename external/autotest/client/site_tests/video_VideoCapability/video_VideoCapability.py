# Copyright 2017 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import os
import subprocess


from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.video import device_capability

class video_VideoCapability(test.test):
    """
    This tests the video static capabilities settings
    with label detector: "avtest_label_detect."
    """
    version = 1
    avtest_label_to_capability = {
        'hw_video_acc_h264'    : 'hw_dec_h264_1080_30',
        'hw_video_acc_vp8'     : 'hw_dec_vp8_1080_30',
        'hw_video_acc_vp9'     : 'hw_dec_vp9_1080_30',
        'hw_video_acc_vp9_2'   : 'hw_dec_vp9-2_1080_30',
        'hw_jpeg_acc_dec'      : 'hw_dec_jpeg',
        'hw_jpeg_acc_enc'      : 'hw_enc_jpeg',
        'hw_video_acc_enc_h264': 'hw_enc_h264_1080_30',
        'hw_video_acc_enc_vp8' : 'hw_enc_vp8_1080_30',
        'hw_video_acc_enc_vp9' : 'hw_enc_vp9_1080_30',
        'webcam'               : 'usb_camera',
    }


    def compare_avtest_label_detect(self, dc_results):
        avtest_label = subprocess.check_output(['avtest_label_detect']).strip()
        logging.debug("avtest_label_detect result\n%s", avtest_label)
        test_failure = False
        avtest_detected_labels = set()
        for line in avtest_label.splitlines():
            label = line.split(':')[1].strip()
            if label in video_VideoCapability.avtest_label_to_capability:
                cap = video_VideoCapability.avtest_label_to_capability[label]
                avtest_detected_labels.add(cap)
        for cap in video_VideoCapability.avtest_label_to_capability.values():
            if dc_results[cap] == 'yes' and cap not in avtest_detected_labels:
                logging.error('Static capability claims %s is available. '
                              "But avtest_label_detect doesn't detect", cap)
                test_failure = True

            if dc_results[cap] == 'no' and cap in avtest_detected_labels:
                logging.error("Static capability claims %s isn't available. "
                              'But avtest_label_detect detects', cap)
                test_failure = True

        if test_failure:
            raise error.TestFail("Dynamic capability detection results did not "
                                 "match static capability configuration.")


    def run_once(self):
        logging.debug('Starting video_VideoCapability')
        dc = device_capability.DeviceCapability()
        dc_results = {}
        for cap in dc.get_managed_caps():
            ret = dc.get_capability(cap)
            dc_results[cap] = ret

        self.compare_avtest_label_detect(dc_results)
