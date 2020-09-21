/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleview;

import android.util.Log;
import com.subtitleparser.*;

public class SubManager {
        private static final String TAG = SubManager.class.getSimpleName();
        private SubtitleApi subapi = null;
        private Subtitle.SUBTYPE type = Subtitle.SUBTYPE.SUB_INVALID;
        private Subtitle subtitle = null;
        private SubData data = null;
        private int display_width = 0;
        private int display_height = 0;
        private int video_width = 0;
        private int video_height = 0;
        private boolean hasopenInsubfile = false;
        private static SubManager submanager = null;

        public static SubManager getinstance() {
            if (submanager == null) {
                submanager = new SubManager();
            }
            return submanager;
        }

        public SubManager() {
            Log.i(TAG,"[SubManager]-1-");
            subtitle = new Subtitle();
            Log.i(TAG,"[SubManager]-2-");
            clear();
        }

        public void clear() {
            display_width = 0;
            display_height = 0;
            video_width = 0;
            video_height = 0;
        }

        public void startSubThread() {
            if (subtitle != null) {
                subtitle.startSubThread();
            }
        }

        public void stopSubThread() {
            if (subtitle != null) {
                subtitle.stopSubThread();
            }
        }

        public void startSocketServer() {
            if (subtitle != null) {
                subtitle.startSocketServer();
            }
        }

        public void stopSocketServer() {
            if (subtitle != null) {
                subtitle.stopSocketServer();
            }
        }

        public void setIOType(int type) {
            if (subtitle != null) {
                subtitle.setIOType(type);
            }
        }

        public String getPcrscr() {
            String ret = null;
            if (subtitle != null) {
                ret = subtitle.getPcrscr();
            }
            return ret;
        }

        public void loadSubtitleFile (String path, String enc) throws Exception {
            if (subapi != null) {
                if (subapi.type() == Subtitle.SUBTYPE.INSUB) {
                    //don't release now,apk will call closeSubtitle when change file.
                }
                else {
                    subapi.closeSubtitle();
                    subapi = null;
                }
            }

            subtitle.setSystemCharset (enc);

            try {
                subtitle.setSubname (path);
                type = subtitle.getSubType();
                if (type == Subtitle.SUBTYPE.SUB_INVALID) {
                    subapi = null;
                }
                else {
                    subapi = subtitle.parse();
                }
            }
            catch (Exception e) {
                Log.e (TAG, "loadSubtitleFile, error is " + e.getMessage());
                throw e;
            }
        }

        public Subtitle.SUBTYPE setFile (SubID file, String enc) throws Exception {
            if (subapi != null) {
                if (subapi.type() == Subtitle.SUBTYPE.INSUB) {
                    //don't release now,apk will call closeSubtitle when change file.
                }
                else {
                    subapi.closeSubtitle();
                    subapi = null;
                }
            }

            subtitle.setSystemCharset (enc);
            // load Input File
            try {
                subtitle.setSubID (file);
                type = subtitle.getSubType();
                if (type == Subtitle.SUBTYPE.SUB_INVALID) {
                    subapi = null;
                }
                else {
                    if (type == Subtitle.SUBTYPE.INSUB) {
                        hasopenInsubfile = true;
                    }
                    subapi = subtitle.parse();
                }
            }
            catch (Exception e) {
                Log.e (TAG, "setFile, error is " + e.getMessage());
                throw e;
            }

            return type;
        }

        public Subtitle.SUBTYPE getSubType() {
            Subtitle.SUBTYPE type = subtitle.getSubType();
            return type;
        }

        public String getFont() {
            return subtitle.getFont();
        }
        public int getDisplayWidth() {
            //Log.d(TAG, "display_width:"+display_width);
            return display_width;
        }

        public int getDisplayHeight() {
            //Log.d(TAG, "display_height:"+display_height);
            return display_height;
        }

        public int getVideoWidth() {
            //Log.d(TAG, "video_height:"+video_width);
            return video_width;
        }

        public int getVideoHeight() {
            //Log.d(TAG, "video_height:"+video_height);
            return video_height;
        }

        public void setDisplayResolution (int width, int height) {
            if ( (width <= 0) || (height <= 0)) {
                return;
            }
            this.display_width = width;
            this.display_height = height;
            //Log.d(TAG,"set display width:" + display_width + ", height:" + display_height);
        }

        public void setVideoResolution (int width, int height) {
            if ( (width <= 0) || (height <= 0)) {
                return;
            }
            this.video_width = width;
            this.video_height = height;
            //Log.d(TAG,"set video width:" + video_width + ", height:" + video_height);
        }

        public void closeSubtitle() {
            if (subapi != null) {
                subapi.closeSubtitle();
                subapi = null;
            }
            if (hasopenInsubfile == true) {
                hasopenInsubfile = false;
                subtitle.setSubname ("INSUB");
                try {
                    subapi = subtitle.parse();
                    subapi.closeSubtitle();
                    subapi = null;
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
            }
        }
        public SubtitleApi getSubtitleFile() {
            return subapi;
        }

        public int  getSubTypeDetial() {
            if (subapi != null) {
                return subapi.getSubTypeDetial();
            }
            return -1;
        }

        public SubData getSubData (int ms) {
            if (subapi != null) {
                /*
                //this has been detected in subtitleview,so remove this.
                if (data != null) {
                    if ((ms < data.beginTime()) || (ms > data.endTime())) {
                        data = subapi.getdata(ms);
                    }
                }
                else {
                    data = subapi.getdata (ms);
                }*/
                data = subapi.getdata (ms);
                return data;
            }
            return null;
        }

    public void resetForSeek() {
        if (subtitle != null) {
            subtitle.resetForSeek();
        }
    }
}
