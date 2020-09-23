/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.server.hdmi;

import android.content.Context;
import android.content.ContentResolver;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.hardware.tv.cec.V1_0.SendMessageResult;
import android.provider.Settings;

import java.util.Arrays;

/**
 * This action is to send 'Request Short Audio Descriptor' HDMI-CEC command to AVR when connects.
 * And call setParameters to Audio HAL to update audio format support list.
 * For now this action only query AC-3, DTS, DD+, DTSHD formats.
 *
 * Reference:
 * HDMI Specifications (Ver 1.4):
 *     CEC 13.15.3 Discovering the Amplifier’s Audio Format
 *     CEC Table 23 (Page CEC-92)
 *     CEC Table 29 (Page CEC-106 and CEC-107)
 * CEA-861-D:
 *     Table 35, 36, 37 (Page 91)
 */
final class RequestShortAudioDescriptorAction extends HdmiCecFeatureAction {
    private static final String TAG = "RequestShortAudioDescriptor";

    // State that waits for <ReportShortAudioDescriptor>
    private static final int STATE_WAITNG_FOR_REQUEST_SHORT_AUDIO_DESCRIPTOR = 1;

    /**
     * Used to Store Short Audio Descriptor after ARC established.
     */
    public static final String SETTINGS_AUDIO_DESCRIPTOR = "settings_audio_descriptor";

    // Logic address of AV Receiver
    private final int mAvrAddress;
    private final boolean mEnabled;
    private HdmiControlService mService;
    private final int mAvrPort;

    private static byte[] sDescriptor = null;

    /**
     *  Audio Format Description of CEC Short Audio Descriptor
     *  CEA-861-D: Table 37
     */
    private static final int SAD_CODEC_RESERVED1   = 0x0;
    private static final int SAD_CODEC_LPCM        = 0x1;
    private static final int SAD_CODEC_AC3         = 0x2;
    private static final int SAD_CODEC_MPEG1       = 0x3;
    private static final int SAD_CODEC_MP3         = 0x4;
    private static final int SAD_CODEC_MPEG2MC     = 0x5;
    private static final int SAD_CODEC_AAC         = 0x6;
    private static final int SAD_CODEC_DTS         = 0x7;
    private static final int SAD_CODEC_ATRAC       = 0x8;
    private static final int SAD_CODEC_OBA         = 0x9;
    private static final int SAD_CODEC_DDP         = 0xA;
    private static final int SAD_CODEC_DTSHD       = 0xB;
    private static final int SAD_CODEC_MAT         = 0xC;
    private static final int SAD_CODEC_DST         = 0xD;
    private static final int SAD_CODEC_WMAPRO      = 0xE;
    private static final int SAD_CODEC_RESERVED2   = 0xF;

    private final int SINGAL_SAD_LEN = 3;
    private final int MAX_SAD_NUM = 4;

    private final int SAD_REQ_TIMEOUT_MS = 10000;

    private final int[] SUPPORT_CODECS = {SAD_CODEC_AC3, SAD_CODEC_DTS, SAD_CODEC_DDP, SAD_CODEC_DTSHD};

    private final byte SUPPORT = 1;
    private final byte UNSUPPORT = 0;

    private final String SET_ARC_HDMI = "set_ARC_hdmi=";
    private final String SET_ARC_FORMAT = "set_ARC_format=";

    /**
     * @param source {@link HdmiCecLocalDevice} instance
     * @param avrAddress address of AV receiver. It should be AUDIO_SYSTEM type
     * @param enabled whether to reset AVR audio codecs
     * @param service {@link HdmiControlService} instance
     * @param avrPort AVR port ID
     */
    RequestShortAudioDescriptorAction(HdmiCecLocalDevice source,
                                      int avrAddress,
                                      boolean enabled,
                                      HdmiControlService service,
                                      int avrPort) {
        super(source);
        HdmiUtils.verifyAddressType(getSourceAddress(), HdmiDeviceInfo.DEVICE_TV);
        HdmiUtils.verifyAddressType(avrAddress, HdmiDeviceInfo.DEVICE_AUDIO_SYSTEM);
        mAvrAddress = avrAddress;
        mEnabled = enabled;
        mService = service;
        mAvrPort = avrPort;
    }

    @Override
    boolean start() {
        if (mEnabled) {
            if (sDescriptor != null) {
                setAudioFormat();
            } else {
                mState = STATE_WAITNG_FOR_REQUEST_SHORT_AUDIO_DESCRIPTOR;
                addTimer(mState, SAD_REQ_TIMEOUT_MS);
                sendRequestShortAudioDescriptor();
            }
        } else {
            sDescriptor = null;
            setAudioFormat();
            finish();
        }
        return true;
    }

    @Override
    boolean processCommand(HdmiCecMessage hdmiCecMessage) {
        if (mState != STATE_WAITNG_FOR_REQUEST_SHORT_AUDIO_DESCRIPTOR) {
            return false;
        }

        int opcode = hdmiCecMessage.getOpcode();
        byte[] params = hdmiCecMessage.getParams();
        switch (opcode) {
            case Constants.MESSAGE_FEATURE_ABORT:
                // If avr returns <Feature Abort> ["Invalid Operand"], it indicates the Amplifier
                // does not support the requested audio format, the TV should select default 2
                // channels LPCM for output. (CEC Figure 34. Typical Operation to discover the
                // Audio Format capability of an Amplifier)
                if (params[0] == (byte) (Constants.MESSAGE_REQUEST_SHORT_AUDIO_DESCRIPTOR & 0xFF)
                        && params[1] == (byte) (Constants.ABORT_INVALID_OPERAND & 0xFF)) {
                    sDescriptor = null;
                    setAudioFormat();
                    finish();
                    return true;
                } else {
                    return false;
                }
            case Constants.MESSAGE_REPORT_SHORT_AUDIO_DESCRIPTOR:
                // If avr returns REPORT_SHORT_AUDIO_DESCRIPTOR, it should have at least one short
                // audio descriptor. If it returns invalid value, TV should ignore the message.
                if (params.length == 0) {
                    sDescriptor = null;
                } else {
                    sDescriptor = new byte[params.length];
                    System.arraycopy(params, 0, sDescriptor, 0, params.length);
                }
                setAudioFormat();
                finish();
                return true;
            default:
                return false;
        }
    }

    @Override
    void handleTimerEvent(int state) {
        if (mState != state || state != STATE_WAITNG_FOR_REQUEST_SHORT_AUDIO_DESCRIPTOR) {
            return;
        }
        HdmiLogger.debug("[T] RequestShortAudioDescriptorAction.");
        finish();
    }

    /**
     * Clean Cached Descriptor
     */
    public static void removeAudioFormat() {
        sDescriptor = null;
        HdmiLogger.debug("Audio Format Cleaned");
    }

    /**
     * This function will send set_ARC_format multiple times (according to the size of
     * SUPPORT_CODECS) and send set_ARC_hdmi one time.
     */
    private void setAudioFormat() {
        String settingsBuffer = "";
        byte edidLength = (byte) (sDescriptor == null ? 0 : sDescriptor.length);
        byte[] edidBuffer = new byte[edidLength + 2];

        edidBuffer[0] = edidLength;
        edidBuffer[1] = (byte) mAvrPort;

        if (sDescriptor != null) {
            System.arraycopy(sDescriptor, 0, edidBuffer, 2, edidLength);
        }
        String edidParameter = SET_ARC_HDMI + Arrays.toString(edidBuffer);
        setParameters(edidParameter);
        settingsBuffer += edidParameter;

        // for each codec, send a set_ARC_format parameters to audio HAL
        for (int codec : SUPPORT_CODECS) {
            // initial array : format, support, channel, sample rate, bit rate
            byte[] formatBuffer = new byte[] { (byte) codec, UNSUPPORT, 0, 0, 0 };

            // If use this Action to query LPCM format information, it should at least
            // give a default value for LPCM.
            if (codec == SAD_CODEC_LPCM && edidLength == 0) {
                formatBuffer[1] = SUPPORT;
                formatBuffer[2] = 0x1; // 2ch
                formatBuffer[3] = 0x6; // 48kHz, 44.1kHz
                formatBuffer[4] = 0x1; // 16bit
            }

            // find a descriptor for each SUPPORT_CODECS
            // CEA-861-D Table 34, 35, 36
            for (int i = 0; i + 2 < edidLength; i += SINGAL_SAD_LEN) {
                if ((byte) codec == (byte) (sDescriptor[i] & 0x78) >> 3) {
                    formatBuffer[0] = (byte) codec;
                    formatBuffer[1] = SUPPORT;
                    formatBuffer[2] = (byte) (sDescriptor[i] & 0x7); // Max Channels - 1
                    formatBuffer[3] = (byte) (sDescriptor[i + 1] & 0x7F); // Support Sample Rate
                    formatBuffer[4] = (byte) (sDescriptor[i + 2] & 0xFF); // Max bit rate / 8kHz
                    break;
                }
            }

            String formatParameter = SET_ARC_FORMAT + Arrays.toString(formatBuffer);
            setParameters(formatParameter);
            settingsBuffer += "|" + formatParameter;
        }

        // Save the newest Audio Parameters to settings.
        // Note: setprop can save only 91 words so we need to use settings.
        ContentResolver cr = mService.getContext().getContentResolver();
        Settings.System.putString(cr, SETTINGS_AUDIO_DESCRIPTOR, settingsBuffer);
    }

    /**
     * Set k-v parameters to audio HAL
     */
    private void setParameters(String parameters) {
        if (mService != null) {
            mService.getAudioManager().setParameters(parameters);
            HdmiLogger.debug("Set Audio Format [%s]", parameters);
        }
    }

    private void sendRequestShortAudioDescriptor() {
        int formatLength = Math.min(SUPPORT_CODECS.length, MAX_SAD_NUM);
        byte[] formats = new byte[formatLength]; //Audio Format ID & Code, max length is 4
        for (int i = 0; i < formatLength; i++) {
            formats[i] = (byte) (SUPPORT_CODECS[i] & 0x0F);
        }

        // CEC Message Descriptions
        HdmiCecMessage command = new HdmiCecMessage(getSourceAddress(),
                mAvrAddress, Constants.MESSAGE_REQUEST_SHORT_AUDIO_DESCRIPTOR, formats);
        HdmiLogger.debug("Sending Message + " + command.toString());
        sendCommand(command, new HdmiControlService.SendMessageCallback() {
            @Override
            public void onSendCompleted(int error) {
                switch (error) {
                    case SendMessageResult.BUSY:
                    case SendMessageResult.FAIL:
                    case SendMessageResult.SUCCESS:
                        // The result of the command transmission, unless it is an obvious
                        // failure indicated by the target device (or lack thereof), should
                        // not affect the ARC status. Ignores it silently.
                        break;
                    case SendMessageResult.NACK:
                        finish();
                        break;
                }
            }
        });
    }
}
