# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import re
import logging
import time

from autotest_lib.client.common_lib import error
from autotest_lib.server.cros.servo import pd_console


class PDDevice(object):
    """Base clase for all PD devices

    This class provides a set of APIs for expected Type C PD required actions
    in TypeC FAFT tests. The base class is specific for Type C devices that
    do not have any console access.

    """

    def is_src(self):
        """Checks if the port is connected as a source

        """
        raise NotImplementedError(
                'is_src should be implemented in derived class')

    def is_snk(self):
        """Checks if the port is connected as a sink

        @returns None
        """
        raise NotImplementedError(
                'is_snk should be implemented in derived class')

    def is_connected(self):
        """Checks if the port is connected

        @returns True if in a connected state, False otherwise
        """
        return self.is_src() or self.is_snk()

    def is_disconnected(self):
        """Checks if the port is disconnected

        """
        raise NotImplementedError(
                'is_disconnected should be implemented in derived class')

    def is_ufp(self):
        """Checks if data role is UFP

        """
        raise NotImplementedError(
                'is_ufp should be implemented in derived class')

    def is_dfp(self):
        """Checks if data role is DFP

        """
        raise NotImplementedError(
                'is_dfp should be implemented in derived class')

    def is_drp(self):
        """Checks if dual role mode is supported

        """
        raise NotImplementedError(
                'is_drp should be implemented in derived class')

    def dr_swap(self):
        """Attempts a data role swap

        """
        raise NotImplementedError(
               'dr_swap should be implemented in derived class')

    def pr_swap(self):
        """Attempts a power role swap

        """
        raise NotImplementedError(
                'pr_swap should be implemented in derived class')

    def vbus_request(self, voltage):
        """Requests a specific VBUS voltage from SRC

        @param voltage: requested voltage level (5, 12, 20) in volts
        """
        raise NotImplementedError(
                'vbus_request should be implemented in derived class')

    def soft_reset(self):
        """Initates a PD soft reset sequence

        """
        raise NotImplementedError(
                'soft_reset should be implemented in derived class')

    def hard_reset(self):
        """Initates a PD hard reset sequence

        """
        raise NotImplementedError(
                'hard_reset should be implemented in derived class')

    def drp_set(self, mode):
        """Sets dualrole mode

        @param mode: desired dual role setting (on, off, snk, src)
        """
        raise NotImplementedError(
                'drp_set should be implemented in derived class')

    def drp_disconnect_connect(self, disc_time_sec):
        """Force PD disconnect/connect via drp settings

        @param disc_time_sec: Time in seconds between disconnect and reconnect
        """
        raise NotImplementedError(
                'drp_disconnect_reconnect should be implemented in \
                derived class')

    def cc_disconnect_connect(self, disc_time_sec):
        """Force PD disconnect/connect using Plankton fw command

        @param disc_time_sec: Time in seconds between disconnect and reconnect
        """
        raise NotImplementedError(
                'cc_disconnect_reconnect should be implemented in \
                derived class')


class PDConsoleDevice(PDDevice):
    """Class for PD devices that have console access

    This class contains methods for common PD actions for any PD device which
    has UART console access. It inherits the PD device base class. In addition,
    it stores both the UART console and port for the PD device.
    """

    def __init__(self, console, port):
        """Initialization method

        @param console: UART console object
        @param port: USB PD port number
        """
        # Save UART console
        self.console = console
        # Instantiate PD utilities used by methods in this class
        self.utils = pd_console.PDConsoleUtils(console)
        # Save the PD port number for this device
        self.port = port
        # Not a Plankton device
        self.is_plankton = False

    def is_src(self):
        """Checks if the port is connected as a source

        @returns True if connected as SRC, False otherwise
        """
        state = self.utils.get_pd_state(self.port)
        return bool(state == self.utils.SRC_CONNECT)

    def is_snk(self):
        """Checks if the port is connected as a sink

        @returns True if connected as SNK, False otherwise
        """
        state = self.utils.get_pd_state(self.port)
        return bool(state == self.utils.SNK_CONNECT)

    def is_connected(self):
        """Checks if the port is connected

        @returns True if in a connected state, False otherwise
        """
        state = self.utils.get_pd_state(self.port)
        return bool(state == self.utils.SNK_CONNECT or
                    state == self.utils.SRC_CONNECT)

    def is_disconnected(self):
        """Checks if the port is disconnected

        @returns True if in a disconnected state, False otherwise
        """
        state = self.utils.get_pd_state(self.port)
        return bool(state == self.utils.SRC_DISC or
                    state == self.utils.SNK_DISC)

    def is_drp(self):
        """Checks if dual role mode is supported

        @returns True if dual role mode is 'on', False otherwise
        """
        return self.utils.is_pd_dual_role_enabled()

    def drp_disconnect_connect(self, disc_time_sec):
        """Disconnect/reconnect using drp mode settings

        A PD console device doesn't have an explicit connect/disconnect
        command. Instead, the dualrole mode setting is used to force
        disconnects in devices which support this feature. To disconnect,
        force the dualrole mode to be the opposite role of the current
        connected state.

        @param disc_time_sec: time in seconds to wait to reconnect

        @returns True if device disconnects, then returns to a connected
        state. False if either step fails.
        """
        # Dualrole mode must be supported
        if self.is_drp() is False:
            logging.warn('Device not DRP capable, unabled to force disconnect')
            return False
        # Force state will be the opposite of current connect state
        if self.is_src():
            drp_mode = 'snk'
            swap_state = self.utils.SNK_CONNECT
        else:
            drp_mode = 'src'
            swap_state = self.utils.SRC_CONNECT
        # Force disconnect
        self.drp_set(drp_mode)
        # Wait for disconnect time
        time.sleep(disc_time_sec)
        # Verify that the device is disconnected
        disconnect = self.is_disconnected()

        # If the other device is dualrole, then forcing dualrole mode will
        # only cause the disconnect to appear momentarily and reconnect
        # in the power role forced by the drp_set() call. For this case,
        # the role swap verifies that a disconnect/connect sequence occurred.
        if disconnect == False:
            time.sleep(self.utils.CONNECT_TIME)
            # Connected, verify if power role swap has ocurred
            if swap_state == self.utils.get_pd_state(self.port):
                # Restore default dualrole mode
                self.drp_set('on')
                # Restore orignal power role
                connect = self.pr_swap()
                if connect == False:
                    logging.warn('DRP on both devices, 2nd power swap failed')
                return connect

        # Restore default dualrole mode
        self.drp_set('on')
        # Allow enough time for protocol state machine
        time.sleep(self.utils.CONNECT_TIME)
        # Check if connected
        connect = self.is_connected()
        logging.info('Disconnect = %r, Connect = %r', disconnect, connect)
        return bool(disconnect and connect)

    def drp_set(self, mode):
        """Sets dualrole mode

        @param mode: desired dual role setting (on, off, snk, src)

        @returns True is set was successful, False otherwise
        """
        # Set desired dualrole mode
        self.utils.set_pd_dualrole(mode)
        # Get the expected output
        resp = self.utils.dualrole_resp[self.utils.dual_index[mode]]
        # Get current setting
        current = self.utils.get_pd_dualrole()
        # Verify that setting is correct
        return bool(resp == current)

    def try_src(self, enable):
        """Enables/Disables Try.SRC PD protocol setting

        @param enable: True to enable, False to disable

        @returns True is setting was successful, False if feature not
        supported by the device, or not set as desired.
        """
        # Create Try.SRC pd command
        cmd = 'pd trysrc %d' % int(enable)
        # Try.SRC on/off is output, if supported feature
        regex = ['Try\.SRC\s([\w]+)|(Parameter)']
        m = self.utils.send_pd_command_get_output(cmd, regex)
        # Determine if Try.SRC feature is supported
        trysrc = re.search('Try\.SRC\s([\w]+)', m[0][0])
        if not trysrc:
            logging.warn('Try.SRC not supported on this PD device')
            return False
        # TrySRC is supported on this PD device, verify setting.
        logging.info('Try.SRC mode = %s', trysrc.group(1))
        if enable:
            val = 'on'
        else:
            val = 'off'
        return bool(val == m[0][1])

    def soft_reset(self):
        """Initates a PD soft reset sequence

        To verify that a soft reset sequence was initiated, the
        reply message is checked to verify that the reset command
        was acknowledged by its port pair. The connect state should
        be same as it was prior to issuing the reset command.

        @returns True if the port pair acknowledges the the reset message
        and if following the command, the device returns to the same
        connected state. False otherwise.
        """
        RESET_DELAY = 0.5
        cmd = 'pd %d soft' % self.port
        state_before = self.utils.get_pd_state(self.port)
        reply = self.utils.send_pd_command_get_reply_msg(cmd)
        if reply != self.utils.PD_CONTROL_MSG_DICT['Accept']:
            return False
        time.sleep(RESET_DELAY)
        state_after = self.utils.get_pd_state(self.port)
        return state_before == state_after

    def hard_reset(self):
        """Initates a PD hard reset sequence

        To verify that a hard reset sequence was initiated, the
        console ouput is scanned for HARD RST TX. In addition, the connect
        state should be same as it was prior to issuing the reset command.

        @returns True if the port pair acknowledges that hard reset was
        initiated and if following the command, the device returns to the same
        connected state. False otherwise.
        """
        RESET_DELAY = 1.0
        cmd = 'pd %d hard' % self.port
        state_before = self.utils.get_pd_state(self.port)
        self.utils.enable_pd_console_debug()
        try:
            self.utils.send_pd_command_get_output(cmd, ['.*(HARD\sRST\sTX)'])
        except error.TestFail:
            logging.warn('HARD RST TX not found')
            return False
        finally:
            self.utils.disable_pd_console_debug()

        time.sleep(RESET_DELAY)
        state_after = self.utils.get_pd_state(self.port)
        return state_before == state_after

    def pr_swap(self):
        """Attempts a power role swap

        In order to attempt a power role swap the device must be
        connected and support dualrole mode. Once these two criteria
        are checked a power role command is issued. Following a delay
        to allow for a reconnection the new power role is checked
        against the power role prior to issuing the command.

        @returns True if the device has swapped power roles, False otherwise.
        """
        # Get starting state
        if not self.is_drp() and not self.drp_set('on'):
            logging.warn('Dualrole Mode not enabled!')
            return False
        if self.is_connected() == False:
            logging.warn('PD contract not established!')
            return False
        current_pr = self.utils.get_pd_state(self.port)
        swap_cmd = 'pd %d swap power' % self.port
        self.utils.send_pd_command(swap_cmd)
        time.sleep(self.utils.CONNECT_TIME)
        new_pr = self.utils.get_pd_state(self.port)
        logging.info('Power swap: %s -> %s', current_pr, new_pr)
        if self.is_connected() == False:
            logging.warn('Device not connected following PR swap attempt.')
            return False
        return current_pr != new_pr


class PDPlanktonDevice(PDConsoleDevice):
    """Class for PD Plankton devices

    This class contains methods for PD funtions which are unique to the
    Plankton Type C factory testing board. It inherits all the methods
    for PD console devices.
    """

    def __init__(self, console, port):
        """Initialization method

        @param console: UART console for this device
        @param port: USB PD port number
        """
        # Instantiate the PD console object
        super(PDPlanktonDevice, self).__init__(console, 0)
        # Indicate this is Plankton device
        self.is_plankton = True

    def _toggle_plankton_drp(self):
        """Issue 'usbc_action drp' Plankton command

        @returns value of drp_enable in Plankton FW
        """
        drp_cmd = 'usbc_action drp'
        drp_re = ['DRP\s=\s(\d)']
        # Send DRP toggle command to Plankton and get value of 'drp_enable'
        m = self.utils.send_pd_command_get_output(drp_cmd, drp_re)
        return int(m[0][1])

    def _enable_plankton_drp(self):
        """Enable DRP mode on Plankton

        DRP mode can only be toggled and is not able to be explicitly
        enabled/disabled via the console. Therefore, this method will
        toggle DRP mode until the console reply indicates that this
        mode is enabled. The toggle happens a maximum of two times
        in case this is called when it's already enabled.

        @returns True when DRP mode is enabled, False if not successful
        """
        for attempt in xrange(2):
            if self._toggle_plankton_drp() == True:
                logging.info('Plankton DRP mode enabled')
                return True
        logging.error('Plankton DRP mode set failure')
        return False

    def _verify_state_sequence(self, states_list, console_log):
        """Compare PD state transitions to expected values

        @param states_list: list of expected PD state transitions
        @param console_log: console output which contains state names

        @returns True if the sequence matches, False otherwise
        """
        # For each state in the expected state transiton table, build
        # the regexp and search for it in the state transition log.
        for state in states_list:
            state_regx = r'C{0}\s+[\w]+:\s({1})'.format(self.port,
                                                        state)
            if re.search(state_regx, console_log) is None:
                return False
        return True

    def cc_disconnect_connect(self, disc_time_sec):
        """Disconnect/reconnect using Plankton

        Plankton supports a feature which simulates a USB Type C disconnect
        and reconnect.

        @param disc_time_sec: Time in seconds for disconnect period.
        """
        DISC_DELAY = 100
        disc_cmd = 'fake_disconnect %d  %d' % (DISC_DELAY,
                                               disc_time_sec * 1000)
        self.utils.send_pd_command(disc_cmd)

    def drp_disconnect_connect(self, disc_time_sec):
        """Disconnect/reconnect using Plankton

        Utilize Plankton disconnect/connect utility and verify
        that both disconnect and reconnect actions were successful.

        @param disc_time_sec: Time in seconds for disconnect period.

        @returns True if device disconnects, then returns to a connected
        state. False if either step fails.
        """
        self.cc_disconnect_connect(disc_time_sec)
        time.sleep(disc_time_sec / 2)
        disconnect = self.is_disconnected()
        time.sleep(disc_time_sec / 2 + self.utils.CONNECT_TIME)
        connect = self.is_connected()
        return disconnect and connect

    def drp_set(self, mode):
        """Sets dualrole mode

        @param mode: desired dual role setting (on, off, snk, src)

        @returns True if dualrole mode matches the requested value or
        is successfully set to that value. False, otherwise.
        """
        # Get correct dualrole console response
        resp = self.utils.dualrole_resp[self.utils.dual_index[mode]]
        # Get current value of dualrole
        drp = self.utils.get_pd_dualrole()
        if drp == resp:
            return True

        if mode == 'on':
            # Setting dpr_enable on Plankton will set dualrole mode to on
            return self._enable_plankton_drp()
        else:
            # If desired setting is other than 'on', need to ensure that
            # drp mode on Plankton is disabled.
            if resp == 'on':
                # This will turn off drp_enable flag and set dualmode to 'off'
                return self._toggle_plankton_drp()
            # With drp_enable flag off, can set to desired setting
            return self.utils.set_pd_dualrole(mode)

    def _reset(self, cmd, states_list):
        """Initates a PD reset sequence

        Plankton device has state names available on the console. When
        a soft reset is issued the console log is extracted and then
        compared against the expected state transisitons.

        @param cmd: reset type (soft or hard)
        @param states_list: list of expected PD state transitions

        @returns True if state transitions match, False otherwise
        """
        # Want to grab all output until either SRC_READY or SNK_READY
        reply_exp = ['(.*)(C\d)\s+[\w]+:\s([\w]+_READY)']
        m = self.utils.send_pd_command_get_output(cmd, reply_exp)
        return self._verify_state_sequence(states_list, m[0][0])

    def soft_reset(self):
        """Initates a PD soft reset sequence

        @returns True if state transitions match, False otherwise
        """
        snk_reset_states = [
            'SOFT_RESET',
            'SNK_DISCOVERY',
            'SNK_REQUESTED',
            'SNK_TRANSITION',
            'SNK_READY'
        ]

        src_reset_states = [
            'SOFT_RESET',
            'SRC_DISCOVERY',
            'SRC_NEGOCIATE',
            'SRC_ACCEPTED',
            'SRC_POWERED',
            'SRC_TRANSITION',
            'SRC_READY'
        ]

        if self.is_src():
            states_list = src_reset_states
        elif self.is_snk():
            states_list = snk_reset_states
        else:
            raise error.TestFail('Port Pair not in a connected state')

        cmd = 'pd %d soft' % self.port
        return self._reset(cmd, states_list)

    def hard_reset(self):
        """Initates a PD hard reset sequence

        @returns True if state transitions match, False otherwise
        """
        snk_reset_states = [
            'HARD_RESET_SEND',
            'HARD_RESET_EXECUTE',
            'SNK_HARD_RESET_RECOVER',
            'SNK_DISCOVERY',
            'SNK_REQUESTED',
            'SNK_TRANSITION',
            'SNK_READY'
        ]

        src_reset_states = [
            'HARD_RESET_SEND',
            'HARD_RESET_EXECUTE',
            'SRC_HARD_RESET_RECOVER',
            'SRC_DISCOVERY',
            'SRC_NEGOCIATE',
            'SRC_ACCEPTED',
            'SRC_POWERED',
            'SRC_TRANSITION',
            'SRC_READY'
        ]

        if self.is_src():
            states_list = src_reset_states
        elif self.is_snk():
            states_list = snk_reset_states
        else:
            raise error.TestFail('Port Pair not in a connected state')

        cmd = 'pd %d hard' % self.port
        return self._reset(cmd, states_list)


class PDPortPartner(object):
    """Methods used to instantiate PD device objects

    This class is initalized with a list of servo consoles. It
    contains methods to determine if USB PD devices are accessible
    via the consoles and attempts to determine USB PD port partners.
    A PD device is USB PD port specific, a single console may access
    multiple PD devices.

    """

    def __init__(self, consoles):
        """Initialization method

        @param consoles: list of servo consoles
        """
        self.consoles = consoles

    def _send_pd_state(self, port, console):
        """Tests if PD device exists on a given port number

        @param port: USB PD port number to try
        @param console: servo UART console

        @returns True if 'pd <port> state' command gives a valid
        response, False otherwise
        """
        cmd = 'pd %d state' % port
        regex = r'(Port C\d)|(Parameter)'
        m = console.send_command_get_output(cmd, [regex])
        # If PD port exists, then output will be Port C0 or C1
        regex = r'Port C{0}'.format(port)
        if re.search(regex, m[0][0]):
            return True
        return False

    def _find_num_pd_ports(self, console):
        """Determine number of PD ports for a given console

        @param console: uart console accssed via servo

        @returns: number of PD ports accessible via console
        """
        MAX_PORTS = 2
        num_ports = 0
        for port in xrange(MAX_PORTS):
            if self._send_pd_state(port, console):
                num_ports += 1
        return num_ports

    def _is_pd_console(self, console):
        """Check if pd option exists in console

        @param console: uart console accssed via servo

        @returns: True if 'pd' is found, False otherwise
        """
        try:
            m = console.send_command_get_output('help', [r'(pd)\s+'])
            return True
        except error.TestFail:
            return False

    def _is_plankton_console(self, console):
        """Check for Plankton console

        This method looks for a console command option 'usbc_action' which
        is unique to Plankton PD devices.

        @param console: uart console accssed via servo

        @returns True if usbc_action command is present, False otherwise
        """
        try:
            m = console.send_command_get_output('help', [r'(usbc_action)'])
            return True
        except error.TestFail:
            return False

    def _check_port_pair(self, dev_pair):
        """Check if two PD devices could be connected

        If two USB PD devices are connected, then they should be in
        either the SRC_READY or SNK_READY states and have opposite
        power roles. In addition, they must be on different servo
        consoles.

        @param: list of two possible PD port parters

        @returns True if not the same console and both PD devices
        are a plausible pair based only on their PD states.
        """
        # Don't test if on the same servo console
        if dev_pair[0].console == dev_pair[1].console:
            logging.info('PD Devices are on same platform -> cant be a pair')
            return False
        # Must be SRC <--> SNK or SNK <--> SRC
        return bool((dev_pair[0].is_src() and dev_pair[1].is_snk()) or
                    (dev_pair[0].is_snk() and dev_pair[1].is_src()))

    def _verify_plankton_connection(self, dev_pair):
        """Verify DUT to Plankton PD connection

        This method checks for a Plankton PD connection for the
        given port by first verifying if a PD connection is present.
        If found, then it uses a Plankton feature to force a PD disconnect.
        If the port is no longer in the connected state, and following
        a delay, is found to be back in the connected state, then
        a DUT pd to Plankton connection is verified.

        @param dev_pair: list of two PD devices

        @returns True if DUT to Plankton pd connection is verified
        """
        DISC_CHECK_TIME = .5
        DISC_WAIT_TIME = 2
        CONNECT_TIME = 4

        if not self._check_port_pair(dev_pair):
            return False

        for index in xrange(len(dev_pair)):
            try:
                # Force PD disconnect
                dev_pair[index].cc_disconnect_connect(DISC_WAIT_TIME)
                time.sleep(DISC_CHECK_TIME)
                # Verify that both devices are now disconnected
                if (dev_pair[0].is_disconnected() and
                    dev_pair[1].is_disconnected()):
                    # Allow enough time for reconnection
                    time.sleep(DISC_WAIT_TIME + CONNECT_TIME)
                    if self._check_port_pair(dev_pair):
                        # Have verifed a pd disconnect/reconnect sequence
                        logging.info('Plankton <-> DUT pair found')
                        return True
                else:
                    # Delay to allow
                    time.sleep(DISC_WAIT_TIME + CONNECT_TIME)
            except NotImplementedError:
                logging.info('dev %d is not Plankton', index)
        return False

    def identify_pd_devices(self):
        """Instantiate PD devices present in test setup

        @returns list of 2 PD devices if a DUT <-> Plankton found. If
        not found, then returns an empty list.
        """
        devices = []
        # For each possible uart console, check to see if a PD console
        # is present and determine the number of PD ports.
        for console in self.consoles:
            if self._is_pd_console(console):
                is_plank = self._is_plankton_console(console)
                num_ports = self._find_num_pd_ports(console)
                # For each PD port that can be accessed via the console,
                # instantiate either PDConsole or PDPlankton device.
                for port in xrange(num_ports):
                    if is_plank:
                        logging.info('Plankton PD Device on port %d', port)
                        devices.append(PDPlanktonDevice(console, port))
                    else:
                        devices.append(PDConsoleDevice(console, port))
                        logging.info('Console PD Device on port %d', port)

        # Determine PD port partners in the list of PD devices. Note, that
        # there can be PD devices which are not accessible via a uart console,
        # but are connected to a PD port which is accessible.
        test_pair = []
        for deva in devices:
            for dev_idx in range(devices.index(deva) + 1, len(devices)):
                devb = devices[dev_idx]
                pair = [deva, devb]
                if self._verify_plankton_connection(pair):
                    test_pair = pair
                    devices.remove(deva)
                    devices.remove(devb)
        return test_pair

