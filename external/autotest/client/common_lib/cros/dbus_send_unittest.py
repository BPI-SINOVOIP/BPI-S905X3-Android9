#!/usr/bin/python
#
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import unittest

import common
import dbus
from autotest_lib.client.common_lib.cros import dbus_send

EXAMPLE_SHILL_GET_PROPERTIES_OUTPUT = (
'method return sender=org.freedesktop.DBus -> destination=:1.37 serial=3 '
'reply_serial=2\n'
"""
   array [
      dict entry(
         string "ActiveProfile"
         variant             string "/profile/default"
      )
      dict entry(
         string "ArpGateway"
         variant             boolean true
      )
      dict entry(
         string "AvailableTechnologies"
         variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "CheckPortalList"
         variant             string "''"
      )
      dict entry(
         string "ConnectedTechnologies" variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "ConnectionState"
         variant             string "online"
      )
      dict entry(
         string "Country"
         variant             string ""
      )
      dict entry(
         string "DefaultService"
         variant             object path "/service/2"
      )
      dict entry(
         string "DefaultTechnology"
         variant             string "ethernet"
      )
      dict entry(
         string "Devices"
         variant             array [
               object path "/device/eth0"
               object path "/device/eth1"
            ]
      )
      dict entry(
         string "DisableWiFiVHT"
         variant             boolean false
      )
      dict entry(
         string "EnabledTechnologies"
         variant             array [
               string "ethernet"
            ]
      )
      dict entry(
         string "HostName"
         variant             string ""
      )
      dict entry(
         string "IgnoredDNSSearchPaths"
         variant             string "gateway.2wire.net"
      )
      dict entry(
         string "LinkMonitorTechnologies"
         variant             string "wifi"
      )
      dict entry(
         string "NoAutoConnectTechnologies"
         variant             string ""
      )
      dict entry(
         string "OfflineMode"
         variant             boolean false
      )
      dict entry(
         string "PortalCheckInterval"
         variant             int32 30
      )
      dict entry(
         string "PortalURL"
         variant             string "http://www.gstatic.com/generate_204"
      )
      dict entry(
         string "Profiles"
         variant             array [
               object path "/profile/default"
            ]
      )
      dict entry(
         string "ProhibitedTechnologies"
         variant             string ""
      )
      dict entry(
         string "ServiceCompleteList"
         variant             array [
               object path "/service/2"
               object path "/service/1"
               object path "/service/0"
            ]
      )
      dict entry(
         string "ServiceWatchList"
         variant             array [
               object path "/service/2"
            ]
      )
      dict entry(
         string "Services"
         variant             array [
               object path "/service/2"
            ]
      )
      dict entry(
         string "State"
         variant             string "online"
      )
      dict entry(
         string "UninitializedTechnologies"
         variant             array [
            ]
      )
      dict entry(
         string "WakeOnLanEnabled"
         variant             boolean true
      )
   ]
""")

PARSED_SHILL_GET_PROPERTIES_OUTPUT = {
    'ActiveProfile': '/profile/default',
    'ArpGateway': True,
    'AvailableTechnologies': ['ethernet'],
    'CheckPortalList': "''",
    'ConnectedTechnologies': ['ethernet'],
    'ConnectionState': 'online',
    'Country': '',
    'DefaultService': '/service/2',
    'DefaultTechnology': 'ethernet',
    'Devices': ['/device/eth0', '/device/eth1'],
    'DisableWiFiVHT': False,
    'EnabledTechnologies': ['ethernet'],
    'HostName': '',
    'IgnoredDNSSearchPaths': 'gateway.2wire.net',
    'LinkMonitorTechnologies': 'wifi',
    'NoAutoConnectTechnologies': '',
    'OfflineMode': False,
    'PortalCheckInterval': 30,
    'PortalURL': 'http://www.gstatic.com/generate_204',
    'Profiles': ['/profile/default'],
    'ProhibitedTechnologies': '',
    'ServiceCompleteList': ['/service/2', '/service/1', '/service/0'],
    'ServiceWatchList': ['/service/2'],
    'Services': ['/service/2'],
    'State': 'online',
    'UninitializedTechnologies': [],
    'WakeOnLanEnabled': True,
}

EXAMPLE_AVAHI_GET_STATE_OUTPUT = (
'method return sender=org.freedesktop.DBus -> destination=:1.40 serial=3 '
'reply_serial=2\n'
'   int32 2')

class DBusSendTest(unittest.TestCase):
    """Check that we're correctly parsing dbus-send output."""


    def testAvahiGetState(self):
        """Test that extremely simple input works."""
        result = dbus_send._parse_dbus_send_output(
            EXAMPLE_AVAHI_GET_STATE_OUTPUT)
        assert result.sender == 'org.freedesktop.DBus', (
            'Sender == %r' % result.sender)
        assert result.responder == ':1.40', 'Responder == %r' % result.responder
        assert result.response == 2, 'Response == %r' % result.response


    def testShillManagerGetProperties(self):
        """Test that we correctly parse fairly complex output.

        We could simply write expected == actual, but this lends
        itself to debugging a little more.

        """
        result = dbus_send._parse_dbus_send_output(
            EXAMPLE_SHILL_GET_PROPERTIES_OUTPUT)
        assert result.sender == 'org.freedesktop.DBus', (
            'Sender == %r' % result.sender)
        assert result.responder == ':1.37', 'Responder == %r' % result.responder
        for k, v in PARSED_SHILL_GET_PROPERTIES_OUTPUT.iteritems():
            assert k in result.response, '%r not in response' % k
            actual_v = result.response.pop(k)
            assert actual_v == v, 'Expected %r, got %r' % (v, actual_v)
        assert len(result.response) == 0, (
            'Got extra response: %r' % result.response)

    def testBuildArgString(self):
        """Test that we correctly form argument strings from dbus.* types."""
        self.assertEquals(dbus_send._build_arg_string(
            [dbus.Int16(42)]),
            'int16:42')
        self.assertEquals(dbus_send._build_arg_string(
            [dbus.Int16(42), dbus.Boolean(True)]),
            'int16:42 boolean:true')
        self.assertEquals(dbus_send._build_arg_string(
            [dbus.Int16(42), dbus.Boolean(True), dbus.String("foo")]),
            'int16:42 boolean:true string:foo')


if __name__ == "__main__":
    unittest.main()
