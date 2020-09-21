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
Controller interface for Anritsu Signalling Tester MD8475A.
"""

import time
import socket
from enum import Enum
from enum import IntEnum

from acts.controllers.anritsu_lib._anritsu_utils import AnritsuError
from acts.controllers.anritsu_lib._anritsu_utils import AnritsuUtils
from acts.controllers.anritsu_lib._anritsu_utils import NO_ERROR
from acts.controllers.anritsu_lib._anritsu_utils import OPERATION_COMPLETE

TERMINATOR = "\0"
# The following wait times (except COMMUNICATION_STATE_WAIT_TIME) are actually
# the times for socket to time out. Increasing them is to make sure there is
# enough time for MD8475A operation to be completed in some cases.
# It won't increase test execution time.
SMARTSTUDIO_LAUNCH_WAIT_TIME = 300  # was 90
SMARTSTUDIO_SIMULATION_START_WAIT_TIME = 300  # was 120
REGISTRATION_STATE_WAIT_TIME = 240
LOAD_SIMULATION_PARAM_FILE_WAIT_TIME = 30
COMMUNICATION_STATE_WAIT_TIME = 240
ANRITSU_SOCKET_BUFFER_SIZE = 8192
COMMAND_COMPLETE_WAIT_TIME = 180  # was 90
SETTLING_TIME = 1
WAIT_TIME_IDENTITY_RESPONSE = 5

IMSI_READ_USERDATA_WCDMA = "081501"
IMEI_READ_USERDATA_WCDMA = "081502"
IMEISV_READ_USERDATA_WCDMA = "081503"
IMSI_READ_USERDATA_LTE = "075501"
IMEI_READ_USERDATA_LTE = "075502"
IMEISV_READ_USERDATA_LTE = "075503"
IMSI_READ_USERDATA_GSM = "081501"
IMEI_READ_USERDATA_GSM = "081502"
IMEISV_READ_USERDATA_GSM = "081503"
IDENTITY_REQ_DATA_LEN = 24
SEQ_LOG_MESSAGE_START_INDEX = 60

WCDMA_BANDS = {
    "I": "1",
    "II": "2",
    "III": "3",
    "IV": "4",
    "V": "5",
    "VI": "6",
    "VII": "7",
    "VIII": "8",
    "IX": "9",
    "X": "10",
    "XI": "11",
    "XII": "12",
    "XIII": "13",
    "XIV": "14"
}


def create(configs, logger):
    objs = []
    for c in configs:
        ip_address = c["ip_address"]
        objs.append(MD8475A(ip_address, logger))
    return objs


def destroy(objs):
    return


class ProcessingStatus(Enum):
    ''' MD8475A processing status for UE,Packet,Voice,Video,SMS,
        PPP, PWS '''
    PROCESS_STATUS_NONE = "NONE"
    PROCESS_STATUS_NOTRUN = "NOTRUN"
    PROCESS_STATUS_POWEROFF = "POWEROFF"
    PROCESS_STATUS_REGISTRATION = "REGISTRATION"
    PROCESS_STATUS_DETACH = "DETACH"
    PROCESS_STATUS_IDLE = "IDLE"
    PROCESS_STATUS_ORIGINATION = "ORIGINATION"
    PROCESS_STATUS_HANDOVER = "HANDOVER"
    PROCESS_STATUS_UPDATING = "UPDATING"
    PROCESS_STATUS_TERMINATION = "TERMINATION"
    PROCESS_STATUS_COMMUNICATION = "COMMUNICATION"
    PROCESS_STATUS_UERELEASE = "UERELEASE"
    PROCESS_STATUS_NWRELEASE = "NWRELEASE"


class BtsNumber(Enum):
    '''ID number for MD8475A supported BTS '''
    BTS1 = "BTS1"
    BTS2 = "BTS2"
    BTS3 = "BTS3"
    BTS4 = "BTS4"


class BtsTechnology(Enum):
    ''' BTS system technology'''
    LTE = "LTE"
    WCDMA = "WCDMA"
    TDSCDMA = "TDSCDMA"
    GSM = "GSM"
    CDMA1X = "CDMA1X"
    EVDO = "EVDO"


class BtsBandwidth(Enum):
    ''' Values for Cell Bandwidth '''
    LTE_BANDWIDTH_1dot4MHz = "1.4MHz"
    LTE_BANDWIDTH_3MHz = "3MHz"
    LTE_BANDWIDTH_5MHz = "5MHz"
    LTE_BANDWIDTH_10MHz = "10MHz"
    LTE_BANDWIDTH_15MHz = "15MHz"
    LTE_BANDWIDTH_20MHz = "20MHz"


class BtsPacketRate(Enum):
    ''' Values for Cell Packet rate '''
    LTE_MANUAL = "MANUAL"
    LTE_BESTEFFORT = "BESTEFFORT"
    WCDMA_DLHSAUTO_REL7_UL384K = "DLHSAUTO_REL7_UL384K"
    WCDMA_DL18_0M_UL384K = "DL18_0M_UL384K"
    WCDMA_DL21_6M_UL384K = "DL21_6M_UL384K"
    WCDMA_DLHSAUTO_REL7_ULHSAUTO = "DLHSAUTO_REL7_ULHSAUTO"
    WCDMA_DL18_0M_UL1_46M = "DL18_0M_UL1_46M"
    WCDMA_DL18_0M_UL2_0M = "DL18_0M_UL2_0M"
    WCDMA_DL18_0M_UL5_76M = "DL18_0M_UL5_76M"
    WCDMA_DL21_6M_UL1_46M = "DL21_6M_UL1_46M"
    WCDMA_DL21_6M_UL2_0M = "DL21_6M_UL2_0M"
    WCDMA_DL21_6M_UL5_76M = "DL21_6M_UL5_76M"
    WCDMA_DLHSAUTO_REL8_UL384K = "DLHSAUTO_REL8_UL384K"
    WCDMA_DL23_4M_UL384K = "DL23_4M_UL384K"
    WCDMA_DL28_0M_UL384K = "DL28_0M_UL384K"
    WCDMA_DL36_0M_UL384K = "DL36_0M_UL384K"
    WCDMA_DL43_2M_UL384K = "DL43_2M_UL384K"
    WCDMA_DLHSAUTO_REL8_ULHSAUTO = "DLHSAUTO_REL8_ULHSAUTO"
    WCDMA_DL23_4M_UL1_46M = "DL23_4M_UL1_46M"
    WCDMA_DL23_4M_UL2_0M = "DL23_4M_UL2_0M"
    WCDMA_DL23_4M_UL5_76M = "DL23_4M_UL5_76M"
    WCDMA_DL28_0M_UL1_46M = "DL28_0M_UL1_46M"
    WCDMA_DL28_0M_UL2_0M = "DL28_0M_UL2_0M"
    WCDMA_DL28_0M_UL5_76M = "L28_0M_UL5_76M"
    WCDMA_DL36_0M_UL1_46M = "DL36_0M_UL1_46M"
    WCDMA_DL36_0M_UL2_0M = "DL36_0M_UL2_0M"
    WCDMA_DL36_0M_UL5_76M = "DL36_0M_UL5_76M"
    WCDMA_DL43_2M_UL1_46M = "DL43_2M_UL1_46M"
    WCDMA_DL43_2M_UL2_0M = "DL43_2M_UL2_0M"
    WCDMA_DL43_2M_UL5_76M = "L43_2M_UL5_76M"


class BtsPacketWindowSize(Enum):
    ''' Values for Cell Packet window size '''
    WINDOW_SIZE_1 = 1
    WINDOW_SIZE_8 = 8
    WINDOW_SIZE_16 = 16
    WINDOW_SIZE_32 = 32
    WINDOW_SIZE_64 = 64
    WINDOW_SIZE_128 = 128
    WINDOW_SIZE_256 = 256
    WINDOW_SIZE_512 = 512
    WINDOW_SIZE_768 = 768
    WINDOW_SIZE_1024 = 1024
    WINDOW_SIZE_1536 = 1536
    WINDOW_SIZE_2047 = 2047


class BtsServiceState(Enum):
    ''' Values for BTS service state '''
    SERVICE_STATE_IN = "IN"
    SERVICE_STATE_OUT = "OUT"


class BtsCellBarred(Enum):
    ''' Values for Cell barred parameter '''
    NOTBARRED = "NOTBARRED"
    BARRED = "BARRED"


class BtsAccessClassBarred(Enum):
    ''' Values for Access class barred parameter '''
    NOTBARRED = "NOTBARRED"
    EMERGENCY = "EMERGENCY"
    BARRED = "BARRED"
    USERSPECIFIC = "USERSPECIFIC"


class BtsLteEmergencyAccessClassBarred(Enum):
    ''' Values for Lte emergency access class barred parameter '''
    NOTBARRED = "NOTBARRED"
    BARRED = "BARRED"


class BtsNwNameEnable(Enum):
    ''' Values for BT network name enable parameter '''
    NAME_ENABLE = "ON"
    NAME_DISABLE = "OFF"


class IPAddressType(Enum):
    ''' Values for IP address type '''
    IPV4 = "IPV4"
    IPV6 = "IPV6"
    IPV4V6 = "IPV4V6"


class TriggerMessageIDs(Enum):
    ''' ID for Trigger messages  '''
    RRC_CONNECTION_REQ = 111101
    RRC_CONN_REESTABLISH_REQ = 111100
    ATTACH_REQ = 141141
    DETACH_REQ = 141145
    MM_LOC_UPDATE_REQ = 221108
    GMM_ATTACH_REQ = 241101
    GMM_RA_UPDATE_REQ = 241108
    IDENTITY_REQUEST_LTE = 141155
    IDENTITY_REQUEST_WCDMA = 241115
    IDENTITY_REQUEST_GSM = 641115


class TriggerMessageReply(Enum):
    ''' Values for Trigger message reply parameter '''
    ACCEPT = "ACCEPT"
    REJECT = "REJECT"
    IGNORE = "IGNORE"
    NONE = "NONE"
    ILLEGAL = "ILLEGAL"


class TestProcedure(Enum):
    ''' Values for different Test procedures in MD8475A '''
    PROCEDURE_BL = "BL"
    PROCEDURE_SELECTION = "SELECTION"
    PROCEDURE_RESELECTION = "RESELECTION"
    PROCEDURE_REDIRECTION = "REDIRECTION"
    PROCEDURE_HO = "HO"
    PROCEDURE_HHO = "HHO"
    PROCEDURE_SHO = "SHO"
    PROCEDURE_MEASUREMENT = "MEASUREMENT"
    PROCEDURE_CELLCHANGE = "CELLCHANGE"
    PROCEDURE_MULTICELL = "MULTICELL"


class TestPowerControl(Enum):
    ''' Values for power control in test procedure '''
    POWER_CONTROL_ENABLE = "ENABLE"
    POWER_CONTROL_DISABLE = "DISABLE"


class TestMeasurement(Enum):
    ''' Values for mesaurement in test procedure '''
    MEASUREMENT_ENABLE = "ENABLE"
    MEASUREMENT_DISABLE = "DISABLE"


'''MD8475A processing states'''
_PROCESS_STATES = {
    "NONE": ProcessingStatus.PROCESS_STATUS_NONE,
    "NOTRUN": ProcessingStatus.PROCESS_STATUS_NOTRUN,
    "POWEROFF": ProcessingStatus.PROCESS_STATUS_POWEROFF,
    "REGISTRATION": ProcessingStatus.PROCESS_STATUS_REGISTRATION,
    "DETACH": ProcessingStatus.PROCESS_STATUS_DETACH,
    "IDLE": ProcessingStatus.PROCESS_STATUS_IDLE,
    "ORIGINATION": ProcessingStatus.PROCESS_STATUS_ORIGINATION,
    "HANDOVER": ProcessingStatus.PROCESS_STATUS_HANDOVER,
    "UPDATING": ProcessingStatus.PROCESS_STATUS_UPDATING,
    "TERMINATION": ProcessingStatus.PROCESS_STATUS_TERMINATION,
    "COMMUNICATION": ProcessingStatus.PROCESS_STATUS_COMMUNICATION,
    "UERELEASE": ProcessingStatus.PROCESS_STATUS_UERELEASE,
    "NWRELEASE": ProcessingStatus.PROCESS_STATUS_NWRELEASE,
}


class ImsCscfStatus(Enum):
    """ MD8475A ims cscf status for UE
    """
    OFF = "OFF"
    SIPIDLE = "SIPIDLE"
    CONNECTED = "CONNECTED"
    CALLING = "CALLING"
    RINGING = "RINGING"
    UNKNOWN = "UNKNOWN"


class ImsCscfCall(Enum):
    """ MD8475A ims cscf call action
    """
    MAKE = "MAKE"
    END = "END"
    MAKEVIDEO = "MAKEVIDEO"
    MAKE2ND = "MAKE2ND"
    END2ND = "END2ND"
    ANSWER = "ANSWER"
    HOLD = "HOLD"
    RESUME = "RESUME"


class VirtualPhoneStatus(IntEnum):
    ''' MD8475A virtual phone status for UE voice and UE video
        PPP, PWS '''
    STATUS_IDLE = 0
    STATUS_VOICECALL_ORIGINATION = 1
    STATUS_VOICECALL_INCOMING = 2
    STATUS_VOICECALL_INPROGRESS = 3
    STATUS_VOICECALL_DISCONNECTING = 4
    STATUS_VOICECALL_DISCONNECTED = 5
    STATUS_VIDEOCALL_ORIGINATION = 6
    STATUS_VIDEOCALL_INCOMING = 7
    STATUS_VIDEOCALL_INPROGRESS = 8
    STATUS_VIDEOCALL_DISCONNECTING = 9
    STATUS_VIDEOCALL_DISCONNECTED = 10


'''Virtual Phone Status '''
_VP_STATUS = {
    "0": VirtualPhoneStatus.STATUS_IDLE,
    "1": VirtualPhoneStatus.STATUS_VOICECALL_ORIGINATION,
    "2": VirtualPhoneStatus.STATUS_VOICECALL_INCOMING,
    "3": VirtualPhoneStatus.STATUS_VOICECALL_INPROGRESS,
    "4": VirtualPhoneStatus.STATUS_VOICECALL_DISCONNECTING,
    "5": VirtualPhoneStatus.STATUS_VOICECALL_DISCONNECTED,
    "6": VirtualPhoneStatus.STATUS_VIDEOCALL_ORIGINATION,
    "7": VirtualPhoneStatus.STATUS_VIDEOCALL_INCOMING,
    "8": VirtualPhoneStatus.STATUS_VIDEOCALL_INPROGRESS,
    "9": VirtualPhoneStatus.STATUS_VIDEOCALL_DISCONNECTING,
    "10": VirtualPhoneStatus.STATUS_VIDEOCALL_DISCONNECTED,
}


class VirtualPhoneAutoAnswer(Enum):
    ''' Virtual phone auto answer enable values'''
    ON = "ON"
    OFF = "OFF"


class CsfbType(Enum):
    ''' CSFB Type values'''
    CSFB_TYPE_REDIRECTION = "REDIRECTION"
    CSFB_TYPE_HANDOVER = "HO"


class ReturnToEUTRAN(Enum):
    '''Return to EUTRAN setting values '''
    RETEUTRAN_ENABLE = "ENABLE"
    RETEUTRAN_DISABLE = "DISABLE"


class CTCHSetup(Enum):
    '''CTCH setting values '''
    CTCH_ENABLE = "ENABLE"
    CTCH_DISABLE = "DISABLE"


class UEIdentityType(Enum):
    '''UE Identity type values '''
    IMSI = "IMSI"
    IMEI = "IMEI"
    IMEISV = "IMEISV"


class CBCHSetup(Enum):
    '''CBCH setting values '''
    CBCH_ENABLE = "ENABLE"
    CBCH_DISABLE = "DISABLE"


class Switch(Enum):
    ''' Values for ENABLE or DISABLE '''
    ENABLE = "ENABLE"
    DISABLE = "DISABLE"


class MD8475A(object):
    """Class to communicate with Anritsu MD8475A Signalling Tester.
       This uses GPIB command to interface with Anritsu MD8475A """

    def __init__(self, ip_address, log_handle, wlan=False):
        self._error_reporting = True
        self._ipaddr = ip_address
        self.log = log_handle
        self._wlan = wlan

        # Open socket connection to Signaling Tester
        self.log.info("Opening Socket Connection with "
                      "Signaling Tester ({}) ".format(self._ipaddr))
        try:
            self._sock = socket.create_connection(
                (self._ipaddr, 28002), timeout=120)
            self.send_query("*IDN?", 60)
            self.log.info("Communication with Signaling Tester OK.")
            self.log.info("Opened Socket connection to ({})"
                          "with handle ({})".format(self._ipaddr, self._sock))
            # launching Smart Studio Application needed for the simulation
            ret = self.launch_smartstudio()
        except socket.timeout:
            raise AnritsuError("Timeout happened while conencting to"
                               " Anritsu MD8475A")
        except socket.error:
            raise AnritsuError("Socket creation error")

    def get_BTS(self, btsnumber):
        """ Returns the BTS object based on the BTS number provided

        Args:
            btsnumber: BTS number (BTS1, BTS2)

        Returns:
            BTS object
        """
        return _BaseTransceiverStation(self, btsnumber)

    def get_AnritsuTestCases(self):
        """ Returns the Anritsu Test Case Module Object

        Args:
            None

        Returns:
            Anritsu Test Case Module Object
        """
        return _AnritsuTestCases(self)

    def get_VirtualPhone(self):
        """ Returns the Anritsu Virtual Phone Module Object

        Args:
            None

        Returns:
            Anritsu Virtual Phone Module Object
        """
        return _VirtualPhone(self)

    def get_PDN(self, pdn_number):
        """ Returns the PDN Module Object

        Args:
            None

        Returns:
            Anritsu PDN Module Object
        """
        return _PacketDataNetwork(self, pdn_number)

    def get_TriggerMessage(self):
        """ Returns the Anritsu Trigger Message Module Object

        Args:
            None

        Returns:
            Anritsu Trigger Message Module Object
        """
        return _TriggerMessage(self)

    def get_IMS(self, vnid):
        """ Returns the IMS Module Object with VNID

        Args:
            vnid: Virtual Network ID

        Returns:
            Anritsu IMS VNID Module Object
        """
        return _IMS_Services(self, vnid)

    def get_ims_cscf_status(self, virtual_network_id):
        """ Get the IMS CSCF Status of virtual network

        Args:
            virtual_network_id: virtual network id

        Returns:
            IMS CSCF status
        """
        cmd = "IMSCSCFSTAT? {}".format(virtual_network_id)
        return self.send_query(cmd)

    def ims_cscf_call_action(self, virtual_network_id, action):
        """ IMS CSCF Call action

        Args:
            virtual_network_id: virtual network id
            action: action to make

        Returns:
            None
        """
        cmd = "IMSCSCFCALL {},{}".format(virtual_network_id, action)
        self.send_command(cmd)

    def send_query(self, query, sock_timeout=10):
        """ Sends a Query message to Anritsu and return response

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
            result = self._sock.recv(ANRITSU_SOCKET_BUFFER_SIZE).rstrip(
                TERMINATOR.encode('utf-8'))
            response = result.decode('utf-8')
            self.log.info('<-- {}'.format(response))
            return response
        except socket.timeout:
            raise AnritsuError("Timeout: Response from Anritsu")
        except socket.error:
            raise AnritsuError("Socket Error")

    def send_command(self, command, sock_timeout=20):
        """ Sends a Command message to Anritsu

        Args:
            command - command string

        Returns:
            None
        """
        self.log.info("--> {}".format(command))
        if self._error_reporting:
            cmdToSend = (command + ";ERROR?" + TERMINATOR).encode('utf-8')
            self._sock.settimeout(sock_timeout)
            try:
                self._sock.send(cmdToSend)
                err = self._sock.recv(ANRITSU_SOCKET_BUFFER_SIZE).rstrip(
                    TERMINATOR.encode('utf-8'))
                error = int(err.decode('utf-8'))
                if error != NO_ERROR:
                    raise AnritsuError(error, command)
            except socket.timeout:
                raise AnritsuError("Timeout for Command Response from Anritsu")
            except socket.error:
                raise AnritsuError("Socket Error for Anritsu command")
            except Exception as e:
                raise AnritsuError(e, command)
        else:
            cmdToSend = (command + TERMINATOR).encode('utf-8')
            try:
                self._sock.send(cmdToSend)
            except socket.error:
                raise AnritsuError("Socket Error", command)
            return

    def launch_smartstudio(self):
        """ launch the Smart studio application
            This should be done before stating simulation

        Args:
            None

        Returns:
            None
        """
        # check the Smart Studio status . If Smart Studio doesn't exist ,
        # start it.if it is running, stop it. Smart Studio should be in
        # NOTRUN (Simulation Stopped) state to start new simulation
        stat = self.send_query("STAT?", 30)
        if stat == "NOTEXIST":
            self.log.info("Launching Smart Studio Application,"
                          "it takes about a minute.")
            time_to_wait = SMARTSTUDIO_LAUNCH_WAIT_TIME
            sleep_interval = 15
            waiting_time = 0

            err = self.send_command("RUN", SMARTSTUDIO_LAUNCH_WAIT_TIME)
            stat = self.send_query("STAT?")
            while stat != "NOTRUN":
                time.sleep(sleep_interval)
                waiting_time = waiting_time + sleep_interval
                if waiting_time <= time_to_wait:
                    stat = self.send_query("STAT?")
                else:
                    raise AnritsuError("Timeout: Smart Studio launch")
        elif stat == "RUNNING":
            # Stop simulation if necessary
            self.send_command("STOP", 60)
            stat = self.send_query("STAT?")

        # The state of the Smart Studio should be NOTRUN at this point
        # after the one of the steps from above
        if stat != "NOTRUN":
            self.log.info(
                "Can not launch Smart Studio, "
                "please shut down all the Smart Studio SW components")
            raise AnritsuError("Could not run SmartStudio")

    def close_smartstudio(self):
        """ Closes the Smart studio application

        Args:
            None

        Returns:
            None
        """
        self.stop_simulation()
        self.send_command("EXIT", 60)

    def get_smartstudio_status(self):
        """ Gets the Smart studio status

        Args:
            None

        Returns:
            Smart studio status
        """
        return self.send_query("STAT?")

    def start_simulation(self):
        """ Starting the simulation of the network model.
            simulation model or simulation parameter file
            should be set before starting the simulation

        Args:
          None

        Returns:
            None
        """
        time_to_wait = SMARTSTUDIO_SIMULATION_START_WAIT_TIME
        sleep_interval = 2
        waiting_time = 0

        self.send_command("START", SMARTSTUDIO_SIMULATION_START_WAIT_TIME)

        self.log.info("Waiting for CALLSTAT=POWEROFF")
        callstat = self.send_query("CALLSTAT? BTS1").split(",")
        while callstat[0] != "POWEROFF":
            time.sleep(sleep_interval)
            waiting_time += sleep_interval
            if waiting_time <= time_to_wait:
                callstat = self.send_query("CALLSTAT? BTS1").split(",")
            else:
                raise AnritsuError("Timeout: Starting simulation")

    def stop_simulation(self):
        """ Stop simulation operation

        Args:
          None

        Returns:
            None
        """
        # Stop virtual network (IMS) #1 if still running
        # this is needed before Sync command is supported in 6.40a
        if self.send_query("IMSVNSTAT? 1") == "RUNNING":
            self.send_command("IMSSTOPVN 1")
        if self.send_query("IMSVNSTAT? 2") == "RUNNING":
            self.send_command("IMSSTOPVN 2")
        stat = self.send_query("STAT?")
        # Stop simulation if its is RUNNING
        if stat == "RUNNING":
            self.send_command("STOP", 60)
            stat = self.send_query("STAT?")
            if stat != "NOTRUN":
                self.log.info("Failed to stop simulation")
                raise AnritsuError("Failed to stop simulation")

    def reset(self):
        """ reset simulation parameters

        Args:
          None

        Returns:
            None
        """
        self.send_command("*RST", COMMAND_COMPLETE_WAIT_TIME)

    def load_simulation_paramfile(self, filepath):
        """ loads simulation model parameter file
        Args:
          filepath : simulation model parameter file path

        Returns:
            None
        """
        self.stop_simulation()
        cmd = "LOADSIMPARAM \"" + filepath + '\";ERROR?'
        self.send_query(cmd, LOAD_SIMULATION_PARAM_FILE_WAIT_TIME)

    def load_cell_paramfile(self, filepath):
        """ loads cell model parameter file

        Args:
          filepath : cell model parameter file path

        Returns:
            None
        """
        self.stop_simulation()
        cmd = "LOADCELLPARAM \"" + filepath + '\";ERROR?'
        status = int(self.send_query(cmd))
        if status != NO_ERROR:
            raise AnritsuError(status, cmd)

    def _set_simulation_model(self, sim_model):
        """ Set simulation model and valid the configuration

        Args:
            sim_model: simulation model

        Returns:
            True/False
        """
        error = int(
            self.send_query("SIMMODEL %s;ERROR?" % sim_model,
                            COMMAND_COMPLETE_WAIT_TIME))
        if error:  # Try again if first set SIMMODEL fails
            time.sleep(3)
            if "WLAN" in sim_model:
                new_sim_model = sim_model[:-5]
                error = int(
                    self.send_query("SIMMODEL %s;ERROR?" % new_sim_model,
                                    COMMAND_COMPLETE_WAIT_TIME))
                time.sleep(3)
            error = int(
                self.send_query("SIMMODEL %s;ERROR?" % sim_model,
                                COMMAND_COMPLETE_WAIT_TIME))
            if error:
                return False
        # Reset every time after SIMMODEL is set because SIMMODEL will load
        # some of the contents from previous parameter files.
        self.reset()
        return True

    def set_simulation_model(self, bts1, bts2=None, bts3=None, bts4=None):
        """ Sets the simulation model

        Args:
            bts1 - BTS1 RAT
            bts1 - BTS2 RAT
            bts3 - Not used now
            bts4 - Not used now

        Returns:
            True or False
        """
        self.stop_simulation()
        simmodel = bts1.value
        if bts2 is not None:
            simmodel = simmodel + "," + bts2.value
        if self._wlan:
            simmodel = simmodel + "," + "WLAN"
        return self._set_simulation_model(simmodel)

    def get_simulation_model(self):
        """ Gets the simulation model

        Args:
            None

        Returns:
            Current simulation model
        """
        cmd = "SIMMODEL?"
        return self.send_query(cmd)

    def set_simulation_state_to_poweroff(self):
        """ Sets the simulation state to POWER OFF

        Args:
          None

        Returns:
            None
        """
        self.send_command("RESETSIMULATION POWEROFF")
        time_to_wait = 30
        sleep_interval = 2
        waiting_time = 0

        self.log.info("Waiting for CALLSTAT=POWEROFF")
        callstat = self.send_query("CALLSTAT?").split(",")
        while callstat[0] != "POWEROFF":
            time.sleep(sleep_interval)
            waiting_time = waiting_time + sleep_interval
            if waiting_time <= time_to_wait:
                callstat = self.send_query("CALLSTAT?").split(",")
            else:
                break

    def set_simulation_state_to_idle(self, btsnumber):
        """ Sets the simulation state to IDLE

        Args:
          None

        Returns:
            None
        """
        if not isinstance(btsnumber, BtsNumber):
            raise ValueError(' The parameter should be of type "BtsNumber" ')
        cmd = "RESETSIMULATION IDLE," + btsnumber.value
        self.send_command(cmd)
        time_to_wait = 30
        sleep_interval = 2
        waiting_time = 0

        self.log.info("Waiting for CALLSTAT=IDLE")
        callstat = self.send_query("CALLSTAT?").split(",")
        while callstat[0] != "IDLE":
            time.sleep(sleep_interval)
            waiting_time = waiting_time + sleep_interval
            if waiting_time <= time_to_wait:
                callstat = self.send_query("CALLSTAT?").split(",")
            else:
                break

    def wait_for_registration_state(self, bts=1):
        """ Waits for UE registration state on Anritsu

        Args:
          bts: index of MD8475A BTS, eg 1, 2

        Returns:
            None
        """
        self.log.info("wait for IDLE/COMMUNICATION state on anritsu.")
        time_to_wait = REGISTRATION_STATE_WAIT_TIME
        sleep_interval = 1
        sim_model = (self.get_simulation_model()).split(",")
        #wait 1 more round for GSM because of PS attach
        registration_check_iterations = 2 if sim_model[bts - 1] == "GSM" else 1
        for _ in range(registration_check_iterations):
            waiting_time = 0
            while waiting_time <= time_to_wait:
                callstat = self.send_query(
                    "CALLSTAT? BTS{}".format(bts)).split(",")
                if callstat[0] == "IDLE" or callstat[1] == "COMMUNICATION":
                    break
                time.sleep(sleep_interval)
                waiting_time += sleep_interval
            else:
                raise AnritsuError(
                    "UE failed to register in {} seconds".format(time_to_wait))
            time.sleep(sleep_interval)

    def wait_for_communication_state(self):
        """ Waits for UE communication state on Anritsu

        Args:
          None

        Returns:
            None
        """
        self.log.info("wait for COMMUNICATION state on anritsu")
        time_to_wait = COMMUNICATION_STATE_WAIT_TIME
        sleep_interval = 1
        waiting_time = 0

        self.log.info("Waiting for CALLSTAT=COMMUNICATION")
        callstat = self.send_query("CALLSTAT? BTS1").split(",")
        while callstat[1] != "COMMUNICATION":
            time.sleep(sleep_interval)
            waiting_time += sleep_interval
            if waiting_time <= time_to_wait:
                callstat = self.send_query("CALLSTAT? BTS1").split(",")
            else:
                raise AnritsuError("UE failed to register on network")

    def get_camping_cell(self):
        """ Gets the current camping cell information

        Args:
          None

        Returns:
            returns a tuple (BTS number, RAT Technology) '
        """
        bts_number, rat_info = self.send_query("CAMPINGCELL?").split(",")
        return bts_number, rat_info

    def get_supported_bands(self, rat):
        """ Gets the supported bands from UE capability information

        Args:
          rat: LTE or WCDMA

        Returns:
            returns a list of bnads
        """
        cmd = "UEINFO? "
        if rat == "LTE":
            cmd += "L"
        elif rat == "WCDMA":
            cmd += "W"
        else:
            raise ValueError('The rat argument needs to be "LTE" or "WCDMA"')
        cmd += "_SupportedBand"
        result = self.send_query(cmd).split(",")
        if result == "NONE":
            return None
        if rat == "WCDMA":
            bands = []
            for band in result:
                bands.append(WCDMA_BANDS[band])
            return bands
        else:
            return result

    def start_testcase(self):
        """ Starts a test case on Anritsu

        Args:
          None

        Returns:
            None
        """
        self.send_command("STARTTEST")

    def get_testcase_status(self):
        """ Gets the current test case status on Anritsu

        Args:
          None

        Returns:
            current test case status
        """
        return self.send_query("TESTSTAT?")

    @property
    def gateway_ipv4addr(self):
        """ Gets the IPv4 address of the default gateway

        Args:
          None

        Returns:
            current UE status
        """
        return self.send_query("DGIPV4?")

    @gateway_ipv4addr.setter
    def gateway_ipv4addr(self, ipv4_addr):
        """ sets the IPv4 address of the default gateway
        Args:
            ipv4_addr: IPv4 address of the default gateway

        Returns:
            None
        """
        cmd = "DGIPV4 " + ipv4_addr
        self.send_command(cmd)

    @property
    def usim_key(self):
        """ Gets the USIM Security Key

        Args:
          None

        Returns:
            USIM Security Key
        """
        return self.send_query("USIMK?")

    @usim_key.setter
    def usim_key(self, usimk):
        """ sets the USIM Security Key
        Args:
            usimk: USIM Security Key, eg "000102030405060708090A0B0C0D0E0F"

        Returns:
            None
        """
        cmd = "USIMK " + usimk
        self.send_command(cmd)

    def get_ue_status(self):
        """ Gets the current UE status on Anritsu

        Args:
          None

        Returns:
            current UE status
        """
        UE_STATUS_INDEX = 0
        ue_status = self.send_query("CALLSTAT?").split(",")[UE_STATUS_INDEX]
        return _PROCESS_STATES[ue_status]

    def get_packet_status(self):
        """ Gets the current Packet status on Anritsu

        Args:
          None

        Returns:
            current Packet status
        """
        PACKET_STATUS_INDEX = 1
        packet_status = self.send_query("CALLSTAT?").split(",")[
            PACKET_STATUS_INDEX]
        return _PROCESS_STATES[packet_status]

    def disconnect(self):
        """ Disconnect the Anritsu box from test PC

        Args:
          None

        Returns:
            None
        """
        # no need to # exit smart studio application
        # self.close_smartstudio()
        self._sock.close()

    def machine_reboot(self):
        """ Reboots the Anritsu Machine

        Args:
          None

        Returns:
            None
        """
        self.send_command("REBOOT")

    def save_sequence_log(self, fileName):
        """ Saves the Anritsu Sequence logs to file

        Args:
          fileName: log file name

        Returns:
            None
        """
        cmd = 'SAVESEQLOG "{}"'.format(fileName)
        self.send_command(cmd)

    def clear_sequence_log(self):
        """ Clears the Anritsu Sequence logs

        Args:
          None

        Returns:
            None
        """
        self.send_command("CLEARSEQLOG")

    def save_message_log(self, fileName):
        """ Saves the Anritsu Message logs to file

        Args:
          fileName: log file name

        Returns:
            None
        """
        cmd = 'SAVEMSGLOG "{}"'.format(fileName)
        self.send_command(cmd)

    def clear_message_log(self):
        """ Clears the Anritsu Message logs

        Args:
          None

        Returns:
            None
        """
        self.send_command("CLEARMSGLOG")

    def save_trace_log(self, fileName, fileType, overwrite, start, end):
        """ Saves the Anritsu Trace logs

        Args:
          fileName: log file name
          fileType: file type (BINARY, TEXT, H245,PACKET, CPLABE)
          overwrite: whether to over write
          start: starting trace number
          end: ending trace number

        Returns:
            None
        """
        cmd = 'SAVETRACELOG "{}",{},{},{},{}'.format(fileName, fileType,
                                                     overwrite, start, end)
        self.send_command(cmd)

    def send_cmas_lte_wcdma(self, serialNo, messageID, warningMessage):
        """ Sends a CMAS message

        Args:
          serialNo: serial number of CMAS message
          messageID: CMAS message ID
          warningMessage:  CMAS Warning message

        Returns:
            None
        """
        cmd = ('PWSSENDWM 3GPP,"BtsNo=1&WarningSystem=CMAS&SerialNo={}'
               '&MessageID={}&wm={}"').format(serialNo, messageID,
                                              warningMessage)
        self.send_command(cmd)

    def send_etws_lte_wcdma(self, serialNo, messageID, warningType,
                            warningMessage, userAlertenable, popUpEnable):
        """ Sends a ETWS message

        Args:
          serialNo: serial number of CMAS message
          messageID: CMAS message ID
          warningMessage:  CMAS Warning message

        Returns:
            None
        """
        cmd = (
            'PWSSENDWM 3GPP,"BtsNo=1&WarningSystem=ETWS&SerialNo={}&'
            'Primary=ON&PrimaryMessageID={}&Secondary=ON&SecondaryMessageID={}'
            '&WarningType={}&wm={}&UserAlert={}&Popup={}&dcs=0x10&LanguageCode=en"'
        ).format(serialNo, messageID, messageID, warningType, warningMessage,
                 userAlertenable, popUpEnable)
        self.send_command(cmd)

    def send_cmas_etws_cdma1x(self, message_id, service_category, alert_ext,
                              response_type, severity, urgency, certainty):
        """ Sends a CMAS/ETWS message on CDMA 1X

        Args:
          serviceCategory: service category of alert
          messageID: message ID
          alertText: Warning message

        Returns:
            None
        """
        cmd = (
            'PWSSENDWM 3GPP2,"BtsNo=1&ServiceCategory={}&MessageID={}&AlertText={}&'
            'CharSet=ASCII&ResponseType={}&Severity={}&Urgency={}&Certainty={}"'
        ).format(service_category, message_id, alert_ext, response_type,
                 severity, urgency, certainty)
        self.send_command(cmd)

    @property
    def csfb_type(self):
        """ Gets the current CSFB type

        Args:
            None

        Returns:
            current CSFB type
        """
        return self.send_query("SIMMODELEX? CSFB")

    @csfb_type.setter
    def csfb_type(self, csfb_type):
        """ sets the CSFB type
        Args:
            csfb_type: CSFB type

        Returns:
            None
        """
        if not isinstance(csfb_type, CsfbType):
            raise ValueError('The parameter should be of type "CsfbType" ')
        cmd = "SIMMODELEX CSFB," + csfb_type.value
        self.send_command(cmd)

    @property
    def csfb_return_to_eutran(self):
        """ Gets the current return to EUTRAN status

        Args:
            None

        Returns:
            current return to EUTRAN status
        """
        return self.send_query("SIMMODELEX? RETEUTRAN")

    @csfb_return_to_eutran.setter
    def csfb_return_to_eutran(self, enable):
        """ sets the return to EUTRAN feature
        Args:
            enable: enable/disable return to EUTRAN feature

        Returns:
            None
        """
        if not isinstance(enable, ReturnToEUTRAN):
            raise ValueError(
                'The parameter should be of type "ReturnToEUTRAN"')
        cmd = "SIMMODELEX RETEUTRAN," + enable.value
        self.send_command(cmd)

    def set_packet_preservation(self):
        """ Set packet state to Preservation

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEPACKET PRESERVATION"
        self.send_command(cmd)

    def set_packet_dormant(self):
        """ Set packet state to Dormant

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEPACKET DORMANT"
        self.send_command(cmd)

    def get_ue_identity(self, identity_type):
        """ Get the UE identity IMSI, IMEI, IMEISV

        Args:
            identity_type : IMSI/IMEI/IMEISV

        Returns:
            IMSI/IMEI/IMEISV value
        """
        bts, rat = self.get_camping_cell()
        if rat == BtsTechnology.LTE.value:
            identity_request = TriggerMessageIDs.IDENTITY_REQUEST_LTE.value
            if identity_type == UEIdentityType.IMSI:
                userdata = IMSI_READ_USERDATA_LTE
            elif identity_type == UEIdentityType.IMEI:
                userdata = IMEI_READ_USERDATA_LTE
            elif identity_type == UEIdentityType.IMEISV:
                userdata = IMEISV_READ_USERDATA_LTE
            else:
                return None
        elif rat == BtsTechnology.WCDMA.value:
            identity_request = TriggerMessageIDs.IDENTITY_REQUEST_WCDMA.value
            if identity_type == UEIdentityType.IMSI:
                userdata = IMSI_READ_USERDATA_WCDMA
            elif identity_type == UEIdentityType.IMEI:
                userdata = IMEI_READ_USERDATA_WCDMA
            elif identity_type == UEIdentityType.IMEISV:
                userdata = IMEISV_READ_USERDATA_WCDMA
            else:
                return None
        elif rat == BtsTechnology.GSM.value:
            identity_request = TriggerMessageIDs.IDENTITY_REQUEST_GSM.value
            if identity_type == UEIdentityType.IMSI:
                userdata = IMSI_READ_USERDATA_GSM
            elif identity_type == UEIdentityType.IMEI:
                userdata = IMEI_READ_USERDATA_GSM
            elif identity_type == UEIdentityType.IMEISV:
                userdata = IMEISV_READ_USERDATA_GSM
            else:
                return None
        else:
            return None

        self.send_command("TMMESSAGEMODE {},USERDATA".format(identity_request))
        time.sleep(SETTLING_TIME)
        self.send_command("TMUSERDATA {}, {}, {}".format(
            identity_request, userdata, IDENTITY_REQ_DATA_LEN))
        time.sleep(SETTLING_TIME)
        self.send_command("TMSENDUSERMSG {}".format(identity_request))
        time.sleep(WAIT_TIME_IDENTITY_RESPONSE)
        # Go through sequence log and find the identity response message
        target = '"{}"'.format(identity_type.value)
        seqlog = self.send_query("SEQLOG?").split(",")
        while (target not in seqlog):
            index = int(seqlog[0]) - 1
            if index < SEQ_LOG_MESSAGE_START_INDEX:
                self.log.error("Can not find " + target)
                return None
            seqlog = self.send_query("SEQLOG? %d" % index).split(",")
        return (seqlog[-1])

    def select_usim(self, usim):
        """ Select pre-defined Anritsu USIM models

        Args:
            usim: any of P0035Bx, P0135Ax, P0250Ax, P0260Ax

        Returns:
            None
        """
        cmd = "SELECTUSIM {}".format(usim)
        self.send_command(cmd)


class _AnritsuTestCases(object):
    '''Class to interact with the MD8475 supported test procedures '''

    def __init__(self, anritsu):
        self._anritsu = anritsu
        self.log = anritsu.log

    @property
    def procedure(self):
        """ Gets the current Test Procedure type

        Args:
            None

        Returns:
            One of TestProcedure type values
        """
        return self._anritsu.send_query("TESTPROCEDURE?")

    @procedure.setter
    def procedure(self, procedure):
        """ sets the Test Procedure type
        Args:
            procedure: One of TestProcedure type values

        Returns:
            None
        """
        if not isinstance(procedure, TestProcedure):
            raise ValueError(
                'The parameter should be of type "TestProcedure" ')
        cmd = "TESTPROCEDURE " + procedure.value
        self._anritsu.send_command(cmd)

    @property
    def bts_direction(self):
        """ Gets the current Test direction

         Args:
            None

        Returns:
            Current Test direction eg:BTS2,BTS1
        """
        return self._anritsu.send_query("TESTBTSDIRECTION?")

    @bts_direction.setter
    def bts_direction(self, direction):
        """ sets the Test direction  eg: BTS1 to BTS2 '''

        Args:
            direction: tuple (from-bts,to_bts) of type BtsNumber

        Returns:
            None
        """
        if not isinstance(direction, tuple) or len(direction) is not 2:
            raise ValueError("Pass a tuple with two items")
        from_bts, to_bts = direction
        if (isinstance(from_bts, BtsNumber) and isinstance(to_bts, BtsNumber)):
            cmd = "TESTBTSDIRECTION {},{}".format(from_bts.value, to_bts.value)
            self._anritsu.send_command(cmd)
        else:
            raise ValueError(' The parameters should be of type "BtsNumber" ')

    @property
    def registration_timeout(self):
        """ Gets the current Test registration timeout

        Args:
            None

        Returns:
            Current test registration timeout value
        """
        return self._anritsu.send_query("TESTREGISTRATIONTIMEOUT?")

    @registration_timeout.setter
    def registration_timeout(self, timeout_value):
        """ sets the Test registration timeout value
        Args:
            timeout_value: test registration timeout value

        Returns:
            None
        """
        cmd = "TESTREGISTRATIONTIMEOUT " + str(timeout_value)
        self._anritsu.send_command(cmd)

    @property
    def power_control(self):
        """ Gets the power control enabled/disabled status for test case

        Args:
            None

        Returns:
            current power control enabled/disabled status
        """
        return self._anritsu.send_query("TESTPOWERCONTROL?")

    @power_control.setter
    def power_control(self, enable):
        """ Sets the power control enabled/disabled status for test case

        Args:
            enable:  enabled/disabled

        Returns:
            None
        """
        if not isinstance(enable, TestPowerControl):
            raise ValueError(' The parameter should be of type'
                             ' "TestPowerControl" ')
        cmd = "TESTPOWERCONTROL " + enable.value
        self._anritsu.send_command(cmd)

    @property
    def measurement_LTE(self):
        """ Checks measurement status for LTE test case

        Args:
            None

        Returns:
            Enabled/Disabled
        """
        return self._anritsu.send_query("TESTMEASUREMENT? LTE")

    @measurement_LTE.setter
    def measurement_LTE(self, enable):
        """ Sets the measurement enabled/disabled status for LTE test case

        Args:
            enable:  enabled/disabled

        Returns:
            None
        """
        if not isinstance(enable, TestMeasurement):
            raise ValueError(' The parameter should be of type'
                             ' "TestMeasurement" ')
        cmd = "TESTMEASUREMENT LTE," + enable.value
        self._anritsu.send_command(cmd)

    @property
    def measurement_WCDMA(self):
        """ Checks measurement status for WCDMA test case

        Args:
            None

        Returns:
            Enabled/Disabled
        """
        return self._anritsu.send_query("TESTMEASUREMENT? WCDMA")

    @measurement_WCDMA.setter
    def measurement_WCDMA(self, enable):
        """ Sets the measurement enabled/disabled status for WCDMA test case

        Args:
            enable:  enabled/disabled

        Returns:
            None
        """
        if not isinstance(enable, TestMeasurement):
            raise ValueError(' The parameter should be of type'
                             ' "TestMeasurement" ')
        cmd = "TESTMEASUREMENT WCDMA," + enable.value
        self._anritsu.send_command(cmd)

    @property
    def measurement_TDSCDMA(self):
        """ Checks measurement status for TDSCDMA test case

        Args:
            None

        Returns:
            Enabled/Disabled
        """
        return self._anritsu.send_query("TESTMEASUREMENT? TDSCDMA")

    @measurement_TDSCDMA.setter
    def measurement_WCDMA(self, enable):
        """ Sets the measurement enabled/disabled status for TDSCDMA test case

        Args:
            enable:  enabled/disabled

        Returns:
            None
        """
        if not isinstance(enable, TestMeasurement):
            raise ValueError(' The parameter should be of type'
                             ' "TestMeasurement" ')
        cmd = "TESTMEASUREMENT TDSCDMA," + enable.value
        self._anritsu.send_command(cmd)

    def set_pdn_targeteps(self, pdn_order, pdn_number=1):
        """ Sets PDN to connect as a target when performing the
           test case for packet handover

        Args:
            pdn_order:  PRIORITY/USER
            pdn_number: Target PDN number

        Returns:
            None
        """
        cmd = "TESTPDNTARGETEPS " + pdn_order
        if pdn_order == "USER":
            cmd = cmd + "," + str(pdn_number)
        self._anritsu.send_command(cmd)


class _BaseTransceiverStation(object):
    '''Class to interact different BTS supported by MD8475 '''

    def __init__(self, anritsu, btsnumber):
        if not isinstance(btsnumber, BtsNumber):
            raise ValueError(' The parameter should be of type "BtsNumber" ')
        self._bts_number = btsnumber.value
        self._anritsu = anritsu
        self.log = anritsu.log

    @property
    def output_level(self):
        """ Gets the Downlink power of the cell

        Args:
            None

        Returns:
            DL Power level
        """
        cmd = "OLVL? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @output_level.setter
    def output_level(self, level):
        """ Sets the Downlink power of the cell

        Args:
            level: Power level

        Returns:
            None
        """
        counter = 1
        while float(level) != float(self.output_level):
            if counter > 3:
                raise AnritsuError("Fail to set output level in 3 tries!")
            cmd = "OLVL {},{}".format(level, self._bts_number)
            self._anritsu.send_command(cmd)
            counter += 1
            time.sleep(1)

    @property
    def input_level(self):
        """ Gets the reference power of the cell

        Args:
            None

        Returns:
            Reference Power level
        """
        cmd = "RFLVL? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @input_level.setter
    def input_level(self, level):
        """ Sets the reference power of the cell

        Args:
            level: Power level

        Returns:
            None
        """
        counter = 1
        while float(level) != float(self.input_level):
            if counter > 3:
                raise AnritsuError("Fail to set intput level in 3 tries!")
            cmd = "RFLVL {},{}".format(level, self._bts_number)
            self._anritsu.send_command(cmd)
            counter += 1
            time.sleep(1)

    @property
    def band(self):
        """ Gets the Band of the cell

        Args:
            None

        Returns:
            Cell band
        """
        cmd = "BAND? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @band.setter
    def band(self, band):
        """ Sets the Band of the cell

        Args:
            band: Band of the cell

        Returns:
            None
        """
        cmd = "BAND {},{}".format(band, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def transmode(self):
        """ Gets the Transmission Mode of the cell

        Args:
            None

        Returns:
            Transmission mode
        """
        cmd = "TRANSMODE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @transmode.setter
    def transmode(self, tm_mode):
        """ Sets the TM of the cell

        Args:
            TM: TM of the cell

        Returns:
            None
        """
        cmd = "TRANSMODE {},{}".format(tm_mode, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def dl_antenna(self):
        """ Gets the DL ANTENNA count of the cell

        Args:
            None

        Returns:
            No of DL Antenna
        """
        cmd = "ANTENNAS? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @dl_antenna.setter
    def dl_antenna(self, num_antenna):
        """ Sets the DL ANTENNA of the cell

        Args:
            c: DL ANTENNA of the cell

        Returns:
            None
        """
        cmd = "ANTENNAS {},{}".format(num_antenna, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def bandwidth(self):
        """ Gets the channel bandwidth of the cell

        Args:
            None

        Returns:
            channel bandwidth
        """
        cmd = "BANDWIDTH? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @bandwidth.setter
    def bandwidth(self, bandwidth):
        """ Sets the channel bandwidth of the cell

        Args:
            bandwidth: channel bandwidth  of the cell

        Returns:
            None
        """
        if not isinstance(bandwidth, BtsBandwidth):
            raise ValueError(' The parameter should be of type "BtsBandwidth"')
        cmd = "BANDWIDTH {},{}".format(bandwidth.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def dl_bandwidth(self):
        """ Gets the downlink bandwidth of the cell

        Args:
            None

        Returns:
            downlink bandwidth
        """
        cmd = "DLBANDWIDTH? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @dl_bandwidth.setter
    def dl_bandwidth(self, bandwidth):
        """ Sets the downlink bandwidth of the cell

        Args:
            bandwidth: downlink bandwidth of the cell

        Returns:
            None
        """
        if not isinstance(bandwidth, BtsBandwidth):
            raise ValueError(' The parameter should be of type "BtsBandwidth"')
        cmd = "DLBANDWIDTH {},{}".format(bandwidth.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def ul_bandwidth(self):
        """ Gets the uplink bandwidth of the cell

        Args:
            None

        Returns:
            uplink bandwidth
        """
        cmd = "ULBANDWIDTH? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @ul_bandwidth.setter
    def ul_bandwidth(self, bandwidth):
        """ Sets the uplink bandwidth of the cell

        Args:
            bandwidth: uplink bandwidth of the cell

        Returns:
            None
        """
        if not isinstance(bandwidth, BtsBandwidth):
            raise ValueError(
                ' The parameter should be of type "BtsBandwidth" ')
        cmd = "ULBANDWIDTH {},{}".format(bandwidth.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def packet_rate(self):
        """ Gets the packet rate of the cell

        Args:
            None

        Returns:
            packet rate
        """
        cmd = "PACKETRATE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @packet_rate.setter
    def packet_rate(self, packetrate):
        """ Sets the packet rate of the cell

        Args:
            packetrate: packet rate of the cell

        Returns:
            None
        """
        if not isinstance(packetrate, BtsPacketRate):
            raise ValueError(' The parameter should be of type'
                             ' "BtsPacketRate" ')
        cmd = "PACKETRATE {},{}".format(packetrate.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def ul_windowsize(self):
        """ Gets the uplink window size of the cell

        Args:
            None

        Returns:
            uplink window size
        """
        cmd = "ULWINSIZE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @ul_windowsize.setter
    def ul_windowsize(self, windowsize):
        """ Sets the uplink window size of the cell

        Args:
            windowsize: uplink window size of the cell

        Returns:
            None
        """
        if not isinstance(windowsize, BtsPacketWindowSize):
            raise ValueError(' The parameter should be of type'
                             ' "BtsPacketWindowSize" ')
        cmd = "ULWINSIZE {},{}".format(windowsize.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def dl_windowsize(self):
        """ Gets the downlink window size of the cell

        Args:
            None

        Returns:
            downlink window size
        """
        cmd = "DLWINSIZE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @dl_windowsize.setter
    def dl_windowsize(self, windowsize):
        """ Sets the downlink window size of the cell

        Args:
            windowsize: downlink window size of the cell

        Returns:
            None
        """
        if not isinstance(windowsize, BtsPacketWindowSize):
            raise ValueError(' The parameter should be of type'
                             ' "BtsPacketWindowSize" ')
        cmd = "DLWINSIZE {},{}".format(windowsize.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def service_state(self):
        """ Gets the service state of BTS

        Args:
            None

        Returns:
            service state IN/OUT
        """
        cmd = "OUTOFSERVICE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @service_state.setter
    def service_state(self, service_state):
        """ Sets the service state of BTS

        Args:
            service_state: service state of BTS , IN/OUT

        Returns:
            None
        """
        if not isinstance(service_state, BtsServiceState):
            raise ValueError(' The parameter should be of type'
                             ' "BtsServiceState" ')
        cmd = "OUTOFSERVICE {},{}".format(service_state.value,
                                          self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def cell_barred(self):
        """ Gets the Cell Barred state of the cell

        Args:
            None

        Returns:
            one of BtsCellBarred value
        """
        cmd = "CELLBARRED?" + self._bts_number
        return self._anritsu.send_query(cmd)

    @cell_barred.setter
    def cell_barred(self, barred_option):
        """ Sets the Cell Barred state of the cell

        Args:
            barred_option: Cell Barred state of the cell

        Returns:
            None
        """
        if not isinstance(barred_option, BtsCellBarred):
            raise ValueError(' The parameter should be of type'
                             ' "BtsCellBarred" ')
        cmd = "CELLBARRED {},{}".format(barred_option.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def accessclass_barred(self):
        """ Gets the Access Class Barred state of the cell

        Args:
            None

        Returns:
            one of BtsAccessClassBarred value
        """
        cmd = "ACBARRED? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @accessclass_barred.setter
    def accessclass_barred(self, barred_option):
        """ Sets the Access Class Barred state of the cell

        Args:
            barred_option: Access Class Barred state of the cell

        Returns:
            None
        """
        if not isinstance(barred_option, BtsAccessClassBarred):
            raise ValueError(' The parameter should be of type'
                             ' "BtsAccessClassBarred" ')
        cmd = "ACBARRED {},{}".format(barred_option.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def lteemergency_ac_barred(self):
        """ Gets the LTE emergency Access Class Barred state of the cell

        Args:
            None

        Returns:
            one of BtsLteEmergencyAccessClassBarred value
        """
        cmd = "LTEEMERGENCYACBARRED? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @lteemergency_ac_barred.setter
    def lteemergency_ac_barred(self, barred_option):
        """ Sets the LTE emergency Access Class Barred state of the cell

        Args:
            barred_option: Access Class Barred state of the cell

        Returns:
            None
        """
        if not isinstance(barred_option, BtsLteEmergencyAccessClassBarred):
            raise ValueError(' The parameter should be of type'
                             ' "BtsLteEmergencyAccessClassBarred" ')
        cmd = "LTEEMERGENCYACBARRED {},{}".format(barred_option.value,
                                                  self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def mcc(self):
        """ Gets the MCC of the cell

        Args:
            None

        Returns:
            MCC of the cell
        """
        cmd = "MCC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @mcc.setter
    def mcc(self, mcc_code):
        """ Sets the MCC of the cell

        Args:
            mcc_code: MCC of the cell

        Returns:
            None
        """
        cmd = "MCC {},{}".format(mcc_code, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def mnc(self):
        """ Gets the MNC of the cell

        Args:
            None

        Returns:
            MNC of the cell
        """
        cmd = "MNC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @mnc.setter
    def mnc(self, mnc_code):
        """ Sets the MNC of the cell

        Args:
            mnc_code: MNC of the cell

        Returns:
            None
        """
        cmd = "MNC {},{}".format(mnc_code, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nw_fullname_enable(self):
        """ Gets the network full name enable status

        Args:
            None

        Returns:
            one of BtsNwNameEnable value
        """
        cmd = "NWFNAMEON? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nw_fullname_enable.setter
    def nw_fullname_enable(self, enable):
        """ Sets the network full name enable status

        Args:
            enable: network full name enable status

        Returns:
            None
        """
        if not isinstance(enable, BtsNwNameEnable):
            raise ValueError(' The parameter should be of type'
                             ' "BtsNwNameEnable" ')
        cmd = "NWFNAMEON {},{}".format(enable.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nw_fullname(self):
        """ Gets the network full name

        Args:
            None

        Returns:
            Network fulll name
        """
        cmd = "NWFNAME? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nw_fullname.setter
    def nw_fullname(self, fullname):
        """ Sets the network full name

        Args:
            fullname: network full name

        Returns:
            None
        """
        cmd = "NWFNAME {},{}".format(fullname, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nw_shortname_enable(self):
        """ Gets the network short name enable status

        Args:
            None

        Returns:
            one of BtsNwNameEnable value
        """
        cmd = "NWSNAMEON? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nw_shortname_enable.setter
    def nw_shortname_enable(self, enable):
        """ Sets the network short name enable status

        Args:
            enable: network short name enable status

        Returns:
            None
        """
        if not isinstance(enable, BtsNwNameEnable):
            raise ValueError(' The parameter should be of type'
                             ' "BtsNwNameEnable" ')
        cmd = "NWSNAMEON {},{}".format(enable.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nw_shortname(self):
        """ Gets the network short name

        Args:
            None

        Returns:
            Network short name
        """
        cmd = "NWSNAME? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nw_shortname.setter
    def nw_shortname(self, shortname):
        """ Sets the network short name

        Args:
            shortname: network short name

        Returns:
            None
        """
        cmd = "NWSNAME {},{}".format(shortname, self._bts_number)
        self._anritsu.send_command(cmd)

    def apply_parameter_changes(self):
        """ apply the parameter changes at run time

        Args:
            None

        Returns:
            None
        """
        cmd = "APPLYPARAM"
        self._anritsu.send_command(cmd)

    @property
    def wcdma_ctch(self):
        """ Gets the WCDMA CTCH enable/disable status

        Args:
            None

        Returns:
            one of CTCHSetup values
        """
        cmd = "CTCHPARAMSETUP? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @wcdma_ctch.setter
    def wcdma_ctch(self, enable):
        """ Sets the WCDMA CTCH enable/disable status

        Args:
            enable: WCDMA CTCH enable/disable status

        Returns:
            None
        """
        cmd = "CTCHPARAMSETUP {},{}".format(enable.value, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def lac(self):
        """ Gets the Location Area Code of the cell

        Args:
            None

        Returns:
            LAC value
        """
        cmd = "LAC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @lac.setter
    def lac(self, lac):
        """ Sets the Location Area Code of the cell

        Args:
            lac: Location Area Code of the cell

        Returns:
            None
        """
        cmd = "LAC {},{}".format(lac, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def rac(self):
        """ Gets the Routing Area Code of the cell

        Args:
            None

        Returns:
            RAC value
        """
        cmd = "RAC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @rac.setter
    def rac(self, rac):
        """ Sets the Routing Area Code of the cell

        Args:
            rac: Routing Area Code of the cell

        Returns:
            None
        """
        cmd = "RAC {},{}".format(rac, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def dl_channel(self):
        """ Gets the downlink channel number of the cell

        Args:
            None

        Returns:
            RAC value
        """
        cmd = "DLCHAN? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @dl_channel.setter
    def dl_channel(self, channel):
        """ Sets the downlink channel number of the cell

        Args:
            channel: downlink channel number of the cell

        Returns:
            None
        """
        cmd = "DLCHAN {},{}".format(channel, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_mcc(self):
        """ Gets the sector 1 MCC of the CDMA cell

        Args:
            None

        Returns:
            sector 1 mcc
        """
        cmd = "S1MCC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_mcc.setter
    def sector1_mcc(self, mcc):
        """ Sets the sector 1 MCC of the CDMA cell

        Args:
            mcc: sector 1 MCC of the CDMA cell

        Returns:
            None
        """
        cmd = "S1MCC {},{}".format(mcc, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_sid(self):
        """ Gets the sector 1 system ID of the CDMA cell

        Args:
            None

        Returns:
            sector 1 system Id
        """
        cmd = "S1SID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_sid.setter
    def sector1_sid(self, sid):
        """ Sets the sector 1 system ID of the CDMA cell

        Args:
            sid: sector 1 system ID of the CDMA cell

        Returns:
            None
        """
        cmd = "S1SID {},{}".format(sid, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_nid(self):
        """ Gets the sector 1 network ID of the CDMA cell

        Args:
            None

        Returns:
            sector 1 network Id
        """
        cmd = "S1NID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_nid.setter
    def sector1_nid(self, nid):
        """ Sets the sector 1 network ID of the CDMA cell

        Args:
            nid: sector 1 network ID of the CDMA cell

        Returns:
            None
        """
        cmd = "S1NID {},{}".format(nid, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_baseid(self):
        """ Gets the sector 1 Base ID of the CDMA cell

        Args:
            None

        Returns:
            sector 1 Base Id
        """
        cmd = "S1BASEID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_baseid.setter
    def sector1_baseid(self, baseid):
        """ Sets the sector 1 Base ID of the CDMA cell

        Args:
            baseid: sector 1 Base ID of the CDMA cell

        Returns:
            None
        """
        cmd = "S1BASEID {},{}".format(baseid, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_latitude(self):
        """ Gets the sector 1 latitude of the CDMA cell

        Args:
            None

        Returns:
            sector 1 latitude
        """
        cmd = "S1LATITUDE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_latitude.setter
    def sector1_latitude(self, latitude):
        """ Sets the sector 1 latitude of the CDMA cell

        Args:
            latitude: sector 1 latitude of the CDMA cell

        Returns:
            None
        """
        cmd = "S1LATITUDE {},{}".format(latitude, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def sector1_longitude(self):
        """ Gets the sector 1 longitude of the CDMA cell

        Args:
            None

        Returns:
            sector 1 longitude
        """
        cmd = "S1LONGITUDE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @sector1_longitude.setter
    def sector1_longitude(self, longitude):
        """ Sets the sector 1 longitude of the CDMA cell

        Args:
            longitude: sector 1 longitude of the CDMA cell

        Returns:
            None
        """
        cmd = "S1LONGITUDE {},{}".format(longitude, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def evdo_sid(self):
        """ Gets the Sector ID of the EVDO cell

        Args:
            None

        Returns:
            Sector Id
        """
        cmd = "S1SECTORID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @evdo_sid.setter
    def evdo_sid(self, sid):
        """ Sets the Sector ID of the EVDO cell

        Args:
            sid: Sector ID of the EVDO cell

        Returns:
            None
        """
        cmd = "S1SECTORID {},{}".format(sid, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def cell_id(self):
        """ Gets the cell identity of the cell

        Args:
            None

        Returns:
            cell identity
        """
        cmd = "CELLID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @cell_id.setter
    def cell_id(self, cell_id):
        """ Sets the cell identity of the cell

        Args:
            cell_id: cell identity of the cell

        Returns:
            None
        """
        cmd = "CELLID {},{}".format(cell_id, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def physical_cellid(self):
        """ Gets the physical cell id of the cell

        Args:
            None

        Returns:
            physical cell id
        """
        cmd = "PHYCELLID? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @physical_cellid.setter
    def physical_cellid(self, physical_cellid):
        """ Sets the physical cell id of the cell

        Args:
            physical_cellid: physical cell id of the cell

        Returns:
            None
        """
        cmd = "PHYCELLID {},{}".format(physical_cellid, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def gsm_mcs_dl(self):
        """ Gets the Modulation and Coding scheme (DL) of the GSM cell

        Args:
            None

        Returns:
            DL MCS
        """
        cmd = "DLMCS? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @gsm_mcs_dl.setter
    def gsm_mcs_dl(self, mcs_dl):
        """ Sets the Modulation and Coding scheme (DL) of the GSM cell

        Args:
            mcs_dl: Modulation and Coding scheme (DL) of the GSM cell

        Returns:
            None
        """
        cmd = "DLMCS {},{}".format(mcs_dl, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def gsm_mcs_ul(self):
        """ Gets the Modulation and Coding scheme (UL) of the GSM cell

        Args:
            None

        Returns:
            UL MCS
        """
        cmd = "ULMCS? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @gsm_mcs_ul.setter
    def gsm_mcs_ul(self, mcs_ul):
        """ Sets the Modulation and Coding scheme (UL) of the GSM cell

        Args:
            mcs_ul:Modulation and Coding scheme (UL) of the GSM cell

        Returns:
            None
        """
        cmd = "ULMCS {},{}".format(mcs_ul, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def lte_scheduling_mode(self):
        """ Gets the Scheduling mode of the LTE cell

        Args:
            None

        Returns:
            Scheduling mode
        """
        cmd = "SCHEDULEMODE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @lte_scheduling_mode.setter
    def lte_scheduling_mode(self, mode):
        """ Sets the Scheduling mode of the LTE cell

        Args:
            mode: STATIC (default) or DYNAMIC

        Returns:
            None
        """
        counter = 1
        while mode != self.lte_scheduling_mode:
            if counter > 3:
                raise AnritsuError("Fail to set scheduling mode in 3 tries!")
            cmd = "SCHEDULEMODE {},{}".format(mode, self._bts_number)
            self._anritsu.send_command(cmd)
            counter += 1
            time.sleep(1)

    @property
    def lte_mcs_dl(self):
        """ Gets the Modulation and Coding scheme (DL) of the LTE cell

        Args:
            None

        Returns:
            DL MCS
        """
        cmd = "DLIMCS? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @lte_mcs_dl.setter
    def lte_mcs_dl(self, mcs_dl):
        """ Sets the Modulation and Coding scheme (DL) of the LTE cell

        Args:
            mcs_dl: Modulation and Coding scheme (DL) of the LTE cell

        Returns:
            None
        """
        cmd = "DLIMCS {},{}".format(mcs_dl, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def lte_mcs_ul(self):
        """ Gets the Modulation and Coding scheme (UL) of the LTE cell

        Args:
            None

        Returns:
            UL MCS
        """
        cmd = "ULIMCS? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @lte_mcs_ul.setter
    def lte_mcs_ul(self, mcs_ul):
        """ Sets the Modulation and Coding scheme (UL) of the LTE cell

        Args:
            mcs_ul: Modulation and Coding scheme (UL) of the LTE cell

        Returns:
            None
        """
        cmd = "ULIMCS {},{}".format(mcs_ul, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nrb_dl(self):
        """ Gets the Downlink N Resource Block of the cell

        Args:
            None

        Returns:
            Downlink NRB
        """
        cmd = "DLNRB? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nrb_dl.setter
    def nrb_dl(self, blocks):
        """ Sets the Downlink N Resource Block of the cell

        Args:
            blocks: Downlink N Resource Block of the cell

        Returns:
            None
        """
        cmd = "DLNRB {},{}".format(blocks, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def nrb_ul(self):
        """ Gets the uplink N Resource Block of the cell

        Args:
            None

        Returns:
            uplink NRB
        """
        cmd = "ULNRB? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @nrb_ul.setter
    def nrb_ul(self, blocks):
        """ Sets the uplink N Resource Block of the cell

        Args:
            blocks: uplink N Resource Block of the cell

        Returns:
            None
        """
        cmd = "ULNRB {},{}".format(blocks, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def neighbor_cell_mode(self):
        """ Gets the neighbor cell mode

        Args:
            None

        Returns:
            current neighbor cell mode
        """
        cmd = "NCLIST? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @neighbor_cell_mode.setter
    def neighbor_cell_mode(self, mode):
        """ Sets the neighbor cell mode

        Args:
            mode: neighbor cell mode , DEFAULT/ USERDATA

        Returns:
            None
        """
        cmd = "NCLIST {},{}".format(mode, self._bts_number)
        self._anritsu.send_command(cmd)

    def get_neighbor_cell_type(self, system, index):
        """ Gets the neighbor cell type

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell type
        """
        cmd = "NCTYPE? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def set_neighbor_cell_type(self, system, index, cell_type):
        """ Sets the neighbor cell type

        Args:
            system: simulation model of neighbor cell
                   LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell
            cell_type: cell type
                BTS1, BTS2, BTS3, BTS4,CELLNAME, DISABLE

        Returns:
            None
        """
        cmd = "NCTYPE {},{},{},{}".format(system, index, cell_type,
                                          self._bts_number)
        self._anritsu.send_command(cmd)

    def get_neighbor_cell_name(self, system, index):
        """ Gets the neighbor cell name

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell name
        """
        cmd = "NCCELLNAME? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def set_neighbor_cell_name(self, system, index, name):
        """ Sets the neighbor cell name

        Args:
            system: simulation model of neighbor cell
                   LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell
            name: cell name

        Returns:
            None
        """
        cmd = "NCCELLNAME {},{},{},{}".format(system, index, name,
                                              self._bts_number)
        self._anritsu.send_command(cmd)

    def get_neighbor_cell_mcc(self, system, index):
        """ Gets the neighbor cell mcc

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell mcc
        """
        cmd = "NCMCC? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_mnc(self, system, index):
        """ Gets the neighbor cell mnc

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell mnc
        """
        cmd = "NCMNC? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_id(self, system, index):
        """ Gets the neighbor cell id

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell id
        """
        cmd = "NCCELLID? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_tac(self, system, index):
        """ Gets the neighbor cell tracking area code

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell tracking area code
        """
        cmd = "NCTAC? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_dl_channel(self, system, index):
        """ Gets the neighbor cell downlink channel

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell tracking downlink channel
        """
        cmd = "NCDLCHAN? {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_dl_bandwidth(self, system, index):
        """ Gets the neighbor cell downlink bandwidth

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell tracking downlink bandwidth
        """
        cmd = "NCDLBANDWIDTH {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_pcid(self, system, index):
        """ Gets the neighbor cell physical cell id

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell physical cell id
        """
        cmd = "NCPHYCELLID {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_lac(self, system, index):
        """ Gets the neighbor cell location area code

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell location area code
        """
        cmd = "NCLAC {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    def get_neighbor_cell_rac(self, system, index):
        """ Gets the neighbor cell routing area code

        Args:
            system: simulation model of neighbor cell
                    LTE, WCDMA, TDSCDMA, GSM, CDMA1X,EVDO
            index: Index of neighbor cell

        Returns:
            neighbor cell routing area code
        """
        cmd = "NCRAC {},{},{}".format(system, index, self._bts_number)
        return self._anritsu.send_query(cmd)

    @property
    def primary_scrambling_code(self):
        """ Gets the primary scrambling code for WCDMA cell

        Args:
            None

        Returns:
            primary scrambling code
        """
        cmd = "PRISCRCODE? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @primary_scrambling_code.setter
    def primary_scrambling_code(self, psc):
        """ Sets the primary scrambling code for WCDMA cell

        Args:
            psc: primary scrambling code

        Returns:
            None
        """
        cmd = "PRISCRCODE {},{}".format(psc, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def tac(self):
        """ Gets the Tracking Area Code of the LTE cell

        Args:
            None

        Returns:
            Tracking Area Code of the LTE cell
        """
        cmd = "TAC? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @tac.setter
    def tac(self, tac):
        """ Sets the Tracking Area Code of the LTE cell

        Args:
            tac: Tracking Area Code of the LTE cell

        Returns:
            None
        """
        cmd = "TAC {},{}".format(tac, self._bts_number)
        self._anritsu.send_command(cmd)

    @property
    def cell(self):
        """ Gets the current cell for BTS

        Args:
            None

        Returns:
            current cell for BTS
        """
        cmd = "CELLSEL? {}".format(self._bts_number)
        return self._anritsu.send_query(cmd)

    @cell.setter
    def cell(self, cell_name):
        """ sets the  cell for BTS
        Args:
            cell_name: cell name

        Returns:
            None
        """
        cmd = "CELLSEL {},{}".format(self._bts_number, cell_name)
        return self._anritsu.send_command(cmd)

    @property
    def gsm_cbch(self):
        """ Gets the GSM CBCH enable/disable status

        Args:
            None

        Returns:
            one of CBCHSetup values
        """
        cmd = "CBCHPARAMSETUP? " + self._bts_number
        return self._anritsu.send_query(cmd)

    @gsm_cbch.setter
    def gsm_cbch(self, enable):
        """ Sets the GSM CBCH enable/disable status

        Args:
            enable: GSM CBCH enable/disable status

        Returns:
            None
        """
        cmd = "CBCHPARAMSETUP {},{}".format(enable.value, self._bts_number)
        self._anritsu.send_command(cmd)


class _VirtualPhone(object):
    '''Class to interact with virtual phone supported by MD8475 '''

    def __init__(self, anritsu):
        self._anritsu = anritsu
        self.log = anritsu.log

    @property
    def id(self):
        """ Gets the virtual phone ID

        Args:
            None

        Returns:
            virtual phone ID
        """
        cmd = "VPID? "
        return self._anritsu.send_query(cmd)

    @id.setter
    def id(self, phonenumber):
        """ Sets the virtual phone ID

        Args:
            phonenumber: virtual phone ID

        Returns:
            None
        """
        cmd = "VPID {}".format(phonenumber)
        self._anritsu.send_command(cmd)

    @property
    def id_c2k(self):
        """ Gets the virtual phone ID for CDMA 1x

        Args:
            None

        Returns:
            virtual phone ID
        """
        cmd = "VPIDC2K? "
        return self._anritsu.send_query(cmd)

    @id_c2k.setter
    def id_c2k(self, phonenumber):
        """ Sets the virtual phone ID for CDMA 1x

        Args:
            phonenumber: virtual phone ID

        Returns:
            None
        """
        cmd = "VPIDC2K {}".format(phonenumber)
        self._anritsu.send_command(cmd)

    @property
    def auto_answer(self):
        """ Gets the auto answer status of virtual phone

        Args:
            None

        Returns:
            auto answer status, ON/OFF
        """
        cmd = "VPAUTOANSWER? "
        return self._anritsu.send_query(cmd)

    @auto_answer.setter
    def auto_answer(self, option):
        """ Sets the auto answer feature

        Args:
            option: tuple with two items for turning on Auto Answer
                    (OFF or (ON, timetowait))

        Returns:
            None
        """
        enable = "OFF"
        time = 5

        try:
            enable, time = option
        except ValueError:
            if enable != "OFF":
                raise ValueError("Pass a tuple with two items for"
                                 " Turning on Auto Answer")
        cmd = "VPAUTOANSWER {},{}".format(enable.value, time)
        self._anritsu.send_command(cmd)

    @property
    def calling_mode(self):
        """ Gets the calling mode of virtual phone

        Args:
            None

        Returns:
            calling mode of virtual phone
        """
        cmd = "VPCALLINGMODE? "
        return self._anritsu.send_query(cmd)

    @calling_mode.setter
    def calling_mode(self, calling_mode):
        """ Sets the calling mode of virtual phone

        Args:
            calling_mode: calling mode of virtual phone

        Returns:
            None
        """
        cmd = "VPCALLINGMODE {}".format(calling_mode)
        self._anritsu.send_command(cmd)

    def set_voice_off_hook(self):
        """ Set the virtual phone operating mode to Voice Off Hook

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEVPHONE 0"
        return self._anritsu.send_command(cmd)

    def set_voice_on_hook(self):
        """ Set the virtual phone operating mode to Voice On Hook

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEVPHONE 1"
        return self._anritsu.send_command(cmd)

    def set_video_off_hook(self):
        """ Set the virtual phone operating mode to Video Off Hook

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEVPHONE 2"
        return self._anritsu.send_command(cmd)

    def set_video_on_hook(self):
        """ Set the virtual phone operating mode to Video On Hook

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEVPHONE 3"
        return self._anritsu.send_command(cmd)

    def set_call_waiting(self):
        """ Set the virtual phone operating mode to Call waiting

        Args:
            None

        Returns:
            None
        """
        cmd = "OPERATEVPHONE 4"
        return self._anritsu.send_command(cmd)

    @property
    def status(self):
        """ Gets the virtual phone status

        Args:
            None

        Returns:
            virtual phone status
        """
        cmd = "VPSTAT?"
        status = self._anritsu.send_query(cmd)
        return _VP_STATUS[status]

    def sendSms(self, phoneNumber, message):
        """ Sends the SMS data from Anritsu to UE

        Args:
            phoneNumber: sender of SMS
            message: message text

        Returns:
            None
        """
        cmd = ("SENDSMS /?PhoneNumber=001122334455&Sender={}&Text={}"
               "&DCS=00").format(phoneNumber, AnritsuUtils.gsm_encode(message))
        return self._anritsu.send_command(cmd)

    def sendSms_c2k(self, phoneNumber, message):
        """ Sends the SMS data from Anritsu to UE (in CDMA)

        Args:
            phoneNumber: sender of SMS
            message: message text

        Returns:
            None
        """
        cmd = ("C2KSENDSMS System=CDMA\&Originating_Address={}\&UserData={}"
               ).format(phoneNumber, AnritsuUtils.cdma_encode(message))
        return self._anritsu.send_command(cmd)

    def receiveSms(self):
        """ Receives SMS messages sent by the UE in an external application

        Args:
            None

        Returns:
            None
        """
        return self._anritsu.send_query("RECEIVESMS?")

    def receiveSms_c2k(self):
        """ Receives SMS messages sent by the UE(in CDMA) in an external application

        Args:
            None

        Returns:
            None
        """
        return self._anritsu.send_query("C2KRECEIVESMS?")

    def setSmsStatusReport(self, status):
        """ Set the Status Report value of the SMS

        Args:
            status: status code

        Returns:
            None
        """
        cmd = "SMSSTATUSREPORT {}".format(status)
        return self._anritsu.send_command(cmd)


class _PacketDataNetwork(object):
    '''Class to configure PDN parameters'''

    def __init__(self, anritsu, pdnnumber):
        self._pdn_number = pdnnumber
        self._anritsu = anritsu
        self.log = anritsu.log

    @property
    def ue_address_iptype(self):
        """ Gets IP type of UE for particular PDN

        Args:
            None

        Returns:
            IP type of UE for particular PDN
        """
        cmd = "PDNIPTYPE? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @ue_address_iptype.setter
    def ue_address_iptype(self, ip_type):
        """ Set IP type of UE for particular PDN

        Args:
            ip_type: IP type of UE

        Returns:
            None
        """
        if not isinstance(ip_type, IPAddressType):
            raise ValueError(
                ' The parameter should be of type "IPAddressType"')
        cmd = "PDNIPTYPE {},{}".format(self._pdn_number, ip_type.value)
        self._anritsu.send_command(cmd)

    @property
    def ue_address_ipv4(self):
        """ Gets UE IPv4 address

        Args:
            None

        Returns:
            UE IPv4 address
        """
        cmd = "PDNIPV4? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @ue_address_ipv4.setter
    def ue_address_ipv4(self, ip_address):
        """ Set UE IPv4 address

        Args:
            ip_address: UE IPv4 address

        Returns:
            None
        """
        cmd = "PDNIPV4 {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def ue_address_ipv6(self):
        """ Gets UE IPv6 address

        Args:
            None

        Returns:
            UE IPv6 address
        """
        cmd = "PDNIPV6? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @ue_address_ipv6.setter
    def ue_address_ipv6(self, ip_address):
        """ Set UE IPv6 address

        Args:
            ip_address: UE IPv6 address

        Returns:
            None
        """
        cmd = "PDNIPV6 {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def primary_dns_address_ipv4(self):
        """ Gets Primary DNS server IPv4 address

        Args:
            None

        Returns:
            Primary DNS server IPv4 address
        """
        cmd = "PDNDNSIPV4PRI? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @primary_dns_address_ipv4.setter
    def primary_dns_address_ipv4(self, ip_address):
        """ Set Primary DNS server IPv4 address

        Args:
            ip_address: Primary DNS server IPv4 address

        Returns:
            None
        """
        cmd = "PDNDNSIPV4PRI {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def secondary_dns_address_ipv4(self):
        """ Gets secondary DNS server IPv4 address

        Args:
            None

        Returns:
            secondary DNS server IPv4 address
        """
        cmd = "PDNDNSIPV4SEC? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @secondary_dns_address_ipv4.setter
    def secondary_dns_address_ipv4(self, ip_address):
        """ Set secondary DNS server IPv4 address

        Args:
            ip_address: secondary DNS server IPv4 address

        Returns:
            None
        """
        cmd = "PDNDNSIPV4SEC {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def dns_address_ipv6(self):
        """ Gets DNS server IPv6 address

        Args:
            None

        Returns:
            DNS server IPv6 address
        """
        cmd = "PDNDNSIPV6? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @dns_address_ipv6.setter
    def dns_address_ipv6(self, ip_address):
        """ Set DNS server IPv6 address

        Args:
            ip_address: DNS server IPv6 address

        Returns:
            None
        """
        cmd = "PDNDNSIPV6 {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def cscf_address_ipv4(self):
        """ Gets Secondary P-CSCF IPv4 address

        Args:
            None

        Returns:
            Secondary P-CSCF IPv4 address
        """
        cmd = "PDNPCSCFIPV4? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @cscf_address_ipv4.setter
    def cscf_address_ipv4(self, ip_address):
        """ Set Secondary P-CSCF IPv4 address

        Args:
            ip_address: Secondary P-CSCF IPv4 address

        Returns:
            None
        """
        cmd = "PDNPCSCFIPV4 {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def cscf_address_ipv6(self):
        """ Gets P-CSCF IPv6 address

        Args:
            None

        Returns:
            P-CSCF IPv6 address
        """
        cmd = "PDNPCSCFIPV6? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @cscf_address_ipv6.setter
    def cscf_address_ipv6(self, ip_address):
        """ Set P-CSCF IPv6 address

        Args:
            ip_address: P-CSCF IPv6 address

        Returns:
            None
        """
        cmd = "PDNPCSCFIPV6 {},{}".format(self._pdn_number, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def pdn_ims(self):
        """ Get PDN IMS VNID binding status

        Args:
            None

        Returns:
            PDN IMS VNID binding status
        """
        cmd = "PDNIMS? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @pdn_ims.setter
    def pdn_ims(self, switch):
        """ Set PDN IMS VNID binding Enable/Disable

        Args:
            switch: "ENABLE/DISABLE"

        Returns:
            None
        """
        if not isinstance(switch, Switch):
            raise ValueError(' The parameter should be of type'
                             ' "Switch", ie, ENABLE or DISABLE ')
        cmd = "PDNIMS {},{}".format(self._pdn_number, switch.value)
        self._anritsu.send_command(cmd)

    @property
    def pdn_vnid(self):
        """ Get PDN IMS VNID

        Args:
            None

        Returns:
            PDN IMS VNID
        """
        cmd = "PDNVNID? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @pdn_vnid.setter
    def pdn_vnid(self, vnid):
        """ Set PDN IMS VNID

        Args:
            vnid: 1~99

        Returns:
            None
        """
        cmd = "PDNVNID {},{}".format(self._pdn_number, vnid)
        self._anritsu.send_command(cmd)

    @property
    def pdn_apn_name(self):
        """ Get PDN APN NAME

        Args:
            None

        Returns:
            PDN APN NAME
        """
        cmd = "PDNCHECKAPN? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @pdn_apn_name.setter
    def pdn_apn_name(self, name):
        """ Set PDN APN NAME

        Args:
            name: fast.t-mobile.com, ims

        Returns:
            None
        """
        cmd = "PDNCHECKAPN {},{}".format(self._pdn_number, name)
        self._anritsu.send_command(cmd)

    @property
    def pdn_qci(self):
        """ Get PDN QCI Value

        Args:
            None

        Returns:
            PDN QCI Value
        """
        cmd = "PDNQCIDEFAULT? " + self._pdn_number
        return self._anritsu.send_query(cmd)

    @pdn_qci.setter
    def pdn_qci(self, qci_value):
        """ Set PDN QCI Value

        Args:
            qci_value: 5, 9

        Returns:
            None
        """
        cmd = "PDNQCIDEFAULT {},{}".format(self._pdn_number, qci_value)
        self._anritsu.send_command(cmd)


class _TriggerMessage(object):
    '''Class to interact with trigger message handling supported by MD8475 '''

    def __init__(self, anritsu):
        self._anritsu = anritsu
        self.log = anritsu.log

    def set_reply_type(self, message_id, reply_type):
        """ Sets the reply type of the trigger information

        Args:
            message_id: trigger information message Id
            reply_type: reply type of the trigger information

        Returns:
            None
        """
        if not isinstance(message_id, TriggerMessageIDs):
            raise ValueError(' The parameter should be of type'
                             ' "TriggerMessageIDs"')
        if not isinstance(reply_type, TriggerMessageReply):
            raise ValueError(' The parameter should be of type'
                             ' "TriggerMessageReply"')

        cmd = "REJECTTYPE {},{}".format(message_id.value, reply_type.value)
        self._anritsu.send_command(cmd)

    def set_reject_cause(self, message_id, cause):
        """ Sets the reject cause of the trigger information

        Args:
            message_id: trigger information message Id
            cause: cause for reject

        Returns:
            None
        """
        if not isinstance(message_id, TriggerMessageIDs):
            raise ValueError(' The parameter should be of type'
                             ' "TriggerMessageIDs"')

        cmd = "REJECTCAUSE {},{}".format(message_id.value, cause)
        self._anritsu.send_command(cmd)


class _IMS_Services(object):
    '''Class to configure and operate IMS Services'''

    def __init__(self, anritsu, vnid):
        self._vnid = vnid
        self._anritsu = anritsu
        self.log = anritsu.log

    @property
    def sync(self):
        """ Gets Sync Enable status

        Args:
            None

        Returns:
            VNID Sync Enable status
        """
        cmd = "IMSSYNCENABLE? " + self._vnid
        return self._anritsu.send_query(cmd)

    @sync.setter
    def sync(self, switch):
        """ Set Sync Enable or Disable

        Args:
            sync: ENABLE/DISABLE

        Returns:
            None
        """
        if not isinstance(switch, Switch):
            raise ValueError(' The parameter should be of type "Switch"')
        cmd = "IMSSYNCENABLE {},{}".format(self._vnid, switch.value)
        self._anritsu.send_command(cmd)

    @property
    def cscf_address_ipv4(self):
        """ Gets CSCF IPv4 address

        Args:
            None

        Returns:
            CSCF IPv4 address
        """
        cmd = "IMSCSCFIPV4? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_address_ipv4.setter
    def cscf_address_ipv4(self, ip_address):
        """ Set CSCF IPv4 address

        Args:
            ip_address: CSCF IPv4 address

        Returns:
            None
        """
        cmd = "IMSCSCFIPV4 {},{}".format(self._vnid, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def cscf_address_ipv6(self):
        """ Gets CSCF IPv6 address

        Args:
            None

        Returns:
            CSCF IPv6 address
        """
        cmd = "IMSCSCFIPV6? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_address_ipv6.setter
    def cscf_address_ipv6(self, ip_address):
        """ Set CSCF IPv6 address

        Args:
            ip_address: CSCF IPv6 address

        Returns:
            None
        """
        cmd = "IMSCSCFIPV6 {},{}".format(self._vnid, ip_address)
        self._anritsu.send_command(cmd)

    @property
    def imscscf_iptype(self):
        """ Gets CSCF IP Type

        Args:
            None

        Returns:
            CSCF IP Type
        """
        cmd = "IMSCSCFIPTYPE? " + self._vnid
        return self._anritsu.send_query(cmd)

    @imscscf_iptype.setter
    def imscscf_iptype(self, iptype):
        """ Set CSCF IP Type

        Args:
            iptype: IPV4, IPV6, IPV4V6

        Returns:
            None
        """
        cmd = "IMSCSCFIPTYPE {},{}".format(self._vnid, iptype)
        self._anritsu.send_command(cmd)

    @property
    def cscf_monitoring_ua(self):
        """ Get CSCF Monitoring UA URI

        Args:
            None

        Returns:
            CSCF Monitoring UA URI
        """
        cmd = "IMSCSCFUAURI? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_monitoring_ua.setter
    def cscf_monitoring_ua(self, ua_uri):
        """ Set CSCF Monitoring UA URI

        Args:
            ua_uri: CSCF Monitoring UA URI

        Returns:
            None
        """
        cmd = "IMSCSCFUAURI {},{}".format(self._vnid, ua_uri)
        self._anritsu.send_command(cmd)

    @property
    def cscf_host_name(self):
        """ Get CSCF Host Name

        Args:
            None

        Returns:
            CSCF Host Name
        """
        cmd = "IMSCSCFNAME? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_host_name.setter
    def cscf_host_name(self, host_name):
        """ Set CSCF Host Name

        Args:
            host_name: CSCF Host Name

        Returns:
            None
        """
        cmd = "IMSCSCFNAME {},{}".format(self._vnid, host_name)
        self._anritsu.send_command(cmd)

    @property
    def cscf_ims_authentication(self):
        """ Get CSCF IMS Auth Value

        Args:
            None

        Returns:
            CSCF IMS Auth
        """
        cmd = "IMSCSCFAUTH? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_ims_authentication.setter
    def cscf_ims_authentication(self, on_off):
        """ Set CSCF IMS Auth Value

        Args:
            on_off: CSCF IMS Auth ENABLE/DISABLE

        Returns:
            None
        """
        cmd = "IMSCSCFAUTH {},{}".format(self._vnid, on_off)
        self._anritsu.send_command(cmd)

    @property
    def cscf_virtual_ua(self):
        """ Get CSCF Virtual UA URI

        Args:
            None

        Returns:
            CSCF Virtual UA URI
        """
        cmd = "IMSCSCFVUAURI? " + self._vnid
        return self._anritsu.send_query(cmd)

    @cscf_virtual_ua.setter
    def cscf_virtual_ua(self, ua_uri):
        """ Set CSCF Virtual UA URI

        Args:
            ua_uri: CSCF Virtual UA URI

        Returns:
            None
        """
        cmd = "IMSCSCFVUAURI {},{}".format(self._vnid, ua_uri)
        self._anritsu.send_command(cmd)

    @property
    def tmo_cscf_userslist_add(self):
        """ Get CSCF USERLIST

        Args:
            None

        Returns:
            CSCF USERLIST
        """
        cmd = "IMSCSCFUSERSLIST? " + self._vnid
        return self._anritsu.send_query(cmd)

    @tmo_cscf_userslist_add.setter
    def tmo_cscf_userslist_add(self, username):
        """ Set CSCF USER to USERLIST
            This is needed if IMS AUTH is enabled

        Args:
            username: CSCF Username

        Returns:
            None
        """
        cmd = "IMSCSCFUSERSLISTADD {},{},00112233445566778899AABBCCDDEEFF,TS34108,AKAV1_MD5,\
        OPC,00000000000000000000000000000000,8000,TRUE,FALSE,0123456789ABCDEF0123456789ABCDEF,\
        54CDFEAB9889000001326754CDFEAB98,6754CDFEAB9889BAEFDC457623100132,\
        326754CDFEAB9889BAEFDC4576231001,TRUE,TRUE,TRUE".format(
            self._vnid, username)
        self._anritsu.send_command(cmd)

    @property
    def vzw_cscf_userslist_add(self):
        """ Get CSCF USERLIST

        Args:
            None

        Returns:
            CSCF USERLIST
        """
        cmd = "IMSCSCFUSERSLIST? " + self._vnid
        return self._anritsu.send_query(cmd)

    @vzw_cscf_userslist_add.setter
    def vzw_cscf_userslist_add(self, username):
        """ Set CSCF USER to USERLIST
            This is needed if IMS AUTH is enabled

        Args:
            username: CSCF Username

        Returns:
            None
        """
        cmd = "IMSCSCFUSERSLISTADD {},{},465B5CE8B199B49FAA5F0A2EE238A6BC,MILENAGE,AKAV1_MD5,\
        OP,5F1D289C5D354D0A140C2548F5F3E3BA,8000,TRUE,FALSE,0123456789ABCDEF0123456789ABCDEF,\
        54CDFEAB9889000001326754CDFEAB98,6754CDFEAB9889BAEFDC457623100132,\
        326754CDFEAB9889BAEFDC4576231001,TRUE,TRUE,TRUE".format(
            self._vnid, username)
        self._anritsu.send_command(cmd)

    @property
    def dns(self):
        """ Gets DNS Enable status

        Args:
            None

        Returns:
            VNID DNS Enable status
        """
        cmd = "IMSDNS? " + self._vnid
        return self._anritsu.send_query(cmd)

    @dns.setter
    def dns(self, switch):
        """ Set DNS Enable or Disable

        Args:
            sync: ENABLE/DISABLE

        Returns:
            None
        """
        if not isinstance(switch, Switch):
            raise ValueError(' The parameter should be of type "Switch"')
        cmd = "IMSDNS {},{}".format(self._vnid, switch.value)
        self._anritsu.send_command(cmd)

    @property
    def ndp_nic(self):
        """ Gets NDP Network Interface name

        Args:
            None

        Returns:
            NDP NIC name
        """
        cmd = "IMSNDPNIC? " + self._vnid
        return self._anritsu.send_query(cmd)

    @ndp_nic.setter
    def ndp_nic(self, nic_name):
        """ Set NDP Network Interface name

        Args:
            nic_name: NDP Network Interface name

        Returns:
            None
        """
        cmd = "IMSNDPNIC {},{}".format(self._vnid, nic_name)
        self._anritsu.send_command(cmd)

    @property
    def ndp_prefix(self):
        """ Gets NDP IPv6 Prefix

        Args:
            None

        Returns:
            NDP IPv6 Prefix
        """
        cmd = "IMSNDPPREFIX? " + self._vnid
        return self._anritsu.send_query(cmd)

    @ndp_prefix.setter
    def ndp_prefix(self, prefix_addr):
        """ Set NDP IPv6 Prefix

        Args:
            prefix_addr: NDP IPV6 Prefix Addr

        Returns:
            None
        """
        cmd = "IMSNDPPREFIX {},{},64".format(self._vnid, prefix_addr)
        self._anritsu.send_command(cmd)

    @property
    def psap(self):
        """ Gets PSAP Enable status

        Args:
            None

        Returns:
            VNID PSAP Enable status
        """
        cmd = "IMSPSAP? " + self._vnid
        return self._anritsu.send_query(cmd)

    @psap.setter
    def psap(self, switch):
        """ Set PSAP Enable or Disable

        Args:
            switch: ENABLE/DISABLE

        Returns:
            None
        """
        if not isinstance(switch, Switch):
            raise ValueError(' The parameter should be of type "Switch"')
        cmd = "IMSPSAP {},{}".format(self._vnid, switch.value)
        self._anritsu.send_command(cmd)

    @property
    def psap_auto_answer(self):
        """ Gets PSAP Auto Answer status

        Args:
            None

        Returns:
            VNID PSAP Auto Answer status
        """
        cmd = "IMSPSAPAUTOANSWER? " + self._vnid
        return self._anritsu.send_query(cmd)

    @psap_auto_answer.setter
    def psap_auto_answer(self, switch):
        """ Set PSAP Auto Answer Enable or Disable

        Args:
            switch: ENABLE/DISABLE

        Returns:
            None
        """
        if not isinstance(switch, Switch):
            raise ValueError(' The parameter should be of type "Switch"')
        cmd = "IMSPSAPAUTOANSWER {},{}".format(self._vnid, switch.value)
        self._anritsu.send_command(cmd)

    def start_virtual_network(self):
        """ Start the specified Virtual Network (IMS service)

        Args:
            None

        Returns:
            None
        """
        cmd = "IMSSTARTVN " + self._vnid
        return self._anritsu.send_command(cmd)
