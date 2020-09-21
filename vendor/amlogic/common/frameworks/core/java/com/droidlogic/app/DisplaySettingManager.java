/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.droidlogic.app;

import android.content.Context;
import android.util.Log;

public class DisplaySettingManager {
    private static final String TAG = "DisplaySettingManager";
    private static final boolean DEBUG = false;

    private Context mContext = null;

    public static final int REQUEST_DISPLAY_FORMAT_1080P=0;
    public static final int REQUEST_DISPLAY_FORMAT_1080I=1;
    public static final int REQUEST_DISPLAY_FORMAT_720P=2;
    public static final int REQUEST_DISPLAY_FORMAT_720I=3;
    public static final int REQUEST_DISPLAY_FORMAT_576P=4;
    public static final int REQUEST_DISPLAY_FORMAT_576I=5;
    public static final int REQUEST_DISPLAY_FORMAT_480P=6;
    public static final int REQUEST_DISPLAY_FORMAT_480I=7;

    public static final int STEREOSCOPIC_3D_FORMAT_OFF=0;
    public static final int REQUEST_3D_FORMAT_SIDE_BY_SIDE=8;
    public static final int REQUEST_3D_FORMAT_TOP_BOTTOM=16;

    static {
        System.loadLibrary("displaysetting");
    }

    private static native void nativeSetDisplaySize(int displayid,int format);
    private static native void nativeSetDisplay2Stereoscopic(int displayid,int format);

    public DisplaySettingManager(Context context){
        mContext = context;
    }

    public static void setDisplaySize(int format){
        if (DEBUG)
            Log.d(TAG, "------nativeSetDisplaySize format is " + format);
        nativeSetDisplaySize(0, format);
    }

    public static void setDisplay2Stereoscopic(int format){
        if (DEBUG)
            Log.d(TAG, "------nativeSetDisplay2Stereoscopic format is " + format);
        nativeSetDisplay2Stereoscopic(0, format);
    }
}
