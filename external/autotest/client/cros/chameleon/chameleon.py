# Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import atexit
import httplib
import logging
import os
import socket
import time
import xmlrpclib
from contextlib import contextmanager

try:
    from PIL import Image
except ImportError:
    Image = None

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros.chameleon import audio_board
from autotest_lib.client.cros.chameleon import edid as edid_lib
from autotest_lib.client.cros.chameleon import usb_controller


CHAMELEON_PORT = 9992
CHAMELEOND_LOG_REMOTE_PATH = '/var/log/chameleond'
CHAMELEON_READY_TEST = 'GetSupportedPorts'


class ChameleonConnectionError(error.TestError):
    """Indicates that connecting to Chameleon failed.

    It is fatal to the test unless caught.
    """
    pass


class _Method(object):
    """Class to save the name of the RPC method instead of the real object.

    It keeps the name of the RPC method locally first such that the RPC method
    can be evaluated to a real object while it is called. Its purpose is to
    refer to the latest RPC proxy as the original previous-saved RPC proxy may
    be lost due to reboot.

    The call_server is the method which does refer to the latest RPC proxy.

    This class and the re-connection mechanism in ChameleonConnection is
    copied from third_party/autotest/files/server/cros/faft/rpc_proxy.py

    """
    def __init__(self, call_server, name):
        """Constructs a _Method.

        @param call_server: the call_server method
        @param name: the method name or instance name provided by the
                     remote server

        """
        self.__call_server = call_server
        self._name = name


    def __getattr__(self, name):
        """Support a nested method.

        For example, proxy.system.listMethods() would need to use this method
        to get system and then to get listMethods.

        @param name: the method name or instance name provided by the
                     remote server

        @return: a callable _Method object.

        """
        return _Method(self.__call_server, "%s.%s" % (self._name, name))


    def __call__(self, *args, **dargs):
        """The call method of the object.

        @param args: arguments for the remote method.
        @param kwargs: keyword arguments for the remote method.

        @return: the result returned by the remote method.

        """
        return self.__call_server(self._name, *args, **dargs)


class ChameleonConnection(object):
    """ChameleonConnection abstracts the network connection to the board.

    When a chameleon board is rebooted, a xmlrpc call would incur a
    socket error. To fix the error, a client has to reconnect to the server.
    ChameleonConnection is a wrapper of chameleond proxy created by
    xmlrpclib.ServerProxy(). ChameleonConnection has the capability to
    automatically reconnect to the server when such socket error occurs.
    The nice feature is that the auto re-connection is performed inside this
    wrapper and is transparent to the caller.

    Note:
    1. When running chameleon autotests in lab machines, it is
       ChameleonConnection._create_server_proxy() that is invoked.
    2. When running chameleon autotests in local chroot, it is
       rpc_server_tracker.xmlrpc_connect() in server/hosts/chameleon_host.py
       that is invoked.

    ChameleonBoard and ChameleonPort use it for accessing Chameleon RPC.

    """

    def __init__(self, hostname, port=CHAMELEON_PORT, proxy_generator=None,
                 ready_test_name=CHAMELEON_READY_TEST):
        """Constructs a ChameleonConnection.

        @param hostname: Hostname the chameleond process is running.
        @param port: Port number the chameleond process is listening on.
        @param proxy_generator: a function to generate server proxy.
        @param ready_test_name: run this method on the remote server ot test
                if the server is connected correctly.

        @raise ChameleonConnectionError if connection failed.
        """
        self._hostname = hostname
        self._port = port

        # Note: it is difficult to put the lambda function as the default
        # value of the proxy_generator argument. In that case, the binding
        # of arguments (hostname and port) would be delayed until run time
        # which requires to pass an instance as an argument to labmda.
        # That becomes cumbersome since server/hosts/chameleon_host.py
        # would also pass a lambda without argument to instantiate this object.
        # Use the labmda function as follows would bind the needed arguments
        # immediately which is much simpler.
        self._proxy_generator = proxy_generator or self._create_server_proxy

        self._ready_test_name = ready_test_name
        self.chameleond_proxy = None


    def _create_server_proxy(self):
        """Creates the chameleond server proxy.

        @param hostname: Hostname the chameleond process is running.
        @param port: Port number the chameleond process is listening on.

        @return ServerProxy object to chameleond.

        @raise ChameleonConnectionError if connection failed.

        """
        remote = 'http://%s:%s' % (self._hostname, self._port)
        chameleond_proxy = xmlrpclib.ServerProxy(remote, allow_none=True)
        logging.info('ChameleonConnection._create_server_proxy() called')
        # Call a RPC to test.
        try:
            getattr(chameleond_proxy, self._ready_test_name)()
        except (socket.error,
                xmlrpclib.ProtocolError,
                httplib.BadStatusLine) as e:
            raise ChameleonConnectionError(e)
        return chameleond_proxy


    def _reconnect(self):
        """Reconnect to chameleond."""
        self.chameleond_proxy = self._proxy_generator()


    def __call_server(self, name, *args, **kwargs):
        """Bind the name to the chameleond proxy and execute the method.

        @param name: the method name or instance name provided by the
                     remote server.
        @param args: arguments for the remote method.
        @param kwargs: keyword arguments for the remote method.

        @return: the result returned by the remote method.

        """
        try:
            return getattr(self.chameleond_proxy, name)(*args, **kwargs)
        except (AttributeError, socket.error):
            # Reconnect and invoke the method again.
            logging.info('Reconnecting chameleond proxy: %s', name)
            self._reconnect()
            return getattr(self.chameleond_proxy, name)(*args, **kwargs)


    def __getattr__(self, name):
        """Get the callable _Method object.

        @param name: the method name or instance name provided by the
                     remote server

        @return: a callable _Method object.

        """
        return _Method(self.__call_server, name)


class ChameleonBoard(object):
    """ChameleonBoard is an abstraction of a Chameleon board.

    A Chameleond RPC proxy is passed to the construction such that it can
    use this proxy to control the Chameleon board.

    User can use host to access utilities that are not provided by
    Chameleond XMLRPC server, e.g. send_file and get_file, which are provided by
    ssh_host.SSHHost, which is the base class of ChameleonHost.

    """

    def __init__(self, chameleon_connection, chameleon_host=None):
        """Construct a ChameleonBoard.

        @param chameleon_connection: ChameleonConnection object.
        @param chameleon_host: ChameleonHost object. None if this ChameleonBoard
                               is not created by a ChameleonHost.
        """
        self.host = chameleon_host
        self._output_log_file = None
        self._chameleond_proxy = chameleon_connection
        self._usb_ctrl = usb_controller.USBController(chameleon_connection)
        if self._chameleond_proxy.HasAudioBoard():
            self._audio_board = audio_board.AudioBoard(chameleon_connection)
        else:
            self._audio_board = None
            logging.info('There is no audio board on this Chameleon.')


    def reset(self):
        """Resets Chameleon board."""
        self._chameleond_proxy.Reset()


    def setup_and_reset(self, output_dir=None):
        """Setup and reset Chameleon board.

        @param output_dir: Setup the output directory.
                           None for just reset the board.
        """
        if output_dir and self.host is not None:
            logging.info('setup_and_reset: dir %s, chameleon host %s',
                         output_dir, self.host.hostname)
            log_dir = os.path.join(output_dir, 'chameleond', self.host.hostname)
            # Only clear the chameleon board log and register get log callback
            # when we first create the log_dir.
            if not os.path.exists(log_dir):
                # remove old log.
                self.host.run('>%s' % CHAMELEOND_LOG_REMOTE_PATH)
                os.makedirs(log_dir)
                self._output_log_file = os.path.join(log_dir, 'log')
                atexit.register(self._get_log)
        self.reset()


    def reboot(self):
        """Reboots Chameleon board."""
        self._chameleond_proxy.Reboot()


    def _get_log(self):
        """Get log from chameleon. It will be registered by atexit.

        It's a private method. We will setup output_dir before using this
        method.
        """
        self.host.get_file(CHAMELEOND_LOG_REMOTE_PATH, self._output_log_file)


    def get_all_ports(self):
        """Gets all the ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbePorts()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_all_inputs(self):
        """Gets all the input ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbeInputs()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_all_outputs(self):
        """Gets all the output ports on Chameleon board which are connected.

        @return: A list of ChameleonPort objects.
        """
        ports = self._chameleond_proxy.ProbeOutputs()
        return [ChameleonPort(self._chameleond_proxy, port) for port in ports]


    def get_label(self):
        """Gets the label which indicates the display connection.

        @return: A string of the label, like 'hdmi', 'dp_hdmi', etc.
        """
        connectors = []
        for port in self._chameleond_proxy.ProbeInputs():
            if self._chameleond_proxy.HasVideoSupport(port):
                connector = self._chameleond_proxy.GetConnectorType(port).lower()
                connectors.append(connector)
        # Eliminate duplicated ports. It simplifies the labels of dual-port
        # devices, i.e. dp_dp categorized into dp.
        return '_'.join(sorted(set(connectors)))


    def get_audio_board(self):
        """Gets the audio board on Chameleon.

        @return: An AudioBoard object.
        """
        return self._audio_board


    def get_usb_controller(self):
        """Gets the USB controller on Chameleon.

        @return: A USBController object.
        """
        return self._usb_ctrl


    def get_bluetooth_hid_mouse(self):
        """Gets the emulated Bluetooth (BR/EDR) HID mouse on Chameleon.

        @return: A BluetoothHIDMouseFlow object.
        """
        return self._chameleond_proxy.bluetooth_mouse


    def get_bluetooth_hog_mouse(self):
        """Gets the emulated Bluetooth Low Energy HID mouse on Chameleon.

        Note that this uses HID over GATT, or HOG.

        @return: A BluetoothHOGMouseFlow object.
        """
        return self._chameleond_proxy.bluetooth_hog_mouse


    def get_avsync_probe(self):
        """Gets the avsync probe device on Chameleon.

        @return: An AVSyncProbeFlow object.
        """
        return self._chameleond_proxy.avsync_probe


    def get_motor_board(self):
        """Gets the motor_board device on Chameleon.

        @return: An MotorBoard object.
        """
        return self._chameleond_proxy.motor_board


    def get_mac_address(self):
        """Gets the MAC address of Chameleon.

        @return: A string for MAC address.
        """
        return self._chameleond_proxy.GetMacAddress()


class ChameleonPort(object):
    """ChameleonPort is an abstraction of a general port of a Chameleon board.

    It only contains some common methods shared with audio and video ports.

    A Chameleond RPC proxy and an port_id are passed to the construction.
    The port_id is the unique identity to the port.
    """

    def __init__(self, chameleond_proxy, port_id):
        """Construct a ChameleonPort.

        @param chameleond_proxy: Chameleond RPC proxy object.
        @param port_id: The ID of the input port.
        """
        self.chameleond_proxy = chameleond_proxy
        self.port_id = port_id


    def get_connector_id(self):
        """Returns the connector ID.

        @return: A number of connector ID.
        """
        return self.port_id


    def get_connector_type(self):
        """Returns the human readable string for the connector type.

        @return: A string, like "VGA", "DVI", "HDMI", or "DP".
        """
        return self.chameleond_proxy.GetConnectorType(self.port_id)


    def has_audio_support(self):
        """Returns if the input has audio support.

        @return: True if the input has audio support; otherwise, False.
        """
        return self.chameleond_proxy.HasAudioSupport(self.port_id)


    def has_video_support(self):
        """Returns if the input has video support.

        @return: True if the input has video support; otherwise, False.
        """
        return self.chameleond_proxy.HasVideoSupport(self.port_id)


    def plug(self):
        """Asserts HPD line to high, emulating plug."""
        logging.info('Plug Chameleon port %d', self.port_id)
        self.chameleond_proxy.Plug(self.port_id)


    def unplug(self):
        """Deasserts HPD line to low, emulating unplug."""
        logging.info('Unplug Chameleon port %d', self.port_id)
        self.chameleond_proxy.Unplug(self.port_id)


    def set_plug(self, plug_status):
        """Sets plug/unplug by plug_status.

        @param plug_status: True to plug; False to unplug.
        """
        if plug_status:
            self.plug()
        else:
            self.unplug()


    @property
    def plugged(self):
        """
        @returns True if this port is plugged to Chameleon, False otherwise.

        """
        return self.chameleond_proxy.IsPlugged(self.port_id)


class ChameleonVideoInput(ChameleonPort):
    """ChameleonVideoInput is an abstraction of a video input port.

    It contains some special methods to control a video input.
    """

    _DUT_STABILIZE_TIME = 3
    _DURATION_UNPLUG_FOR_EDID = 5
    _TIMEOUT_VIDEO_STABLE_PROBE = 10
    _EDID_ID_DISABLE = -1
    _FRAME_RATE = 60

    def __init__(self, chameleon_port):
        """Construct a ChameleonVideoInput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id
        self._original_edid = None


    def wait_video_input_stable(self, timeout=None):
        """Waits the video input stable or timeout.

        @param timeout: The time period to wait for.

        @return: True if the video input becomes stable within the timeout
                 period; otherwise, False.
        """
        is_input_stable = self.chameleond_proxy.WaitVideoInputStable(
                                self.port_id, timeout)

        # If video input of Chameleon has been stable, wait for DUT software
        # layer to be stable as well to make sure all the configurations have
        # been propagated before proceeding.
        if is_input_stable:
            logging.info('Video input has been stable. Waiting for the DUT'
                         ' to be stable...')
            time.sleep(self._DUT_STABILIZE_TIME)
        return is_input_stable


    def read_edid(self):
        """Reads the EDID.

        @return: An Edid object or NO_EDID.
        """
        edid_binary = self.chameleond_proxy.ReadEdid(self.port_id)
        if edid_binary is None:
            return edid_lib.NO_EDID
        # Read EDID without verify. It may be made corrupted as intended
        # for the test purpose.
        return edid_lib.Edid(edid_binary.data, skip_verify=True)


    def apply_edid(self, edid):
        """Applies the given EDID.

        @param edid: An Edid object or NO_EDID.
        """
        if edid is edid_lib.NO_EDID:
          self.chameleond_proxy.ApplyEdid(self.port_id, self._EDID_ID_DISABLE)
        else:
          edid_binary = xmlrpclib.Binary(edid.data)
          edid_id = self.chameleond_proxy.CreateEdid(edid_binary)
          self.chameleond_proxy.ApplyEdid(self.port_id, edid_id)
          self.chameleond_proxy.DestroyEdid(edid_id)


    def set_edid_from_file(self, filename):
        """Sets EDID from a file.

        The method is similar to set_edid but reads EDID from a file.

        @param filename: path to EDID file.
        """
        self.set_edid(edid_lib.Edid.from_file(filename))


    def set_edid(self, edid):
        """The complete flow of setting EDID.

        Unplugs the port if needed, sets EDID, plugs back if it was plugged.
        The original EDID is stored so user can call restore_edid after this
        call.

        @param edid: An Edid object.
        """
        plugged = self.plugged
        if plugged:
            self.unplug()

        self._original_edid = self.read_edid()

        logging.info('Apply EDID on port %d', self.port_id)
        self.apply_edid(edid)

        if plugged:
            time.sleep(self._DURATION_UNPLUG_FOR_EDID)
            self.plug()
            self.wait_video_input_stable(self._TIMEOUT_VIDEO_STABLE_PROBE)


    def restore_edid(self):
        """Restores original EDID stored when set_edid was called."""
        current_edid = self.read_edid()
        if (self._original_edid and
            self._original_edid.data != current_edid.data):
            logging.info('Restore the original EDID.')
            self.apply_edid(self._original_edid)


    @contextmanager
    def use_edid(self, edid):
        """Uses the given EDID in a with statement.

        It sets the EDID up in the beginning and restores to the original
        EDID in the end. This function is expected to be used in a with
        statement, like the following:

            with chameleon_port.use_edid(edid):
                do_some_test_on(chameleon_port)

        @param edid: An EDID object.
        """
        # Set the EDID up in the beginning.
        self.set_edid(edid)

        try:
            # Yeild to execute the with statement.
            yield
        finally:
            # Restore the original EDID in the end.
            self.restore_edid()


    def use_edid_file(self, filename):
        """Uses the given EDID file in a with statement.

        It sets the EDID up in the beginning and restores to the original
        EDID in the end. This function is expected to be used in a with
        statement, like the following:

            with chameleon_port.use_edid_file(filename):
                do_some_test_on(chameleon_port)

        @param filename: A path to the EDID file.
        """
        return self.use_edid(edid_lib.Edid.from_file(filename))


    def fire_hpd_pulse(self, deassert_interval_usec, assert_interval_usec=None,
                       repeat_count=1, end_level=1):

        """Fires one or more HPD pulse (low -> high -> low -> ...).

        @param deassert_interval_usec: The time in microsecond of the
                deassert pulse.
        @param assert_interval_usec: The time in microsecond of the
                assert pulse. If None, then use the same value as
                deassert_interval_usec.
        @param repeat_count: The count of HPD pulses to fire.
        @param end_level: HPD ends with 0 for LOW (unplugged) or 1 for
                HIGH (plugged).
        """
        self.chameleond_proxy.FireHpdPulse(
                self.port_id, deassert_interval_usec,
                assert_interval_usec, repeat_count, int(bool(end_level)))


    def fire_mixed_hpd_pulses(self, widths):
        """Fires one or more HPD pulses, starting at low, of mixed widths.

        One must specify a list of segment widths in the widths argument where
        widths[0] is the width of the first low segment, widths[1] is that of
        the first high segment, widths[2] is that of the second low segment...
        etc. The HPD line stops at low if even number of segment widths are
        specified; otherwise, it stops at high.

        @param widths: list of pulse segment widths in usec.
        """
        self.chameleond_proxy.FireMixedHpdPulses(self.port_id, widths)


    def capture_screen(self):
        """Captures Chameleon framebuffer.

        @return An Image object.
        """
        return Image.fromstring(
                'RGB',
                self.get_resolution(),
                self.chameleond_proxy.DumpPixels(self.port_id).data)


    def get_resolution(self):
        """Gets the source resolution.

        @return: A (width, height) tuple.
        """
        # The return value of RPC is converted to a list. Convert it back to
        # a tuple.
        return tuple(self.chameleond_proxy.DetectResolution(self.port_id))


    def set_content_protection(self, enable):
        """Sets the content protection state on the port.

        @param enable: True to enable; False to disable.
        """
        self.chameleond_proxy.SetContentProtection(self.port_id, enable)


    def is_content_protection_enabled(self):
        """Returns True if the content protection is enabled on the port.

        @return: True if the content protection is enabled; otherwise, False.
        """
        return self.chameleond_proxy.IsContentProtectionEnabled(self.port_id)


    def is_video_input_encrypted(self):
        """Returns True if the video input on the port is encrypted.

        @return: True if the video input is encrypted; otherwise, False.
        """
        return self.chameleond_proxy.IsVideoInputEncrypted(self.port_id)


    def start_monitoring_audio_video_capturing_delay(self):
        """Starts an audio/video synchronization utility."""
        self.chameleond_proxy.StartMonitoringAudioVideoCapturingDelay()


    def get_audio_video_capturing_delay(self):
        """Gets the time interval between the first audio/video cpatured data.

        @return: A floating points indicating the time interval between the
                 first audio/video data captured. If the result is negative,
                 then the first video data is earlier, otherwise the first
                 audio data is earlier.
        """
        return self.chameleond_proxy.GetAudioVideoCapturingDelay()


    def start_capturing_video(self, box=None):
        """
        Captures video frames. Asynchronous, returns immediately.

        @param box: int tuple, (x, y, width, height) pixel coordinates.
                    Defines the rectangular boundary within which to capture.
        """

        if box is None:
            self.chameleond_proxy.StartCapturingVideo(self.port_id)
        else:
            self.chameleond_proxy.StartCapturingVideo(self.port_id, *box)


    def stop_capturing_video(self):
        """
        Stops the ongoing video frame capturing.

        """
        self.chameleond_proxy.StopCapturingVideo()


    def get_captured_frame_count(self):
        """
        @return: int, the number of frames that have been captured.

        """
        return self.chameleond_proxy.GetCapturedFrameCount()


    def read_captured_frame(self, index):
        """
        @param index: int, index of the desired captured frame.
        @return: xmlrpclib.Binary object containing a byte-array of the pixels.

        """

        frame = self.chameleond_proxy.ReadCapturedFrame(index)
        return Image.fromstring('RGB',
                                self.get_captured_resolution(),
                                frame.data)


    def get_captured_checksums(self, start_index=0, stop_index=None):
        """
        @param start_index: int, index of the frame to start with.
        @param stop_index: int, index of the frame (excluded) to stop at.
        @return: a list of checksums of frames captured.

        """
        return self.chameleond_proxy.GetCapturedChecksums(start_index,
                                                          stop_index)


    def get_captured_fps_list(self, time_to_start=0, total_period=None):
        """
        @param time_to_start: time in second, support floating number, only
                              measure the period starting at this time.
                              If negative, it is the time before stop, e.g.
                              -2 meaning 2 seconds before stop.
        @param total_period: time in second, integer, the total measuring
                             period. If not given, use the maximum time
                             (integer) to the end.
        @return: a list of fps numbers, or [-1] if any error.

        """
        checksums = self.get_captured_checksums()

        frame_to_start = int(round(time_to_start * self._FRAME_RATE))
        if total_period is None:
            # The default is the maximum time (integer) to the end.
            total_period = (len(checksums) - frame_to_start) / self._FRAME_RATE
        frame_to_stop = frame_to_start + total_period * self._FRAME_RATE

        if frame_to_start >= len(checksums) or frame_to_stop >= len(checksums):
            logging.error('The given time interval is out-of-range.')
            return [-1]

        # Only pick the checksum we are interested.
        checksums = checksums[frame_to_start:frame_to_stop]

        # Count the unique checksums per second, i.e. FPS
        logging.debug('Output the fps info below:')
        fps_list = []
        for i in xrange(0, len(checksums), self._FRAME_RATE):
            unique_count = 0
            debug_str = ''
            for j in xrange(i, i + self._FRAME_RATE):
                if j == 0 or checksums[j] != checksums[j - 1]:
                    unique_count += 1
                    debug_str += '*'
                else:
                    debug_str += '.'
            fps_list.append(unique_count)
            logging.debug('%2dfps %s', unique_count, debug_str)

        return fps_list


    def search_fps_pattern(self, pattern_diff_frame, pattern_window=None,
                           time_to_start=0):
        """Search the captured frames and return the time where FPS is greater
        than given FPS pattern.

        A FPS pattern is described as how many different frames in a sliding
        window. For example, 5 differnt frames in a window of 60 frames.

        @param pattern_diff_frame: number of different frames for the pattern.
        @param pattern_window: number of frames for the sliding window. Default
                               is 1 second.
        @param time_to_start: time in second, support floating number,
                              start to search from the given time.
        @return: the time matching the pattern. -1.0 if not found.

        """
        if pattern_window is None:
            pattern_window = self._FRAME_RATE

        checksums = self.get_captured_checksums()

        frame_to_start = int(round(time_to_start * self._FRAME_RATE))
        first_checksum = checksums[frame_to_start]

        for i in xrange(frame_to_start + 1, len(checksums) - pattern_window):
            unique_count = 0
            for j in xrange(i, i + pattern_window):
                if j == 0 or checksums[j] != checksums[j - 1]:
                    unique_count += 1
            if unique_count >= pattern_diff_frame:
                return float(i) / self._FRAME_RATE

        return -1.0


    def get_captured_resolution(self):
        """
        @return: (width, height) tuple, the resolution of captured frames.

        """
        return self.chameleond_proxy.GetCapturedResolution()



class ChameleonAudioInput(ChameleonPort):
    """ChameleonAudioInput is an abstraction of an audio input port.

    It contains some special methods to control an audio input.
    """

    def __init__(self, chameleon_port):
        """Construct a ChameleonAudioInput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id


    def start_capturing_audio(self):
        """Starts capturing audio."""
        return self.chameleond_proxy.StartCapturingAudio(self.port_id)


    def stop_capturing_audio(self):
        """Stops capturing audio.

        Returns:
          A tuple (remote_path, format).
          remote_path: The captured file path on Chameleon.
          format: A dict containing:
            file_type: 'raw' or 'wav'.
            sample_format: 'S32_LE' for 32-bit signed integer in little-endian.
              Refer to aplay manpage for other formats.
            channel: channel number.
            rate: sampling rate.
        """
        remote_path, data_format = self.chameleond_proxy.StopCapturingAudio(
                self.port_id)
        return remote_path, data_format


class ChameleonAudioOutput(ChameleonPort):
    """ChameleonAudioOutput is an abstraction of an audio output port.

    It contains some special methods to control an audio output.
    """

    def __init__(self, chameleon_port):
        """Construct a ChameleonAudioOutput.

        @param chameleon_port: A general ChameleonPort object.
        """
        self.chameleond_proxy = chameleon_port.chameleond_proxy
        self.port_id = chameleon_port.port_id


    def start_playing_audio(self, path, data_format):
        """Starts playing audio.

        @param path: The path to the file to play on Chameleon.
        @param data_format: A dict containing data format. Currently Chameleon
                            only accepts data format:
                            dict(file_type='raw', sample_format='S32_LE',
                                 channel=8, rate=48000).

        """
        self.chameleond_proxy.StartPlayingAudio(self.port_id, path, data_format)


    def stop_playing_audio(self):
        """Stops capturing audio."""
        self.chameleond_proxy.StopPlayingAudio(self.port_id)


def make_chameleon_hostname(dut_hostname):
    """Given a DUT's hostname, returns the hostname of its Chameleon.

    @param dut_hostname: Hostname of a DUT.

    @return Hostname of the DUT's Chameleon.
    """
    host_parts = dut_hostname.split('.')
    host_parts[0] = host_parts[0] + '-chameleon'
    return '.'.join(host_parts)


def create_chameleon_board(dut_hostname, args):
    """Given either DUT's hostname or argments, creates a ChameleonBoard object.

    If the DUT's hostname is in the lab zone, it connects to the Chameleon by
    append the hostname with '-chameleon' suffix. If not, checks if the args
    contains the key-value pair 'chameleon_host=IP'.

    @param dut_hostname: Hostname of a DUT.
    @param args: A string of arguments passed from the command line.

    @return A ChameleonBoard object.

    @raise ChameleonConnectionError if unknown hostname.
    """
    connection = None
    hostname = make_chameleon_hostname(dut_hostname)
    if utils.host_is_in_lab_zone(hostname):
        connection = ChameleonConnection(hostname)
    else:
        args_dict = utils.args_to_dict(args)
        hostname = args_dict.get('chameleon_host', None)
        port = args_dict.get('chameleon_port', CHAMELEON_PORT)
        if hostname:
            connection = ChameleonConnection(hostname, port)
        else:
            raise ChameleonConnectionError('No chameleon_host is given in args')

    return ChameleonBoard(connection)
