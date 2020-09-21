/*
 * Copyright 2017 The Android Open Source Project
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

package com.android.bluetooth.hfp;

import android.bluetooth.BluetoothDevice;

import java.util.Objects;

/**
 * Callback events from native layer
 */
public class HeadsetStackEvent extends HeadsetMessageObject {
    public static final int EVENT_TYPE_NONE = 0;
    public static final int EVENT_TYPE_CONNECTION_STATE_CHANGED = 1;
    public static final int EVENT_TYPE_AUDIO_STATE_CHANGED = 2;
    public static final int EVENT_TYPE_VR_STATE_CHANGED = 3;
    public static final int EVENT_TYPE_ANSWER_CALL = 4;
    public static final int EVENT_TYPE_HANGUP_CALL = 5;
    public static final int EVENT_TYPE_VOLUME_CHANGED = 6;
    public static final int EVENT_TYPE_DIAL_CALL = 7;
    public static final int EVENT_TYPE_SEND_DTMF = 8;
    public static final int EVENT_TYPE_NOICE_REDUCTION = 9;
    public static final int EVENT_TYPE_AT_CHLD = 10;
    public static final int EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST = 11;
    public static final int EVENT_TYPE_AT_CIND = 12;
    public static final int EVENT_TYPE_AT_COPS = 13;
    public static final int EVENT_TYPE_AT_CLCC = 14;
    public static final int EVENT_TYPE_UNKNOWN_AT = 15;
    public static final int EVENT_TYPE_KEY_PRESSED = 16;
    public static final int EVENT_TYPE_WBS = 17;
    public static final int EVENT_TYPE_BIND = 18;
    public static final int EVENT_TYPE_BIEV = 19;
    public static final int EVENT_TYPE_BIA = 20;

    public final int type;
    public final int valueInt;
    public final int valueInt2;
    public final String valueString;
    public final HeadsetMessageObject valueObject;
    public final BluetoothDevice device;

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, BluetoothDevice device) {
        this(type, 0, 0, null, null, device);
    }

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param valueInt an integer value in the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, int valueInt, BluetoothDevice device) {
        this(type, valueInt, 0, null, null, device);
    }

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param valueInt an integer value in the event
     * @param valueInt2 another integer value in the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, int valueInt, int valueInt2, BluetoothDevice device) {
        this(type, valueInt, valueInt2, null, null, device);
    }

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param valueString an string value in the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, String valueString, BluetoothDevice device) {
        this(type, 0, 0, valueString, null, device);
    }

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param valueObject an object value in the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, HeadsetMessageObject valueObject, BluetoothDevice device) {
        this(type, 0, 0, null, valueObject, device);
    }

    /**
     * Create a headset stack event
     *
     * @param type type of the event
     * @param valueInt an integer value in the event
     * @param valueInt2 another integer value in the event
     * @param valueString a string value in the event
     * @param valueObject an object value in the event
     * @param device device of interest
     */
    public HeadsetStackEvent(int type, int valueInt, int valueInt2, String valueString,
            HeadsetMessageObject valueObject, BluetoothDevice device) {
        this.type = type;
        this.valueInt = valueInt;
        this.valueInt2 = valueInt2;
        this.valueString = valueString;
        this.valueObject = valueObject;
        this.device = Objects.requireNonNull(device);
    }

    /**
     * Get a string that represents this event
     *
     * @return String that represents this event
     */
    public String getTypeString() {
        switch (type) {
            case EVENT_TYPE_NONE:
                return "EVENT_TYPE_NONE";
            case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                return "EVENT_TYPE_CONNECTION_STATE_CHANGED";
            case EVENT_TYPE_AUDIO_STATE_CHANGED:
                return "EVENT_TYPE_AUDIO_STATE_CHANGED";
            case EVENT_TYPE_VR_STATE_CHANGED:
                return "EVENT_TYPE_VR_STATE_CHANGED";
            case EVENT_TYPE_ANSWER_CALL:
                return "EVENT_TYPE_ANSWER_CALL";
            case EVENT_TYPE_HANGUP_CALL:
                return "EVENT_TYPE_HANGUP_CALL";
            case EVENT_TYPE_VOLUME_CHANGED:
                return "EVENT_TYPE_VOLUME_CHANGED";
            case EVENT_TYPE_DIAL_CALL:
                return "EVENT_TYPE_DIAL_CALL";
            case EVENT_TYPE_SEND_DTMF:
                return "EVENT_TYPE_SEND_DTMF";
            case EVENT_TYPE_NOICE_REDUCTION:
                return "EVENT_TYPE_NOICE_REDUCTION";
            case EVENT_TYPE_AT_CHLD:
                return "EVENT_TYPE_AT_CHLD";
            case EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST:
                return "EVENT_TYPE_SUBSCRIBER_NUMBER_REQUEST";
            case EVENT_TYPE_AT_CIND:
                return "EVENT_TYPE_AT_CIND";
            case EVENT_TYPE_AT_COPS:
                return "EVENT_TYPE_AT_COPS";
            case EVENT_TYPE_AT_CLCC:
                return "EVENT_TYPE_AT_CLCC";
            case EVENT_TYPE_UNKNOWN_AT:
                return "EVENT_TYPE_UNKNOWN_AT";
            case EVENT_TYPE_KEY_PRESSED:
                return "EVENT_TYPE_KEY_PRESSED";
            case EVENT_TYPE_WBS:
                return "EVENT_TYPE_WBS";
            case EVENT_TYPE_BIND:
                return "EVENT_TYPE_BIND";
            case EVENT_TYPE_BIEV:
                return "EVENT_TYPE_BIEV";
            case EVENT_TYPE_BIA:
                return "EVENT_TYPE_BIA";
            default:
                return "UNKNOWN";
        }
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(getTypeString())
                .append("[")
                .append(type)
                .append("]")
                .append(", valInt=")
                .append(valueInt)
                .append(", valInt2=")
                .append(valueInt2)
                .append(", valString=")
                .append(valueString)
                .append(", valObject=")
                .append(valueObject)
                .append(", device=")
                .append(device);
    }
}
