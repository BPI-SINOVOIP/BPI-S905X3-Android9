# Copyrigh The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Get speaker/microphone status from cras_client_test, /proc/asound and
   atrus.log.
"""

from __future__ import print_function

import logging
import re


NUM_AUDIO_STREAM_IN_MEETING = 3

def get_soundcard_by_name(dut, name):
    """
    Returns the soundcard number of specified soundcard by name.
    @param dut: The handle of the device under test.
    @param name: The name of Speaker
                 For example: 'Hangouts Meet speakermic'
    @returns the soundcard, if no device found returns None.
    """
    soundcard = None
    cmd = "cat /proc/asound/cards | grep \"{}\" | grep USB".format(name)
    logging.info('---cmd: %s', cmd)
    try:
        soundcard = dut.run(cmd, ignore_status=True).stdout.strip().split()[0]
    except Exception as e:
        soundcard = None
        logging.exception('Fail to execute %s.', cmd)
    if soundcard:
        soundcard = "card{}".format(soundcard)
        logging.info('---audio card %s', soundcard)
    else:
        logging.exception('Fail to get sound card, cli=%s.', cmd)
    return soundcard

def check_soundcard_by_name(dut, name):
    """
    check soundcard by name exists
    @param dut: The handle of the device under test.
    @param name: The name of Speaker
                 For example: 'Hangouts Meet speakermic'
    @returns: True, None if test passes,
              False, errMsg if test fails
    """
    if get_soundcard_by_name(dut, name):
        return True, None
    else:
        return False, 'Soundcard is not found under /proc/asound/cards.'

def check_audio_stream(dut, is_in_meeting):
    """
    Verify speaker is streaming or not streaming as expected.
    @param dut: The handle of the device under test.
    @is_in_meeting: True if CfM is in meeting, False, if not
    @returns: True, None if test passes,
              False, errMsg if test fails
    """
    number_stream = get_number_of_active_streams(dut)
    if is_in_meeting:
       if number_stream  >= NUM_AUDIO_STREAM_IN_MEETING:
           return True, None
       else:
           return False, 'Number of Audio streams is not expected.'
    else:
       if number_stream  <=  NUM_AUDIO_STREAM_IN_MEETING:
           return True, None
       else:
           return False, 'Number of Audio streams is not expected.'

def get_audio_stream_state(dut, soundcard):
    """
    Returns the state of stream0 for specified soundcard.

    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @param soundcard: soundcard
                 For example: 'card0'

    @returns the list of state of steam0, "Running" or "Stop"

    """
    stream_state = []
    try:
        cmd = ("cat /proc/asound/%s/stream0 | grep Status | "
               "awk -v N=2 '{print $N}'" % soundcard)
        stream_state = dut.run(cmd, ignore_status=True).stdout.split()
    except Exception as e:
        logging.exception('Fail to run cli: %s.', cmd)
    return stream_state


def check_default_speaker_volume(dut, cfm_facade):
    """Check volume of speaker is the same as expected.
    @param dut: The handle of the device under test.
    @param cfm_facade: the handle of cfm facade
    @returns True, None if default speakers have same volume as one read
             from hotrod,
             False, errMsg, otherwise
    """
    try:
        expected_volume = int(cfm_facade.get_speaker_volume())
    except Exception as e:
        errmsg = 'Fail to run telemetry api to get speaker volume.'
        logging.exception(errmsg)
        return False, errmsg
    if expected_volume < 1:
        return False, 'Fail to get speaker volume from Hotrod.'
    nodes = get_nodes_for_default_speakers_cras(dut)
    if not nodes:
        logging.info('---Fail to get node for default speaker.')
        return False, 'Fail to get node for default speaker.'
    for node in nodes:
        cras_volume =  get_speaker_volume_cras(dut, node)
        logging.info('---Volume for default speaker are sync for '
                     'node %s? cfm: %d, cras: %d.'
                     'format(node, expected_volume, cras_volume)')
        if not expected_volume == cras_volume:
            logging.info('---Volume Check Fail for default speaker: '
                         'expected_volume:%d, actual_volume:%d.'
                         'format(expected_volume, cras_volume)')
            return False, ('Volume Check fails for default speaker: '
                           'expected_volume:%d, actual_volume:%d',
                           '% expected_volume, cras_volume')
    logging.info('---Expected volume: %d, actual: %d',
                     expected_volume, cras_volume)
    return True, None

def get_number_of_active_streams(dut):
    """
    Returns the number of active stream.
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @returns the number of active streams.
    """
    cmd = ("cras_test_client --dump_server_info "
           "| grep 'Num active streams:' "
           "| awk -v N=4 '{print $N}'")

    try:
        number_of_streams = int(dut.run(cmd, ignore_status=True).stdout.strip())
    except Exception as e:
        logging.exception('Fail to execute cli to get number of streams: %s.',
                          cmd)
        return None
    logging.info('---number of audio streaming: %d', number_of_streams)
    return number_of_streams


def get_nodes_for_default_speakers_cras(dut):
    """get node for default speakers from cras_test_client.
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @returns the list of nodes for default speakers. If device not found,
     returns [].
    """
    nodes = []
    cmd = ("cras_test_client --dump_server_info | awk '/Output Nodes:/,"
           "/Input Devices:/'")
    try:
        lines = dut.run(cmd, ignore_status=True).stdout.splitlines()
    except Exception as e:
        logging.exception('Fail to execute cli to get nodes for default'
                          'speaker: %s', cmd)
        return nodes
    for _line in lines:
        match = re.findall(r"(\d+):\d+.*USB\s+\*.*", _line)
        if match:
            nodes.append(match[0])
    logging.info('---found nodes for default speaker %s', nodes)
    return nodes


def get_speaker_for_node_cras(dut, node):
    """get node for default speakers from cras_test_client.
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.

    @returns the list of nodes for default speakers. If device not found,
     returns [].
    """
    cmd = ("cras_test_client --dump_server_info | awk '/Output Devices:/,"
           "/Output Nodes:/' | grep '%s'" % node)

    try:
        line = dut.run(cmd, ignore_status=True).stdout.stripe()
        speaker = re.findall(r"^[0-9]+\s*(.*):\s+USB\s+Audio:", line)[0]
    except Exception as e:
        logging.exception('Fail to execute cli to get nodes for default'
                          'speaker: %s.', cmd)

    logging.info('---speaker for %s is %s', node, speaker)
    return speaker


def get_nodes_for_default_microphone_cras(dut):
    """get node for default microphones from cras_test_client.
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.

    @returns the list of nodes for default microphone. If device not found,
     returns [].
    """
    nodes = None
    cmd = ("cras_test_client --dump_server_info | awk '/Input Nodes:/,"
           "/Attached clients:/'")
    try:
        lines = dut.run(cmd, ignore_status=True).stdout.splitlines()
        for _line in lines:
            nodes.append(re.findall(r"(\d+):\d+.*USB\s+\*.*", _line)[0])
    except Exception as e:
        logging.exception('Fail to execute cli to get nodes for default'
                          ' speaker: %s.', cmd)
    return nodes


def get_microphone_for_node_cras(dut, node):
    """get node for default speakers from cras_test_client.
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.

    @returns the list of nodes for default speakers. If device not found,
     returns [].
    """
    cmd = ("cras_test_client --dump_server_info | awk '/Input Devices:/,"
           "/Input Nodes:/' | grep '%s' " % node)

    try:
        line = dut.run(cmd, ignore_status=True).stdout
        microphone = re.findall(r"10\t(.*):\s+USB\s+Audio:", line)[0]
    except Exception as e:
        logging.exception('Fail to execute cli to get nodes for default'
                          ' speaker: %s.', cmd)
    logging.info('---mic for %s is %s', node, microphone)
    return microphone


def get_speaker_volume_cras(dut, node):
    """get volume for speaker from cras_test_client based on node
    @param dut: The handle of the device under test. Should be initialized in
                 autotest.
    @param node: The node of Speaker
                 Example cli:
                 cras_test_client --dump_server_info | awk
                 '/Output Nodes:/,/Input Devices:/' |  grep 9:0 |
                 awk -v N=3 '{print $N}'

    @returns the volume of speaker. If device not found, returns None.
    """
    cmd = ("cras_test_client --dump_server_info | awk '/Output Nodes:/,"
           "/Input Devices:/' | grep -E 'USB' | grep '%s':0 "
           "|  awk -v N=3 '{print $N}'" % node)
    try:
        volume = int(dut.run(cmd, ignore_status=True).stdout.strip())
    except Exception as e:
        logging.exception('Fail to execute cli %s to get volume for node %d',
                           cmd, node)
        return None
    return volume


def check_cras_mic_mute(dut, cfm_facade):
    """
    check microphone is muted or unmuted as expected/.
    @param dut: The handle of the device under test.
    @param cfm_facade:  facade of CfM
    @param is_mic_muted: True if muted, False otherwise
    @returns True, none if test passes
             False, errMsg if test fails
    """
    try:
        is_mic_muted = cfm_facade.is_mic_muted()
    except Exception as e:
        errmsg = 'Fail to run telemetry api to check mute state for mic.'
        logging.exception(errmsg)
        return False, errmsg
    actual_muted = get_mic_muted_cras(dut)
    if is_mic_muted == actual_muted:
        return True, None
    else:
        if is_mic_muted:
            logging.info('Hotrod setting: microphone is muted')
        else:
            logging.info('Hotrod setting: microphone is not muted')
        if actual_muted:
            logging.info('cras setting: microphone is muted')
        else:
            logging.info('cras setting: microphone is not muted')
        return False, 'Microphone is not muted/unmuted as shown in Hotrod.'

def check_is_preferred_speaker(dut, name):
    """
    check preferred speaker is speaker to be tested.
    @param dut: The handle of the device under test.
    @param cfm_facade:  facade of CfM
    @param name: name of speaker
    @returns True, none if test passes
             False, errMsg if test fails
    """
    node = None
    cmd = ("cras_test_client --dump_server_info | awk "
           "'/Output Devices:/,/Output Nodes:/' "
           "| grep '%s' " % name)
    try:
        output = dut.run(cmd, ignore_status=True).stdout.strip()
    except Exception as e:
        logging.exception('Fail to run cli %s to find %s in cras_test_client.',
                          cmd, node)
        return False, 'Fail to run cli {}:, reason: {}'.format(cmd, str(e))
    logging.info('---output = %s', output)
    if output:
        node = output.split()[0]
        logging.info('---found node for %s is %s', name, node)
    else:
        return False, 'Fail in get node for speaker in cras.'
    default_nodes = get_nodes_for_default_speakers_cras(dut)
    logging.info('---default speaker node is %s', default_nodes)
    if node in default_nodes:
        return True, None
    return False, '{} is not set to preferred speaker.'.format(name)


def check_is_preferred_mic(dut, name):
    """check preferred mic is set to speaker to be tested."""
    cmd = ("cras_test_client --dump_server_info | "
           "awk '/Input Devices/,/Input Nodes/'  | grep '%s' | "
           "awk -v N=1 '{print $N}'" % name)
    logging.info('---cmd = %s',cmd)
    try:
        mic_node = dut.run(cmd, ignore_status=True).stdout.strip()
        logging.info('---mic_node : %s', mic_node)
    except Exception as e:
        logging.exception('Fail to execute: %s to check preferred mic.', cmd)
        return False, 'Fail to run cli.'
    try:
         cmd = ("cras_test_client --dump_server_info | awk '/Input Nodes:/,"
               "/Attached clients:/'  | grep default "
               "| awk -v N=2 '{print $N}'")
         mic_node_default = dut.run(cmd, ignore_status=True).stdout.strip()
         if not mic_node_default:
             cmd = ("cras_test_client --dump_server_info | awk '/Input Nodes:/,"
                   "/Attached clients:/'  | grep '%s' "
                   "| awk -v N=2 '{print $N}'" %name)
             mic_node_default = dut.run(cmd,ignore_status=True).stdout.strip()
         logging.info('---%s',cmd)
         logging.info('---%s', mic_node_default)
    except Exception as e:
         msg = 'Fail to execute: {} to check preferred mic'.format(cmd)
         logging.exception(msg)
         return False, msg
    logging.info('---mic node:%s, default node:%s',
                 mic_node, mic_node_default)
    if mic_node == mic_node_default.split(':')[0]:
        return True,  None
    return False, '{} is not preferred microphone.'.format(name)


def get_mic_muted_cras(dut):
    """
    Get the status of mute or unmute for microphone
    @param dut: the handle of CfM under test
    @returns True if mic is muted
             False if mic not not muted
    """
    cmd = 'cras_test_client --dump_server_info | grep "Capture Gain"'
    try:
        microphone_muted = dut.run(cmd, ignore_status=True).stdout.strip()
    except Exception as e:
        logging.exception('Fail to execute: %s.', cmd)
        return False, 'Fail to execute: {}.'.format(cmd)
    logging.info('---%s',  microphone_muted)
    if "Muted" in microphone_muted:
       return True
    else:
       return False


def check_speaker_exist_cras(dut, name):
    """
    Check speaker exists in cras.
    @param dut: The handle of the device under test.
    @param name: name of speaker
    @returns: True, None if test passes,
              False, errMsg if test fails
    """
    cmd = ("cras_test_client --dump_server_info | awk "
           "'/Output Devices:/, /Output Nodes:/' "
           "| grep '%s'" % name)
    try:
        speaker = dut.run(cmd, ignore_status=True).stdout.splitlines()[0]
    except Exception as e:
        logging.exception('Fail to find %s in cras_test_client running %s.',
                          name, cmd)
        speaker = None
    logging.info('---cmd: %s\n---output = %s', cmd, speaker)
    if speaker:
        return True, None
    return False, 'Fail to execute cli {}: Reason: {}'.format(cmd, str(e))


def check_microphone_exist_cras(dut, name):
    """
    Check microphone exists in cras.
    @param dut: The handle of the device under test.
    @param name: name of speaker
    @returns: True, None if test passes,
              False, errMsg if test fails
    """
    microphone = None
    cmd = ("cras_test_client --dump_server_info | awk "
           "'/Input Devices:/, /Input Nodes:/' "
           "| grep '%s'" % name )
    try:
        microphone = dut.run(cmd, ignore_status=True).stdout.splitlines()[0]
    except Exception as e:
        logging.exception('Fail to execute cli %s.', cmd)
    logging.info('---cmd: %s\n---output = %s', cmd, microphone)
    if microphone:
        return True, None
    return False, 'Fail to find microphone {}.'.format(name)

def check_audio_stream(dut, is_in_meet):
    """
    Verify speaker is streaming or not streaming as expected.
    @param dut: The handle of the device under test.
    @is_in_meeting: True if CfM is in meeting, False, if not
    @returns: True, None if test passes,
              False, errMsg if test fails
    """
    number_stream = get_number_of_active_streams(dut)
    if is_in_meet:
       if number_stream  >= NUM_AUDIO_STREAM_IN_MEETING:
           return True, None
       else:
           return False, 'Number of Audio streams is not expected.'
    else:
       if number_stream  <=  NUM_AUDIO_STREAM_IN_MEETING:
           return True, None
       else:
           return False, 'Number of Audio streams is not expected.'

