#!/usr/bin/env python3.4
#
#   Copyright 2017 - The Android Open Source Project
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
import time
import xmlrpc.client
from subprocess import call

from acts import signals

ACTS_CONTROLLER_CONFIG_NAME = "ChameleonDevice"
ACTS_CONTROLLER_REFERENCE_NAME = "chameleon_devices"

CHAMELEON_DEVICE_EMPTY_CONFIG_MSG = "Configuration is empty, abort!"
CHAMELEON_DEVICE_NOT_LIST_CONFIG_MSG = "Configuration should be a list, abort!"

audio_bus_endpoints = {
    'CROS_HEADPHONE': 'Cros device headphone',
    'CROS_EXTERNAL_MICROPHONE': 'Cros device external microphone',
    'PERIPHERAL_MICROPHONE': 'Peripheral microphone',
    'PERIPHERAL_SPEAKER': 'Peripheral speaker',
    'FPGA_LINEOUT': 'Chameleon FPGA line-out',
    'FPGA_LINEIN': 'Chameleon FPGA line-in',
    'BLUETOOTH_OUTPUT': 'Bluetooth module output',
    'BLUETOOTH_INPUT': 'Bluetooth module input'
}


class ChameleonDeviceError(signals.ControllerError):
    pass


def create(configs):
    if not configs:
        raise ChameleonDeviceError(CHAMELEON_DEVICE_EMPTY_CONFIG_MSG)
    elif not isinstance(configs, list):
        raise ChameleonDeviceError(CHAMELEON_DEVICE_NOT_LIST_CONFIG_MSG)
    elif isinstance(configs[0], str):
        # Configs is a list of IP addresses
        chameleons = get_instances(configs)
    return chameleons


def destroy(chameleons):
    for chameleon in chameleons:
        del chameleon


def get_info(chameleons):
    """Get information on a list of ChameleonDevice objects.

    Args:
        ads: A list of ChameleonDevice objects.

    Returns:
        A list of dict, each representing info for ChameleonDevice objects.
    """
    device_info = []
    for chameleon in chameleons:
        info = {"address": chameleon.address, "port": chameleon.port}
        device_info.append(info)
    return device_info


def get_instances(ips):
    """Create ChameleonDevice instances from a list of IPs.

    Args:
        ips: A list of Chameleon IPs.

    Returns:
        A list of ChameleonDevice objects.
    """
    return [ChameleonDevice(ip) for ip in ips]


class ChameleonDevice:
    """Class representing a Chameleon device.

    Each object of this class represents one Chameleon device in ACTS.

    Attributes:
        address: The full address to contact the Chameleon device at
        client: The ServiceProxy of the XMLRPC client.
        log: A logger object.
        port: The TCP port number of the Chameleon device.
    """

    def __init__(self, ip="", port=9992):
        self.ip = ip
        self.log = logging.getLogger()
        self.port = port
        self.address = "http://{}:{}".format(ip, self.port)
        try:
            self.client = xmlrpc.client.ServerProxy(
                self.address, allow_none=True, verbose=False)
        except ConnectionRefusedError as err:
            self.log.exception(
                "Failed to connect to Chameleon Device at: {}".format(
                    self.address))
        self.client.Reset()

    def pull_file(self, chameleon_location, destination):
        """Pulls a file from the Chameleon device. Usually the raw audio file.

        Args:
            chameleon_location: The path to the file on the Chameleon device
            destination: The destination to where to pull it locally.
        """
        # TODO: (tturney) implement
        self.log.error("Definition not yet implemented")

    def start_capturing_audio(self, port_id, has_file=True):
        """Starts capturing audio.

        Args:
            port_id: The ID of the audio input port.
            has_file: True for saving audio data to file. False otherwise.
        """
        self.client.StartCapturingAudio(port_id, has_file)

    def stop_capturing_audio(self, port_id):
        """Stops capturing audio.

        Args:
            port_id: The ID of the audio input port.
        Returns:
            List contain the location of the recorded audio and a dictionary
            of values relating to the raw audio including: file_type, channel,
            sample_format, and rate.
        """
        return self.client.StopCapturingAudio(port_id)

    def audio_board_connect(self, bus_number, endpoint):
        """Connects an endpoint to an audio bus.

        Args:
            bus_number: 1 or 2 for audio bus 1 or bus 2.
            endpoint: An endpoint defined in audio_bus_endpoints.
        """
        self.client.AudioBoardConnect(bus_number, endpoint)

    def audio_board_disconnect(self, bus_number, endpoint):
        """Connects an endpoint to an audio bus.

        Args:
            bus_number: 1 or 2 for audio bus 1 or bus 2.
            endpoint: An endpoint defined in audio_bus_endpoints.
        """
        self.client.AudioBoardDisconnect(bus_number, endpoint)

    def audio_board_disable_bluetooth(self):
        """Disables Bluetooth module on audio board."""
        self.client.AudioBoardDisableBluetooth()

    def audio_board_clear_routes(self, bus_number):
        """Clears routes on an audio bus.

        Args:
            bus_number: 1 or 2 for audio bus 1 or bus 2.
        """
        self.client.AudioBoardClearRoutes(bus_number)

    def scp(self, source, destination):
        """Copies files from the Chameleon device to the host machine.

        Args:
            source: The file path on the Chameleon board.
            dest: The file path on the host machine.
        """
        cmd = "scp root@{}:/{} {}".format(self.ip, source, destination)
        try:
            call(cmd.split(" "))
        except FileNotFoundError as err:
            self.log.exception("File not found {}".format(source))
