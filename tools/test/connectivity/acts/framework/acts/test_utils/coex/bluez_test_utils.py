# /usr/bin/env python3.4
#
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.

import dbus
import dbus.mainloop.glib
import dbus.service
import logging
import time

from acts.test_utils.coex.coex_constants import ADAPTER_INTERFACE
from acts.test_utils.coex.coex_constants import CALL_MANAGER
from acts.test_utils.coex.coex_constants import DBUS_INTERFACE
from acts.test_utils.coex.coex_constants import DEVICE_INTERFACE
from acts.test_utils.coex.coex_constants import DISCOVERY_TIME
from acts.test_utils.coex.coex_constants import MEDIA_CONTROL_INTERACE
from acts.test_utils.coex.coex_constants import MEDIA_PLAY_INTERFACE
from acts.test_utils.coex.coex_constants import OBJECT_MANGER
from acts.test_utils.coex.coex_constants import OFONO_MANAGER
from acts.test_utils.coex.coex_constants import PROPERTIES
from acts.test_utils.coex.coex_constants import PROPERTIES_CHANGED
from acts.test_utils.coex.coex_constants import SERVICE_NAME
from acts.test_utils.coex.coex_constants import VOICE_CALL
from acts.test_utils.coex.coex_constants import WAIT_TIME
from gi.repository import GObject

dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

class BluezUtils():

    def __init__(self):
        devices = {}
        self.device_interface = False
        self.mainloop = 0
        self.property_changed = False
        self.bd_address = None
        self.bus = dbus.SystemBus()
        self.bus.add_signal_receiver(self.properties_changed,
                                     dbus_interface=DBUS_INTERFACE,
                                     signal_name=PROPERTIES_CHANGED,
                                     arg0=DEVICE_INTERFACE,
                                     path_keyword="path")
        self.om = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, "/"), OBJECT_MANGER)
        objects = self.om.GetManagedObjects()
        for path, interfaces in objects.items():
            if ADAPTER_INTERFACE in interfaces:
                devices[path] = interfaces[ADAPTER_INTERFACE]
                self.adapter = self.find_adapter(0)

    def register_signal(self):
        """Start signal_dispatcher"""
        self.mainloop = GObject.MainLoop()
        self.mainloop.run()

    def unregister_signal(self):
        """Stops signal_dispatcher"""
        self.mainloop.quit()

    def get_properties(self,props, path, check):
        """Return's status for parameter check .

        Args:
            props:dbus interface
            path:path for getting status
            check:String for which status need to be checked
        """
        return props.Get(path,check)

    def properties_changed(self, interface, changed, invalidated, path):
        """
            Function to be executed when specified signal is caught
        """
        if path == "/org/bluez/hci0/dev_" + (self.bd_address).replace(":", "_"):
            self.unregister_signal()
            return

    def get_managed_objects(self):
        """Gets the instance of all the objects in dbus.

        Returns:
            Dictionary containing path and interface of
            all the instance in dbus.
        """
        manager = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, "/"), OBJECT_MANGER)
        return manager.GetManagedObjects()

    def find_adapter(self, pattern=None):
        """Gets the adapter interface with specified pattern in dbus.

        Args:
            pattern: Adapter name pattern to be found out.

        Returns:
             Adapter interface with specified pattern.
        """
        return self.find_adapter_in_objects(self.get_managed_objects(), pattern)

    def find_adapter_in_objects(self, objects, pattern=None):
        """Gets the adapter interface with specified pattern in dbus.

        Args:
            objects: Dictionary containing path and interface of
            all the instance in dbus.
            pattern: Adapter name pattern to be found out.

        Returns:
             Adapter interface if successful else raises an exception.
        """
        for path, ifaces in objects.items():
            adapter = ifaces.get(ADAPTER_INTERFACE)
            if adapter is None:
                continue
            if not pattern or pattern == adapter["Address"] or \
                    path.endswith(pattern):
                adapter_obj = self.bus.get_object(SERVICE_NAME, path)
                return dbus.Interface(adapter_obj, ADAPTER_INTERFACE)
        raise Exception("Bluetooth adapter not found")

    def find_device_in_objects(self,
                               objects,
                               device_address,
                               adapter_pattern=None):
        """Gets the device interface in objects with specified device
        address and pattern.

        Args:
            objects: Dictionary containing path and interface of
            all the instance in dbus.
            device_address: Bluetooth interface MAC address of the device
            which is to be found out.
            adapter_pattern: Adapter name pattern to be found out.

        Returns:
             Device interface if successful else raises an exception.
        """
        path_prefix = ""
        if adapter_pattern:
            adapter = self.find_adapter_in_objects(objects, adapter_pattern)
            path_prefix = adapter.object_path
        for path, ifaces in objects.items():
            device = ifaces.get(DEVICE_INTERFACE)
            if device is None:
                continue
            if (device["Address"] == device_address and
                    path.startswith(path_prefix)):
                device_obj = self.bus.get_object(SERVICE_NAME, path)
                return dbus.Interface(device_obj, DEVICE_INTERFACE)
        raise Exception("Bluetooth device not found")

    def get_bluetooth_adapter_address(self):
        """Gets the bluetooth adapter address.

        Returns:
            Address of bluetooth adapter.
        """
        path = self.adapter.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        address = props.Get(ADAPTER_INTERFACE, "Address")
        return address

    def find_device(self, device_address):
        """Discovers the DUT and returns its dbus interface.

        Args:
            Device_address: Bluetooth interface MAC address of the device.

        Returns:
            Dbus interface of the device.
        """
        addr = "dev_" + str(device_address).replace(":", "_")
        device_path = "org/bluez/hci0/" + addr
        self.adapter.StartDiscovery()
        time.sleep(DISCOVERY_TIME)
        objects = self.om.GetManagedObjects()
        for path, interfaces in objects.items():
            if device_path in path:
                obj = self.bus.get_object(SERVICE_NAME, path)
                self.device_interface = dbus.Interface(obj, DEVICE_INTERFACE)
                self.adapter.StopDiscovery()
        if not self.device_interface:
            self.adapter.StopDiscovery()
            return False
        return True

    def media_control_iface(self, device_address):
        """Gets the dbus media control interface for the device
        and returns it.

        Args:
            device_address: Bluetooth interface MAC address of the device.

        Returns:
            Dbus media control interface of the device.
        """
        control_iface = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, '/org/bluez/hci0/dev_' +
                                device_address.replace(":", "_")),
            MEDIA_CONTROL_INTERACE)
        return control_iface

    def get_a2dp_interface(self, device_address):
        """Gets the dbus media interface for the device.

        Args:
            device_address: Bluetooth interface MAC address of the device.

        Returns:
            Dbus media interface of the device.
        """
        a2dp_interface = dbus.Interface(
            self.bus.get_object(
                SERVICE_NAME, '/org/bluez/hci0/dev_' +
                              device_address.replace(":", "_") + '/player0'),
            MEDIA_PLAY_INTERFACE)
        return a2dp_interface

    def ofo_iface(self):
        """Gets dbus hfp interface for the device.

        Returns:
            Dbus hfp interface of the device.
        """
        manager = dbus.Interface(
            self.bus.get_object('org.ofono', '/'), OFONO_MANAGER)
        modems = manager.GetModems()
        return modems

    def call_manager(self, path):
        """Gets Ofono(HFP) interface for the device.

        Args:
            path: Ofono interface path of the device.

        Returns:
            Ofono interface for the device.
        """
        vcm = dbus.Interface(
            self.bus.get_object('org.ofono', path), CALL_MANAGER)
        return vcm

    def answer_call_interface(self, path):
        """Gets the voice call interface for the device.

        Args
            path: Voice call path of the device.

        Returns:
             Interface for the voice call.
        """
        call = dbus.Interface(
            self.bus.get_object('org.ofono', path), VOICE_CALL)
        return call

    def pair_bluetooth_device(self):
        """Pairs the bluez machine with DUT.

        Returns:
            True if pairing is successful else False.
        """
        self.device_interface.Pair()
        path = self.device_interface.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        paired = self.get_properties(props, DEVICE_INTERFACE, "Paired")
        return paired

    def connect_bluetooth_device(self, *args):
        """Connects the bluez machine to DUT with the specified
        profile.

        Args:
            uuid: Profile UUID which is to be connected.

        Returns:
            True if connection is successful else False.
        """

        self.register_signal()
        for uuid in args:
            self.device_interface.ConnectProfile(uuid)
        path = self.device_interface.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        connect = self.get_properties(props, DEVICE_INTERFACE, "Connected")
        return connect

    def disconnect_bluetooth_profile(self, uuid, pri_ad):
        """Disconnects the DUT for the specified profile.

        Args:
            uuid: Profile UUID which is to be disconnected.
            pri_ad: An android device object.

        Returns:
            True if disconnection of profile is successful else False.
        """

        self.register_signal()
        self.device_interface.DisconnectProfile(uuid)
        time.sleep(6)
        connected_devices = pri_ad.droid.bluetoothGetConnectedDevices()
        if len(connected_devices) > 0:
            return False
        return True

    def play_media(self, address):
        """Initiate media play for the specified device.

        Args:
            address: Bluetooth interface MAC address of the device.

        Returns:
            "playing" if successful else "stopped" or "paused".
        """
        self.register_signal()
        a2dp = self.media_control_iface(address)
        time.sleep(WAIT_TIME)
        a2dp.Play()
        play_pause = self.get_a2dp_interface(address)
        path = play_pause.object_path
        time.sleep(WAIT_TIME)
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        status = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Status")
        return status

    def pause_media(self, address):
        """Pauses the media palyer for the specified device.

        Args:
            address: Bluetooth interface MAC address of the device.

        Return:
            "paused" or "stopped" if successful else "playing".
        """
        self.register_signal()
        a2dp = self.get_a2dp_interface(address)
        time.sleep(WAIT_TIME)
        a2dp.Pause()
        path = a2dp.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        status = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Status")
        return status

    def remove_bluetooth_device(self, address):
        """Removes the device from the paired list.

        Args:
            address: Bluetooth interface MAC address of the device.

        Returns:
            True if removing of device is successful else False.
        """
        managed_objects = self.get_managed_objects()
        adapter = self.find_adapter_in_objects(managed_objects)
        try:
            dev = self.find_device_in_objects(managed_objects, address)
            path = dev.object_path
        except:
            return False

        adapter.RemoveDevice(path)
        return True

    def stop_media(self, address):
        """Stops the media player for the specified device.

        Args:
            address: Bluetooth interface MAC address of the device.

        Returns:
            "paused" or "stopped" if successful else "playing".
        """
        self.register_signal()
        a2dp = self.get_a2dp_interface(address)
        time.sleep(WAIT_TIME)
        a2dp.Stop()
        path = a2dp.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        status = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Status")
        return status

    def skip_next(self, address):
        """Skips to Next track in media player.

        Args:
            address: Bluetooth interface MAC address of the device.

        Returns:
            True if the media track change is successful else False.
        """
        self.register_signal()
        a2dp = self.get_a2dp_interface(address)
        time.sleep(WAIT_TIME)
        path = a2dp.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        track = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Track")
        Title = track['Title']
        a2dp.Next()
        time.sleep(WAIT_TIME)
        track = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Track")
        if Title == track['Title']:
            return False
        return True

    def skip_previous(self, address):
        """Skips to previous track in media player.

        Args:
            address: Buetooth interface MAC address of the device.

        Returns:
            True if media track change is successful else False.
        """
        a2dp = self.get_a2dp_interface(address)
        time.sleep(WAIT_TIME)
        path = a2dp.object_path
        props = dbus.Interface(
            self.bus.get_object(SERVICE_NAME, path), PROPERTIES)
        track = self.get_properties(props, MEDIA_PLAY_INTERFACE, "Track")
        Title = track['Title']
        a2dp.Previous()
        a2dp.Previous()
        time.sleep(WAIT_TIME)
        track = self.get_properties(props,MEDIA_PLAY_INTERFACE, "Track")
        if Title == track['Title']:
            return False
        return True

    def avrcp_actions(self, address):
        """Performs AVRCP actions for the device

        Args:
            address: Bluetooth interface MAC address of the device.

        Returns:
            True if avrcp actions are performed else False.
        """
        if not self.skip_next(address):
            logging.info("skip Next failed")
            return False
        time.sleep(WAIT_TIME)

        if not self.skip_previous(address):
            logging.info("skip previous failed")
            return False
        time.sleep(WAIT_TIME)
        return True

    def initiate_and_disconnect_call_from_hf(self, phone_no, duration):
        """Initiates the call from bluez for the specified phone number.

        Args:
            phone_no: Phone number to which the call should be made.
            duration: Time till which the call should be active.

        Returns:
             True if the call is initiated and disconnected else False.
        """
        modems = self.ofo_iface()
        modem = modems[0][0]
        hide_callerid = "default"
        vcm = self.call_manager(modem)
        time.sleep(WAIT_TIME)
        path = vcm.Dial(phone_no, hide_callerid)
        if 'voicecall' not in path:
            return False
        time.sleep(duration)
        vcm.HangupAll()
        return True

    def answer_call(self, duration):
        """Answers the incoming call from bluez.

        Args:
            duration: Time till which the call should be active.

        Returns:
             True if call is answered else False.
        """
        modems = self.ofo_iface()
        for path, properties in modems:
            if CALL_MANAGER not in properties["Interfaces"]:
                continue
            mgr = self.call_manager(path)
            calls = mgr.GetCalls()
            for path, properties in calls:
                state = properties["State"]
                if state != "incoming":
                    continue
                call = self.answer_call_interface(path)
                call.Answer()
                time.sleep(duration)
                call.Hangup()
        return True
