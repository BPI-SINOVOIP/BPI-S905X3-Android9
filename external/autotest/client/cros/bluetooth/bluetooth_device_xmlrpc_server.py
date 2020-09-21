#!/usr/bin/env python

# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import base64
import dbus
import dbus.mainloop.glib
import dbus.service
import gobject
import json
import logging
import logging.handlers
import os
import shutil

import common
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib.cros.bluetooth import bluetooth_socket
from autotest_lib.client.cros import constants
from autotest_lib.client.cros import xmlrpc_server
from autotest_lib.client.cros.bluetooth import advertisement
from autotest_lib.client.cros.bluetooth import output_recorder


def _dbus_byte_array_to_b64_string(dbus_byte_array):
    """Base64 encodes a dbus byte array for use with the xml rpc proxy."""
    return base64.standard_b64encode(bytearray(dbus_byte_array))


def _b64_string_to_dbus_byte_array(b64_string):
  """Base64 decodes a dbus byte array for use with the xml rpc proxy."""
  dbus_array = dbus.Array([], signature=dbus.Signature('y'))
  bytes = bytearray(base64.standard_b64decode(b64_string))
  for byte in bytes:
    dbus_array.append(dbus.Byte(byte))
  return dbus_array


class PairingAgent(dbus.service.Object):
    """The agent handling the authentication process of bluetooth pairing.

    PairingAgent overrides RequestPinCode method to return a given pin code.
    User can use this agent to pair bluetooth device which has a known
    pin code.

    TODO (josephsih): more pairing modes other than pin code would be
    supported later.

    """

    def __init__(self, pin, *args, **kwargs):
        super(PairingAgent, self).__init__(*args, **kwargs)
        self._pin = pin


    @dbus.service.method('org.bluez.Agent1',
                         in_signature='o', out_signature='s')
    def RequestPinCode(self, device_path):
        """Requests pin code for a device.

        Returns the known pin code for the request.

        @param device_path: The object path of the device.

        @returns: The known pin code.

        """
        logging.info('RequestPinCode for %s; return %s', device_path, self._pin)
        return self._pin


class BluetoothDeviceXmlRpcDelegate(xmlrpc_server.XmlRpcDelegate):
    """Exposes DUT methods called remotely during Bluetooth autotests.

    All instance methods of this object without a preceding '_' are exposed via
    an XML-RPC server. This is not a stateless handler object, which means that
    if you store state inside the delegate, that state will remain around for
    future calls.
    """

    UPSTART_PATH = 'unix:abstract=/com/ubuntu/upstart'
    UPSTART_MANAGER_PATH = '/com/ubuntu/Upstart'
    UPSTART_MANAGER_IFACE = 'com.ubuntu.Upstart0_6'
    UPSTART_JOB_IFACE = 'com.ubuntu.Upstart0_6.Job'

    UPSTART_ERROR_UNKNOWNINSTANCE = \
            'com.ubuntu.Upstart0_6.Error.UnknownInstance'
    UPSTART_ERROR_ALREADYSTARTED = \
            'com.ubuntu.Upstart0_6.Error.AlreadyStarted'

    BLUETOOTHD_JOB = 'bluetoothd'

    DBUS_ERROR_SERVICEUNKNOWN = 'org.freedesktop.DBus.Error.ServiceUnknown'

    BLUEZ_SERVICE_NAME = 'org.bluez'
    BLUEZ_MANAGER_PATH = '/'
    BLUEZ_MANAGER_IFACE = 'org.freedesktop.DBus.ObjectManager'
    BLUEZ_ADAPTER_IFACE = 'org.bluez.Adapter1'
    BLUEZ_DEVICE_IFACE = 'org.bluez.Device1'
    BLUEZ_GATT_IFACE = 'org.bluez.GattCharacteristic1'
    BLUEZ_LE_ADVERTISING_MANAGER_IFACE = 'org.bluez.LEAdvertisingManager1'
    BLUEZ_AGENT_MANAGER_PATH = '/org/bluez'
    BLUEZ_AGENT_MANAGER_IFACE = 'org.bluez.AgentManager1'
    BLUEZ_PROFILE_MANAGER_PATH = '/org/bluez'
    BLUEZ_PROFILE_MANAGER_IFACE = 'org.bluez.ProfileManager1'
    BLUEZ_ERROR_ALREADY_EXISTS = 'org.bluez.Error.AlreadyExists'
    DBUS_PROP_IFACE = 'org.freedesktop.DBus.Properties'
    AGENT_PATH = '/test/agent'

    BLUETOOTH_LIBDIR = '/var/lib/bluetooth'
    BTMON_STOP_DELAY_SECS = 3

    # Timeout for how long we'll wait for BlueZ and the Adapter to show up
    # after reset.
    ADAPTER_TIMEOUT = 30

    def __init__(self):
        super(BluetoothDeviceXmlRpcDelegate, self).__init__()

        # Open the Bluetooth Raw socket to the kernel which provides us direct,
        # raw, access to the HCI controller.
        self._raw = bluetooth_socket.BluetoothRawSocket()

        # Open the Bluetooth Control socket to the kernel which provides us
        # raw management access to the Bluetooth Host Subsystem. Read the list
        # of adapter indexes to determine whether or not this device has a
        # Bluetooth Adapter or not.
        self._control = bluetooth_socket.BluetoothControlSocket()
        self._has_adapter = len(self._control.read_index_list()) > 0

        # Set up the connection to Upstart so we can start and stop services
        # and fetch the bluetoothd job.
        self._upstart_conn = dbus.connection.Connection(self.UPSTART_PATH)
        self._upstart = self._upstart_conn.get_object(
                None,
                self.UPSTART_MANAGER_PATH)

        bluetoothd_path = self._upstart.GetJobByName(
                self.BLUETOOTHD_JOB,
                dbus_interface=self.UPSTART_MANAGER_IFACE)
        self._bluetoothd = self._upstart_conn.get_object(
                None,
                bluetoothd_path)

        # Arrange for the GLib main loop to be the default.
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

        # Set up the connection to the D-Bus System Bus, get the object for
        # the Bluetooth Userspace Daemon (BlueZ) and that daemon's object for
        # the Bluetooth Adapter, and the advertising manager.
        self._system_bus = dbus.SystemBus()
        self._update_bluez()
        self._update_adapter()
        self._update_advertising()

        # The agent to handle pin code request, which will be
        # created when user calls pair_legacy_device method.
        self._pairing_agent = None
        # The default capability of the agent.
        self._capability = 'KeyboardDisplay'

        # Initailize a btmon object to record bluetoothd's activity.
        self.btmon = output_recorder.OutputRecorder(
                'btmon', stop_delay_secs=self.BTMON_STOP_DELAY_SECS)

        self.advertisements = []
        self._adv_mainloop = gobject.MainLoop()


    @xmlrpc_server.dbus_safe(False)
    def start_bluetoothd(self):
        """start bluetoothd.

        This includes powering up the adapter.

        @returns: True if bluetoothd is started correctly.
                  False otherwise.

        """
        try:
            self._bluetoothd.Start(dbus.Array(signature='s'), True,
                                   dbus_interface=self.UPSTART_JOB_IFACE)
        except dbus.exceptions.DBusException as e:
            # if bluetoothd was already started, the exception looks like
            #     dbus.exceptions.DBusException:
            #     com.ubuntu.Upstart0_6.Error.AlreadyStarted: Job is already
            #     running: bluetoothd
            if e.get_dbus_name() != self.UPSTART_ERROR_ALREADYSTARTED:
                logging.error('Error starting bluetoothd: %s', e)
                return False

        logging.debug('waiting for bluez start')
        try:
            utils.poll_for_condition(
                    condition=self._update_bluez,
                    desc='Bluetooth Daemon has started.',
                    timeout=self.ADAPTER_TIMEOUT)
        except Exception as e:
            logging.error('timeout: error starting bluetoothd: %s', e)
            return False

        # Waiting for the self._adapter object.
        # This does not mean that the adapter is powered on.
        logging.debug('waiting for bluez to obtain adapter information')
        try:
            utils.poll_for_condition(
                    condition=self._update_adapter,
                    desc='Bluetooth Daemon has adapter information.',
                    timeout=self.ADAPTER_TIMEOUT)
        except Exception as e:
            logging.error('timeout: error starting adapter: %s', e)
            return False

        # Waiting for the self._advertising interface object.
        logging.debug('waiting for bluez to obtain interface manager.')
        try:
            utils.poll_for_condition(
                    condition=self._update_advertising,
                    desc='Bluetooth Daemon has advertising interface.',
                    timeout=self.ADAPTER_TIMEOUT)
        except utils.TimeoutError:
            logging.error('timeout: error getting advertising interface')
            return False

        return True


    @xmlrpc_server.dbus_safe(False)
    def stop_bluetoothd(self):
        """stop bluetoothd.

        @returns: True if bluetoothd is stopped correctly.
                  False otherwise.

        """
        def bluez_stopped():
            """Checks the bluetooth daemon status.

            @returns: True if bluez is stopped. False otherwise.

            """
            return not self._update_bluez()

        try:
            self._bluetoothd.Stop(dbus.Array(signature='s'), True,
                                  dbus_interface=self.UPSTART_JOB_IFACE)
        except dbus.exceptions.DBusException as e:
            # If bluetoothd was stopped already, the exception looks like
            #    dbus.exceptions.DBusException:
            #    com.ubuntu.Upstart0_6.Error.UnknownInstance: Unknown instance:
            if e.get_dbus_name() != self.UPSTART_ERROR_UNKNOWNINSTANCE:
                logging.error('Error stopping bluetoothd!')
                return False

        logging.debug('waiting for bluez stop')
        try:
            utils.poll_for_condition(
                    condition=bluez_stopped,
                    desc='Bluetooth Daemon has stopped.',
                    timeout=self.ADAPTER_TIMEOUT)
            bluetoothd_stopped = True
        except Exception as e:
            logging.error('timeout: error stopping bluetoothd: %s', e)
            bluetoothd_stopped = False

        return bluetoothd_stopped


    def is_bluetoothd_running(self):
        """Is bluetoothd running?

        @returns: True if bluetoothd is running

        """
        return bool(self._get_dbus_proxy_for_bluetoothd())


    def _update_bluez(self):
        """Store a D-Bus proxy for the Bluetooth daemon in self._bluez.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready after it has been started.

        @return True on success, False otherwise.

        """
        self._bluez = self._get_dbus_proxy_for_bluetoothd()
        return bool(self._bluez)


    @xmlrpc_server.dbus_safe(False)
    def _get_dbus_proxy_for_bluetoothd(self):
        """Get the D-Bus proxy for the Bluetooth daemon.

        @return True on success, False otherwise.

        """
        bluez = None
        try:
            bluez = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                self.BLUEZ_MANAGER_PATH)
            logging.debug('bluetoothd is running')
        except dbus.exceptions.DBusException as e:
            # When bluetoothd is not running, the exception looks like
            #     dbus.exceptions.DBusException:
            #     org.freedesktop.DBus.Error.ServiceUnknown: The name org.bluez
            #     was not provided by any .service files
            if e.get_dbus_name() == self.DBUS_ERROR_SERVICEUNKNOWN:
                logging.debug('bluetoothd is not running')
            else:
                logging.error('Error getting dbus proxy for Bluez: %s', e)
        return bluez


    def _update_adapter(self):
        """Store a D-Bus proxy for the local adapter in self._adapter.

        This may be called in a loop until it returns True to wait for the
        daemon to be ready, and have obtained the adapter information itself,
        after it has been started.

        Since not all devices will have adapters, this will also return True
        in the case where we have obtained an empty adapter index list from the
        kernel.

        Note that this method does not power on the adapter.

        @return True on success, including if there is no local adapter,
            False otherwise.

        """
        self._adapter = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning')
            return True
        self._adapter = self._get_adapter()
        return bool(self._adapter)

    def _update_advertising(self):
        """Store a D-Bus proxy for the local advertising interface manager.

        This may be called repeatedly in a loop until True is returned;
        otherwise we wait for bluetoothd to start. After bluetoothd starts, we
        check the existence of a local adapter and proceed to get the
        advertisement interface manager.

        Since not all devices will have adapters, this will also return True
        in the case where there is no adapter.

        @return True on success, including if there is no local adapter,
                False otherwise.

        """
        self._advertising = None
        if self._bluez is None:
            logging.warning('Bluez not found!')
            return False
        if not self._has_adapter:
            logging.debug('Device has no adapter; returning')
            return True
        self._advertising = self._get_advertising()
        return bool(self._advertising)


    @xmlrpc_server.dbus_safe(False)
    def _get_adapter(self):
        """Get the D-Bus proxy for the local adapter.

        @return the adapter on success. None otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for path, ifaces in objects.iteritems():
            logging.debug('%s -> %r', path, ifaces.keys())
            if self.BLUEZ_ADAPTER_IFACE in ifaces:
                logging.debug('using adapter %s', path)
                adapter = self._system_bus.get_object(
                        self.BLUEZ_SERVICE_NAME,
                        path)
                return adapter
        else:
            logging.warning('No adapter found in interface!')
            return None


    @xmlrpc_server.dbus_safe(False)
    def _get_advertising(self):
        """Get the D-Bus proxy for the local advertising interface.

        @return the advertising interface object.

        """
        return dbus.Interface(self._adapter,
                              self.BLUEZ_LE_ADVERTISING_MANAGER_IFACE)


    @xmlrpc_server.dbus_safe(False)
    def reset_on(self):
        """Reset the adapter and settings and power up the adapter.

        @return True on success, False otherwise.

        """
        return self._reset(set_power=True)


    @xmlrpc_server.dbus_safe(False)
    def reset_off(self):
        """Reset the adapter and settings, leave the adapter powered off.

        @return True on success, False otherwise.

        """
        return self._reset(set_power=False)


    def has_adapter(self):
        """Return if an adapter is present.

        This will only return True if we have determined both that there is
        a Bluetooth adapter on this device (kernel adapter index list is not
        empty) and that the Bluetooth daemon has exported an object for it.

        @return True if an adapter is present, False if not.

        """
        return self._has_adapter and self._adapter is not None


    def _reset(self, set_power=False):
        """Remove remote devices and set adapter to set_power state.

        Do not restart bluetoothd as this may incur a side effect.
        The unhappy chrome may disable the adapter randomly.

        @param set_power: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        logging.debug('_reset')

        if not self._adapter:
            logging.warning('Adapter not found!')
            return False

        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)

        devices = []
        for path, ifaces in objects.iteritems():
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                devices.append(objects[path][self.BLUEZ_DEVICE_IFACE])

        # Turn on the adapter in order to remove all remote devices.
        if not self._is_powered_on():
            self._set_powered(True)

        for device in devices:
            logging.debug('removing %s', device.get('Address'))
            self.remove_device_object(device.get('Address'))

        if not set_power:
            self._set_powered(False)

        return True


    @xmlrpc_server.dbus_safe(False)
    def set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        @return True on success, False otherwise.

        """
        if not self._adapter:
            if not powered:
                # Return success if we are trying to power off an adapter that's
                # missing or gone away, since the expected result has happened.
                return True
            else:
                logging.warning('Adapter not found!')
                return False
        self._set_powered(powered)
        return True


    @xmlrpc_server.dbus_safe(False)
    def _set_powered(self, powered):
        """Set the adapter power state.

        @param powered: adapter power state to set (True or False).

        """
        logging.debug('_set_powered %r', powered)
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE, 'Powered', powered,
                          dbus_interface=dbus.PROPERTIES_IFACE)


    @xmlrpc_server.dbus_safe(False)
    def set_discoverable(self, discoverable):
        """Set the adapter discoverable state.

        @param discoverable: adapter discoverable state to set (True or False).

        @return True on success, False otherwise.

        """
        if not discoverable and not self._adapter:
            # Return success if we are trying to make an adapter that's
            # missing or gone away, undiscoverable, since the expected result
            # has happened.
            return True
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE,
                          'Discoverable', discoverable,
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def set_pairable(self, pairable):
        """Set the adapter pairable state.

        @param pairable: adapter pairable state to set (True or False).

        @return True on success, False otherwise.

        """
        self._adapter.Set(self.BLUEZ_ADAPTER_IFACE, 'Pairable', pairable,
                          dbus_interface=dbus.PROPERTIES_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def _get_adapter_properties(self):
        """Read the adapter properties from the Bluetooth Daemon.

        @return the properties as a JSON-encoded dictionary on success,
            the value False otherwise.

        """
        if self._bluez:
            objects = self._bluez.GetManagedObjects(
                    dbus_interface=self.BLUEZ_MANAGER_IFACE)
            props = objects[self._adapter.object_path][self.BLUEZ_ADAPTER_IFACE]
        else:
            props = {}
        logging.debug('get_adapter_properties: %s', props)
        return props


    def get_adapter_properties(self):
        return json.dumps(self._get_adapter_properties())


    def _is_powered_on(self):
        return bool(self._get_adapter_properties().get(u'Powered'))


    def read_version(self):
        """Read the version of the management interface from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( version, revision )

        """
        return json.dumps(self._control.read_version())


    def read_supported_commands(self):
        """Read the set of supported commands from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( commands, events )

        """
        return json.dumps(self._control.read_supported_commands())


    def read_index_list(self):
        """Read the list of currently known controllers from the Kernel.

        @return the information as a JSON-encoded array of controller indexes.

        """
        return json.dumps(self._control.read_index_list())


    def read_info(self):
        """Read the adapter information from the Kernel.

        @return the information as a JSON-encoded tuple of:
          ( address, bluetooth_version, manufacturer_id,
            supported_settings, current_settings, class_of_device,
            name, short_name )

        """
        return json.dumps(self._control.read_info(0))


    def add_device(self, address, address_type, action):
        """Add a device to the Kernel action list.

        @param address: Address of the device to add.
        @param address_type: Type of device in @address.
        @param action: Action to take.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        return json.dumps(self._control.add_device(
                0, address, address_type, action))


    def remove_device(self, address, address_type):
        """Remove a device from the Kernel action list.

        @param address: Address of the device to remove.
        @param address_type: Type of device in @address.

        @return on success, a JSON-encoded typle of:
          ( address, address_type ), None on failure.

        """
        return json.dumps(self._control.remove_device(
                0, address, address_type))


    @xmlrpc_server.dbus_safe(False)
    def get_devices(self):
        """Read information about remote devices known to the adapter.

        @return the properties of each device as a JSON-encoded array of
            dictionaries on success, the value False otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)
        devices = []
        for path, ifaces in objects.iteritems():
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                devices.append(objects[path][self.BLUEZ_DEVICE_IFACE])
        return json.dumps(devices)


    @xmlrpc_server.dbus_safe(False)
    def get_device_by_address(self, address):
        """Read information about the remote device with the specified address.

        @param address: Address of the device to get.

        @return the properties of the device as a JSON-encoded dictionary
            on success, the value False otherwise.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=True)
        devices = []
        for path, ifaces in objects.iteritems():
            if self.BLUEZ_DEVICE_IFACE in ifaces:
                device = objects[path][self.BLUEZ_DEVICE_IFACE]
                if device.get('Address') == address:
                    return json.dumps(device)

        devices = json.loads(self.get_devices())
        for device in devices:
            if device.get['Address'] == address:
                return json.dumps(device)
        return json.dumps(dict())


    @xmlrpc_server.dbus_safe(False)
    def start_discovery(self):
        """Start discovery of remote devices.

        Obtain the discovered device information using get_devices(), called
        stop_discovery() when done.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return False
        self._adapter.StartDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def stop_discovery(self):
        """Stop discovery of remote devices.

        @return True on success, False otherwise.

        """
        if not self._adapter:
            return False
        self._adapter.StopDiscovery(dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    def get_dev_info(self):
        """Read raw HCI device information.

        @return JSON-encoded tuple of:
                (index, name, address, flags, device_type, bus_type,
                       features, pkt_type, link_policy, link_mode,
                       acl_mtu, acl_pkts, sco_mtu, sco_pkts,
                       err_rx, err_tx, cmd_tx, evt_rx, acl_tx, acl_rx,
                       sco_tx, sco_rx, byte_rx, byte_tx) on success,
                None on failure.

        """
        return json.dumps(self._raw.get_dev_info(0))


    @xmlrpc_server.dbus_safe(False)
    def register_profile(self, path, uuid, options):
        """Register new profile (service).

        @param path: Path to the profile object.
        @param uuid: Service Class ID of the service as string.
        @param options: Dictionary of options for the new service, compliant
                        with BlueZ D-Bus Profile API standard.

        @return True on success, False otherwise.

        """
        profile_manager = dbus.Interface(
                              self._system_bus.get_object(
                                  self.BLUEZ_SERVICE_NAME,
                                  self.BLUEZ_PROFILE_MANAGER_PATH),
                              self.BLUEZ_PROFILE_MANAGER_IFACE)
        profile_manager.RegisterProfile(path, uuid, options)
        return True


    def has_device(self, address):
        """Checks if the device with a given address exists.

        @param address: Address of the device.

        @returns: True if there is an interface object with that address.
                  False if the device is not found.

        @raises: Exception if a D-Bus error is encountered.

        """
        result = self._find_device(address)
        logging.debug('has_device result: %s', str(result))

        # The result being False indicates that there is a D-Bus error.
        if result is False:
            raise Exception('dbus.Interface error')

        # Return True if the result is not None, e.g. a D-Bus interface object;
        # False otherwise.
        return bool(result)


    @xmlrpc_server.dbus_safe(False)
    def _find_device(self, address):
        """Finds the device with a given address.

        Find the device with a given address and returns the
        device interface.

        @param address: Address of the device.

        @returns: An 'org.bluez.Device1' interface to the device.
                  None if device can not be found.
        """
        path = self._get_device_path(address)
        if path:
            obj = self._system_bus.get_object(
                        self.BLUEZ_SERVICE_NAME, path)
            return dbus.Interface(obj, self.BLUEZ_DEVICE_IFACE)
        logging.info('Device not found')
        return None


    @xmlrpc_server.dbus_safe(False)
    def _get_device_path(self, address):
        """Gets the path for a device with a given address.

        Find the device with a given address and returns the
        the path for the device.

        @param address: Address of the device.

        @returns: The path to the address of the device, or None if device is
            not found in the object tree.

        """
        objects = self._bluez.GetManagedObjects(
                dbus_interface=self.BLUEZ_MANAGER_IFACE)
        for path, ifaces in objects.iteritems():
            device = ifaces.get(self.BLUEZ_DEVICE_IFACE)
            if device is None:
                continue
            if (device['Address'] == address and
                path.startswith(self._adapter.object_path)):
                return path
        logging.info('Device path not found')


    @xmlrpc_server.dbus_safe(False)
    def _setup_pairing_agent(self, pin):
        """Initializes and resiters a PairingAgent to handle authentication.

        @param pin: The pin code this agent will answer.

        """
        if self._pairing_agent:
            logging.info('Removing the old agent before initializing a new one')
            self._pairing_agent.remove_from_connection()
            self._pairing_agent = None
        self._pairing_agent= PairingAgent(pin, self._system_bus,
                                          self.AGENT_PATH)
        agent_manager = dbus.Interface(
                self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                            self.BLUEZ_AGENT_MANAGER_PATH),
                self.BLUEZ_AGENT_MANAGER_IFACE)
        try:
            agent_manager.RegisterAgent(self.AGENT_PATH, self._capability)
        except dbus.exceptions.DBusException, e:
            if e.get_dbus_name() == self.BLUEZ_ERROR_ALREADY_EXISTS:
                logging.info('Unregistering old agent and registering the new')
                agent_manager.UnregisterAgent(self.AGENT_PATH)
                agent_manager.RegisterAgent(self.AGENT_PATH, self._capability)
            else:
                logging.error('Error setting up pin agent: %s', e)
                raise
        logging.info('Agent registered: %s', self.AGENT_PATH)


    @xmlrpc_server.dbus_safe(False)
    def _is_paired(self,  device):
        """Checks if a device is paired.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is paired. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        paired = props.Get(self.BLUEZ_DEVICE_IFACE, 'Paired')
        return bool(paired)


    @xmlrpc_server.dbus_safe(False)
    def device_is_paired(self, address):
        """Checks if a device is paired.

        @param address: address of the device.

        @returns: True if device is paired. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        return self._is_paired(device)


    @xmlrpc_server.dbus_safe(False)
    def _is_connected(self, device):
        """Checks if a device is connected.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is connected. False otherwise.

        """
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        connected = props.Get(self.BLUEZ_DEVICE_IFACE, 'Connected')
        logging.info('Got connected = %r', connected)
        return bool(connected)



    @xmlrpc_server.dbus_safe(False)
    def _set_trusted_by_device(self, device, trusted=True):
        """Set the device trusted by device object.

        @param device: the device object to set trusted.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            properties = dbus.Interface(device, self.DBUS_PROP_IFACE)
            properties.Set(self.BLUEZ_DEVICE_IFACE, 'Trusted', trusted)
            return True
        except Exception as e:
            logging.error('_set_trusted_by_device: %s', e)
        except:
            logging.error('_set_trusted_by_device: unexpected error')
        return False


    @xmlrpc_server.dbus_safe(False)
    def _set_trusted_by_path(self, device_path, trusted=True):
        """Set the device trusted by the device path.

        @param device_path: the object path of the device.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            device = self._system_bus.get_object(self.BLUEZ_SERVICE_NAME,
                                                 device_path)
            return self._set_trusted_by_device(device, trusted)
        except Exception as e:
            logging.error('_set_trusted_by_path: %s', e)
        except:
            logging.error('_set_trusted_by_path: unexpected error')
        return False


    @xmlrpc_server.dbus_safe(False)
    def set_trusted(self, address, trusted=True):
        """Set the device trusted by address.

        @param address: The bluetooth address of the device.
        @param trusted: True or False indicating whether to set trusted or not.

        @returns: True if successful. False otherwise.

        """
        try:
            device = self._find_device(address)
            return self._set_trusted_by_device(device, trusted)
        except Exception as e:
            logging.error('set_trusted: %s', e)
        except:
            logging.error('set_trusted: unexpected error')
        return False


    @xmlrpc_server.dbus_safe(False)
    def pair_legacy_device(self, address, pin, trusted, timeout=60):
        """Pairs a device with a given pin code.

        Registers a agent who handles pin code request and
        pairs a device with known pin code.

        Note that the adapter does not automatically connnect to the device
        when pairing is done. The connect_device() method has to be invoked
        explicitly to connect to the device. This provides finer control
        for testing purpose.

        @param address: Address of the device to pair.
        @param pin: The pin code of the device to pair.
        @param trusted: indicating whether to set the device trusted.
        @param timeout: The timeout in seconds for pairing.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if self._is_paired(device):
            logging.info('Device is already paired')
            return True

        device_path = device.object_path
        logging.info('Device %s is found.', device.object_path)

        self._setup_pairing_agent(pin)
        mainloop = gobject.MainLoop()


        def pair_reply():
            """Handler when pairing succeeded."""
            logging.info('Device paired: %s', device_path)
            if trusted:
                self._set_trusted_by_path(device_path, trusted=True)
                logging.info('Device trusted: %s', device_path)
            mainloop.quit()


        def pair_error(error):
            """Handler when pairing failed.

            @param error: one of errors defined in org.bluez.Error representing
                          the error in pairing.

            """
            try:
                error_name = error.get_dbus_name()
                if error_name == 'org.freedesktop.DBus.Error.NoReply':
                    logging.error('Timed out after %d ms. Cancelling pairing.',
                                  timeout)
                    device.CancelPairing()
                else:
                    logging.error('Pairing device failed: %s', error)
            finally:
                mainloop.quit()


        device.Pair(reply_handler=pair_reply, error_handler=pair_error,
                    timeout=timeout * 1000)
        mainloop.run()
        return self._is_paired(device)


    @xmlrpc_server.dbus_safe(False)
    def remove_device_object(self, address):
        """Removes a device object and the pairing information.

        Calls RemoveDevice method to remove remote device
        object and the pairing information.

        @param address: Address of the device to unpair.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        self._adapter.RemoveDevice(
                device.object_path, dbus_interface=self.BLUEZ_ADAPTER_IFACE)
        return True


    @xmlrpc_server.dbus_safe(False)
    def connect_device(self, address):
        """Connects a device.

        Connects a device if it is not connected.

        @param address: Address of the device to connect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if self._is_connected(device):
          logging.info('Device is already connected')
          return True
        device.Connect()
        return self._is_connected(device)


    @xmlrpc_server.dbus_safe(False)
    def device_is_connected(self, address):
        """Checks if a device is connected.

        @param address: Address of the device to connect.

        @returns: True if device is connected. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        return self._is_connected(device)


    @xmlrpc_server.dbus_safe(False)
    def disconnect_device(self, address):
        """Disconnects a device.

        Disconnects a device if it is connected.

        @param address: Address of the device to disconnect.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False
        if not self._is_connected(device):
          logging.info('Device is not connected')
          return True
        device.Disconnect()
        return not self._is_connected(device)


    @xmlrpc_server.dbus_safe(False)
    def _device_services_resolved(self, device):
        """Checks if services are resolved.

        @param device: An 'org.bluez.Device1' interface to the device.

        @returns: True if device is connected. False otherwise.

        """
        logging.info('device for services resolved: %s', device)
        props = dbus.Interface(device, dbus.PROPERTIES_IFACE)
        resolved = props.Get(self.BLUEZ_DEVICE_IFACE, 'ServicesResolved')
        logging.info('Services resolved = %r', resolved)
        return bool(resolved)


    @xmlrpc_server.dbus_safe(False)
    def device_services_resolved(self, address):
        """Checks if service discovery is complete on a device.

        Checks whether service discovery has been completed..

        @param address: Address of the remote device.

        @returns: True on success. False otherwise.

        """
        device = self._find_device(address)
        if not device:
            logging.error('Device not found')
            return False

        if not self._is_connected(device):
          logging.info('Device is not connected')
          return False

        return self._device_services_resolved(device)


    def btmon_start(self):
        """Start btmon monitoring."""
        self.btmon.start()


    def btmon_stop(self):
        """Stop btmon monitoring."""
        self.btmon.stop()


    def btmon_get(self, search_str, start_str):
        """Get btmon output contents.

        @param search_str: only lines with search_str would be kept.
        @param start_str: all lines before the occurrence of start_str would be
                filtered.

        @returns: the recorded btmon output.

        """
        return self.btmon.get_contents(search_str=search_str,
                                       start_str=start_str)


    def btmon_find(self, pattern_str):
        """Find if a pattern string exists in btmon output.

        @param pattern_str: the pattern string to find.

        @returns: True on success. False otherwise.

        """
        return self.btmon.find(pattern_str)


    @xmlrpc_server.dbus_safe(False)
    def advertising_async_method(self, dbus_method,
                                 reply_handler, error_handler, *args):
        """Run an async dbus method.

        @param dbus_method: the dbus async method to invoke.
        @param reply_handler: the reply handler for the dbus method.
        @param error_handler: the error handler for the dbus method.
        @param *args: additional arguments for the dbus method.

        @returns: an empty string '' on success;
                  None if there is no _advertising interface manager; and
                  an error string if the dbus method fails.

        """

        def successful_cb():
            """Called when the dbus_method completed successfully."""
            reply_handler()
            self.advertising_cb_msg = ''
            self._adv_mainloop.quit()


        def error_cb(error):
            """Called when the dbus_method failed."""
            error_handler(error)
            self.advertising_cb_msg = str(error)
            self._adv_mainloop.quit()


        if not self._advertising:
            return None

        # Call dbus_method with handlers.
        dbus_method(*args, reply_handler=successful_cb, error_handler=error_cb)

        self._adv_mainloop.run()

        return self.advertising_cb_msg


    def register_advertisement(self, advertisement_data):
        """Register an advertisement.

        Note that rpc supports only conformable types. Hence, a
        dict about the advertisement is passed as a parameter such
        that the advertisement object could be constructed on the host.

        @param advertisement_data: a dict of the advertisement to register.

        @returns: True on success. False otherwise.

        """
        adv = advertisement.Advertisement(self._system_bus, advertisement_data)
        self.advertisements.append(adv)
        return self.advertising_async_method(
                self._advertising.RegisterAdvertisement,
                # reply handler
                lambda: logging.info('register_advertisement: succeeded.'),
                # error handler
                lambda error: logging.error(
                    'register_advertisement: failed: %s', str(error)),
                # other arguments
                adv.get_path(), {})


    def unregister_advertisement(self, advertisement_data):
        """Unregister an advertisement.

        Note that to unregister an advertisement, it is required to use
        the same self._advertising interface manager. This is because
        bluez only allows the same sender to invoke UnregisterAdvertisement
        method. Hence, watch out that the bluetoothd is not restarted or
        self.start_bluetoothd() is not executed between the time span that
        an advertisement is registered and unregistered.

        @param advertisement_data: a dict of the advertisements to unregister.

        @returns: True on success. False otherwise.

        """
        path = advertisement_data.get('Path')
        for index, adv in enumerate(self.advertisements):
            if adv.get_path() == path:
                break
        else:
            logging.error('Fail to find the advertisement under the path: %s',
                          path)
            return False

        result = self.advertising_async_method(
                self._advertising.UnregisterAdvertisement,
                # reply handler
                lambda: logging.info('unregister_advertisement: succeeded.'),
                # error handler
                lambda error: logging.error(
                    'unregister_advertisement: failed: %s', str(error)),
                # other arguments
                adv.get_path())

        # Call remove_from_connection() so that the same path could be reused.
        adv.remove_from_connection()
        del self.advertisements[index]

        return result


    def set_advertising_intervals(self, min_adv_interval_ms,
                                  max_adv_interval_ms):
        """Set advertising intervals.

        @param min_adv_interval_ms: the min advertising interval in ms.
        @param max_adv_interval_ms: the max advertising interval in ms.

        @returns: True on success. False otherwise.

        """
        return self.advertising_async_method(
                self._advertising.SetAdvertisingIntervals,
                # reply handler
                lambda: logging.info('set_advertising_intervals: succeeded.'),
                # error handler
                lambda error: logging.error(
                    'set_advertising_intervals: failed: %s', str(error)),
                # other arguments
                min_adv_interval_ms, max_adv_interval_ms)


    def reset_advertising(self):
        """Reset advertising.

        This includes un-registering all advertisements, reset advertising
        intervals, and disable advertising.

        @returns: True on success. False otherwise.

        """
        # It is required to execute remove_from_connection() to unregister the
        # object-path handler of each advertisement. In this way, we could
        # register an advertisement with the same path repeatedly.
        for adv in self.advertisements:
            adv.remove_from_connection()
        del self.advertisements[:]

        return self.advertising_async_method(
                self._advertising.ResetAdvertising,
                # reply handler
                lambda: logging.info('reset_advertising: succeeded.'),
                # error handler
                lambda error: logging.error(
                    'reset_advertising: failed: %s', str(error)))


    @xmlrpc_server.dbus_safe(False)
    def get_characteristic_map(self, address):
        """Gets a map of characteristic paths for a device.

        Walks the object tree, and returns a map of uuids to object paths for
        all resolved gatt characteristics.

        @param address: The MAC address of the device to retrieve
            gatt characteristic uuids and paths from.

        @returns: A dictionary of characteristic paths, keyed by uuid.

        """
        device_path = self._get_device_path(address)
        char_map = {}

        if device_path:
          objects = self._bluez.GetManagedObjects(
              dbus_interface=self.BLUEZ_MANAGER_IFACE, byte_arrays=False)

          for path, ifaces in objects.iteritems():
              if (self.BLUEZ_GATT_IFACE in ifaces and
                  path.startswith(device_path)):
                  uuid = ifaces[self.BLUEZ_GATT_IFACE]['UUID'].lower()
                  char_map[uuid] = path
        else:
            logging.warning('Device %s not in object tree.', address)

        return char_map


    @xmlrpc_server.dbus_safe(False)
    def _get_char_object(self, uuid, address):
        """Gets a characteristic object.

        Gets a characteristic object for a given uuid and address.

        @param uuid: The uuid of the characteristic, as a string.
        @param address: The MAC address of the remote device.

        @returns: A dbus interface for the characteristic if the uuid/address
                      is in the object tree.
                  None if the address/uuid is not found in the object tree.

        """
        path = self.get_characteristic_map(address).get(uuid)
        if not path:
            return None
        return dbus.Interface(
            self._system_bus.get_object(self.BLUEZ_SERVICE_NAME, path),
            self.BLUEZ_GATT_IFACE)


    @xmlrpc_server.dbus_safe(None)
    def read_characteristic(self, uuid, address):
        """Reads the value of a gatt characteristic.

        Reads the current value of a gatt characteristic. Base64 endcoding is
        used for compatibility with the XML RPC interface.

        @param uuid: The uuid of the characteristic to read, as a string.
        @param address: The MAC address of the remote device.

        @returns: A b64 encoded version of a byte array containing the value
                      if the uuid/address is in the object tree.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the read operation.

        """
        char_obj = self._get_char_object(uuid, address)
        if char_obj is None:
            return None
        value = char_obj.ReadValue(dbus.Dictionary())
        return _dbus_byte_array_to_b64_string(value)


    @xmlrpc_server.dbus_safe(None)
    def write_characteristic(self, uuid, address, value):
        """Performs a write operation on a gatt characteristic.

        Writes to a GATT characteristic on a remote device. Base64 endcoding is
        used for compatibility with the XML RPC interface.

        @param uuid: The uuid of the characteristic to write to, as a string.
        @param address: The MAC address of the remote device, as a string.
        @param value: A byte array containing the data to write.

        @returns: True if the write operation does not raise an exception.
                  None if the uuid/address was not found in the object tree, or
                      if a DBus exception was raised by the write operation.

        """
        char_obj = self._get_char_object(uuid, address)
        if char_obj is None:
            return None
        dbus_value = _b64_string_to_dbus_byte_array(value)
        char_obj.WriteValue(dbus_value, dbus.Dictionary())
        return True


    @xmlrpc_server.dbus_safe(False)
    def is_characteristic_path_resolved(self, uuid, address):
        """Checks whether a characteristic is in the object tree.

        Checks whether a characteristic is curently found in the object tree.

        @param uuid: The uuid of the characteristic to search for.
        @param address: The MAC address of the device on which to search for
            the characteristic.

        @returns: True if the characteristic is found.
                  False if the characteristic path is not found.

        """
        return bool(self.get_characteristic_map(address).get(uuid))


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    handler = logging.handlers.SysLogHandler(address='/dev/log')
    formatter = logging.Formatter(
            'bluetooth_device_xmlrpc_server: [%(levelname)s] %(message)s')
    handler.setFormatter(formatter)
    logging.getLogger().addHandler(handler)
    logging.debug('bluetooth_device_xmlrpc_server main...')
    server = xmlrpc_server.XmlRpcServer(
            'localhost',
            constants.BLUETOOTH_DEVICE_XMLRPC_SERVER_PORT)
    server.register_delegate(BluetoothDeviceXmlRpcDelegate())
    server.run()
