/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;


public class SubtitleTime implements Comparable {
        //-1 if the value is not set
        private int frames = -1;
        //hour, minute, second, millisec
        private int h = -1, min = -1, sec = -1, mil = -1;
        private boolean isTimeSet = false;
        private boolean isFrameSet = false;
        static float framerate = 0;

        public SubtitleTime (int frames) {
            this.frames = frames;
            isFrameSet = true;
            if (!isFramesValid()) { normalizeFrames(); }
        }

        public SubtitleTime (int h, int min, int sec, int mil) {
            this.h = h;
            this.min = min;
            this.sec = sec;
            this.mil = mil;
            isTimeSet = true;
            if (!isTimeValid()) { normalizeTime(); }
        }


        public void setAllValues (float framerate) throws Exception {

            if ( (!isFrameSet) && (isTimeSet)) {
                frames = getFrameValue();
                isFrameSet = true;
            }
            else if ( (!isTimeSet) && (isFrameSet)) {
                h = 0; min = 0; sec = 0;
                mil = getMilValue();
                isTimeSet = true;
                if (!isTimeValid()) { normalizeTime(); }
            }
        }


        //getValue
        /**
        * @return value in milliseconds, -1 if time is not valid
        */
        public int getMilValue() throws Exception {
            if (isTimeSet) {
                return mil + sec * 1000 + min * 60000 + h * 3600000;
            }
            else if (isFrameSet) {
                return Subtitle.frame2mil (frames, framerate);
            }
            else { return -1; }
        }


        public int getFrameValue() throws Exception {

            if (isFramesValid()) {
                return frames;
            }
            else if (isTimeSet) {
                return Subtitle.mil2frame (getMilValue(), framerate);
            }
            else { return -1; }
        }

        //shift
        public void timeShiftMil (int millisec, float framerate) throws Exception {
            if (isTimeSet) {
                this.mil = this.mil + millisec;
                if (!isTimeValid()) { normalizeTime(); }
            }
            else { timeShiftFr (Subtitle.mil2frame (millisec, framerate), 0); }
        }

        public void timeShiftFr (int frames, float framerate) throws Exception {
            if (isFrameSet) {
                this.frames = this.frames + frames;
                if (!isFramesValid()) { normalizeFrames(); }
            }
            else { timeShiftMil (Subtitle.frame2mil (frames, framerate), 0); }
        }

        //check consistency
        public boolean isFramesValid() {
            if (frames < 0) { return false; }
            else { return true; }
        }

        public boolean isTimeValid() {
            if (h < 0) {h = 0;}
            if ( (min < 0) || (min > 59)) {
                return false;
            }
            if ( (sec < 0) || (sec > 59)) {
                return false;
            }
            if ( (mil < 0) || (mil > 999)) {
                return false;
            }
            return true;
        }


        //normalize
        public void normalizeTime() {
            int addSec, addMin, addH;
            addSec = addMin = addH = 0;
            //millisecs
            addSec = mil / 1000;
            mil = mil % 1000;
            if (mil < 0) {mil = 1000 + mil; addSec--;}
            //sec
            sec = sec + addSec;
            addMin = sec / 60;
            sec = sec % 60;
            if (sec < 0) {sec = 60 + sec; addMin--;}
            //min
            min = min + addMin;
            addH = min / 60;
            min = min % 60;
            if (min < 0) {min = 60 + min; addH--;}
            //hour
            h = h + addH;
            if (h < 0) {h = 0;}
        }

        public void normalizeFrames() {
            if (frames < 0) {frames = 0;}
        }

        public int compareTo (Object o) {
            /*SubtitleTime st=(SubtitleTime)o;
            if (isFrameSet) {
                if (frames < st.getFrameValue(0)) return -1;
                if (frames == st.getFrameValue(0)) return 0;
                if (frames > st.getFrameValue(0)) return 1;
            }else if(isTimeSet) {
                if ((this.getMilValue(0)) < (st.getMilValue(0)))
                    return -1;
                if ((this.getMilValue(0)) == (st.getMilValue(0)))
                    return 0;
                if ((this.getMilValue(0)) > (st.getMilValue(0)))
                    return 1;
            }
            */
            return -2;
        }

        public int getH() {
            return h;
        }


        public int getMin() {
            return min;
        }


        public int getSec() {
            return sec;
        }


        public int getMil() {
            return mil;
        }

        public boolean getIsTimeSet() {
            return isTimeSet;
        }

        public boolean getIsFrameSet() {
            return isFrameSet;
        }

}
