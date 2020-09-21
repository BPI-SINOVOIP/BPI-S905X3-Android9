# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Power cycle a usb port on DUT(device under test)."""

from __future__ import print_function

import logging
import os
import time

TOKEN_NEW_BUS = '/:  '
TOKEN_ROOT_DEVICE = '\n    |__ '

# On board guado, there are three gpios that control usb port power:
# Front left usb port:  218, port number: 2
# Front right usb port: 219, port number: 3
# Rear dual usb ports:  209, port number: 5,6
PORT_NUM_DICT = {
    'guado': {
        'bus1': {
            2: 'front_left',
            3: 'front_right',
            5: 'back_dual',
            6: 'back_dual'
        }
    }
}
PORT_GPIO_DICT = {
    'guado': {
        'bus1': {
            'front_left': 218,
            'front_right': 219,
            'back_dual': 209
        }
    }
}


def power_cycle_usb_gpio(dut, gpio_idx, pause=1):
    """
    Power cycle a usb port on dut via its gpio index.

    Each usb port has corresponding gpio controling its power. If the gpio
    index of the gpio is known, the power cycling procedure is pretty
    straightforward.

    @param dut: The handle of the device under test. Should be initialized in
            autotest.
    @param gpio_idx: The index of the gpio that controls power of the usb port
            we want to reset.
    @param pause: The waiting time before powering on usb device, unit is second.

    """
    if gpio_idx is None:
        return
    export_flag = False
    if not dut.path_exists('/sys/class/gpio/gpio{}'.format(gpio_idx)):
        export_flag = True
        cmd = 'echo {} > /sys/class/gpio/export'.format(gpio_idx)
        dut.run(cmd)
    cmd = 'echo out > /sys/class/gpio/gpio{}/direction'.format(gpio_idx)
    dut.run(cmd)
    cmd = 'echo 0 > /sys/class/gpio/gpio{}/value'.format(gpio_idx)
    dut.run(cmd)
    time.sleep(pause)
    cmd = 'echo 1 > /sys/class/gpio/gpio{}/value'.format(gpio_idx)
    dut.run(cmd)
    if export_flag:
        cmd = 'echo {} > /sys/class/gpio/unexport'.format(gpio_idx)
        dut.run(cmd)


def power_cycle_usb_vidpid(dut, board, vid, pid):
    """
    Power cycle a usb port on DUT via peripharel's VID and PID.

    When only the VID and PID of the peripharel is known, a search is needed
    to decide which port it connects to by its VID and PID and look up the gpio
    index according to the board and port number in the dictionary. Then the
    USB port is power cycled using the gpio number.

    @param dut: The handle of the device under test.
    @param board: Board name ('guado', etc.)
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @raise KeyError if the target device wasn't found by given VID and PID.

    """
    bus_idx, port_idx = get_port_number_from_vidpid(dut, vid, pid)
    if port_idx is None:
        raise KeyError('Couldn\'t find target device, {}:{}.'.format(vid, pid))
    logging.info('found device bus {} port {}'.format(bus_idx, port_idx))
    token_bus = 'bus{}'.format(bus_idx)
    target_gpio_pos = (PORT_NUM_DICT.get(board, {})
                       .get(token_bus, {}).get(port_idx, ''))
    target_gpio = (PORT_GPIO_DICT.get(board, {})
                   .get(token_bus, {}).get(target_gpio_pos, None))
    logging.info('target gpio num {}'.format(target_gpio))
    power_cycle_usb_gpio(dut, target_gpio)


def get_port_number_from_vidpid(dut, vid, pid):
    """
    Get bus number and port number a device is connected to on DUT.

    Get the bus number and port number of the usb port the target perpipharel
    device is connected to.

    @param dut: The handle of the device under test.
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @returns the target bus number and port number, if device not found, returns
          (None, None).

    """
    cmd = 'lsusb -d {}:{}'.format(vid, pid)
    lsusb_output = dut.run(cmd, ignore_status=True).stdout
    logging.info('lsusb output {}'.format(lsusb_output))
    target_bus_idx, target_dev_idx = get_bus_dev_id(lsusb_output, vid, pid)
    if target_bus_idx is None:
        return None, None
    cmd = 'lsusb -t'
    lsusb_output = dut.run(cmd, ignore_status=True).stdout
    target_port_number = get_port_number(
        lsusb_output, target_bus_idx, target_dev_idx)
    return target_bus_idx, target_port_number


def get_bus_dev_id(lsusb_output, vid, pid):
    """
    Get bus number and device index a device is connected to on DUT.

    Get the bus number and port number of the usb port the target perpipharel
    device is connected to based on the output of command 'lsusb -d VID:PID'.

    @param lsusb_output: output of command 'lsusb -d VID:PID' running on DUT.
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @returns the target bus number and device index, if device not found,
          returns (None, None).

    """
    if lsusb_output == '':
        return None, None
    lsusb_device_info = lsusb_output.strip().split('\n')
    if len(lsusb_device_info) > 1:
        logging.info('find more than one device with VID:PID: %s:%s', vid, pid)
        return None, None
    # An example of the info line is 'Bus 001 Device 006:  ID 266e:0110 ...'
    fields = lsusb_device_info[0].split(' ')
    assert len(fields) >= 6, 'Wrong info format: {}'.format(lsusb_device_info)
    target_bus_idx = int(fields[1])
    target_device_idx = int(fields[3][:-1])
    logging.info('found target device %s:%s, bus: %d, dev: %d',
                 vid, pid, target_bus_idx, target_device_idx)
    return target_bus_idx, target_device_idx

def get_port_number(lsusb_tree_output, bus, dev):
    """
    Get port number that certain device is connected to on DUT.

    Get the port number of the usb port that the target peripharel device is
    connected to based on the output of command 'lsusb -t', its bus number and
    device index.
    An example of lsusb_tree_output could be:
    /:  Bus 02.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/4p, 5000M
        |__ Port 2: Dev 2, If 0, Class=Hub, Driver=hub/4p, 5000M
    /:  Bus 01.Port 1: Dev 1, Class=root_hub, Driver=xhci_hcd/11p, 480M
        |__ Port 2: Dev 52, If 0, Class=Hub, Driver=hub/4p, 480M
            |__ Port 1: Dev 55, If 0, Class=Human Interface Device,
                        Driver=usbhid, 12M
            |__ Port 3: Dev 54, If 0, Class=Vendor Specific Class,
                        Driver=udl, 480M
        |__ Port 3: Dev 3, If 0, Class=Hub, Driver=hub/4p, 480M
        |__ Port 4: Dev 4, If 0, Class=Wireless, Driver=btusb, 12M
        |__ Port 4: Dev 4, If 1, Class=Wireless, Driver=btusb, 12M

    @param lsusb_tree_output: The output of command 'lsusb -t' on DUT.
    @param bus: The bus number the peripharel device is connected to.
    @param dev: The device index of the peripharel device on DUT.

    @returns the target port number, if device not found, returns None.

    """
    lsusb_device_buses = lsusb_tree_output.strip().split(TOKEN_NEW_BUS)
    target_bus_token = 'Bus {:02d}.'.format(bus)
    for bus_info in lsusb_device_buses:
        if bus_info.find(target_bus_token) != 0:
            continue
        target_dev_token = 'Dev {}'.format(dev)
        device_info = bus_info.strip(target_bus_token).split(TOKEN_ROOT_DEVICE)
        for device in device_info:
            if target_dev_token not in device:
                continue
            target_port_number = int(device.split(':')[0].split(' ')[1])
            return target_port_number
    return None


def get_all_port_number_from_vidpid(dut, vid, pid):
    """
    Get the list of bus number and port number devices are connected to DUT.

    Get the the list of bus number and port number of the usb ports the target
           perpipharel devices are connected to.

    @param dut: The handle of the device under test.
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @returns the list of target bus number and port number, if device not found,
            returns empty list.

    """
    port_number = []
    cmd = 'lsusb -d {}:{}'.format(vid, pid)
    lsusb_output = dut.run(cmd, ignore_status=True).stdout
    (target_bus_idx, target_dev_idx) = get_all_bus_dev_id(lsusb_output, vid, pid)
    if target_bus_idx is None:
        return None, None
    cmd = 'lsusb -t'
    lsusb_output = dut.run(cmd, ignore_status=True).stdout
    for bus, dev in zip(target_bus_idx, target_dev_idx):
        port_number.append(get_port_number(
            lsusb_output, bus, dev))
    return (target_bus_idx, port_number)


def get_all_bus_dev_id(lsusb_output, vid, pid):
    """
    Get the list of bus number and device index devices are connected to DUT.

    Get the bus number and port number of the usb ports the target perpipharel
            devices are connected to based on the output of command 'lsusb -d VID:PID'.

    @param lsusb_output: output of command 'lsusb -d VID:PID' running on DUT.
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @returns the list of target bus number and device index, if device not found,
           returns empty list.

    """
    bus_idx = []
    device_idx =[]
    if lsusb_output == '':
        return None, None
    lsusb_device_info = lsusb_output.strip().split('\n')
    for lsusb_device in lsusb_device_info:
        fields = lsusb_device.split(' ')
        assert len(fields) >= 6, 'Wrong info format: {}'.format(lsusb_device_info)
        target_bus_idx = int(fields[1])
        target_device_idx = int(fields[3][:-1])
        bus_idx.append(target_bus_idx)
        device_idx.append( target_device_idx)
    return (bus_idx, device_idx)


def get_target_all_gpio(dut, board, vid, pid):
    """
    Get GPIO for all devices with vid, pid connected to on DUT.

    Get gpio of usb port the target perpipharel  devices are
    connected to based on the output of command 'lsusb -d VID:PID'.

    @param dut: The handle of the device under test.
    @param board: Board name ('guado', etc.)
    @param vid: Vendor ID of the peripharel device.
    @param pid: Product ID of the peripharel device.

    @returns the list of gpio, if no device found return []

    """
    gpio_list = []
    (bus_idx, port_idx) = get_all_port_number_from_vidpid(dut, vid, pid)
    if port_idx is None:
        raise KeyError('Couldn\'t find target device, {}:{}.'.format(vid, pid))

    for bus, port in zip(bus_idx, port_idx):
        logging.info('found device bus {} port {}'.format(bus, port))
        token_bus = 'bus{}'.format(bus)
        target_gpio_pos = (PORT_NUM_DICT.get(board, {})
                       .get(token_bus, {}).get(port, ''))
        target_gpio = (PORT_GPIO_DICT.get(board, {})
                   .get(token_bus, {}).get(target_gpio_pos, None))
        logging.info('Target gpio num {}'.format(target_gpio))
        gpio_list.append(target_gpio)
    return gpio_list
