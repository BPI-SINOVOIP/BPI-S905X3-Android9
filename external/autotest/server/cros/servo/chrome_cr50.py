# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import functools
import logging
import time

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import cr50_utils
from autotest_lib.server.cros.servo import chrome_ec


def servo_v4_command(func):
    """Decorator for methods only relevant to tests running with servo v4."""
    @functools.wraps(func)
    def wrapper(instance, *args, **kwargs):
        """Ignore servo v4 functions it's not being used."""
        if instance.using_servo_v4():
            return func(instance, *args, **kwargs)
        logging.info("not using servo v4. ignoring %s", func.func_name)
    return wrapper


class ChromeCr50(chrome_ec.ChromeConsole):
    """Manages control of a Chrome Cr50.

    We control the Chrome Cr50 via the console of a Servo board. Chrome Cr50
    provides many interfaces to set and get its behavior via console commands.
    This class is to abstract these interfaces.
    """
    # The amount of time you need to show physical presence.
    PP_SHORT = 15
    PP_LONG = 300
    IDLE_COUNT = 'count: (\d+)'
    # The version has four groups: the partition, the header version, debug
    # descriptor and then version string.
    # There are two partitions A and B. The active partition is marked with a
    # '*'. If it is a debug image '/DBG' is added to the version string. If the
    # image has been corrupted, the version information will be replaced with
    # 'Error'.
    # So the output may look something like this.
    #   RW_A:    0.0.21/cr50_v1.1.6133-fd788b
    #   RW_B:  * 0.0.22/DBG/cr50_v1.1.6138-b9f0b1d
    # Or like this if the region was corrupted.
    #   RW_A:  * 0.0.21/cr50_v1.1.6133-fd788b
    #   RW_B:    Error
    VERSION_FORMAT = '\nRW_(A|B): +%s +(\d+\.\d+\.\d+|Error)(/DBG)?(\S+)?\s'
    INACTIVE_VERSION = VERSION_FORMAT % ''
    ACTIVE_VERSION = VERSION_FORMAT % '\*'
    # Following lines of the version output may print the image board id
    # information. eg.
    # BID A:   5a5a4146:ffffffff:00007f00 Yes
    # BID B:   00000000:00000000:00000000 Yes
    # Use the first group from ACTIVE_VERSION to match the active board id
    # partition.
    BID_ERROR = 'read_board_id: failed'
    BID_FORMAT = ':\s+[a-f0-9:]+ '
    ACTIVE_BID = r'%s.*(\1%s|%s.*>)' % (ACTIVE_VERSION, BID_FORMAT,
            BID_ERROR)
    WAKE_CHAR = '\n'
    START_UNLOCK_TIMEOUT = 20
    GETTIME = ['= (\S+)']
    FWMP_LOCKED_PROD = ["Managed device console can't be unlocked"]
    FWMP_LOCKED_DBG = ['Ignoring FWMP unlock setting']
    MAX_RETRY_COUNT = 5
    START_STR = ['(.*Console is enabled;)']
    REBOOT_DELAY_WITH_CCD = 60
    REBOOT_DELAY_WITH_FLEX = 3
    ON_STRINGS = ['enable', 'enabled', 'on']


    def __init__(self, servo):
        super(ChromeCr50, self).__init__(servo, 'cr50_uart')


    def send_command(self, commands):
        """Send command through UART.

        Cr50 will drop characters input to the UART when it resumes from sleep.
        If servo is not using ccd, send some dummy characters before sending the
        real command to make sure cr50 is awake.
        """
        if not self.using_ccd():
            super(ChromeCr50, self).send_command(self.WAKE_CHAR)
        super(ChromeCr50, self).send_command(commands)


    def send_command_get_output(self, command, regexp_list):
        """Send command through UART and wait for response.

        Cr50 will drop characters input to the UART when it resumes from sleep.
        If servo is not using ccd, send some dummy characters before sending the
        real command to make sure cr50 is awake.
        """
        if not self.using_ccd():
            super(ChromeCr50, self).send_command(self.WAKE_CHAR)
        return super(ChromeCr50, self).send_command_get_output(command,
                                                               regexp_list)


    def get_deep_sleep_count(self):
        """Get the deep sleep count from the idle task"""
        result = self.send_command_get_output('idle', [self.IDLE_COUNT])
        return int(result[0][1])


    def clear_deep_sleep_count(self):
        """Clear the deep sleep count"""
        result = self.send_command_get_output('idle c', [self.IDLE_COUNT])
        if int(result[0][1]):
            raise error.TestFail("Could not clear deep sleep count")


    def has_command(self, cmd):
        """Returns 1 if cr50 has the command 0 if it doesn't"""
        try:
            self.send_command_get_output('help', [cmd])
        except:
            logging.info("Image does not include '%s' command", cmd)
            return 0
        return 1


    def erase_nvmem(self):
        """Use flasherase to erase both nvmem sections"""
        if not self.has_command('flasherase'):
            raise error.TestError("need image with 'flasherase'")

        self.send_command('flasherase 0x7d000 0x3000')
        self.send_command('flasherase 0x3d000 0x3000')


    def reboot(self):
        """Reboot Cr50 and wait for cr50 to reset"""
        response = [] if self.using_ccd() else self.START_STR
        self.send_command_get_output('reboot', response)

        # ccd will stop working after the reboot. Wait until that happens and
        # reenable it.
        if self.using_ccd():
            self.wait_for_reboot()


    def _uart_wait_for_reboot(self, timeout=60):
        """Wait for the cr50 to reboot and enable the console.

        This will wait up to timeout seconds for cr50 to print the start string.

        Args:
            timeout: seconds to wait to detect the reboot.
        """
        original_timeout = float(self._servo.get('cr50_uart_timeout'))
        # Change the console timeout to timeout, so we wait at least that long
        # for cr50 to print the start string.
        self._servo.set_nocheck('cr50_uart_timeout', timeout)
        try:
            self.send_command_get_output('\n', self.START_STR)
            logging.debug('Detected cr50 reboot')
        except error.TestFail, e:
            logging.debug('Failed to detect cr50 reboot')
        # Reset the timeout.
        self._servo.set_nocheck('cr50_uart_timeout', original_timeout)


    def wait_for_reboot(self, timeout=60):
        """Wait for cr50 to reboot"""
        if self.using_ccd():
            # Cr50 USB is reset when it reboots. Wait for the CCD connection to
            # go down to detect the reboot.
            self.wait_for_ccd_disable(timeout, raise_error=False)
            self.ccd_enable()
        else:
            self._uart_wait_for_reboot(timeout)


    def rollback(self, eraseflashinfo=True, chip_bid=None, chip_flags=None):
        """Set the reset counter high enough to force a rollback then reboot

        Set the new board id before rolling back if one is given.

        Args:
            eraseflashinfo: True if eraseflashinfo should be run before rollback
            chip_bid: the integer representation of chip board id or None if the
                      board id should be erased during rollback
            chip_flags: the integer representation of chip board id flags or
                        None if the board id should be erased during rollback
        """
        if (not self.has_command('rollback') or not
            self.has_command('eraseflashinfo')):
            raise error.TestError("need image with 'rollback' and "
                "'eraseflashinfo'")

        inactive_partition = self.get_inactive_version_info()[0]
        # Set the board id if both the board id and flags have been given.
        set_bid = chip_bid and chip_flags

        # Erase the infomap
        if eraseflashinfo or set_bid:
            self.send_command('eraseflashinfo')

        # Update the board id after it has been erased
        if set_bid:
            self.send_command('bid 0x%x 0x%x' % (chip_bid, chip_flags))

        self.send_command('rollback')

        # If we aren't using ccd, we should be able to detect the reboot
        # almost immediately
        self.wait_for_reboot(self.REBOOT_DELAY_WITH_CCD if self.using_ccd() else
                self.REBOOT_DELAY_WITH_FLEX)

        running_partition = self.get_active_version_info()[0]
        if inactive_partition != running_partition:
            raise error.TestError("Failed to rollback to inactive image")


    def rolledback(self):
        """Returns true if cr50 just rolled back"""
        return int(self._servo.get('cr50_reset_count')) > self.MAX_RETRY_COUNT


    def get_version_info(self, regexp):
        """Get information from the version command"""
        return self.send_command_get_output('ver', [regexp])[0][1::]


    def get_inactive_version_info(self):
        """Get the active partition, version, and hash"""
        return self.get_version_info(self.INACTIVE_VERSION)


    def get_active_version_info(self):
        """Get the active partition, version, and hash"""
        return self.get_version_info(self.ACTIVE_VERSION)


    def get_active_board_id_str(self):
        """Get the running image board id.

        Returns:
            The board id string or None if the image does not support board id
            or the image is not board id locked.
        """
        # Getting the board id from the version console command is only
        # supported in board id locked images .22 and above. Any image that is
        # board id locked will have support for getting the image board id.
        #
        # If board id is not supported on the device, return None. This is
        # still expected on all current non board id locked release images.
        try:
            version_info = self.get_version_info(self.ACTIVE_BID)
        except error.TestFail, e:
            logging.info(e.message)
            logging.info('Cannot use the version to get the board id')
            return None

        if self.BID_ERROR in version_info[4]:
            raise error.TestError(version_info)
        bid = version_info[4].split()[1]
        return bid if bid != cr50_utils.EMPTY_IMAGE_BID else None


    def get_version(self):
        """Get the RW version"""
        return self.get_active_version_info()[1].strip()


    def using_servo_v4(self):
        """Returns true if the console is being served using servo v4"""
        return 'servo_v4' in self._servo.get_servo_version()


    def using_ccd(self):
        """Returns true if the console is being served using CCD"""
        return 'ccd_cr50' in self._servo.get_servo_version()


    def ccd_is_enabled(self):
        """Return True if ccd is enabled.

        If the test is running through ccd, return the ccd_state value. If
        a flex cable is being used, use the CCD_MODE_L gpio setting to determine
        if Cr50 has ccd enabled.

        Returns:
            'off' or 'on' based on whether the cr50 console is working.
        """
        if self.using_ccd():
            return self._servo.get('ccd_state') == 'on'
        else:
            result = self.send_command_get_output('gpioget',
                    ['(0|1)..CCD_MODE_L'])
            return not bool(int(result[0][1]))


    @servo_v4_command
    def wait_for_ccd_state(self, state, timeout, raise_error):
        """Wait up to timeout seconds for CCD to be 'on' or 'off'
        Args:
            state: a string either 'on' or 'off'.
            timeout: time in seconds to wait
            raise_error: Raise TestFail if the value is state is not reached.

        Raises
            TestFail if ccd never reaches the specified state
        """
        wait_for_enable = state == 'on'
        logging.info("Wait until ccd is '%s'", state)
        value = utils.wait_for_value(self.ccd_is_enabled, wait_for_enable,
                                     timeout_sec=timeout)
        if value != wait_for_enable:
            error_msg = "timed out before detecting ccd '%s'" % state
            if raise_error:
                raise error.TestFail(error_msg)
            logging.warning(error_msg)
        logging.info("ccd is '%s'", state)


    @servo_v4_command
    def wait_for_ccd_disable(self, timeout=60, raise_error=True):
        """Wait for the cr50 console to stop working"""
        self.wait_for_ccd_state('off', timeout, raise_error)


    @servo_v4_command
    def wait_for_ccd_enable(self, timeout=60, raise_error=False):
        """Wait for the cr50 console to start working"""
        self.wait_for_ccd_state('on', timeout, raise_error)


    @servo_v4_command
    def ccd_disable(self, raise_error=True):
        """Change the values of the CC lines to disable CCD"""
        logging.info("disable ccd")
        self._servo.set_nocheck('servo_v4_dts_mode', 'off')
        self.wait_for_ccd_disable(raise_error=raise_error)


    @servo_v4_command
    def ccd_enable(self, raise_error=False):
        """Reenable CCD and reset servo interfaces"""
        logging.info("reenable ccd")
        self._servo.set_nocheck('servo_v4_dts_mode', 'on')
        # If the test is actually running with ccd, reset usb and wait for
        # communication to come up.
        if self.using_ccd():
            self._servo.set_nocheck('power_state', 'ccd_reset')
        self.wait_for_ccd_enable(raise_error=raise_error)


    def _level_change_req_pp(self, level):
        """Returns True if setting the level will require physical presence"""
        testlab_pp = level != 'testlab open' and 'testlab' in level
        open_pp = level == 'open'
        return testlab_pp or open_pp


    def _state_to_bool(self, state):
        """Converts the state string to True or False"""
        # TODO(mruthven): compare to 'on' once servo is up to date in the lab
        return state.lower() in self.ON_STRINGS


    def testlab_is_on(self):
        """Returns True of testlab mode is on"""
        return self._state_to_bool(self._servo.get('cr50_testlab'))


    def set_ccd_testlab(self, state):
        """Set the testlab mode

        Args:
            state: the desired testlab mode string: 'on' or 'off'

        Raises:
            TestFail if testlab mode was not changed
        """
        if self.using_ccd():
            raise error.TestError('Cannot set testlab mode with CCD. Use flex '
                    'cable instead.')

        request_on = self._state_to_bool(state)
        testlab_on = self.testlab_is_on()
        request_str = 'on' if request_on else 'off'

        if testlab_on == request_on:
            logging.info('ccd testlab already set to %s', request_str)
            return

        original_level = self.get_ccd_level()

        # We can only change the testlab mode when the device is open. If
        # testlab mode is already enabled, we can go directly to open using 'ccd
        # testlab open'. This will save 5 minutes, because we can skip the
        # physical presence check.
        if testlab_on:
            self.send_command('ccd testlab open')
        else:
            self.set_ccd_level('open')

        # Set testlab mode
        rv = self.send_command_get_output('ccd testlab %s' % request_str,
                ['.*>'])[0]
        if 'Access Denied' in rv:
            raise error.TestFail("'ccd %s' %s" % (request_str, rv))

        # Press the power button once a second for 15 seconds.
        self.run_pp(self.PP_SHORT)

        self.set_ccd_level(original_level)

        if request_on != self.testlab_is_on():
            raise error.TestFail('Failed to set ccd testlab to %s' % state)


    def get_ccd_level(self):
        """Returns the current ccd privilege level"""
        # TODO(mruthven): delete the part removing the trailing 'ed' once
        # servo is up to date in the lab
        return self._servo.get('cr50_ccd_level').lower().rstrip('ed')


    def set_ccd_level(self, level):
        """Set the Cr50 CCD privilege level.

        Args:
            level: a string of the ccd privilege level: 'open', 'lock', or
                   'unlock'.

        Raises:
            TestFail if the level couldn't be set
        ."""
        # TODO(mruthven): add support for CCD password
        level = level.lower()

        if level == self.get_ccd_level():
            logging.info('CCD privilege level is already %s', level)
            return

        if 'testlab' in level:
            raise error.TestError("Can't change testlab mode using "
                "ccd_set_level")

        testlab_on = self._state_to_bool(self._servo.get('cr50_testlab'))
        req_pp = self._level_change_req_pp(level)
        has_pp = not self.using_ccd()
        dbg_en = 'DBG' in self._servo.get('cr50_version')

        if req_pp and not has_pp:
            raise error.TestError("Can't change privilege level to '%s' "
                "without physical presence." % level)

        if not testlab_on and not has_pp:
            raise error.TestError("Wont change privilege level without "
                "physical presence or testlab mode enabled")

        # Start the unlock process.
        rv = self.send_command_get_output('ccd %s' % level, ['.*>'])[0]
        logging.info(rv)
        if 'Access Denied' in rv:
            raise error.TestFail("'ccd %s' %s" % (level, rv))

        # Press the power button once a second, if we need physical presence.
        if req_pp:
            # DBG images have shorter unlock processes
            self.run_pp(self.PP_SHORT if dbg_en else self.PP_LONG)

        if level != self.get_ccd_level():
            raise error.TestFail('Could not set privilege level to %s' % level)

        logging.info('Successfully set CCD privelege level to %s', level)


    def run_pp(self, unlock_timeout):
        """Press the power button a for unlock_timeout seconds.

        This will press the power button many more times than it needs to be
        pressed. Cr50 doesn't care if you press it too often. It just cares that
        you press the power button at least once within the detect interval.

        For privilege level changes you need to press the power button 5 times
        in the short interval and then 4 times within the long interval.
        Short Interval
        100msec < power button press < 5 seconds
        Long Interval
        60s < power button press < 300s

        For testlab enable/disable you must press the power button 5 times
        spaced between 100msec and 5 seconds apart.
        """
        end_time = time.time() + unlock_timeout

        logging.info('Pressing power button for %ds to unlock the console.',
                     unlock_timeout)
        logging.info('The process should end at %s', time.ctime(end_time))

        # Press the power button once a second to unlock the console.
        while time.time() < end_time:
            self._servo.power_short_press()
            time.sleep(1)


    def gettime(self):
        """Get the current cr50 system time"""
        result = self.send_command_get_output('gettime', [' = (.*) s'])
        return float(result[0][1])


    def servo_v4_supports_dts_mode(self):
        """Returns True if cr50 registers changes in servo v4 dts mode."""
        # This is to test that Cr50 actually recognizes the change in ccd state
        # We cant do that with tests using ccd, because the cr50 communication
        # goes down once ccd is enabled.
        if 'servo_v4_with_servo_micro' != self._servo.get_servo_version():
            return False

        start_val = self._servo.get('servo_v4_dts_mode')
        try:
            # Verify both ccd enable and disable
            self.ccd_disable(raise_error=True)
            self.ccd_enable(raise_error=True)
            rv = True
        except Exception, e:
            logging.info(e)
            rv = False
        self._servo.set_nocheck('servo_v4_dts_mode', start_val)
        self.wait_for_ccd_state(start_val, 60, True)
        logging.info('Test setup does%s support servo DTS mode',
                '' if rv else 'n\'t')
        return rv


    def wait_until_update_is_allowed(self):
        """Wait until cr50 will be able to accept an update.

        Cr50 rejects any attempt to update if it has been less than 60 seconds
        since it last recovered from deep sleep or came up from reboot. This
        will wait until cr50 gettime shows a time greater than 60.
        """
        if self.get_active_version_info()[2]:
            logging.info("Running DBG image. Don't need to wait for update.")
            return
        cr50_time = self.gettime()
        if cr50_time < 60:
            sleep_time = 61 - cr50_time
            logging.info('Cr50 has been up for %ds waiting %ds before update',
                         cr50_time, sleep_time)
            time.sleep(sleep_time)
