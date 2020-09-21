# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.


"""This module provides the information of Chameleon board."""


import collections
import logging


# Mapping from Chameleon MAC address to other information including
# bluetooth MAC address on audio board.
ChameleonInfo = collections.namedtuple(
        'ChameleonInfo', ['bluetooth_mac_address'])

_CHAMELEON_BOARD_INFO = {
        '94:eb:2c:00:00:fb': ChameleonInfo('00:1F:84:01:03:68'),
        '94:eb:2c:00:00:f9': ChameleonInfo('00:1F:84:01:03:73'),
        '94:eb:2c:00:01:25': ChameleonInfo('00:1F:84:01:03:4F'),
        '94:eb:2c:00:01:27': ChameleonInfo('00:1F:84:01:03:5B'),
        '94:eb:2c:00:01:28': ChameleonInfo('00:1F:84:01:03:46'),
        '94:eb:2c:00:01:29': ChameleonInfo('00:1F:84:01:03:26'),
        '94:eb:2c:00:01:2b': ChameleonInfo('00:1F:84:01:03:5E'),
        '94:eb:2c:00:01:2d': ChameleonInfo('00:1F:84:01:03:B6'),
        '94:eb:2c:00:01:30': ChameleonInfo('00:1F:84:01:03:2F'),
        '94:eb:2c:00:01:3a': ChameleonInfo('00:1F:84:01:03:42'),
        '94:eb:2c:00:01:3b': ChameleonInfo('00:1F:84:01:03:44'),
        '94:eb:2c:00:01:3d': ChameleonInfo('00:1F:84:01:03:59'),
        '94:eb:2c:00:01:3e': ChameleonInfo('00:1F:84:01:03:74'),
        '94:eb:2c:00:01:3f': ChameleonInfo('00:1F:84:01:03:8C'),
        '94:eb:2c:00:01:41': ChameleonInfo('00:1F:84:01:03:B3'),
        '94:eb:2c:10:06:65': ChameleonInfo('00:1F:84:01:03:6A'),
        '94:eb:2c:10:06:66': ChameleonInfo('00:1F:84:01:03:21'),
        '94:eb:2c:10:06:67': ChameleonInfo('00:1F:84:01:03:38'),
        '94:eb:2c:10:06:68': ChameleonInfo('00:1F:84:01:03:52'),
        '94:eb:2c:10:06:6c': ChameleonInfo('00:1F:84:01:03:2E'),
        '94:eb:2c:10:06:6d': ChameleonInfo('00:1F:84:01:03:84'),
        '94:eb:2c:10:06:6e': ChameleonInfo('00:1F:84:01:03:98'),
        '94:eb:2c:10:06:72': ChameleonInfo('00:1F:84:01:03:61'),
        '94:eb:2c:10:06:73': ChameleonInfo('00:1F:84:01:03:2C'),
        '94:eb:2c:10:06:76': ChameleonInfo('00:1F:84:01:03:83'),
        '94:eb:2c:10:06:7a': ChameleonInfo('00:1F:84:01:03:1C'),
        '94:eb:2c:10:06:7b': ChameleonInfo('00:1F:84:01:03:A7'),
        '94:eb:2c:10:06:7c': ChameleonInfo('00:1F:84:01:03:4B'),
        '94:eb:2c:10:06:7d': ChameleonInfo('00:1F:84:01:03:78'),
        '94:eb:2c:10:06:7e': ChameleonInfo('00:1F:84:01:03:7B'),
        '94:eb:2c:10:06:7f': ChameleonInfo('00:1F:84:01:03:36'),
        '94:eb:2c:00:01:26': ChameleonInfo('00:1F:84:01:03:56'),
        '94:eb:2c:00:01:17': ChameleonInfo('00:1F:84:01:03:76'),
        '94:eb:2c:00:01:31': ChameleonInfo('00:1F:84:01:03:20'),
        '94:eb:2c:00:01:18': ChameleonInfo('00:1F:84:01:03:A1'),
        '94:eb:2c:10:06:84': ChameleonInfo('00:1F:84:01:03:32'),

        # TODO (rjahagir@): Verify the addresses listed above as
        # as many were reworked/relocated. Some are duplicates.
        # Listed below are added as of 4/13/17.
        '94:eb:2c:10:06:74': ChameleonInfo('00:1F:84:01:03:88'),
        '94:eb:2c:10:06:a9': ChameleonInfo('00:1F:84:01:03:6C'),
        '94:eb:2c:10:06:89': ChameleonInfo('00:1F:84:01:03:40'),
        '94:eb:2c:10:06:a3': ChameleonInfo('00:1F:84:01:03:24'),
        '94:eb:2c:10:06:99': ChameleonInfo('00:1F:84:01:03:93'),
        '94:eb:2c:10:06:9a': ChameleonInfo('00:1F:84:01:03:B1'),
        '94:eb:2c:10:06:90': ChameleonInfo('00:1F:84:01:03:6E'),
        '94:eb:2c:00:01:00': ChameleonInfo('00:1F:84:01:03:54'),
        '94:eb:2c:00:01:01': ChameleonInfo('00:1F:84:01:03:3E'),
        '94:eb:2c:10:06:9e': ChameleonInfo('00:1F:84:01:03:97'),
        '94:eb:2c:10:06:9f': ChameleonInfo('00:1F:84:01:03:49'),

        # Listed below added as of 5/12/17.
        '94:eb:2c:10:06:98': ChameleonInfo('00:1F:84:01:03:65'),
        '94:eb:2c:00:01:19': ChameleonInfo('00:1F:84:01:03:91'),
        '94:eb:2c:10:06:8a': ChameleonInfo('00:1F:84:01:03:AB'),
        '94:eb:2c:00:01:1d': ChameleonInfo('00:1F:84:01:03:A6'),
        '94:eb:2c:10:06:95': ChameleonInfo('00:1F:84:01:03:66'),

        # Lars device changed as of 10/17/17.
        '94:eb:2c:00:01:1a': ChameleonInfo('00:1F:84:01:03:20')
}

class ChameleonInfoError(Exception):
    """Error in chameleon_info."""
    pass


def get_bluetooth_mac_address(chameleon_board):
    """Gets bluetooth MAC address of a ChameleonBoard.

    @param chameleon_board: A ChameleonBoard object.

    @returns: A string for bluetooth MAC address of bluetooth module on the
              audio board.

    @raises: ChameleonInfoError if bluetooth MAC address of this Chameleon
             board can not be found.

    """
    chameleon_mac_address = chameleon_board.get_mac_address().lower()
    if chameleon_mac_address not in _CHAMELEON_BOARD_INFO:
        raise ChameleonInfoError(
                'Chameleon info not found for %s' % chameleon_mac_address)
    board_info = _CHAMELEON_BOARD_INFO[chameleon_mac_address]
    logging.debug('Chameleon board info: %r', board_info)
    return board_info.bluetooth_mac_address
