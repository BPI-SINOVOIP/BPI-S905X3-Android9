#!/usr/bin/python
# -*- coding: utf-8 -*-
# Copyright 2017 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""AT-Factory-Tool manager module.

This module provides the logical implementation of the graphical tool for
managing the ATFA and AT communication.
"""
import base64
from datetime import datetime
import json
import os
import re
import tempfile
import threading
import uuid

from fastboot_exceptions import DeviceNotFoundException
from fastboot_exceptions import FastbootFailure
from fastboot_exceptions import NoAlgorithmAvailableException
from fastboot_exceptions import ProductAttributesFileFormatError
from fastboot_exceptions import ProductNotSpecifiedException

BOOTLOADER_STRING = '(bootloader) '


class EncryptionAlgorithm(object):
  """The support encryption algorithm constant."""
  ALGORITHM_P256 = 1
  ALGORITHM_CURVE25519 = 2


class ProvisionStatus(object):
  """The provision status constant."""
  _PROCESSING        = 0
  _SUCCESS           = 1
  _FAILED            = 2

  IDLE              = 0
  WAITING           = 1
  FUSEVBOOT_ING     = (10 + _PROCESSING)
  FUSEVBOOT_SUCCESS = (10 + _SUCCESS)
  FUSEVBOOT_FAILED  = (10 + _FAILED)
  REBOOT_ING        = (20 + _PROCESSING)
  REBOOT_SUCCESS    = (20 + _SUCCESS)
  REBOOT_FAILED     = (20 + _FAILED)
  FUSEATTR_ING      = (30 + _PROCESSING)
  FUSEATTR_SUCCESS  = (30 + _SUCCESS)
  FUSEATTR_FAILED   = (30 + _FAILED)
  LOCKAVB_ING       = (40 + _PROCESSING)
  LOCKAVB_SUCCESS   = (40 + _SUCCESS)
  LOCKAVB_FAILED    = (40 + _FAILED)
  PROVISION_ING     = (50 + _PROCESSING)
  PROVISION_SUCCESS = (50 + _SUCCESS)
  PROVISION_FAILED  = ( + _FAILED)

  STRING_MAP = {
    IDLE              : ['Idle', '初始'],
    WAITING           : ['Waiting', '等待'],
    FUSEVBOOT_ING     : ['Fusing Bootloader Vboot Key...', '烧录引导密钥中...'],
    FUSEVBOOT_SUCCESS : ['Bootloader Locked', '烧录引导密钥成功'],
    FUSEVBOOT_FAILED  : ['Fuse Bootloader Vboot Key Failed', '烧录引导密钥失败'],
    REBOOT_ING        : ['Rebooting Device To Check Vboot Key...',
                         '重启设备中...'],
    REBOOT_SUCCESS    : ['Bootloader Verified Boot Checked', '重启设备成功'],
    REBOOT_FAILED     : ['Reboot Device Failed', '重启设备失败'],
    FUSEATTR_ING      : ['Fusing Permanent Attributes', '烧录产品信息中...'],
    FUSEATTR_SUCCESS  : ['Permanent Attributes Fused', '烧录产品信息成功'],
    FUSEATTR_FAILED   : ['Fuse Permanent Attributes Failed', '烧录产品信息失败'],
    LOCKAVB_ING       : ['Locking Android Verified Boot', '锁定AVB中...'],
    LOCKAVB_SUCCESS   : ['Android Verified Boot Locked', '锁定AVB成功'],
    LOCKAVB_FAILED    : ['Lock Android Verified Boot Failed', '锁定AVB失败'],
    PROVISION_ING     : ['Provisioning Attestation Key', '传输密钥中...'],
    PROVISION_SUCCESS : ['Attestation Key Provisioned', '传输密钥成功'],
    PROVISION_FAILED  : ['Provision Attestation Key Failed', '传输密钥失败']

  }

  @staticmethod
  def ToString(provision_status, language_index):
    return ProvisionStatus.STRING_MAP[provision_status][language_index]

  @staticmethod
  def isSuccess(provision_status):
    return provision_status % 10 == ProvisionStatus._SUCCESS

  @staticmethod
  def isProcessing(provision_status):
    return provision_status % 10 == ProvisionStatus._PROCESSING

  @staticmethod
  def isFailed(provision_status):
    return provision_status % 10 == ProvisionStatus._FAILED


class ProvisionState(object):
  """The provision state of the target device."""
  bootloader_locked = False
  avb_perm_attr_set = False
  avb_locked = False
  provisioned = False


class ProductInfo(object):
  """The information about a product.

  Attributes:
    product_id: The id for the product.
    product_name: The name for the product.
    product_attributes: The byte array of the product permanent attributes.
  """

  def __init__(self, product_id, product_name, product_attributes, vboot_key):
    self.product_id = product_id
    self.product_name = product_name
    self.product_attributes = product_attributes
    self.vboot_key = vboot_key


class DeviceInfo(object):
  """The class to wrap the information about a fastboot device.

  Attributes:
    serial_number: The serial number for the device.
    location: The physical USB location for the device.
  """

  def __init__(self, _fastboot_device_controller, serial_number,
               location=None, provision_status=ProvisionStatus.IDLE,
               provision_state=ProvisionState()):
    self._fastboot_device_controller = _fastboot_device_controller
    self.serial_number = serial_number
    self.location = location
    # The provision status and provision state is only meaningful for target
    # device.
    self.provision_status = provision_status
    self.provision_state = provision_state
    # The number of attestation keys left for the selected product. This
    # attribute is only meaning for ATFA device.
    self.keys_left = None

  def Copy(self):
    return DeviceInfo(None, self.serial_number, self.location,
                      self.provision_status)

  def Reboot(self):
    return self._fastboot_device_controller.Reboot()

  def Oem(self, oem_command, err_to_out=False):
    return self._fastboot_device_controller.Oem(oem_command, err_to_out)

  def Flash(self, partition, file_path):
    return self._fastboot_device_controller.Flash(partition, file_path)

  def Upload(self, file_path):
    return self._fastboot_device_controller.Upload(file_path)

  def Download(self, file_path):
    return self._fastboot_device_controller.Download(file_path)

  def GetVar(self, var):
    return self._fastboot_device_controller.GetVar(var)

  def __eq__(self, other):
    return (self.serial_number == other.serial_number and
            self.location == other.location and
            self.provision_status == other.provision_status)

  def __ne__(self, other):
    return not self.__eq__(other)

  def __str__(self):
    if self.location:
      return self.serial_number + ' at location: ' + self.location
    else:
      return self.serial_number


class RebootCallback(object):
  """The class to handle reboot success and timeout callbacks."""

  def __init__(
      self, timeout, success_callback, timeout_callback):
    """Initiate a reboot callback handler class.

    Args:
      timeout: How much time to wait for the device to reappear.
      success_callback: The callback to be called if the device reappear
        before timeout.
      timeout_callback: The callback to be called if the device doesn't reappear
        before timeout.
    """
    self.success = success_callback
    self.fail = timeout_callback
    # Lock to make sure only one callback is called. (either success or timeout)
    # This lock can only be obtained once.
    self.lock = threading.Lock()
    self.timer = threading.Timer(timeout, self._TimeoutCallback)
    self.timer.start()

  def _TimeoutCallback(self):
    """The function to handle timeout callback.

    Call the timeout_callback that is registered.
    """
    if self.lock and self.lock.acquire(False):
      self.fail()

  def Release(self):
    lock = self.lock
    timer = self.timer
    self.lock = None
    self.timer = None
    lock.release()
    timer.cancel()


class AtftManager(object):
  """The manager to implement ATFA tasks.

  Attributes:
    atfa_dev: A FastbootDevice object identifying the detected ATFA device.
    target_dev: A FastbootDevice object identifying the AT device
      to be provisioned.
  """
  SORT_BY_SERIAL = 0
  SORT_BY_LOCATION = 1
  # The length of the permanent attribute should be 1052.
  EXPECTED_ATTRIBUTE_LENGTH = 1052

  # The Permanent Attribute File JSON Key Names:
  JSON_PRODUCT_NAME = 'productName'
  JSON_PRODUCT_ATTRIBUTE = 'productPermanentAttribute'
  JSON_PRODUCT_ATTRIBUTE = 'productPermanentAttribute'
  JSON_VBOOT_KEY = 'bootloaderPublicKey'

  def __init__(self, fastboot_device_controller, serial_mapper, configs):
    """Initialize attributes and store the supplied fastboot_device_controller.

    Args:
      fastboot_device_controller:
        The interface to interact with a fastboot device.
      serial_mapper:
        The interface to get the USB physical location to serial number map.
      configs:
        The additional configurations. Need to contain 'ATFA_REBOOT_TIMEOUT'.
    """
    # The timeout period for ATFA device reboot.
    self.ATFA_REBOOT_TIMEOUT = 30
    if configs and 'ATFA_REBOOT_TIMEOUT' in configs:
      try:
        self.ATFA_REBOOT_TIMEOUT = float(configs['ATFA_REBOOT_TIMEOUT'])
      except ValueError:
        pass

    # The serial numbers for the devices that are at least seen twice.
    self.stable_serials = []
    # The serail numbers for the devices that are only seen once.
    self.pending_serials = []
    # The atfa device DeviceInfo object.
    self.atfa_dev = None
    # The atfa device currently rebooting to set the os.
    self._atfa_dev_setting = None
    # The list of target devices DeviceInfo objects.
    self.target_devs = []
    # The product information for the selected product.
    self.product_info = None
     # The atfa device manager.
    self._atfa_dev_manager = AtfaDeviceManager(self)
    # The fastboot controller.
    self._fastboot_device_controller = fastboot_device_controller
    # The map mapping serial number to USB location.
    self._serial_mapper = serial_mapper()
    # The map mapping rebooting device serial number to their reboot callback
    # objects.
    self._reboot_callbacks = {}

    self._atfa_reboot_lock = threading.Lock()

  def GetATFAKeysLeft(self):
    if not self.atfa_dev:
      return None
    return self.atfa_dev.keys_left

  def CheckATFAStatus(self):
    return self._atfa_dev_manager.CheckStatus()

  def SwitchATFAStorage(self):
    if self._fastboot_device_controller.GetHostOs() == 'Windows':
      # Only windows need to switch. For Linux the partition should already
      # mounted.
      self._atfa_dev_manager.SwitchStorage()
    else:
      self.CheckDevice(self.atfa_dev)

  def RebootATFA(self):
    return self._atfa_dev_manager.Reboot()

  def ShutdownATFA(self):
    return self._atfa_dev_manager.Shutdown()

  def ProcessATFAKey(self):
    return self._atfa_dev_manager.ProcessKey()

  def ListDevices(self, sort_by=SORT_BY_LOCATION):
    """Get device list.

    Get the serial number of the ATFA device and the target device. If the
    device does not exist, the returned serial number would be None.

    Args:
      sort_by: The field to sort by.
    """
    # ListDevices returns a list of USBHandles
    device_serials = self._fastboot_device_controller.ListDevices()
    self.UpdateDevices(device_serials)
    self._HandleRebootCallbacks()
    self._SortTargetDevices(sort_by)

  def UpdateDevices(self, device_serials):
    """Update device list.

    Args:
      device_serials: The device serial numbers.
    """
    self._UpdateSerials(device_serials)
    if not self.stable_serials:
      self.target_devs = []
      self.atfa_dev = None
      return
    self._HandleSerials()

  @staticmethod
  def _SerialAsKey(device):
    return device.serial_number

  @staticmethod
  def _LocationAsKey(device):
    if device.location is None:
      return ''
    return device.location

  def _SortTargetDevices(self, sort_by):
    """Sort the target device list according to sort_by field.

    Args:
      sort_by: The field to sort by, possible values are:
        self.SORT_BY_LOCATION and self.SORT_BY_SERIAL.
    """
    if sort_by == self.SORT_BY_LOCATION:
      self.target_devs.sort(key=AtftManager._LocationAsKey)
    elif sort_by == self.SORT_BY_SERIAL:
      self.target_devs.sort(key=AtftManager._SerialAsKey)

  def _UpdateSerials(self, device_serials):
    """Update the stored pending_serials and stable_serials.

    Note that we cannot check status once the fastboot device is found since the
    device may not be ready yet. So we put the new devices into the pending
    state. Once we see the device again in the next refresh, we add that device.
    If that device is not seen in the next refresh, we remove it from pending.
    This makes sure that the device would have a refresh interval time after
    it's recognized as a fastboot device until it's issued command.

    Args:
      device_serials: The list of serial numbers of the fastboot devices.
    """
    stable_serials_copy = self.stable_serials[:]
    pending_serials_copy = self.pending_serials[:]
    self.stable_serials = []
    self.pending_serials = []
    for serial in device_serials:
      if serial in stable_serials_copy or serial in pending_serials_copy:
        # Was in stable or pending state, seen twice, add to stable state.
        self.stable_serials.append(serial)
      else:
        # First seen, add to pending state.
        self.pending_serials.append(serial)

  def _CheckAtfaSetOs(self):
    """Check whether the ATFA device reappear after a 'set-os' command.

    If it reappears, we create a new ATFA device object.
    If not, something wrong happens, we need to clean the rebooting state.
    """
    atfa_serial = self._atfa_dev_setting.serial_number
    if atfa_serial in self.stable_serials:
      # We found the ATFA device again.
      self._serial_mapper.refresh_serial_map()
      controller = self._fastboot_device_controller(atfa_serial)
      location = self._serial_mapper.get_location(atfa_serial)
      self.atfa_dev = DeviceInfo(controller, atfa_serial, location)

    # Clean the state
    self._atfa_dev_setting = None
    self._atfa_reboot_lock.release()

  def _HandleSerials(self):
    """Create new devices and remove old devices.

    Add device location information and target device provision status.
    """
    device_serials = self.stable_serials
    new_targets = []
    atfa_serial = None
    for serial in device_serials:
      if not serial:
        continue

      if serial.startswith('ATFA'):
        atfa_serial = serial
      else:
        new_targets.append(serial)

    if atfa_serial is None:
      # No ATFA device found.
      self.atfa_dev = None
    elif self.atfa_dev is None or self.atfa_dev.serial_number != atfa_serial:
      self._AddNewAtfa(atfa_serial)

    # Remove those devices that are not in new targets and not rebooting.
    self.target_devs = [
        device for device in self.target_devs
        if (device.serial_number in new_targets or
            device.provision_status == ProvisionStatus.REBOOT_ING)
    ]

    common_serials = [device.serial_number for device in self.target_devs]

    # Create new device object for newly added devices.
    self._serial_mapper.refresh_serial_map()
    for serial in new_targets:
      if serial not in common_serials:
        self._CreateNewTargetDevice(serial)

  def _CreateNewTargetDevice(self, serial, check_status=True):
    """Create a new target device object.

    Args:
      serial: The serial number for the new target device.
      check_status: Whether to check provision status for the target device.
    """
    try:
      controller = self._fastboot_device_controller(serial)
      location = self._serial_mapper.get_location(serial)

      new_target_dev = DeviceInfo(controller, serial, location)
      if check_status:
        self.CheckProvisionStatus(new_target_dev)
      self.target_devs.append(new_target_dev)
    except FastbootFailure as e:
      e.msg = ('Error while creating new device: ' + str(new_target_dev) +
               '\n'+ e.msg)
      self.stable_serials.remove(serial)
      raise e

  def _AddNewAtfa(self, atfa_serial):
    """Create a new ATFA device object.

    If the OS variable on the ATFA device is not the same as the host OS
    version, we would use set the correct OS version.

    Args:
      atfa_serial: The serial number of the ATFA device to be added.
    """
    self._serial_mapper.refresh_serial_map()
    controller = self._fastboot_device_controller(atfa_serial)
    location = self._serial_mapper.get_location(atfa_serial)
    if self._atfa_reboot_lock.acquire(False):
      # If there's not an atfa setting os already happening
      self._atfa_dev_setting = DeviceInfo(controller, atfa_serial, location)
      try:
        atfa_os = self._GetOs(self._atfa_dev_setting)
      except FastbootFailure:
        # The device is not ready for get OS command, we just ignore the device.
        self._atfa_reboot_lock.release()
        return
      host_os = controller.GetHostOs()
      if atfa_os == host_os:
        # The OS set for the ATFA is correct, we just create the new device.
        self.atfa_dev = self._atfa_dev_setting
        self._atfa_dev_setting = None
        self._atfa_reboot_lock.release()
      else:
        # The OS set for the ATFA is not correct, need to set it.
        try:
          self._SetOs(self._atfa_dev_setting, host_os)
          # SetOs include a rebooting process, but the device would not
          # disappear from the device list immediately after the command.
          # We would check if the ATFA reappear after ATFA_REBOOT_TIMEOUT.
          timer = threading.Timer(
              self.ATFA_REBOOT_TIMEOUT, self._CheckAtfaSetOs)
          timer.start()
        except FastbootFailure:
          self._atfa_dev_setting = None
          self._atfa_reboot_lock.release()

  def _SetOs(self, target_dev, os_version):
    """Change the os version on the target device.

    Args:
      target_dev: The target device.
      os_version: The os version to set, options are 'Windows' or 'Linux'.
    Raises:
      FastbootFailure: when fastboot command fails.
    """
    target_dev.Oem('set-os ' + os_version)

  def _GetOs(self, target_dev):
    """Get the os version of the target device.

    Args:
      target_dev: The target deivce.
    Returns:
      The os version.
    Raises:
      FastbootFailure: when fastboot command fails.
    """
    output = target_dev.Oem('get-os', True)
    if output and 'Linux' in output:
      return 'Linux'
    else:
      return 'Windows'

  def _HandleRebootCallbacks(self):
    """Handle the callback functions after the reboot."""
    success_serials = []
    for serial in self._reboot_callbacks:
      if serial in self.stable_serials:
        callback_lock = self._reboot_callbacks[serial].lock
        # Make sure the timeout callback would not be called at the same time.
        if callback_lock and callback_lock.acquire(False):
          success_serials.append(serial)

    for serial in success_serials:
      self._reboot_callbacks[serial].success()

  def _ParseStateString(self, state_string):
    """Parse the string returned by 'at-vboot-state' to a key-value map.

    Args:
      state_string: The string returned by oem at-vboot-state command.

    Returns:
      A key-value map.
    """
    state_map = {}
    lines = state_string.splitlines()
    for line in lines:
      if line.startswith(BOOTLOADER_STRING):
        key_value = re.split(r':[\s]*|=', line.replace(BOOTLOADER_STRING, ''))
        if len(key_value) == 2:
          state_map[key_value[0]] = key_value[1]
    return state_map

  def CheckProvisionStatus(self, target_dev):
    """Check whether the target device has been provisioned.

    Args:
      target_dev: The target device (DeviceInfo).
    """
    at_attest_uuid = target_dev.GetVar('at-attest-uuid')
    state_string = target_dev.GetVar('at-vboot-state')

    target_dev.provision_status = ProvisionStatus.IDLE
    target_dev.provision_state = ProvisionState()

    status_set = False

    # TODO(shanyu): We only need empty string here
    # NOT_PROVISIONED is for test purpose.
    if at_attest_uuid and at_attest_uuid != 'NOT_PROVISIONED':
      target_dev.provision_status = ProvisionStatus.PROVISION_SUCCESS
      status_set = True
      target_dev.provision_state.provisioned = True

    # state_string should be in format:
    # (bootloader) bootloader-locked: 1
    # (bootloader) bootloader-min-versions: -1,0,3
    # (bootloader) avb-perm-attr-set: 1
    # (bootloader) avb-locked: 0
    # (bootloader) avb-unlock-disabled: 0
    # (bootloader) avb-min-versions: 0:1,1:1,2:1,4097 :2,4098:2
    if not state_string:
      return
    state_map = self._ParseStateString(state_string)
    if state_map.get('avb-locked') and state_map['avb-locked'] == '1':
      if not status_set:
        target_dev.provision_status = ProvisionStatus.LOCKAVB_SUCCESS
        status_set = True
      target_dev.provision_state.avb_locked = True

    if (state_map.get('avb-perm-attr-set') and
        state_map['avb-perm-attr-set'] == '1'):
      if not status_set:
        target_dev.provision_status = ProvisionStatus.FUSEATTR_SUCCESS
        status_set = True
      target_dev.provision_state.avb_perm_attr_set = True

    if (state_map.get('bootloader-locked') and
        state_map['bootloader-locked'] == '1'):
      if not status_set:
        target_dev.provision_status = ProvisionStatus.FUSEVBOOT_SUCCESS
      target_dev.provision_state.bootloader_locked = True


  def TransferContent(self, src, dst):
    """Transfer content from a device to another device.

    Download file from one device and store it into a tmp file. Upload file from
    the tmp file onto another device.

    Args:
      src: The source device to be copied from.
      dst: The destination device to be copied to.
    """
    # create a tmp folder
    tmp_folder = tempfile.mkdtemp()
    # temperate file name is a UUID based on host ID and current time.
    tmp_file_name = str(uuid.uuid1())
    file_path = os.path.join(tmp_folder, tmp_file_name)
    # pull file to local fs
    src.Upload(file_path)
    # push file to fastboot device
    dst.Download(file_path)
    # delete the temperate file afterwards
    if os.path.exists(file_path):
      os.remove(file_path)
    # delete the temperate folder afterwards
    if os.path.exists(tmp_folder):
      os.rmdir(tmp_folder)

  def GetTargetDevice(self, serial):
    """Get the target DeviceInfo object according to the serial number.

    Args:
      serial: The serial number for the device object.
    Returns:
      The DeviceInfo object for the device. None if not exists.
    """
    for device in self.target_devs:
      if device.serial_number == serial:
        return device

    return None

  def Provision(self, target):
    """Provision the key to the target device.

    1. Get supported encryption algorithm
    2. Send atfa-start-provisioning message to ATFA
    3. Transfer content from ATFA to target
    4. Send at-get-ca-request to target
    5. Transfer content from target to ATFA
    6. Send atfa-finish-provisioning message to ATFA
    7. Transfer content from ATFA to target
    8. Send at-set-ca-response message to target

    Args:
      target: The target device to be provisioned to.
    """
    try:
      target.provision_status = ProvisionStatus.PROVISION_ING
      atfa = self.atfa_dev
      AtftManager.CheckDevice(atfa)
      # Set the ATFA's time first.
      self._atfa_dev_manager.SetTime()
      algorithm_list = self._GetAlgorithmList(target)
      algorithm = self._ChooseAlgorithm(algorithm_list)
      # First half of the DH key exchange
      atfa.Oem('atfa-start-provisioning ' + str(algorithm))
      self.TransferContent(atfa, target)
      # Second half of the DH key exchange
      target.Oem('at-get-ca-request')
      self.TransferContent(target, atfa)
      # Encrypt and transfer key bundle
      atfa.Oem('atfa-finish-provisioning')
      self.TransferContent(atfa, target)
      # Provision the key on device
      target.Oem('at-set-ca-response')

      # After a success provision, the status should be updated.
      self.CheckProvisionStatus(target)
      if not target.provision_state.provisioned:
        raise FastbootFailure('Status not updated.')
    except (FastbootFailure, DeviceNotFoundException) as e:
      target.provision_status = ProvisionStatus.PROVISION_FAILED
      raise e

  def FuseVbootKey(self, target):
    """Fuse the verified boot key to the target device.

    Args:
      target: The target device.
    """
    if not self.product_info:
      target.provision_status = ProvisionStatus.FUSEVBOOT_FAILED
      raise ProductNotSpecifiedException

    # Create a temporary file to store the vboot key.
    target.provision_status = ProvisionStatus.FUSEVBOOT_ING
    try:
      temp_file = tempfile.NamedTemporaryFile(delete=False)
      temp_file.write(self.product_info.vboot_key)
      temp_file.close()
      temp_file_name = temp_file.name
      target.Download(temp_file_name)
      # Delete the temporary file.
      os.remove(temp_file_name)
      target.Oem('fuse at-bootloader-vboot-key')

      # After a success fuse, the status should be updated.
      self.CheckProvisionStatus(target)
      if not target.provision_state.bootloader_locked:
        raise FastbootFailure('Status not updated.')

      # # Another possible flow:
      # target.Flash('sec', temp_file_name)
      # os.remove(temp_file_name)
    except FastbootFailure as e:
      target.provision_status = ProvisionStatus.FUSEVBOOT_FAILED
      raise e

  def FusePermAttr(self, target):
    """Fuse the permanent attributes to the target device.

    Args:
      target: The target device.
    """
    if not self.product_info:
      target.provision_status = ProvisionStatus.FUSEATTR_FAILED
      raise ProductNotSpecifiedException
    try:
      target.provision_status = ProvisionStatus.FUSEATTR_ING
      temp_file = tempfile.NamedTemporaryFile(delete=False)
      temp_file.write(self.product_info.product_attributes)
      temp_file.close()
      temp_file_name = temp_file.name
      target.Download(temp_file_name)
      os.remove(temp_file_name)
      target.Oem('fuse at-perm-attr')

      self.CheckProvisionStatus(target)
      if not target.provision_state.avb_perm_attr_set:
        raise FastbootFailure('Status not updated')

    except FastbootFailure as e:
      target.provision_status = ProvisionStatus.FUSEATTR_FAILED
      raise e

  def LockAvb(self, target):
    """Lock the android verified boot for the target.

    Args:
      target: The target device.
    """
    try:
      target.provision_status = ProvisionStatus.LOCKAVB_ING
      target.Oem('at-lock-vboot')
      self.CheckProvisionStatus(target)
      if not target.provision_state.avb_locked:
        raise FastbootFailure('Status not updated')
    except FastbootFailure as e:
      target.provision_status = ProvisionStatus.LOCKAVB_FAILED
      raise e

  def Reboot(self, target, timeout, success_callback, timeout_callback):
    """Reboot the target device.

    Args:
      target: The target device.
      timeout: The time out value.
      success_callback: The callback function called when the device reboots
        successfully.
      timeout_callback: The callback function called when the device reboots
        timeout.

    The device would disappear from the list after reboot.
    If we see the device again within timeout, call the success_callback,
    otherwise call the timeout_callback.
    """
    try:
      target.Reboot()
      serial = target.serial_number
      location = target.location
      # We assume after the reboot the device would disappear
      self.target_devs.remove(target)
      del target
      self.stable_serials.remove(serial)
      # Create a rebooting target device that only contains serial and location.
      rebooting_target = DeviceInfo(None, serial, location)
      rebooting_target.provision_status = ProvisionStatus.REBOOT_ING
      self.target_devs.append(rebooting_target)

      reboot_callback = RebootCallback(
          timeout,
          self.RebootCallbackWrapper(success_callback, serial, True),
          self.RebootCallbackWrapper(timeout_callback, serial, False))
      self._reboot_callbacks[serial] = reboot_callback

    except FastbootFailure as e:
      target.provision_status = ProvisionStatus.REBOOT_FAILED
      raise e

  def RebootCallbackWrapper(self, callback, serial, success):
    """This wrapper function wraps the original callback function.

    Some clean up operations are added. We need to remove the handler if
    callback is called. We need to release the resource the handler requires.
    We also needs to remove the rebooting device from the target list since a
    new device would be created if the device reboot successfully.

    Args:
      callback: The original callback function.
      serial: The serial number for the device.
      success: Whether this is the success callback.
    Returns:
      An extended callback function.
    """
    def RebootCallbackFunc(callback=callback, serial=serial, success=success):
      try:
        rebooting_dev = self.GetTargetDevice(serial)
        if rebooting_dev:
          self.target_devs.remove(rebooting_dev)
          del rebooting_dev
        if success:
          self._serial_mapper.refresh_serial_map()
          self._CreateNewTargetDevice(serial, True)
          self.GetTargetDevice(serial).provision_status = (
              ProvisionStatus.REBOOT_SUCCESS)
        callback()
        self._reboot_callbacks[serial].Release()
        del self._reboot_callbacks[serial]
      except FastbootFailure as e:
        # Release the lock so that it can be obtained again.
        self._reboot_callbacks[serial].lock.release()
        raise e



    return RebootCallbackFunc

  def _GetAlgorithmList(self, target):
    """Get the supported algorithm list.

    Get the available algorithm list using getvar at-attest-dh
    at_attest_dh should be in format 1:p256,2:curve25519
    or 1:p256
    or 2:curve25519.

    Args:
      target: The target device to check for supported algorithm.
    Returns:
      A list of available algorithms.
      Options are ALGORITHM_P256 or ALGORITHM_CURVE25519
    """
    at_attest_dh = target.GetVar('at-attest-dh')
    algorithm_strings = at_attest_dh.split(',')
    algorithm_list = []
    for algorithm_string in algorithm_strings:
      algorithm_list.append(int(algorithm_string.split(':')[0]))
    return algorithm_list

  def _ChooseAlgorithm(self, algorithm_list):
    """Choose the encryption algorithm to use for key provisioning.

    We favor ALGORITHM_CURVE25519 over ALGORITHM_P256

    Args:
      algorithm_list: The list containing all available algorithms.
    Returns:
      The selected available algorithm
    Raises:
      NoAlgorithmAvailableException:
        When there's no available valid algorithm to use.
    """
    if not algorithm_list:
      raise NoAlgorithmAvailableException()
    if EncryptionAlgorithm.ALGORITHM_CURVE25519 in algorithm_list:
      return EncryptionAlgorithm.ALGORITHM_CURVE25519
    elif EncryptionAlgorithm.ALGORITHM_P256 in algorithm_list:
      return EncryptionAlgorithm.ALGORITHM_P256

    raise NoAlgorithmAvailableException()

  def ProcessProductAttributesFile(self, content):
    """Process the product attributes file.

    The file should follow the following JSON format:
      {
        "productName": "",
        "productDescription": "",
        "productConsoleId": "",
        "productPermanentAttribute": "",
        "bootloaderPublicKey": "",
        "creationTime": ""
      }

    Args:
      content: The content of the product attributes file.
    Raises:
      ProductAttributesFileFormatError: When the file format is wrong.
    """
    try:
      file_object = json.loads(content)
    except ValueError:
      raise ProductAttributesFileFormatError(
          'Wrong JSON format!')
    product_name = file_object.get(self.JSON_PRODUCT_NAME)
    attribute_string = file_object.get(self.JSON_PRODUCT_ATTRIBUTE)
    vboot_key_string = file_object.get(self.JSON_VBOOT_KEY)
    if not product_name or not attribute_string or not vboot_key_string:
      raise ProductAttributesFileFormatError(
          'Essential field missing!')
    try:
      attribute = base64.standard_b64decode(attribute_string)
      attribute_array = bytearray(attribute)
      if self.EXPECTED_ATTRIBUTE_LENGTH != len(attribute_array):
        raise ProductAttributesFileFormatError(
            'Incorrect permanent product attributes length')

      # We only need the last 16 byte for product ID
      # We store the hex representation of the product ID
      product_id = self._ByteToHex(attribute_array[-16:])

      vboot_key_array = bytearray(base64.standard_b64decode(vboot_key_string))

    except TypeError:
      raise ProductAttributesFileFormatError(
          'Incorrect Base64 encoding for permanent product attributes')

    self.product_info = ProductInfo(product_id, product_name, attribute_array,
                                    vboot_key_array)

  def _ByteToHex(self, byte_array):
    """Transform a byte array into a hex string."""
    return ''.join('{:02x}'.format(x) for x in byte_array)

  @staticmethod
  def CheckDevice(device):
    """Check if the device is a connected fastboot device.

    Args:
      device: The device to be checked.
    Raises:
      DeviceNotFoundException: When the device is not found
    """
    if device is None:
      raise DeviceNotFoundException()


class AtfaDeviceManager(object):
  """The class to manager ATFA device related operations."""

  def __init__(self, atft_manager):
    """Initiate the atfa device manager using the at-factory-tool manager.

    Args:
      atft_manager: The at-factory-tool manager that
        includes this atfa device manager.
    """
    self.atft_manager = atft_manager

  def GetSerial(self):
    """Issue fastboot command to get serial number for the ATFA device.

    Raises:
      DeviceNotFoundException: When the device is not found.
    """
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    self.atft_manager.atfa_dev.Oem('serial')

  def SwitchStorage(self):
    """Switch the ATFA device to storage mode.

    Raises:
      DeviceNotFoundException: When the device is not found
    """
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    self.atft_manager.atfa_dev.Oem('storage')

  def ProcessKey(self):
    """Ask the ATFA device to process the stored key bundle.

    Raises:
      DeviceNotFoundException: When the device is not found
    """
    # Need to set time first so that certificates would validate.
    self.SetTime()
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    self.atft_manager.atfa_dev.Oem('process-keybundle')

  def Reboot(self):
    """Reboot the ATFA device.

    Raises:
      DeviceNotFoundException: When the device is not found
    """
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    self.atft_manager.atfa_dev.Oem('reboot')

  def Shutdown(self):
    """Shutdown the ATFA device.

    Raises:
      DeviceNotFoundException: When the device is not found
    """
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    self.atft_manager.atfa_dev.Oem('shutdown')

  def CheckStatus(self):
    """Update the number of available AT keys for the current product.

    Need to use GetKeysLeft() function to get the number of keys left. If some
    error happens, keys_left would be set to -1 to prevent checking again.

    Raises:
      FastbootFailure: If error happens with the fastboot oem command.
    """

    if not self.atft_manager.product_info:
      raise ProductNotSpecifiedException()

    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    # -1 means some error happens.
    self.atft_manager.atfa_dev.keys_left = -1
    out = self.atft_manager.atfa_dev.Oem(
        'num-keys ' + self.atft_manager.product_info.product_id, True)
    # Note: use splitlines instead of split('\n') to prevent '\r\n' problem on
    # windows.
    for line in out.splitlines():
      if line.startswith('(bootloader) '):
        try:
          self.atft_manager.atfa_dev.keys_left = int(
              line.replace('(bootloader) ', ''))
          return
        except ValueError:
          raise FastbootFailure(
              'ATFA device response has invalid format')

    raise FastbootFailure('ATFA device response has invalid format')

  def SetTime(self):
    """Inject the host time into the ATFA device.

    Raises:
      DeviceNotFoundException: When the device is not found
    """
    AtftManager.CheckDevice(self.atft_manager.atfa_dev)
    time = datetime.utcnow().strftime('%Y-%m-%d %H:%M:%S')
    self.atft_manager.atfa_dev.Oem('set-date ' + time)
