/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import android.os.Parcel;
import android.util.Log;

public class EasManager {
    private static final String TAG = "EasManager";
    private static final long GPS_UTC_OFFSET_IN_SECONDS = 315964800;
    private static final int EAS_TEXT_MESSAGE           = 0;
    private static final int EAS_LOW_PRIORITY           = 4;
    private static final int EAS_MEDIUM_PRIORITY        = 8;
    private static final int EAS_HIGH_PRIORITY          = 12;

    private EasEvent preEasEvent = null;
    private EasEvent curEasEvent = null;

    public boolean isEasEventNeedProcess(EasEvent easEvent) {
        Log.i(TAG,"isEasEventNeedProcess");
        curEasEvent = easEvent;
        easEvent.printEasEventInfo();
        if (easEvent.protocolVersion != 0) {
            return false;
        }

        if (easEvent.alertPriority == EAS_TEXT_MESSAGE) {
            return false;
        }

        preEasEvent = easEvent;
        return true;
    }
 }

