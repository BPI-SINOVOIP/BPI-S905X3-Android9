#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google
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

# Defines utilities that can be used for making calls indenpendent of
# subscription IDs. This can be useful when making calls over mediums not SIM
# based.

# Make a phone call to the specified URI. It is assumed that we are making the
# call to the user selected default account.
#
# We usually want to make sure that the call has ended up in a good state.
#
# NOTE: This util is applicable to only non-conference type calls. It is best
# suited to test cases where only one call is in action at any point of time.

import queue
import time

from acts import logger
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.bt.BtEnum import *


def set_car_profile_priorities_off(car_droid, ph_droid):
    """Sets priority of car related profiles to OFF. This avoids
    autoconnect being triggered randomly. The use of this function
    is encouraged when you're testing individual profiles in isolation

    Args:
        log: log object
        car_droid: Car droid
        ph_droid: Phone droid

    Returns:
        True if success, False if fail.
    """
    # TODO investigate MCE
    car_profiles = [BluetoothProfile.A2DP_SINK,
                    BluetoothProfile.HEADSET_CLIENT,
                    BluetoothProfile.PBAP_CLIENT, BluetoothProfile.MAP_MCE]
    bt_test_utils.set_profile_priority(car_droid, ph_droid, car_profiles,
                                       BluetoothPriorityLevel.PRIORITY_OFF)
