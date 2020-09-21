package com.android.car;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothProfile;

/**
 * Some potentially useful static methods.
 */
public class Utils {
    static final Boolean DBG = false;

    static String getDeviceDebugInfo(BluetoothDevice device) {
        return "(name = " + device.getName() + ", addr = " + device.getAddress() + ")";
    }

    static String getProfileName(int profile) {
        switch(profile) {
            case BluetoothProfile.A2DP_SINK:
                return "A2DP_SINK";
            case BluetoothProfile.HEADSET_CLIENT:
                return "HFP";
            case BluetoothProfile.PBAP_CLIENT:
                return "PBAP";
            case BluetoothProfile.MAP_CLIENT:
                return "MAP";
            case BluetoothProfile.AVRCP_CONTROLLER:
                return "AVRCP";
            case BluetoothProfile.PAN:
                return "PAN";
            default:
                return profile + "";

        }
    }

    /**
     * An utility class to dump transition events across different car service components.
     * The output will be of the form
     * <p>
     * "Time <svc name>: [optional context information] changed from <from state> to <to state>"
     * This can be used in conjunction with the dump() method to dump this information through
     * adb shell dumpsys activity service com.android.car
     * <p>
     * A specific service in CarService can choose to use a circular buffer of N records to keep
     * track of the last N transitions.
     *
     */
    public static class TransitionLog {
        private String mServiceName; // name of the service or tag
        private int mFromState; // old state
        private int mToState; // new state
        private long mTimestampMs; // System.currentTimeMillis()
        private String mExtra; // Additional information as a String

        public TransitionLog(String name, int fromState, int toState, long timestamp,
                String extra) {
            this(name, fromState, toState, timestamp);
            mExtra = extra;
        }

        public TransitionLog(String name, int fromState, int toState, long timeStamp) {
            mServiceName = name;
            mFromState = fromState;
            mToState = toState;
            mTimestampMs = timeStamp;
        }

        private CharSequence timeToLog(long timestamp) {
            return android.text.format.DateFormat.format("MM-dd HH:mm:ss", timestamp);
        }

        @Override
        public String toString() {
            return timeToLog(mTimestampMs) + " " + mServiceName + ": " + (mExtra != null ? mExtra
                    : "") + " changed from " + mFromState + " to " + mToState;
        }
    }
}
