# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
import glob
import logging
import os
import re
import shutil
import time
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import upstart


# Possible display power settings. Copied from chromeos::DisplayPowerState
# in Chrome's dbus service constants.
DISPLAY_POWER_ALL_ON = 0
DISPLAY_POWER_ALL_OFF = 1
DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON = 2
DISPLAY_POWER_INTERNAL_ON_EXTERNAL_OFF = 3
# for bounds checking
DISPLAY_POWER_MAX = 4

# Retry times for ectool chargecontrol
ECTOOL_CHARGECONTROL_RETRY_TIMES = 3
ECTOOL_CHARGECONTROL_TIMEOUT_SECS = 3


def get_x86_cpu_arch():
    """Identify CPU architectural type.

    Intel's processor naming conventions is a mine field of inconsistencies.
    Armed with that, this method simply tries to identify the architecture of
    systems we care about.

    TODO(tbroch) grow method to cover processors numbers outlined in:
        http://www.intel.com/content/www/us/en/processors/processor-numbers.html
        perhaps returning more information ( brand, generation, features )

    Returns:
      String, explicitly (Atom, Core, Celeron) or None
    """
    cpuinfo = utils.read_file('/proc/cpuinfo')

    if re.search(r'AMD.*A6-92[0-9][0-9].*RADEON.*R[245]', cpuinfo):
        return 'Stoney'
    if re.search(r'Intel.*Atom.*[NZ][2-6]', cpuinfo):
        return 'Atom'
    if re.search(r'Intel.*Celeron.*N2[89][0-9][0-9]', cpuinfo):
        return 'Celeron N2000'
    if re.search(r'Intel.*Celeron.*N3[0-9][0-9][0-9]', cpuinfo):
        return 'Celeron N3000'
    if re.search(r'Intel.*Celeron.*[0-9]{3,4}', cpuinfo):
        return 'Celeron'
    # https://ark.intel.com/products/series/94028/5th-Generation-Intel-Core-M-Processors
    # https://ark.intel.com/products/series/94025/6th-Generation-Intel-Core-m-Processors
    # https://ark.intel.com/products/series/95542/7th-Generation-Intel-Core-m-Processors
    if re.search(r'Intel.*Core.*[mM][357]-[567][Y0-9][0-9][0-9]', cpuinfo):
        return 'Core M'
    if re.search(r'Intel.*Core.*i[357]-[234][0-9][0-9][0-9]', cpuinfo):
        return 'Core'

    logging.info(cpuinfo)
    return None


def has_rapl_support():
    """Identify if CPU microarchitecture supports RAPL energy profile.

    TODO(harry.pan): Since Sandy Bridge, all microarchitectures have RAPL
    in various power domains. With that said, the Silvermont and Airmont
    support RAPL as well, while the ESU (Energy Status Unit of MSR 606H)
    are in different multipiler against others, hense not list by far.

    Returns:
        Boolean, True if RAPL supported, False otherwise.
    """
    rapl_set = set(["Haswell", "Haswell-E", "Broadwell", "Skylake", "Goldmont",
                    "Kaby Lake"])
    cpu_uarch = utils.get_intel_cpu_uarch()
    if (cpu_uarch in rapl_set):
        return True
    else:
        # The cpu_uarch here is either unlisted uarch, or family_model.
        logging.debug("%s is not in RAPL support collection", cpu_uarch)
    return False


def has_powercap_support():
    """Identify if OS supports powercap sysfs.

    Returns:
        Boolean, True if powercap supported, False otherwise.
    """
    return os.path.isdir('/sys/devices/virtual/powercap/intel-rapl/')


def _call_dbus_method(destination, path, interface, method_name, args):
    """Performs a generic dbus method call."""
    command = ('dbus-send --type=method_call --system '
               '--dest=%s %s %s.%s %s') % (destination, path, interface,
                                           method_name, args)
    utils.system_output(command)


def call_powerd_dbus_method(method_name, args=''):
    """
    Calls a dbus method exposed by powerd.

    Arguments:
    @param method_name: name of the dbus method.
    @param args: string containing args to dbus method call.
    """
    _call_dbus_method(destination='org.chromium.PowerManager',
                      path='/org/chromium/PowerManager',
                      interface='org.chromium.PowerManager',
                      method_name=method_name, args=args)


def get_power_supply():
    """
    Determine what type of power supply the host has.

    Copied from server/host/cros_hosts.py

    @returns a string representing this host's power supply.
             'power:battery' when the device has a battery intended for
                    extended use
             'power:AC_primary' when the device has a battery not intended
                    for extended use (for moving the machine, etc)
             'power:AC_only' when the device has no battery at all.
    """
    try:
        psu = utils.system_output('mosys psu type')
    except Exception:
        # The psu command for mosys is not included for all platforms. The
        # assumption is that the device will have a battery if the command
        # is not found.
        return 'power:battery'

    psu_str = psu.strip()
    if psu_str == 'unknown':
        return None

    return 'power:%s' % psu_str


def get_sleep_state():
    """
    Returns the current powerd configuration of the sleep state.
    Can be "freeze" or "mem".
    """
    cmd = 'check_powerd_config --suspend_to_idle'
    result = utils.run(cmd, ignore_status=True)
    return 'freeze' if result.exit_status == 0 else 'mem'


def has_battery():
    """Determine if DUT has a battery.

    Returns:
        Boolean, False if known not to have battery, True otherwise.
    """
    rv = True
    power_supply = get_power_supply()
    if power_supply == 'power:battery':
        # TODO(tbroch) if/when 'power:battery' param is reliable
        # remove board type logic.  Also remove verbose mosys call.
        _NO_BATTERY_BOARD_TYPE = ['CHROMEBOX', 'CHROMEBIT', 'CHROMEBASE']
        board_type = utils.get_board_type()
        if board_type in _NO_BATTERY_BOARD_TYPE:
            logging.warn('Do NOT believe type %s has battery. '
                         'See debug for mosys details', board_type)
            psu = utils.system_output('mosys -vvvv psu type',
                                      ignore_status=True)
            logging.debug(psu)
            rv = False
    elif power_supply == 'power:AC_only':
        rv = False

    return rv


def get_low_battery_shutdown_percent():
    """Get the percent-based low-battery shutdown threshold.

    Returns:
        Float, percent-based low-battery shutdown threshold. 0 if error.
    """
    ret = 0.0
    try:
        command = 'check_powerd_config --low_battery_shutdown_percent'
        ret = float(utils.run(command).stdout)
    except error.CmdError:
        logging.debug("Can't run %s", command)
    except ValueError:
        logging.debug("Didn't get number from %s", command)

    return ret


def _charge_control_by_ectool(is_charge):
    """execute ectool command.

    Args:
      is_charge: Boolean, True for charging, False for discharging.

    Returns:
      Boolean, True if the command success, False otherwise.
    """
    ec_cmd_discharge = 'ectool chargecontrol discharge'
    ec_cmd_normal = 'ectool chargecontrol normal'
    try:
       if is_charge:
           utils.run(ec_cmd_normal)
       else:
           utils.run(ec_cmd_discharge)
    except error.CmdError as e:
        logging.warning('Unable to use ectool: %s', e)
        return False

    success = utils.wait_for_value(lambda: (
        is_charge != bool(re.search(r'Flags.*DISCHARGING',
                                    utils.run('ectool battery',
                                              ignore_status=True).stdout,
                                    re.MULTILINE))),
        expected_value=True, timeout_sec=ECTOOL_CHARGECONTROL_TIMEOUT_SECS)
    return success


def charge_control_by_ectool(is_charge):
    """Force the battery behavior by the is_charge paremeter.

    Args:
      is_charge: Boolean, True for charging, False for discharging.

    Returns:
      Boolean, True if the command success, False otherwise.
    """
    for i in xrange(ECTOOL_CHARGECONTROL_RETRY_TIMES):
        if _charge_control_by_ectool(is_charge):
            return True

    return False


class BacklightException(Exception):
    """Class for Backlight exceptions."""


class Backlight(object):
    """Class for control of built-in panel backlight.

    Public methods:
       set_level: Set backlight level to the given brightness.
       set_percent: Set backlight level to the given brightness percent.
       set_resume_level: Set backlight level on resume to the given brightness.
       set_resume_percent: Set backlight level on resume to the given brightness
                           percent.
       set_default: Set backlight to CrOS default.

       get_level: Get backlight level currently.
       get_max_level: Get maximum backight level.
       get_percent: Get backlight percent currently.
       restore: Restore backlight to initial level when instance created.

    Public attributes:
        default_brightness_percent: float of default brightness

    Private methods:
        _try_bl_cmd: run a backlight command.

    Private attributes:
        _init_level: integer of backlight level when object instantiated.
        _can_control_bl: boolean determining whether backlight can be controlled
                         or queried
    """
    # Default brightness is based on expected average use case.
    # See http://www.chromium.org/chromium-os/testing/power-testing for more
    # details.

    def __init__(self, default_brightness_percent=0):
        """Constructor.

        attributes:
        """
        cmd = "mosys psu type"
        result = utils.system_output(cmd, ignore_status=True).strip()
        self._can_control_bl = not result == "AC_only"

        self._init_level = self.get_level()
        self.default_brightness_percent = default_brightness_percent

        logging.debug("device can_control_bl: %s", self._can_control_bl)
        if not self._can_control_bl:
            return

        if not self.default_brightness_percent:
            cmd = \
                "backlight_tool --get_initial_brightness --lux=150 2>/dev/null"
            try:
                level = float(utils.system_output(cmd).rstrip())
                self.default_brightness_percent = \
                    (level / self.get_max_level()) * 100
                logging.info("Default backlight brightness percent = %f",
                             self.default_brightness_percent)
            except error.CmdError:
                self.default_brightness_percent = 40.0
                logging.warning("Unable to determine default backlight "
                                "brightness percent.  Setting to %f",
                                self.default_brightness_percent)

    def _try_bl_cmd(self, arg_str):
        """Perform backlight command.

        Args:
          arg_str:  String of additional arguments to backlight command.

        Returns:
          String output of the backlight command.

        Raises:
          error.TestFail: if 'cmd' returns non-zero exit status.
        """
        if not self._can_control_bl:
            return 0
        cmd = 'backlight_tool %s' % (arg_str)
        logging.debug("backlight_cmd: %s", cmd)
        try:
            return utils.system_output(cmd).rstrip()
        except error.CmdError:
            raise error.TestFail(cmd)

    def set_level(self, level):
        """Set backlight level to the given brightness.

        Args:
          level: integer of brightness to set
        """
        self._try_bl_cmd('--set_brightness=%d' % (level))

    def set_percent(self, percent):
        """Set backlight level to the given brightness percent.

        Args:
          percent: float between 0 and 100
        """
        self._try_bl_cmd('--set_brightness_percent=%f' % (percent))

    def set_resume_level(self, level):
        """Set backlight level on resume to the given brightness.

        Args:
          level: integer of brightness to set
        """
        self._try_bl_cmd('--set_resume_brightness=%d' % (level))

    def set_resume_percent(self, percent):
        """Set backlight level on resume to the given brightness percent.

        Args:
          percent: float between 0 and 100
        """
        self._try_bl_cmd('--set_resume_brightness_percent=%f' % (percent))

    def set_default(self):
        """Set backlight to CrOS default.
        """
        self.set_percent(self.default_brightness_percent)

    def get_level(self):
        """Get backlight level currently.

        Returns integer of current backlight level or zero if no backlight
        exists.
        """
        return int(self._try_bl_cmd('--get_brightness'))

    def get_max_level(self):
        """Get maximum backight level.

        Returns integer of maximum backlight level or zero if no backlight
        exists.
        """
        return int(self._try_bl_cmd('--get_max_brightness'))

    def get_percent(self):
        """Get backlight percent currently.

        Returns float of current backlight percent or zero if no backlight
        exists
        """
        return float(self._try_bl_cmd('--get_brightness_percent'))

    def restore(self):
        """Restore backlight to initial level when instance created."""
        self.set_level(self._init_level)


class KbdBacklightException(Exception):
    """Class for KbdBacklight exceptions."""


class KbdBacklight(object):
    """Class for control of keyboard backlight.

    Example code:
        kblight = power_utils.KbdBacklight()
        kblight.set(10)
        print "kblight % is %.f" % kblight.get_percent()

    Public methods:
        set_percent: Sets the keyboard backlight to a percent.
        get_percent: Get current keyboard backlight percentage.
        set_level: Sets the keyboard backlight to a level.
        get_default_level: Get default keyboard backlight brightness level

    Private attributes:
        _default_backlight_level: keboard backlight level set by default

    """

    def __init__(self):
        cmd = 'check_powerd_config --keyboard_backlight'
        result = utils.run(cmd, ignore_status=True)
        if result.exit_status:
            raise KbdBacklightException('Keyboard backlight support' +
                                        'is not enabled')
        try:
            cmd = \
                "backlight_tool --keyboard --get_initial_brightness 2>/dev/null"
            self._default_backlight_level = int(
                utils.system_output(cmd).rstrip())
            logging.info("Default keyboard backlight brightness level = %d",
                         self._default_backlight_level)
        except Exception:
            raise KbdBacklightException('Keyboard backlight is malfunctioning')

    def get_percent(self):
        """Get current keyboard brightness setting percentage.

        Returns:
            float, percentage of keyboard brightness in the range [0.0, 100.0].
        """
        cmd = 'backlight_tool --keyboard --get_brightness_percent'
        return float(utils.system_output(cmd).strip())

    def get_default_level(self):
        """
        Returns the default backlight level.

        Returns:
            The default keyboard backlight level.
        """
        return self._default_backlight_level

    def set_percent(self, percent):
        """Set keyboard backlight percent.

        Args:
        @param percent: float value in the range [0.0, 100.0]
                        to set keyboard backlight to.
        """
        cmd = ('backlight_tool --keyboard --set_brightness_percent=' +
               str(percent))
        utils.system(cmd)

    def set_level(self, level):
        """
        Set keyboard backlight to given level.
        Args:
        @param level: level to set keyboard backlight to.
        """
        cmd = 'backlight_tool --keyboard --set_brightness=' + str(level)
        utils.system(cmd)


class BacklightController(object):
    """Class to simulate control of backlight via keyboard or Chrome UI.

    Public methods:
      increase_brightness: Increase backlight by one adjustment step.
      decrease_brightness: Decrease backlight by one adjustment step.
      set_brightness_to_max: Increase backlight to max by calling
          increase_brightness()
      set_brightness_to_min: Decrease backlight to min or zero by calling
          decrease_brightness()

    Private attributes:
      _max_num_steps: maximum number of backlight adjustment steps between 0 and
                      max brightness.
    """

    def __init__(self):
        self._max_num_steps = 16

    def decrease_brightness(self, allow_off=False):
        """
        Decrease brightness by one step, as if the user pressed the brightness
        down key or button.

        Arguments
        @param allow_off: Boolean flag indicating whether the brightness can be
                     reduced to zero.
                     Set to true to simulate brightness down key.
                     set to false to simulate Chrome UI brightness down button.
        """
        call_powerd_dbus_method('DecreaseScreenBrightness',
                                'boolean:%s' %
                                ('true' if allow_off else 'false'))

    def increase_brightness(self):
        """
        Increase brightness by one step, as if the user pressed the brightness
        up key or button.
        """
        call_powerd_dbus_method('IncreaseScreenBrightness')

    def set_brightness_to_max(self):
        """
        Increases the brightness using powerd until the brightness reaches the
        maximum value. Returns when it reaches the maximum number of brightness
        adjustments
        """
        num_steps_taken = 0
        while num_steps_taken < self._max_num_steps:
            self.increase_brightness()
            num_steps_taken += 1

    def set_brightness_to_min(self, allow_off=False):
        """
        Decreases the brightness using powerd until the brightness reaches the
        minimum value (zero or the minimum nonzero value). Returns when it
        reaches the maximum number of brightness adjustments.

        Arguments
        @param allow_off: Boolean flag indicating whether the brightness can be
                     reduced to zero.
                     Set to true to simulate brightness down key.
                     set to false to simulate Chrome UI brightness down button.
        """
        num_steps_taken = 0
        while num_steps_taken < self._max_num_steps:
            self.decrease_brightness(allow_off)
            num_steps_taken += 1


class DisplayException(Exception):
    """Class for Display exceptions."""


def set_display_power(power_val):
    """Function to control screens via Chrome.

    Possible arguments:
      DISPLAY_POWER_ALL_ON,
      DISPLAY_POWER_ALL_OFF,
      DISPLAY_POWER_INTERNAL_OFF_EXTERNAL_ON,
      DISPLAY_POWER_INTERNAL_ON_EXTENRAL_OFF
    """
    if (not isinstance(power_val, int)
            or power_val < DISPLAY_POWER_ALL_ON
            or power_val >= DISPLAY_POWER_MAX):
        raise DisplayException('Invalid display power setting: %d' % power_val)
    _call_dbus_method(destination='org.chromium.DisplayService',
                      path='/org/chromium/DisplayService',
                      interface='org.chomium.DisplayServiceInterface',
                      method_name='SetPower',
                      args='int32:%d' % power_val)


class PowerPrefChanger(object):
    """
    Class to temporarily change powerd prefs. Construct with a dict of
    pref_name/value pairs (e.g. {'disable_idle_suspend':0}). Destructor (or
    reboot) will restore old prefs automatically."""

    _PREFDIR = '/var/lib/power_manager'
    _TEMPDIR = '/tmp/autotest_powerd_prefs'

    def __init__(self, prefs):
        shutil.copytree(self._PREFDIR, self._TEMPDIR)
        for name, value in prefs.iteritems():
            utils.write_one_line('%s/%s' % (self._TEMPDIR, name), value)
        utils.system('mount --bind %s %s' % (self._TEMPDIR, self._PREFDIR))
        upstart.restart_job('powerd')

    def finalize(self):
        """finalize"""
        if os.path.exists(self._TEMPDIR):
            utils.system('umount %s' % self._PREFDIR, ignore_status=True)
            shutil.rmtree(self._TEMPDIR)
            upstart.restart_job('powerd')

    def __del__(self):
        self.finalize()


class Registers(object):
    """Class to examine PCI and MSR registers."""

    def __init__(self):
        self._cpu_id = 0
        self._rdmsr_cmd = 'iotools rdmsr'
        self._mmio_read32_cmd = 'iotools mmio_read32'
        self._rcba = 0xfed1c000

        self._pci_read32_cmd = 'iotools pci_read32'
        self._mch_bar = None
        self._dmi_bar = None

    def _init_mch_bar(self):
        if self._mch_bar != None:
            return
        # MCHBAR is at offset 0x48 of B/D/F 0/0/0
        cmd = '%s 0 0 0 0x48' % (self._pci_read32_cmd)
        self._mch_bar = int(utils.system_output(cmd), 16) & 0xfffffffe
        logging.debug('MCH BAR is %s', hex(self._mch_bar))

    def _init_dmi_bar(self):
        if self._dmi_bar != None:
            return
        # DMIBAR is at offset 0x68 of B/D/F 0/0/0
        cmd = '%s 0 0 0 0x68' % (self._pci_read32_cmd)
        self._dmi_bar = int(utils.system_output(cmd), 16) & 0xfffffffe
        logging.debug('DMI BAR is %s', hex(self._dmi_bar))

    def _read_msr(self, register):
        cmd = '%s %d %s' % (self._rdmsr_cmd, self._cpu_id, register)
        return int(utils.system_output(cmd), 16)

    def _read_mmio_read32(self, address):
        cmd = '%s 0x%x' % (self._mmio_read32_cmd, address)
        return int(utils.system_output(cmd), 16)

    def _read_dmi_bar(self, offset):
        self._init_dmi_bar()
        return self._read_mmio_read32(self._dmi_bar + int(offset, 16))

    def _read_mch_bar(self, offset):
        self._init_mch_bar()
        return self._read_mmio_read32(self._mch_bar + int(offset, 16))

    def _read_rcba(self, offset):
        return self._read_mmio_read32(self._rcba + int(offset, 16))

    def _shift_mask_match(self, reg_name, value, match):
        expr = match[1]
        bits = match[0].split(':')
        operator = match[2] if len(match) == 3 else '=='
        hi_bit = int(bits[0])
        if len(bits) == 2:
            lo_bit = int(bits[1])
        else:
            lo_bit = int(bits[0])

        value >>= lo_bit
        mask = (1 << (hi_bit - lo_bit + 1)) - 1
        value &= mask

        good = eval("%d %s %d" % (value, operator, expr))
        if not good:
            logging.error('FAILED: %s bits: %s value: %s mask: %s expr: %s ' +
                          'operator: %s', reg_name, bits, hex(value), mask,
                          expr, operator)
        return good

    def _verify_registers(self, reg_name, read_fn, match_list):
        errors = 0
        for k, v in match_list.iteritems():
            r = read_fn(k)
            for item in v:
                good = self._shift_mask_match(reg_name, r, item)
                if not good:
                    errors += 1
                    logging.error('Error(%d), %s: reg = %s val = %s match = %s',
                                  errors, reg_name, k, hex(r), v)
                else:
                    logging.debug('ok, %s: reg = %s val = %s match = %s',
                                  reg_name, k, hex(r), v)
        return errors

    def verify_msr(self, match_list):
        """
        Verify MSR

        @param match_list: match list
        """
        errors = 0
        for cpu_id in xrange(0, max(utils.count_cpus(), 1)):
            self._cpu_id = cpu_id
            errors += self._verify_registers('msr', self._read_msr, match_list)
        return errors

    def verify_dmi(self, match_list):
        """
        Verify DMI

        @param match_list: match list
        """
        return self._verify_registers('dmi', self._read_dmi_bar, match_list)

    def verify_mch(self, match_list):
        """
        Verify MCH

        @param match_list: match list
        """
        return self._verify_registers('mch', self._read_mch_bar, match_list)

    def verify_rcba(self, match_list):
        """
        Verify RCBA

        @param match_list: match list
        """
        return self._verify_registers('rcba', self._read_rcba, match_list)


class USBDevicePower(object):
    """Class for USB device related power information.

    Public Methods:
        autosuspend: Return boolean whether USB autosuspend is enabled or False
                     if not or unable to determine

    Public attributes:
        vid: string of USB Vendor ID
        pid: string of USB Product ID
        whitelisted: Boolean if USB device is whitelisted for USB auto-suspend

    Private attributes:
       path: string to path of the USB devices in sysfs ( /sys/bus/usb/... )

    TODO(tbroch): consider converting to use of pyusb although not clear its
    beneficial if it doesn't parse power/control
    """

    def __init__(self, vid, pid, whitelisted, path):
        self.vid = vid
        self.pid = pid
        self.whitelisted = whitelisted
        self._path = path

    def autosuspend(self):
        """Determine current value of USB autosuspend for device."""
        control_file = os.path.join(self._path, 'control')
        if not os.path.exists(control_file):
            logging.info('USB: power control file not found for %s', dir)
            return False

        out = utils.read_one_line(control_file)
        logging.debug('USB: control set to %s for %s', out, control_file)
        return (out == 'auto')


class USBPower(object):
    """Class to expose USB related power functionality.

    Initially that includes the policy around USB auto-suspend and our
    whitelisting of devices that are internal to CrOS system.

    Example code:
       usbdev_power = power_utils.USBPower()
       for device in usbdev_power.devices
           if device.is_whitelisted()
               ...

    Public attributes:
        devices: list of USBDevicePower instances

    Private functions:
        _is_whitelisted: Returns Boolean if USB device is whitelisted for USB
                         auto-suspend
        _load_whitelist: Reads whitelist and stores int _whitelist attribute

    Private attributes:
        _wlist_file: path to laptop-mode-tools (LMT) USB autosuspend
                         conf file.
        _wlist_vname: string name of LMT USB autosuspend whitelist
                          variable
        _whitelisted: list of USB device vid:pid that are whitelisted.
                        May be regular expressions.  See LMT for details.
    """

    def __init__(self):
        self._wlist_file = \
            '/etc/laptop-mode/conf.d/board-specific/usb-autosuspend.conf'
        self._wlist_vname = '$AUTOSUSPEND_USBID_WHITELIST'
        self._whitelisted = None
        self.devices = []

    def _load_whitelist(self):
        """Load USB device whitelist for enabling USB autosuspend

        CrOS whitelists only internal USB devices to enter USB auto-suspend mode
        via laptop-mode tools.
        """
        cmd = "source %s && echo %s" % (self._wlist_file,
                                        self._wlist_vname)
        out = utils.system_output(cmd, ignore_status=True)
        logging.debug('USB whitelist = %s', out)
        self._whitelisted = out.split()

    def _is_whitelisted(self, vid, pid):
        """Check to see if USB device vid:pid is whitelisted.

        Args:
          vid: string of USB vendor ID
          pid: string of USB product ID

        Returns:
          True if vid:pid in whitelist file else False
        """
        if self._whitelisted is None:
            self._load_whitelist()

        match_str = "%s:%s" % (vid, pid)
        for re_str in self._whitelisted:
            if re.match(re_str, match_str):
                return True
        return False

    def query_devices(self):
        """."""
        dirs_path = '/sys/bus/usb/devices/*/power'
        dirs = glob.glob(dirs_path)
        if not dirs:
            logging.info('USB power path not found')
            return 1

        for dirpath in dirs:
            vid_path = os.path.join(dirpath, '..', 'idVendor')
            pid_path = os.path.join(dirpath, '..', 'idProduct')
            if not os.path.exists(vid_path):
                logging.debug("No vid for USB @ %s", vid_path)
                continue
            vid = utils.read_one_line(vid_path)
            pid = utils.read_one_line(pid_path)
            whitelisted = self._is_whitelisted(vid, pid)
            self.devices.append(USBDevicePower(vid, pid, whitelisted, dirpath))


class DisplayPanelSelfRefresh(object):
    """Class for control and monitoring of display's PSR."""
    _PSR_STATUS_FILE_X86 = '/sys/kernel/debug/dri/0/i915_edp_psr_status'
    _PSR_STATUS_FILE_ARM = '/sys/kernel/debug/dri/*/psr_active_ms'

    def __init__(self, init_time=time.time()):
        """Initializer.

        @Public attributes:
            supported: Boolean of whether PSR is supported or not

        @Private attributes:
            _init_time: time when PSR class was instantiated.
            _init_counter: integer of initial value of residency counter.
            _keyvals: dictionary of keyvals
        """
        self._psr_path = ''
        if os.path.exists(self._PSR_STATUS_FILE_X86):
            self._psr_path = self._PSR_STATUS_FILE_X86
            self._psr_parse_prefix = 'Performance_Counter:'
        else:
            paths = glob.glob(self._PSR_STATUS_FILE_ARM)
            if paths:
                # Should be only one PSR file
                self._psr_path = paths[0]
                self._psr_parse_prefix = ''

        self._init_time = init_time
        self._init_counter = self._get_counter()
        self._keyvals = {}
        self.supported = (self._init_counter != None)

    def _get_counter(self):
        """Get the current value of the system PSR counter.

        This counts the number of milliseconds the system has resided in PSR.

        @returns: amount of time PSR has been active since boot in ms, or None if
        the performance counter can't be read.
        """
        try:
            count = utils.get_field(utils.read_file(self._psr_path),
                                    0, linestart=self._psr_parse_prefix)
        except IOError:
            logging.info("Can't find or read PSR status file")
            return None

        logging.debug("PSR performance counter: %s", count)
        return int(count) if count else None

    def _calc_residency(self):
        """Calculate the PSR residency."""
        if not self.supported:
            return 0

        tdelta = time.time() - self._init_time
        cdelta = self._get_counter() - self._init_counter
        return cdelta / (10 * tdelta)

    def refresh(self):
        """Refresh PSR related data."""
        self._keyvals['percent_psr_residency'] = self._calc_residency()

    def get_keyvals(self):
        """Get keyvals associated with PSR data.

        @returns dictionary of keyvals
        """
        return self._keyvals
