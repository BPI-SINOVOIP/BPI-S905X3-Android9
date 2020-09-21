# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides functions to record input events."""

import logging
import re
import select
import subprocess
import threading
import time

from linux_input import EV_MSC, EV_SYN, MSC_SCAN, SYN_REPORT


# Define extra misc events below as they are not defined in linux_input.
MSC_SCAN_BTN_LEFT = 90001
MSC_SCAN_BTN_RIGHT = 90002
MSC_SCAN_BTN_MIDDLE = 90003


class InputEventRecorderError(Exception):
    """An exception class for input_event_recorder module."""
    pass


class Event(object):
    """An event class based on evtest constructed from an evtest event.

    An ordinary event looks like:
    Event: time 133082.748019, type 3 (EV_ABS), code 0 (ABS_X), value 316

    A SYN_REPORT event looks like:
    Event: time 10788.289613, -------------- SYN_REPORT ------------

    """

    def __init__(self, type=0, code=0, value=0):
        """Construction of an input event.

        @param type: the event type
        @param code: the event code
        @param value: the event value

        """
        self.type = type
        self.code = code
        self.value= value


    @staticmethod
    def from_string(ev_string):
        """Convert an event string to an event object.

        @param ev_string: an event string.

        @returns: an event object if the event string conforms to
                  event pattern format. None otherwise.

        """
        # Get the pattern of an ordinary event
        ev_pattern_time = r'Event:\s*time\s*(\d+\.\d+)'
        ev_pattern_type = r'type\s*(\d+)\s*\(\w+\)'
        ev_pattern_code = r'code\s*(\d+)\s*\(\w+\)'
        ev_pattern_value = r'value\s*(-?\d+)'
        ev_sep = r',\s*'
        ev_pattern_str = ev_sep.join([ev_pattern_time,
                                      ev_pattern_type,
                                      ev_pattern_code,
                                      ev_pattern_value])
        ev_pattern = re.compile(ev_pattern_str, re.I)

        # Get the pattern of the SYN_REPORT event
        ev_pattern_type_SYN_REPORT = r'-+\s*SYN_REPORT\s-+'
        ev_pattern_SYN_REPORT_str = ev_sep.join([ev_pattern_time,
                                                 ev_pattern_type_SYN_REPORT])
        ev_pattern_SYN_REPORT = re.compile(ev_pattern_SYN_REPORT_str, re.I)

        # Check if it is a SYN event.
        result = ev_pattern_SYN_REPORT.search(ev_string)
        if result:
            return Event(EV_SYN, SYN_REPORT, 0)
        else:
            # Check if it is a regular event.
            result = ev_pattern.search(ev_string)
            if result:
                ev_type = int(result.group(2))
                ev_code = int(result.group(3))
                ev_value = int(result.group(4))
                return Event(ev_type, ev_code, ev_value)
            else:
                logging.warn('not an event: %s', ev_string)
                return None


    def is_syn(self):
        """Determine if the event is a SYN report event.

        @returns: True if it is a SYN report event. False otherwise.

        """
        return self.type == EV_SYN and self.code == SYN_REPORT


    def value_tuple(self):
        """A tuple of the event type, code, and value.

        @returns: the tuple of the event type, code, and value.

        """
        return (self.type, self.code, self.value)


    def __eq__(self, other):
        """determine if two events are equal.

        @param line: an event string line.

        @returns: True if two events are equal. False otherwise.

        """
        return (self.type == other.type and
                self.code == other.code and
                self.value == other.value)


    def __str__(self):
        """A string representation of the event.

        @returns: a string representation of the event.

        """
        return '%d %d %d' % (self.type, self.code, self.value)


class InputEventRecorder(object):
    """An input event recorder.

    Refer to recording_example() below about how to record input events.

    """

    INPUT_DEVICE_INFO_FILE = '/proc/bus/input/devices'
    SELECT_TIMEOUT_SECS = 1

    def __init__(self, device_name):
        """Construction of input event recorder.

        @param device_name: the device name of the input device node to record.

        """
        self.device_name = device_name
        self.device_node = self.get_device_node_by_name(device_name)
        if self.device_node is None:
            err_msg = 'Failed to find the device node of %s' % device_name
            raise InputEventRecorderError(err_msg)
        self._recording_thread = None
        self._stop_recording_thread_event = threading.Event()
        self.tmp_file = '/tmp/evtest.dat'
        self.events = []


    def get_device_node_by_name(self, device_name):
        """Get the input device node by name.

        Example of a RN-42 emulated mouse device information looks like

        I: Bus=0005 Vendor=0000 Product=0000 Version=0000
        N: Name="RNBT-A96F"
        P: Phys=6c:29:95:1a:b8:18
        S: Sysfs=/devices/pci0000:00/0000:00:14.0/usb1/1-8/1-8:1.0/bluetooth/hci0/hci0:512:29/0005:0000:0000.0004/input/input15
        U: Uniq=00:06:66:75:a9:6f
        H: Handlers=event12
        B: PROP=0
        B: EV=17
        B: KEY=70000 0 0 0 0
        B: REL=103
        B: MSC=10

        @param device_name: the device name of the target input device node.

        @returns: the corresponding device node of the device.

        """
        device_node = None
        device_found = None
        device_pattern = re.compile('N: Name=.*%s' % device_name, re.I)
        event_number_pattern = re.compile('H: Handlers=.*event(\d*)', re.I)
        with open(self.INPUT_DEVICE_INFO_FILE) as info:
            for line in info:
                if device_found:
                    result = event_number_pattern.search(line)
                    if result:
                        event_number = int(result.group(1))
                        device_node = '/dev/input/event%d' % event_number
                        break
                else:
                    device_found = device_pattern.search(line)
        return device_node


    def record(self):
        """Record input events."""
        logging.info('Recording input events of %s.', self.device_node)
        cmd = 'evtest %s' % self.device_node
        self._recorder = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                          shell=True)
        with open(self.tmp_file, 'w') as output_f:
            while True:
                read_list, _, _ = select.select(
                        [self._recorder.stdout], [], [], 1)
                if read_list:
                    line = self._recorder.stdout.readline()
                    output_f.write(line)
                    ev = Event.from_string(line)
                    if ev:
                        self.events.append(ev.value_tuple())
                elif self._stop_recording_thread_event.is_set():
                    self._stop_recording_thread_event.clear()
                    break


    def start(self):
        """Start the recording thread."""
        logging.info('Start recording thread.')
        self._recording_thread = threading.Thread(target=self.record)
        self._recording_thread.start()


    def stop(self):
        """Stop the recording thread."""
        logging.info('Stop recording thread.')
        self._stop_recording_thread_event.set()
        self._recording_thread.join()


    def clear_events(self):
        """Clear the event list."""
        self.events = []


    def get_events(self):
        """Get the event list.

        @returns: the event list.
        """
        return self.events


SYN_EVENT = Event(EV_SYN, SYN_REPORT, 0)
MSC_SCAN_BTN_EVENT = {'LEFT': Event(EV_MSC, MSC_SCAN, MSC_SCAN_BTN_LEFT),
                      'RIGHT': Event(EV_MSC, MSC_SCAN, MSC_SCAN_BTN_RIGHT),
                      'MIDDLE': Event(EV_MSC, MSC_SCAN, MSC_SCAN_BTN_MIDDLE)}


def recording_example():
    """Example code for capturing input events on a Samus.

    For a quick swipe, it outputs events in numeric format:

    (3, 57, 9)
    (3, 53, 641)
    (3, 54, 268)
    (3, 58, 60)
    (3, 48, 88)
    (1, 330, 1)
    (1, 325, 1)
    (3, 0, 641)
    (3, 1, 268)
    (3, 24, 60)
    (0, 0, 0)
    (3, 53, 595)
    (3, 54, 288)
    (3, 0, 595)
    (3, 1, 288)
    (0, 0, 0)
    (3, 57, -1)
    (1, 330, 0)
    (1, 325, 0)
    (3, 24, 0)
    (0, 0, 0)

    The above events in corresponding evtest text format are:

    Event: time .782950, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value 9
    Event: time .782950, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 641
    Event: time .782950, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 268
    Event: time .782950, type 3 (EV_ABS), code 58 (ABS_MT_PRESSURE), value 60
    Event: time .782950, type 3 (EV_ABS), code 59 (?), value 0
    Event: time .782950, type 3 (EV_ABS), code 48 (ABS_MT_TOUCH_MAJOR), value 88
    Event: time .782950, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 1
    Event: time .782950, type 1 (EV_KEY), code 325 (BTN_TOOL_FINGER), value 1
    Event: time .782950, type 3 (EV_ABS), code 0 (ABS_X), value 641
    Event: time .782950, type 3 (EV_ABS), code 1 (ABS_Y), value 268
    Event: time .782950, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 60
    Event: time .782950, -------------- SYN_REPORT ------------
    Event: time .798273, type 3 (EV_ABS), code 53 (ABS_MT_POSITION_X), value 595
    Event: time .798273, type 3 (EV_ABS), code 54 (ABS_MT_POSITION_Y), value 288
    Event: time .798273, type 3 (EV_ABS), code 0 (ABS_X), value 595
    Event: time .798273, type 3 (EV_ABS), code 1 (ABS_Y), value 288
    Event: time .798273, -------------- SYN_REPORT ------------
    Event: time .821437, type 3 (EV_ABS), code 57 (ABS_MT_TRACKING_ID), value -1
    Event: time .821437, type 1 (EV_KEY), code 330 (BTN_TOUCH), value 0
    Event: time .821437, type 1 (EV_KEY), code 325 (BTN_TOOL_FINGER), value 0
    Event: time .821437, type 3 (EV_ABS), code 24 (ABS_PRESSURE), value 0
    Event: time .821437, -------------- SYN_REPORT ------------
    """
    device_name = 'Atmel maXTouch Touchpad'
    recorder = InputEventRecorder(device_name)
    print 'Samus touchpad device name:', recorder.device_name
    print 'Samus touchpad device node:', recorder.device_node
    print 'Please make gestures on the touchpad for up to 5 seconds.'
    recorder.clear_events()
    recorder.start()
    time.sleep(5)
    recorder.stop()
    for e in recorder.get_events():
        print e


if __name__ == '__main__':
    recording_example()
