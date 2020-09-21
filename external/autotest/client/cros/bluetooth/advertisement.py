# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Construction of an Advertisement object from an advertisement data
dictionary.

Much of this module refers to the code of test/example-advertisement in
bluez project.
"""

import dbus
import dbus.mainloop.glib
import dbus.service
import gobject
import logging


DBUS_PROP_IFACE = 'org.freedesktop.DBus.Properties'
LE_ADVERTISEMENT_IFACE = 'org.bluez.LEAdvertisement1'


class Advertisement(dbus.service.Object):
    """An advertisement object."""

    def __init__(self, bus, advertisement_data):
        """Construction of an Advertisement object.

        @param bus: a dbus system bus.
        @param advertisement_data: advertisement data dictionary.

        """
        self.bus = bus
        self._get_advertising_data(advertisement_data)
        super(Advertisement, self).__init__(self.bus, self.path)


    def _get_advertising_data(self, advertisement_data):
        """Get advertising data from the advertisement_data dictionary.

        @param bus: a dbus system bus.

        """
        self.path = advertisement_data.get('Path')
        self.type = advertisement_data.get('Type')
        self.service_uuids = advertisement_data.get('ServiceUUIDs', [])
        self.solicit_uuids = advertisement_data.get('SolicitUUIDs', [])

        # The xmlrpclib library requires that only string keys are allowed in
        # python dictionary. Hence, we need to define the manufacturer data
        # in an advertisement dictionary like
        #    'ManufacturerData': {'0xff00': [0xa1, 0xa2, 0xa3, 0xa4, 0xa5]},
        # in order to let autotest server transmit the advertisement to
        # a client DUT for testing.
        # On the other hand, the dbus method of advertising requires that
        # the signature of the manufacturer data to be 'qv' where 'q' stands
        # for unsigned 16-bit integer. Hence, we need to convert the key
        # from a string, e.g., '0xff00', to its hex value, 0xff00.
        # For signatures of the advertising properties, refer to
        #     device_properties in src/third_party/bluez/src/device.c
        # For explanation about signature types, refer to
        #     https://dbus.freedesktop.org/doc/dbus-specification.html
        self.manufacturer_data = dbus.Dictionary({}, signature='qv')
        manufacturer_data = advertisement_data.get('ManufacturerData', {})
        for key, value in manufacturer_data.items():
            self.manufacturer_data[int(key, 16)] = dbus.Array(value,
                                                              signature='y')

        self.service_data = dbus.Dictionary({}, signature='sv')
        service_data = advertisement_data.get('ServiceData', {})
        for uuid, data in service_data.items():
            self.service_data[uuid] = dbus.Array(data, signature='y')

        self.include_tx_power = advertisement_data.get('IncludeTxPower')


    def get_path(self):
        """Get the dbus object path of the advertisement.

        @returns: the advertisement object path.

        """
        return dbus.ObjectPath(self.path)


    @dbus.service.method(DBUS_PROP_IFACE, in_signature='s',
                         out_signature='a{sv}')
    def GetAll(self, interface):
        """Get the properties dictionary of the advertisement.

        @param interface: the bluetooth dbus interface.

        @returns: the advertisement properties dictionary.

        """
        if interface != LE_ADVERTISEMENT_IFACE:
            raise InvalidArgsException()

        properties = dict()
        properties['Type'] = dbus.String(self.type)

        if self.service_uuids is not None:
            properties['ServiceUUIDs'] = dbus.Array(self.service_uuids,
                                                    signature='s')
        if self.solicit_uuids is not None:
            properties['SolicitUUIDs'] = dbus.Array(self.solicit_uuids,
                                                    signature='s')
        if self.manufacturer_data is not None:
            properties['ManufacturerData'] = dbus.Dictionary(
                self.manufacturer_data, signature='qv')

        if self.service_data is not None:
            properties['ServiceData'] = dbus.Dictionary(self.service_data,
                                                        signature='sv')
        if self.include_tx_power is not None:
            properties['IncludeTxPower'] = dbus.Boolean(self.include_tx_power)

        return properties


    @dbus.service.method(LE_ADVERTISEMENT_IFACE, in_signature='',
                         out_signature='')
    def Release(self):
        """The method callback at release."""
        logging.info('%s: Advertisement Release() called.', self.path)


def example_advertisement():
    """A demo example of creating an Advertisement object.

    @returns: the Advertisement object.

    """
    ADVERTISEMENT_DATA = {
        'Path': '/org/bluez/test/advertisement1',

        # Could be 'central' or 'peripheral'.
        'Type': 'peripheral',

        # Refer to the specification for a list of service assgined numbers:
        # https://www.bluetooth.com/specifications/gatt/services
        # e.g., 180D represents "Heart Reate" service, and
        #       180F "Battery Service".
        'ServiceUUIDs': ['180D', '180F'],

        # Service solicitation UUIDs.
        'SolicitUUIDs': [],

        # Two bytes of manufacturer id followed by manufacturer specific data.
        'ManufacturerData': {'0xff00': [0xa1, 0xa2, 0xa3, 0xa4, 0xa5]},

        # service UUID followed by additional service data.
        'ServiceData': {'9999': [0x10, 0x20, 0x30, 0x40, 0x50]},

        # Does it include transmit power level?
        'IncludeTxPower': True}

    return Advertisement(bus, ADVERTISEMENT_DATA)


if __name__ == '__main__':
    # It is required to set the mainloop before creating the system bus object.
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    bus = dbus.SystemBus()

    adv = example_advertisement()
    print adv.GetAll(LE_ADVERTISEMENT_IFACE)
