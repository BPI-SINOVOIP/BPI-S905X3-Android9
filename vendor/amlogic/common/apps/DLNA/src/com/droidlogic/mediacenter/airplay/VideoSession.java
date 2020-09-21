/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter.airplay;

import android.net.Uri;

public class VideoSession {
        private static VideoSession sInstance = null;
        public String mCurrentSessionID = "";
        public int mCurrentPosition = 0;
        public int mCurrentDuration = 0;
        public int mStartPosition = 0;
        public Uri mUri = null;
        public boolean isPlaying = false;
        public boolean isActive = false;
        public static VideoSession getInstance() {
            if ( sInstance == null ) {
                sInstance = new VideoSession();
            }
            return sInstance;
        }

}
