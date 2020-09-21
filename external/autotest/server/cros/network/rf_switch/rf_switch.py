# Copyright (c) 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Interface to control RF Switch.

Helper module to control RF Switch. Common commands to control the relays
are made available as methods that can be called.

Please refer go/rf-switch for more info on the switch.
"""
import contextlib
import logging

from autotest_lib.server.cros.network.rf_switch import scpi


class RfSwitchException(Exception):
    """Exception for RfSwitch Errors."""


class RfSwitch(scpi.Scpi):
    """RF Switch Controller."""

    _ALL_AP_RELAYS = 'k1_1:k1_24'  # Each AP uses 6 relays
    _ALL_CLIENT_RELAYS = 'k1_25:k1_48'  # Each client uses 6 relays
    _MIN_ENCLOSURE = 1  # Starting enclosure number
    _MAX_ENCLOSURE = 4  # Last enclosure number
    _ALL_ENCLOSURE = 0  # All the enclosures
    _RELAYS_PER_ENCLOSURE = 6  # Number of relays in an enclosure

    # Each AP enclosure uses 6 relays and a R relay to set attenuation
    _AP_ATTENUATOR_RELAYS = {
        1: ['k1_49', 'k1_50', 'k1_51', 'k1_52', 'k1_53', 'k1_54', 'R1_9'],
        2: ['k1_55', 'k1_56', 'k1_57', 'k1_58', 'k1_59', 'k1_60', 'R1_10'],
        3: ['k1_61', 'k1_62', 'k1_63', 'k1_64', 'k1_65', 'k1_66', 'R1_11'],
        4: ['k1_67', 'k1_68', 'k1_69', 'k1_70', 'k1_71', 'k1_72', 'R1_12'],
    }
    # Shorter version
    _AP_ATTENUATOR_RELAYS_SHORT = {
        1: 'k1_49:k1_54,R1_9',
        2: 'k1_55:k1_60,R1_10',
        3: 'k1_61:k1_66,R1_11',
        4: 'k1_67:k1_72,R1_12',
    }

    _CMD_CLOSE_RELAYS = 'ROUT:CLOS'
    _CMD_CHECK_RELAYS_CLOSED = 'ROUT:CLOS?'
    _CMD_OPEN_RELAYS = 'ROUT:OPEN'
    _CMD_OPEN_ALL_RELAYS = 'ROUT:OPEN:ALL'
    _CMD_SET_VERIFY = 'ROUT:CHAN:VER'
    _CMD_GET_VERIFY = 'ROUT:CHAN:VER?'
    _CMD_SET_VERIFY_INVERTED = 'ROUT:CHAN:VER:POL'
    _CMD_GET_VERIFY_INVERTED = 'ROUT:CHAN:VER:POL?'
    _CMD_GET_VERIFY_STATE = 'ROUT:CHAN:VER:POS:STAT?'
    _CMD_CHECK_BUSY = 'ROUT:MOD:BUSY?'
    _CMD_WAIT = 'ROUT:MOD:WAIT'

    def __init__(self, host, port=scpi.Scpi.SCPI_PORT):
        """Controller for RF Switch.

        @param host: Hostname or IP address of RF Switch
        @param port: Int SCPI port number (default 5025)

        """
        scpi.Scpi.__init__(self, host, port)

    def send_cmd_check_error(self, cmd):
        """Send command to switch and check for any error.

        @param cmd: string cmd to send to switch

        """
        self.write(cmd)
        code, error = self.error_query()
        if code:
            raise RfSwitchException('Error on command: "%s" - code: %s,'
                                    ' message: %s', cmd, code, error)
        self.wait()

    def close_relays(self, relays):
        """Close relays.

        @param relays: relays to close (, to separate and : for range)

        """
        self.send_cmd_check_error('%s (@%s)\n'
                                  % (self._CMD_CLOSE_RELAYS, relays))

    def relays_closed(self, relays):
        """Get open/closed status of relays.

        @param relays: relays to check (, to separate and : for range)

        @returns tuple of relay status, status is true if a relay is closed.

        """

        status = self.query('%s (@%s)\n' % (self._CMD_CHECK_RELAYS_CLOSED,
                                            relays)).strip().split(',')
        return tuple(bool(int(x)) for x in status)

    def open_relays(self, relays):
        """Open relays.

        @param relays: string relays to close (, to separate and : for range)

        """
        self.send_cmd_check_error('%s (@%s)\n' % (
                self._CMD_OPEN_RELAYS, relays))

    def open_all_relays(self):
        """Open all relays.

        This will open all relays including the attenuator, which will set it
        to Max. Please remember to set your attenuation to right level after
        this call.

        """
        self.send_cmd_check_error('%s\n' % self._CMD_OPEN_ALL_RELAYS)

    def set_verify(self, relays, on):
        """Configure Close? to return indicator state instead of driven state.

        @param relays: relays to verify (, to separate and : for range)
        @param on: string state to verify (on(True)/off(False))

        """
        self.send_cmd_check_error('%s %s,(@%s)\n' % (
            self._CMD_SET_VERIFY, int(bool(on)), relays))

    def get_verify(self, relays):
        """Get the verify mode of relays.

        @param relays: relays to verify (, to separate and : for range)
        @returns tuple of verify mode for relays

        """
        status = self.query('%s (@%s)\n' % (
            self._CMD_GET_VERIFY, relays)).strip().split(',')
        return tuple(bool(int(x)) for x in status)

    def set_verify_inverted(self, relays, inverted):
        """Set the polarity confidence of relay to be inverted.

        @param relays: relays to set (, to separate and : for range)
        @param inverted: Boolean True if INV, False for NORM

        """
        self.send_cmd_check_error('%s %s,(@%s)\n' % (
                self._CMD_SET_VERIFY_INVERTED,
                'INV' if inverted else 'NORM', relays))

    def get_verify_inverted(self, relays):
        """Get the confidence polarity. 1 is inverted, 0 is value as-is.

        @param relays: relays to get (, to separate and : for range)
        @returns tuple of status where 1 in inverted and 0 for value as-is

        """
        status = self.query('%s (@%s)\n' % (self._CMD_GET_VERIFY_INVERTED,
                                            relays)).strip().split(',')
        return tuple(bool(int(x)) for x in status)

    def get_verify_state(self, relays):
        """If verify set get driven state, else confidence state.

        @param relays: relays to get (, to separate and : for range)

        @returns tuple of verify status for given relays

        """
        status = self.query('%s (@%s)\n' % (self._CMD_GET_VERIFY_STATE,
                                            relays)).strip().split(',')
        return tuple(bool(int(x)) for x in status)

    @property
    def busy(self):
        """Check relays are still settling.

        @returns Boolean True if relays are settling, False if not.

        """
        return bool(int(self.query('%s\n' % self._CMD_CHECK_BUSY).strip()))

    def wait(self):
        """Wait for relays to debounce."""
        self.write('%s\n' % self._CMD_WAIT)

    def get_attenuation(self, ap_enclosure):
        """Get Attenuation for an AP enclosure.

        Attenuation is set by turning on/off the relays.  Each relay specifies
        a bit in the binary value of attenuation.  Find the relay status and
        build the binary value, then find the decimal value from it.

        @param ap_enclosure: Int 1/2/3/4 (AP enclosure index)

        @returns attenuation value in int

        @raises ValueError: on bad ap_enclosure value

        """
        if (ap_enclosure < self._MIN_ENCLOSURE or
                ap_enclosure > self._MAX_ENCLOSURE):
            raise ValueError('Invalid AP enclosure number: %s.', ap_enclosure)
        else:
            a_status = self.relays_closed(
                self._AP_ATTENUATOR_RELAYS_SHORT[ap_enclosure])
            status = a_status[::-1]  # reverse for Endian transform
            logging.debug('attenuator status: %s', status)

            # build the binary value
            binary = ''.join(['0' if x else '1' for x in status])
            return int(binary, 2)

    def get_all_attenuation(self):
        """Get attenuation value for all AP enclosures.

        @returns tuple of attenuation value for each enclosure

        """
        attenuations = []
        for x in xrange(self._MIN_ENCLOSURE, self._MAX_ENCLOSURE + 1):
            attenuations.append(self.get_attenuation(x))
        return tuple(attenuations)

    def set_attenuation(self, ap_enclosure, attenuation):
        """Set attenuation for an AP enclosure.

        @param ap_enclosure: Int 0,1,2,3,4 AP enclosure number. 0 for all
        @param attenuation: Int Attenuation value to set

        @raises ValueError: on bad ap_enclosure value

        """
        if ap_enclosure == self._ALL_ENCLOSURE:
            # set attenuation on all
            for x in xrange(self._MIN_ENCLOSURE, self._MAX_ENCLOSURE + 1):
                self.set_attenuation(x, attenuation)
        elif (ap_enclosure < self._MIN_ENCLOSURE or
              ap_enclosure > self._MAX_ENCLOSURE):
            raise ValueError('Bad AP enclosure value: %s' % ap_enclosure)
        else:
            # convert attenuation decimal value to binary
            bin_array = [int(x) for x in list('{0:07b}'.format(attenuation))]
            # For endian
            reverse_bits = self._AP_ATTENUATOR_RELAYS[ap_enclosure][:: -1]

            # determine which bits should be set
            relays_to_close = [
                reverse_bits[i] for i, j in enumerate(bin_array) if not j
            ]

            # open all relay for attenuator & then close the ones we need
            relays = ','.join(self._AP_ATTENUATOR_RELAYS[ap_enclosure])
            self.open_relays(relays)
            logging.debug('Attenuator relays opened: %s', relays)
            relays = ','.join(relays_to_close)
            self.close_relays(relays)
            logging.debug('Attenuator relays closed: %s', relays)

    def get_ap_connections(self):
        """Get a list of AP to client connections.

        @returns tuple of dict of connections ({'AP': 1, 'Client': 1}, ...)

        """
        # Get the closed status for relays connected to AP enclosures.
        ap_connections = self.relays_closed(self._ALL_AP_RELAYS)

        # Find out the connections
        connections = []
        for index, relay_num in enumerate(ap_connections):
            # if the relay is closed, there is a connection
            if relay_num:
                connection = {
                    'AP': (index / self._RELAYS_PER_ENCLOSURE) + 1,
                    'Client': (index % self._RELAYS_PER_ENCLOSURE) + 1
                }
                connections.append(connection)
        return tuple(connections)

    def get_client_connections(self):
        """Get a list of AP to client connections.

        @returns tuple of dict of connections ({'AP': 1, 'Client': 1}, ...)

        """
        # Get the closed status for relays connected to client enclosures.
        c_connections = self.relays_closed(self._ALL_CLIENT_RELAYS)

        # Find out the connections
        connections = []
        for index, relay_num in enumerate(c_connections):
            # if the relay is closed, there is a connection
            if relay_num:
                connection = {
                    'AP': (index % self._RELAYS_PER_ENCLOSURE) + 1,
                    'Client': (index / self._RELAYS_PER_ENCLOSURE) + 1
                }
                connections.append(connection)
        return tuple(connections)

    def connect_ap_client(self, ap, client):
        """Connect AP and a Client enclosure.

        @param ap: Int 1/2/3/4 AP enclosure index
        @param client: Int 1/2/3/4 Client enclosure index

        @raises ValueError: when ap / client value is not 1/2/3/4

        """
        if (ap < self._MIN_ENCLOSURE or ap > self._MAX_ENCLOSURE):
            raise ValueError(
                'AP enclosure should be 1/2/3/4. Given: %s' % ap)
        if (client < self._MIN_ENCLOSURE or
                client > self._MAX_ENCLOSURE):
            raise ValueError('Client enclosure should be 1/2/3/4. Given: %s' %
                             client)

        # Relay index start from 0 so subtract 1 for index
        relays = 'k1_%s,k1_%s' % (
            ((int(ap) - 1) * self._RELAYS_PER_ENCLOSURE) + int(client),
            (((int(client) - 1) + self._MAX_ENCLOSURE) *
             self._RELAYS_PER_ENCLOSURE) + int(ap))
        logging.debug('Relays to close: %s', relays)
        self.close_relays(relays)


@contextlib.contextmanager
def RfSwitchSession(host, port=scpi.Scpi.SCPI_PORT):
    """Start a RF Switch session and close it when done.

    @param host: Hostname or IP address of RF Switch
    @param port: Int SCPI port number (default 5025)

    @yields: an instance of InteractiveSsh
    """
    session = RfSwitch(host, port)
    try:
        yield session
    finally:
        session.close()
