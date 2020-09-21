/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.example.partnersupportsampletvinput;

import android.content.Context;
import android.media.tv.TvInputService;
import android.net.Uri;
import android.view.Surface;

/** SampleTvInputService */
public class SampleTvInputService extends TvInputService {
    public static final String INPUT_ID =
            "com.example.partnersupportsampletvinput/.SampleTvInputService";

    public BaseTvInputSessionImpl onCreateSession(String s) {
        return null;
    }

    class BaseTvInputSessionImpl extends Session {

        public BaseTvInputSessionImpl(Context context) {
            super(context);
        }

        public void onRelease() {}

        public boolean onSetSurface(Surface surface) {
            return false;
        }

        public void onSetStreamVolume(float v) {}

        public boolean onTune(Uri uri) {
            return false;
        }

        public void onSetCaptionEnabled(boolean b) {}
    }
}
