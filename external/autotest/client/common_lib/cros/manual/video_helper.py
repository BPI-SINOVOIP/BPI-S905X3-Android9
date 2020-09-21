# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Get v4l2 interface, and chrome processes which access the video interface"""

from __future__ import print_function

import logging


LSOF_CHROME_VIDEO = {
                    '2bd9:0011': 3,
                    '046d:0843': 2,
                    '046d:082d': 2,
                    '046d:0853': 2,
                    '064e:9405': 2,
                    '046d:0853': 2
                    }


def get_video_by_name(dut, name):
    """
    Get v4l2 interface based on WebCamera
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @param name: The name of web camera
                 For example: 'Huddly GO'
                              'Logitech Webcam C930e'
    @returns:  if video device v4l2 found, return True,
               else return False.
    """
    cmd = 'ls /sys/class/video4linux/'
    video4linux_list = dut.run(cmd, ignore_status=True).stdout.split()
    for video_dev in video4linux_list:
        cmd = 'cat /sys/class/video4linux/{}/name'.format(video_dev)
        video_dev_name = dut.run(cmd, ignore_status=True).stdout.strip()
        logging.info('---%s', cmd)
        logging.info('---%s', video_dev_name)
        if name in video_dev_name and not 'overview' in video_dev_name:
            logging.info('---found interface for %s', name)
            return video_dev
    return None


def get_lsof4_video(dut, video):
    """
    Get output of chrome processes which attach to video device.
    @param dut: The handle of the device under test. Should be initialized in
                autotest.
    @param video: video device name for camera.
    @returns: output of lsof /dev/videox.
    """
    cmd = 'lsof /dev/{} | grep chrome'.format(video)
    lsof_output = dut.run(cmd, ignore_status=True).stdout.strip().split('\n')
    logging.info('---%s', cmd)
    logging.info('---%s', lsof_output)
    return lsof_output


def get_video_streams(dut, name):
    """
    Get output of chrome processes which attach to video device.
    @param dut: The handle of the device under test.
    @param name: name of camera.
    @returns: output of lsof for v4l2 device.
    """
    video_dev = get_video_by_name(dut, name)
    lsof_output = get_lsof4_video(dut, video_dev)
    return lsof_output


def check_v4l2_interface(dut, vidpid, camera):
    """
    Check v4l2 interface exists for camera.
    @param dut: The handle of the device under test.
    @param vidpid: vidpid of camera.
    @param camera: name of camera
    @returns: True if v4l2 interface found for camera,
              False if not found.
    """
    logging.info('---check v4l2 interface for %s', camera)
    if get_video_by_name(dut, camera):
        return True, None
    return False, '{} have no v4l2 interface.'.format(camera)


def check_video_stream(dut, is_muted, vidpid, camera):
    """
    Check camera is streaming as expected.
    @param dut: The handle of the device under test.
    @is_streaming: True if camera is expected to be streaming,
                   False if not.
    @param vidpid: vidpid of camera
    @param camera: name of camera.
    @returns: True if camera is streaming or not based on
              expectation,
              False, errMsg if not found.
    """
    process_camera = get_video_streams(dut, camera)
    if is_muted:
        if len(process_camera) >= LSOF_CHROME_VIDEO[vidpid]:
            return False, '{} fails to stop video streaming.'.format(camera)
    else:
        if not len(process_camera) >= LSOF_CHROME_VIDEO[vidpid]:
            return False, '{} fails to start video streaming.'.format(camera)
    return True, None
