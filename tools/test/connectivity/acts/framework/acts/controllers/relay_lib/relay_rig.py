#!/usr/bin/env python
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
from acts.controllers.relay_lib.errors import RelayConfigError
from acts.controllers.relay_lib.helpers import validate_key
from acts.controllers.relay_lib.rdl_relay_board import RdlRelayBoard
from acts.controllers.relay_lib.sain_smart_board import SainSmartBoard
from acts.controllers.relay_lib.sain_smart_8_channel_usb_relay_board import SainSmart8ChannelUsbRelayBoard
from acts.controllers.relay_lib.generic_relay_device import GenericRelayDevice
from acts.controllers.relay_lib.fugu_remote import FuguRemote
from acts.controllers.relay_lib.i6s_headset import I6sHeadset
from acts.controllers.relay_lib.logitech_headset import LogitechAudioReceiver
from acts.controllers.relay_lib.sony_xb2_speaker import SonyXB2Speaker
from acts.controllers.relay_lib.sony_xb20_speaker import SonyXB20Speaker
from acts.controllers.relay_lib.ak_xb10_speaker import AkXB10Speaker
from acts.controllers.relay_lib.dongles import SingleButtonDongle
from acts.controllers.relay_lib.dongles import ThreeButtonDongle


class RelayRig:
    """A group of relay boards and their connected devices.

    This class is also responsible for handling the creation of the relay switch
    boards, as well as the devices and relays associated with them.

    The boards dict can contain different types of relay boards. They share a
    common interface through inheriting from RelayBoard. This layer can be
    ignored by the user.

    The relay devices are stored in a dict of (device_name: device). These
    device references should be used by the user when they want to directly
    interface with the relay switches. See RelayDevice or GeneralRelayDevice for
    implementation.

    """
    DUPLICATE_ID_ERR_MSG = 'The {} "{}" is not unique. Duplicated in:\n {}'

    # A dict of lambdas that instantiate relay board upon invocation.
    # The key is the class type name, the value is the lambda.
    _board_constructors = {
        'SainSmartBoard':
        lambda x: SainSmartBoard(x),
        'RdlRelayBoard':
        lambda x: RdlRelayBoard(x),
        'SainSmart8ChannelUsbRelayBoard':
        lambda x: SainSmart8ChannelUsbRelayBoard(x),
    }

    # Similar to the dict above, except for devices.
    _device_constructors = {
        'GenericRelayDevice': lambda x, rig: GenericRelayDevice(x, rig),
        'FuguRemote': lambda x, rig: FuguRemote(x, rig),
        'I6sHeadset': lambda x, rig: I6sHeadset(x, rig),
        "LogitechAudioReceiver" :lambda x, rig: LogitechAudioReceiver(x, rig),
        'SonyXB2Speaker': lambda x, rig: SonyXB2Speaker(x, rig),
        'SonyXB20Speaker': lambda x, rig: SonyXB20Speaker(x, rig),
        'AkXB10Speaker': lambda x, rig: AkXB10Speaker(x, rig),
        'SingleButtonDongle': lambda x, rig: SingleButtonDongle(x, rig),
        'ThreeButtonDongle': lambda x, rig: ThreeButtonDongle(x, rig),
    }

    def __init__(self, config):
        self.relays = dict()
        self.boards = dict()
        self.devices = dict()

        validate_key('boards', config, list, 'relay config file')

        for elem in config['boards']:
            board = self.create_relay_board(elem)
            if board.name in self.boards:
                raise RelayConfigError(
                    self.DUPLICATE_ID_ERR_MSG.format('name', elem['name'],
                                                     elem))
            self.boards[board.name] = board

        # Note: 'boards' is a necessary value, 'devices' is not.
        if 'devices' in config:
            for elem in config['devices']:
                relay_device = self.create_relay_device(elem)
                if relay_device.name in self.devices:
                    raise RelayConfigError(
                        self.DUPLICATE_ID_ERR_MSG.format(
                            'name', elem['name'], elem))
                self.devices[relay_device.name] = relay_device
        else:
            device_config = dict()
            device_config['name'] = 'GenericRelayDevice'
            device_config['relays'] = dict()
            for relay_id in self.relays:
                device_config['relays'][relay_id] = relay_id
            self.devices['device'] = self.create_relay_device(device_config)

    def create_relay_board(self, config):
        """Builds a RelayBoard from the given config.

        Args:
            config: An object containing 'type', 'name', 'relays', and
            (optionally) 'properties'. See the example json file.

        Returns:
            A RelayBoard with the given type found in the config.

        Raises:
            RelayConfigError if config['type'] doesn't exist or is not a string.

        """
        validate_key('type', config, str, '"boards" element')
        try:
            ret = self._board_constructors[config['type']](config)
        except LookupError:
            raise RelayConfigError(
                'RelayBoard with type {} not found. Has it been added '
                'to the _board_constructors dict?'.format(config['type']))
        for _, relay in ret.relays.items():
            self.relays[relay.relay_id] = relay
        return ret

    def create_relay_device(self, config):
        """Builds a RelayDevice from the given config.

        When given no 'type' key in the config, the function will default to
        returning a GenericRelayDevice with the relays found in the 'relays'
        array.

        Args:
            config: An object containing 'name', 'relays', and (optionally)
            type.

        Returns:
            A RelayDevice with the given type found in the config. If no type is
            found, it will default to GenericRelayDevice.

        Raises:
            RelayConfigError if the type given does not match any from the
            _device_constructors dictionary.

        """
        if 'type' in config:
            if config['type'] not in RelayRig._device_constructors:
                raise RelayConfigError(
                    'Device with type {} not found. Has it been added '
                    'to the _device_constructors dict?'.format(config['type']))
            else:
                device = self._device_constructors[config['type']](config,
                                                                   self)

        else:
            device = GenericRelayDevice(config, self)

        return device
