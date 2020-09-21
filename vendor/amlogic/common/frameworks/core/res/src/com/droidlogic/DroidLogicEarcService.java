/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC
 */

package com.droidlogic;

import android.annotation.Nullable;
import android.app.Service;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.media.AudioDeviceInfo;
import android.media.AudioManager;
import android.media.AudioSystem;
import android.os.IBinder;
import android.os.SystemProperties;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

import org.xmlpull.v1.XmlPullParserException;

import com.android.internal.util.HexDump;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.HdmiUtils.Constants.AudioCodec;
import com.droidlogic.HdmiUtils.CodecSad;
import com.droidlogic.HdmiUtils.DeviceConfig;

public class DroidLogicEarcService extends Service {
    private static final String TAG = "DroidLogicEarcService";
    private static final String ARC_RX_ENABLE_STATE = "EARCRX-ARC=1EARCRX-EARC=0";
    private static final String ARC_TX_ENABLE_STATE = "EARCTX-ARC=1EARCTX-EARC=0";
    private static final String EARC_RX_ENABLE_STATE = "EARCRX-ARC=0EARCRX-EARC=1";
    private static final String EARC_TX_ENABLE_STATE = "EARCTX-ARC=0EARCTX-EARC=1";
    private static final String EARC_RX_CDS = "eARC_RX CDS";
    private static final String EARC_TX_CDS = "eARC_TX CDS";
    private static final String EVENT_EARCRX = "earcrx";
    private static final String EVENT_EARCTX = "earctx";
    private static final String HDMI_ARC_SWITCH = "HDMI ARC Switch=";
    private static final String KEY_STATE = "STATE";
    private static final String PROPERTY_DEVICE_TYPE = "ro.hdmi.device_type";
    private static final String SETTINGS_AUDIO_DESCRIPTOR = "settings_audio_descriptor";
    private static final String SET_ARC_FORMAT = "set_ARC_format=";
    private static final String SHORT_AUDIO_DESCRIPTOR_CONFIG_PATH = "/vendor/etc/sadConfig.xml";
    private static final String SPEAKER_MUTE = "speaker_mute=";
    private static final String SYS_NODE_EARCRX = "/sys/class/extcon/earcrx/state";
    private static final String SYS_NODE_EARCTX = "/sys/class/extcon/earctx/state";
    private final int SINGAL_SAD_LEN = 3;
    @AudioCodec
    private final int[] SUPPORT_CODECS = {
            HdmiUtils.Constants.AUDIO_CODEC_DD,
            HdmiUtils.Constants.AUDIO_CODEC_DTS,
            HdmiUtils.Constants.AUDIO_CODEC_DDP,
            HdmiUtils.Constants.AUDIO_CODEC_DTSHD};
    @AudioCodec
    private final int[] AUDIO_FORMAT_CODECS = {
            HdmiUtils.Constants.AUDIO_CODEC_NONE,
            HdmiUtils.Constants.AUDIO_CODEC_LPCM,
            HdmiUtils.Constants.AUDIO_CODEC_DD,
            HdmiUtils.Constants.AUDIO_CODEC_MPEG1,
            HdmiUtils.Constants.AUDIO_CODEC_MP3,
            HdmiUtils.Constants.AUDIO_CODEC_MPEG2,
            HdmiUtils.Constants.AUDIO_CODEC_AAC,
            HdmiUtils.Constants.AUDIO_CODEC_DTS,
            HdmiUtils.Constants.AUDIO_CODEC_ATRAC,
            HdmiUtils.Constants.AUDIO_CODEC_ONEBITAUDIO,
            HdmiUtils.Constants.AUDIO_CODEC_DDP,
            HdmiUtils.Constants.AUDIO_CODEC_DTSHD,
            HdmiUtils.Constants.AUDIO_CODEC_TRUEHD,
            HdmiUtils.Constants.AUDIO_CODEC_DST,
            HdmiUtils.Constants.AUDIO_CODEC_WMAPRO,
            HdmiUtils.Constants.AUDIO_CODEC_MAX};
    private final byte SUPPORT = 1;
    private final byte UNSUPPORT = 0;
    private static final int EARC_CONNECT_TYPE = 2;
    private static final int ARC_CONNECT_TYPE = 1;
    private static final int EARC_ARC_UNCONNECT_TYPE = 0;

    private List<Integer> mLocalDevices;
    private SystemControlManager mSystemControlManager;
    private AudioManager mAudioManager;
    private int mCurrentConnectType;
    private byte[] mDescriptor;
    private String mCurrentRXCDS;

    private UEventObserver mEarcObserver = new UEventObserver() {
        @Override
        public void onUEvent(UEvent event) {
            String state = event.get(KEY_STATE);
            int connectType = getCurrentConnectType(state.replaceAll("\n", ""));
            if ((mCurrentConnectType == EARC_CONNECT_TYPE)
                    && (connectType != EARC_CONNECT_TYPE)
                    && isTvDevice()) {
                setWiredDeviceConnectionState(AudioSystem.DEVICE_OUT_HDMI_ARC,
                        0, "", "");
            } else if ((mCurrentConnectType != EARC_CONNECT_TYPE)
                    && (connectType == EARC_CONNECT_TYPE)) {
                if (isTvDevice()) {
                    setWiredDeviceConnectionState(AudioSystem.DEVICE_OUT_HDMI_ARC,
                            1, "", "");
                } else if (isAudioSystemDevice()) {
                    if (TextUtils.isEmpty(mCurrentRXCDS)) {
                        mCurrentRXCDS = getSupportedShortAudioDescriptorsFromConfig();
                    }
                    if (!TextUtils.isEmpty(mCurrentRXCDS)) {
                        setShortAudioDescriptors();
                    }
                }
            }
            mCurrentConnectType = connectType;
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mSystemControlManager = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) this.getSystemService (Context.AUDIO_SERVICE);
        mLocalDevices = getIntList(SystemProperties.get(PROPERTY_DEVICE_TYPE));
        if (isTvDevice()) {
            mCurrentConnectType = getCurrentConnectType(
                    mSystemControlManager.readSysFs(SYS_NODE_EARCTX));
            if (mCurrentConnectType == EARC_CONNECT_TYPE) {
                setWiredDeviceConnectionState(AudioSystem.DEVICE_OUT_HDMI_ARC,
                    1, "", "");
            }
            mEarcObserver.startObserving(EVENT_EARCTX);
        } else if (isAudioSystemDevice()) {
            mCurrentConnectType = getCurrentConnectType(
                    mSystemControlManager.readSysFs(SYS_NODE_EARCRX));
            mCurrentRXCDS = getSupportedShortAudioDescriptorsFromConfig();
            if ((mCurrentConnectType == EARC_CONNECT_TYPE) &&
                    !TextUtils.isEmpty(mCurrentRXCDS)) {
                setShortAudioDescriptors();
            }
            mEarcObserver.startObserving(EVENT_EARCRX);
        }
    }

    private static List<Integer> getIntList(String string) {
        ArrayList<Integer> list = new ArrayList<>();
        TextUtils.SimpleStringSplitter splitter = new TextUtils.SimpleStringSplitter(',');
        splitter.setString(string);
        for (String item : splitter) {
            try {
                list.add(Integer.parseInt(item));
            } catch (NumberFormatException e) {
                Log.w(TAG, "Can't parseInt: " + item);
            }
        }
        return Collections.unmodifiableList(list);
    }

    private boolean isTvDevice() {
        return mLocalDevices.contains(HdmiDeviceInfo.DEVICE_TV);
    }

    private boolean isAudioSystemDevice() {
        return mLocalDevices.contains(HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM);
    }

    private int getCurrentConnectType(String currentConnectState) {
        if ((EARC_RX_ENABLE_STATE.equals(currentConnectState) && isAudioSystemDevice())
                || (EARC_TX_ENABLE_STATE.equals(currentConnectState) && isTvDevice())) {
            return EARC_CONNECT_TYPE;
        } else if ((ARC_TX_ENABLE_STATE.equals(currentConnectState) && isTvDevice())
                || (ARC_RX_ENABLE_STATE.equals(currentConnectState) && isAudioSystemDevice())) {
            return ARC_CONNECT_TYPE;
        } else {
            return EARC_ARC_UNCONNECT_TYPE;
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mEarcObserver.stopObserving();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void setWiredDeviceConnectionState(int type, int state, String address, String name) {
        mAudioManager.setWiredDeviceConnectionState(type, state, address, name);
        updateTvAudioStatus(state);
    }

    private void updateTvAudioStatus(int state) {
        if (state == 0) {
            mDescriptor = null;
        } else {
            mDescriptor = HexDump.hexStringToByteArray(getShortAudioDescriptors(EARC_TX_CDS));
        }
        setAudioFormat();
        setParameters(HDMI_ARC_SWITCH + state);
        setParameters(SPEAKER_MUTE + state);
    }

    private void setShortAudioDescriptors() {
        setParameters(EARC_RX_CDS + "=" + addBlankInMiddle(mCurrentRXCDS).trim());
    }

    private String addBlankInMiddle(String str) {
        int strLenth = str.length();
        int blankCount = 0;
        if (strLenth <= 2) {
            blankCount = 0;
        } else {
            blankCount = strLenth % 2 > 0 ? strLenth / 2 : strLenth / 2 - 1;
        }
        if (blankCount > 0) {
            for (int i = 0; i < blankCount; i++) {
                str = str.substring(0, (i + 1) * 2 + i) + " " + str.substring((i + 1) * 2 + i, strLenth + i);
            }
        }
        return str;
    }

    private String getSupportedShortAudioDescriptorsFromConfig() {
        List<DeviceConfig> config = null;
        File file = new File(SHORT_AUDIO_DESCRIPTOR_CONFIG_PATH);
        if (file.exists()) {
            try {
                InputStream in = new FileInputStream(file);
                config = HdmiUtils.ShortAudioDescriptorXmlParser.parse(in);
                in.close();
            } catch (IOException e) {
                Log.e(TAG, "Error reading file: " + file, e);
            } catch (XmlPullParserException e) {
                Log.e(TAG, "Unable to parse file: " + file, e);
            }
        }
        byte[] sadBytes;
        if (config != null && config.size() > 0) {
            sadBytes = getSupportedShortAudioDescriptorsFromConfig(config, AUDIO_FORMAT_CODECS);
        } else {
            AudioDeviceInfo deviceInfo = getSystemAudioDeviceInfo();
            if (deviceInfo == null) {
                return null;
            }
            sadBytes = getSupportedShortAudioDescriptors(deviceInfo, AUDIO_FORMAT_CODECS);
        }
        if (sadBytes.length == 0) {
            return null;
        } else {
            return HexDump.toHexString(sadBytes);
        }
    }

    @Nullable
    private AudioDeviceInfo getSystemAudioDeviceInfo() {
        if (mAudioManager == null) {
            Log.e(TAG,
                    "Error getting system audio device because AudioManager not available.");
            return null;
        }
        AudioDeviceInfo[] devices = mAudioManager.getDevices(AudioManager.GET_DEVICES_INPUTS);
        Log.d(TAG, "Found " + devices.length + " audio input devices");
        for (AudioDeviceInfo device : devices) {
            // TODO(b/80297701) use the actual device type that system audio mode is connected to.
            if (device.getType() == AudioDeviceInfo.TYPE_HDMI_ARC) {
                return device;
            }
        }
        return null;
    }

    private byte[] getSupportedShortAudioDescriptors(
            AudioDeviceInfo deviceInfo, @AudioCodec int[] audioFormatCodes) {
        ArrayList<byte[]> sads = new ArrayList<>(audioFormatCodes.length);
        for (@AudioCodec int audioFormatCode : audioFormatCodes) {
            byte[] sad = getSupportedShortAudioDescriptor(deviceInfo, audioFormatCode);
            if (sad != null) {
                if (sad.length == 3) {
                    sads.add(sad);
                } else {
                    Log.w(TAG,
                            "Dropping Short Audio Descriptor with length " + sad.length + " for requested codec " + audioFormatCode);
                }
            }
        }
        return getShortAudioDescriptorBytes(sads);
    }

    private byte[] getSupportedShortAudioDescriptorsFromConfig(
            List<DeviceConfig> deviceConfig, @AudioCodec int[] audioFormatCodes) {
        DeviceConfig deviceConfigToUse = null;
        for (DeviceConfig device : deviceConfig) {
            // TODO(amyjojo) use PROPERTY_SYSTEM_AUDIO_MODE_AUDIO_PORT to get the audio device name
            if (device.name.equals("VX_AUDIO_DEVICE_IN_HDMI_ARC")) {
                deviceConfigToUse = device;
                break;
            }
        }
        if (deviceConfigToUse == null) {
            // TODO(amyjojo) use PROPERTY_SYSTEM_AUDIO_MODE_AUDIO_PORT to get the audio device name
            Log.w(TAG, "sadConfig.xml does not have required device info for "
                        + "VX_AUDIO_DEVICE_IN_HDMI_ARC");
            return new byte[0];
        }
        HashMap<Integer, byte[]> map = new HashMap<>();
        ArrayList<byte[]> sads = new ArrayList<>(audioFormatCodes.length);
        for (CodecSad codecSad : deviceConfigToUse.supportedCodecs) {
            map.put(codecSad.audioCodec, codecSad.sad);
        }
        for (int i = 0; i < audioFormatCodes.length; i++) {
            if (map.containsKey(audioFormatCodes[i])) {
                byte[] sad = map.get(audioFormatCodes[i]);
                if (sad != null && sad.length == 3) {
                    sads.add(sad);
                }
            }
        }
        return getShortAudioDescriptorBytes(sads);
    }

    private byte[] getShortAudioDescriptorBytes(ArrayList<byte[]> sads) {
        // Short Audio Descriptors are always 3 bytes long.
        byte[] bytes = new byte[sads.size() * 3];
        int index = 0;
        for (byte[] sad : sads) {
            System.arraycopy(sad, 0, bytes, index, 3);
            index += 3;
        }
        return bytes;
    }

    /**
     * Returns a 3 byte short audio descriptor as described in CEC 1.4 table 29 or null if the
     * audioFormatCode is not supported.
     */
    @Nullable
    private byte[] getSupportedShortAudioDescriptor(
            AudioDeviceInfo deviceInfo, @AudioCodec int audioFormatCode) {
        switch (audioFormatCode) {
            case HdmiUtils.Constants.AUDIO_CODEC_NONE: {
                return null;
            }
            case HdmiUtils.Constants.AUDIO_CODEC_LPCM: {
                return getLpcmShortAudioDescriptor(deviceInfo);
            }
            // TODO(b/80297701): implement the rest of the codecs
            case HdmiUtils.Constants.AUDIO_CODEC_DD:
            case HdmiUtils.Constants.AUDIO_CODEC_MPEG1:
            case HdmiUtils.Constants.AUDIO_CODEC_MP3:
            case HdmiUtils.Constants.AUDIO_CODEC_MPEG2:
            case HdmiUtils.Constants.AUDIO_CODEC_AAC:
            case HdmiUtils.Constants.AUDIO_CODEC_DTS:
            case HdmiUtils.Constants.AUDIO_CODEC_ATRAC:
            case HdmiUtils.Constants.AUDIO_CODEC_ONEBITAUDIO:
            case HdmiUtils.Constants.AUDIO_CODEC_DDP:
            case HdmiUtils.Constants.AUDIO_CODEC_DTSHD:
            case HdmiUtils.Constants.AUDIO_CODEC_TRUEHD:
            case HdmiUtils.Constants.AUDIO_CODEC_DST:
            case HdmiUtils.Constants.AUDIO_CODEC_WMAPRO:
            default: {
                return null;
            }
        }
    }

    @Nullable
    private byte[] getLpcmShortAudioDescriptor(AudioDeviceInfo deviceInfo) {
        // TODO(b/80297701): implement
        return null;
    }

    private String getShortAudioDescriptors(String cdsType) {
        String earcCDS = mAudioManager.getParameters(cdsType);
        if (!TextUtils.isEmpty(earcCDS) && earcCDS.endsWith("=")) {
            earcCDS = earcCDS.substring(0, earcCDS.length() - 1);
        }
        return earcCDS;
    }

    private void setParameters(String parameters) {
        mAudioManager.setParameters(parameters);
    }

    /**
     * This function will send set_ARC_format multiple times (according to the size of
     * SUPPORT_CODECS).
     */
    private void setAudioFormat() {
        String settingsBuffer = "";
        byte edidLength = (byte) (mDescriptor == null ? 0 : mDescriptor.length);

        // for each codec, send a set_ARC_format parameters to audio HAL
        for (int codec : SUPPORT_CODECS) {
            // initial array : format, support, channel, sample rate, bit rate
            byte[] formatBuffer = new byte[] { (byte) codec, UNSUPPORT, 0, 0, 0 };

            // If use this Action to query LPCM format information, it should at least
            // give a default value for LPCM.
            if (codec == HdmiUtils.Constants.AUDIO_CODEC_LPCM && edidLength == 0) {
                formatBuffer[1] = SUPPORT;
                formatBuffer[2] = 0x1; // 2ch
                formatBuffer[3] = 0x6; // 48kHz, 44.1kHz
                formatBuffer[4] = 0x1; // 16bit
            }

            // find a descriptor for each SUPPORT_CODECS
            // CEA-861-D Table 34, 35, 36
            for (int i = 0; i + 2 < edidLength; i += SINGAL_SAD_LEN) {
                if ((byte) codec == (byte) (mDescriptor[i] & 0x78) >> 3) {
                    formatBuffer[0] = (byte) codec;
                    formatBuffer[1] = SUPPORT;
                    formatBuffer[2] = (byte) (mDescriptor[i] & 0x7); // Max Channels - 1
                    formatBuffer[3] = (byte) (mDescriptor[i + 1] & 0x7F); // Support Sample Rate
                    formatBuffer[4] = (byte) (mDescriptor[i + 2] & 0xFF); // Max bit rate / 8kHz
                    break;
                }
            }

            String formatParameter = SET_ARC_FORMAT + Arrays.toString(formatBuffer);
            setParameters(formatParameter);
            settingsBuffer += "|" + formatParameter;
        }

        // Save the newest Audio Parameters to settings.
        // Note: setprop can save only 91 words so we need to use settings.
        Settings.System.putString(getContentResolver(),
                SETTINGS_AUDIO_DESCRIPTOR, settingsBuffer);
    }
}