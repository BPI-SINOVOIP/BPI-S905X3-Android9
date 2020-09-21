# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""This module provides the utilities for chameleon streaming server usage.

Sampe Code for dump realtime video frame:
  stream = ChameleonStreamServer(IP)
  stream.reset_video_session()

  chameleon_proxy.StartCapturingVideo(port)
  stream.dump_realtime_video_frame(False, RealtimeMode.BestEffort)
  while True:
    video_frame = stream.receive_realtime_video_frame()
    if not video_frame:
        break
    (frame_number, width, height, channel, data) = video_frame
    image = Image.fromstring('RGB', (width, height), data)
    image.save('%d.bmp' % frame_number)

Sampe Code for dump realtime audio page:
  stream = ChameleonStreamServer(IP)
  stream.reset_audio_session()

  chameleon_proxy.StartCapturingAudio(port)
  stream.dump_realtime_audio_page(RealtimeMode.BestEffort)
  f = open('audio.raw',  'w')
  while True:
    audio_page = stream.receive_realtime_audio_page()
    if not audio_page:
        break
    (page_count, data) = audio_page
    f.write(data)

"""

import logging
import socket
from struct import calcsize, pack, unpack


CHAMELEON_STREAN_SERVER_PORT = 9994
SUPPORT_MAJOR_VERSION = 1
SUPPORT_MINOR_VERSION = 0


class StreamServerVersionError(Exception):
    """Version is not compatible between client and server."""
    pass


class ErrorCode(object):
    """Error codes of response from the stream server."""
    OK = 0
    NonSupportCommand = 1
    Argument = 2
    RealtimeStreamExists = 3
    VideoMemoryOverflowStop = 4
    VideoMemoryOverflowDrop = 5
    AudioMemoryOverflowStop = 6
    AudioMemoryOverflowDrop = 7
    MemoryAllocFail = 8


class RealtimeMode(object):
    """Realtime mode of dumping data."""
    # Stop dump when memory overflow
    StopWhenOverflow = 1

    # Drop data when memory overflow
    BestEffort = 2

    # Strings used for logging.
    LogStrings = ['None', 'Stop when overflow', 'Best effort']


class ChameleonStreamServer(object):
    """
    This class provides easy-to-use APIs to access the stream server.

    """

    # Main message types.
    _REQUEST_TYPE = 0
    _RESPONSE_TYPE = 1
    _DATA_TYPE = 2

    # Message types.
    _Reset = 0
    _GetVersion = 1
    _ConfigVideoStream = 2
    _ConfigShrinkVideoStream = 3
    _DumpVideoFrame = 4
    _DumpRealtimeVideoFrame = 5
    _StopDumpVideoFrame = 6
    _DumpRealtimeAudioPage = 7
    _StopDumpAudioPage = 8

    _PACKET_HEAD_SIZE = 8

    # uint16 type, uint16 error_code, uint32 length.
    packet_head_struct = '!HHL'
    # uint8 major, uint8 minor.
    version_struct = '!BB'
    # unt16 screen_width, uint16 screen_height.
    config_video_stream_struct = '!HH'
    # uint8 shrink_width, uint8 shrink_height.
    config_shrink_video_stream_struct = '!BB'
    # uint32 memory_address1, uint32 memory_address2, uint16 number_of_frames.
    dump_video_frame_struct = '!LLH'
    # uint8 is_dual, uint8 mode.
    dump_realtime_video_frame_struct = '!BB'
    # uint32 frame_number, uint16 width, uint16 height, uint8 channel,
    # uint8 padding[3]
    video_frame_data_struct = '!LHHBBBB'
    # uint8 mode.
    dump_realtime_audio_page_struct = '!B'
    # uint32 page_count.
    audio_page_data_struct = '!L'

    def __init__(self, hostname, port=CHAMELEON_STREAN_SERVER_PORT):
        """Constructs a ChameleonStreamServer.

        @param hostname: Hostname of stream server.
        @param port: Port number the stream server is listening on.

        """
        self._video_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._audio_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._hostname = hostname
        self._port = port
        # Used for non-realtime dump video frames.
        self._remain_frame_count = 0
        self._is_realtime_video = False
        self._is_realtime_audio = False

    def _get_request_type(self, message):
        """Get the request type of the message."""
        return (self._REQUEST_TYPE << 8) | message

    def _get_response_type(self, message):
        """Get the response type of the message."""
        return (self._RESPONSE_TYPE << 8) | message

    def _is_data_type(self, message):
        """Check if the message type is data type."""
        return (self._DATA_TYPE << 8) & message

    def _receive_whole_packet(self, sock):
        """Receive one whole packet, contains packet head and content.

        @param sock: Which socket to be used.

        @return: A tuple with 4 elements contains message_type, error code,
        length and content.

        """
        # receive packet header
        data = sock.recv(self._PACKET_HEAD_SIZE)
        if not data:
            return None

        while len(data) != self._PACKET_HEAD_SIZE:
            remain_length = self._PACKET_HEAD_SIZE - len(data)
            recv_content = sock.recv(remain_length)
            data += recv_content

        message_type, error_code, length = unpack(self.packet_head_struct, data)

        # receive content
        content = ''
        remain_length = length
        while remain_length:
            recv_content = sock.recv(remain_length)
            if not recv_content:
                return None
            remain_length -= len(recv_content)
            content += recv_content

        if error_code != ErrorCode.OK:
            logging.warn('Receive error code %d, %r', error_code, content)

        return (message_type, error_code, length, content)

    def _send_and_receive(self, packet, sock, check_error=True):
        """Send packet to server and receive response from server.

        @param packet: The packet to be sent.
        @param sock: Which socket to be used.
        @param check_error: Check the error code. If this is True, this function
        will check the error code from response and raise exception if the error
        code is not OK.

        @return: The response packet from server. A tuple with 4 elements
        contains message_type, error code, length and content.

        @raise ValueError if check_error and error code is not OK.

        """
        sock.send(packet)
        packet = self._receive_whole_packet(sock)
        if check_error:
            (_, error_code, _, _) = packet
            if error_code != ErrorCode.OK:
                raise ValueError('Error code is not OK')

        return packet

    def _generate_packet_head(self, message_type, length):
        """Generate packet head with request message.

        @param message_type: Message type.
        @param length: The length in the head.

        @return: The packet head content.

        """
        head = pack(self.packet_head_struct,
                    self._get_request_type(message_type),
                    ErrorCode.OK, length)
        return head

    def _generate_config_video_stream_packet(self, width, height):
        """Generate config video stream whole packet.

        @param width: The screen width of the video frame by pixel per channel.
        @param height: The screen height of the video frame by pixel per
                       channel.

        @return: The whole packet content.

        """
        content = pack(self.config_video_stream_struct, width, height)
        head = self._generate_packet_head(self._ConfigVideoStream, len(content))
        return head + content

    def _generate_config_shrink_video_stream_packet(self, shrink_width,
                                                    shrink_height):
        """Generate config shrink video stream whole packet.

        @param shrink_width: Shrink (shrink_width+1) pixels to 1 pixel when do
                             video dump.
        @param shrink_height: Shrink (shrink_height+1) to 1 height when do video
                              dump.

        @return: The whole packet content.

        """
        content = pack(self.config_shrink_video_stream_struct, shrink_width,
                       shrink_height)
        head = self._generate_packet_head(self._ConfigShrinkVideoStream,
                                          len(content))
        return head + content

    def _generate_dump_video_stream_packet(self, count, address1, address2):
        """Generate dump video stream whole packet.

        @param count: Specify number of video frames for dumping.
        @param address1: Dump memory address1.
        @param address2: Dump memory address2. If it is 0. It means we only dump
                         from address1.

        @return: The whole packet content.

        """
        content = pack(self.dump_video_frame_struct, address1, address2, count)
        head = self._generate_packet_head(self._DumpVideoFrame, len(content))
        return head + content

    def _generate_dump_realtime_video_stream_packet(self, is_dual, mode):
        """Generate dump realtime video stream whole packet.

        @param is_dual: False: means only dump from channel1,
                        True: means dump from dual channels.
        @param mode: The values of RealtimeMode.

        @return: The whole packet content.

        """
        content = pack(self.dump_realtime_video_frame_struct, is_dual, mode)
        head = self._generate_packet_head(self._DumpRealtimeVideoFrame,
                                          len(content))
        return head + content

    def _generate_dump_realtime_audio_stream_packet(self, mode):
        """Generate dump realtime audio stream whole packet.

        @param mode: The values of RealtimeMode.

        @return: The whole packet content.

        """
        content = pack(self.dump_realtime_audio_page_struct, mode)
        head = self._generate_packet_head(self._DumpRealtimeAudioPage,
                                          len(content))
        return head + content

    def _receive_video_frame(self):
        """Receive one video frame from server.

        This function will assume it only can receive video frame data packet
        from server. Unless the error code is not OK.

        @return A tuple with error code on first element.
                if error code is OK. A decoded values will be stored in a tuple.
                (error_code, frame number, width, height, channel, data)
                if error code is not OK. It will return a tuple with
                (error code, content). The content is the error message from
                server.

        @raise ValueError if packet is not data packet.

        """
        (message, error_code, _, content) = self._receive_whole_packet(
            self._video_sock)
        if error_code != ErrorCode.OK:
            return (error_code, content)

        if not self._is_data_type(message):
            raise ValueError('Message is not data')

        video_frame_head_size = calcsize(self.video_frame_data_struct)
        frame_number, width, height, channel, _, _, _ = unpack(
            self.video_frame_data_struct, content[:video_frame_head_size])
        data = content[video_frame_head_size:]
        return (error_code, frame_number, width, height, channel, data)

    def _get_version(self):
        """Get the version of the server.

        @return A tuple with Major and Minor number of the server.

        @raise ValueError if error code from response is not OK.

        """
        packet = self._generate_packet_head(self._GetVersion, 0)
        (_, _, _, content) = self._send_and_receive(packet, self._video_sock)
        return unpack(self.version_struct, content)

    def _check_version(self):
        """Check if this client is compatible with the server.

        The major number must be the same and the minor number of the server
        must larger then the client's.

        @return Compatible or not

        """
        (major, minor) = self._get_version()
        logging.debug('Major %d, minor %d', major, minor)
        return major == SUPPORT_MAJOR_VERSION and minor >= SUPPORT_MINOR_VERSION

    def connect(self):
        """Connect to the server and check the compatibility."""
        server_address = (self._hostname, self._port)
        logging.info('connecting to %s:%s', self._hostname, self._port)
        self._video_sock.connect(server_address)
        self._audio_sock.connect(server_address)
        if not self._check_version():
            raise StreamServerVersionError()

    def reset_video_session(self):
        """Reset the video session.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('Reset session')
        packet = self._generate_packet_head(self._Reset, 0)
        self._send_and_receive(packet, self._video_sock)

    def reset_audio_session(self):
        """Reset the audio session.
        For audio, we don't need to reset any thing.

        """
        pass

    def config_video_stream(self, width, height):
        """Configure the properties of the non-realtime video stream.

        @param width: The screen width of the video frame by pixel per channel.
        @param height: The screen height of the video frame by pixel per
                       channel.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('Config video, width %d, height %d', width, height)
        packet = self._generate_config_video_stream_packet(width, height)
        self._send_and_receive(packet, self._video_sock)

    def config_shrink_video_stream(self, shrink_width, shrink_height):
        """Configure the shrink operation of the video frame dump.

        @param shrink_width: Shrink (shrink_width+1) pixels to 1 pixel when do
                             video dump. 0 means no shrink.
        @param shrink_height: Shrink (shrink_height+1) to 1 height when do video
                              dump. 0 means no shrink.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('Config shrink video, shirnk_width %d, shrink_height %d',
                     shrink_width, shrink_height)
        packet = self._generate_config_shrink_video_stream_packet(shrink_width,
                                                                  shrink_height)
        self._send_and_receive(packet, self._video_sock)

    def dump_video_frame(self, count, address1, address2):
        """Ask server to dump video frames.

        User must use receive_video_frame() to receive video frames after
        calling this API.

        Sampe Code:
            address = chameleon_proxy.GetCapturedFrameAddresses(0)
            count = chameleon_proxy.GetCapturedFrameCount()
            server.dump_video_frame(count, int(address), 0)
            while True:
                video_frame = server.receive_video_frame()
                if not video_frame:
                    break
                (frame_number, width, height, channel, data) = video_frame
                image = Image.fromstring('RGB', (width, height), data)
                image.save('%s.bmp' % frame_number)

        @param count: Specify number of video frames.
        @param address1: Dump memory address1.
        @param address2: Dump memory address2. If it is 0. It means we only dump
                         from address1.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('dump video frame count %d, address1 0x%x, address2 0x%x',
                     count, address1, address2)
        packet = self._generate_dump_video_stream_packet(count, address1,
                                                         address2)
        self._send_and_receive(packet, self._video_sock)
        self._remain_frame_count = count

    def dump_realtime_video_frame(self, is_dual, mode):
        """Ask server to dump realtime video frames.

        User must use receive_realtime_video_frame() to receive video frames
        after calling this API.

        Sampe Code:
            server.dump_realtime_video_frame(False,
                                             RealtimeMode.StopWhenOverflow)
            while True:
                video_frame = server.receive_realtime_video_frame()
                if not video_frame:
                    break
                (frame_number, width, height, channel, data) = video_frame
                image = Image.fromstring('RGB', (width, height), data)
                image.save('%s.bmp' % frame_number)

        @param is_dual: False: means only dump from channel1,
                        True: means dump from dual channels.
        @param mode: The values of RealtimeMode.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('dump realtime video frame is_dual %d, mode %s', is_dual,
                     RealtimeMode.LogStrings[mode])
        packet = self._generate_dump_realtime_video_stream_packet(is_dual, mode)
        self._send_and_receive(packet, self._video_sock)
        self._is_realtime_video = True

    def receive_video_frame(self):
        """Receive one video frame from server after calling dump_video_frame().

        This function will assume it only can receive video frame data packet
        from server. Unless the error code is not OK.

        @return A tuple with video frame information.
                (frame number, width, height, channel, data)
                None if error happens.

        @raise ValueError if packet is not data packet.

        """
        if not self._remain_frame_count:
            return None
        self._remain_frame_count -= 1
        frame_info = self._receive_video_frame()
        if frame_info[0] != ErrorCode.OK:
            self._remain_frame_count = 0
            return None
        return frame_info[1:]

    def receive_realtime_video_frame(self):
        """Receive one video frame from server after calling
        dump_realtime_video_frame(). The video frame may be dropped if we use
        BestEffort mode. We can detect it by the frame number.

        This function will assume it only can receive video frame data packet
        from server. Unless the error code is not OK.

        @return A tuple with video frame information.
                (frame number, width, height, channel, data)
                None if error happens or no more frames.

        @raise ValueError if packet is not data packet.

        """
        if not self._is_realtime_video:
            return None

        frame_info = self._receive_video_frame()
        # We can still receive video frame for drop case.
        while frame_info[0] == ErrorCode.VideoMemoryOverflowDrop:
            frame_info = self._receive_video_frame()

        if frame_info[0] != ErrorCode.OK:
            return None

        return frame_info[1:]

    def stop_dump_realtime_video_frame(self):
        """Ask server to stop dump realtime video frame."""
        if not self._is_realtime_video:
            return
        packet = self._generate_packet_head(self._StopDumpVideoFrame, 0)
        self._video_sock.send(packet)
        # Drop video frames until receive _StopDumpVideoFrame response.
        while True:
            (message, _, _, _) = self._receive_whole_packet(self._video_sock)
            if message == self._get_response_type(self._StopDumpVideoFrame):
                break
        self._is_realtime_video = False

    def dump_realtime_audio_page(self, mode):
        """Ask server to dump realtime audio pages.

        User must use receive_realtime_audio_page() to receive audio pages
        after calling this API.

        Sampe Code for BestEffort:
            server.dump_realtime_audio_page(RealtimeMode.kBestEffort)
            f = open('audio.raw'), 'w')
            while True:
                audio_page = server.receive_realtime_audio_page()
                if audio_page:
                    break
                (page_count, data) = audio_page
                f.write(data)

        @param mode: The values of RealtimeMode.

        @raise ValueError if error code from response is not OK.

        """
        logging.info('dump realtime audio page mode %s',
                     RealtimeMode.LogStrings[mode])
        packet = self._generate_dump_realtime_audio_stream_packet(mode)
        self._send_and_receive(packet, self._audio_sock)
        self._is_realtime_audio = True

    def receive_realtime_audio_page(self):
        """Receive one audio page from server after calling
        dump_realtime_audio_page(). The behavior is the same as
        receive_realtime_video_frame(). The audio page may be dropped if we use
        BestEffort mode. We can detect it by the page count.

        This function will assume it only can receive audio page data packet
        from server. Unless the error code is not OK.

        @return A tuple with audio page information. (page count, data)
                None if error happens or no more frames.

        @raise ValueError if packet is not data packet.

        """
        if not self._is_realtime_audio:
            return None
        (message, error_code, _, content) = self._receive_whole_packet(
            self._audio_sock)
        # We can still receive audio page for drop case.
        while error_code == ErrorCode.AudioMemoryOverflowDrop:
            (message, error_code, _, content) = self._receive_whole_packet(
                self._audio_sock)

        if error_code != ErrorCode.OK:
            return None
        if not self._is_data_type(message):
            raise ValueError('Message is not data')

        page_count = unpack(self.audio_page_data_struct, content[:4])[0]
        data = content[4:]
        return (page_count, data)

    def stop_dump_realtime_audio_page(self):
        """Ask server to stop dump realtime audio page."""
        if not self._is_realtime_audio:
            return
        packet = self._generate_packet_head(self._StopDumpAudioPage, 0)
        self._audio_sock.send(packet)
        # Drop audio pages until receive _StopDumpAudioPage response.
        while True:
            (message, _, _, _) = self._receive_whole_packet(self._audio_sock)
            if message == self._get_response_type(self._StopDumpAudioPage):
                break
        self._is_realtime_audio = False
