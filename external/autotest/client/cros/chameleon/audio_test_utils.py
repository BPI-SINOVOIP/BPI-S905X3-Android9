# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the test utilities for audio tests using chameleon."""

# TODO (cychiang) Move test utilities from chameleon_audio_helpers
# to this module.

import logging
import multiprocessing
import os
import pprint
import re
from contextlib import contextmanager

from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants
from autotest_lib.client.cros.audio import audio_analysis
from autotest_lib.client.cros.audio import audio_spec
from autotest_lib.client.cros.audio import audio_data
from autotest_lib.client.cros.audio import audio_helper
from autotest_lib.client.cros.audio import audio_quality_measurement
from autotest_lib.client.cros.chameleon import chameleon_audio_ids

CHAMELEON_AUDIO_IDS_TO_CRAS_NODE_TYPES = {
       chameleon_audio_ids.CrosIds.HDMI: 'HDMI',
       chameleon_audio_ids.CrosIds.HEADPHONE: 'HEADPHONE',
       chameleon_audio_ids.CrosIds.EXTERNAL_MIC: 'MIC',
       chameleon_audio_ids.CrosIds.SPEAKER: 'INTERNAL_SPEAKER',
       chameleon_audio_ids.CrosIds.INTERNAL_MIC: 'INTERNAL_MIC',
       chameleon_audio_ids.CrosIds.BLUETOOTH_HEADPHONE: 'BLUETOOTH',
       chameleon_audio_ids.CrosIds.BLUETOOTH_MIC: 'BLUETOOTH',
       chameleon_audio_ids.CrosIds.USBIN: 'USB',
       chameleon_audio_ids.CrosIds.USBOUT: 'USB',
}


def cros_port_id_to_cras_node_type(port_id):
    """Gets Cras node type from Cros port id.

    @param port_id: A port id defined in chameleon_audio_ids.CrosIds.

    @returns: A Cras node type defined in cras_utils.CRAS_NODE_TYPES.

    """
    return CHAMELEON_AUDIO_IDS_TO_CRAS_NODE_TYPES[port_id]


def check_output_port(audio_facade, port_id):
    """Checks selected output node on Cros device is correct for a port.

    @param port_id: A port id defined in chameleon_audio_ids.CrosIds.

    """
    output_node_type = cros_port_id_to_cras_node_type(port_id)
    check_audio_nodes(audio_facade, ([output_node_type], None))


def check_input_port(audio_facade, port_id):
    """Checks selected input node on Cros device is correct for a port.

    @param port_id: A port id defined in chameleon_audio_ids.CrosIds.

    """
    input_node_type = cros_port_id_to_cras_node_type(port_id)
    check_audio_nodes(audio_facade, (None, [input_node_type]))


def check_audio_nodes(audio_facade, audio_nodes):
    """Checks the node selected by Cros device is correct.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @param audio_nodes: A tuple (out_audio_nodes, in_audio_nodes) containing
                        expected selected output and input nodes.

    @raises: error.TestFail if the nodes selected by Cros device are not expected.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_selected_node_types()
    out_audio_nodes, in_audio_nodes = audio_nodes
    if (in_audio_nodes != None and
        sorted(curr_in_nodes) != sorted(in_audio_nodes)):
        raise error.TestFail('Wrong input node(s) selected: %s '
                'expected: %s' % (str(curr_in_nodes), str(in_audio_nodes)))

    # Treat line-out node as headphone node in Chameleon test since some
    # Cros devices detect audio board as lineout. This actually makes sense
    # because 3.5mm audio jack is connected to LineIn port on Chameleon.
    if (out_audio_nodes == ['HEADPHONE'] and curr_out_nodes == ['LINEOUT']):
        return

    if (out_audio_nodes != None and
        sorted(curr_out_nodes) != sorted(out_audio_nodes)):
        raise error.TestFail('Wrong output node(s) selected %s '
                'expected: %s' % (str(curr_out_nodes), str(out_audio_nodes)))


def check_plugged_nodes(audio_facade, audio_nodes):
    """Checks the nodes that are currently plugged on Cros device are correct.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @param audio_nodes: A tuple (out_audio_nodes, in_audio_nodes) containing
                        expected plugged output and input nodes.

    @raises: error.TestFail if the plugged nodes on Cros device are not expected.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_plugged_node_types()
    out_audio_nodes, in_audio_nodes = audio_nodes
    if (in_audio_nodes != None and
        sorted(curr_in_nodes) != sorted(in_audio_nodes)):
        raise error.TestFail('Wrong input node(s) plugged: %s '
                'expected: %s!' % (str(curr_in_nodes), str(in_audio_nodes)))
    if (out_audio_nodes != None and
        sorted(curr_out_nodes) != sorted(out_audio_nodes)):
        raise error.TestFail('Wrong output node(s) plugged: %s '
                'expected: %s!' % (str(curr_out_nodes), str(out_audio_nodes)))


def bluetooth_nodes_plugged(audio_facade):
    """Checks bluetooth nodes are plugged.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @raises: error.TestFail if either input or output bluetooth node is
             not plugged.

    """
    curr_out_nodes, curr_in_nodes = audio_facade.get_plugged_node_types()
    return 'BLUETOOTH' in curr_out_nodes and 'BLUETOOTH' in curr_in_nodes


def _get_board_name(host):
    """Gets the board name.

    @param host: The CrosHost object.

    @returns: The board name.

    """
    return host.get_board().split(':')[1]


def has_internal_speaker(host):
    """Checks if the Cros device has speaker.

    @param host: The CrosHost object.

    @returns: True if Cros device has internal speaker. False otherwise.

    """
    board_name = _get_board_name(host)
    if not audio_spec.has_internal_speaker(host.get_board_type(), board_name):
        logging.info('Board %s does not have speaker.', board_name)
        return False
    return True


def has_internal_microphone(host):
    """Checks if the Cros device has internal microphone.

    @param host: The CrosHost object.

    @returns: True if Cros device has internal microphone. False otherwise.

    """
    board_name = _get_board_name(host)
    if not audio_spec.has_internal_microphone(host.get_board_type()):
        logging.info('Board %s does not have internal microphone.', board_name)
        return False
    return True


def has_headphone(host):
    """Checks if the Cros device has headphone.

    @param host: The CrosHost object.

    @returns: True if Cros device has headphone. False otherwise.

    """
    board_name = _get_board_name(host)
    if not audio_spec.has_headphone(host.get_board_type()):
        logging.info('Board %s does not have headphone.', board_name)
        return False
    return True


def suspend_resume(host, suspend_time_secs, resume_network_timeout_secs=50):
    """Performs the suspend/resume on Cros device.

    @param suspend_time_secs: Time in seconds to let Cros device suspend.
    @resume_network_timeout_secs: Time in seconds to let Cros device resume and
                                  obtain network.
    """
    def action_suspend():
        """Calls the host method suspend."""
        host.suspend(suspend_time=suspend_time_secs)

    boot_id = host.get_boot_id()
    proc = multiprocessing.Process(target=action_suspend)
    logging.info("Suspending...")
    proc.daemon = True
    proc.start()
    host.test_wait_for_sleep(suspend_time_secs / 3)
    logging.info("DUT suspended! Waiting to resume...")
    host.test_wait_for_resume(
            boot_id, suspend_time_secs + resume_network_timeout_secs)
    logging.info("DUT resumed!")


def dump_cros_audio_logs(host, audio_facade, directory, suffix=''):
    """Dumps logs for audio debugging from Cros device.

    @param host: The CrosHost object.
    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.
    @directory: The directory to dump logs.

    """
    def get_file_path(name):
        """Gets file path to dump logs.

        @param name: The file name.

        @returns: The file path with an optional suffix.

        """
        file_name = '%s.%s' % (name, suffix) if suffix else name
        file_path = os.path.join(directory, file_name)
        return file_path

    audio_facade.dump_diagnostics(get_file_path('audio_diagnostics.txt'))

    host.get_file('/var/log/messages', get_file_path('messages'))

    host.get_file(constants.MULTIMEDIA_XMLRPC_SERVER_LOG_FILE,
                  get_file_path('multimedia_xmlrpc_server.log'))


def examine_audio_diagnostics(path):
    """Examines audio diagnostic content.

    @param path: Path to audio diagnostic file.

    @returns: Warning messages or ''.

    """
    warning_msgs = []
    line_number = 1

    underrun_pattern = re.compile('num_underruns: (\d*)')

    with open(path) as f:
        for line in f.readlines():

            # Check for number of underruns.
            search_result = underrun_pattern.search(line)
            if search_result:
                num_underruns = int(search_result.group(1))
                if num_underruns != 0:
                    warning_msgs.append(
                            'Found %d underrun at line %d: %s' % (
                                    num_underruns, line_number, line))

            # TODO(cychiang) add other check like maximum client reply delay.
            line_number = line_number + 1

    if warning_msgs:
        return ('Found issue in audio diganostics result : %s' %
                '\n'.join(warning_msgs))

    logging.info('audio_diagnostic result looks fine')
    return ''


@contextmanager
def monitor_no_nodes_changed(audio_facade, callback=None):
    """Context manager to monitor nodes changed signal on Cros device.

    Starts the counter in the beginning. Stops the counter in the end to make
    sure there is no NodesChanged signal during the try block.

    E.g. with monitor_no_nodes_changed(audio_facade):
             do something on playback/recording

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.
    @param fail_callback: The callback to call before raising TestFail
                          when there is unexpected NodesChanged signals.

    @raises: error.TestFail if there is NodesChanged signal on
             Cros device during the context.

    """
    try:
        audio_facade.start_counting_signal('NodesChanged')
        yield
    finally:
        count = audio_facade.stop_counting_signal()
        if count:
            message = 'Got %d unexpected NodesChanged signal' % count
            logging.error(message)
            if callback:
                callback()
            raise error.TestFail(message)


# The second dominant frequency should have energy less than -26dB of the
# first dominant frequency in the spectrum.
_DEFAULT_SECOND_PEAK_RATIO = 0.05

# Tolerate more noise for bluetooth audio using HSP.
_HSP_SECOND_PEAK_RATIO = 0.2

# Tolerate more noise for speaker.
_SPEAKER_SECOND_PEAK_RATIO = 0.1

# Tolerate more noise for internal microphone.
_INTERNAL_MIC_SECOND_PEAK_RATIO = 0.2

# maximum tolerant noise level
DEFAULT_TOLERANT_NOISE_LEVEL = 0.01

# If relative error of two durations is less than 0.2,
# they will be considered equivalent.
DEFAULT_EQUIVALENT_THRESHOLD = 0.2

# The frequency at lower than _DC_FREQ_THRESHOLD should have coefficient
# smaller than _DC_COEFF_THRESHOLD.
_DC_FREQ_THRESHOLD = 0.001
_DC_COEFF_THRESHOLD = 0.01

def get_second_peak_ratio(source_id, recorder_id, is_hsp=False):
    """Gets the second peak ratio suitable for use case.

    @param source_id: ID defined in chameleon_audio_ids for source widget.
    @param recorder_id: ID defined in chameleon_audio_ids for recorder widget.
    @param is_hsp: For bluetooth HSP use case.

    @returns: A float for proper second peak ratio to be used in
              check_recorded_frequency.
    """
    if is_hsp:
        return _HSP_SECOND_PEAK_RATIO
    elif source_id == chameleon_audio_ids.CrosIds.SPEAKER:
        return _SPEAKER_SECOND_PEAK_RATIO
    elif recorder_id == chameleon_audio_ids.CrosIds.INTERNAL_MIC:
        return _INTERNAL_MIC_SECOND_PEAK_RATIO
    else:
        return _DEFAULT_SECOND_PEAK_RATIO


# The deviation of estimated dominant frequency from golden frequency.
DEFAULT_FREQUENCY_DIFF_THRESHOLD = 5

def check_recorded_frequency(
        golden_file, recorder,
        second_peak_ratio=_DEFAULT_SECOND_PEAK_RATIO,
        frequency_diff_threshold=DEFAULT_FREQUENCY_DIFF_THRESHOLD,
        ignore_frequencies=None, check_anomaly=False, check_artifacts=False,
        mute_durations=None, volume_changes=None,
        tolerant_noise_level=DEFAULT_TOLERANT_NOISE_LEVEL):
    """Checks if the recorded data contains sine tone of golden frequency.

    @param golden_file: An AudioTestData object that serves as golden data.
    @param recorder: An AudioWidget used in the test to record data.
    @param second_peak_ratio: The test fails when the second dominant
                              frequency has coefficient larger than this
                              ratio of the coefficient of first dominant
                              frequency.
    @param frequency_diff_threshold: The maximum difference between estimated
                                     frequency of test signal and golden
                                     frequency. This value should be small for
                                     signal passed through line.
    @param ignore_frequencies: A list of frequencies to be ignored. The
                               component in the spectral with frequency too
                               close to the frequency in the list will be
                               ignored. The comparison of frequencies uses
                               frequency_diff_threshold as well.
    @param check_anomaly: True to check anomaly in the signal.
    @param check_artifacts: True to check artifacts in the signal.
    @param mute_durations: Each duration of mute in seconds in the signal.
    @param volume_changes: A list containing alternative -1 for decreasing
                           volume and +1 for increasing volume.
    @param tolerant_noise_level: The maximum noise level can be tolerated

    @returns: A list containing tuples of (dominant_frequency, coefficient) for
              valid channels. Coefficient can be a measure of signal magnitude
              on that dominant frequency. Invalid channels where golden_channel
              is None are ignored.

    @raises error.TestFail if the recorded data does not contain sine tone of
            golden frequency.

    """
    if not ignore_frequencies:
        ignore_frequencies = []

    # Also ignore harmonics of ignore frequencies.
    ignore_frequencies_harmonics = []
    for ignore_freq in ignore_frequencies:
        ignore_frequencies_harmonics += [ignore_freq * n for n in xrange(1, 4)]

    data_format = recorder.data_format
    recorded_data = audio_data.AudioRawData(
            binary=recorder.get_binary(),
            channel=data_format['channel'],
            sample_format=data_format['sample_format'])

    errors = []
    dominant_spectrals = []

    for test_channel, golden_channel in enumerate(recorder.channel_map):
        if golden_channel is None:
            logging.info('Skipped channel %d', test_channel)
            continue

        signal = recorded_data.channel_data[test_channel]
        saturate_value = audio_data.get_maximum_value_from_sample_format(
                data_format['sample_format'])
        logging.debug('Channel %d max signal: %f', test_channel, max(signal))
        normalized_signal = audio_analysis.normalize_signal(
                signal, saturate_value)
        logging.debug('saturate_value: %f', saturate_value)
        logging.debug('max signal after normalized: %f', max(normalized_signal))
        spectral = audio_analysis.spectral_analysis(
                normalized_signal, data_format['rate'])
        logging.debug('spectral: %s', spectral)

        if not spectral:
            errors.append(
                    'Channel %d: Can not find dominant frequency.' %
                            test_channel)

        golden_frequency = golden_file.frequencies[golden_channel]
        logging.debug('Checking channel %s spectral %s against frequency %s',
                test_channel, spectral, golden_frequency)

        dominant_frequency = spectral[0][0]

        if (abs(dominant_frequency - golden_frequency) >
            frequency_diff_threshold):
            errors.append(
                    'Channel %d: Dominant frequency %s is away from golden %s' %
                    (test_channel, dominant_frequency, golden_frequency))

        if check_anomaly:
            detected_anomaly = audio_analysis.anomaly_detection(
                    signal=normalized_signal,
                    rate=data_format['rate'],
                    freq=golden_frequency)
            if detected_anomaly:
                errors.append(
                        'Channel %d: Detect anomaly near these time: %s' %
                        (test_channel, detected_anomaly))
            else:
                logging.info(
                        'Channel %d: Quality is good as there is no anomaly',
                        test_channel)

        if check_artifacts or mute_durations or volume_changes:
            result = audio_quality_measurement.quality_measurement(
                                        normalized_signal,
                                        data_format['rate'],
                                        dominant_frequency=dominant_frequency)
            logging.debug('Quality measurement result:\n%s', pprint.pformat(result))
            if check_artifacts:
                if len(result['artifacts']['noise_before_playback']) > 0:
                    errors.append(
                        'Channel %d: Detects artifacts before playing near'
                        ' these time and duration: %s' %
                        (test_channel,
                         str(result['artifacts']['noise_before_playback'])))

                if len(result['artifacts']['noise_after_playback']) > 0:
                    errors.append(
                        'Channel %d: Detects artifacts after playing near'
                        ' these time and duration: %s' %
                        (test_channel,
                         str(result['artifacts']['noise_after_playback'])))

            if mute_durations:
                delays = result['artifacts']['delay_during_playback']
                delay_durations = []
                for x in delays:
                    delay_durations.append(x[1])
                mute_matched, delay_matched = longest_common_subsequence(
                        mute_durations,
                        delay_durations,
                        DEFAULT_EQUIVALENT_THRESHOLD)

                # updated delay list
                new_delays = [delays[i]
                                for i in delay_matched if not delay_matched[i]]

                result['artifacts']['delay_during_playback'] = new_delays

                unmatched_mutes = [mute_durations[i]
                                for i in mute_matched if not mute_matched[i]]

                if len(unmatched_mutes) > 0:
                    errors.append(
                        'Channel %d: Unmatched mute duration: %s' %
                        (test_channel, unmatched_mutes))

            if check_artifacts:
                if len(result['artifacts']['delay_during_playback']) > 0:
                    errors.append(
                        'Channel %d: Detects delay during playing near'
                        ' these time and duration: %s' %
                        (test_channel,
                         result['artifacts']['delay_during_playback']))

                if len(result['artifacts']['burst_during_playback']) > 0:
                    errors.append(
                        'Channel %d: Detects burst/pop near these time: %s' %
                        (test_channel,
                         result['artifacts']['burst_during_playback']))

                if result['equivalent_noise_level'] > tolerant_noise_level:
                    errors.append(
                        'Channel %d: noise level is higher than tolerant'
                        ' noise level: %f > %f' %
                        (test_channel,
                         result['equivalent_noise_level'],
                         tolerant_noise_level))

            if volume_changes:
                matched = True
                volume_changing = result['volume_changes']
                if len(volume_changing) != len(volume_changes):
                    matched = False
                else:
                    for i in xrange(len(volume_changing)):
                        if volume_changing[i][1] != volume_changes[i]:
                            matched = False
                            break
                if not matched:
                    errors.append(
                        'Channel %d: volume changing is not as expected, '
                        'found changing time and events are: %s while '
                        'expected changing events are %s'%
                        (test_channel,
                         volume_changing,
                         volume_changes))

        # Filter out the harmonics resulted from imperfect sin wave.
        # This list is different for different channels.
        harmonics = [dominant_frequency * n for n in xrange(2, 10)]

        def should_be_ignored(frequency):
            """Checks if frequency is close to any frequency in ignore list.

            The ignore list is harmonics of frequency to be ignored
            (like power noise), plus harmonics of dominant frequencies,
            plus DC.

            @param frequency: The frequency to be tested.

            @returns: True if the frequency should be ignored. False otherwise.

            """
            for ignore_frequency in (ignore_frequencies_harmonics + harmonics
                                     + [0.0]):
                if (abs(frequency - ignore_frequency) <
                    frequency_diff_threshold):
                    logging.debug('Ignore frequency: %s', frequency)
                    return True

        # Checks DC is small enough.
        for freq, coeff in spectral:
            if freq < _DC_FREQ_THRESHOLD and coeff > _DC_COEFF_THRESHOLD:
                errors.append(
                        'Channel %d: Found large DC coefficient: '
                        '(%f Hz, %f)' % (test_channel, freq, coeff))

        # Filter out the frequencies to be ignored.
        spectral_post_ignore = [
                x for x in spectral if not should_be_ignored(x[0])]

        if len(spectral_post_ignore) > 1:
            first_coeff = spectral_post_ignore[0][1]
            second_coeff = spectral_post_ignore[1][1]
            if second_coeff > first_coeff * second_peak_ratio:
                errors.append(
                        'Channel %d: Found large second dominant frequencies: '
                        '%s' % (test_channel, spectral_post_ignore))

        if not spectral_post_ignore:
            errors.append(
                    'Channel %d: No frequency left after removing unwanted '
                    'frequencies. Spectral: %s; After removing unwanted '
                    'frequencies: %s' %
                    (test_channel, spectral, spectral_post_ignore))

        else:
            dominant_spectrals.append(spectral_post_ignore[0])

    if errors:
        raise error.TestFail(', '.join(errors))

    return dominant_spectrals


def longest_common_subsequence(list1, list2, equivalent_threshold):
    """Finds longest common subsequence of list1 and list2

    Such as list1: [0.3, 0.4],
            list2: [0.001, 0.299, 0.002, 0.401, 0.001]
            equivalent_threshold: 0.001
    it will return matched1: [True, True],
                   matched2: [False, True, False, True, False]

    @param list1: a list of integer or float value
    @param list2: a list of integer or float value
    @param equivalent_threshold: two values are considered equivalent if their
                                 relative error is less than
                                 equivalent_threshold.

    @returns: a tuple of list (matched_1, matched_2) indicating each item
              of list1 and list2 are matched or not.

    """
    length1, length2 = len(list1), len(list2)
    matching = [[0] * (length2 + 1)] * (length1 + 1)
    # matching[i][j] is the maximum number of matched pairs for first i items
    # in list1 and first j items in list2.
    for i in xrange(length1):
        for j in xrange(length2):
            # Maximum matched pairs may be obtained without
            # i-th item in list1 or without j-th item in list2
            matching[i + 1][j + 1] = max(matching[i + 1][j],
                                         matching[i][j + 1])
            diff = abs(list1[i] - list2[j])
            relative_error = diff / list1[i]
            # If i-th item in list1 can be matched to j-th item in list2
            if relative_error < equivalent_threshold:
                matching[i + 1][j + 1] = matching[i][j] + 1

    # Backtracking which item in list1 and list2 are matched
    matched1 = [False] * length1
    matched2 = [False] * length2
    i, j = length1, length2
    while i > 0 and j > 0:
        # Maximum number is obtained by matching i-th item in list1
        # and j-th one in list2.
        if matching[i][j] == matching[i - 1][j - 1] + 1:
            matched1[i - 1] = True
            matched2[j - 1] = True
            i, j = i - 1, j - 1
        elif matching[i][j] == matching[i - 1][j]:
            i -= 1
        else:
            j -= 1
    return (matched1, matched2)


def switch_to_hsp(audio_facade):
    """Switches to HSP profile.

    Selects bluetooth microphone and runs a recording process on Cros device.
    This triggers bluetooth profile be switched from A2DP to HSP.
    Note the user can call stop_recording on audio facade to stop the recording
    process, or let multimedia_xmlrpc_server terminates it in its cleanup.

    """
    audio_facade.set_chrome_active_node_type(None, 'BLUETOOTH')
    check_audio_nodes(audio_facade, (None, ['BLUETOOTH']))
    audio_facade.start_recording(
            dict(file_type='raw', sample_format='S16_LE', channel=2,
                 rate=48000))


def compare_recorded_correlation(golden_file, recorder, parameters=None):
    """Checks recorded audio in an AudioInputWidget against a golden file.

    Compares recorded data with golden data by cross correlation method.
    Refer to audio_helper.compare_data for details of comparison.

    @param golden_file: An AudioTestData object that serves as golden data.
    @param recorder: An AudioInputWidget that has recorded some audio data.
    @param parameters: A dict containing parameters for method.

    """
    logging.info('Comparing recorded data with golden file %s ...',
                 golden_file.path)
    audio_helper.compare_data_correlation(
            golden_file.get_binary(), golden_file.data_format,
            recorder.get_binary(), recorder.data_format, recorder.channel_map,
            parameters)


def check_and_set_chrome_active_node_types(audio_facade, output_type=None,
                                           input_type=None):
   """Check the target types are available, and set them to be active nodes.

   @param audio_facade: An AudioFacadeNative or AudioFacadeAdapter object.
   @output_type: An output node type defined in cras_utils.CRAS_NODE_TYPES.
                 None to skip.
   @input_type: An input node type defined in cras_utils.CRAS_NODE_TYPES.
                 None to skip.

   @raises: error.TestError if the expected node type is missing. We use
            error.TestError here because usually this step is not the main
            purpose of the test, but a setup step.

   """
   output_types, input_types = audio_facade.get_plugged_node_types()
   logging.debug('Plugged types: output: %r, input: %r',
                 output_types, input_types)
   if output_type and output_type not in output_types:
       raise error.TestError(
               'Target output type %s not present' % output_type)
   if input_type and input_type not in input_types:
       raise error.TestError(
               'Target input type %s not present' % input_type)
   audio_facade.set_chrome_active_node_type(output_type, input_type)


def check_hp_or_lineout_plugged(audio_facade):
    """Checks whether line-out or headphone is plugged.

    @param audio_facade: A RemoteAudioFacade to access audio functions on
                         Cros device.

    @returns: 'LINEOUT' if line-out node is plugged.
              'HEADPHONE' if headphone node is plugged.

    @raises: error.TestFail if the plugged nodes does not contain one of
             'LINEOUT' and 'HEADPHONE'.

    """
    # Checks whether line-out or headphone is detected.
    output_nodes, _ = audio_facade.get_plugged_node_types()
    if 'LINEOUT' in output_nodes:
        return 'LINEOUT'
    if 'HEADPHONE' in output_nodes:
        return 'HEADPHONE'
    raise error.TestFail('Can not detect line-out or headphone')
