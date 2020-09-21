# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import logging
import time
import sys

from multiprocessing import Process
from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.faft.utils import shell_wrapper

class ConnectionError(Exception):
    """Raised on an error of connecting DUT."""
    pass


class _BaseFwBypasser(object):
    """Base class that controls bypass logic for firmware screens."""

    def __init__(self, faft_framework):
        self.servo = faft_framework.servo
        self.faft_config = faft_framework.faft_config
        self.client_host = faft_framework._client


    def bypass_dev_mode(self):
        """Bypass the dev mode firmware logic to boot internal image."""
        raise NotImplementedError


    def bypass_dev_boot_usb(self):
        """Bypass the dev mode firmware logic to boot USB."""
        raise NotImplementedError


    def bypass_rec_mode(self):
        """Bypass the rec mode firmware logic to boot USB."""
        raise NotImplementedError


    def trigger_dev_to_rec(self):
        """Trigger to the rec mode from the dev screen."""
        raise NotImplementedError


    def trigger_rec_to_dev(self):
        """Trigger to the dev mode from the rec screen."""
        raise NotImplementedError


    def trigger_dev_to_normal(self):
        """Trigger to the normal mode from the dev screen."""
        raise NotImplementedError


class _CtrlDBypasser(_BaseFwBypasser):
    """Controls bypass logic via Ctrl-D combo."""

    def bypass_dev_mode(self):
        """Bypass the dev mode firmware logic to boot internal image."""
        time.sleep(self.faft_config.firmware_screen)
        self.servo.ctrl_d()


    def bypass_dev_boot_usb(self):
        """Bypass the dev mode firmware logic to boot USB."""
        time.sleep(self.faft_config.firmware_screen)
        self.servo.ctrl_u()


    def bypass_rec_mode(self):
        """Bypass the rec mode firmware logic to boot USB."""
        self.servo.switch_usbkey('host')
        time.sleep(self.faft_config.usb_plug)
        self.servo.switch_usbkey('dut')
        logging.info('Enabled dut_sees_usb')
        if not self.client_host.ping_wait_up(
                timeout=self.faft_config.delay_reboot_to_ping):
            logging.info('ping timed out, try REC_ON')
            psc = self.servo.get_power_state_controller()
            psc.power_on(psc.REC_ON)


    def trigger_dev_to_rec(self):
        """Trigger to the rec mode from the dev screen."""
        time.sleep(self.faft_config.firmware_screen)

        # Pressing Enter for too long triggers a second key press.
        # Let's press it without delay
        self.servo.enter_key(press_secs=0)

        # For Alex/ZGB, there is a dev warning screen in text mode.
        # Skip it by pressing Ctrl-D.
        if self.faft_config.need_dev_transition:
            time.sleep(self.faft_config.legacy_text_screen)
            self.servo.ctrl_d()


    def trigger_rec_to_dev(self):
        """Trigger to the dev mode from the rec screen."""
        time.sleep(self.faft_config.firmware_screen)
        self.servo.ctrl_d()
        time.sleep(self.faft_config.confirm_screen)
        if self.faft_config.rec_button_dev_switch:
            logging.info('RECOVERY button pressed to switch to dev mode')
            self.servo.toggle_recovery_switch()
        else:
            logging.info('ENTER pressed to switch to dev mode')
            self.servo.enter_key()


    def trigger_dev_to_normal(self):
        """Trigger to the normal mode from the dev screen."""
        time.sleep(self.faft_config.firmware_screen)
        self.servo.enter_key()
        time.sleep(self.faft_config.confirm_screen)
        self.servo.enter_key()


class _JetstreamBypasser(_BaseFwBypasser):
    """Controls bypass logic of Jetstream devices."""

    def bypass_dev_mode(self):
        """Bypass the dev mode firmware logic to boot internal image."""
        # Jetstream does nothing to bypass.
        pass


    def bypass_dev_boot_usb(self):
        """Bypass the dev mode firmware logic to boot USB."""
        self.servo.switch_usbkey('dut')
        time.sleep(self.faft_config.firmware_screen)
        self.servo.toggle_development_switch()


    def bypass_rec_mode(self):
        """Bypass the rec mode firmware logic to boot USB."""
        self.servo.switch_usbkey('host')
        time.sleep(self.faft_config.usb_plug)
        self.servo.switch_usbkey('dut')
        if not self.client_host.ping_wait_up(
                timeout=self.faft_config.delay_reboot_to_ping):
            psc = self.servo.get_power_state_controller()
            psc.power_on(psc.REC_ON)


    def trigger_dev_to_rec(self):
        """Trigger to the rec mode from the dev screen."""
        # Jetstream does not have this triggering logic.
        raise NotImplementedError


    def trigger_rec_to_dev(self):
        """Trigger to the dev mode from the rec screen."""
        self.servo.disable_development_mode()
        time.sleep(self.faft_config.firmware_screen)
        self.servo.toggle_development_switch()


    def trigger_dev_to_normal(self):
        """Trigger to the normal mode from the dev screen."""
        # Jetstream does not have this triggering logic.
        raise NotImplementedError


class _TabletDetachableBypasser(_BaseFwBypasser):
    """Controls bypass logic of tablet/ detachable chromebook devices."""

    def set_button(self, button, duration, info):
        """Helper method that sets the button hold time for UI selections"""
        self.servo.set_nocheck(button, duration)
        time.sleep(self.faft_config.confirm_screen)
        logging.info(info)


    def bypass_dev_boot_usb(self):
        """Bypass the dev mode firmware logic to boot USB.

        On tablets/ detachables, recovery entered by pressing pwr, vol up
        & vol down buttons for 10s.
           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in DEV screen:
                 Boot legacy BIOS
                 Boot USB image
                 Boot developer image*
                 Cancel
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        self.trigger_dev_screen()
        time.sleep(self.faft_config.firmware_screen)
        self.set_button('volume_up_hold', 100, ('Selecting power as'
                        ' enter key to select Boot USB Image'))
        self.servo.power_short_press()


    def bypass_rec_mode(self):
        """Bypass the rec mode firmware logic to boot USB."""
        self.servo.switch_usbkey('host')
        time.sleep(self.faft_config.usb_plug)
        self.servo.switch_usbkey('dut')
        logging.info('Enabled dut_sees_usb')
        if not self.client_host.ping_wait_up(
                timeout=self.faft_config.delay_reboot_to_ping):
            logging.info('ping timed out, try REC_ON')
            psc = self.servo.get_power_state_controller()
            psc.power_on(psc.REC_ON)


    def bypass_dev_mode(self):
        """Bypass the dev mode firmware logic to boot internal image

        On tablets/ detachables, recovery entered by pressing pwr, vol up
        & vol down buttons for 10s.
           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in DEV screen:
                 Boot legacy BIOS
                 Boot USB image
                 Boot developer image*
                 Cancel
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        self.trigger_dev_screen()
        time.sleep(self.faft_config.firmware_screen)
        logging.info('Selecting power as enter key to select '
                     'Boot Developer Image')
        self.servo.power_short_press()


    def trigger_dev_screen(self):
        """Helper method that transitions from DEVELOPER WARNING to DEV screen

           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in DEV screen:
                 Boot legacy BIOS
                 Boot USB image
                 Boot developer image*
                 Cancel
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        time.sleep(self.faft_config.firmware_screen)
        self.servo.set_nocheck('volume_up_hold', 100)
        time.sleep(self.faft_config.confirm_screen)
        self.servo.set_nocheck('volume_up_hold', 100)
        time.sleep(self.faft_config.confirm_screen)
        self.set_button('volume_up_hold', 100, ('Selecting power '
                        'as enter key to select Developer Options'))
        self.servo.power_short_press()


    def trigger_rec_to_dev(self):
        """Trigger to the dev mode from the rec screen using vol up button.

        On tablets/ detachables, recovery entered by pressing pwr, vol up
        & vol down buttons for 10s. TO_DEV screen is entered by pressing
        vol up & vol down buttons together on the INSERT screen.
           Menu options seen in TO_DEV screen:
                 Confirm enabling developer mode
                 Cancel*
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        time.sleep(self.faft_config.firmware_screen)
        self.set_button('volume_up_down_hold', 100, ('Enter Recovery Menu.'))
        time.sleep(self.faft_config.confirm_screen)
        self.set_button('volume_up_hold', 100, ('Selecting power as '
                        'enter key to select Confirm Enabling Developer Mode'))
        self.servo.power_short_press()
        time.sleep(self.faft_config.firmware_screen)


    def trigger_dev_to_normal(self):
        """Trigger to the normal mode from the dev screen.

           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in TO_NORM screen:
                 Confirm Enabling Verified Boot*
                 Cancel
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        time.sleep(self.faft_config.firmware_screen)
        self.set_button('volume_up_hold', 100, ('Selecting '
                        'Enable Root Verification using pwr '
                        'button to enter TO_NORM screen'))
        self.servo.power_short_press()
        logging.info('Transitioning from DEV to TO_NORM screen.')
        time.sleep(self.faft_config.firmware_screen)
        logging.info('Selecting Confirm Enabling Verified '
                        'Boot using pwr button in '
                        'TO_NORM screen')
        self.servo.power_short_press()

    def trigger_dev_to_rec(self):
        """Trigger to the TO_NORM screen from the dev screen.
           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in TO_NORM screen:
                 Confirm Enabling Verified Boot*
                 Cancel
                 Power off
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        time.sleep(self.faft_config.firmware_screen)
        self.set_button('volume_up_hold', 100, ('Selecting '
                        'Enable Root Verification using pwr '
                        'button to enter TO_NORM screen'))
        self.servo.power_short_press()
        logging.info('Transitioning from DEV to TO_NORM screen.')
        time.sleep(self.faft_config.firmware_screen)

        # In firmware_FwScreenPressPower, test will power off the DUT using
        # Power button in second screen (TO_NORM screen) so scrolling to
        # Power-off is necessary in this case. Hence scroll to Power-off as
        # a generic action and wait for next action of either Lid close or
        # power button press.
        self.servo.set_nocheck('volume_down_hold', 100)
        time.sleep(self.faft_config.confirm_screen)
        self.servo.set_nocheck('volume_down_hold', 100)
        time.sleep(self.faft_config.confirm_screen)

def _create_fw_bypasser(faft_framework):
    """Creates a proper firmware bypasser.

    @param faft_framework: The main FAFT framework object.
    """
    bypasser_type = faft_framework.faft_config.fw_bypasser_type
    if bypasser_type == 'ctrl_d_bypasser':
        logging.info('Create a CtrlDBypasser')
        return _CtrlDBypasser(faft_framework)
    elif bypasser_type == 'jetstream_bypasser':
        logging.info('Create a JetstreamBypasser')
        return _JetstreamBypasser(faft_framework)
    elif bypasser_type == 'ryu_bypasser':
        # FIXME Create an RyuBypasser
        logging.info('Create a CtrlDBypasser')
        return _CtrlDBypasser(faft_framework)
    elif bypasser_type == 'tablet_detachable_bypasser':
        logging.info('Create a TabletDetachableBypasser')
        return _TabletDetachableBypasser(faft_framework)
    else:
        raise NotImplementedError('Not supported fw_bypasser_type: %s',
                                  bypasser_type)


class _BaseModeSwitcher(object):
    """Base class that controls firmware mode switching."""

    def __init__(self, faft_framework):
        self.faft_framework = faft_framework
        self.client_host = faft_framework._client
        self.faft_client = faft_framework.faft_client
        self.servo = faft_framework.servo
        self.faft_config = faft_framework.faft_config
        self.checkers = faft_framework.checkers
        self.bypasser = _create_fw_bypasser(faft_framework)
        self._backup_mode = None


    def setup_mode(self, mode):
        """Setup for the requested mode.

        It makes sure the system in the requested mode. If not, it tries to
        do so.

        @param mode: A string of mode, one of 'normal', 'dev', or 'rec'.
        @raise TestFail: If the system not switched to expected mode after
                         reboot_to_mode.

        """
        if not self.checkers.mode_checker(mode):
            logging.info('System not in expected %s mode. Reboot into it.',
                         mode)
            if self._backup_mode is None:
                # Only resume to normal/dev mode after test, not recovery.
                self._backup_mode = 'dev' if mode == 'normal' else 'normal'
            self.reboot_to_mode(mode)
            if not self.checkers.mode_checker(mode):
                raise error.TestFail('System not switched to expected %s'
                        ' mode after setup_mode.' % mode)

    def restore_mode(self):
        """Restores original dev mode status if it has changed.

        @raise TestFail: If the system not restored to expected mode.
        """
        if (self._backup_mode is not None and
            not self.checkers.mode_checker(self._backup_mode)):
            self.reboot_to_mode(self._backup_mode)
            if not self.checkers.mode_checker(self._backup_mode):
                raise error.TestFail('System not restored to expected %s'
                        ' mode in cleanup.' % self._backup_mode)



    def reboot_to_mode(self, to_mode, from_mode=None, sync_before_boot=True,
                       wait_for_dut_up=True):
        """Reboot and execute the mode switching sequence.

        @param to_mode: The target mode, one of 'normal', 'dev', or 'rec'.
        @param from_mode: The original mode, optional, one of 'normal, 'dev',
                          or 'rec'.
        @param sync_before_boot: True to sync to disk before booting.
        @param wait_for_dut_up: True to wait DUT online again. False to do the
                                reboot and mode switching sequence only and may
                                need more operations to pass the firmware
                                screen.
        """
        logging.info('-[ModeSwitcher]-[ start reboot_to_mode(%r, %r, %r) ]-',
                     to_mode, from_mode, wait_for_dut_up)
        if sync_before_boot:
            self.faft_framework.blocking_sync()
        if to_mode == 'rec':
            self._enable_rec_mode_and_reboot(usb_state='dut')
            if wait_for_dut_up:
                self.wait_for_client()

        elif to_mode == 'dev':
            self._enable_dev_mode_and_reboot()
            if wait_for_dut_up:
                self.bypass_dev_mode()
                self.wait_for_client()

        elif to_mode == 'normal':
            self._enable_normal_mode_and_reboot()
            if wait_for_dut_up:
                self.wait_for_client()

        else:
            raise NotImplementedError(
                    'Not supported mode switching from %s to %s' %
                     (str(from_mode), to_mode))
        logging.info('-[ModeSwitcher]-[ end reboot_to_mode(%r, %r, %r) ]-',
                     to_mode, from_mode, wait_for_dut_up)

    def simple_reboot(self, reboot_type='warm', sync_before_boot=True):
        """Simple reboot method

        Just reboot the DUT using either cold or warm reset.  Does not wait for
        DUT to come back online.  Will wait for test to handle this.

        @param reboot_type: A string of reboot type, 'warm' or 'cold'.
                            If reboot_type != warm/cold, raise exception.
        @param sync_before_boot: True to sync to disk before booting.
                                 If sync_before_boot=False, DUT offline before
                                 calling mode_aware_reboot.
        """
        if reboot_type == 'warm':
            reboot_method = self.servo.get_power_state_controller().warm_reset
        elif reboot_type == 'cold':
            reboot_method = self.servo.get_power_state_controller().reset
        else:
            raise NotImplementedError('Not supported reboot_type: %s',
                                      reboot_type)
        if sync_before_boot:
            boot_id = self.faft_framework.get_bootid()
            self.faft_framework.blocking_sync()
        logging.info("-[ModeSwitcher]-[ start simple_reboot(%r) ]-",
                     reboot_type)
        reboot_method()
        if sync_before_boot:
            self.wait_for_client_offline(orig_boot_id=boot_id)
        logging.info("-[ModeSwitcher]-[ end simple_reboot(%r) ]-",
                     reboot_type)

    def mode_aware_reboot(self, reboot_type=None, reboot_method=None,
                          sync_before_boot=True, wait_for_dut_up=True):
        """Uses a mode-aware way to reboot DUT.

        For example, if DUT is in dev mode, it requires pressing Ctrl-D to
        bypass the developer screen.

        @param reboot_type: A string of reboot type, one of 'warm', 'cold', or
                            'custom'. Default is a warm reboot.
        @param reboot_method: A custom method to do the reboot. Only use it if
                              reboot_type='custom'.
        @param sync_before_boot: True to sync to disk before booting.
                                 If sync_before_boot=False, DUT offline before
                                 calling mode_aware_reboot.
        @param wait_for_dut_up: True to wait DUT online again. False to do the
                                reboot only.
        """
        if reboot_type is None or reboot_type == 'warm':
            reboot_method = self.servo.get_power_state_controller().warm_reset
        elif reboot_type == 'cold':
            reboot_method = self.servo.get_power_state_controller().reset
        elif reboot_type != 'custom':
            raise NotImplementedError('Not supported reboot_type: %s',
                                      reboot_type)

        logging.info("-[ModeSwitcher]-[ start mode_aware_reboot(%r, %s, ..) ]-",
                     reboot_type, reboot_method.__name__)
        is_dev = is_rec = is_devsw_boot = False
        if sync_before_boot:
            is_dev = self.checkers.mode_checker('dev')
            is_rec = self.checkers.mode_checker('rec')
            is_devsw_boot = self.checkers.crossystem_checker(
                                               {'devsw_boot': '1'}, True)
            boot_id = self.faft_framework.get_bootid()
            self.faft_framework.blocking_sync()
        if is_rec:
            logging.info("-[mode_aware_reboot]-[ is_rec=%s is_dev_switch=%s ]-",
                         is_rec, is_devsw_boot)
        else:
            logging.info("-[mode_aware_reboot]-[ is_dev=%s ]-", is_dev)
        reboot_method()
        if sync_before_boot:
            self.wait_for_client_offline(orig_boot_id=boot_id)
        # Encapsulating the behavior of skipping dev firmware screen,
        # hitting ctrl-D
        # Note that if booting from recovery mode, we can predict the next
        # boot based on the developer switch position at boot (devsw_boot).
        # If devsw_boot is True, we will call bypass_dev_mode after reboot.
        if is_dev or is_devsw_boot:
            self.bypass_dev_mode()
        if wait_for_dut_up:
            self.wait_for_client()
        logging.info("-[ModeSwitcher]-[ end mode_aware_reboot(%r, %s, ..) ]-",
                     reboot_type, reboot_method.__name__)


    def _enable_rec_mode_and_reboot(self, usb_state=None):
        """Switch to rec mode and reboot.

        This method emulates the behavior of the old physical recovery switch,
        i.e. switch ON + reboot + switch OFF, and the new keyboard controlled
        recovery mode, i.e. just press Power + Esc + Refresh.

        @param usb_state: A string, one of 'dut', 'host', or 'off'.
        """
        psc = self.servo.get_power_state_controller()
        psc.power_off()
        if usb_state:
            self.servo.switch_usbkey(usb_state)
        psc.power_on(psc.REC_ON)


    def _disable_rec_mode_and_reboot(self, usb_state=None):
        """Disable the rec mode and reboot.

        It is achieved by calling power state controller to do a normal
        power on.
        """
        psc = self.servo.get_power_state_controller()
        psc.power_off()
        time.sleep(self.faft_config.ec_boot_to_pwr_button)
        psc.power_on(psc.REC_OFF)


    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot."""
        raise NotImplementedError


    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot."""
        raise NotImplementedError


    # Redirects the following methods to FwBypasser
    def bypass_dev_mode(self):
        """Bypass the dev mode firmware logic to boot internal image."""
        logging.info("-[bypass_dev_mode]-")
        self.bypasser.bypass_dev_mode()


    def bypass_dev_boot_usb(self):
        """Bypass the dev mode firmware logic to boot USB."""
        logging.info("-[bypass_dev_boot_usb]-")
        self.bypasser.bypass_dev_boot_usb()


    def bypass_rec_mode(self):
        """Bypass the rec mode firmware logic to boot USB."""
        logging.info("-[bypass_rec_mode]-")
        self.bypasser.bypass_rec_mode()


    def trigger_dev_to_rec(self):
        """Trigger to the rec mode from the dev screen."""
        self.bypasser.trigger_dev_to_rec()


    def trigger_rec_to_dev(self):
        """Trigger to the dev mode from the rec screen."""
        self.bypasser.trigger_rec_to_dev()


    def trigger_dev_to_normal(self):
        """Trigger to the normal mode from the dev screen."""
        self.bypasser.trigger_dev_to_normal()


    def wait_for_client(self, timeout=180):
        """Wait for the client to come back online.

        New remote processes will be launched if their used flags are enabled.

        @param timeout: Time in seconds to wait for the client SSH daemon to
                        come up.
        @raise ConnectionError: Failed to connect DUT.
        """
        logging.info("-[FAFT]-[ start wait_for_client ]---")
        # Wait for the system to respond to ping before attempting ssh
        if not self.client_host.ping_wait_up(timeout):
            logging.warning("-[FAFT]-[ system did not respond to ping ]")
        if self.client_host.wait_up(timeout):
            # Check the FAFT client is avaiable.
            self.faft_client.system.is_available()
            # Stop update-engine as it may change firmware/kernel.
            self.faft_framework.faft_client.updater.stop_daemon()
        else:
            logging.error('wait_for_client() timed out.')
            raise ConnectionError('DUT is still down unexpectedly')
        logging.info("-[FAFT]-[ end wait_for_client ]-----")


    def wait_for_client_offline(self, timeout=60, orig_boot_id=None):
        """Wait for the client to come offline.

        @param timeout: Time in seconds to wait the client to come offline.
        @param orig_boot_id: A string containing the original boot id.
        @raise ConnectionError: Failed to wait DUT offline.
        """
        # When running against panther, we see that sometimes
        # ping_wait_down() does not work correctly. There needs to
        # be some investigation to the root cause.
        # If we sleep for 120s before running get_boot_id(), it
        # does succeed. But if we change this to ping_wait_down()
        # there are implications on the wait time when running
        # commands at the fw screens.
        if not self.client_host.ping_wait_down(timeout):
            if orig_boot_id and self.client_host.get_boot_id() != orig_boot_id:
                logging.warn('Reboot done very quickly.')
                return
            raise ConnectionError('DUT is still up unexpectedly')


class _PhysicalButtonSwitcher(_BaseModeSwitcher):
    """Class that switches firmware mode via physical button."""

    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot."""
        self.servo.enable_development_mode()
        self.faft_client.system.run_shell_command(
                'chromeos-firmwareupdate --mode todev && reboot')


    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot."""
        self.servo.disable_development_mode()
        self.faft_client.system.run_shell_command(
                'chromeos-firmwareupdate --mode tonormal && reboot')


class _KeyboardDevSwitcher(_BaseModeSwitcher):
    """Class that switches firmware mode via keyboard combo."""

    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot."""
        logging.info("Enabling keyboard controlled developer mode")
        # Rebooting EC with rec mode on. Should power on AP.
        # Plug out USB disk for preventing recovery boot without warning
        self._enable_rec_mode_and_reboot(usb_state='host')
        self.wait_for_client_offline()
        self.bypasser.trigger_rec_to_dev()


    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot."""
        logging.info("Disabling keyboard controlled developer mode")
        self._disable_rec_mode_and_reboot()
        self.wait_for_client_offline()
        self.bypasser.trigger_dev_to_normal()


class _JetstreamSwitcher(_BaseModeSwitcher):
    """Class that switches firmware mode in Jetstream devices."""

    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot."""
        logging.info("Enabling Jetstream developer mode")
        self._enable_rec_mode_and_reboot(usb_state='host')
        self.wait_for_client_offline()
        self.bypasser.trigger_rec_to_dev()


    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot."""
        logging.info("Disabling Jetstream developer mode")
        self.servo.disable_development_mode()
        self._enable_rec_mode_and_reboot(usb_state='host')
        time.sleep(self.faft_config.firmware_screen)
        self._disable_rec_mode_and_reboot(usb_state='host')


class _TabletDetachableSwitcher(_BaseModeSwitcher):
    """Class that switches fw mode in tablets/detachables with fw menu UI."""

    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot.

        On tablets/ detachables, recovery entered by pressing pwr, vol up
        & vol down buttons for 10s.
           Menu options seen in RECOVERY screen:
                 Enable Developer Mode
                 Show Debug Info
                 Power off*
                 Language
        """
        logging.info('Enabling tablets/detachable recovery mode')
        self._enable_rec_mode_and_reboot(usb_state='host')
        self.wait_for_client_offline()
        self.bypasser.trigger_rec_to_dev()


    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot.

           Menu options seen in DEVELOPER WARNING screen:
                 Developer Options
                 Show Debug Info
                 Enable Root Verification
                 Power Off*
                 Language
           Menu options seen in TO_NORM screen:
                 Confirm Enabling Verified Boot
                 Cancel
                 Power off*
                 Language
        Vol up button selects previous item, vol down button selects
        next item and pwr button selects current activated item.
        """
        self._disable_rec_mode_and_reboot()
        self.wait_for_client_offline()
        self.bypasser.trigger_dev_to_normal()


class _RyuSwitcher(_BaseModeSwitcher):
    """Class that switches firmware mode via physical button."""

    FASTBOOT_OEM_DELAY = 10
    RECOVERY_TIMEOUT = 2400
    RECOVERY_SETUP = 60
    ANDROID_BOOTUP = 600
    FWTOOL_STARTUP_DELAY = 30

    def wait_for_client(self, timeout=180):
        """Wait for the client to come back online.

        New remote processes will be launched if their used flags are enabled.

        @param timeout: Time in seconds to wait for the client SSH daemon to
                        come up.
        @raise ConnectionError: Failed to connect DUT.
        """
        if not self.faft_client.system.wait_for_client(timeout):
            raise ConnectionError('DUT is still down unexpectedly')

        # there's a conflict between fwtool and crossystem trying to access
        # the nvram after the OS boots up.  Temporarily put a hard wait of
        # 30 seconds to try to wait for fwtool to finish up.
        time.sleep(self.FWTOOL_STARTUP_DELAY)


    def wait_for_client_offline(self, timeout=60, orig_boot_id=None):
        """Wait for the client to come offline.

        @param timeout: Time in seconds to wait the client to come offline.
        @param orig_boot_id: A string containing the original boot id.
        @raise ConnectionError: Failed to wait DUT offline.
        """
        # TODO: Add a way to check orig_boot_id
        if not self.faft_client.system.wait_for_client_offline(timeout):
            raise ConnectionError('DUT is still up unexpectedly')

    def print_recovery_warning(self):
        """Print recovery warning"""
        logging.info("***")
        logging.info("*** Entering recovery mode.  This may take awhile ***")
        logging.info("***")
        # wait a minute for DUT to get settled into wipe stage
        time.sleep(self.RECOVERY_SETUP)

    def is_fastboot_mode(self):
        """Return True if DUT in fastboot mode, False otherwise"""
        result = self.faft_client.host.run_shell_command_get_output(
            'fastboot devices')
        if not result:
            return False
        else:
            return True

    def wait_for_client_fastboot(self, timeout=30):
        """Wait for the client to come online in fastboot mode

        @param timeout: Time in seconds to wait the client
        @raise ConnectionError: Failed to wait DUT offline.
        """
        utils.wait_for_value(self.is_fastboot_mode, True, timeout_sec=timeout)

    def _run_cmd(self, args):
        """Wrapper for run_shell_command

        For Process creation
        """
        return self.faft_client.host.run_shell_command(args)

    def _enable_dev_mode_and_reboot(self):
        """Switch to developer mode and reboot."""
        logging.info("Entering RyuSwitcher: _enable_dev_mode_and_reboot")
        try:
            self.faft_client.system.run_shell_command('reboot bootloader')
            self.wait_for_client_fastboot()

            process = Process(
                target=self._run_cmd,
                args=('fastboot oem unlock',))
            process.start()

            # need a slight delay to give the ap time to get into valid state
            time.sleep(self.FASTBOOT_OEM_DELAY)
            self.servo.power_key(self.faft_config.hold_pwr_button_poweron)
            process.join()

            self.print_recovery_warning()
            self.wait_for_client_fastboot(self.RECOVERY_TIMEOUT)
            self.faft_client.host.run_shell_command('fastboot continue')
            self.wait_for_client(self.ANDROID_BOOTUP)

        # need to reset DUT into clean state
        except shell_wrapper.ShellError:
            raise error.TestError('Error executing shell command')
        except ConnectionError:
            raise error.TestError('Timed out waiting for DUT to exit recovery')
        except:
            raise error.TestError('Unexpected Exception: %s' % sys.exc_info()[0])
        logging.info("Exiting RyuSwitcher: _enable_dev_mode_and_reboot")

    def _enable_normal_mode_and_reboot(self):
        """Switch to normal mode and reboot."""
        try:
            self.faft_client.system.run_shell_command('reboot bootloader')
            self.wait_for_client_fastboot()

            process = Process(
                target=self._run_cmd,
                args=('fastboot oem lock',))
            process.start()

            # need a slight delay to give the ap time to get into valid state
            time.sleep(self.FASTBOOT_OEM_DELAY)
            self.servo.power_key(self.faft_config.hold_pwr_button_poweron)
            process.join()

            self.print_recovery_warning()
            self.wait_for_client_fastboot(self.RECOVERY_TIMEOUT)
            self.faft_client.host.run_shell_command('fastboot continue')
            self.wait_for_client(self.ANDROID_BOOTUP)

        # need to reset DUT into clean state
        except shell_wrapper.ShellError:
            raise error.TestError('Error executing shell command')
        except ConnectionError:
            raise error.TestError('Timed out waiting for DUT to exit recovery')
        except:
            raise error.TestError('Unexpected Exception: %s' % sys.exc_info()[0])
        logging.info("Exiting RyuSwitcher: _enable_normal_mode_and_reboot")

def create_mode_switcher(faft_framework):
    """Creates a proper mode switcher.

    @param faft_framework: The main FAFT framework object.
    """
    switcher_type = faft_framework.faft_config.mode_switcher_type
    if switcher_type == 'physical_button_switcher':
        logging.info('Create a PhysicalButtonSwitcher')
        return _PhysicalButtonSwitcher(faft_framework)
    elif switcher_type == 'keyboard_dev_switcher':
        logging.info('Create a KeyboardDevSwitcher')
        return _KeyboardDevSwitcher(faft_framework)
    elif switcher_type == 'jetstream_switcher':
        logging.info('Create a JetstreamSwitcher')
        return _JetstreamSwitcher(faft_framework)
    elif switcher_type == 'ryu_switcher':
        logging.info('Create a RyuSwitcher')
        return _RyuSwitcher(faft_framework)
    elif switcher_type == 'tablet_detachable_switcher':
        logging.info('Create a TabletDetachableSwitcher')
        return _TabletDetachableSwitcher(faft_framework)
    else:
        raise NotImplementedError('Not supported mode_switcher_type: %s',
                                  switcher_type)
