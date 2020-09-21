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
package com.droidlogic.mediacenter;


import android.app.Application;

public class MediaCenterApplication extends Application {
        public static boolean mIsDaemonRun = false;
        public static boolean mIsPhoto = false;
        public static boolean mIsPlayer = false;

        @Override
        public void onCreate() {
            // TODO Auto-generated method stub
            super.onCreate();
        }

        @Override
        public void onTerminate() {
            // TODO Auto-generated method stub
            super.onTerminate();
        }


        public static void setPhoto(boolean isrun) {
            mIsPhoto = isrun;
        }

        public static boolean getPhoto() {
            return mIsPhoto;
        }
        public static void setPlayer(boolean isrun) {
            mIsPlayer = isrun;
        }

        public static boolean getPlayer() {
            return mIsPlayer;
        }

        public boolean isDaemonRun() {
            return mIsDaemonRun;
        }

}
