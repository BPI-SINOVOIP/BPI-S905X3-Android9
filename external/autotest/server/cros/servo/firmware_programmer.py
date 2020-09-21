# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A utility to program Chrome OS devices' firmware using servo.

This utility expects the DUT to be connected to a servo device. This allows us
to put the DUT into the required state and to actually program the DUT's
firmware using FTDI, USB and/or serial interfaces provided by servo.

Servo state is preserved across the programming process.
"""

import glob
import logging
import os
import re
import site
import time
import xml.etree.ElementTree

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.faft.config.config import Config as FAFTConfig


# Number of seconds for program EC/BIOS to time out.
FIRMWARE_PROGRAM_TIMEOUT_SEC = 600

class ProgrammerError(Exception):
    """Local exception class wrapper."""
    pass


class _BaseProgrammer(object):
    """Class implementing base programmer services.

    Private attributes:
      _servo: a servo object controlling the servo device
      _servo_host: a host object running commands like 'flashrom'
      _servo_prog_state: a tuple of strings of "<control>:<value>" pairs,
                         listing servo controls and their required values for
                         programming
      _servo_prog_state_delay: time in second to wait after changing servo
                               controls for programming.
      _servo_saved_state: a list of the same elements as _servo_prog_state,
                          those which need to be restored after programming
      _program_cmd: a string, the shell command to run on the servo host
                    to actually program the firmware. Dependent on
                    firmware/hardware type, set by subclasses.
    """

    def __init__(self, servo, req_list, servo_host=None):
        """Base constructor.
        @param servo: a servo object controlling the servo device
        @param req_list: a list of strings, names of the utilities required
                         to be in the path for the programmer to succeed
        @param servo_host: a host object to execute commands. Default to None,
                           using the host object from the above servo object
        """
        self._servo = servo
        self._servo_prog_state = ()
        self._servo_prog_state_delay = 0
        self._servo_saved_state = []
        self._program_cmd = ''
        self._servo_host = servo_host
        if self._servo_host is None:
            self._servo_host = self._servo._servo_host

        try:
            self._servo_host.run('which %s' % ' '.join(req_list))
        except error.AutoservRunError:
            # TODO: We turn this exception into a warn since the fw programmer
            # is not working right now, and some systems do not package the
            # required utilities its checking for.
            # We should reinstate this exception once the programmer is working
            # to indicate the missing utilities earlier in the test cycle.
            # Bug chromium:371011 filed to track this.
            logging.warn("Ignoring exception when verify required bins : %s",
                         ' '.join(req_list))


    def _set_servo_state(self):
        """Set servo for programming, while saving the current state."""
        logging.debug("Setting servo state for programming")
        for item in self._servo_prog_state:
            key, value = item.split(':')
            try:
                present = self._servo.get(key)
            except error.TestFail:
                logging.warn('Missing servo control: %s', key)
                continue
            if present != value:
                self._servo_saved_state.append('%s:%s' % (key, present))
            self._servo.set(key, value)
        time.sleep(self._servo_prog_state_delay)


    def _restore_servo_state(self):
        """Restore previously saved servo state."""
        logging.debug("Restoring servo state after programming")
        self._servo_saved_state.reverse()  # Do it in the reverse order.
        for item in self._servo_saved_state:
            key, value = item.split(':')
            self._servo.set(key, value)


    def program(self):
        """Program the firmware as configured by a subclass."""
        self._set_servo_state()
        try:
            logging.debug("Programmer command: %s", self._program_cmd)
            self._servo_host.run(self._program_cmd,
                                 timeout=FIRMWARE_PROGRAM_TIMEOUT_SEC)
        finally:
            self._restore_servo_state()


class FlashromProgrammer(_BaseProgrammer):
    """Class for programming AP flashrom."""

    def __init__(self, servo, keep_ro=False):
        """Configure required servo state.

        @param servo: a servo object controlling the servo device
        @param keep_ro: True to keep the RO portion unchanged
        """
        super(FlashromProgrammer, self).__init__(servo, ['flashrom',])
        self._keep_ro = keep_ro
        self._fw_path = None
        self._tmp_path = '/tmp'
        self._fw_main = os.path.join(self._tmp_path, 'fw_main')
        self._wp_ro = os.path.join(self._tmp_path, 'wp_ro')
        self._ro_vpd = os.path.join(self._tmp_path, 'ro_vpd')
        self._rw_vpd = os.path.join(self._tmp_path, 'rw_vpd')
        self._gbb = os.path.join(self._tmp_path, 'gbb')
        self._servo_version = self._servo.get_servo_version()
        self._servo_serials = self._servo._server.get_servo_serials()


    def program(self):
        """Program the firmware but preserve VPD and HWID."""
        assert self._fw_path is not None
        self._set_servo_state()
        try:
            wp_ro_section = [('WP_RO', self._wp_ro)]
            rw_vpd_section = [('RW_VPD', self._rw_vpd)]
            ro_vpd_section = [('RO_VPD', self._ro_vpd)]
            gbb_section = [('GBB', self._gbb)]
            if self._keep_ro:
                # Keep the whole RO portion
                preserved_sections = wp_ro_section + rw_vpd_section
            else:
                preserved_sections = ro_vpd_section + rw_vpd_section

            servo_v2_programmer = 'ft2232_spi:type=servo-v2'
            servo_v3_programmer = 'linux_spi'
            servo_v4_with_micro_programmer = 'raiden_spi'
            servo_v4_with_ccd_programmer = 'raiden_debug_spi:target=AP'
            if self._servo_version == 'servo_v2':
                programmer = servo_v2_programmer
                servo_serial = self._servo_serials.get('main')
                if servo_serial:
                    programmer += ',serial=%s' % servo_serial
            elif self._servo_version == 'servo_v3':
                programmer = servo_v3_programmer
            elif self._servo_version == 'servo_v4_with_servo_micro':
                # When a uServo is connected to a DUT with CCD support, the
                # firmware programmer will always use the uServo to program.
                servo_micro_serial = self._servo_serials.get('servo_micro')
                programmer = servo_v4_with_micro_programmer
                programmer += ',serial=%s' % servo_micro_serial
            elif self._servo_version == 'servo_v4_with_ccd_cr50':
                ccd_serial = self._servo_serials.get('ccd')
                programmer = servo_v4_with_ccd_programmer
                programmer += ',serial=%s' % ccd_serial
            else:
                raise Exception('Servo version %s is not supported.' %
                                self._servo_version)
            # Save needed sections from current firmware
            for section in preserved_sections + gbb_section:
                self._servo_host.run(' '.join([
                    'flashrom', '-V', '-p', programmer,
                    '-r', self._fw_main, '-i', '%s:%s' % section]),
                    timeout=FIRMWARE_PROGRAM_TIMEOUT_SEC)

            # Pack the saved VPD into new firmware
            self._servo_host.run('cp %s %s' % (self._fw_path, self._fw_main))
            img_size = self._servo_host.run_output(
                    "stat -c '%%s' %s" % self._fw_main)
            pack_cmd = ['flashrom',
                    '-p', 'dummy:image=%s,size=%s,emulate=VARIABLE_SIZE' % (
                        self._fw_main, img_size),
                    '-w', self._fw_main]
            for section in preserved_sections:
                pack_cmd.extend(['-i', '%s:%s' % section])
            self._servo_host.run(' '.join(pack_cmd),
                                 timeout=FIRMWARE_PROGRAM_TIMEOUT_SEC)

            # HWID is inside the RO portion. Don't preserve HWID if we keep RO.
            if not self._keep_ro:
                # Read original HWID. The output format is:
                #    hardware_id: RAMBI TEST A_A 0128
                gbb_hwid_output = self._servo_host.run_output(
                        'gbb_utility -g --hwid %s' % self._gbb)
                original_hwid = gbb_hwid_output.split(':', 1)[1].strip()

                # Write HWID to new firmware
                self._servo_host.run("gbb_utility -s --hwid='%s' %s" %
                        (original_hwid, self._fw_main))

            # Flash the new firmware
            self._servo_host.run(' '.join([
                    'flashrom', '-V', '-p', programmer,
                    '-w', self._fw_main]), timeout=FIRMWARE_PROGRAM_TIMEOUT_SEC)
        finally:
            self._servo.get_power_state_controller().reset()
            self._restore_servo_state()


    def prepare_programmer(self, path):
        """Prepare programmer for programming.

        @param path: a string, name of the file containing the firmware image.
        """
        self._fw_path = path
        # CCD takes care holding AP/EC. Don't need the following steps.
        if self._servo_version != 'servo_v4_with_ccd_cr50':
            faft_config = FAFTConfig(self._servo.get_board())
            self._servo_prog_state_delay = faft_config.servo_prog_state_delay
            self._servo_prog_state = (
                'spi2_vref:%s' % faft_config.spi_voltage,
                'spi2_buf_en:on',
                'spi2_buf_on_flex_en:on',
                'spi_hold:off',
                'cold_reset:on',
                'usbpd_reset:on',
                )


class FlashECProgrammer(_BaseProgrammer):
    """Class for programming AP flashrom."""

    def __init__(self, servo, host=None, ec_chip=None):
        """Configure required servo state.

        @param servo: a servo object controlling the servo device
        @param host: a host object to execute commands. Default to None,
                     using the host object from the above servo object, i.e.
                     a servo host. A CrOS host object can be passed here
                     such that it executes commands on the CrOS device.
        @param ec_chip: a string of EC chip. Default to None, using the
                        EC chip name reported by servo, the primary EC.
                        Can pass a different chip name, for the case of
                        the base EC.

        """
        super(FlashECProgrammer, self).__init__(servo, ['flash_ec'], host)
        self._servo_version = self._servo.get_servo_version()
        if ec_chip is None:
            self._ec_chip = servo.get('ec_chip')
        else:
            self._ec_chip = ec_chip

    def prepare_programmer(self, image):
        """Prepare programmer for programming.

        @param image: string with the location of the image file
        """
        # Get the port of servod. flash_ec may use it to talk to servod.
        port = self._servo._servo_host.servo_port
        self._program_cmd = ('flash_ec --chip=%s --image=%s --port=%d' %
                             (self._ec_chip, image, port))
        if self._servo_version == 'servo_v4_with_ccd_cr50':
            self._program_cmd += ' --raiden'


class ProgrammerV2(object):
    """Main programmer class which provides programmer for BIOS and EC with
    servo V2."""

    def __init__(self, servo):
        self._servo = servo
        self._valid_boards = self._get_valid_v2_boards()
        self._bios_programmer = self._factory_bios(self._servo)
        self._ec_programmer = self._factory_ec(self._servo)


    @staticmethod
    def _get_valid_v2_boards():
        """Greps servod config files to look for valid v2 boards.

        @return A list of valid board names.
        """
        site_packages_paths = site.getsitepackages()
        SERVOD_CONFIG_DATA_DIR = None
        for p in site_packages_paths:
            servo_data_path = os.path.join(p, 'servo', 'data')
            if os.path.exists(servo_data_path):
                SERVOD_CONFIG_DATA_DIR = servo_data_path
                break
        if not SERVOD_CONFIG_DATA_DIR:
            raise ProgrammerError(
                    'Unable to locate data directory of Python servo module')
        SERVOFLEX_V2_R0_P50_CONFIG = 'servoflex_v2_r0_p50.xml'
        SERVO_CONFIG_GLOB = 'servo_*_overlay.xml'
        SERVO_CONFIG_REGEXP = 'servo_(?P<board>.+)_overlay.xml'

        def is_v2_compatible_board(board_config_path):
            """Check if the given board config file is v2-compatible.

            @param board_config_path: Path to a board config XML file.

            @return True if the board is v2-compatible; False otherwise.
            """
            configs = []
            def get_all_includes(config_path):
                """Get all included XML config names in the given config file.

                @param config_path: Path to a servo config file.
                """
                root = xml.etree.ElementTree.parse(config_path).getroot()
                for element in root.findall('include'):
                    include_name = element.find('name').text
                    configs.append(include_name)
                    get_all_includes(os.path.join(
                            SERVOD_CONFIG_DATA_DIR, include_name))

            get_all_includes(board_config_path)
            return True if SERVOFLEX_V2_R0_P50_CONFIG in configs else False

        result = []
        board_overlays = glob.glob(
                os.path.join(SERVOD_CONFIG_DATA_DIR, SERVO_CONFIG_GLOB))
        for overlay_path in board_overlays:
            if is_v2_compatible_board(overlay_path):
                result.append(re.search(SERVO_CONFIG_REGEXP,
                                        overlay_path).group('board'))
        return result


    def _get_flashrom_programmer(self, servo):
        """Gets a proper flashrom programmer.

        @param servo: A servo object.

        @return A programmer for flashrom.
        """
        return FlashromProgrammer(servo)


    def _factory_bios(self, servo):
        """Instantiates and returns (bios, ec) programmers for the board.

        @param servo: A servo object.

        @return A programmer for ec. If the programmer is not supported
            for the board, None will be returned.
        """
        _bios_prog = None
        _board = servo.get_board()

        logging.debug('Setting up BIOS programmer for board: %s', _board)
        if _board in self._valid_boards:
            _bios_prog = self._get_flashrom_programmer(servo)
        else:
            logging.warning('No BIOS programmer found for board: %s', _board)

        return _bios_prog


    def _factory_ec(self, servo):
        """Instantiates and returns ec programmer for the board.

        @param servo: A servo object.

        @return A programmer for ec. If the programmer is not supported
            for the board, None will be returned.
        """
        _ec_prog = None
        _board = servo.get_board()

        logging.debug('Setting up EC programmer for board: %s', _board)
        if _board in self._valid_boards:
            _ec_prog = FlashECProgrammer(servo)
        else:
            logging.warning('No EC programmer found for board: %s', _board)

        return _ec_prog


    def program_bios(self, image):
        """Programs the DUT with provide bios image.

        @param image: (required) location of bios image file.

        """
        self._bios_programmer.prepare_programmer(image)
        self._bios_programmer.program()


    def program_ec(self, image):
        """Programs the DUT with provide ec image.

        @param image: (required) location of ec image file.

        """
        self._ec_programmer.prepare_programmer(image)
        self._ec_programmer.program()


class ProgrammerV2RwOnly(ProgrammerV2):
    """Main programmer class which provides programmer for only updating the RW
    portion of BIOS with servo V2.

    It does nothing on EC, as EC software sync on the next boot will
    automatically overwrite the EC RW portion, using the EC RW image inside
    the BIOS RW image.

    """

    def _get_flashrom_programmer(self, servo):
        """Gets a proper flashrom programmer.

        @param servo: A servo object.

        @return A programmer for flashrom.
        """
        return FlashromProgrammer(servo, keep_ro=True)


    def program_ec(self, image):
        """Programs the DUT with provide ec image.

        @param image: (required) location of ec image file.

        """
        # Do nothing. EC software sync will update the EC RW.
        pass


class ProgrammerV3(object):
    """Main programmer class which provides programmer for BIOS and EC with
    servo V3.

    Different from programmer for servo v2, programmer for servo v3 does not
    try to validate if the board can use servo V3 to update firmware. As long as
    the servod process running in beagblebone with given board, the program will
    attempt to flash bios and ec.

    """

    def __init__(self, servo):
        self._servo = servo
        self._bios_programmer = FlashromProgrammer(servo)
        self._ec_programmer = FlashECProgrammer(servo)


    def program_bios(self, image):
        """Programs the DUT with provide bios image.

        @param image: (required) location of bios image file.

        """
        self._bios_programmer.prepare_programmer(image)
        self._bios_programmer.program()


    def program_ec(self, image):
        """Programs the DUT with provide ec image.

        @param image: (required) location of ec image file.

        """
        self._ec_programmer.prepare_programmer(image)
        self._ec_programmer.program()


class ProgrammerV3RwOnly(ProgrammerV3):
    """Main programmer class which provides programmer for only updating the RW
    portion of BIOS with servo V3.

    It does nothing on EC, as EC software sync on the next boot will
    automatically overwrite the EC RW portion, using the EC RW image inside
    the BIOS RW image.

    """

    def __init__(self, servo):
        self._servo = servo
        self._bios_programmer = FlashromProgrammer(servo, keep_ro=True)


    def program_ec(self, image):
        """Programs the DUT with provide ec image.

        @param image: (required) location of ec image file.

        """
        # Do nothing. EC software sync will update the EC RW.
        pass


class ProgrammerDfu(object):
    """Main programmer class which provides programmer for Base EC via DFU.

    It programs through the DUT, i.e. running the flash_ec script on DUT
    instead of running it in beaglebone (a host of the servo board).
    It is independent of the version of servo board as long as the servo
    board has the ec_boot_mode interface.

    """

    def __init__(self, servo, cros_host):
        self._servo = servo
        self._cros_host = cros_host
        # Get the chip name of the base EC and append '_dfu' to it, like
        # 'stm32' -> 'stm32_dfu'.
        ec_chip = servo.get(servo.get_base_board() + '_ec_chip') + '_dfu'
        self._ec_programmer = FlashECProgrammer(servo, cros_host, ec_chip)


    def program_ec(self, image):
        """Programs the DUT with provide ec image.

        @param image: (required) location of ec image file.

        """
        self._ec_programmer.prepare_programmer(image)
        ec_boot_mode = self._servo.get_base_board() + '_ec_boot_mode'
        try:
            self._servo.set(ec_boot_mode, 'on')
            # Power cycle the base to enter DFU mode
            self._cros_host.run('ectool gpioset PP3300_DX_BASE 0')
            self._cros_host.run('ectool gpioset PP3300_DX_BASE 1')
            self._ec_programmer.program()
        finally:
            self._servo.set(ec_boot_mode, 'off')
            # Power cycle the base to back normal mode
            self._cros_host.run('ectool gpioset PP3300_DX_BASE 0')
            self._cros_host.run('ectool gpioset PP3300_DX_BASE 1')
