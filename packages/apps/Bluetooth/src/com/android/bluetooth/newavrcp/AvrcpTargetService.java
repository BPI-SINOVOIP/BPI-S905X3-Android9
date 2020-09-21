/*
 * Copyright 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.bluetooth.avrcp;

import android.bluetooth.BluetoothA2dp;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.IBluetoothAvrcpTarget;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.media.AudioManager;
import android.os.Looper;
import android.os.SystemProperties;
import android.os.UserManager;
import android.util.Log;

import com.android.bluetooth.BluetoothMetricsProto;
import com.android.bluetooth.Utils;
import com.android.bluetooth.a2dp.A2dpService;
import com.android.bluetooth.btservice.MetricsLogger;
import com.android.bluetooth.btservice.ProfileService;

import java.util.List;
import java.util.Objects;

/**
 * Provides Bluetooth AVRCP Target profile as a service in the Bluetooth application.
 * @hide
 */
public class AvrcpTargetService extends ProfileService {
    private static final String TAG = "NewAvrcpTargetService";
    private static final boolean DEBUG = true;
    private static final String AVRCP_ENABLE_PROPERTY = "persist.bluetooth.enablenewavrcp";

    private static final int AVRCP_MAX_VOL = 127;
    private static int sDeviceMaxVolume = 0;

    private MediaPlayerList mMediaPlayerList;
    private AudioManager mAudioManager;
    private AvrcpBroadcastReceiver mReceiver;
    private AvrcpNativeInterface mNativeInterface;
    private AvrcpVolumeManager mVolumeManager;

    // Only used to see if the metadata has changed from its previous value
    private MediaData mCurrentData;

    private static AvrcpTargetService sInstance = null;

    class ListCallback implements MediaPlayerList.MediaUpdateCallback,
            MediaPlayerList.FolderUpdateCallback {
        @Override
        public void run(MediaData data) {
            boolean metadata = !Objects.equals(mCurrentData.metadata, data.metadata);
            boolean state = !MediaPlayerWrapper.playstateEquals(mCurrentData.state, data.state);
            boolean queue = !Objects.equals(mCurrentData.queue, data.queue);

            if (DEBUG) {
                Log.d(TAG, "onMediaUpdated: track_changed=" + metadata
                        + " state=" + state + " queue=" + queue);
            }
            mCurrentData = data;

            mNativeInterface.sendMediaUpdate(metadata, state, queue);
        }

        @Override
        public void run(boolean availablePlayers, boolean addressedPlayers,
                boolean uids) {
            mNativeInterface.sendFolderUpdate(availablePlayers, addressedPlayers, uids);
        }
    }

    private class AvrcpBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(BluetoothA2dp.ACTION_ACTIVE_DEVICE_CHANGED)) {
                if (mNativeInterface == null) return;

                // Update all the playback status info for each connected device
                mNativeInterface.sendMediaUpdate(false, true, false);
            }
        }
    }

    /**
     * Get the AvrcpTargetService instance. Returns null if the service hasn't been initialized.
     */
    public static AvrcpTargetService get() {
        return sInstance;
    }

    @Override
    public String getName() {
        return TAG;
    }

    @Override
    protected IProfileServiceBinder initBinder() {
        return new AvrcpTargetBinder(this);
    }

    @Override
    protected void setUserUnlocked(int userId) {
        Log.i(TAG, "User unlocked, initializing the service");

        if (!SystemProperties.getBoolean(AVRCP_ENABLE_PROPERTY, true)) {
            Log.w(TAG, "Skipping initialization of the new AVRCP Target Player List");
            sInstance = null;
            return;
        }

        if (mMediaPlayerList != null) {
            mMediaPlayerList.init(new ListCallback());
        }
    }

    @Override
    protected boolean start() {
        if (sInstance != null) {
            Log.wtfStack(TAG, "The service has already been initialized");
            return false;
        }

        Log.i(TAG, "Starting the AVRCP Target Service");
        mCurrentData = new MediaData(null, null, null);

        mReceiver = new AvrcpBroadcastReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothA2dp.ACTION_ACTIVE_DEVICE_CHANGED);
        registerReceiver(mReceiver, filter);

        if (!SystemProperties.getBoolean(AVRCP_ENABLE_PROPERTY, true)) {
            Log.w(TAG, "Skipping initialization of the new AVRCP Target Service");
            sInstance = null;
            return true;
        }

        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);
        sDeviceMaxVolume = mAudioManager.getStreamMaxVolume(AudioManager.STREAM_MUSIC);

        mMediaPlayerList = new MediaPlayerList(Looper.myLooper(), this);

        UserManager userManager = UserManager.get(getApplicationContext());
        if (userManager.isUserUnlocked()) {
            mMediaPlayerList.init(new ListCallback());
        }

        mNativeInterface = AvrcpNativeInterface.getInterface();
        mNativeInterface.init(AvrcpTargetService.this);

        mVolumeManager = new AvrcpVolumeManager(this, mAudioManager, mNativeInterface);

        // Only allow the service to be used once it is initialized
        sInstance = this;

        return true;
    }

    @Override
    protected boolean stop() {
        Log.i(TAG, "Stopping the AVRCP Target Service");

        if (sInstance == null) {
            Log.w(TAG, "stop() called before start()");
            return true;
        }

        sInstance = null;
        unregisterReceiver(mReceiver);

        // We check the interfaces first since they only get set on User Unlocked
        if (mMediaPlayerList != null) mMediaPlayerList.cleanup();
        if (mNativeInterface != null) mNativeInterface.cleanup();

        mMediaPlayerList = null;
        mNativeInterface = null;
        mAudioManager = null;
        mReceiver = null;
        return true;
    }

    private void init() {
    }

    void deviceConnected(BluetoothDevice device, boolean absoluteVolume) {
        Log.i(TAG, "deviceConnected: device=" + device + " absoluteVolume=" + absoluteVolume);
        mVolumeManager.deviceConnected(device, absoluteVolume);
        MetricsLogger.logProfileConnectionEvent(BluetoothMetricsProto.ProfileId.AVRCP);
    }

    void deviceDisconnected(BluetoothDevice device) {
        Log.i(TAG, "deviceDisconnected: device=" + device);
        mVolumeManager.deviceDisconnected(device);
    }

    /**
     * Signal to the service that the current audio out device has changed and to inform
     * the audio service whether the new device supports absolute volume. If it does, also
     * set the absolute volume level on the remote device.
     */
    public void volumeDeviceSwitched(BluetoothDevice device) {
        if (DEBUG) {
            Log.d(TAG, "volumeDeviceSwitched: device=" + device);
        }
        mVolumeManager.volumeDeviceSwitched(device);
    }

    /**
     * Store the current system volume for a device in order to be retrieved later.
     */
    public void storeVolumeForDevice(BluetoothDevice device) {
        if (device == null) return;

        mVolumeManager.storeVolumeForDevice(device);
    }

    /**
     * Retrieve the remembered volume for a device. Returns -1 if there is no volume for the
     * device.
     */
    public int getRememberedVolumeForDevice(BluetoothDevice device) {
        if (device == null) return -1;

        return mVolumeManager.getVolume(device, -1);
    }

    // TODO (apanicke): Add checks to blacklist Absolute Volume devices if they behave poorly.
    void setVolume(int avrcpVolume) {
        int deviceVolume =
                (int) Math.floor((double) avrcpVolume * sDeviceMaxVolume / AVRCP_MAX_VOL);
        if (DEBUG) {
            Log.d(TAG, "SendVolumeChanged: avrcpVolume=" + avrcpVolume
                    + " deviceVolume=" + deviceVolume
                    + " sDeviceMaxVolume=" + sDeviceMaxVolume);
        }
        mAudioManager.setStreamVolume(AudioManager.STREAM_MUSIC, deviceVolume,
                AudioManager.FLAG_SHOW_UI | AudioManager.FLAG_BLUETOOTH_ABS_VOLUME);
    }

    /**
     * Set the volume on the remote device. Does nothing if the device doesn't support absolute
     * volume.
     */
    public void sendVolumeChanged(int deviceVolume) {
        int avrcpVolume =
                (int) Math.floor((double) deviceVolume * AVRCP_MAX_VOL / sDeviceMaxVolume);
        if (avrcpVolume > 127) avrcpVolume = 127;
        if (DEBUG) {
            Log.d(TAG, "SendVolumeChanged: avrcpVolume=" + avrcpVolume
                    + " deviceVolume=" + deviceVolume
                    + " sDeviceMaxVolume=" + sDeviceMaxVolume);
        }
        mNativeInterface.sendVolumeChanged(avrcpVolume);
    }

    Metadata getCurrentSongInfo() {
        return mMediaPlayerList.getCurrentSongInfo();
    }

    PlayStatus getPlayState() {
        return PlayStatus.fromPlaybackState(mMediaPlayerList.getCurrentPlayStatus(),
                Long.parseLong(getCurrentSongInfo().duration));
    }

    String getCurrentMediaId() {
        String id = mMediaPlayerList.getCurrentMediaId();
        if (id != null) return id;

        Metadata song = getCurrentSongInfo();
        if (song != null) return song.mediaId;

        // We always want to return something, the error string just makes debugging easier
        return "error";
    }

    List<Metadata> getNowPlayingList() {
        return mMediaPlayerList.getNowPlayingList();
    }

    int getCurrentPlayerId() {
        return mMediaPlayerList.getCurrentPlayerId();
    }

    // TODO (apanicke): Have the Player List also contain info about the play state of each player
    List<PlayerInfo> getMediaPlayerList() {
        return mMediaPlayerList.getMediaPlayerList();
    }

    void getPlayerRoot(int playerId, MediaPlayerList.GetPlayerRootCallback cb) {
        mMediaPlayerList.getPlayerRoot(playerId, cb);
    }

    void getFolderItems(int playerId, String mediaId, MediaPlayerList.GetFolderItemsCallback cb) {
        mMediaPlayerList.getFolderItems(playerId, mediaId, cb);
    }

    void playItem(int playerId, boolean nowPlaying, String mediaId) {
        // NOTE: playerId isn't used if nowPlaying is true, since its assumed to be the current
        // active player
        mMediaPlayerList.playItem(playerId, nowPlaying, mediaId);
    }

    // TODO (apanicke): Handle key events here in the service. Currently it was more convenient to
    // handle them there but logically they make more sense handled here.
    void sendMediaKeyEvent(int event, boolean pushed) {
        if (DEBUG) Log.d(TAG, "getMediaKeyEvent: event=" + event + " pushed=" + pushed);
        mMediaPlayerList.sendMediaKeyEvent(event, pushed);
    }

    void setActiveDevice(BluetoothDevice device) {
        Log.i(TAG, "setActiveDevice: device=" + device);
        if (device == null) {
            Log.wtfStack(TAG, "setActiveDevice: could not find device " + device);
        }
        A2dpService.getA2dpService().setActiveDevice(device);
    }

    /**
     * Dump debugging information to the string builder
     */
    public void dump(StringBuilder sb) {
        sb.append("\nProfile: AvrcpTargetService:\n");
        if (sInstance == null) {
            sb.append("AvrcpTargetService not running");
            return;
        }

        StringBuilder tempBuilder = new StringBuilder();
        if (mMediaPlayerList != null) {
            mMediaPlayerList.dump(tempBuilder);
        } else {
            tempBuilder.append("\nMedia Player List is empty\n");
        }

        mVolumeManager.dump(tempBuilder);

        // Tab everything over by two spaces
        sb.append(tempBuilder.toString().replaceAll("(?m)^", "  "));
    }

    private static class AvrcpTargetBinder extends IBluetoothAvrcpTarget.Stub
            implements IProfileServiceBinder {
        private AvrcpTargetService mService;

        AvrcpTargetBinder(AvrcpTargetService service) {
            mService = service;
        }

        @Override
        public void cleanup() {
            mService = null;
        }

        @Override
        public void sendVolumeChanged(int volume) {
            if (!Utils.checkCaller()) {
                Log.w(TAG, "sendVolumeChanged not allowed for non-active user");
                return;
            }

            if (mService == null) {
                return;
            }

            mService.sendVolumeChanged(volume);
        }
    }
}
