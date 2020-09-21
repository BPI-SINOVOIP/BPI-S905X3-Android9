# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import glob
import os
import logging
import subprocess

def chrome_vmodule_flag():
    """Return vmodule flag for chrome to enable VDAs/VEAs/JDAs/V4L2IP logs"""
    logging_patterns = ['*/media/gpu/*video_decode_accelerator.cc=2',
                        '*/media/gpu/*video_encode_accelerator.cc=2',
                        '*/media/gpu/*jpeg_decode_accelerator.cc=2',
                        '*/media/gpu/v4l2_image_processor.cc=2']
    chrome_video_vmodule_flag = '--vmodule=' + ','.join(logging_patterns)
    logging.info('chrome video vmodule flag: %s', chrome_video_vmodule_flag)
    return chrome_video_vmodule_flag


def video_log_wrapper(func):
    """
    Return decorator that make verbose video logs enable
    before test and make them disable after completing test.

    @param func: function, the test function, e.g., run_once
    @returns decorator function
    """
    vlog = VideoLog()

    #videobuf2 log
    files = glob.glob('/sys/module/videobuf2_*/parameters/debug')
    vlog.add_log(files,
                 ['1'] * len(files),
                 ['0'] * len(files),
                 'videobuf2 log')

    #s5p_mfc log
    fpath = '/sys/module/s5p_mfc/parameters/debug'
    if os.path.exists(fpath):
        vlog.add_log([fpath],
                     ['1'],
                     ['0'],
                     's5p_mfc log')

    #rk3399 log
    #rk3399 debug level is controlled by bits.
    #Here, 3 means to enable log level 0 and 1.
    fpath = '/sys/module/rockchip_vpu/parameters/debug'
    if os.path.exists(fpath):
        vlog.add_log([fpath],
                     ['3'],
                     ['0'],
                     'rk3399 log')

    #rk3288 log
    #rk3288 debug level is controlled by bits.
    #Here, 3 means to enable log level 0 and 1.
    fpath = '/sys/module/rk3288_vpu/parameters/debug'
    if os.path.exists(fpath):
        vlog.add_log([fpath],
                     ['3'],
                     ['0'],
                     'rk3288 log')

    #go2001 log
    fpath = '/sys/module/go2001/parameters/go2001_debug_level'
    if os.path.exists(fpath):
        vlog.add_log([fpath],
                     ['1'],
                     ['0'],
                     'go2001 log')

    def call(*args, **kwargs):
        """
        Enable logs before the test, execute the test and disable logs
        after the test.

        In any case, it is guranteed to disable logs.
        """
        with vlog:
            return func(*args, **kwargs)

    return call


def cmdexec_with_log(cmd, info_message):
    """
    Execute command, logging infomation message.

    @param cmd: string, command to be executed
    @param info_message: string, the messages to be shown in log message
    """
    try:
        logging.info('%s : %s', info_message, cmd)
        subprocess.check_call(cmd, shell=True)
    except subprocess.CalledProcessError:
        logging.warning('Fail to execute command [%s] : %s',
                        info_message, cmd)


class VideoLog:
    """
    Enable/Disable video logs.
    """
    def __init__(self):
        self.logs = []

    def add_log(self, files, enable_values, disable_values, log_name):
        """
        Add new log

        @param files: list of string, file paths
        @param enable_values: list of string, the list of value
                              to write to each file path for enabling
        @param disable_values: list of string, the list of value
                               to write to each file path for disabling
        @param log_name: string, name to be shown in log message
        """
        self.logs.append({'files': files,
                          'enable values': enable_values,
                          'disable values': disable_values,
                          'log name': log_name})

    def __enter__(self):
        """Enable logs"""
        for log in self.logs:
            log_name = log['log name']
            for f, ev in zip(log['files'], log['enable values']):
                cmdexec_with_log('echo %s > %s' % (ev, f),
                                 '%s enable' % log_name)

    def __exit__(self, exception_type, exception_value, traceback):
        """Disable logs"""
        for log in self.logs:
            log_name = log['log name']
            for f, dv in zip(log['files'], log['disable values']):
                cmdexec_with_log('echo %s > %s' % (dv, f),
                                 '%s disable' % log_name)
