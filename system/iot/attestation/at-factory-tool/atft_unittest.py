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

"""Unit test for atft."""
import types
import unittest

import atft
from atftman import ProvisionStatus
from atftman import ProvisionState
import fastboot_exceptions
from mock import call
from mock import MagicMock
from mock import patch
import wx


class MockAtft(atft.Atft):

  def __init__(self):
    self.InitializeUI = MagicMock()
    self.StartRefreshingDevices = MagicMock()
    self.ChooseProduct = MagicMock()
    self.CreateAtftManager = MagicMock()
    self.CreateAtftLog = MagicMock()
    self.ParseConfigFile = self._MockParseConfig
    self._SendPrintEvent = MagicMock()
    atft.Atft.__init__(self)

  def _MockParseConfig(self):
    self.ATFT_VERSION = 'vTest'
    self.COMPATIBLE_ATFA_VERSION = 'v1'
    self.DEVICE_REFRESH_INTERVAL = 1.0
    self.DEFAULT_KEY_THRESHOLD = 0
    self.LOG_DIR = 'test_log_dir'
    self.LOG_SIZE = 1000
    self.LOG_FILE_NUMBER = 2
    self.LANGUAGE = 'ENG'
    self.REBOOT_TIMEOUT = 1.0
    self.PRODUCT_ATTRIBUTE_FILE_EXTENSION = '*.atpa'

    return {}


class TestDeviceInfo(object):

  def __init__(self, serial_number, location=None, provision_status=None):
    self.serial_number = serial_number
    self.location = location
    self.provision_status = provision_status
    self.provision_state = ProvisionState()
    self.time_set = False

  def __eq__(self, other):
    return (self.serial_number == other.serial_number and
            self.location == other.location)

  def __ne__(self, other):
    return not self.__eq__(other)

  def Copy(self):
    return TestDeviceInfo(self.serial_number, self.location,
                          self.provision_status)


class AtftTest(unittest.TestCase):
  TEST_SERIAL1 = 'test-serial1'
  TEST_LOCATION1 = 'test-location1'
  TEST_SERIAL2 = 'test-serial2'
  TEST_LOCATION2 = 'test-location2'
  TEST_SERIAL3 = 'test-serial3'
  TEST_SERIAL4 = 'test-serial4'
  TEST_TEXT = 'test-text'
  TEST_TEXT2 = 'test-text2'

  def setUp(self):
    self.test_target_devs = []
    self.test_dev1 = TestDeviceInfo(
        self.TEST_SERIAL1, self.TEST_LOCATION1, ProvisionStatus.IDLE)
    self.test_dev2 = TestDeviceInfo(
        self.TEST_SERIAL2, self.TEST_LOCATION2, ProvisionStatus.IDLE)
    self.test_text_window = ''
    self.atfa_keys = None
    self.device_map = {}
    # Disable the test mode. (This mode is just for usage test, not unit test)
    atft.TEST_MODE = False

  def AppendTargetDevice(self, device):
    self.test_target_devs.append(device)

  def DeleteAllItems(self):
    self.test_target_devs = []

  # Test atft._DeviceListedEventHandler
  def testDeviceListedEventHandler(self):
    mock_atft = MockAtft()
    mock_atft.atfa_devs_output = MagicMock()
    mock_atft.last_target_list = []
    mock_atft.target_devs_output = MagicMock()
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.atfa_dev = None
    mock_atft.PrintToWindow = MagicMock()
    mock_atft._HandleKeysLeft = MagicMock()
    mock_atft.target_devs_output.Append.side_effect = self.AppendTargetDevice
    (mock_atft.target_devs_output.DeleteAllItems.side_effect
    ) = self.DeleteAllItems
    mock_atft.atft_manager.target_devs = []
    mock_atft._DeviceListedEventHandler(None)
    mock_atft.atft_manager.target_devs = [self.test_dev1]
    mock_atft._DeviceListedEventHandler(None)
    mock_atft.target_devs_output.Append.assert_called_once()
    self.assertEqual(1, len(self.test_target_devs))
    self.assertEqual(self.test_dev1.serial_number, self.test_target_devs[0][0])
    mock_atft.atft_manager.target_devs = [self.test_dev1, self.test_dev2]
    mock_atft._DeviceListedEventHandler(None)
    self.assertEqual(2, len(self.test_target_devs))
    self.assertEqual(self.test_dev2.serial_number, self.test_target_devs[1][0])
    mock_atft.atft_manager.target_devs = [self.test_dev1, self.test_dev2]
    mock_atft.target_devs_output.Append.reset_mock()
    mock_atft._DeviceListedEventHandler(None)
    mock_atft.target_devs_output.Append.assert_not_called()
    mock_atft.atft_manager.target_devs = [self.test_dev2, self.test_dev1]
    mock_atft.target_devs_output.Append.reset_mock()
    mock_atft._DeviceListedEventHandler(None)
    mock_atft.target_devs_output.Append.assert_called()
    self.assertEqual(2, len(self.test_target_devs))
    mock_atft.atft_manager.target_devs = [self.test_dev2]
    mock_atft._DeviceListedEventHandler(None)
    self.assertEqual(1, len(self.test_target_devs))
    self.assertEqual(self.test_dev2.serial_number, self.test_target_devs[0][0])
    mock_atft.atft_manager.target_devs = []
    mock_atft._DeviceListedEventHandler(None)
    self.assertEqual(0, len(self.test_target_devs))

  # Test atft._SelectFileEventHandler
  @patch('wx.FileDialog')
  def testSelectFileEventHandler(self, mock_file_dialog):
    mock_atft = MockAtft()
    mock_event = MagicMock()
    mock_callback = MagicMock()
    mock_dialog = MagicMock()
    mock_instance = MagicMock()
    mock_file_dialog.return_value = mock_instance
    mock_instance.__enter__.return_value = mock_dialog
    mock_event.GetValue.return_value = (mock_atft.SelectFileArg(
        self.TEST_TEXT, self.TEST_TEXT2, mock_callback))
    mock_dialog.ShowModal.return_value = wx.ID_OK
    mock_dialog.GetPath.return_value = self.TEST_TEXT
    mock_atft._SelectFileEventHandler(mock_event)
    mock_callback.assert_called_once_with(self.TEST_TEXT)

  @patch('wx.FileDialog')
  def testSelectFileEventHandlerCancel(self, mock_file_dialog):
    mock_atft = MockAtft()
    mock_event = MagicMock()
    mock_callback = MagicMock()
    mock_dialog = MagicMock()
    mock_instance = MagicMock()
    mock_file_dialog.return_value = mock_instance
    mock_instance.__enter__.return_value = mock_dialog
    mock_event.GetValue.return_value = (mock_atft.SelectFileArg(
        self.TEST_TEXT, self.TEST_TEXT2, mock_callback))
    mock_dialog.ShowModal.return_value = wx.ID_CANCEL
    mock_dialog.GetPath.return_value = self.TEST_TEXT
    mock_atft._SelectFileEventHandler(mock_event)
    mock_callback.assert_not_called()

  # Test atft.PrintToWindow
  def MockAppendText(self, text):
    self.test_text_window += text

  def MockClear(self):
    self.test_text_window = ''

  def MockGetValue(self):
    return self.test_text_window

  def testPrintToWindow(self):
    self.test_text_window = ''
    mock_atft = MockAtft()
    mock_text_entry = MagicMock()
    mock_text_entry.AppendText.side_effect = self.MockAppendText
    mock_text_entry.Clear.side_effect = self.MockClear
    mock_text_entry.GetValue.side_effect = self.MockGetValue
    mock_atft.PrintToWindow(mock_text_entry, self.TEST_TEXT)
    self.assertEqual(self.TEST_TEXT, self.test_text_window)
    mock_atft.PrintToWindow(mock_text_entry, self.TEST_TEXT2)
    self.assertEqual(self.TEST_TEXT2, self.test_text_window)
    mock_text_entry.AppendText.reset_mock()
    mock_atft.PrintToWindow(mock_text_entry, self.TEST_TEXT2)
    self.assertEqual(False, mock_text_entry.AppendText.called)
    self.assertEqual(self.TEST_TEXT2, self.test_text_window)
    mock_text_entry.Clear()
    mock_atft.PrintToWindow(mock_text_entry, self.TEST_TEXT, True)
    mock_atft.PrintToWindow(mock_text_entry, self.TEST_TEXT2, True)
    self.assertEqual(self.TEST_TEXT + self.TEST_TEXT2, self.test_text_window)

  # Test atft.StartRefreshingDevices(), atft.StopRefresh()
  # Test atft.PauseRefresh(), atft.ResumeRefresh()

  @patch('threading.Timer')
  @patch('wx.QueueEvent')
  def testStartRefreshingDevice(self, mock_queue_event, mock_timer):
    mock_atft = MockAtft()
    mock_atft.StartRefreshingDevices = types.MethodType(
        atft.Atft.StartRefreshingDevices, mock_atft, atft.Atft)
    mock_atft.DEVICE_REFRESH_INTERVAL = 0.01
    mock_atft._ListDevices = MagicMock()
    mock_atft.dev_listed_event = MagicMock()
    mock_timer.side_effect = MagicMock()

    mock_atft.StartRefreshingDevices()

    mock_atft._ListDevices.assert_called()
    mock_atft.StopRefresh()
    self.assertEqual(None, mock_atft.refresh_timer)

  @patch('threading.Timer')
  def testPauseResumeRefreshingDevice(self, mock_timer):
    mock_atft = MockAtft()
    mock_atft.StartRefreshingDevices = types.MethodType(
        atft.Atft.StartRefreshingDevices, mock_atft, atft.Atft)
    mock_atft.DEVICE_REFRESH_INTERVAL = 0.01
    mock_atft._ListDevices = MagicMock()
    mock_atft.dev_listed_event = MagicMock()
    mock_atft._SendDeviceListedEvent = MagicMock()
    mock_timer.side_effect = MagicMock()

    mock_atft.PauseRefresh()
    mock_atft.StartRefreshingDevices()
    mock_atft._ListDevices.assert_not_called()
    mock_atft._SendDeviceListedEvent.assert_called()
    mock_atft.ResumeRefresh()
    mock_atft.StartRefreshingDevices()
    mock_atft._ListDevices.assert_called()
    mock_atft.StopRefresh()

  # Test atft.OnToggleAutoProv
  def testOnEnterAutoProvNormal(self):
    mock_atft = MockAtft()
    mock_atft.StartRefreshingDevices = types.MethodType(
        atft.Atft.StartRefreshingDevices, mock_atft, atft.Atft)
    mock_atft.toolbar = MagicMock()
    mock_atft.toolbar_auto_provision = MagicMock()
    mock_atft.toolbar_auto_provision.IsToggled.return_value = True
    mock_atft.atft_manager.atfa_dev = MagicMock()
    mock_atft.atft_manager.product_info = MagicMock()
    mock_atft._ToggleToolbarMenu = MagicMock()
    mock_atft.PrintToCommandWindow = MagicMock()
    mock_atft._CreateThread = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft.OnToggleAutoProv(None)
    self.assertEqual(True, mock_atft.auto_prov)

  def testOnEnterAutoProvNoAtfa(self):
    # Cannot enter auto provisioning mode without an ATFA device
    mock_atft = MockAtft()
    mock_atft.toolbar = MagicMock()
    mock_atft.toolbar_auto_provision = MagicMock()
    mock_atft.toolbar_auto_provision.IsToggled.return_value = True
    # No ATFA device
    mock_atft.atft_manager.atfa_dev = None
    mock_atft.atft_manager.product_info = MagicMock()
    mock_atft._ToggleToolbarMenu = MagicMock()
    mock_atft.PrintToCommandWindow = MagicMock()
    mock_atft._CreateThread = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft.OnToggleAutoProv(None)
    self.assertEqual(False, mock_atft.auto_prov)

  def testOnEnterAutoProvNoProduct(self):
    # Cannot enter auto provisioning mode without an ATFA device
    mock_atft = MockAtft()
    mock_atft.toolbar = MagicMock()
    mock_atft.toolbar_auto_provision = MagicMock()
    mock_atft.toolbar_auto_provision.IsToggled.return_value = True
    mock_atft.atft_manager.atfa_dev = MagicMock()
    # No product info
    mock_atft.atft_manager.product_info = None
    mock_atft._ToggleToolbarMenu = MagicMock()
    mock_atft.PrintToCommandWindow = MagicMock()
    mock_atft._CreateThread = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft.OnToggleAutoProv(None)
    self.assertEqual(False, mock_atft.auto_prov)

  def testLeaveAutoProvNormal(self):
    mock_atft = MockAtft()
    mock_atft.toolbar = MagicMock()
    mock_atft.toolbar_auto_provision = MagicMock()
    mock_atft.toolbar_auto_provision.IsToggled.return_value = False
    mock_atft.atft_manager.atfa_dev = MagicMock()
    mock_atft.atft_manager.product_info = MagicMock()
    mock_atft._ToggleToolbarMenu = MagicMock()
    mock_atft.PrintToCommandWindow = MagicMock()
    mock_atft._CreateThread = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft.atft_manager.target_devs = []
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.PROVISION_SUCCESS)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft.atft_manager.target_devs.append(test_dev1)
    mock_atft.atft_manager.target_devs.append(test_dev2)
    mock_atft.atft_manager.CheckProvisionStatus.side_effect = (
        lambda target=test_dev2, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft.OnToggleAutoProv(None)
    self.assertEqual(False, mock_atft.auto_prov)
    self.assertEqual(test_dev1.provision_status,
                     ProvisionStatus.PROVISION_SUCCESS)
    mock_atft.atft_manager.CheckProvisionStatus.assert_called_once()
    self.assertEqual(test_dev2.provision_status, ProvisionStatus.LOCKAVB_SUCCESS)

  # Test atft.OnChangeKeyThreshold
  def testOnChangeKeyThreshold(self):
    mock_atft = MockAtft()
    mock_atft.change_threshold_dialog = MagicMock()
    mock_atft.change_threshold_dialog.ShowModal.return_value = wx.ID_OK
    mock_atft.change_threshold_dialog.GetValue.return_value = '100'
    mock_atft.OnChangeKeyThreshold(None)
    self.assertEqual(100, mock_atft.key_threshold)

  def testOnChangeKeyThresholdNotInt(self):
    mock_atft = MockAtft()
    mock_atft.key_threshold = 100
    mock_atft.change_threshold_dialog = MagicMock()
    mock_atft.change_threshold_dialog.ShowModal.return_value = wx.ID_OK
    mock_atft.change_threshold_dialog.GetValue.return_value = 'ABC'
    mock_atft.OnChangeKeyThreshold(None)
    self.assertEqual(100, mock_atft.key_threshold)
    mock_atft.change_threshold_dialog.GetValue.return_value = '2'
    mock_atft.OnChangeKeyThreshold(None)
    self.assertEqual(2, mock_atft.key_threshold)
    mock_atft.change_threshold_dialog.GetValue.return_value = '-10'
    mock_atft.OnChangeKeyThreshold(None)
    self.assertEqual(2, mock_atft.key_threshold)

  # Test atft._HandleAutoProv
  def testHandleAutoProv(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.PROVISION_SUCCESS)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION1,
                               ProvisionStatus.IDLE)
    mock_atft.atft_manager.target_devs = []
    mock_atft.atft_manager.target_devs.append(test_dev1)
    mock_atft.atft_manager.target_devs.append(test_dev2)
    mock_atft._CreateThread = MagicMock()
    mock_atft._HandleStateTransition = MagicMock()
    mock_atft._HandleAutoProv()
    self.assertEqual(test_dev2.provision_status, ProvisionStatus.WAITING)
    self.assertEqual(1, mock_atft._CreateThread.call_count)

  # Test atft._HandleKeysLeft
  def MockGetKeysLeft(self, keys_left_array):
    if keys_left_array:
      return keys_left_array[0]
    else:
      return None

  def MockSetKeysLeft(self, keys_left_array):
    keys_left_array.append(10)

  def testHandleKeysLeft(self):
    mock_atft = MockAtft()
    keys_left_array = []
    mock_atft.atft_manager.GetATFAKeysLeft = MagicMock()
    mock_atft.atft_manager.GetATFAKeysLeft.side_effect = (
        lambda: self.MockGetKeysLeft(keys_left_array))
    mock_atft.atft_manager.CheckATFAStatus.side_effect = (
        lambda: self.MockSetKeysLeft(keys_left_array))
    mock_atft.keys_left_display = MagicMock()
    mock_atft._HandleKeysLeft()
    mock_atft.keys_left_display.SetLabelText.assert_called_once_with('10')

  def testHandleKeysLeftKeysNotNone(self):
    mock_atft = MockAtft()
    keys_left_array = [10]
    mock_atft.atft_manager.GetATFAKeysLeft = MagicMock()
    mock_atft.atft_manager.GetATFAKeysLeft.side_effect = (
        lambda: self.MockGetKeysLeft(keys_left_array))
    mock_atft.keys_left_display = MagicMock()
    mock_atft._HandleKeysLeft()
    mock_atft.keys_left_display.SetLabelText.assert_called_once_with('10')
    mock_atft.atft_manager.CheckATFAStatus.assert_not_called()

  def testHandleKeysLeftKeysNone(self):
    mock_atft = MockAtft()
    keys_left_array = []
    mock_atft.atft_manager.GetATFAKeysLeft = MagicMock()
    mock_atft.atft_manager.GetATFAKeysLeft.side_effect = (
        lambda: self.MockGetKeysLeft(keys_left_array))
    mock_atft.keys_left_display = MagicMock()
    mock_atft._HandleKeysLeft()
    mock_atft.keys_left_display.SetLabelText.assert_called_once_with('')

  # Test atft._HandleStateTransition
  def MockStateChange(self, target, state):
    target.provision_status = state
    if state == ProvisionStatus.REBOOT_SUCCESS:
      target.provision_state.bootloader_locked = True
    if state == ProvisionStatus.FUSEATTR_SUCCESS:
      target.provision_state.avb_perm_attr_set = True
    if state == ProvisionStatus.LOCKAVB_SUCCESS:
      target.provision_state.avb_locked = True
    if state == ProvisionStatus.PROVISION_SUCCESS:
      target.provision_state.provisioned = True

  def testHandleStateTransition(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.PROVISION_SUCCESS,
                     test_dev1.provision_status)

  def testHandleStateTransitionFuseVbootFail(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEVBOOT_FAILED:
            self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
            self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
            self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
            self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.FUSEVBOOT_FAILED,
                     test_dev1.provision_status)

  def testHandleStateTransitionRebootFail(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_FAILED:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.REBOOT_FAILED, test_dev1.provision_status)

  def testHandleStateTransitionFuseAttrFail(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_FAILED:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.FUSEATTR_FAILED,
                     test_dev1.provision_status)

  def testHandleStateTransitionLockAVBFail(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_FAILED:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.LOCKAVB_FAILED, test_dev1.provision_status)

  def testHandleStateTransitionProvisionFail(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_FAILED:
        self.MockStateChange(target, state))
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.PROVISION_FAILED,
                     test_dev1.provision_status)

  def testHandleStateTransitionSkipStep(self):
    mock_atft = MockAtft()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft._FuseVbootKeyTarget = MagicMock()
    mock_atft._FuseVbootKeyTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.REBOOT_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._FusePermAttrTarget = MagicMock()
    mock_atft._FusePermAttrTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.FUSEATTR_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._LockAvbTarget = MagicMock()
    mock_atft._LockAvbTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.LOCKAVB_SUCCESS:
        self.MockStateChange(target, state))
    mock_atft._ProvisionTarget = MagicMock()
    mock_atft._ProvisionTarget.side_effect = (
        lambda target=mock_atft, state=ProvisionStatus.PROVISION_SUCCESS:
        self.MockStateChange(target, state))

    # The device has bootloader_locked and avb_locked set. Should fuse perm attr
    # and provision key.
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_locked = True
    mock_atft.auto_dev_serials = [self.TEST_SERIAL1]
    mock_atft.auto_prov = True
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager.GetTargetDevice = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.return_value = test_dev1
    mock_atft._HandleStateTransition(test_dev1)
    self.assertEqual(ProvisionStatus.PROVISION_SUCCESS,
                     test_dev1.provision_status)
    mock_atft._FusePermAttrTarget.assert_called_once()
    mock_atft._ProvisionTarget.assert_called_once()
    self.assertEqual(True, test_dev1.provision_state.bootloader_locked)
    self.assertEqual(True, test_dev1.provision_state.avb_perm_attr_set)
    self.assertEqual(True, test_dev1.provision_state.avb_locked)
    self.assertEqual(True, test_dev1.provision_state.provisioned)

  # Test atft._CheckATFAStatus
  def testCheckATFAStatus(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._CheckATFAStatus()
    mock_atft.atft_manager.CheckATFAStatus.assert_called()

  # Test atft._FuseVbootKey
  def MockGetTargetDevice(self, serial):
    return self.device_map.get(serial)

  def MockReboot(self, target, timeout, success, fail):
    success()
    target.provision_state.bootloader_locked = True

  @patch('wx.QueueEvent')
  def testFuseVbootKey(self, mock_queue_event):
    mock_atft = MockAtft()
    mock_atft.dev_listed_event = MagicMock()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.IDLE)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_FAILED)
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft.atft_manager.Reboot.side_effect = self.MockReboot
    mock_atft._FuseVbootKey(serials)
    calls = [call(test_dev1), call(test_dev2)]
    mock_atft.atft_manager.FuseVbootKey.assert_has_calls(calls)
    mock_queue_event.assert_called()

  @patch('wx.QueueEvent')
  def testFuseVbootKeyExceptions(self, mock_queue_event):
    mock_atft = MockAtft()
    mock_atft.dev_listed_event = MagicMock()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.IDLE)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_FAILED)
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft.atft_manager.Reboot.side_effect = self.MockReboot
    mock_atft.atft_manager.FuseVbootKey.side_effect = (
        fastboot_exceptions.ProductNotSpecifiedException)
    mock_atft._FuseVbootKey(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)
    mock_queue_event.assert_not_called()

    # Reset states.
    mock_atft._HandleException.reset_mock()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.IDLE)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_FAILED)
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft.atft_manager.FuseVbootKey.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._FuseVbootKey(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)

    # Reset states, test reboot failure
    mock_atft._HandleException.reset_mock()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.IDLE)
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_FAILED)
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft.atft_manager.Reboot.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft.atft_manager.FuseVbootKey = MagicMock()
    mock_atft._FuseVbootKey(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)

  # Test atft._FusePermAttr
  def testFusePermAttr(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(
        self.TEST_SERIAL1, self.TEST_LOCATION1,
        ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev1.provision_state.bootloader_locked = True
    test_dev2 = TestDeviceInfo(
        self.TEST_SERIAL2, self.TEST_LOCATION2,
        ProvisionStatus.REBOOT_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev3 = TestDeviceInfo(
        self.TEST_SERIAL3, self.TEST_LOCATION2,
        ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    test_dev4 = TestDeviceInfo(
        self.TEST_SERIAL4, self.TEST_LOCATION2,
        ProvisionStatus.FUSEATTR_SUCCESS)
    test_dev4.provision_state.bootloader_locked = True
    test_dev4.provision_state.avb_perm_attr_set = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    self.device_map[self.TEST_SERIAL4] = test_dev4
    serials = [
        self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3,
        self.TEST_SERIAL4
    ]
    mock_atft._FusePermAttr(serials)
    calls = [call(test_dev1), call(test_dev2), call(test_dev3)]
    mock_atft.atft_manager.FusePermAttr.assert_has_calls(calls)

  def testFusePermAttrExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev1.provision_state.bootloader_locked = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.REBOOT_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    test_dev4 = TestDeviceInfo(self.TEST_SERIAL4, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_SUCCESS)
    test_dev4.provision_state.bootloader_locked = True
    test_dev4.provision_state.avb_perm_attr_set = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    self.device_map[self.TEST_SERIAL4] = test_dev4
    serials = [
        self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3,
        self.TEST_SERIAL4
    ]
    mock_atft.atft_manager.FusePermAttr.side_effect = (
        fastboot_exceptions.ProductNotSpecifiedException)
    mock_atft._FusePermAttr(serials)
    self.assertEqual(3, mock_atft._HandleException.call_count)
    # Reset states
    mock_atft._HandleException.reset_mock()
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.FUSEVBOOT_SUCCESS)
    test_dev1.provision_state.bootloader_locked = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.REBOOT_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    test_dev4 = TestDeviceInfo(self.TEST_SERIAL4, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_SUCCESS)
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    self.device_map[self.TEST_SERIAL4] = test_dev4
    mock_atft.atft_manager.FusePermAttr.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._FusePermAttr(serials)
    self.assertEqual(3, mock_atft._HandleException.call_count)

  # Test atft._LockAvb
  def testLockAvb(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.FUSEATTR_SUCCESS)
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_perm_attr_set = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.LOCKAVB_FAILED)
    test_dev2.provision_state.bootloader_locked = True
    test_dev2.provision_state.avb_perm_attr_set = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    test_dev3.provision_state.avb_perm_attr_set = False
    test_dev4 = TestDeviceInfo(self.TEST_SERIAL4, self.TEST_LOCATION2,
                               ProvisionStatus.IDLE)
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    self.device_map[self.TEST_SERIAL4] = test_dev4
    serials = [
        self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3,
        self.TEST_SERIAL4
    ]
    mock_atft._LockAvb(serials)
    calls = [call(test_dev1), call(test_dev2)]
    mock_atft.atft_manager.LockAvb.assert_has_calls(calls)

  def testLockAvbExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._SendMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.FUSEATTR_SUCCESS)
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_perm_attr_set = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.LOCKAVB_FAILED)
    test_dev2.provision_state.bootloader_locked = True
    test_dev2.provision_state.avb_perm_attr_set = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    test_dev3.provision_state.avb_perm_attr_set = False
    test_dev4 = TestDeviceInfo(self.TEST_SERIAL4, self.TEST_LOCATION2,
                               ProvisionStatus.IDLE)
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    self.device_map[self.TEST_SERIAL4] = test_dev4
    serials = [
        self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3,
        self.TEST_SERIAL4
    ]
    mock_atft.atft_manager.LockAvb.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._LockAvb(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)

  # Test atft._CheckLowKeyAlert
  def MockCheckATFAStatus(self):
    return self.atfa_keys

  def MockSuccessProvision(self, target):
    self.atfa_keys -= 1

  def MockFailedProvision(self, target):
    pass

  def testCheckLowKeyAlert(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendLowKeyAlertEvent = MagicMock()
    mock_atft.key_threshold = 100
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.WAITING)
    mock_atft.atft_manager.GetATFAKeysLeft.side_effect = (
        self.MockCheckATFAStatus)
    self.atfa_keys = 102
    # First provision succeed
    # First check 101 left, no alert
    mock_atft.atft_manager.Provision.side_effect = self.MockSuccessProvision
    mock_atft._ProvisionTarget(test_dev1)
    mock_atft._SendLowKeyAlertEvent.assert_not_called()
    # Second provision failed
    # Second check, assume 100 left, verify, 101 left no alert
    mock_atft.atft_manager.Provision.side_effect = self.MockFailedProvision
    mock_atft._ProvisionTarget(test_dev1)
    mock_atft._SendLowKeyAlertEvent.assert_not_called()
    # Third check, assuem 100 left, verify, 100 left, alert
    mock_atft.atft_manager.Provision.side_effect = self.MockSuccessProvision
    mock_atft._ProvisionTarget(test_dev1)
    mock_atft._SendLowKeyAlertEvent.assert_called()

  def testCheckLowKeyAlertException(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SendLowKeyAlertEvent = MagicMock()
    mock_atft.key_threshold = 100
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.CheckATFAStatus.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._CheckLowKeyAlert()
    mock_atft._HandleException.assert_called_once()
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.CheckATFAStatus.side_effect = (
        fastboot_exceptions.ProductNotSpecifiedException)
    mock_atft._CheckLowKeyAlert()
    mock_atft._HandleException.assert_called_once()
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.CheckATFAStatus.side_effect = (
        fastboot_exceptions.DeviceNotFoundException)
    mock_atft._CheckLowKeyAlert()
    mock_atft._HandleException.assert_called_once()

  # Test atft._SwitchStorageMode
  def testSwitchStorageMode(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._SwitchStorageMode()
    mock_atft.atft_manager.SwitchATFAStorage.assert_called_once()

  def testSwitchStorageModeExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.SwitchATFAStorage.side_effect = (
        fastboot_exceptions.DeviceNotFoundException())
    mock_atft._SwitchStorageMode()
    mock_atft._HandleException.assert_called_once()
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.SwitchATFAStorage.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._SwitchStorageMode()
    mock_atft._HandleException.assert_called_once()

  # Test atft._Reboot
  def testReboot(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._Reboot()
    mock_atft.atft_manager.RebootATFA.assert_called_once()

  def testRebootExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.RebootATFA.side_effect = (
        fastboot_exceptions.DeviceNotFoundException())
    mock_atft._Reboot()
    mock_atft._HandleException.assert_called_once()
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.RebootATFA.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._Reboot()
    mock_atft._HandleException.assert_called_once()

  # Test atft._Shutdown
  def testShutdown(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._Shutdown()
    mock_atft.atft_manager.ShutdownATFA.assert_called_once()

  def testShutdownExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.ShutdownATFA.side_effect = (
        fastboot_exceptions.DeviceNotFoundException())
    mock_atft._Shutdown()
    mock_atft._HandleException.assert_called_once()
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.ShutdownATFA.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._Shutdown()
    mock_atft._HandleException.assert_called_once()

  # Test atft._ManualProvision
  def testManualProvision(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.Provision = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.PROVISION_FAILED)
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_perm_attr_set = True
    test_dev1.provision_state.avb_locked = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.LOCKAVB_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev2.provision_state.avb_perm_attr_set = True
    test_dev2.provision_state.avb_locked = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft._ManualProvision(serials)
    calls = [call(test_dev1), call(test_dev2)]
    mock_atft.atft_manager.Provision.assert_has_calls(calls)

  def testManualProvisionExceptions(self):
    mock_atft = MockAtft()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._SendStartMessageEvent = MagicMock()
    mock_atft._SendSucceedMessageEvent = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.Provision = MagicMock()
    mock_atft._SendAlertEvent = MagicMock()
    mock_atft._CheckLowKeyAlert = MagicMock()
    mock_atft.atft_manager.GetTargetDevice.side_effect = (
        self.MockGetTargetDevice)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.PROVISION_FAILED)
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_perm_attr_set = True
    test_dev1.provision_state.avb_locked = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.LOCKAVB_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev2.provision_state.avb_perm_attr_set = True
    test_dev2.provision_state.avb_locked = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft.atft_manager.Provision.side_effect = (
        fastboot_exceptions.FastbootFailure(''))
    mock_atft._ManualProvision(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)
    test_dev1 = TestDeviceInfo(self.TEST_SERIAL1, self.TEST_LOCATION1,
                               ProvisionStatus.PROVISION_FAILED)
    test_dev1.provision_state.bootloader_locked = True
    test_dev1.provision_state.avb_perm_attr_set = True
    test_dev1.provision_state.avb_locked = True
    test_dev2 = TestDeviceInfo(self.TEST_SERIAL2, self.TEST_LOCATION2,
                               ProvisionStatus.LOCKAVB_SUCCESS)
    test_dev2.provision_state.bootloader_locked = True
    test_dev2.provision_state.avb_perm_attr_set = True
    test_dev2.provision_state.avb_locked = True
    test_dev3 = TestDeviceInfo(self.TEST_SERIAL3, self.TEST_LOCATION2,
                               ProvisionStatus.FUSEATTR_FAILED)
    test_dev3.provision_state.bootloader_locked = True
    self.device_map[self.TEST_SERIAL1] = test_dev1
    self.device_map[self.TEST_SERIAL2] = test_dev2
    self.device_map[self.TEST_SERIAL3] = test_dev3
    serials = [self.TEST_SERIAL1, self.TEST_SERIAL2, self.TEST_SERIAL3]
    mock_atft._HandleException.reset_mock()
    mock_atft.atft_manager.Provision.side_effect = (
        fastboot_exceptions.DeviceNotFoundException())
    mock_atft._ManualProvision(serials)
    self.assertEqual(2, mock_atft._HandleException.call_count)

  # Test atft._ProcessKey
  def testProcessKeySuccess(self):
    mock_atft = MockAtft()
    mock_atft.atft_manager = MagicMock()
    mock_atft.atft_manager._atfa_dev_manager = MagicMock()
    mock_atft._CheckATFAStatus = MagicMock()
    mock_atft._SendOperationStartEvent = MagicMock()
    mock_atft._SendOperationSucceedEvent = MagicMock()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._HandleException = MagicMock()

    mock_atft._ProcessKey()
    mock_atft.PauseRefresh.assert_called_once()
    mock_atft.ResumeRefresh.assert_called_once()
    mock_atft._HandleException.assert_not_called()

    mock_atft._SendOperationStartEvent.assert_called_once()
    mock_atft._SendOperationSucceedEvent.assert_called_once()
    mock_atft._CheckATFAStatus.assert_called_once()

  def testProcessKeyFailure(self):
    self.TestProcessKeyFailureCommon(fastboot_exceptions.FastbootFailure(''))
    self.TestProcessKeyFailureCommon(
        fastboot_exceptions.DeviceNotFoundException)

  def TestProcessKeyFailureCommon(self, exception):
    mock_atft = MockAtft()
    mock_atft.atft_manager = MagicMock()
    mock_atft._SendOperationStartEvent = MagicMock()
    mock_atft._SendOperationSucceedEvent = MagicMock()
    mock_atft.PauseRefresh = MagicMock()
    mock_atft.ResumeRefresh = MagicMock()
    mock_atft._HandleException = MagicMock()
    mock_atft.atft_manager.ProcessATFAKey = MagicMock()
    mock_atft.atft_manager.ProcessATFAKey.side_effect = exception

    mock_atft._ProcessKey()
    mock_atft.PauseRefresh.assert_called_once()
    mock_atft.ResumeRefresh.assert_called_once()
    mock_atft._HandleException.assert_called_once()

    mock_atft._SendOperationStartEvent.assert_called_once()
    mock_atft._SendOperationSucceedEvent.assert_not_called()


if __name__ == '__main__':
  unittest.main()
