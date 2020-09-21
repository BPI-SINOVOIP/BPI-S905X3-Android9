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

package com.android.bluetooth.a2dp;

import android.bluetooth.BluetoothCodecStatus;
import android.bluetooth.BluetoothDevice;

/**
 * Stack event sent via a callback from JNI to Java, or generated
 * internally by the A2DP State Machine.
 */
public class A2dpStackEvent {
    // Event types for STACK_EVENT message (coming from native)
    private static final int EVENT_TYPE_NONE = 0;
    public static final int EVENT_TYPE_CONNECTION_STATE_CHANGED = 1;
    public static final int EVENT_TYPE_AUDIO_STATE_CHANGED = 2;
    public static final int EVENT_TYPE_CODEC_CONFIG_CHANGED = 3;

    // Do not modify without updating the HAL bt_av.h files.
    // Match up with btav_connection_state_t enum of bt_av.h
    static final int CONNECTION_STATE_DISCONNECTED = 0;
    static final int CONNECTION_STATE_CONNECTING = 1;
    static final int CONNECTION_STATE_CONNECTED = 2;
    static final int CONNECTION_STATE_DISCONNECTING = 3;
    // Match up with btav_audio_state_t enum of bt_av.h
    static final int AUDIO_STATE_REMOTE_SUSPEND = 0;
    static final int AUDIO_STATE_STOPPED = 1;
    static final int AUDIO_STATE_STARTED = 2;

    public int type = EVENT_TYPE_NONE;
    public BluetoothDevice device;
    public int valueInt = 0;
    public BluetoothCodecStatus codecStatus;

    A2dpStackEvent(int type) {
        this.type = type;
    }

    @Override
    public String toString() {
        // event dump
        StringBuilder result = new StringBuilder();
        result.append("A2dpStackEvent {type:" + eventTypeToString(type));
        result.append(", device:" + device);
        result.append(", value1:" + eventTypeValueIntToString(type, valueInt));
        if (codecStatus != null) {
            result.append(", codecStatus:" + codecStatus);
        }
        result.append("}");
        return result.toString();
    }

    private static String eventTypeToString(int type) {
        switch (type) {
            case EVENT_TYPE_NONE:
                return "EVENT_TYPE_NONE";
            case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                return "EVENT_TYPE_CONNECTION_STATE_CHANGED";
            case EVENT_TYPE_AUDIO_STATE_CHANGED:
                return "EVENT_TYPE_AUDIO_STATE_CHANGED";
            case EVENT_TYPE_CODEC_CONFIG_CHANGED:
                return "EVENT_TYPE_CODEC_CONFIG_CHANGED";
            default:
                return "EVENT_TYPE_UNKNOWN:" + type;
        }
    }

    private static String eventTypeValueIntToString(int type, int value) {
        switch (type) {
            case EVENT_TYPE_CONNECTION_STATE_CHANGED:
                switch (value) {
                    case CONNECTION_STATE_DISCONNECTED:
                        return "DISCONNECTED";
                    case CONNECTION_STATE_CONNECTING:
                        return "CONNECTING";
                    case CONNECTION_STATE_CONNECTED:
                        return "CONNECTED";
                    case CONNECTION_STATE_DISCONNECTING:
                        return "DISCONNECTING";
                    default:
                        break;
                }
                break;
            case EVENT_TYPE_AUDIO_STATE_CHANGED:
                switch (value) {
                    case AUDIO_STATE_REMOTE_SUSPEND:
                        return "REMOTE_SUSPEND";
                    case AUDIO_STATE_STOPPED:
                        return "STOPPED";
                    case AUDIO_STATE_STARTED:
                        return "STARTED";
                    default:
                        break;
                }
                break;
            default:
                break;
        }
        return Integer.toString(value);
    }
}
