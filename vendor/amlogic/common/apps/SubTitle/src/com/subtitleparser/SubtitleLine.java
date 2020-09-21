/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;

import android.graphics.Bitmap;



/**
 * List that contains an entire file
 *
 * @author
 */
public class SubtitleLine {

        private int subN;   //useless code , jeff

        private String text;
        private SubtitleTime begin, end;
        public Bitmap bitmap;
        //private other sub info

        public SubtitleLine() {
            subN = 0;
            text = "";
        }

        public SubtitleLine (SubtitleTime begin, SubtitleTime end, String text) {
            this.begin = begin;
            this.end = end;
            this.text = text;
        }

        public SubtitleLine (int subN, SubtitleTime begin, SubtitleTime end, String text) {
            this (begin, end, text);
            this.subN = subN;
        }

        public void timeShiftMil (int millisec, float framerate) throws Exception {
            begin.timeShiftMil (millisec, framerate);
            end.timeShiftMil (millisec, framerate);
        };

        public void timeShiftFr (int frames, float framerate) throws Exception {
            begin.timeShiftFr (frames, framerate);
            end.timeShiftFr (frames, framerate);
        }


        public void setText (String text) {
            this.text = text;
        }

        public String getText() {
            return text;
        }

        //add support pixmap: get w, h,source,and then change to Bitmap
        /*public Bitmap curBitmap() {
            //call jni
            return Bitmap.createBitmap (null);
        }*/
        public SubtitleTime getBegin() {
            return begin;
        }

        public void setBegin (SubtitleTime t) {
            begin = t;
        }

        public SubtitleTime getEnd() {
            return end;
        }

        public void setEnd (SubtitleTime t) {
            end = t;
        }

        public int getSubN() {
            return subN;
        }

        public void setSubN (int n) {
            subN = n;;
        }

        public boolean isTextEmpty() {
            if (text.equals ("")) {
                return true;
            }
            else { return false; }
        }

        /**
        * TO IMPLEMENT
        */
        public boolean isValid() {
            //if((begin.isValid())&&(end.isValid())&&(begin.compareTo(end)>0)&&(!isTextEmpty()))return true;
            //else
            return false;
        }

        //toString depends on the format
        //public abstract String toString(){}
}
