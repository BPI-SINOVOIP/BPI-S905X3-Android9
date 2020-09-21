# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module defines the motor board interface.

For example, chameleon is a ChameleonBoard object.
- Get motor board:

motor = chameleon.get_motor_board

- Touch/Release actions:

motor.Touch(ButtonFunction.CALL)
motor.Release(ButtonFunction.CALL)

"""


class ButtonFunction(object):
    """Button functions that motor touch/release."""
    CALL = 'Call'
    HANG_UP = 'Hang Up'
    MUTE = 'Mute'
    VOL_UP = 'Vol Up'
    VOL_DOWN = 'Vol Down'
