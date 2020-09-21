# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Handler for audio extension functionality."""

import logging

from autotest_lib.client.bin import utils
from autotest_lib.client.cros.multimedia import facade_resource

class AudioExtensionHandlerError(Exception):
    """Class for exceptions thrown from the AudioExtensionHandler"""
    pass


class AudioExtensionHandler(object):
    """Wrapper around test extension that uses chrome.audio API to get audio
    device information
    """
    def __init__(self, extension):
        """Initializes an AudioExtensionHandler.

        @param extension: Extension got from telemetry chrome wrapper.

        """
        self._extension = extension
        self._check_api_available()


    def _check_api_available(self):
        """Checks chrome.audio is available.

        @raises: AudioExtensionHandlerError if extension is not available.

        """
        success = utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "chrome.audio") != None),
                expected_value=True)
        if not success:
            raise AudioExtensionHandlerError('chrome.audio is not available.')


    @facade_resource.retry_chrome_call
    def get_audio_devices(self, device_filter=None):
        """Gets the audio device info from Chrome audio API.

        @param device_filter: Filter for returned device nodes.
            An optional dict that can have the following properties:
                string array streamTypes
                    Restricts stream types that returned devices can have.
                    It should contain "INPUT" for result to include input
                    devices, and "OUTPUT" for results to include output devices.
                    If not set, returned devices will not be filtered by the
                    stream type.

                boolean isActive
                   If true, only active devices will be included in the result.
                   If false, only inactive devices will be included in the
                   result.

            The filter param defaults to {}, requests all available audio
            devices.

        @returns: An array of audioDeviceInfo.
                  Each audioDeviceInfo dict
                  contains these key-value pairs:
                     string  id
                         The unique identifier of the audio device.

                     string stableDeviceId
                         The stable identifier of the audio device.

                     string  streamType
                         "INPUT" if the device is an input audio device,
                         "OUTPUT" if the device is an output audio device.

                     string displayName
                         The user-friendly name (e.g. "Bose Amplifier").

                     string deviceName
                         The devuce name

                     boolean isActive
                         True if this is the current active device.

                     boolean isMuted
                         True if this is muted.

                     long level
                         The output volume or input gain.

        """
        def filter_to_str(device_filter):
            """Converts python dict device filter to JS object string.

            @param device_filter: Device filter dict.

            @returns: Device filter as a srting representation of a
                      JavaScript object.

            """
            return str(device_filter or {}).replace('True', 'true').replace(
                        'False', 'false')

        self._extension.ExecuteJavaScript('window.__audio_devices = null;')
        self._extension.ExecuteJavaScript(
                "chrome.audio.getDevices(%s, function(devices) {"
                "window.__audio_devices = devices;})"
                % filter_to_str(device_filter))
        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__audio_devices") != None),
                expected_value=True)
        return self._extension.EvaluateJavaScript("window.__audio_devices")


    def _get_active_id_for_stream_type(self, stream_type):
        """Gets active node id of the specified stream type.

        Assume there is only one active node.

        @param stream_type: 'INPUT' to get the active input device,
                            'OUTPUT' to get the active output device.

        @returns: A string for the active device id.

        @raises: AudioExtensionHandlerError if active id is not unique.

        """
        nodes = self.get_audio_devices(
            {'streamTypes': [stream_type], 'isActive': True})
        if len(nodes) != 1:
            logging.error(
                    'Node info contains multiple active nodes: %s', nodes)
            raise AudioExtensionHandlerError('Active id should be unique')

        return nodes[0]['id']


    @facade_resource.retry_chrome_call
    def set_active_volume(self, volume):
        """Sets the active audio output volume using chrome.audio API.

        This method also unmutes the node.

        @param volume: Volume to set (0~100).

        """
        output_id = self._get_active_id_for_stream_type('OUTPUT')
        logging.debug('output_id: %s', output_id)

        self.set_mute(False)

        self._extension.ExecuteJavaScript('window.__set_volume_done = null;')
        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setProperties(
                    '%s',
                    {level: %s},
                    function() {window.__set_volume_done = true;});
                """
                % (output_id, volume))
        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_volume_done") != None),
                expected_value=True)


    @facade_resource.retry_chrome_call
    def set_mute(self, mute):
        """Mutes the audio output using chrome.audio API.

        @param mute: True to mute. False otherwise.

        """
        is_muted_string = 'true' if mute else 'false'

        self._extension.ExecuteJavaScript('window.__set_mute_done = null;')

        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setMute(
                    'OUTPUT', %s,
                    function() {window.__set_mute_done = true;});
                """
                % (is_muted_string))

        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_mute_done") != None),
                expected_value=True)


    @facade_resource.retry_chrome_call
    def get_mute(self):
        """Determines whether audio output is muted.

        @returns Whether audio output is muted.

        """
        self._extension.ExecuteJavaScript('window.__output_muted = null;')
        self._extension.ExecuteJavaScript(
                "chrome.audio.getMute('OUTPUT', function(isMute) {"
                "window.__output_muted = isMute;})")
        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__output_muted") != None),
                expected_value=True)
        return self._extension.EvaluateJavaScript("window.__output_muted")


    @facade_resource.retry_chrome_call
    def get_active_volume_mute(self):
        """Gets the volume state of active audio output using chrome.audio API.

        @param returns: A tuple (volume, mute), where volume is 0~100, and mute
                        is True if node is muted, False otherwise.

        """
        nodes = self.get_audio_devices(
            {'streamTypes': ['OUTPUT'], 'isActive': True})
        if len(nodes) != 1:
            logging.error('Node info contains multiple active nodes: %s', nodes)
            raise AudioExtensionHandlerError('Active id should be unique')

        return (nodes[0]['level'], self.get_mute())


    @facade_resource.retry_chrome_call
    def set_active_node_id(self, node_id):
        """Sets the active node by node id.

        The current active node will be disabled first if the new active node
        is different from the current one.

        @param node_id: Node id obtained from cras_utils.get_cras_nodes.
                        Chrome.audio also uses this id to specify input/output
                        nodes.
                        Note that node id returned by cras_utils.get_cras_nodes
                        is a number, while chrome.audio API expects a string.

        @raises AudioExtensionHandlerError if there is no such id.

        """
        nodes = self.get_audio_devices({})
        target_node = None
        for node in nodes:
            if node['id'] == str(node_id):
                target_node = node
                break

        if not target_node:
            logging.error('Node %s not found.', node_id)
            raise AudioExtensionHandlerError('Node id not found')

        if target_node['isActive']:
            logging.debug('Node %s is already active.', node_id)
            return

        logging.debug('Setting active id to %s', node_id)

        self._extension.ExecuteJavaScript('window.__set_active_done = null;')

        is_input = target_node['streamType'] == 'INPUT'
        stream_type = 'input' if is_input else 'output'
        self._extension.ExecuteJavaScript(
                """
                chrome.audio.setActiveDevices(
                    {'%s': ['%s']},
                    function() {window.__set_active_done = true;});
                """
                % (stream_type, node_id))

        utils.wait_for_value(
                lambda: (self._extension.EvaluateJavaScript(
                         "window.__set_active_done") != None),
                expected_value=True)
