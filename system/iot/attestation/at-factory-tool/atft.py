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

"""Graphical tool for managing the ATFA and AT communication.

This tool allows for easy graphical access to common ATFA commands.  It also
locates Fastboot devices and can initiate communication between the ATFA and
an Android Things device.
"""
from datetime import datetime
import json
import math
import os
import sys
import tempfile
import threading

from atftman import AtftManager
from atftman import ProvisionStatus
from fastboot_exceptions import DeviceNotFoundException
from fastboot_exceptions import FastbootFailure
from fastboot_exceptions import ProductAttributesFileFormatError
from fastboot_exceptions import ProductNotSpecifiedException

import wx

if sys.platform.startswith('linux'):
  from fastbootsh import FastbootDevice
  from serialmapperlinux import SerialMapper
elif sys.platform.startswith('win'):
  from fastbootsubp import FastbootDevice
  from serialmapperwin import SerialMapper


# If this is set to True, no prerequisites would be checked against manual
# operation, such as you can do key provisioning before fusing the vboot key.
TEST_MODE = False


class AtftException(Exception):
  """The exception class to include device and operation information.
  """

  def __init__(self, exception, operation=None, target=None):
    """Init the exception class.

    Args:
      exception: The original exception object.
      operation: The operation that generates this exception.
      target: The operating target device.
    """
    Exception.__init__(self)
    self.exception = exception
    self.operation = operation
    self.target = target

  def __str__(self):
    msg = ''
    if self.target:
      msg += '{' + str(self.target) + '} '
    if self.operation:
      msg += self.operation + ' Failed! \n'
    msg += self._AddExceptionType(self.exception)
    return msg

  def _AddExceptionType(self, e):
    """Format the exception. Concatenate the exception type with the message.

    Args:
      e: The exception to be printed.
    Returns:
      The exception message.
    """
    return '{0}: {1}'.format(e.__class__.__name__, e)


class AtftLog(object):
  """The class to handle logging.

  Logs would be created under LOG_DIR with the time stamp when the log is
  created as file name. There would be at most LOG_FILE_NUMBER log files and
  each log file size would be less than log_size/log_file_number, so the total
  log size would less than log_size.
  """

  def __init__(self, log_dir, log_size, log_file_number):
    """Initiate the AtftLog object.

    This function would also write the first 'Program Start' log entry.

    Args:
      log_dir: The directory to store logs.
      log_size: The maximum total size for all the log files.
      log_file_number: The maximum number for log files.
    """
    if not os.path.exists(log_dir):
      # If log directory does not exist, try to create it.
      try:
        os.mkdir(log_dir)
      except IOError:
        return
    self.log_dir = log_dir
    self.log_dir_file = None
    self.file_size = 0
    self.log_size = log_size
    self.log_file_number = log_file_number
    self.file_size_max = math.floor(self.log_size / self.log_file_number)
    self.lock = threading.Lock()

    log_files = []
    for file_name in os.listdir(self.log_dir):
      if (os.path.isfile(os.path.join(self.log_dir, file_name)) and
          file_name.startswith('atft_log_')):
        log_files.append(file_name)
    if not log_files:
      # Create the first log file.
      self._CreateLogFile()
    else:
      log_files.sort()
      self.log_dir_file = os.path.join(self.log_dir, log_files.pop())
    self.Info('Program', 'Program start')

  def Error(self, tag, string):
    """Print an error message to the log.

    Args:
      tag: The tag for the message.
      string: The error message.
    """
    self._Output('E', tag, string)

  def Debug(self, tag, string):
    """Print a debug message to the log.

    Args:
      tag: The tag for the message.
      string: The debug message.
    """
    self._Output('D', tag, string)

  def Warning(self, tag, string):
    """Print a warning message to the log.

    Args:
      tag: The tag for the message.
      string: The warning message.
    """
    self._Output('W', tag, string)

  def Info(self, tag, string):
    """Print an info message to the log.

    Args:
      tag: The tag for the message.
      string: The info message.
    """
    self._Output('I', tag, string)

  def _Output(self, code, tag, string):
    """Output a line of message to the log file.

    Args:
      code: The log level.
      tag: The log tag.
      string: The log message.
    """
    time = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
    message = '[{0}] {1}/{2}: {3}'.format(
        time, code, tag, string.replace('\n', '\t'))
    if self.log_dir_file:
      message += '\n'
      with self.lock:
        self._LimitSize(message)
        with open(self.log_dir_file, 'a') as log_file:
          log_file.write(message)
          log_file.flush()

  def _LimitSize(self, message):
    """This function limits the total size of logs.

    It would create a new log file if the log file is too large. If the total
    number of log files is larger than threshold, then it would delete the
    oldest log.

    Args:
      message: The log message about to be added.
    """
    file_size = os.path.getsize(self.log_dir_file)
    if file_size + len(message) > self.file_size_max:
      # If file size will exceed file_size_max, then create a new file and close
      # the current one.
      self._CreateLogFile()
    log_files = []
    for file_name in os.listdir(self.log_dir):
      if (os.path.isfile(os.path.join(self.log_dir, file_name)) and
          file_name.startswith('atft_log_')):
        log_files.append(file_name)

    if len(log_files) > self.log_file_number:
      # If file number exceeds LOG_FILE_NUMBER, then delete the oldest file.
      try:
        log_files.sort()
        oldest_file = os.path.join(self.log_dir, log_files[0])
        os.remove(oldest_file)
      except IOError:
        pass

  def _CreateLogFile(self):
    """Create a new log file using timestamp as file name.
    """
    timestamp = int((datetime.now() - datetime(1970, 1, 1)).total_seconds())
    log_file_name = 'atft_log_' + str(timestamp)
    log_file_path = os.path.join(self.log_dir, log_file_name)
    i = 1
    while os.path.exists(log_file_path):
      # If already exists, create another name, timestamp_1, timestamp_2, etc.
      log_file_name_new = log_file_name + '_' + str(i)
      log_file_path = os.path.join(self.log_dir, log_file_name_new)
      i += 1
    try:
      log_file = open(log_file_path, 'w+')
      log_file.close()
      self.log_dir_file = log_file_path
    except IOError:
      self.log_dir_file = None

  def __del__(self):
    """Cleanup function. This would log the 'Program Exit' message.
    """
    self.Info('Program', 'Program exit')


class Event(wx.PyCommandEvent):
  """The customized event class.
  """

  def __init__(self, etype, eid=-1, value=None):
    """Create a new customized event.

    Args:
      etype: The event type.
      eid: The event id.
      value: The additional data included in the event.
    """
    wx.PyCommandEvent.__init__(self, etype, eid)
    self._value = value

  def GetValue(self):
    """Get the data included in this event.

    Returns:
      The event data.
    """
    return self._value

class Atft(wx.Frame):
  """wxpython class to handle all GUI commands for the ATFA.

  Creates the GUI and provides various functions for interacting with an
  ATFA and an Android Things device.

  """
  CONFIG_FILE = 'config.json'

  ID_TOOL_PROVISION = 1
  ID_TOOL_CLEAR = 2

  def __init__(self):

    self.configs = self.ParseConfigFile()

    self.SetLanguage()

    self.TITLE += ' ' + self.ATFT_VERSION

    # The atft_manager instance to manage various operations.
    self.atft_manager = self.CreateAtftManager()

    # The target devices refresh timer object
    self.refresh_timer = None

    # The field to sort target devices
    self.sort_by = self.atft_manager.SORT_BY_LOCATION

    # List of serial numbers for the devices in auto provisioning mode.
    self.auto_dev_serials = []

    # Store the last refreshed target list, we use this list to prevent
    # refreshing the same list.
    self.last_target_list = []

    # Indicate whether in auto provisioning mode.
    self.auto_prov = False

    # Indicate whether refresh is paused. If we could acquire this lock, this
    # means that the refresh is paused. We would pause the refresh during each
    # fastboot command since on Windows, a fastboot device would disappear from
    # fastboot devices while a fastboot command is issued.
    self.refresh_pause_lock = threading.Semaphore(value=0)

    # 'fastboot devices' can only run sequentially, so we use this lock to check
    # if there's already a 'fastboot devices' command running. If so, we ignore
    # the second request.
    self.listing_device_lock = threading.Lock()

    # To prevent low key alert to show by each provisioning.
    # We only show it once per auto provision.
    self.low_key_alert_shown = False

    # Lock to make sure only one device is doing auto provisioning at one time.
    self.auto_prov_lock = threading.Lock()

    # Lock for showing alert box
    self.alert_lock = threading.Lock()
    # The key threshold, if the number of attestation key in the ATFA device
    # is lower than this number, an alert would appear.
    self.key_threshold = self.DEFAULT_KEY_THRESHOLD

    self.InitializeUI()

    if self.configs == None:
      self.ShowAlert(self.ALERT_FAIL_TO_PARSE_CONFIG)
      sys.exit(0)

    self.log = self.CreateAtftLog()

    if not self.log.log_dir_file:
      self._SendAlertEvent(self.ALERT_FAIL_TO_CREATE_LOG)

    self.StartRefreshingDevices()
    self.ChooseProduct(None)

  def CreateAtftManager(self):
    """Create an AtftManager object.

    This function exists for test mocking.
    """
    return AtftManager(FastbootDevice, SerialMapper, self.configs)

  def CreateAtftLog(self):
    """Create an AtftLog object.

    This function exists for test mocking.
    """
    return AtftLog(self.LOG_DIR, self.LOG_SIZE, self.LOG_FILE_NUMBER)

  def ParseConfigFile(self):
    """Parse the configuration file and read in the necessary configurations.

    Returns:
      The parsed configuration map.
    """
    # Give default values
    self.ATFT_VERSION = 'v0.0'
    self.COMPATIBLE_ATFA_VERSION = 'v0'
    self.DEVICE_REFRESH_INTERVAL = 1.0
    self.DEFAULT_KEY_THRESHOLD = 0
    self.LOG_DIR = None
    self.LOG_SIZE = 0
    self.LOG_FILE_NUMBER = 0
    self.LANGUAGE = 'eng'
    self.REBOOT_TIMEOUT = 0
    self.PRODUCT_ATTRIBUTE_FILE_EXTENSION = '*.atpa'

    config_file_path = os.path.join(self._GetCurrentPath(), self.CONFIG_FILE)
    if not os.path.exists(config_file_path):
      return None

    with open(config_file_path, 'r') as config_file:
      configs = json.loads(config_file.read())

    if not configs:
      return None

    try:
      self.ATFT_VERSION = str(configs['ATFT_VERSION'])
      self.COMPATIBLE_ATFA_VERSION = str(configs['COMPATIBLE_ATFA_VERSION'])
      self.DEVICE_REFRESH_INTERVAL = float(configs['DEVICE_REFRESH_INTERVAL'])
      self.DEFAULT_KEY_THRESHOLD = int(configs['DEFAULT_KEY_THRESHOLD'])
      self.LOG_DIR = str(configs['LOG_DIR'])
      self.LOG_SIZE = int(configs['LOG_SIZE'])
      self.LOG_FILE_NUMBER = int(configs['LOG_FILE_NUMBER'])
      self.LANGUAGE = str(configs['LANGUAGE'])
      self.REBOOT_TIMEOUT = float(configs['REBOOT_TIMEOUT'])
      self.PRODUCT_ATTRIBUTE_FILE_EXTENSION = str(
          configs['PRODUCT_ATTRIBUTE_FILE_EXTENSION'])
    except (KeyError, ValueError):
      return None

    return configs

  def _StoreConfigToFile(self):
    """Store the configuration to the configuration file.

    By storing the configuration back, the program would remember the
    configuration if it's opened again.
    """
    config_file_path = os.path.join(self._GetCurrentPath(), self.CONFIG_FILE)
    with open(config_file_path, 'w') as config_file:
      config_file.write(json.dumps(self.configs, sort_keys=True, indent=4))

  def _GetCurrentPath(self):
    """Get the current directory.

    Returns:
      The current directory.
    """
    if getattr(sys, 'frozen', False):
      # we are running in a bundle
      path = sys._MEIPASS  # pylint: disable=protected-access
    else:
      # we are running in a normal Python environment
      path = os.path.dirname(os.path.abspath(__file__))
    return path

  def GetLanguageIndex(self):
    """Translate language setting to an index.

    Returns:
      index: A index representing the language.
    """
    index = 0
    if self.LANGUAGE == 'eng':
      index = 0
    if self.LANGUAGE == 'cn':
      index = 1
    return index

  def SetLanguage(self):
    """Set the string constants according to the language setting.
    """
    index = self.GetLanguageIndex()

    self.SORT_BY_LOCATION_TEXT = ['Sort by location', '按照位置排序'][index]
    self.SORT_BY_SERIAL_TEXT = ['Sort by serial', '按照序列号排序'][index]

    # Top level menus
    self.MENU_APPLICATION = ['Application', '应用'][index]
    self.MENU_KEY_PROVISIONING = ['Key Provisioning', '密钥传输'][index]
    self.MENU_ATFA_DEVICE = ['ATFA Device', 'ATFA 管理'][index]
    self.MENU_AUDIT = ['Audit', '审计'][index]
    self.MENU_KEY_MANAGEMENT = ['Key Management', '密钥管理'][index]

    # Second level menus
    self.MENU_CLEAR_COMMAND = ['Clear Command Output', '清空控制台'][index]
    self.MENU_SHOW_STATUS_BAR = ['Show Statusbar', '显示状态栏'][index]
    self.MENU_SHOW_TOOL_BAR = ['Show Toolbar', '显示工具栏'][index]
    self.MENU_CHOOSE_PRODUCT = ['Choose Product', '选择产品'][index]
    self.MENU_QUIT = ['quit', '退出'][index]

    self.MENU_MANUAL_FUSE_VBOOT = ['Fuse Bootloader Vboot Key',
                                   '烧录引导密钥'][index]
    self.MENU_MANUAL_FUSE_ATTR = ['Fuse Permanent Attributes',
                                  '烧录产品信息'][index]
    self.MENU_MANUAL_LOCK_AVB = ['Lock Android Verified Boot', '锁定AVB'][index]
    self.MENU_MANUAL_PROV = ['Provision Key', '传输密钥'][index]

    self.MENU_STORAGE = ['Storage Mode', 'U盘模式'][index]

    self.MENU_ATFA_STATUS = ['ATFA Status', '查询余量'][index]
    self.MENU_KEY_THRESHOLD = ['Key Warning Threshold', '密钥警告阈值'][index]
    self.MENU_REBOOT = ['Reboot', '重启'][index]
    self.MENU_SHUTDOWN = ['Shutdown', '关闭'][index]

    self.MENU_STOREKEY = ['Store Key Bundle', '存储密钥打包文件'][index]
    self.MENU_PROCESSKEY = ['Process Key Bundle', '处理密钥打包文件'][index]

    # Toolbar icon names
    self.TOOLBAR_AUTO_PROVISION = ['Automatic Provision', '自动模式'][index]
    self.TOOLBAR_ATFA_STATUS = self.MENU_ATFA_STATUS
    self.TOOLBAR_CLEAR_COMMAND = self.MENU_CLEAR_COMMAND

    # Title
    self.TITLE = ['Google Android Things Factory Tool',
                  'Google Android Things 工厂程序'][index]

    # Area titles
    self.TITLE_ATFA_DEV = ['Atfa Device', 'ATFA 设备'][index]
    self.TITLE_PRODUCT_NAME = ['Product:', '产品：'][index]
    self.TITLE_PRODUCT_NAME_NOTCHOSEN = ['Not Chosen', '未选择'][index]
    self.TITLE_KEYS_LEFT = ['Attestation Keys Left:', '剩余密钥:'][index]
    self.TITLE_TARGET_DEV = ['Target Devices', '目标设备'][index]
    self.TITLE_COMMAND_OUTPUT = ['Command Output', '控制台输出'][index]

    # Field names
    self.FIELD_SERIAL_NUMBER = ['Serial Number', '序列号'][index]
    self.FIELD_USB_LOCATION = ['USB Location', '插入位置'][index]
    self.FIELD_STATUS = ['Status', '状态'][index]
    self.FIELD_SERIAL_WIDTH = 200
    self.FIELD_USB_WIDTH = 350
    self.FIELD_STATUS_WIDTH = 240

    # Dialogs
    self.DIALOG_CHANGE_THRESHOLD_TEXT = ['ATFA Key Warning Threshold:',
                                         '密钥警告阈值:'][index]
    self.DIALOG_CHANGE_THRESHOLD_TITLE = ['Change ATFA Key Warning Threshold',
                                          '更改密钥警告阈值'][index]
    self.DIALOG_LOW_KEY_TEXT = ''
    self.DIALOG_LOW_KEY_TITLE = ['Low Key Alert', '密钥不足警告'][index]
    self.DIALOG_ALERT_TEXT = ''
    self.DIALOG_ALERT_TITLE = ['Alert', '警告'][index]
    self.DIALOG_CHOOSE_PRODUCT_ATTRIBUTE_FILE = [
        'Choose Product Attributes File', '选择产品文件'][index]

    # Buttons
    self.BUTTON_TARGET_DEV_TOGGLE_SORT = ['target_device_sort_button',
                                     '目标设备排序按钮'][index]

    # Alerts
    self.ALERT_AUTO_PROV_NO_ATFA = [
        'Cannot enter auto provision mode\nNo ATFA device available!',
        '无法开启自动模式\n没有可用的ATFA设备！'][index]
    self.ALERT_AUTO_PROV_NO_PRODUCT = [
        'Cannot enter auto provision mode\nNo product specified!',
        '无法开启自动模式\n没有选择产品！'][index]
    self.ALERT_PROV_NO_SELECTED = [
        "Can't Provision! No target device selected!",
        '无法传输密钥！目标设备没有选择！'][index]
    self.ALERT_PROV_NO_ATFA = [
        "Can't Provision! No Available ATFA device!",
        '无法传输密钥！没有ATFA设备!'][index]
    self.ALERT_PROV_NO_KEYS = [
        "Can't Provision! No keys left!",
        '无法传输密钥！没有剩余密钥!'][index]
    self.ALERT_FUSE_NO_SELECTED = [
        "Can't Fuse vboot key! No target device selected!",
        '无法烧录！目标设备没有选择！'][index]
    self.ALERT_FUSE_NO_PRODUCT = [
        "Can't Fuse vboot key! No product specified!",
        '无法烧录！没有选择产品！'][index]
    self.ALERT_FUSE_PERM_NO_SELECTED = [
        "Can't Fuse permanent attributes! No target device selected!",
        '无法烧录产品信息！目标设备没有选择！'][index]
    self.ALERT_FUSE_PERM_NO_PRODUCT = [
        "Can't Fuse permanent attributes! No product specified!",
        '无法烧录产品信息！没有选择产品！'][index]
    self.ALERT_LOCKAVB_NO_SELECTED = [
        "Can't Lock Android Verified Boot! No target device selected!",
        '无法锁定AVB！目标设备没有选择！'][index]
    self.ALERT_FAIL_TO_CREATE_LOG = [
        'Failed to create log!',
        '无法创建日志文件！'][index]
    self.ALERT_FAIL_TO_PARSE_CONFIG = [
        'Failed to find or parse config file!',
        '无法找到或解析配置文件！'][index]
    self.ALERT_NO_DEVICE = [
        'No devices found!',
        '无设备！'][index]
    self.ALERT_CANNOT_OPEN_FILE = [
        'Can not open file: ',
        '无法打开文件: '][index]
    self.ALERT_PRODUCT_FILE_FORMAT_WRONG = [
        'The format for the product attributes file is not correct!',
        '产品文件格式不正确！'][index]
    self.ALERT_ATFA_UNPLUG = [
        'ATFA device unplugged, exit auto mode!',
        'ATFA设备拔出，退出自动模式！'][index]
    self.ALERT_NO_KEYS_LEFT_LEAVE_PROV = [
        'No keys left! Leave auto provisioning mode!',
        '没有剩余密钥，退出自动模式！'][index]
    self.ALERT_FUSE_VBOOT_FUSED = [
        'Cannot fuse bootloader vboot key for device that is already fused!',
        '无法烧录一个已经烧录过引导密钥的设备！'][index]
    self.ALERT_FUSE_PERM_ATTR_FUSED = [
        'Cannot fuse permanent attributes for device that is not fused '
        'bootloader vboot key or already fused permanent attributes!',
        '无法烧录一个没有烧录过引导密钥或者已经烧录过产品信息的设备！'][index]
    self.ALERT_LOCKAVB_LOCKED = [
        'Cannot lock android verified boot for device that is not fused '
        'permanent attributes or already locked!',
        '无法锁定一个没有烧录过产品信息或者已经锁定AVB的设备！'][index]
    self.ALERT_PROV_PROVED = [
        'Cannot provision device that is not ready for provisioning or '
        'already provisioned!',
        '无法传输密钥给一个不在正确状态或者已经拥有密钥的设备！'][index]



  def InitializeUI(self):
    """Initialize the application UI."""
    # The frame style is default style without border.
    style = long(wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)
    wx.Frame.__init__(self, None, style=style)

    # Menu:
    # Application   -> Clear Command Output
    #               -> Show Statusbar
    #               -> Show Toolbar
    #               -> Choose Product
    #               -> Quit

    # Key Provision -> Fuse Bootloader Vboot Key
    #               -> Fuse Permanent Attributes
    #               -> Lock Android Verified Boot
    #               -> Provision Key

    # ATFA Device   -> ATFA Status
    #               -> Key Warning Threshold
    #               -> Reboot
    #               -> Shutdown

    # Audit         -> Storage Mode
    #               -> ???

    # Key Management-> Store Key Bundle
    #               -> Process Key Bundle

    # Add Menu items to Menubar
    self.menubar = wx.MenuBar()
    self.app_menu = wx.Menu()
    self.menubar.Append(self.app_menu, self.MENU_APPLICATION)
    self.provision_menu = wx.Menu()
    self.menubar.Append(self.provision_menu, self.MENU_KEY_PROVISIONING)
    self.atfa_menu = wx.Menu()
    self.menubar.Append(self.atfa_menu, self.MENU_ATFA_DEVICE)
    self.audit_menu = wx.Menu()
    self.menubar.Append(self.audit_menu, self.MENU_AUDIT)
    self.key_menu = wx.Menu()
    self.menubar.Append(self.key_menu, self.MENU_KEY_MANAGEMENT)

    # App Menu Options
    menu_clear_command = self.app_menu.Append(
        wx.ID_ANY, self.MENU_CLEAR_COMMAND)

    self.Bind(wx.EVT_MENU, self.OnClearCommandWindow, menu_clear_command)

    self.menu_show_status_bar = self.app_menu.Append(
        wx.ID_ANY, self.MENU_SHOW_STATUS_BAR, kind=wx.ITEM_CHECK)
    self.app_menu.Check(self.menu_show_status_bar.GetId(), True)
    self.Bind(wx.EVT_MENU, self.ToggleStatusBar, self.menu_show_status_bar)

    self.menu_show_tool_bar = self.app_menu.Append(
        wx.ID_ANY, self.MENU_SHOW_TOOL_BAR, kind=wx.ITEM_CHECK)
    self.app_menu.Check(self.menu_show_tool_bar.GetId(), True)
    self.Bind(wx.EVT_MENU, self.ToggleToolBar, self.menu_show_tool_bar)

    self.menu_choose_product = self.app_menu.Append(
        wx.ID_ANY, self.MENU_CHOOSE_PRODUCT)
    self.Bind(wx.EVT_MENU, self.ChooseProduct, self.menu_choose_product)

    menu_quit = self.app_menu.Append(wx.ID_EXIT, self.MENU_QUIT)
    self.Bind(wx.EVT_MENU, self.OnQuit, menu_quit)

    # Key Provision Menu Options
    menu_manual_fuse_vboot = self.provision_menu.Append(
        wx.ID_ANY, self.MENU_MANUAL_FUSE_VBOOT)
    self.Bind(wx.EVT_MENU, self.OnFuseVbootKey, menu_manual_fuse_vboot)
    menu_manual_fuse_attr = self.provision_menu.Append(
        wx.ID_ANY, self.MENU_MANUAL_FUSE_ATTR)
    self.Bind(wx.EVT_MENU, self.OnFusePermAttr, menu_manual_fuse_attr)
    menu_manual_lock_avb = self.provision_menu.Append(
        wx.ID_ANY, self.MENU_MANUAL_LOCK_AVB)
    self.Bind(wx.EVT_MENU, self.OnLockAvb, menu_manual_lock_avb)
    menu_manual_prov = self.provision_menu.Append(
        wx.ID_ANY, self.MENU_MANUAL_PROV)
    self.Bind(wx.EVT_MENU, self.OnManualProvision, menu_manual_prov)

    # Audit Menu Options
    # TODO(shanyu): audit-related
    menu_storage = self.audit_menu.Append(wx.ID_ANY, self.MENU_STORAGE)
    self.Bind(wx.EVT_MENU, self.OnStorageMode, menu_storage)

    # ATFA Menu Options
    menu_atfa_status = self.atfa_menu.Append(wx.ID_ANY, self.MENU_ATFA_STATUS)
    self.Bind(wx.EVT_MENU, self.OnCheckATFAStatus, menu_atfa_status)

    menu_key_threshold = self.atfa_menu.Append(
        wx.ID_ANY, self.MENU_KEY_THRESHOLD)
    self.Bind(wx.EVT_MENU, self.OnChangeKeyThreshold, menu_key_threshold)

    menu_reboot = self.atfa_menu.Append(wx.ID_ANY, self.MENU_REBOOT)
    self.Bind(wx.EVT_MENU, self.OnReboot, menu_reboot)

    menu_shutdown = self.atfa_menu.Append(wx.ID_ANY, self.MENU_SHUTDOWN)
    self.Bind(wx.EVT_MENU, self.OnShutdown, menu_shutdown)

    # Key Management Menu Options
    menu_storekey = self.key_menu.Append(wx.ID_ANY, self.MENU_STOREKEY)
    self.Bind(wx.EVT_MENU, self.OnStoreKey, menu_storekey)
    menu_processkey = self.key_menu.Append(wx.ID_ANY, self.MENU_PROCESSKEY)
    self.Bind(wx.EVT_MENU, self.OnProcessKey, menu_processkey)

    self.SetMenuBar(self.menubar)

    # Toolbar buttons
    # -> 'Automatic Provision'
    # -> 'Refresh Devices'
    # -> 'Manual Provision'
    # -> 'ATFA Status'
    # -> 'Clear Command Output'
    self.toolbar = self.CreateToolBar()
    self.tools = []
    toolbar_auto_provision = self.toolbar.AddCheckTool(
        self.ID_TOOL_PROVISION, self.TOOLBAR_AUTO_PROVISION,
        wx.Bitmap('rocket.png'))
    self.Bind(wx.EVT_TOOL, self.OnToggleAutoProv, toolbar_auto_provision)
    self.toolbar_auto_provision = toolbar_auto_provision

    toolbar_atfa_status = self.toolbar.AddTool(
        wx.ID_ANY, self.TOOLBAR_ATFA_STATUS, wx.Bitmap('pie-chart.png'))
    self.Bind(wx.EVT_TOOL, self.OnCheckATFAStatus, toolbar_atfa_status)

    toolbar_clear_command = self.toolbar.AddTool(
        self.ID_TOOL_CLEAR, self.TOOLBAR_CLEAR_COMMAND, wx.Bitmap('eraser.png'))
    self.Bind(wx.EVT_TOOL, self.OnClearCommandWindow, toolbar_clear_command)

    self.tools.append(toolbar_auto_provision)
    self.tools.append(toolbar_atfa_status)
    self.tools.append(toolbar_clear_command)
    self.tools.append(toolbar_atfa_status)
    self.tools.append(toolbar_clear_command)

    # The main panel
    self.panel = wx.Panel(self)
    self.vbox = wx.BoxSizer(wx.VERTICAL)
    self.st = wx.StaticLine(self.panel, wx.ID_ANY, style=wx.LI_HORIZONTAL)
    self.vbox.Add(self.st, 0, wx.ALL | wx.EXPAND, 5)

    # Product Name Display
    self.product_name_title = wx.StaticText(
        self.panel, wx.ID_ANY, self.TITLE_PRODUCT_NAME)
    self.product_name_display = wx.StaticText(
        self.panel, wx.ID_ANY, self.TITLE_PRODUCT_NAME_NOTCHOSEN)
    self.bold_font = wx.Font(10, wx.DEFAULT, wx.NORMAL, wx.FONTWEIGHT_BOLD)
    self.product_name_display.SetFont(self.bold_font)
    self.product_name_sizer = wx.BoxSizer(wx.HORIZONTAL)
    self.product_name_sizer.Add(self.product_name_title)
    self.product_name_sizer.Add(self.product_name_display, 0, wx.LEFT, 2)
    self.vbox.Add(self.product_name_sizer, 0, wx.ALL, 5)

    # Keys Left Display
    self.keys_left_title = wx.StaticText(
        self.panel, wx.ID_ANY, self.TITLE_KEYS_LEFT)
    self.keys_left_display = wx.StaticText(
        self.panel, wx.ID_ANY, '')
    self.keys_left_display.SetFont(self.bold_font)
    self.keys_left_sizer = wx.BoxSizer(wx.HORIZONTAL)
    self.keys_left_sizer.Add(self.keys_left_title)
    self.keys_left_sizer.Add(self.keys_left_display, 0, wx.LEFT, 2)
    self.vbox.Add(self.keys_left_sizer, 0, wx.ALL, 5)

    # Device Output Title
    self.atfa_dev_title = wx.StaticText(
        self.panel, wx.ID_ANY, self.TITLE_ATFA_DEV)
    self.atfa_dev_title_sizer = wx.BoxSizer(wx.HORIZONTAL)
    self.atfa_dev_title_sizer.Add(self.atfa_dev_title, 0, wx.ALL, 5)
    self.vbox.Add(self.atfa_dev_title_sizer, 0, wx.LEFT)

    # Device Output Window
    self.atfa_devs_output = wx.TextCtrl(
        self.panel,
        wx.ID_ANY,
        size=(800, 20),
        style=wx.TE_MULTILINE | wx.TE_READONLY)
    self.vbox.Add(self.atfa_devs_output, 0, wx.ALL | wx.EXPAND, 5)

    # Device Output Title
    self.target_dev_title = wx.StaticText(self.panel, wx.ID_ANY,
                                          self.TITLE_TARGET_DEV)
    self.target_dev_title_sizer = wx.BoxSizer(wx.HORIZONTAL)
    self.target_dev_title_sizer.Add(self.target_dev_title, 0, wx.ALL, 5)

    # Device Output Sort Button
    self.target_dev_toggle_sort = wx.Button(
        self.panel,
        wx.ID_ANY,
        self.SORT_BY_SERIAL_TEXT,
        style=wx.BU_LEFT,
        name=self.BUTTON_TARGET_DEV_TOGGLE_SORT,
        size=wx.Size(110, 30))
    self.target_dev_title_sizer.Add(self.target_dev_toggle_sort, 0, wx.LEFT, 10)
    self.Bind(wx.EVT_BUTTON, self.OnToggleTargetSort,
              self.target_dev_toggle_sort)

    self.vbox.Add(self.target_dev_title_sizer, 0, wx.LEFT)

    # Device Output Window
    self.target_devs_output = wx.ListCtrl(
        self.panel, wx.ID_ANY, size=(800, 200), style=wx.LC_REPORT)
    self.target_devs_output.InsertColumn(
        0, self.FIELD_SERIAL_NUMBER, width=self.FIELD_SERIAL_WIDTH)
    self.target_devs_output.InsertColumn(
        1, self.FIELD_USB_LOCATION, width=self.FIELD_USB_WIDTH)
    self.target_devs_output.InsertColumn(
        2, self.FIELD_STATUS, width=self.FIELD_STATUS_WIDTH)
    self.vbox.Add(self.target_devs_output, 0, wx.ALL | wx.EXPAND, 5)

    # Command Output Title
    self.command_title = wx.StaticText(
        self.panel, wx.ID_ANY, self.TITLE_COMMAND_OUTPUT)
    self.command_title_sizer = wx.BoxSizer(wx.HORIZONTAL)
    self.command_title_sizer.Add(self.command_title, 0, wx.ALL, 5)
    self.vbox.Add(self.command_title_sizer, 0, wx.LEFT)

    # Command Output Window
    self.cmd_output = wx.TextCtrl(
        self.panel,
        wx.ID_ANY,
        size=(800, 190),
        style=wx.TE_MULTILINE | wx.TE_READONLY | wx.HSCROLL)
    self.vbox.Add(self.cmd_output, 0, wx.ALL | wx.EXPAND, 5)

    self.panel.SetSizer(self.vbox)
    self.toolbar.Realize()
    self.statusbar = self.CreateStatusBar()
    self.statusbar.SetStatusText('Ready')
    self.SetSize((800, 720))
    self.SetTitle(self.TITLE)
    self.Center()
    self.Show(True)

    # Change Key Threshold Dialog
    self.change_threshold_dialog = wx.TextEntryDialog(
        self,
        self.DIALOG_CHANGE_THRESHOLD_TEXT,
        self.DIALOG_CHANGE_THRESHOLD_TITLE,
        style=wx.TextEntryDialogStyle | wx.CENTRE)

    # Low Key Alert Dialog
    self.low_key_dialog = wx.MessageDialog(
        self,
        self.DIALOG_LOW_KEY_TEXT,
        self.DIALOG_LOW_KEY_TITLE,
        style=wx.OK | wx.ICON_EXCLAMATION | wx.CENTRE)

    # General Alert Dialog
    self.alert_dialog = wx.MessageDialog(
        self,
        self.DIALOG_ALERT_TEXT,
        self.DIALOG_ALERT_TITLE,
        style=wx.OK | wx.ICON_EXCLAMATION | wx.CENTRE)

    self._CreateBindEvents()

  def PauseRefresh(self):
    """Pause the refresh for device list during fastboot operations.
    """
    self.refresh_pause_lock.release()

  def ResumeRefresh(self):
    """Resume the refresh for device list.
    """
    self.refresh_pause_lock.acquire()

  def PrintToWindow(self, text_entry, text, append=False):
    """Print some message to a text_entry window.

    Args:
      text_entry: The window to print to.
      text: The text to be printed.
      append: Whether to replace or append the message.
    """
    # Append the message.
    if append:
      text_entry.AppendText(text)
      return

    # Replace existing message. Need to clean first. The GetValue() returns
    # unicode string, need to encode that to utf-8 to compare.
    current_text = text_entry.GetValue().encode('utf-8')
    if text == current_text:
      # If nothing changes, don't refresh.
      return
    text_entry.Clear()
    text_entry.AppendText(text)

  def PrintToCommandWindow(self, text):
    """Print some message to the command window.

    Args:
      text: The text to be printed.
    """
    msg = '[' + datetime.now().strftime('%Y-%m-%d %H:%M:%S') + '] '
    msg += text + '\n'
    self.PrintToWindow(self.cmd_output, msg, True)

  def StartRefreshingDevices(self):
    """Refreshing the device list by interval of DEVICE_REFRESH_INTERVAL.
    """
    # If there's already a timer running, stop it first.
    self.StopRefresh()
    # Start a new timer.
    self.refresh_timer = threading.Timer(self.DEVICE_REFRESH_INTERVAL,
                                         self.StartRefreshingDevices)
    self.refresh_timer.start()

    if self.refresh_pause_lock.acquire(False):
      self.refresh_pause_lock.release()
      self._SendDeviceListedEvent()
    else:
      # If refresh is not paused, refresh the devices.
      self._ListDevices()

  def StopRefresh(self):
    """Stop the refresh timer if there's any.
    """
    if self.refresh_timer:
      timer = self.refresh_timer
      self.refresh_timer = None
      timer.cancel()

  def OnClearCommandWindow(self, event=None):
    """Clear the command window.

    Args:
      event: The triggering event.
    """
    self.cmd_output.Clear()

  def OnListDevices(self, event=None):
    """List devices asynchronously.

    Args:
      event: The triggering event.
    """
    if event is not None:
      event.Skip()

    self._CreateThread(self._ListDevices)

  def OnStorageMode(self, event):
    """Switch ATFA device to storage mode asynchronously.

    Args:
      event: The triggering event.
    """
    self._CreateThread(self._SwitchStorageMode)

  def OnReboot(self, event):
    """Reboot ATFA device asynchronously.

    Args:
      event: The triggering event.
    """
    self._CreateThread(self._Reboot)

  def OnShutdown(self, event):
    """Shutdown ATFA device asynchronously.

    Args:
      event: The triggering event.
    """
    self._CreateThread(self._Shutdown)

  def OnToggleTargetSort(self, event):
    """Switch the target device list sorting field.

    Args:
      event: The triggering event.
    """
    if self.sort_by == self.atft_manager.SORT_BY_LOCATION:
      self.sort_by = self.atft_manager.SORT_BY_SERIAL
      self.target_dev_toggle_sort.SetLabel(self.SORT_BY_LOCATION_TEXT)
    else:
      self.sort_by = self.atft_manager.SORT_BY_LOCATION
      self.target_dev_toggle_sort.SetLabel(self.SORT_BY_SERIAL_TEXT)
    self._ListDevices()

  def OnToggleAutoProv(self, event):
    """Enter/Leave auto provisioning mode.

    Args:
      event: The triggering event.
    """
    self.auto_prov = self.toolbar_auto_provision.IsToggled()
    # If no available ATFA device.
    if self.auto_prov and not self.atft_manager.atfa_dev:
      self.auto_prov = False
      self.toolbar.ToggleTool(self.ID_TOOL_PROVISION, False)
      self._SendAlertEvent(self.ALERT_AUTO_PROV_NO_ATFA)
      return

    # If no product specified.
    if self.auto_prov and not self.atft_manager.product_info:
      self.auto_prov = False
      self.toolbar.ToggleTool(self.ID_TOOL_PROVISION, False)
      self._SendAlertEvent(self.ALERT_AUTO_PROV_NO_PRODUCT)
      return

    self._ToggleToolbarMenu(self.auto_prov)

    if self.auto_prov:
      # Enter auto provisioning mode.

      # Reset the low key alert shown indicator
      self.low_key_alert_shown = False
      message = 'Automatic key provisioning start'
      self.PrintToCommandWindow(message)
      self.log.Info('Autoprov', message)
    else:
      # Leave auto provisioning mode.
      for device in self.atft_manager.target_devs:
        # Change all waiting devices' status to it's original state.
        if device.provision_status == ProvisionStatus.WAITING:
          self.atft_manager.CheckProvisionStatus(device)
      message = 'Automatic key provisioning end'
      self.PrintToCommandWindow(message)
      self.log.Info('Autoprov', message)

  def _ToggleToolbarMenu(self, auto_prov):
    """Disable/Enable buttons and menu items while entering/leaving auto mode.

    Args:
      auto_prov: Whether entering auto provisioning mode(True) or
        leaving(False).
    """
    for tool in self.tools:
      tool_id = tool.GetId()
      if tool_id != self.ID_TOOL_PROVISION and tool_id != self.ID_TOOL_CLEAR:
        if self.auto_prov:
          self.toolbar.EnableTool(tool_id, False)
        else:
          self.toolbar.EnableTool(tool_id, True)

    # Disable menu items.
    for i in range(0, len(self.menubar.GetMenus())):
      if self.auto_prov:
        self.menubar.EnableTop(i, False)
      else:
        self.menubar.EnableTop(i, True)

  def OnManualProvision(self, event):
    """Manual provision key asynchronously.

    Args:
      event: The triggering event.
    """
    selected_serials = self._GetSelectedSerials()
    if not selected_serials:
      self._SendAlertEvent(self.ALERT_PROV_NO_SELECTED)
      return
    if not self.atft_manager.atfa_dev:
      self._SendAlertEvent(self.ALERT_PROV_NO_ATFA)
      return
    if self.atft_manager.GetATFAKeysLeft() == 0:
      self._SendAlertEvent(self.ALERT_PROV_NO_KEYS)
      return
    self._CreateThread(self._ManualProvision, selected_serials)

  def OnCheckATFAStatus(self, event):
    """Check the attestation key status from ATFA device asynchronously.

    Args:
      event: The triggering event.
    """
    self._CreateThread(self._ShowATFAStatus)

  def OnFuseVbootKey(self, event):
    """Fuse the vboot key to the target device asynchronously.

    Args:
      event: The triggering event.
    """
    selected_serials = self._GetSelectedSerials()
    if not selected_serials:
      self._SendAlertEvent(self.ALERT_FUSE_NO_SELECTED)
      return
    if not self.atft_manager.product_info:
      self._SendAlertEvent(self.ALERT_FUSE_NO_PRODUCT)
      return

    self._CreateThread(self._FuseVbootKey, selected_serials)

  def OnFusePermAttr(self, event):
    """Fuse the permanent attributes to the target device asynchronously.

    Args:
      event: The triggering event.
    """
    selected_serials = self._GetSelectedSerials()
    if not selected_serials:
      self._SendAlertEvent(self.ALERT_FUSE_PERM_NO_SELECTED)
      return
    if not self.atft_manager.product_info:
      self._SendAlertEvent(self.ALERT_FUSE_PERM_NO_PRODUCT)
      return

    self._CreateThread(self._FusePermAttr, selected_serials)

  def OnLockAvb(self, event):
    """Lock the AVB asynchronously.

    Args:
      event: The triggering event
    """
    selected_serials = self._GetSelectedSerials()
    if not selected_serials:
      self._SendAlertEvent(self.ALERT_LOCKAVB_NO_SELECTED)
      return

    self._CreateThread(self._LockAvb, selected_serials)

  def OnQuit(self, event):
    """Quit the application.

    Args:
      event: The triggering event.
    """
    self.Close()

  def ToggleStatusBar(self, event):
    """Toggle the status bar.

    Args:
      event: The triggering event.
    """
    if self.menu_show_status_bar.IsChecked():
      self.statusbar.Show()
    else:
      self.statusbar.Hide()

  def ToggleToolBar(self, event):
    """Toggle the tool bar.

    Args:
      event: The triggering event.
    """
    if self.menu_show_tool_bar.IsChecked():
      self.toolbar.Show()
    else:
      self.toolbar.Hide()

  class SelectFileArg(object):
    """The argument structure for SelectFileHandler.

    Attributes:
      message: The message for the select file window.
      wildcard: The wildcard to filter the files to be selected.
      callback: The callback to be called once the file is selected with
        argument pathname of the selected file.

    """

    def __init__(self, message, wildcard, callback):
      self.message = message
      self.wildcard = wildcard
      self.callback = callback

  def ChooseProduct(self, event):
    """Ask user to choose the product attributes file.

    Args:
      event: The triggering event.
    """
    message = self.DIALOG_CHOOSE_PRODUCT_ATTRIBUTE_FILE
    wildcard = self.PRODUCT_ATTRIBUTE_FILE_EXTENSION
    callback = self.ProcessProductAttributesFile
    data = self.SelectFileArg(message, wildcard, callback)
    event = Event(self.select_file_event, value=data)
    wx.QueueEvent(self, event)

  def ProcessProductAttributesFile(self, pathname):
    """Process the selected product attributes file.

    Args:
      pathname: The path for the product attributes file to parse.
    """
    try:
      with open(pathname, 'r') as attribute_file:
        content = attribute_file.read()
        self.atft_manager.ProcessProductAttributesFile(content)
        # Update the product name display
        self.product_name_display.SetLabelText(
            self.atft_manager.product_info.product_name)
        # User choose a new product, reset how many keys left.
        if self.atft_manager.atfa_dev and self.atft_manager.product_info:
          self._CheckATFAStatus()
    except IOError:
      self._SendAlertEvent(self.ALERT_CANNOT_OPEN_FILE + pathname)
    except ProductAttributesFileFormatError:
      self._SendAlertEvent(self.ALERT_PRODUCT_FILE_FORMAT_WRONG)

  def OnChangeKeyThreshold(self, event):
    """Change the threshold for low number of key warning.

    Args:
      event: The button click event.
    """
    self.change_threshold_dialog.SetValue(str(self.key_threshold))
    self.change_threshold_dialog.CenterOnParent()
    if self.change_threshold_dialog.ShowModal() == wx.ID_OK:
      value = self.change_threshold_dialog.GetValue()
      try:
        number = int(value)
        if number <= 0:
          # Invalid setting, just ignore.
          return
        self.key_threshold = number
        # Update the configuration.
        self.configs['DEFAULT_KEY_THRESHOLD'] = str(self.key_threshold)
      except ValueError:
        pass

  def OnStoreKey(self, event):
    """Store the keybundle to the ATFA device.

    Args:
      event: The button click event.
    """
    self.OnStorageMode(event)

  def OnProcessKey(self, event):
    """The async operation to ask ATFA device to process the stored keybundle.

    Args:
      event: The button click event.
    """
    self._CreateThread(self._ProcessKey)

  def ShowAlert(self, msg):
    """Show an alert box at the center of the parent window.

    Args:
      msg: The message to be shown in the alert box.
    """
    self.alert_dialog.CenterOnParent()
    self.alert_dialog.SetMessage(msg)
    self.alert_dialog.ShowModal()

  def OnClose(self, event):
    """This is the place for close callback, need to do cleanups.

    Args:
      event: The triggering event.
    """
    self._StoreConfigToFile()
    # Stop the refresh timer on close.
    self.StopRefresh()
    self.Destroy()

  def _HandleAutoProv(self):
    """Do the state transition for devices if in auto provisioning mode.

    """
    # All idle devices -> waiting.
    for target_dev in self.atft_manager.target_devs:
      if (target_dev.serial_number not in self.auto_dev_serials and
          target_dev.provision_status != ProvisionStatus.PROVISION_SUCCESS and
          not ProvisionStatus.isFailed(target_dev.provision_status)
          ):
        self.auto_dev_serials.append(target_dev.serial_number)
        target_dev.provision_status = ProvisionStatus.WAITING
        self._CreateThread(self._HandleStateTransition, target_dev)


  def _HandleKeysLeft(self):
    """Display how many keys left in the ATFA device.
    """
    if self.atft_manager.atfa_dev and self.atft_manager.product_info:
      keys_left = self.atft_manager.GetATFAKeysLeft()
      if not keys_left:
        # If keys_left is not set, try to set it.
        self._CheckATFAStatus()
        keys_left = self.atft_manager.GetATFAKeysLeft()
      if keys_left and keys_left >= 0:
        self.keys_left_display.SetLabelText(str(keys_left))
        return

    self.keys_left_display.SetLabelText('')

  def _CopyList(self, old_list):
    """Copy a device list.

    Args:
      old_list: The original list
    Returns:
      The duplicate with all the public member copied.
    """
    copy_list = []
    for dev in old_list:
      copy_list.append(dev.Copy())
    return copy_list

  def _HandleException(self, level, e, operation=None, target=None):
    """Handle the exception.

    Fires a exception event which would be handled in main thread. The exception
    would be shown in the command window. This function also wraps the
    associated operation and device object.

    Args:
      level: The log level for the exception.
      e: The original exception.
      operation: The operation associated with this exception.
      target: The DeviceInfo object associated with this exception.
    """
    atft_exception = AtftException(e, operation, target)
    wx.QueueEvent(self,
                  Event(
                      self.exception_event,
                      wx.ID_ANY,
                      value=str(atft_exception)))
    self._LogException(level, atft_exception)

  def _LogException(self, level, atft_exception):
    """Log the exceptions.

    Args:
      level: The log level for this exception. 'E': error or 'W': warning.
      atft_exception: The exception to be logged.
    """
    if level == 'E':
      self.log.Error('OpException', str(atft_exception))
    elif level == 'W':
      self.log.Warning('OpException', str(atft_exception))

  def _CreateBindEvents(self):
    """Create customized events and bind them to the event handlers.
    """

    # Event for refreshing device list.
    self.refresh_event = wx.NewEventType()
    self.refresh_event_bind = wx.PyEventBinder(self.refresh_event)

    # Event for device listed.
    self.dev_listed_event = wx.NewEventType()
    self.dev_listed_event_bind = wx.PyEventBinder(self.dev_listed_event)
    # Event when general exception happens.
    self.exception_event = wx.NewEventType()
    self.exception_event_bind = wx.PyEventBinder(self.exception_event)
    # Event for alert box.
    self.alert_event = wx.NewEventType()
    self.alert_event_bind = wx.PyEventBinder(self.alert_event)
    # Event for general message to be printed in command window.
    self.print_event = wx.NewEventType()
    self.print_event_bind = wx.PyEventBinder(self.print_event)
    # Event for low key alert.
    self.low_key_alert_event = wx.NewEventType()
    self.low_key_alert_event_bind = wx.PyEventBinder(self.low_key_alert_event)
    # Event for select a file.
    self.select_file_event = wx.NewEventType()
    self.select_file_event_bind = wx.PyEventBinder(self.select_file_event)

    self.Bind(self.refresh_event_bind, self.OnListDevices)
    self.Bind(self.dev_listed_event_bind, self._DeviceListedEventHandler)
    self.Bind(self.exception_event_bind, self._PrintEventHandler)
    self.Bind(self.alert_event_bind, self._AlertEventHandler)
    self.Bind(self.print_event_bind, self._PrintEventHandler)
    self.Bind(self.low_key_alert_event_bind, self._LowKeyAlertEventHandler)
    self.Bind(self.select_file_event_bind, self._SelectFileEventHandler)

    # Bind the close event
    self.Bind(wx.EVT_CLOSE, self.OnClose)

  def _SendAlertEvent(self, msg):
    """Send an event to generate an alert box.

    Args:
      msg: The message to be displayed in the alert box.
    """
    evt = Event(self.alert_event, wx.ID_ANY, msg)
    wx.QueueEvent(self, evt)

  def _PrintEventHandler(self, event):
    """The handler to handle the event to display a message in the cmd output.

    Args:
      event: The message to be displayed.
    """
    msg = str(event.GetValue())
    self.PrintToCommandWindow(msg)

  def _SendPrintEvent(self, msg):
    """Send an event to print a message to the cmd output.

    Args:
      msg: The message to be displayed.
    """
    evt = Event(self.print_event, wx.ID_ANY, msg)
    wx.QueueEvent(self, evt)

  def _SendOperationStartEvent(self, operation, target=None):
    """Send an event to print an operation start message.

    Args:
      operation: The operation name.
      target: The target of the operation.
    """
    msg = ''
    if target:
      msg += '{' + str(target) + '} '
    msg += operation + ' Start'
    self._SendPrintEvent(msg)
    self.log.Info('OpStart', msg)

  def _SendOperationSucceedEvent(self, operation, target=None):
    """Send an event to print an operation succeed message.

    Args:
      operation: The operation name.
      target: The target of the operation.
    """
    msg = ''
    if target:
      msg += '{' + str(target) + '} '
    msg += operation + ' Succeed'
    self._SendPrintEvent(msg)
    self.log.Info('OpSucceed', msg)

  def _SendDeviceListedEvent(self):
    """Send an event to indicate device list is refreshed, need to refresh UI.
    """
    wx.QueueEvent(self, Event(self.dev_listed_event))

  def _SendLowKeyAlertEvent(self):
    """Send low key alert event.

    Send an event to indicate the keys left in the ATFA device is lower than
    threshold.
    """
    wx.QueueEvent(self, Event(self.low_key_alert_event))

  def _AlertEventHandler(self, event):
    """The handler to handle the event to display an alert box.

    Args:
      event: The alert event containing the message to be displayed.
    """
    msg = event.GetValue()
    # Need to check if any other handler is using the alert box.
    # All the handler is in the main thread
    # So we cannot block to acquire this lock
    # The main reason of the async is the showModal is async
    # However, we cannot make sure SetMsg and ShowModel is atomic
    # So we can only ignore the overlapping request.
    if self.alert_lock.acquire(False):
      self.ShowAlert(msg)
      self.alert_lock.release()

  def _DeviceListedEventHandler(self, event):
    """Handles the device listed event and list the devices.

    Args:
      event: The event object.
    """
    self._HandleKeysLeft()
    if self.atft_manager.atfa_dev:
      atfa_message = str(self.atft_manager.atfa_dev)
    else:
      atfa_message = self.ALERT_NO_DEVICE

    if self.auto_prov and not self.atft_manager.atfa_dev:
      # If ATFA unplugged during auto mode,
      # exit the mode with an alert.
      self.toolbar.ToggleTool(self.ID_TOOL_PROVISION, False)
      self.OnToggleAutoProv(None)
      self._SendAlertEvent(self.ALERT_ATFA_UNPLUG)

    # If in auto provisioning mode, handle the newly added devices.
    if self.auto_prov:
      self._HandleAutoProv()

    self.PrintToWindow(self.atfa_devs_output, atfa_message)
    if self.last_target_list == self.atft_manager.target_devs:
      # Nothing changes, no need to refresh
      return

    # Update the stored target list. Need to make a deep copy instead of copying
    # the reference.
    self.last_target_list = self._CopyList(self.atft_manager.target_devs)
    self.target_devs_output.DeleteAllItems()
    for target_dev in self.atft_manager.target_devs:
      provision_status_string = ProvisionStatus.ToString(
          target_dev.provision_status, self.GetLanguageIndex())
      # This is a utf-8 string, need to transfer to unicode.
      provision_status_string = provision_status_string.decode('utf-8')
      self.target_devs_output.Append(
          (target_dev.serial_number, target_dev.location,
           provision_status_string))

  def _SelectFileEventHandler(self, event):
    """Show the select file window.

    Args:
      event: containing data of SelectFileArg type.
    """
    # TODO(shanyu): Need to know the extension of the file from console.
    data = event.GetValue()
    message = data.message
    wildcard = data.wildcard
    callback = data.callback
    with wx.FileDialog(
        self,
        message,
        wildcard=wildcard,
        style=wx.FD_OPEN | wx.FD_FILE_MUST_EXIST) as file_dialog:
      if file_dialog.ShowModal() == wx.ID_CANCEL:
        return  # the user changed their mind
      pathname = file_dialog.GetPath()
      callback(pathname)

  def _LowKeyAlertEventHandler(self, event):
    """Show the alert box to alert user that the key in ATFA device is low.

    Args:
      event: The triggering event.
    """
    if self.low_key_alert_shown:
      # If already shown once in this auto provisioning mode
      # Then do not show again.
      return
    self.low_key_alert_shown = True
    self.low_key_dialog.SetMessage(
        'The attestation keys available in this ATFA device is lower than ' +
        str(self.key_threshold) + ' for this product!')
    self.low_key_dialog.CenterOnParent()
    self.low_key_dialog.ShowModal()

  def _CreateThread(self, target, *args):
    """Create and start a thread.

    Args:
      target: The function that the thread should run.
      *args: The arguments for the function
    Returns:
      The thread object
    """
    t = threading.Thread(target=target, args=args)
    t.setDaemon(True)
    t.start()
    return t

  def _ListDevices(self):
    """List fastboot devices.
    """

    # We need to check the lock to prevent two _ListDevices running at the same
    # time.
    if self.listing_device_lock.acquire(False):
      operation = 'List Devices'
      try:
        self.atft_manager.ListDevices(self.sort_by)
      except FastbootFailure as e:
        self._HandleException('W', e, operation)
        return
      finally:
        # 'Release the lock'.
        self.listing_device_lock.release()

      wx.QueueEvent(self, Event(self.dev_listed_event, wx.ID_ANY))

  def _CheckATFAStatus(self):
    """Get the attestation key status of the ATFA device.

    Update the number of keys left for the selected product in the ATFA device.

    Returns:
      Whether the check succeed or not.
    """
    operation = 'Check ATFA status'
    self._SendOperationStartEvent(operation)
    self.PauseRefresh()

    try:
      self.atft_manager.CheckATFAStatus()
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation)
      return False
    except ProductNotSpecifiedException as e:
      self._HandleException('W', e, operation)
      return False
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
      return False
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation)
    return True

  def _ShowATFAStatus(self):
    """Show the attestation key status of the ATFA device.
    """
    if self._CheckATFAStatus():
      self._SendAlertEvent(
          'There are ' + str(self.atft_manager.GetATFAKeysLeft()) +
          ' keys left for this product in the ATFA device.')

  def _FuseVbootKey(self, selected_serials):
    """Fuse the verified boot key to the devices.

    Args:
      selected_serials: The list of serial numbers for the selected devices.
    """
    pending_targets = []
    for serial in selected_serials:
      target = self.atft_manager.GetTargetDevice(serial)
      if not target:
        continue
      # Start state could be IDLE or FUSEVBOOT_FAILED
      if (TEST_MODE or not target.provision_state.bootloader_locked):
        target.provision_status = ProvisionStatus.WAITING
        pending_targets.append(target)
      else:
        self._SendAlertEvent(self.ALERT_FUSE_VBOOT_FUSED)

    for target in pending_targets:
      self._FuseVbootKeyTarget(target)

  def _FuseVbootKeyTarget(self, target):
    """Fuse the verified boot key to a specific device.

    We would first fuse the bootloader vboot key
    and then reboot the device to check whether the bootloader is locked.
    This function would block until the reboot succeed or timeout.

    Args:
      target: The target device DeviceInfo object.
    """
    operation = 'Fuse bootloader verified boot key'
    serial = target.serial_number
    self._SendOperationStartEvent(operation, target)
    self.PauseRefresh()

    try:
      self.atft_manager.FuseVbootKey(target)
    except ProductNotSpecifiedException as e:
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation, target)

    operation = 'Verify bootloader locked, rebooting'
    self._SendOperationStartEvent(operation, target)
    success_msg = '{' + str(target) + '} ' + 'Reboot Succeed'
    timeout_msg = '{' + str(target) + '} ' + 'Reboot Failed! Timeout!'
    reboot_lock = threading.Lock()
    reboot_lock.acquire()

    def LambdaSuccessCallback(msg=success_msg, lock=reboot_lock):
      self._RebootSuccessCallback(msg, lock)

    def LambdaTimeoutCallback(msg=timeout_msg, lock=reboot_lock):
      self._RebootTimeoutCallback(msg, lock)

    # Reboot the device to verify the bootloader is locked.
    try:
      target.provision_status = ProvisionStatus.REBOOT_ING
      wx.QueueEvent(self, Event(self.dev_listed_event, wx.ID_ANY))

      # Reboot would change device status, so we disable reading device status
      # during reboot.
      self.listing_device_lock.acquire()
      self.atft_manager.Reboot(
          target, self.REBOOT_TIMEOUT, LambdaSuccessCallback,
          LambdaTimeoutCallback)
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
      return
    finally:
      self.listing_device_lock.release()

    # Wait until callback finishes. After the callback, reboot_lock would be
    # released.
    reboot_lock.acquire()

    target = self.atft_manager.GetTargetDevice(serial)
    if target and not target.provision_state.bootloader_locked:
      target.provision_status = ProvisionStatus.FUSEVBOOT_FAILED
      e = FastbootFailure('Status not updated.')
      self._HandleException('E', e, operation)
      return

  def _RebootSuccessCallback(self, msg, lock):
    """The callback if reboot succeed.

    Args:
      msg: The message to be shown
      lock: The lock to indicate the callback is called.
    """
    self._SendPrintEvent(msg)
    self.log.Info('OpSucceed', msg)
    lock.release()

  def _RebootTimeoutCallback(self, msg, lock):
    """The callback if reboot timeout.

    Args:
      msg: The message to be shown
      lock: The lock to indicate the callback is called.
    """
    self._SendPrintEvent(msg)
    self.log.Error('OpException', msg)
    lock.release()

  def _FusePermAttr(self, selected_serials):
    """Fuse the permanent attributes to the target devices.

    Args:
      selected_serials: The list of serial numbers for the selected devices.
    """
    pending_targets = []
    for serial in selected_serials:
      target = self.atft_manager.GetTargetDevice(serial)
      if not target:
        return
      # Start state could be FUSEVBOOT_SUCCESS or REBOOT_SUCCESS
      # or FUSEATTR_FAILED
      # Note: Reboot to check vboot is optional, user can skip that manually.
      if (TEST_MODE or (
            target.provision_state.bootloader_locked and
            not target.provision_state.avb_perm_attr_set
          )):
        pending_targets.append(target)
      else:
        self._SendAlertEvent(self.ALERT_FUSE_PERM_ATTR_FUSED)

    for target in pending_targets:
      self._FusePermAttrTarget(target)

  def _FusePermAttrTarget(self, target):
    """Fuse the permanent attributes to the specific target device.

    Args:
      target: The target device DeviceInfo object.
    """
    operation = 'Fuse permanent attributes'
    self._SendOperationStartEvent(operation, target)
    self.PauseRefresh()

    try:
      self.atft_manager.FusePermAttr(target)
    except ProductNotSpecifiedException as e:
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation, target)

  def _LockAvb(self, selected_serials):
    """Lock android verified boot for selected devices.

    Args:
      selected_serials: The list of serial numbers for the selected devices.
    """
    pending_targets = []
    for serial in selected_serials:
      target = self.atft_manager.GetTargetDevice(serial)
      if not target:
        continue
      # Start state could be FUSEATTR_SUCCESS or LOCKAVB_FAIELD
      if (TEST_MODE or(
            target.provision_state.bootloader_locked and
            target.provision_state.avb_perm_attr_set and
            not target.provision_state.avb_locked
          )):
        target.provision_status = ProvisionStatus.WAITING
        pending_targets.append(target)
      else:
        self._SendAlertEvent(self.ALERT_LOCKAVB_LOCKED)

    for target in pending_targets:
      self._LockAvbTarget(target)

  def _LockAvbTarget(self, target):
    """Lock android verified boot for the specific target device.

    Args:
      target: The target device DeviceInfo object.
    """
    operation = 'Lock android verified boot'
    self._SendOperationStartEvent(operation, target)
    self.PauseRefresh()

    try:
      self.atft_manager.LockAvb(target)
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation, target)

  def _CheckLowKeyAlert(self):
    """Check whether the attestation key is lower than the threshold.

    If so, an alert box would appear to warn the user.
    """
    operation = 'Check ATFA Status'
    threshold = self.key_threshold

    if self._CheckATFAStatus():
      keys_left = self.atft_manager.GetATFAKeysLeft()
      if keys_left and keys_left >= 0 and keys_left <= threshold:
        # If the confirmed number is lower than threshold, fire low key event.
        self._SendLowKeyAlertEvent()

  def _SwitchStorageMode(self):
    """Switch ATFA device to storage mode.
    """
    operation = 'Switch ATFA device to Storage Mode'
    self._SendOperationStartEvent(operation)
    self.PauseRefresh()

    try:
      self.atft_manager.SwitchATFAStorage()
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation, self.atft_manager.atfa_dev)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation)

  def _Reboot(self):
    """Reboot ATFA device.
    """
    operation = 'Reboot ATFA device'
    self._SendOperationStartEvent(operation)
    self.PauseRefresh()

    try:
      self.atft_manager.RebootATFA()
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation, self.atft_manager.atfa_dev)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation)

  def _Shutdown(self):
    """Shutdown ATFA device.
    """
    operation = 'Shutdown ATFA device'
    self._SendOperationStartEvent(operation)
    self.PauseRefresh()

    try:
      self.atft_manager.ShutdownATFA()
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation, self.atft_manager.atfa_dev)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation)

  def _ManualProvision(self, selected_serials):
    """Manual provision the selected devices.

    Args:
      selected_serials: A list of the serial numbers of the target devices.
    """
    # Reset low_key_alert_shown
    self.low_key_alert_shown = False
    pending_targets = []
    for serial in selected_serials:
      target_dev = self.atft_manager.GetTargetDevice(serial)
      if not target_dev:
        continue
      pending_targets.append(target_dev)
      status = target_dev.provision_status
      if (TEST_MODE or (
          target_dev.provision_state.bootloader_locked and
          target_dev.provision_state.avb_perm_attr_set and
          target_dev.provision_state.avb_locked and
          not target_dev.provision_state.provisioned
        )):
        target_dev.provision_status = ProvisionStatus.WAITING
      else:
        self._SendAlertEvent(self.ALERT_PROV_PROVED)
    for target in pending_targets:
      if target.provision_status == ProvisionStatus.WAITING:
        self._ProvisionTarget(target)

  def _ProvisionTarget(self, target):
    """Provision the attestation key into the specific target.

    Args:
      target: The target to be provisioned.
    """
    operation = 'Attestation Key Provisioning'
    self._SendOperationStartEvent(operation, target)
    self.PauseRefresh()

    try:
      self.atft_manager.Provision(target)
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation, target)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation, target)
      # If it fails, one key might also be used.
      self._CheckATFAStatus()
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation, target)
    self._CheckLowKeyAlert()

  def _HandleStateTransition(self, target):
    """Handles the state transition for automatic key provisioning.

    A normal flow should be:
      WAITING->FUSEVBOOT_SUCCESS->REBOOT_SUCCESS->LOCKAVB_SUCCESS
      ->PROVISION_SUCCESS

    Args:
      target: The target device object.
    """
    self.auto_prov_lock.acquire()
    serial = target.serial_number
    while not ProvisionStatus.isFailed(target.provision_status):
      target = self.atft_manager.GetTargetDevice(serial)
      if not target:
        # The target disappear somehow.
        break
      if not self.auto_prov:
        # Auto provision mode exited.
        break
      if not target.provision_state.bootloader_locked:
        self._FuseVbootKeyTarget(target)
        continue
      elif not target.provision_state.avb_perm_attr_set:
        self._FusePermAttrTarget(target)
        continue
      elif not target.provision_state.avb_locked:
        self._LockAvbTarget(target)
        continue
      elif not target.provision_state.provisioned:
        self._ProvisionTarget(target)
        if self.atft_manager.GetATFAKeysLeft() == 0:
          # No keys left. If it's auto provisioning mode, exit.
          self._SendAlertEvent(self.ALERT_NO_KEYS_LEFT_LEAVE_PROV)
          self.toolbar.ToggleTool(self.ID_TOOL_PROVISION, False)
          self.OnToggleAutoProv(None)
      break
    self.auto_dev_serials.remove(serial)
    self.auto_prov_lock.release()

  def _ProcessKey(self):
    """Ask ATFA device to process the stored keybundle.
    """
    operation = 'ATFA device process key bundle'
    self._SendOperationStartEvent(operation)
    self.PauseRefresh()

    try:
      self.atft_manager.ProcessATFAKey()
    except DeviceNotFoundException as e:
      e.SetMsg('No Available ATFA!')
      self._HandleException('W', e, operation)
      return
    except FastbootFailure as e:
      self._HandleException('E', e, operation)
      return
    finally:
      self.ResumeRefresh()

    self._SendOperationSucceedEvent(operation)

    # Check ATFA status after new key stored.
    if self.atft_manager.product_info:
      self._CheckATFAStatus()

  def _GetSelectedSerials(self):
    """Get the list of selected serial numbers in the device list.

    Returns:
      A list of serial numbers of the selected target devices.
    """
    selected_serials = []
    selected_item = self.target_devs_output.GetFirstSelected()
    if selected_item == -1:
      return selected_serials
    serial = self.target_devs_output.GetItem(selected_item, 0).GetText()
    selected_serials.append(serial)
    while True:
      selected_item = self.target_devs_output.GetNextSelected(selected_item)
      if selected_item == -1:
        break
      serial = self.target_devs_output.GetItem(selected_item, 0).GetText()
      selected_serials.append(serial)
    return selected_serials


def main():
  app = wx.App()
  Atft()
  app.MainLoop()


if __name__ == '__main__':
  main()
