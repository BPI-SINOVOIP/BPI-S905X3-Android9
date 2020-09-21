#!/usr/bin/env python3
#
#   Copyright 2016 - The Android Open Source Project
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

import copy
import os
import tempfile
import shutil
import unittest

from acts.controllers.relay_lib import generic_relay_device
from acts.controllers.relay_lib import relay
from acts.controllers.relay_lib import relay_board
from acts.controllers.relay_lib import relay_device
from acts.controllers.relay_lib import relay_rig
from acts.controllers.relay_lib import sain_smart_board

import acts.controllers.relay_lib.errors as errors
import acts.controllers.relay_lib.fugu_remote as fugu_remote

# Shorthand versions of the long class names.
GenericRelayDevice = generic_relay_device.GenericRelayDevice
Relay = relay.Relay
RelayDict = relay.RelayDict
RelayState = relay.RelayState
SynchronizeRelays = relay.SynchronizeRelays
RelayBoard = relay_board.RelayBoard
RelayDevice = relay_device.RelayDevice
RelayRig = relay_rig.RelayRig
SainSmartBoard = sain_smart_board.SainSmartBoard
RelayDeviceConnectionError = errors.RelayDeviceConnectionError


class MockBoard(RelayBoard):
    def __init__(self, config):
        self.relay_states = dict()
        self.relay_previous_states = dict()
        RelayBoard.__init__(self, config)

    def get_relay_position_list(self):
        return [0, 1]

    def get_relay_status(self, relay_position):
        if relay_position not in self.relay_states:
            self.relay_states[relay_position] = RelayState.NO
            self.relay_previous_states[relay_position] = RelayState.NO
        return self.relay_states[relay_position]

    def set(self, relay_position, state):
        self.relay_previous_states[relay_position] = self.get_relay_status(
            relay_position)
        self.relay_states[relay_position] = state
        return state


class ActsRelayTest(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0
        self.config = {
            'name': 'MockBoard',
            'relays': [{
                'name': 'Relay',
                'relay_pos': 0
            }]
        }
        self.board = MockBoard(self.config)
        self.relay = Relay(self.board, 'Relay')
        self.board.set(self.relay.position, RelayState.NO)

    def tearDown(self):
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_turn_on_from_off(self):
        self.board.set(self.relay.position, RelayState.NO)
        self.relay.set_nc()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NC)

    def test_turn_on_from_on(self):
        self.board.set(self.relay.position, RelayState.NC)
        self.relay.set_nc()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NC)

    def test_turn_off_from_on(self):
        self.board.set(self.relay.position, RelayState.NC)
        self.relay.set_no()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NO)

    def test_turn_off_from_off(self):
        self.board.set(self.relay.position, RelayState.NO)
        self.relay.set_no()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NO)

    def test_toggle_off_to_on(self):
        self.board.set(self.relay.position, RelayState.NO)
        self.relay.toggle()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NC)

    def test_toggle_on_to_off(self):
        self.board.set(self.relay.position, RelayState.NC)
        self.relay.toggle()
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NO)

    def test_set_on(self):
        self.board.set(self.relay.position, RelayState.NO)
        self.relay.set(RelayState.NC)
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NC)

    def test_set_off(self):
        self.board.set(self.relay.position, RelayState.NC)
        self.relay.set(RelayState.NO)
        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NO)

    def test_set_foo(self):
        with self.assertRaises(ValueError):
            self.relay.set('FOO')

    def test_set_nc_for(self):
        # Here we set twice so relay_previous_state will also be OFF
        self.board.set(self.relay.position, RelayState.NO)
        self.board.set(self.relay.position, RelayState.NO)

        self.relay.set_nc_for(0)

        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NO)
        self.assertEqual(self.board.relay_previous_states[self.relay.position],
                         RelayState.NC)

    def test_set_no_for(self):
        # Here we set twice so relay_previous_state will also be OFF
        self.board.set(self.relay.position, RelayState.NC)
        self.board.set(self.relay.position, RelayState.NC)

        self.relay.set_no_for(0)

        self.assertEqual(
            self.board.get_relay_status(self.relay.position), RelayState.NC)
        self.assertEqual(self.board.relay_previous_states[self.relay.position],
                         RelayState.NO)

    def test_get_status_on(self):
        self.board.set(self.relay.position, RelayState.NC)
        self.assertEqual(self.relay.get_status(), RelayState.NC)

    def test_get_status_off(self):
        self.board.set(self.relay.position, RelayState.NO)
        self.assertEqual(self.relay.get_status(), RelayState.NO)

    def test_clean_up_default_on(self):
        new_relay = Relay(self.board, 0)
        new_relay._original_state = RelayState.NO
        self.board.set(new_relay.position, RelayState.NO)
        new_relay.clean_up()

        self.assertEqual(
            self.board.get_relay_status(new_relay.position), RelayState.NO)

    def test_clean_up_default_off(self):
        new_relay = Relay(self.board, 0)
        new_relay._original_state = RelayState.NO
        self.board.set(new_relay.position, RelayState.NC)
        new_relay.clean_up()

        self.assertEqual(
            self.board.get_relay_status(new_relay.position), RelayState.NO)

    def test_clean_up_original_state_none(self):
        val = 'STAYS_THE_SAME'
        new_relay = Relay(self.board, 0)
        # _original_state is none by default
        # The line below sets the dict to an impossible value.
        self.board.set(new_relay.position, val)
        new_relay.clean_up()
        # If the impossible value is cleared, then the test should fail.
        self.assertEqual(self.board.get_relay_status(new_relay.position), val)


class ActsSainSmartBoardTest(unittest.TestCase):
    STATUS_MSG = ('<small><a href="{}"></a>'
                  '</small><a href="{}/{}TUX">{}TUX</a><p>')

    RELAY_ON_PAGE_CONTENTS = 'relay_on page'
    RELAY_OFF_PAGE_CONTENTS = 'relay_off page'

    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0
        self.test_dir = 'file://' + tempfile.mkdtemp() + '/'

        # Creates the files used for testing
        self._set_status_page('0000000000000000')
        with open(self.test_dir[7:] + '00', 'w+') as file:
            file.write(self.RELAY_OFF_PAGE_CONTENTS)
        with open(self.test_dir[7:] + '01', 'w+') as file:
            file.write(self.RELAY_ON_PAGE_CONTENTS)

        self.config = ({
            'name': 'SSBoard',
            'base_url': self.test_dir,
            'relays': [{
                'name': '0',
                'relay_pos': 0
            }, {
                'name': '1',
                'relay_pos': 1
            }, {
                'name': '2',
                'relay_pos': 7
            }]
        })
        self.ss_board = SainSmartBoard(self.config)
        self.r0 = Relay(self.ss_board, 0)
        self.r1 = Relay(self.ss_board, 1)
        self.r7 = Relay(self.ss_board, 7)

    def tearDown(self):
        shutil.rmtree(self.test_dir[7:])
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_get_url_code(self):
        result = self.ss_board._get_relay_url_code(self.r0.position,
                                                   RelayState.NO)
        self.assertEqual(result, '00')

        result = self.ss_board._get_relay_url_code(self.r0.position,
                                                   RelayState.NC)
        self.assertEqual(result, '01')

        result = self.ss_board._get_relay_url_code(self.r7.position,
                                                   RelayState.NO)
        self.assertEqual(result, '14')

        result = self.ss_board._get_relay_url_code(self.r7.position,
                                                   RelayState.NC)
        self.assertEqual(result, '15')

    def test_load_page_status(self):
        self._set_status_page('0000111100001111')
        result = self.ss_board._load_page(SainSmartBoard.HIDDEN_STATUS_PAGE)
        self.assertTrue(
            result.endswith(
                '0000111100001111TUX">0000111100001111TUX</a><p>'))

    def test_load_page_relay(self):
        result = self.ss_board._load_page('00')
        self.assertEqual(result, self.RELAY_OFF_PAGE_CONTENTS)

        result = self.ss_board._load_page('01')
        self.assertEqual(result, self.RELAY_ON_PAGE_CONTENTS)

    def test_load_page_no_connection(self):
        with self.assertRaises(errors.RelayDeviceConnectionError):
            self.ss_board._load_page('**')

    def _set_status_page(self, status_16_chars):
        with open(self.test_dir[7:] + '99', 'w+') as status_file:
            status_file.write(
                self.STATUS_MSG.format(self.test_dir[:-1], self.test_dir[:-1],
                                       status_16_chars, status_16_chars))

    def _test_sync_status_dict(self, status_16_chars):
        self._set_status_page(status_16_chars)
        expected_dict = dict()

        for index, char in enumerate(status_16_chars):
            expected_dict[
                index] = RelayState.NC if char == '1' else RelayState.NO

        self.ss_board._sync_status_dict()
        self.assertDictEqual(expected_dict, self.ss_board.status_dict)

    def test_sync_status_dict(self):
        self._test_sync_status_dict('0000111100001111')
        self._test_sync_status_dict('0000000000000000')
        self._test_sync_status_dict('0101010101010101')
        self._test_sync_status_dict('1010101010101010')
        self._test_sync_status_dict('1111111111111111')

    def test_get_relay_status_status_dict_none(self):
        self._set_status_page('1111111111111111')
        self.ss_board.status_dict = None
        self.assertEqual(
            self.ss_board.get_relay_status(self.r0.position), RelayState.NC)

    def test_get_relay_status_status_dict_on(self):
        self.r0.set(RelayState.NC)
        self.assertEqual(
            self.ss_board.get_relay_status(self.r0.position), RelayState.NC)

    def test_get_relay_status_status_dict_off(self):
        self.r0.set(RelayState.NO)
        self.assertEqual(
            self.ss_board.get_relay_status(self.r0.position), RelayState.NO)

    def test_set_on(self):
        os.utime(self.test_dir[7:] + '01', (0, 0))
        self.ss_board.set(self.r0.position, RelayState.NC)
        self.assertNotEqual(os.stat(self.test_dir[7:] + '01').st_atime, 0)

    def test_set_off(self):
        os.utime(self.test_dir[7:] + '00', (0, 0))
        self.ss_board.set(self.r0.position, RelayState.NO)
        self.assertNotEqual(os.stat(self.test_dir[7:] + '00').st_atime, 0)

    def test_connection_error_no_tux(self):
        default_status_msg = self.STATUS_MSG
        self.STATUS_MSG = self.STATUS_MSG.replace('TUX', '')
        try:
            self._set_status_page('1111111111111111')
            self.ss_board.get_relay_status(0)
        except RelayDeviceConnectionError:
            self.STATUS_MSG = default_status_msg
            return

        self.fail('Should have thrown an error without TUX appearing.')


class ActsRelayRigTest(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0
        self.config = {
            'boards': [{
                'type': 'SainSmartBoard',
                'name': 'ss_control',
                'base_url': 'http://192.168.1.4/30000/'
            }, {
                'type': 'SainSmartBoard',
                'name': 'ss_control_2',
                'base_url': 'http://192.168.1.4/30000/'
            }],
            'devices': [{
                'type': 'GenericRelayDevice',
                'name': 'device',
                'relays': {
                    'Relay00': 'ss_control/0',
                    'Relay10': 'ss_control/1'
                }
            }]
        }

    def tearDown(self):
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_init_relay_rig_missing_boards(self):
        flawed_config = copy.deepcopy(self.config)
        del flawed_config['boards']
        with self.assertRaises(errors.RelayConfigError):
            RelayRig(flawed_config)

    def test_init_relay_rig_is_not_list(self):
        flawed_config = copy.deepcopy(self.config)
        flawed_config['boards'] = self.config['boards'][0]
        with self.assertRaises(errors.RelayConfigError):
            RelayRig(flawed_config)

    def test_init_relay_rig_duplicate_board_names(self):
        flawed_config = copy.deepcopy(self.config)
        flawed_config['boards'][1]['name'] = (self.config['boards'][0]['name'])
        with self.assertRaises(errors.RelayConfigError):
            RelayRigMock(flawed_config)

    def test_init_relay_rig_device_gets_relays(self):
        modded_config = copy.deepcopy(self.config)
        del modded_config['devices'][0]['relays']['Relay00']
        rig = RelayRigMock(modded_config)
        self.assertEqual(len(rig.relays), 4)
        self.assertEqual(len(rig.devices['device'].relays), 1)

        rig = RelayRigMock(self.config)
        self.assertEqual(len(rig.devices['device'].relays), 2)

    def test_init_relay_rig_correct_device_type(self):
        rig = RelayRigMock(self.config)
        self.assertEqual(len(rig.devices), 1)
        self.assertIsInstance(rig.devices['device'], GenericRelayDevice)

    def test_init_relay_rig_missing_devices_creates_generic_device(self):
        modded_config = copy.deepcopy(self.config)
        del modded_config['devices']
        rig = RelayRigMock(modded_config)
        self.assertEqual(len(rig.devices), 1)
        self.assertIsInstance(rig.devices['device'], GenericRelayDevice)
        self.assertDictEqual(rig.devices['device'].relays, rig.relays)


class RelayRigMock(RelayRig):
    """A RelayRig that substitutes the MockBoard for any board."""

    _board_constructors = {
        'SainSmartBoard': lambda x: MockBoard(x),
        'FuguMockBoard': lambda x: FuguMockBoard(x)
    }

    def __init__(self, config=None):
        if not config:
            config = {
                "boards": [{
                    'name': 'MockBoard',
                    'type': 'SainSmartBoard'
                }]
            }

        RelayRig.__init__(self, config)


class ActsGenericRelayDeviceTest(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0
        self.board_config = {'name': 'MockBoard', 'type': 'SainSmartBoard'}

        self.board = MockBoard(self.board_config)
        self.r0 = self.board.relays[0]
        self.r1 = self.board.relays[1]

        self.device_config = {
            'name': 'MockDevice',
            'relays': {
                'r0': 'MockBoard/0',
                'r1': 'MockBoard/1'
            }
        }
        config = {
            'boards': [self.board_config],
            'devices': [self.device_config]
        }
        self.rig = RelayRigMock(config)
        self.rig.boards['MockBoard'] = self.board
        self.rig.relays[self.r0.relay_id] = self.r0
        self.rig.relays[self.r1.relay_id] = self.r1

    def tearDown(self):
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_setup_single_relay(self):
        self.r0.set(RelayState.NC)
        self.r1.set(RelayState.NC)

        modified_config = copy.deepcopy(self.device_config)
        del modified_config['relays']['r1']

        grd = GenericRelayDevice(modified_config, self.rig)
        grd.setup()

        self.assertEqual(self.r0.get_status(), RelayState.NO)
        self.assertEqual(self.r1.get_status(), RelayState.NC)

    def test_setup_multiple_relays(self):
        self.board.set(self.r0.position, RelayState.NC)
        self.board.set(self.r1.position, RelayState.NC)

        grd = GenericRelayDevice(self.device_config, self.rig)
        grd.setup()

        self.assertEqual(self.r0.get_status(), RelayState.NO)
        self.assertEqual(self.r1.get_status(), RelayState.NO)

    def test_cleanup_single_relay(self):
        self.test_setup_single_relay()

    def test_cleanup_multiple_relays(self):
        self.test_setup_multiple_relays()

    def change_state(self, begin_state, call, end_state, previous_state=None):
        self.board.set(self.r0.position, begin_state)
        grd = GenericRelayDevice(self.device_config, self.rig)
        call(grd)
        self.assertEqual(self.r0.get_status(), end_state)
        if previous_state:
            self.assertEqual(
                self.board.relay_previous_states[self.r0.position],
                previous_state)

    def test_press_while_no(self):
        self.change_state(RelayState.NO, lambda x: x.press('r0'),
                          RelayState.NO, RelayState.NC)

    def test_press_while_nc(self):
        self.change_state(RelayState.NC, lambda x: x.press('r0'),
                          RelayState.NO, RelayState.NC)

    def test_hold_down_while_no(self):
        self.change_state(RelayState.NO, lambda x: x.hold_down('r0'),
                          RelayState.NC)

    def test_hold_down_while_nc(self):
        self.change_state(RelayState.NC, lambda x: x.hold_down('r0'),
                          RelayState.NC)

    def test_release_while_nc(self):
        self.change_state(RelayState.NC, lambda x: x.release('r0'),
                          RelayState.NO)


class ActsRelayDeviceTest(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0

        self.board_config = {
            'name': 'MockBoard',
            'relays': [{
                'id': 0,
                'relay_pos': 0
            }, {
                'id': 1,
                'relay_pos': 1
            }]
        }

        self.board = MockBoard(self.board_config)
        self.r0 = Relay(self.board, 0)
        self.r1 = Relay(self.board, 1)
        self.board.set(self.r0.position, RelayState.NO)
        self.board.set(self.r1.position, RelayState.NO)

        self.rig = RelayRigMock()
        self.rig.boards['MockBoard'] = self.board
        self.rig.relays[self.r0.relay_id] = self.r0
        self.rig.relays[self.r1.relay_id] = self.r1

        self.device_config = {
            "type": "GenericRelayDevice",
            "name": "device",
            "relays": {
                'r0': 'MockBoard/0',
                'r1': 'MockBoard/1'
            }
        }

    def tearDown(self):
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_init_raise_on_name_missing(self):
        flawed_config = copy.deepcopy(self.device_config)
        del flawed_config['name']
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_name_wrong_type(self):
        flawed_config = copy.deepcopy(self.device_config)
        flawed_config['name'] = {}
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_relays_missing(self):
        flawed_config = copy.deepcopy(self.device_config)
        del flawed_config['relays']
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_relays_wrong_type(self):
        flawed_config = copy.deepcopy(self.device_config)
        flawed_config['relays'] = str
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_relays_is_empty(self):
        flawed_config = copy.deepcopy(self.device_config)
        flawed_config['relays'] = []
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_relays_are_dicts_without_names(self):
        flawed_config = copy.deepcopy(self.device_config)
        flawed_config['relays'] = [{'id': 0}, {'id': 1}]
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_raise_on_relays_are_dicts_without_ids(self):
        flawed_config = copy.deepcopy(self.device_config)
        flawed_config['relays'] = [{'name': 'r0'}, {'name': 'r1'}]
        with self.assertRaises(errors.RelayConfigError):
            RelayDevice(flawed_config, self.rig)

    def test_init_pass_relays_have_ids_and_names(self):
        RelayDevice(self.device_config, self.rig)


class TestRelayRigParser(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        Relay.button_press_time = 0
        self.board_config = {
            'name': 'MockBoard',
            'relays': [{
                'id': 'r0',
                'relay_pos': 0
            }, {
                'id': 'r1',
                'relay_pos': 1
            }]
        }
        self.r0 = self.board_config['relays'][0]
        self.r1 = self.board_config['relays'][1]
        self.board = MockBoard(self.board_config)

    def tearDown(self):
        Relay.transition_wait_time = .2
        Relay.button_press_time = .25

    def test_create_relay_board_raise_on_missing_type(self):
        with self.assertRaises(errors.RelayConfigError):
            RelayRigMock().create_relay_board(self.board_config)

    def test_create_relay_board_valid_config(self):
        config = copy.deepcopy(self.board_config)
        config['type'] = 'SainSmartBoard'
        RelayRigMock().create_relay_board(config)

    def test_create_relay_board_raise_on_type_not_found(self):
        flawed_config = copy.deepcopy(self.board_config)
        flawed_config['type'] = 'NonExistentBoard'
        with self.assertRaises(errors.RelayConfigError):
            RelayRigMock().create_relay_board(flawed_config)

    def test_create_relay_device_create_generic_on_missing_type(self):
        rig = RelayRigMock()
        rig.relays['r0'] = self.r0
        rig.relays['r1'] = self.r1
        config = {
            'name': 'name',
            'relays': {
                'r0': 'MockBoard/0',
                'r1': 'MockBoard/1'
            }
        }
        device = rig.create_relay_device(config)
        self.assertIsInstance(device, GenericRelayDevice)

    def test_create_relay_device_config_with_type(self):
        rig = RelayRigMock()
        rig.relays['r0'] = self.r0
        rig.relays['r1'] = self.r1
        config = {
            'type': 'GenericRelayDevice',
            'name': '.',
            'relays': {
                'r0': 'MockBoard/0',
                'r1': 'MockBoard/1'
            }
        }
        device = rig.create_relay_device(config)
        self.assertIsInstance(device, GenericRelayDevice)

    def test_create_relay_device_raise_on_type_not_found(self):
        rig = RelayRigMock()
        rig.relays['r0'] = self.r0
        rig.relays['r1'] = self.r1
        config = {
            'type': 'SomeInvalidType',
            'name': '.',
            'relays': [{
                'name': 'r0',
                'pos': 'MockBoard/0'
            }, {
                'name': 'r1',
                'pos': 'MockBoard/1'
            }]
        }
        with self.assertRaises(errors.RelayConfigError):
            rig.create_relay_device(config)


class TestSynchronizeRelays(unittest.TestCase):
    def test_synchronize_relays(self):
        Relay.transition_wait_time = .1
        with SynchronizeRelays():
            self.assertEqual(Relay.transition_wait_time, 0)
        self.assertEqual(Relay.transition_wait_time, .1)


class FuguMockBoard(MockBoard):
    def get_relay_position_list(self):
        return range(4)


class TestFuguRemote(unittest.TestCase):
    def setUp(self):
        Relay.transition_wait_time = 0
        self.mock_rig = RelayRigMock({
            "boards": [{
                'name': 'MockBoard',
                'type': 'FuguMockBoard'
            }]
        })
        self.mock_board = self.mock_rig.boards['MockBoard']
        self.fugu_config = {
            'type': 'FuguRemote',
            'name': 'UniqueDeviceName',
            'mac_address': '00:00:00:00:00:00',
            'relays': {
                'Power': 'MockBoard/0',
                fugu_remote.Buttons.BACK.value: 'MockBoard/1',
                fugu_remote.Buttons.HOME.value: 'MockBoard/2',
                fugu_remote.Buttons.PLAY_PAUSE.value: 'MockBoard/3'
            }
        }
        Relay.button_press_time = 0

    def tearDown(self):
        Relay.button_press_time = .25
        Relay.transition_wait_time = .2

    def test_config_missing_button(self):
        """FuguRemote __init__ should throw an error if a relay is missing."""
        flawed_config = copy.deepcopy(self.fugu_config)
        del flawed_config['relays']['Power']
        del flawed_config['relays'][fugu_remote.Buttons.BACK.value]
        with self.assertRaises(errors.RelayConfigError):
            fugu_remote.FuguRemote(flawed_config, self.mock_rig)

    def test_config_missing_mac_address(self):
        """FuguRemote __init__ should throw an error without a mac address."""
        flawed_config = copy.deepcopy(self.fugu_config)
        del flawed_config['mac_address']
        with self.assertRaises(errors.RelayConfigError):
            fugu_remote.FuguRemote(flawed_config, self.mock_rig)

    def test_config_no_issues(self):
        """FuguRemote __init__ should not throw errors for a correct config."""
        fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)

    def test_power_nc_after_setup(self):
        """Power should be NORMALLY_CLOSED after calling setup if it exists."""
        fugu = fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)
        fugu.setup()
        self.assertEqual(self.mock_board.get_relay_status(0), RelayState.NC)
        pass

    def press_button_success(self, relay_position):
        self.assertEqual(self.mock_board.relay_states[relay_position],
                         RelayState.NO)
        self.assertEqual(self.mock_board.relay_previous_states[relay_position],
                         RelayState.NC)

    def test_press_play_pause(self):
        fugu = fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)
        fugu.press_play_pause()
        self.press_button_success(3)

    def test_press_back(self):
        fugu = fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)
        fugu.press_back()
        self.press_button_success(1)

    def test_press_home(self):
        fugu = fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)
        fugu.press_home()
        self.press_button_success(2)

    def test_enter_pairing_mode(self):
        fugu = fugu_remote.FuguRemote(self.fugu_config, self.mock_rig)
        fugu_remote.PAIRING_MODE_WAIT_TIME = 0
        fugu.enter_pairing_mode()
        self.press_button_success(2)
        self.press_button_success(1)


class TestRelayDict(unittest.TestCase):
    def test_init(self):
        mock_device = object()
        blank_dict = dict()
        relay_dict = RelayDict(mock_device, blank_dict)
        self.assertEqual(relay_dict._store, blank_dict)
        self.assertEqual(relay_dict.relay_device, mock_device)

    def test_get_item_valid_key(self):
        mock_device = object()
        blank_dict = {'key': 'value'}
        relay_dict = RelayDict(mock_device, blank_dict)
        self.assertEqual(relay_dict['key'], 'value')

    def test_get_item_invalid_key(self):
        # Create an object with a single attribute 'name'
        mock_device = type('', (object, ), {'name': 'name'})()
        blank_dict = {'key': 'value'}
        relay_dict = RelayDict(mock_device, blank_dict)
        with self.assertRaises(errors.RelayConfigError):
            value = relay_dict['not_key']

    def test_iter(self):
        mock_device = type('', (object, ), {'name': 'name'})()
        data_dict = {'a': '1', 'b': '2', 'c': '3'}
        relay_dict = RelayDict(mock_device, data_dict)

        rd_set = set()
        for key in relay_dict:
            rd_set.add(key)
        dd_set = set()
        for key in data_dict:
            dd_set.add(key)

        self.assertSetEqual(rd_set, dd_set)


if __name__ == "__main__":
    unittest.main()
