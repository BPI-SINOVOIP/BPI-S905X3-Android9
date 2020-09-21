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
Utility functions for for Anritsu Signalling Tester.
"""
# yapf: disable

OPERATION_COMPLETE = 1
NO_ERROR = 0

ANRITSU_ERROR_CODES = {
    0: 'No errors occurred',
    2: 'The specified file does not exist',
    14: 'The buffer size is insufficient',
    29: 'The save destination is a write-protected file.',
    80: 'A file with the same name already exists.'
        ' (If Overwrite is specified to 0.)',
    87: 'The specified value is wrong.',
    112: 'The disk space is insufficient.',
    183: 'SmartStudio is already running.',
    1060: 'The control software has not been started or has already terminated',
    1067: 'SmartStudio, control software or SMS Centre could not start due to'
          'a problem or problems resulting from OS or the MD8475A system.',
    1229: 'Connecting to the server failed.',
    1235: 'A request is suspended.',
    1460: 'The operation is terminated due to the expiration of the'
          ' timeout period.',
    9999: 'A GPIB command error occurred.',
    536870912: 'The license could not be confirmed.',
    536870913: 'The specified file cannot be loaded by the SmartStudio.',
    536870914: 'The specified process ID does not exist.',
    536870915: 'The received data does not exist.',
    536870916: 'Simulation is not running.',
    536870917: 'Simulation is running.',
    536870918: 'Test Case has never been executed.',
    536870919: 'The resource cannot be obtained.',
    536870920: 'A resource protocol error, such as download error or'
               ' license error, occurred.',
    536870921: 'The function call has been in invalid status.',
    536870922: 'The current Simulation Model does not allow the operation.',
    536870923: 'The Cell name to be set does not exist.',
    536870924: 'The test is being executed.',
    536870925: 'The current UE status does not correspond to the'
               ' test parameters.',
    536870926: 'There is no LOG information because the simulation'
               ' has not been executed.',
    536870927: 'Measure Export has already been executed.',
    536870928: 'SmartStudio is not connected to the SMS Centre.',
    536870929: 'SmartStudio failed to send an SMS message to the SMS Centre.',
    536870930: 'SmartStudio has successfully sent an SMS message'
               ' to the SMS Centre,but the SMS Centre judges it as an error.',
    536870931: 'The processing that is unavailable with the current system'
               ' status has been executed.',
    536870932: 'The option could not be confirmed.',
    536870933: 'Measure Export has been stopped.',
    536870934: 'SmartStudio cannot load the specified file because the'
               ' version is old.',
    536870935: 'The data with the specified PDN number does not exist.',
    536870936: 'The data with the specified Dedicated number does not exist.',
    536870937: 'The PDN data cannot be added because the upper limit of the'
               ' number of PDN data has been reached.',
    536870938: 'The number of antennas, which cannot be set to the current'
               ' Simulation Model,has been specified.',
    536870939: 'Calibration of path loss failed.',
    536870940: 'There is a parameter conflict.',
    536870941: 'The DL Ref Power setting is out of the setting range'
               ' at W-CDMA (Evolution).',
    536870942: 'DC-HSDPA is not available for the current channel setting.',
    536870943: 'The specified Packet Rate cannot be used by the current'
               ' Simulation Model.',
    536870944: 'The W-CDMA Cell parameter F-DPCH is set to Enable.',
    536870945: 'Target is invalid.',
    536870946: 'The PWS Centre detects an error.',
    536870947: 'The Ec/Ior setting is invalid.',
    536870948: 'The combination of Attach Type and TA Update Type is invalid.',
    536870949: 'The license of the option has expired.',
    536870950: 'The Ping command is being executed.',
    536870951: 'The Ping command is not being executed.',
    536870952: 'The current Test Case parameter setting is wrong.',
    536870953: 'The specified IP address is the same as that of Default Gateway'
               'specified by Simulation parameter.',
    536870954: 'TFT IE conversion failed.',
    536875008: 'An error exists in the parameter configuration.'
               '(This error applies only to the current version.)',
    536936448: 'License verification failed.',
    536936449: 'The IMS Services cannot load the specified file.',
    536936462: 'Simulation is not performed and no log information exists.',
    536936467: 'The executed process is inoperable in the current status'
               ' of Visual User Agent.',
    536936707: 'The specified Virtual Network is not running.',
    536936709: 'The specified Virtual Network is running. '
               'Any one of the Virtual Networks is running.',
    536936727: 'The specified Virtual Network does not exist.',
    536936729: 'When the Virtual Network already exists.',
    554762241: 'The RF Measurement launcher cannot be accessed.',
    554762242: 'License check of the RF Measurement failed.',
    554762243: 'Function is called when RF Measurement cannot be set.',
    554762244: 'RF Measurement has been already started.',
    554762245: 'RF Measurement failed to start due to a problem resulting'
               ' from OS or the MD8475A system.',
    554762246: 'RF Measurement is not started or is already terminated.',
    554762247: 'There is a version mismatch between RF Measurement and CAL.',
    554827777: 'The specified value for RF Measurement is abnormal.',
    554827778: 'GPIB command error has occurred in RF Measurement.',
    554827779: 'Invalid file path was specified to RF Measurement.',
    554827780: 'RF Measurement argument is NULL pointer.',
    555810817: 'RF Measurement is now performing the measurement.',
    555810818: 'RF Measurement is now not performing the measurement.',
    555810819: 'RF Measurement is not measured yet. (There is no result '
               'information since measurement is not performed.)',
    555810820: 'An error has occurred when RF Measurement'
               ' starts the measurement.',
    555810821: 'Simulation has stopped when RF Measurement is '
               'performing the measurement.',
    555810822: 'An error has been retrieved from the Platform when '
               'RF Measurement is performing the measurement.',
    555810823: 'Measurement has been started in the system state where RF '
               'Measurement is invalid.',
    556859393: 'RF Measurement is now saving a file.',
    556859394: 'There is insufficient disk space when saving'
               'a Measure Result file of RF Measurement.',
    556859395: 'An internal error has occurred or USB cable has been'
               ' disconnected when saving a Measure Result'
               ' file of RF Measurement.',
    556859396: 'A write-protected file was specified as the save destination'
               ' when saving a Measure Result file of RF Measurement.',
    568328193: 'An internal error has occurred in RF Measurement.',
    687865857: 'Calibration Measure DSP is now being measured.',
    687865858: 'Calibration measurement failed.',
    687865859: 'Calibration slot is empty or its system does not apply.',
    687865860: 'Unexpected command is received from Calibration HWC.',
    687865861: 'Failed to receive the Calibration measurement result.',
    687865862: 'Failed to open the correction value file on the'
               ' Calibration HDD.',
    687865863: 'Failed to move the pointer on the Calibration correction'
               ' value table.',
    687865864: 'Failed to write the correction value to the Calibration'
               ' correction value file on the Calibration HDD.',
    687865865: 'Failed to load the correction value from the Calibration HDD.',
    687865866: 'Failed to create a directory to which the correction value '
               'file on the Calibration HDD is saved.',
    687865867: 'Correction data has not been written in the'
               ' Calibration-specified correction table.',
    687865868: 'Data received from Calibration HWC does not exist.',
    687865869: 'Data has not been written to the Flash ROM'
               ' of Calibration BASE UNIT.',
    687865870: 'Correction data has not been written to the'
               ' Calibration-specified sector.',
    687866111: 'An calibration error other than described above occurred.',
}


def _error_code_tostring(error_code):
    ''' returns the description of the error from the error code
    returned by anritsu MD8475A '''
    try:
        error_string = ANRITSU_ERROR_CODES[error_code]
    except KeyError:
        error_string = "Error : {} ".format(error_code)

    return error_string


class AnritsuUtils(object):
    def gsm_encode(text):
        '''To encode text string with GSM 7-bit alphabet for common symbols'''
        table = {' ': '%20', '!': '%21', '\"': '%22', '#': '%23', '$': '%24',
                 '/': '%2F', '%': '%25', '&': '%26', '\'': '%27', '(': '%28',
                 ')': '%29', '*': '%2A', '+': '%2B', ',': '%2C', ':': '%3A',
                 ';': '%3B', '<': '%3C', '=': '%3D', '>': '%3E', '?': '%3F',
                 '@': '%40', '[': '%5B', ']': '%5D', '_': '%5F', 'é': '%C3%A9'}
        coded_str = ""
        for char in text:
            if char in table:
                coded_str += table[char]
            else:
                coded_str += char
        return coded_str

    def gsm_decode(text):
        '''To decode text string with GSM 7-bit alphabet for common symbols'''
        table = {'%20': ' ', '%21': '!', '%22': '\"', '%23': '#', '%24': '$',
                 '%2F': '/', '%25': '%', '%26': '&', '%27': '\'', '%28': '(',
                 '%29': ')', '%2A': '*', '%2B': '+', '%2C': ',', '%3A': ':',
                 '%3B': ';', '%3C': '<', '%3D': '=', '%3E': '>', '%3F': '?',
                 '%40': '@', '%5B': '[', '%5D': ']', '%5F': '_', '%C3%A9': 'é'}
        coded_str = text
        for char in table:
            if char in text:
                coded_str = coded_str.replace(char, table[char])
        return coded_str

    def cdma_encode(text):
        '''To encode text string with GSM 7-bit alphabet for common symbols'''
        table = {' ': '%20', '!': '%21', '\"': '%22', '#': '%23', '$': '%24',
                 '/': '%2F', '%': '%25', '&': '%26', '\'': '%27', '(': '%28',
                 ')': '%29', '*': '%2A', '+': '%2B', ',': '%2C', ':': '%3A',
                 ';': '%3B', '<': '%3C', '=': '%3D', '>': '%3E', '?': '%3F',
                 '@': '%40', '[': '%5B', ']': '%5D', '_': '%5F'}
        coded_str = ""
        for char in text:
            if char in table:
                coded_str += table[char]
            else:
                coded_str += char
        return coded_str

class AnritsuError(Exception):
    '''Exception for errors related to Anritsu.'''
    def __init__(self, error, command=None):
        self._error_code = error
        self._error_message = _error_code_tostring(self._error_code)
        if command is not None:
            self._error_message = "Command {} returned the error: '{}'".format(
                                  command, self._error_message)

    def __str__(self):
        return self._error_message
# yapf: enable
