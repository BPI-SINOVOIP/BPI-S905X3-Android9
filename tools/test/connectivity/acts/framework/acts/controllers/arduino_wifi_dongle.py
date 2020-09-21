#!/usr/bin/env python3
#
#   Copyright 2018 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import logging
import os
import re
import serial
import subprocess
import threading
import time

from acts import logger
from acts import signals
from acts import tracelogger
from acts import utils
from acts.test_utils.wifi import wifi_test_utils as wutils

from datetime import datetime

ACTS_CONTROLLER_CONFIG_NAME = "ArduinoWifiDongle"
ACTS_CONTROLLER_REFERENCE_NAME = "arduino_wifi_dongles"

WIFI_DONGLE_EMPTY_CONFIG_MSG = "Configuration is empty, abort!"
WIFI_DONGLE_NOT_LIST_CONFIG_MSG = "Configuration should be a list, abort!"

DEV = "/dev/"
IP = "IP: "
STATUS = "STATUS: "
SSID = "SSID: "
RSSI = "RSSI: "
PING = "PING: "
SCAN_BEGIN = "Scan Begin"
SCAN_END = "Scan End"
READ_TIMEOUT = 10
BAUD_RATE = 9600
TMP_DIR = "tmp/"
SSID_KEY = wutils.WifiEnums.SSID_KEY
PWD_KEY = wutils.WifiEnums.PWD_KEY


class ArduinoWifiDongleError(signals.ControllerError):
    pass

class DoesNotExistError(ArduinoWifiDongleError):
    """Raised when something that does not exist is referenced."""

def create(configs):
    """Creates ArduinoWifiDongle objects.

    Args:
        configs: A list of dicts or a list of serial numbers, each representing
                 a configuration of a arduino wifi dongle.

    Returns:
        A list of Wifi dongle objects.
    """
    wcs = []
    if not configs:
        raise ArduinoWifiDongleError(WIFI_DONGLE_EMPTY_CONFIG_MSG)
    elif not isinstance(configs, list):
        raise ArduinoWifiDongleError(WIFI_DONGLE_NOT_LIST_CONFIG_MSG)
    elif isinstance(configs[0], str):
        # Configs is a list of serials.
        wcs = get_instances(configs)
    else:
        # Configs is a list of dicts.
        wcs = get_instances_with_configs(configs)

    return wcs

def destroy(wcs):
    for wc in wcs:
        wc.clean_up()

def get_instances(configs):
    wcs = []
    for s in configs:
        wcs.append(ArduinoWifiDongle(s))
    return wcs

def get_instances_with_configs(configs):
    wcs = []
    for c in configs:
        try:
            s = c.pop("serial")
        except KeyError:
            raise ArduinoWifiDongleError(
                "'serial' is missing for ArduinoWifiDongle config %s." % c)
        wcs.append(ArduinoWifiDongle(s))
    return wcs

class ArduinoWifiDongle(object):
    """Class representing an arduino wifi dongle.

    Each object of this class represents one wifi dongle in ACTS.

    Attribtues:
        serial: Short serial number of the wifi dongle in string.
        port: The terminal port the dongle is connected to in string.
        log: A logger adapted from root logger with added token specific to an
             ArduinoWifiDongle instance.
        log_file_fd: File handle of the log file.
        set_logging: Logging for the dongle is enabled when this param is set
        lock: Lock to acquire and release set_logging variable
        ssid: SSID of the wifi network the dongle is connected to.
        ip_addr: IP address on the wifi interface.
        scan_results: Most recent scan results.
        ping: Ping status in bool - ping to www.google.com
    """
    def __init__(self, serial=''):
        """Initializes the ArduinoWifiDongle object."""
        self.serial = serial
        self.port = self._get_serial_port()
        self.log = logger.create_tagged_trace_logger(
            "ArduinoWifiDongle|%s" % self.serial)
        log_path_base = getattr(logging, "log_path", "/tmp/logs")
        self.log_file_path = os.path.join(
            log_path_base, "ArduinoWifiDongle_%s_serial_log.txt" % self.serial)
        self.log_file_fd = open(self.log_file_path, "a")

        self.set_logging = True
        self.lock = threading.Lock()
        self.start_controller_log()

        self.ssid = None
        self.ip_addr = None
        self.status = 0
        self.scan_results = []
        self.scanning = False
        self.ping = False

        try:
            os.stat(TMP_DIR)
        except:
            os.mkdir(TMP_DIR)

    def clean_up(self):
        """Cleans up the ArduinoifiDongle object and releases any resources it
        claimed.
        """
        self.stop_controller_log()
        self.log_file_fd.close()

    def _get_serial_port(self):
        """Get the serial port for a given ArduinoWifiDongle serial number.

        Returns:
            Serial port in string if the dongle is attached.
        """
        if not self.serial:
            raise ArduinoWifiDongleError(
                "Wifi dongle's serial should not be empty")
        cmd = "ls %s" % DEV
        serial_ports = utils.exe_cmd(cmd).decode("utf-8", "ignore").split("\n")
        for port in serial_ports:
            if "USB" not in port:
                continue
            tty_port = "%s%s" % (DEV, port)
            cmd = "udevadm info %s" % tty_port
            udev_output = utils.exe_cmd(cmd).decode("utf-8", "ignore")
            result = re.search("ID_SERIAL_SHORT=(.*)\n", udev_output)
            if self.serial == result.group(1):
                logging.info("Found wifi dongle %s at serial port %s" %
                             (self.serial, tty_port))
                return tty_port
        raise ArduinoWifiDongleError("Wifi dongle %s is specified in config"
                                    " but is not attached." % self.serial)

    def write(self, arduino, file_path, network=None):
        """Write an ino file to the arduino wifi dongle.

        Args:
            arduino: path of the arduino executable.
            file_path: path of the ino file to flash onto the dongle.
            network: wifi network to connect to.

        Returns:
            True: if the write is sucessful.
            False: if not.
        """
        return_result = True
        self.stop_controller_log("Flashing %s\n" % file_path)
        cmd = arduino + file_path + " --upload --port " + self.port
        if network:
            cmd = self._update_ino_wifi_network(arduino, file_path, network)
        self.log.info("Command is %s" % cmd)
        proc = subprocess.Popen(cmd,
            stdout=subprocess.PIPE, stderr=subprocess.PIPE, shell=True)
        out, err = proc.communicate()
        return_code = proc.returncode
        if return_code != 0:
            self.log.error("Failed to write file %s" % return_code)
            return_result = False
        self.start_controller_log("Flashing complete\n")
        return return_result

    def _update_ino_wifi_network(self, arduino, file_path, network):
        """Update wifi network in the ino file.

        Args:
            arduino: path of the arduino executable.
            file_path: path of the ino file to flash onto the dongle
            network: wifi network to update the ino file with

        Returns:
            cmd: arduino command to run to flash the ino file
        """
        tmp_file = "%s%s" % (TMP_DIR, file_path.split('/')[-1])
        utils.exe_cmd("cp %s %s" % (file_path, tmp_file))
        ssid = network[SSID_KEY]
        pwd = network[PWD_KEY]
        sed_cmd = "sed -i 's/\"wifi_tethering_test\"/\"%s\"/' %s" % (ssid, tmp_file)
        utils.exe_cmd(sed_cmd)
        sed_cmd = "sed -i  's/\"password\"/\"%s\"/' %s" % (pwd, tmp_file)
        utils.exe_cmd(sed_cmd)
        cmd = "%s %s --upload --port %s" %(arduino, tmp_file, self.port)
        return cmd

    def start_controller_log(self, msg=None):
        """Reads the serial port and writes the data to ACTS log file.

        This method depends on the logging enabled in the .ino files. The logs
        are read from the serial port and are written to the ACTS log after
        adding a timestamp to the data.

        Args:
            msg: Optional param to write to the log file.
        """
        if msg:
            curr_time = str(datetime.now())
            self.log_file_fd.write(curr_time + " INFO: " + msg)
        t = threading.Thread(target=self._start_log)
        t.daemon = True
        t.start()

    def stop_controller_log(self, msg=None):
        """Stop the controller log.

        Args:
            msg: Optional param to write to the log file.
        """
        with self.lock:
            self.set_logging = False
        if msg:
            curr_time = str(datetime.now())
            self.log_file_fd.write(curr_time + " INFO: " + msg)

    def _start_log(self):
        """Target method called by start_controller_log().

        This method is called as a daemon thread, which continously reads the
        serial port. Stops when set_logging is set to False or when the test
        ends.
        """
        self.set_logging = True
        ser = serial.Serial(self.port, BAUD_RATE)
        while True:
            curr_time = str(datetime.now())
            data = ser.readline().decode("utf-8", "ignore")
            self._set_vars(data)
            with self.lock:
                if not self.set_logging:
                    break
            self.log_file_fd.write(curr_time + " " + data)

    def _set_vars(self, data):
        """Sets the variables by reading from the serial port.

        Wifi dongle data such as wifi status, ip address, scan results
        are read from the serial port and saved inside the class.

        Args:
            data: New line from the serial port.
        """
        # 'data' represents each line retrieved from the device's serial port.
        # since we depend on the serial port logs to get the attributes of the
        # dongle, every line has the format of {ino_file: method: param: value}.
        # We look for the attribute in the log and retrieve its value.
        # Ex: data = "connect_wifi: loop(): STATUS: 3" then val = "3"
        # Similarly, we check when the scan has begun and ended and get all the
        # scan results in between.
        if data.count(":") != 3:
            return
        val = data.split(":")[-1].lstrip().rstrip()
        if SCAN_BEGIN in data:
            self.scan_results = []
            self.scanning = True
        elif SCAN_END in data:
            self.scanning = False
        elif self.scanning:
            self.scan_results.append(data)
        elif IP in data:
            self.ip_addr = None if val == "0.0.0.0" else val
        elif SSID in data:
            self.ssid = val
        elif STATUS in data:
            self.status = int(val)
        elif PING in data:
            self.ping = False if int(val) == 0 else True

    def ip_address(self, exp_result=True, timeout=READ_TIMEOUT):
        """Get the ip address of the wifi dongle.

        Args:
            exp_result: True if IP address is expected (wifi connected).
            timeout: Optional param that specifies the wait time for the IP
                     address to come up on the dongle.

        Returns:
            IP: addr in string, if wifi connected.
                None if not connected.
        """
        curr_time = time.time()
        while time.time() < curr_time + timeout:
            if (exp_result and self.ip_addr) or \
                (not exp_result and not self.ip_addr):
                  break
            time.sleep(1)
        return self.ip_addr

    def wifi_status(self, exp_result=True, timeout=READ_TIMEOUT):
        """Get wifi status on the dongle.

        Returns:
            True: if wifi is connected.
            False: if not connected.
        """
        curr_time = time.time()
        while time.time() < curr_time + timeout:
            if (exp_result and self.status == 3) or \
                (not exp_result and not self.status):
                  break
            time.sleep(1)
        return self.status == 3

    def wifi_scan(self, exp_result=True, timeout=READ_TIMEOUT):
        """Get the wifi scan results.

        Args:
            exp_result: True if scan results are expected.
            timeout: Optional param that specifies the wait time for the scan
                     results to come up on the dongle.

        Returns:
            list of dictionaries each with SSID and RSSI of the network
            found in the scan.
        """
        scan_networks = []
        d = {}
        curr_time = time.time()
        while time.time() < curr_time + timeout:
            if (exp_result and self.scan_results) or \
                (not exp_result and not self.scan_results):
                  break
            time.sleep(1)
        for i in range(len(self.scan_results)):
            if SSID in self.scan_results[i]:
                d = {}
                d[SSID] = self.scan_results[i].split(":")[-1].rstrip()
            elif RSSI in self.scan_results[i]:
                d[RSSI] = self.scan_results[i].split(":")[-1].rstrip()
                scan_networks.append(d)

        return scan_networks

    def ping_status(self, exp_result=True, timeout=READ_TIMEOUT):
        """ Get ping status on the dongle.

        Returns:
            True: if ping is successful
            False: if not successful
        """
        curr_time = time.time()
        while time.time() < curr_time + timeout:
            if (exp_result and self.ping) or \
                (not exp_result and not self.ping):
                  break
            time.sleep(1)
        return self.ping
