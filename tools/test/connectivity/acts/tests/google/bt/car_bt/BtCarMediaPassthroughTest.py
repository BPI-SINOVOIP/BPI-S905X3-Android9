#/usr/bin/env python3.4
#
# Copyright (C) 2016 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""
Automated tests for the testing passthrough commands in Avrcp/A2dp profile.
"""

import os
import time

from acts.test_decorators import test_tracker_info
from acts.test_utils.bt.BluetoothBaseTest import BluetoothBaseTest
from acts.test_utils.bt import bt_test_utils
from acts.test_utils.bt import BtEnum
from acts.test_utils.car import car_media_utils
from acts.test_utils.bt.bt_test_utils import is_a2dp_connected
from acts.keys import Config


DEFAULT_WAIT_TIME = 1.0
DEFAULT_EVENT_TIMEOUT = 1.0
PHONE_MEDIA_BROWSER_SERVICE_NAME = 'BluetoothSL4AAudioSrcMBS'
CAR_MEDIA_BROWSER_SERVICE_NAME = 'A2dpMediaBrowserService'
# This test requires some media files to play, skip and compare metadata.
# The setup part of BtCarMediaPassthroughTest pushes media files from
# the local_media_path in user params defined below to ANDROID_MEDIA_PATH
# via adb. Before running these tests, place some media files in your host
# location.
ANDROID_MEDIA_PATH = '/sdcard/Music/test'


class BtCarMediaPassthroughTest(BluetoothBaseTest):
    local_media_path = ""

    def setup_class(self):
        if not super(BtCarMediaPassthroughTest, self).setup_class():
            return False
        # AVRCP roles
        self.CT = self.android_devices[0]
        self.TG = self.android_devices[1]
        # A2DP roles for the same devices
        self.SNK = self.CT
        self.SRC = self.TG
        # To keep track of the state of the MediaBrowserService
        self.mediaBrowserServiceRunning = False
        self.btAddrCT = self.CT.droid.bluetoothGetLocalAddress()
        self.btAddrTG = self.TG.droid.bluetoothGetLocalAddress()
        self.android_music_path = ANDROID_MEDIA_PATH

        if not "local_media_path" in self.user_params.keys():
            self.log.error(
                "Missing mandatory user config \"local_media_path\"!")
            return False
        self.local_media_path = self.user_params["local_media_path"]
        if type(self.local_media_path) is list:
            self.log.info("Media ready to push as is.")
        elif not os.path.isdir(self.local_media_path):
            self.local_media_path = os.path.join(
                self.user_params[Config.key_config_path],
                self.local_media_path)
            if not os.path.isdir(self.local_media_path):
                self.log.error("Unable to load user config " + self.
                               local_media_path + " from test config file.")
                return False

        # Additional time from the stack reset in setup.
        time.sleep(4)
        # Pair and connect the devices.
        if not bt_test_utils.pair_pri_to_sec(
                self.CT, self.TG, attempts=4, auto_confirm=False):
            self.log.error("Failed to pair")
            return False

        # TODO - check for Avrcp Connection state as well.
        # For now, the passthrough tests will catch Avrcp Connection failures
        # But add an explicit test for it.
        bt_test_utils.connect_pri_to_sec(
            self.SNK, self.SRC, set([BtEnum.BluetoothProfile.A2DP_SINK.value]))

        # Push media files from self.local_media_path to ANDROID_MEDIA_PATH
        # Refer to note in the beginning of file
        if type(self.local_media_path) is list:
            for item in self.local_media_path:
                self.TG.adb.push("{} {}".format(item, self.android_music_path))
        else:
            self.TG.adb.push("{} {}".format(self.local_media_path,
                                        self.android_music_path))

        return True

    def _init_mbs(self):
        """
        This is required to be done before running any of the passthrough
        commands.
        1. Starts up the AvrcpMediaBrowserService on the TG.
           This MediaBrowserService is part of the SL4A app
        2. Connects a MediaBrowser to the Carkitt's A2dpMediaBrowserService
        """
        if (not self.mediaBrowserServiceRunning):
            self.TG.log.info("Starting AvrcpMediaBrowserService")
            self.TG.droid.bluetoothMediaPhoneSL4AMBSStart()
            time.sleep(DEFAULT_WAIT_TIME)
            self.mediaBrowserServiceRunning = True

        self.CT.droid.bluetoothMediaConnectToCarMBS()
        #TODO - Wait for an event back instead of sleep
        time.sleep(DEFAULT_WAIT_TIME)

    def teardown_test(self):
        # Stop the browser service if it is running to clean up the slate.
        if self.mediaBrowserServiceRunning:
            self.TG.log.info("Stopping AvrcpMediaBrowserService")
            self.TG.droid.bluetoothMediaPhoneSL4AMBSStop()
            self.mediaBrowserServiceRunning = False
        if not super(BtCarMediaPassthroughTest, self).teardown_test():
            return False
        # If A2dp connection was disconnected as part of the test, connect it back
        if not (is_a2dp_connected(self.SNK,self.SRC)):
            result = bt_test_utils.connect_pri_to_sec(
                self.SRC, self.SNK, set([BtEnum.BluetoothProfile.A2DP.value]))
            if not result:
                if not bt_test_utils.is_a2dp_src_device_connected(
                        self.SRC, self.SNK.droid.bluetoothGetLocalAddress()):
                    self.SRC.log.error("Failed to connect on A2dp")
                    return False
        return True

    @test_tracker_info(uuid='cf4fae08-f4f6-4e0d-b00a-4f6c41d69ff9')
    @BluetoothBaseTest.bt_test_wrap
    def test_play_pause(self):
        """
        Test the Play and Pause passthrough commands

        Pre-Condition:
        1. Devices previously bonded & Connected

        Steps:
        1. Invoke Play, Pause from CT
        2. Wait to receive the corresponding received event from TG

        Returns:
        True    if the event was received
        False   if the event was not received

        Priority: 0
        """
        # Set up the MediaBrowserService
        self._init_mbs()
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG, car_media_utils.CMD_MEDIA_PLAY,
                car_media_utils.EVENT_PLAY_RECEIVED, DEFAULT_EVENT_TIMEOUT):
            return False
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG, car_media_utils.CMD_MEDIA_PAUSE,
                car_media_utils.EVENT_PAUSE_RECEIVED, DEFAULT_EVENT_TIMEOUT):
            return False
        return True

    @test_tracker_info(uuid='15615b26-3a49-4fa0-b369-41962e8de192')
    @BluetoothBaseTest.bt_test_wrap
    def test_passthrough(self):
        """
        Test the Skip Next & Skip Previous passthrough commands

        Pre-Condition:
        1. Devices previously bonded & Connected

        Steps:
        1. Invoke other passthrough commands (skip >> & <<) from CT
        2. Wait to receive the corresponding received event from TG

        Returns:
        True    if the event was received
        False   if the event was not received

        Priority: 0
        """
        # Set up the MediaBrowserService
        self._init_mbs()
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG,
                car_media_utils.CMD_MEDIA_SKIP_NEXT,
                car_media_utils.EVENT_SKIP_NEXT_RECEIVED,
                DEFAULT_EVENT_TIMEOUT):
            return False
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG,
                car_media_utils.CMD_MEDIA_SKIP_PREV,
                car_media_utils.EVENT_SKIP_PREV_RECEIVED,
                DEFAULT_EVENT_TIMEOUT):
            return False

        # Just pause media before test ends
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG, car_media_utils.CMD_MEDIA_PAUSE,
                car_media_utils.EVENT_PAUSE_RECEIVED):
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='d4103c82-6d21-486b-bc25-007f988245b9')
    def test_media_metadata(self):
        """
        Test if the metadata matches between the two ends.
        Send some random sequence of passthrough commands and compare metadata.
        TODO: truely randomize of the seq of passthrough commands.
        Pre-Condition:
        1. Devices previously bonded & Connected

        Steps:
        1. Invoke Play from CT
        2. Compare the metadata between CT and TG. Fail if they don't match
        3. Send Skip Next from CT
        4. Compare the metadata between CT and TG. Fail if they don't match
        5. Repeat steps 3 & 4
        6. Send Skip Prev from CT
        7. Compare the metadata between CT and TG. Fail if they don't match

        Returns:
        True    if the metadata matched all the way
        False   if there was a metadata mismatch at any point

        Priority: 0
        """
        if not (is_a2dp_connected(self.SNK,self.SRC)):
            self.SNK.log.error('No A2dp Connection')
            return False

        self._init_mbs()
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG, car_media_utils.CMD_MEDIA_PLAY,
                car_media_utils.EVENT_PLAY_RECEIVED, DEFAULT_EVENT_TIMEOUT):
            return False
        time.sleep(DEFAULT_WAIT_TIME)
        if not car_media_utils.check_metadata(self.log, self.TG, self.CT):
            return False

        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG,
                car_media_utils.CMD_MEDIA_SKIP_NEXT,
                car_media_utils.EVENT_SKIP_NEXT_RECEIVED,
                DEFAULT_EVENT_TIMEOUT):
            return False
        time.sleep(DEFAULT_WAIT_TIME)
        if not car_media_utils.check_metadata(self.log, self.TG, self.CT):
            return False

        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG,
                car_media_utils.CMD_MEDIA_SKIP_NEXT,
                car_media_utils.EVENT_SKIP_NEXT_RECEIVED,
                DEFAULT_EVENT_TIMEOUT):
            return False
        time.sleep(DEFAULT_WAIT_TIME)
        if not car_media_utils.check_metadata(self.log, self.TG, self.CT):
            return False

        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG,
                car_media_utils.CMD_MEDIA_SKIP_PREV,
                car_media_utils.EVENT_SKIP_PREV_RECEIVED,
                DEFAULT_EVENT_TIMEOUT):
            return False
        time.sleep(DEFAULT_WAIT_TIME)
        if not car_media_utils.check_metadata(self.log, self.TG, self.CT):
            return False

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='8f6179db-b800-4ff0-b55f-ee79e009c1a8')
    def test_disconnect_while_media_playing(self):
        """
        Disconnect BT between CT and TG in the middle of a audio streaming session and check
        1) If TG continues to still play music
        2) If CT stops playing

        Pre-Condition:
        1. Devices previously bonded & Connected

        Steps:
        1. Invoke Play from CT
        2. Check if both CT and TG are playing music by checking if the respective
           MediaSessions are active
        3. Fail if either the CT or TG is not playing
        4. Disconnect Bluetooth connection between CT and TG
        5. Check if the CT MediaSession stopped being active.
           Fail if its mediasession is still active.
        6. Check if the TG MediaSession is still active.
        7. Fail if TG stopped playing music.

        Returns:
        True    if the CT stopped playing audio and the TG continued after BT disconnect
        False   if the CT still was playing audio or TG stopped after BT disconnect.

        Priority: 0
        """
        self._init_mbs()
        self.log.info("Sending Play command from Car")
        if not car_media_utils.send_media_passthrough_cmd(
                self.log, self.CT, self.TG, car_media_utils.CMD_MEDIA_PLAY,
                car_media_utils.EVENT_PLAY_RECEIVED, DEFAULT_EVENT_TIMEOUT):
            return False

        time.sleep(DEFAULT_WAIT_TIME)

        self.TG.log.info("Phone Media Sessions:")
        if not car_media_utils.isMediaSessionActive(
                self.log, self.TG, PHONE_MEDIA_BROWSER_SERVICE_NAME):
            self.TG.log.error("Media not playing in connected Phone")
            return False

        self.CT.log.info("Car Media Sessions:")
        if not car_media_utils.isMediaSessionActive(
                self.log, self.CT, CAR_MEDIA_BROWSER_SERVICE_NAME):
            self.CT.log.error("Media not playing in connected Car")
            return False

        self.log.info("Bluetooth Disconnect the car and phone")
        result = bt_test_utils.disconnect_pri_from_sec(
            self.SRC, self.SNK, [BtEnum.BluetoothProfile.A2DP.value])
        if not result:
            if bt_test_utils.is_a2dp_src_device_connected(
                    self.SRC, self.SNK.droid.bluetoothGetLocalAddress()):
                self.SRC.log.error("Failed to disconnect on A2dp")
                return False

        self.TG.log.info("Phone Media Sessions:")
        if not car_media_utils.isMediaSessionActive(
                self.log, self.TG, PHONE_MEDIA_BROWSER_SERVICE_NAME):
            self.TG.log.error(
                "Media stopped playing in phone after BT disconnect")
            return False

        self.CT.log.info("Car Media Sessions:")
        if car_media_utils.isMediaSessionActive(
                self.log, self.CT, CAR_MEDIA_BROWSER_SERVICE_NAME):
            self.CT.log.error(
                "Media still playing in a Car after BT disconnect")
            return False

        return True

    @BluetoothBaseTest.bt_test_wrap
    @test_tracker_info(uuid='46cd95c8-2066-4018-846d-03366796e94f')
    def test_connect_while_media_playing(self):
        """
        BT connect SRC and SNK when the SRC is already playing music and verify SNK strarts streaming
        after connection.
        Connect to another device (Audio Sink) via BT while it is playing audio.
        Check if the audio starts streaming on the Sink.

        Pre-Condition:
        1. Devices previously bonded & Connected

        Steps:
        1. Disconnect TG from CT (since they are connected as a precondition)
        2. Play Music on TG (Audio SRC)
        3. Get the metadata of the playing music
        4. Connect TG and CT
        5. Check if the music is streaming on CT (Audio SNK) by checking if its MediaSession became active.
        6. Fail if CT is not streaming.
        7. Get the metdata from the CT (Audio SNK) and compare it with the metadata from Step 3
        8. Fail if the metadata did not match.

        Returns:
        True    if the event was received
        False   if the event was not received

        Priority: 0
        """
        self.log.info("Bluetooth Disconnect the car and phone")
        result = bt_test_utils.disconnect_pri_from_sec(
            self.SRC, self.SNK, [BtEnum.BluetoothProfile.A2DP.value])
        if not result:
            # Temporary timeout
            time.sleep(3)
            if bt_test_utils.is_a2dp_src_device_connected(
                    self.SRC, self.SNK.droid.bluetoothGetLocalAddress()):
                self.SRC.log.error("Failed to disconnect on A2dp")
                return False

        self._init_mbs()

        # Play Media on Phone
        self.TG.droid.bluetoothMediaHandleMediaCommandOnPhone(
            car_media_utils.CMD_MEDIA_PLAY)
        # At this point, media should be playing only on phone, not on Car, since they are disconnected
        if not car_media_utils.isMediaSessionActive(
                self.log, self.TG, PHONE_MEDIA_BROWSER_SERVICE_NAME
        ) or car_media_utils.isMediaSessionActive(
                self.log, self.CT, CAR_MEDIA_BROWSER_SERVICE_NAME):
            self.log.error("Media playing in wrong end")
            return False

        # Get the metadata of the song that the phone is playing
        metadata_TG = self.TG.droid.bluetoothMediaGetCurrentMediaMetaData()
        if metadata_TG is None:
            self.TG.log.error("No Media Metadata available from Phone")
            return False

        # Now connect to Car on Bluetooth
        if (not bt_test_utils.connect_pri_to_sec(
                self.SRC, self.SNK, set(
                    [BtEnum.BluetoothProfile.A2DP.value]))):
            return False

        # Wait for a bit for the information to show up in the car side
        time.sleep(2)

        # At this point, since we have connected while the Phone was playing media, the car
        # should automatically play.  Both devices should have their respective MediaSessions active
        if not car_media_utils.isMediaSessionActive(
                self.log, self.TG, PHONE_MEDIA_BROWSER_SERVICE_NAME):
            self.TG.log.error("Media not playing in Phone")
            return False
        if not car_media_utils.isMediaSessionActive(
                self.log, self.CT, CAR_MEDIA_BROWSER_SERVICE_NAME):
            self.CT.log.error("Media not playing in Car")
            return False

        # Get the metadata from Car and compare it with the Phone's media metadata before the connection happened.
        metadata_CT = self.CT.droid.bluetoothMediaGetCurrentMediaMetaData()
        if metadata_CT is None:
            self.CT.log.info("No Media Metadata available from car")
        return car_media_utils.compare_metadata(self.log, metadata_TG,
                                                metadata_CT)
