/*
 * Copyright (C) 2011 The Android Open Source Project
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
 *  @author   Luan.Yuan
 *  @version  2.0
 *  @date     2017/07/05
 *  @par function description:
 *  - This is HDR Manager, control HDR feature.
 */
package com.droidlogic.app;

import android.content.Context;
import android.provider.Settings;
import android.util.Log;

public class HdrManager {
    private static final String TAG                 = "HdrManager";

    private static final String KEY_HDR_MODE        = "persist.vendor.sys.hdr.state";

    public static final int MODE_OFF  = 0;
    public static final int MODE_ON   = 1;
    public static final int MODE_AUTO = 2;

    private Context mContext;
    private SystemControlManager mSystemControl;

    public HdrManager(Context context){
        mContext = context;
        mSystemControl = SystemControlManager.getInstance();
    }

    public int getHdrMode() {
        String value = mSystemControl.getPropertyString(KEY_HDR_MODE, "0");
        return Integer.parseInt(value);
    }

    public void setHdrMode(int mode) {
        if ((mode >=0) && (mode <= 2)) {
            mSystemControl.setHdrMode(String.valueOf(mode));
        } else {
            mSystemControl.setHdrMode(String.valueOf(MODE_AUTO));
        }
    }
}
