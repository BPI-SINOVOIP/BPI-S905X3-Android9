/*
*Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
*This source code is subject to the terms and conditions defined in the
*file 'LICENSE' which is part of this source code package.
*/

package com.droidlogic.pppoe;

import java.util.regex.Matcher;

import android.os.SystemProperties;
import android.net.NetworkInfo;
import android.util.Config;
import android.util.Slog;
import java.util.StringTokenizer;
import com.droidlogic.app.SystemControlManager;

/**
 * Listens for events for pppoe, and passes them on
 * to the {@link PppoeStateTracker} for handling. Runs in its own thread.
 *
 * @hide
 */
public class PppoeMonitor {
    private static final String TAG = "PppoeMonitor";
    private static final int CONNECTED  = 1;
    private static final int INTERFACE_DOWN = 2;
    private static final int INTERFACE_UP = 3;
    private static final int DEV_ADDED = 4;
    private static final int DEV_REMOVED = 5;
    private static final String connectedEvent = "CONNECTED";
    private static final String disconnectedEvent = "DISCONNECTED";

    private static final int NEW_LINK = 16;
    private static final int DEL_LINK = 17;
    private static final boolean DEBUG = false;
    private final String pppoe_running_flag = "net.pppoe.running";

    private PppoeStateTracker mTracker;
    private SystemControlManager mSystemControlManager;

    public PppoeMonitor(PppoeStateTracker tracker) {
        mTracker = tracker;
    }

    public void startMonitoring() {
        if (DEBUG) Slog.i(TAG, "Start startMonitoring");
        new MonitorThread().start();
    }

    class MonitorThread extends Thread {

        public MonitorThread() {
            super("PppoeMonitor");
        }

        public void run() {
            int index;
            int i;
            mSystemControlManager = new SystemControlManager(null);

            if (DEBUG) Slog.i(TAG, "Start run");
            for (;;) {
                String eventName = PppoeNative.waitForEvent();

                String propVal = mSystemControlManager.getProperty(pppoe_running_flag);
                if (DEBUG) Slog.i(TAG, "Start run for "+"eventName"+eventName+"propVal"+propVal);
                int n = 0;
                if (propVal.length() != 0) {
                    try {
                        n = Integer.parseInt(propVal);
                    } catch (NumberFormatException e) {}
                }

                if ( 0 == n) {
                    continue;
                }

                if (eventName == null) {
                    continue;
                }
                if (DEBUG) Slog.i(TAG, "EVENT[" + eventName+"]");
                /*
                 * Map event name into event enum
                 */
                String [] events = eventName.split(":");
                index = events.length;
                if (index < 2)
                    continue;
                i = 0;
                while (index != 0 && i < index-1) {
                    int event = 0;
                    if ("added".equals(events[i+1]) ) {
                        event = DEV_ADDED;
                    }
                    else if ("removed".equals(events[i+1])) {
                        event = DEV_REMOVED;
                    }
                    else {
                        int cmd =Integer.parseInt(events[i+1]);
                        if ( cmd == DEL_LINK) {
                            event = INTERFACE_DOWN;
                            handleEvent(events[i],event);
                        }
                        else if (cmd == NEW_LINK) {
                            event = INTERFACE_UP;
                            handleEvent(events[i],event);
                        }
                    }
                    i = i + 2;
                }
            }
        }

        void handleEvent(String ifname,int event) {
            switch (event) {
                case INTERFACE_DOWN:
                    mTracker.notifyStateChange(ifname,NetworkInfo.DetailedState.DISCONNECTED);
                    break;
                case INTERFACE_UP:
                    mTracker.notifyStateChange(ifname,NetworkInfo.DetailedState.CONNECTED);
                    break;
                default:
                    mTracker.notifyStateChange(ifname,NetworkInfo.DetailedState.FAILED);
            }
        }

    }
}
