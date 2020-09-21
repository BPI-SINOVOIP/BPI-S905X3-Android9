#!/usr/bin/env python
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
from acts import base_test
from acts.controllers.relay_lib.relay import SynchronizeRelays


class RelayDeviceSampleTest(base_test.BaseTestClass):
    """ Demonstrates example usage of a configurable access point."""

    def setup_class(self):
        # Take devices from relay_devices.
        self.relay_device = self.relay_devices[0]

        # You can use this workaround to get devices by name:

        relay_rig = self.relay_devices[0].rig
        self.other_relay_device = relay_rig.devices['UniqueDeviceName']
        # Note: If the "devices" key from the config is missing
        # a GenericRelayDevice that contains every switch in the config
        # will be stored in relay_devices[0]. Its name will be
        # "GenericRelayDevice".

    def setup_test(self):
        # setup() will set the relay device to the default state.
        # Unless overridden, the default state is all switches set to off.
        self.relay_device.setup()

    def teardown_test(self):
        # clean_up() will set the relay device back to a default state.
        # Unless overridden, the default state is all switches set to off.
        self.relay_device.clean_up()

    # Typical use of a GenericRelayDevice looks like this:
    def test_relay_device(self):

        # This function call will sleep until .25 seconds are up.
        # Blocking_nc_for will emulate a button press, which turns on the relay
        # (or stays on if it already was on) for the given time, and then turns
        # off.
        self.relay_device.relays['BT_Power_Button'].set_nc_for(.25)

        # do_something_after_turning_on_bt_power()

        # Note that the relays are mechanical switches, and do take real time
        # to go from one state to the next.

        self.relay_device.relays['BT_Pair'].set_nc()

        # do_something_while_holding_down_the_pair_button()

        self.relay_device.relays['BT_Pair'].set_no()

        # do_something_after_releasing_bt_pair()

        # Note that although cleanup sets the relays to the 'NO' state after
        # each test, they do not press things like the power button to turn
        # off whatever hardware is attached. When using a GenericRelayDevice,
        # you'll have to do this manually.
        # Other RelayDevices may handle this for you in their clean_up() call.
        self.relay_device.relays['BT_Power_Button'].set_nc_for(.25)

    def test_toggling(self):
        # This test just spams the toggle on each relay.
        for _ in range(0, 2):
            self.relay_device.relays['BT_Power_Button'].toggle()
            self.relay_device.relays['BT_Pair'].toggle()
            self.relay_device.relays['BT_Reset'].toggle()
            self.relay_device.relays['BT_SomethingElse'].toggle()

    def test_synchronize_relays(self):
        """Toggles relays using SynchronizeRelays().

        This makes each relay do it's action at the same time, without waiting
        after each relay to swap. Instead, all relays swap at the same time, and
        the wait is done after exiting the with statement.
        """
        for _ in range(0, 10):
            with SynchronizeRelays():
                self.relay_device.relays['BT_Power_Button'].toggle()
                self.relay_device.relays['BT_Pair'].toggle()
                self.relay_device.relays['BT_Reset'].toggle()
                self.relay_device.relays['BT_SomethingElse'].toggle()

        # For more fine control over the wait time of relays, you can set
        # Relay.transition_wait_time. This is not recommended unless you are
        # using solid state relays, or async calls.
