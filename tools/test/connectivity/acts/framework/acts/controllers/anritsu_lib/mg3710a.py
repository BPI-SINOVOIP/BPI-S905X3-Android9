#!/usr/bin/env python3.4
#
#   Copyright 2016 - The Android Open Source Project
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
"""
Controller interface for Anritsu Signal Generator MG3710A.
"""

import time
import socket
from enum import Enum
from enum import IntEnum

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib._anritsu_utils import NO_ERROR
from acts.controllers.anritsu_lib._anritsu_utils import OPERATION_COMPLETE

TERMINATOR = "\n"


def create(configs, logger):
    objs = []
    for c in configs:
        ip_address = c["ip_address"]
        objs.append(MG3710A(ip_address, logger))
    return objs


def destroy(objs):
    return


class MG3710A(object):
    """Class to communicate with Anritsu Signal Generator MG3710A.
       This uses GPIB command to interface with Anritsu MG3710A """

    def __init__(self, ip_address, log_handle):
        self._ipaddr = ip_address
        self.log = log_handle

        # Open socket connection to Signaling Tester
        self.log.info("Opening Socket Connection with "
                      "Signal Generator MG3710A ({}) ".format(self._ipaddr))
        try:
            self._sock = socket.create_connection(
                (self._ipaddr, 49158), timeout=30)
            self.send_query("*IDN?", 60)
            self.log.info("Communication Signal Generator MG3710A OK.")
            self.log.info("Opened Socket connection to ({})"
                          "with handle ({})".format(self._ipaddr, self._sock))
        except socket.timeout:
            raise AnritsuError("Timeout happened while conencting to"
                               " Anritsu MG3710A")
        except socket.error:
            raise AnritsuError("Socket creation error")

    def disconnect(self):
        """ Disconnect Signal Generator MG3710A

        Args:
          None

        Returns:
            None
        """
        self.send_command(":SYST:COMM:GTL", opc=False)
        self._sock.close()

    def send_query(self, query, sock_timeout=10):
        """ Sends a Query message to Anritsu MG3710A and return response

        Args:
            query - Query string

        Returns:
            query response
        """
        self.log.info("--> {}".format(query))
        querytoSend = (query + TERMINATOR).encode('utf-8')
        self._sock.settimeout(sock_timeout)
        try:
            self._sock.send(querytoSend)
            result = self._sock.recv(256).rstrip(TERMINATOR.encode('utf-8'))
            response = result.decode('utf-8')
            self.log.info('<-- {}'.format(response))
            return response
        except socket.timeout:
            raise AnritsuError("Timeout: Response from Anritsu")
        except socket.error:
            raise AnritsuError("Socket Error")

    def send_command(self, command, sock_timeout=30, opc=True):
        """ Sends a Command message to Anritsu MG3710A

        Args:
            command - command string

        Returns:
            None
        """
        self.log.info("--> {}".format(command))
        cmdToSend = (command + TERMINATOR).encode('utf-8')
        self._sock.settimeout(sock_timeout)
        try:
            self._sock.send(cmdToSend)
            if opc:
                # check operation status
                status = self.send_query("*OPC?")
                if int(status) != OPERATION_COMPLETE:
                    raise AnritsuError("Operation not completed")
        except socket.timeout:
            raise AnritsuError("Timeout for Command Response from Anritsu")
        except socket.error:
            raise AnritsuError("Socket Error for Anritsu command")
        return

    @property
    def sg(self):
        """ Gets current selected signal generator(SG)

        Args:
            None

        Returns:
            selected signal generatr number
        """
        return self.send_query("PORT?")

    @sg.setter
    def sg(self, sg_number):
        """ Selects the signal generator to be controlled

        Args:
            sg_number: sg number 1 | 2

        Returns:
            None
        """
        cmd = "PORT {}".format(sg_number)
        self.send_command(cmd)

    def get_modulation_state(self, sg=1):
        """ Gets the RF signal modulation state (ON/OFF) of signal generator

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            modulation state . 0 (OFF) | 1(ON)
        """
        return self.send_query("OUTP{}:MOD?".format(sg))

    def set_modulation_state(self, state, sg=1):
        """ Sets the RF signal modulation state

        Args:
            sg: signal generator number.
                Default is 1
            state : ON/OFF

        Returns:
            None
        """
        cmd = "OUTP{}:MOD {}".format(sg, state)
        self.send_command(cmd)

    def get_rf_output_state(self, sg=1):
        """ Gets RF signal output state (ON/OFF) of signal generator

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            RF signal output state . 0 (OFF) | 1(ON)
        """
        return self.send_query("OUTP{}?".format(sg))

    def set_rf_output_state(self, state, sg=1):
        """ Sets the RF signal output state

        Args:
            sg: signal generator number.
                Default is 1
            state : ON/OFF

        Returns:
            None
        """
        cmd = "OUTP{} {}".format(sg, state)
        self.send_command(cmd)

    def get_frequency(self, sg=1):
        """ Gets the selected frequency of signal generator

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            selected frequency
        """
        return self.send_query("SOUR{}:FREQ?".format(sg))

    def set_frequency(self, freq, sg=1):
        """ Sets the frequency of signal generator

        Args:
            sg: signal generator number.
                Default is 1
            freq : frequency

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ {}".format(sg, freq)
        self.send_command(cmd)

    def get_frequency_offset_state(self, sg=1):
        """ Gets the Frequency Offset enable state (ON/OFF) of signal generator

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            Frequency Offset enable state . 0 (OFF) | 1(ON)
        """
        return self.send_query("SOUR{}:FREQ:OFFS:STAT?".format(sg))

    def set_frequency_offset_state(self, state, sg=1):
        """ Sets the Frequency Offset enable state

        Args:
            sg: signal generator number.
                Default is 1
            state : enable state, ON/OFF

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:OFFS:STAT {}".format(sg, state)
        self.send_command(cmd)

    def get_frequency_offset(self, sg=1):
        """ Gets the current frequency offset value

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current frequency offset value
        """
        return self.send_query("SOUR{}:FREQ:OFFS?".format(sg))

    def set_frequency_offset(self, offset, sg=1):
        """ Sets the frequency offset value

        Args:
            sg: signal generator number.
                Default is 1
            offset : frequency offset value

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:OFFS {}".format(sg, offset)
        self.send_command(cmd)

    def get_frequency_offset_multiplier_state(self, sg=1):
        """ Gets the Frequency Offset multiplier enable state (ON/OFF) of
            signal generator

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            Frequency Offset  multiplier enable state . 0 (OFF) | 1(ON)
        """
        return self.send_query("SOUR{}:FREQ:MULT:STAT?".format(sg))

    def set_frequency_offset_multiplier_state(self, state, sg=1):
        """ Sets the  Frequency Offset multiplier enable state

        Args:
            sg: signal generator number.
                Default is 1
            state : enable state, ON/OFF

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:MULT:STAT {}".format(sg, state)
        self.send_command(cmd)

    def get_frequency_offset_multiplier(self, sg=1):
        """ Gets the current frequency offset multiplier value

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            frequency offset multiplier value
        """
        return self.send_query("SOUR{}:FREQ:MULT?".format(sg))

    def set_frequency_offset_multiplier(self, multiplier, sg=1):
        """ Sets the frequency offset multiplier value

        Args:
            sg: signal generator number.
                Default is 1
            multiplier : frequency offset multiplier value

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:MULT {}".format(sg, multiplier)
        self.send_command(cmd)

    def get_channel(self, sg=1):
        """ Gets the current channel number

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current channel number
        """
        return self.send_query("SOUR{}:FREQ:CHAN:NUMB?".format(sg))

    def set_channel(self, channel, sg=1):
        """ Sets the channel number

        Args:
            sg: signal generator number.
                Default is 1
            channel : channel number

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:CHAN:NUMB {}".format(sg, channel)
        self.send_command(cmd)

    def get_channel_group(self, sg=1):
        """ Gets the current channel group number

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current channel group number
        """
        return self.send_query("SOUR{}:FREQ:CHAN:GRO?".format(sg))

    def set_channel_group(self, group, sg=1):
        """ Sets the channel group number

        Args:
            sg: signal generator number.
                Default is 1
            group : channel group number

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:CHAN:GRO {}".format(sg, group)
        self.send_command(cmd)

    def get_rf_output_level(self, sg=1):
        """ Gets the current RF output level

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current RF output level
        """
        return self.send_query("SOUR{}:POW:CURR?".format(sg))

    def get_output_level_unit(self, sg=1):
        """ Gets the current RF output level unit

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current RF output level unit
        """
        return self.send_query("UNIT{}:POW?".format(sg))

    def set_output_level_unit(self, unit, sg=1):
        """ Sets the RF output level unit

        Args:
            sg: signal generator number.
                Default is 1
            unit : Output level unit

        Returns:
            None
        """
        cmd = "UNIT{}:POW {}".format(sg, unit)
        self.send_command(cmd)

    def get_output_level(self, sg=1):
        """ Gets the Output level

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            Output level
        """
        return self.send_query("SOUR{}:POW?".format(sg))

    def set_output_level(self, level, sg=1):
        """ Sets the Output level

        Args:
            sg: signal generator number.
                Default is 1
            level : Output level

        Returns:
            None
        """
        cmd = "SOUR{}:POW {}".format(sg, level)
        self.send_command(cmd)

    def get_arb_state(self, sg=1):
        """ Gets the ARB function state

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            ARB function state . 0 (OFF) | 1(ON)
        """
        return self.send_query("SOUR{}:RAD:ARB?".format(sg))

    def set_arb_state(self, state, sg=1):
        """ Sets the ARB function state

        Args:
            sg: signal generator number.
                Default is 1
            state : enable state (ON/OFF)

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB {}".format(sg, state)
        self.send_command(cmd)

    def restart_arb_waveform_pattern(self, sg=1):
        """ playback the waveform pattern from the beginning.

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:WAV:REST".format(sg)
        self.send_command(cmd)

    def load_waveform(self, package_name, pattern_name, memory, sg=1):
        """ loads the waveform from HDD to specified memory

        Args:
            sg: signal generator number.
                Default is 1
            package_name : Package name of signal
            pattern_name : Pattern name of signal
            memory: memory for the signal - "A" or "B"

        Returns:
            None
        """
        cmd = "MMEM{}:LOAD:WAV:WM{} '{}','{}'".format(sg, memory, package_name,
                                                      pattern_name)
        self.send_command(cmd)

    def select_waveform(self, package_name, pattern_name, memory, sg=1):
        """ Selects the waveform to output on specified memory

        Args:
            sg: signal generator number.
                Default is 1
            package_name : Package name of signal
            pattern_name : Pattern name of signal
            memory: memory for the signal - "A" or "B"

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:WM{}:WAV '{}','{}'".format(
            sg, memory, package_name, pattern_name)
        self.send_command(cmd)

    def get_freq_relative_display_status(self, sg=1):
        """ Gets the frequency relative display status

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            frequency relative display status.   0 (OFF) | 1(ON)
        """
        return self.send_query("SOUR{}:FREQ:REF:STAT?".format(sg))

    def set_freq_relative_display_status(self, enable, sg=1):
        """ Sets frequency relative display status

        Args:
            sg: signal generator number.
                Default is 1
            enable : enable type (ON/OFF)

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:REF:STAT {}".format(sg, enable)
        self.send_command(cmd)

    def get_freq_channel_display_type(self, sg=1):
        """ Gets the selected type(frequency/channel) for input display

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            selected type(frequecy/channel) for input display
        """
        return self.send_query("SOUR{}:FREQ:TYPE?".format(sg))

    def set_freq_channel_display_type(self, freq_channel, sg=1):
        """ Sets thes type(frequency/channel) for input display

        Args:
            sg: signal generator number.
                Default is 1
            freq_channel : display type (frequency/channel)

        Returns:
            None
        """
        cmd = "SOUR{}:FREQ:TYPE {}".format(sg, freq_channel)
        self.send_command(cmd)

    def get_arb_combination_mode(self, sg=1):
        """ Gets the current mode to generate the pattern

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            current mode to generate the pattern
        """
        return self.send_query("SOUR{}:RAD:ARB:PCOM?".format(sg))

    def set_arb_combination_mode(self, mode, sg=1):
        """ Sets the mode to generate the pattern

        Args:
            sg: signal generator number.
                Default is 1
            mode : pattern generation mode

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:PCOM {}".format(sg, mode)
        self.send_command(cmd)

    def get_arb_pattern_aorb_state(self, a_or_b, sg=1):
        """ Gets the Pattern A/B output state

        Args:
            sg: signal generator number.
                Default is 1
            a_or_b : Patten A or Pattern B( "A" or "B")

        Returns:
            Pattern A/B output state . 0(OFF) | 1(ON)
        """
        return self.send_query("SOUR{}:RAD:ARB:WM{}:OUTP?".format(a_or_b, sg))

    def set_arb_pattern_aorb_state(self, a_or_b, state, sg=1):
        """ Sets the Pattern A/B output state

        Args:
            sg: signal generator number.
                Default is 1
            a_or_b : Patten A or Pattern B( "A" or "B")
            state : output state

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:WM{}:OUTP {}".format(sg, a_or_b, state)
        self.send_command(cmd)

    def get_arb_level_aorb(self, a_or_b, sg=1):
        """ Gets the Pattern A/B output level

        Args:
            sg: signal generator number.
                Default is 1
            a_or_b : Patten A or Pattern B( "A" or "B")

        Returns:
             Pattern A/B output level
        """
        return self.send_query("SOUR{}:RAD:ARB:WM{}:POW?".format(sg, a_or_b))

    def set_arb_level_aorb(self, a_or_b, level, sg=1):
        """ Sets the Pattern A/B output level

        Args:
            sg: signal generator number.
                Default is 1
            a_or_b : Patten A or Pattern B( "A" or "B")
            level : output level

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:WM{}:POW {}".format(sg, a_or_b, level)
        self.send_command(cmd)

    def get_arb_freq_offset(self, sg=1):
        """ Gets the frequency offset between Pattern A and Patten B
            when CenterSignal is A or B.

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            frequency offset between Pattern A and Patten B
        """
        return self.send_query("SOUR{}:RAD:ARB:FREQ:OFFS?".format(sg))

    def set_arb_freq_offset(self, offset, sg=1):
        """ Sets the frequency offset between Pattern A and Patten B when
            CenterSignal is A or B.

        Args:
            sg: signal generator number.
                Default is 1
            offset : frequency offset

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:FREQ:OFFS {}".format(sg, offset)
        self.send_command(cmd)

    def get_arb_freq_offset_aorb(self, sg=1):
        """ Gets the frequency offset of Pattern A/Pattern B based on Baseband
            center frequency

        Args:
            sg: signal generator number.
                Default is 1

        Returns:
            frequency offset
        """
        return self.send_query(
            "SOUR{}:RAD:ARB:WM{}:FREQ:OFFS?".format(sg, a_or_b))

    def set_arb_freq_offset_aorb(self, a_or_b, offset, sg=1):
        """ Sets the frequency offset of Pattern A/Pattern B based on Baseband
            center frequency

        Args:
            sg: signal generator number.
                Default is 1
            a_or_b : Patten A or Pattern B( "A" or "B")
            offset : frequency offset

        Returns:
            None
        """
        cmd = "SOUR{}:RAD:ARB:WM{}:FREQ:OFFS {}".format(sg, a_or_b, offset)
        self.send_command(cmd)
