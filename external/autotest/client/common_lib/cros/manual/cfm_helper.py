# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Check USB device by running linux command on CfM"""

from __future__ import print_function

import logging
import re
import time
from autotest_lib.client.common_lib.cros.manual import get_usb_devices
from autotest_lib.client.common_lib.cros import power_cycle_usb_util

CORE_DIR_LINES = 3
ATRUS = '18d1:8001'

def check_chrome_logfile(dut):
    """
    Get the latest chrome log file.
    @param dut: The handle of the device under test.
    @returns: the latest chrome log file
    """
    output = None
    logging.info('---Get the latest chrome log file')
    cmd = 'ls -latr /var/log/chrome/chrome'
    try:
        output = dut.run(cmd, ignore_status=True).stdout
    except Exception as e:
        logging.exception('Fail to run command %s.', cmd)
        return None
    logging.info('---cmd: %s', cmd)
    logging.info('---output: %s', output.lower().strip())
    return output

def check_last_reboot(dut):
    """
    Get the last line of eventlog.txt
    @param dut: The handle of the device under test.
    @returns: the last line in eventlog.txt
    """
    output = None
    cmd = 'tail -1 /var/log/eventlog.txt'
    logging.info('---Get the latest reboot log')
    try:
        output = dut.run(cmd, ignore_status=True).stdout
    except Exception as e:
        logging.exception('Fail to run command %s.', cmd)
        return None
    logging.info('---cmd: %s', cmd)
    logging.info('---output: %s', output.lower().strip())
    return output

def check_is_platform(dut, name):
    """
    Check whether CfM is expected platform.
    @param dut: The handle of the device under test.
    @param name: The name of platform
    @returns: True, if CfM's platform is same as expected.
              False, if not.
    """
    cmd = ("cat /var/log/platform_info.txt | grep name | "
           "awk -v N=3 \'{print $N}\'")
    output = dut.run(cmd, ignore_status=True).stdout.split()[0]
    logging.info('---cmd: %s', cmd)
    logging.info('---output: %s', output.lower())
    return output.lower() == name


def get_mgmt_ipv4(dut):
    """
    Get mgmt ipv4 address
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @return: ipv4 address for mgmt interface.
    """
    cmd = 'ifconfig -a | grep eth0 -A 2 | grep netmask'
    try:
        output = dut.run(cmd, ignore_status=True).stdout
    except Exception as e:
        logging.exception('Fail to run command %s.', cmd)
        return None
    ipv4 = re.findall(r"inet\s*([0-9.]+)\s*netmask.*", output)[0]
    return ipv4


def retrieve_usb_devices(dut):
    """
    Populate output of usb-devices on CfM.
    @param dut: handle of CfM under test
    @returns dict of all usb devices detected on CfM.
    """
    usb_devices = (dut.run('usb-devices', ignore_status=True).
                   stdout.strip().split('\n\n'))
    usb_data = get_usb_devices.extract_usb_data(
               '\nUSB-Device\n'+'\nUSB-Device\n'.join(usb_devices))
    return usb_data


def extract_peripherals_for_cfm(usb_data):
    """
    Check CfM has camera, speaker and Mimo connected.
    @param usb_data: dict extracted from output of "usb-devices"
    """
    peripheral_map = {}

    speaker_list = get_usb_devices.get_speakers(usb_data)
    camera_list = get_usb_devices.get_cameras(usb_data)
    display_list = get_usb_devices.get_display_mimo(usb_data)
    controller_list = get_usb_devices.get_controller_mimo(usb_data)
    for pid_vid, device_count in speaker_list.iteritems():
        if device_count > 0:
            peripheral_map[pid_vid] = device_count

    for pid_vid, device_count in camera_list.iteritems():
        if device_count > 0:
            peripheral_map[pid_vid] = device_count

    for pid_vid, device_count in controller_list.iteritems():
        if device_count > 0:
            peripheral_map[pid_vid] = device_count

    for pid_vid, device_count in display_list.iteritems():
        if device_count > 0:
            peripheral_map[pid_vid] = device_count

    for pid_vid, device_count in peripheral_map.iteritems():
        logging.info('---device: %s (%s), count: %d',
                     pid_vid, get_usb_devices.get_device_prod(pid_vid),
                     device_count)

    return peripheral_map


def check_peripherals_for_cfm(peripheral_map):
    """
    Check CfM has one and only one camera,
    one and only one speaker,
    or one and only one mimo.
    @param peripheral_map: dict for connected camera, speaker, or mimo.
    @returns: True if check passes,
              False if check fails.
    """
    peripherals = peripheral_map.keys()

    type_camera = set(peripherals).intersection(get_usb_devices.CAMERA_LIST)
    type_speaker = set(peripherals).intersection(get_usb_devices.SPEAKER_LIST)
    type_controller = set(peripherals).intersection(\
                      get_usb_devices.TOUCH_CONTROLLER_LIST)
    type_panel = set(peripherals).intersection(\
                 get_usb_devices.TOUCH_DISPLAY_LIST)

    # check CfM have one, and only one type camera, huddly and mimo
    if len(type_camera) == 0:
        logging.info('No camera is found on CfM.')
        return False

    if not len(type_camera) == 1:
        logging.info('More than one type of cameras are found on CfM.')
        return False

    if len(type_speaker) == 0:
        logging.info('No speaker is found on CfM.')
        return False

    if not len(type_speaker) == 1:
        logging.info('More than one type of speakers are found on CfM.')
        return False

    if len(type_controller) == 0:
       logging.info('No controller is found on CfM.')
       return False


    if not len(type_controller) == 1:
        logging.info('More than one type of controller are found on CfM.')
        return False

    if len(type_panel) == 0:
        logging.info('No Display is found on CfM.')
        return False

    if not len(type_panel) == 1:
        logging.info('More than one type of displays are found on CfM.')
        return False

    # check CfM have only one camera, huddly and mimo
    for pid_vid, device_count in peripheral_map.iteritems():
        if device_count > 1:
            logging.info('Number of device %s connected to CfM : %d',
                         get_usb_devices.get_device_prod(pid_vid),
                         device_count)
            return False

    return True


def check_usb_enumeration(dut, puts):
    """
    Check USB enumeration for devices
    @param dut: the handle of CfM under test
    @param puts: the list of peripherals under test
    @returns True, none if test passes
             False, errMsg if test test fails
    """
    usb_data = retrieve_usb_devices(dut)
    if not usb_data:
        logging.warning('No usb devices found on DUT')
        return False, 'No usb device found on DUT.'
    else:
        usb_device_list = extract_peripherals_for_cfm(usb_data)
        logging.info('---usb device = %s', usb_device_list)
        if not set(puts).issubset(set(usb_device_list.keys())):
            logging.info('Detect device fails for usb enumeration')
            logging.info('Expect enumerated devices: %s', puts)
            logging.info('Actual enumerated devices: %s',
                         usb_device_list.keys())
            return False, 'Some usb devices are not found.'
        return True, None


def check_usb_interface_initializion(dut, puts):
    """
    Check CfM shows valid interface for all peripherals connected.
    @param dut: the handle of CfM under test
    @param puts: the list of peripherals under test
    @returns True, none if test passes
             False, errMsg if test test fails
    """
    usb_data = retrieve_usb_devices(dut)
    for put in puts:
        number, health = get_usb_devices.is_usb_device_ok(usb_data, put)
        logging.info('---device interface = %d, %s for %s',
                     number, health,  get_usb_devices.get_device_prod(put))
        if '0' in health:
            logging.warning('Device %s has invalid interface', put)
            return False, 'Device {} has invalid interface.'.format(put)
    return True, None


def clear_core_file(dut):
    """clear core files"""
    cmd = "rm -rf /var/spool/crash/*.*"
    try:
        dut.run_output(cmd)
    except Exception as e:
        logging.exception('Fail to clean core files under '
                     '/var/spool/crash')
        logging.exception('Fail to execute %s :', cmd)


def check_process_crash(dut, cdlines):
    """Check whether there is core file."""
    cmd = 'ls -latr /var/spool/crash'
    try:
        core_files_output = dut.run_output(cmd).splitlines()
    except Exception as e:
        logging.exception('Can not find file under /var/spool/crash.')
        logging.exception('Fail to execute %s:', cmd)
        return True,  CORE_DIR_LINES
    logging.info('---%s\n---%s', cmd, core_files_output)
    if len(core_files_output) - cdlines <= 0:
        logging.info('---length of files: %d', len(core_files_output))
        return True, len(core_files_output)
    else:
        return False, len(core_files_output)


def gpio_usb_test(dut, gpio_list, device_list, pause, board):
    """
    Run GPIO test to powercycle usb port.
    @parama dut: handler of CfM,
    @param gpio_list: the list of gpio ports,
    @param device_list: the list of usb devices,
    @param pause: time needs to wait before restoring power to usb port,
                  in seconds
    @param board: board name for CfM
    @returns True
    """
    if not gpio_list:
       gpio_list = []
       for device in device_list:
           vid, pid  = device.split(':')
           logging.info('---check gpio for device %s:%s', vid, pid)
           try:
               ports = power_cycle_usb_util.get_target_all_gpio(dut, board,
                                                            vid , pid)
           except Exception as e:
               errmsg = 'Fail to get gpio port.'
               logging.exception('%s.', errmsg)
               return False, errmsg
           [gpio_list.append(_port) for _port in ports]

    for port in gpio_list:
        if not port:
            continue
        logging.info('+++powercycle gpio port %s', port)
        try:
            power_cycle_usb_util.power_cycle_usb_gpio(dut, port, pause)
        except Exception as e:
            errmsg = 'Fail to powercycle gpio port.'
            logging.exception('%s.', errmsg)
            return False, errmsg
    return True, None


def reboot_test(dut, pause):
    """
    Reboot CfM.
    @parama dut: handler of CfM,
    @param pause: time needs to wait after issuing reboot command, in seconds,

    """
    try:
        dut.reboot()
    except Exception as e:
        logging.exception('Fail to reboot CfM.')
        return False
    logging.info('---reboot done')
    time.sleep(pause)
    return True



def find_last_log(dut, speaker):
    """
    Get the lastlast_lines line for log files.
    @param dut: handler of CfM
    @param speaker: vidpid if speaker.
    @returns: the list of string of the last line of logs.
    """
    last_lines = {
              'messages':[],
              'chrome':[],
              'ui': [],
              'atrus': []
                  }
    logging.debug('Get the last line of log file, speaker %s', speaker)
    try:
        cmd = "tail -1 /var/log/messages | awk -v N=1 '{print $N}'"
        last_lines['messages'] = dut.run_output(cmd).strip().split()[0]
        cmd = "tail -1 /var/log/chrome/chrome | awk -v N=1 '{print $N}'"
        last_lines['chrome'] = dut.run_output(cmd).strip().split()[0]
        cmd = "tail -1 /var/log/ui/ui.LATEST | awk -v N=1 '{print $N}'"
        last_lines['ui']= dut.run_output(cmd)
        if speaker == ATRUS and check_is_platform(dut, 'guado'):
            logging.info('---atrus speaker %s connected to CfM', speaker)
            cmd = 'tail -1 /var/log/atrus.log | awk -v N=1 "{print $N}"'
            last_lines['atrus'] = dut.run_output(cmd).strip().split()[0]
    except Exception as e:
        logging.exception('Fail to get the last line from log files.')
    for item, timestamp in last_lines.iteritems():
        logging.debug('---%s: %s', item, timestamp)
    return last_lines


def collect_log_since_last_check(dut, lastlines, logfile):
    """Collect log file since last check."""
    output = None
    if logfile == "messages":
        cmd ='awk \'/{}/,0\' /var/log/messages'.format(lastlines[logfile])
    if logfile == "chrome":
        cmd ='awk \'/{}/,0\' /var/log/chrome/chrome'.format(lastlines[logfile])
    if logfile == "ui":
        cmd ='awk \'/{}/,0\' /var/log/ui/ui.LATEST'.format(lastlines[logfile])
    if logfile == 'atrus':
         cmd ='awk \'/{}/,0\' /var/log/atrus.log'.format(lastlines[logfile])
    logging.info('---cmd = %s', cmd)
    try:
        output =  dut.run_output(cmd).split('\n')
    except Exception as e:
        logging.exception('Fail to get output from log files.')
    logging.info('---length of log: %d', len(output))
    if not output:
        logging.info('--fail to find match log, check the latest log.')

    if not output:
        if logfile == "messages":
            cmd ='cat /var/log/messages'
        if logfile == "chrome":
            cmd ='cat /var/log/chrome/chrome'
        if logfile == "ui":
            cmd ='cat /var/log/ui/ui.LATEST'
        if logfile == 'atrus':
            cmd ='cat /var/log/atrus.log'
        output =  dut.run_output(cmd).split('\n')
        logging.info('---length of log: %d', len(output))
    return output

def check_log(dut, timestamp, error_list, checkitem, logfile):
    """
    Check logfile does not contain any element in error_list[checkitem].
    """
    error_log_list = []
    logging.info('---now check log %s in file %s', checkitem, logfile)
    output = collect_log_since_last_check(dut, timestamp, logfile)
    for _error in error_list[checkitem]:
         error_log_list.extend([s for s in output if _error in str(s)])
    if not error_log_list:
        return True, None
    else:
        tempmsg = '\n'.join(error_log_list)
        errmsg = 'Error_Found:in_log_file:{}:{}.'.format(logfile, tempmsg)
        logging.info('---%s', errmsg)
        return False, errmsg
