/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;

import java.io.UnsupportedEncodingException;
import java.util.LinkedList;
import android.util.Log;
import com.subtitleparser.Subtitle;
import java.util.List;
import java.util.ArrayList;

/**
 * an subtitle file.
 *
 * @author
 */
public class SubtitleFile extends LinkedList {

        private float framerate;
        private int curIndex;


        public SubtitleFile() {
            framerate = 0;
            curIndex = 0;
        }

        /**
         * Remove hearing impaired subs in entire file.
         *
         * @param start
         *            char (eg. [);
         * @param end
         *            char (eg. ]);
         * @return number of HI subs removed.
         */
        public int removeHearImp (String start, String end) {
            SubtitleLine tmp = null;
            int removed = 0;
            String s = "";
            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                s = Subtitle.removeHearImp (tmp.getText(), start, end);
                if (!s.equals (tmp.getText())) {
                    removed++;
                    tmp.setText (s);
                }
            }
            return removed;
        }

        /**
         * Recreate the subtitles numeration to avoid inconsistency.
         */
        public void reindex() {
            SubtitleLine tmp = null;
            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                tmp.setSubN (i + 1);
            }
        }

        /**
         * Calculate the frame/sec conversion in entire file Needs a positive
         * framerate.
         *
         */
        public void setAllTimeValues() throws Exception {

            SubtitleLine tmp = null;
            // SubtitleTime begin=null,end=null;

            if (!isValidFramerate())
                throw new Exception (
                    "Error::: I need a positive framerate to perform this conversion!");
            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                (tmp.getBegin()).setAllValues (getFramerate());
                (tmp.getEnd()).setAllValues (getFramerate());
            }
        }

        /**
         * Calculate the frame/sec conversion in entire file Needs a positive
         * framerate.
         *
         */
        public int removeEmptySubs() {
            SubtitleLine tmp = null;
            int removed = 0;
            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                if (tmp.isTextEmpty()) {
                    this.remove (i);
                    removed++;
                }
            }
            return removed;
        }

        /**
         * Time shift, positive or negative.
         *
         * @param millisec
         *            milliseconds, positive or negative value;
         */
        public void timeShiftMil (int millisec) throws Exception {
            SubtitleLine tmp = null;

            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                tmp.timeShiftMil (millisec, getFramerate());
            }
        }

        /**
         * Time shift, positive or negative.
         *
         * @param frames
         *            milliseconds, positive or negative value;
         */
        public void timeShiftFr (int frames) throws Exception {
            SubtitleLine tmp = null;

            for (int i = 0; i < this.size(); i++) {
                tmp = (SubtitleLine) get (i);
                tmp.timeShiftFr (frames, getFramerate());
            }
        }

        public void setFramerate (float framerate) {
            this.framerate = framerate;
            SubtitleTime.framerate = framerate;
        }

        public float getFramerate() {
            return framerate;
        }

        public boolean isValidFramerate() {
            if (getFramerate() > 0) {
                return true;
            }
            else {
                return false;
            }
        }

        public int curSubtitleIndex() {
            return curIndex;
        }

        public SubtitleLine curSubtitle() {
            return (SubtitleLine) get (curIndex);
        }

        public SubtitleLine getSubtitle (int idx) {
            return (SubtitleLine) get (idx);
        }

        public void setCurSubtitleIndex (int index) {
            if (index >= 0 && index < size() - 1) {
                curIndex = index;
            }
        }

        public int toNextSubtitle() {
            if (curIndex < size() - 1) {
                curIndex++;
            }
            return curIndex;
        }

        public int toPrevSubtitle() {
            if (curIndex > 0) {
                curIndex--;
            }
            return curIndex;
        }

        public int findSubtitle (int millisec) {
            int ret = -1;
            SubtitleLine p = null;
            SubtitleLine n = null;
            int i;
            p = (SubtitleLine) get (curIndex);
            try {
                if (millisec >= p.getBegin().getMilValue()) {
                    while (curIndex < size() - 1) {
                        n = (SubtitleLine) get (curIndex + 1);
                        if (millisec >= n.getBegin().getMilValue()) {
                            curIndex += 1;
                        }
                        else {
                            ret = curIndex;
                            break;
                        }
                    }
                }
                else {
                    while (curIndex > 0) {
                        int tmpIdx = curIndex - preOffset;
                        if (tmpIdx <= 0) {
                            tmpIdx = 0;
                        }
                        n = (SubtitleLine) get (tmpIdx);
                        if (millisec >= n.getBegin().getMilValue()) {
                            ret = tmpIdx;
                            break;
                        }
                        else {
                            curIndex -= preOffset;
                        }
                    }
                    if (curIndex <= 0) {
                        curIndex = 0;
                    }
                }
            }
            catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            return ret;
        }

        public int matchSubtitle (int millisec) {
            int ret = -1;
            ret = findSubtitle (millisec);
            if (ret != -1) {
                setCurSubtitleIndex (ret);
            }
            return ret;
        }

        private final int preOffset = 2;
        private ArrayList<Integer> idxlist = new ArrayList<Integer>();
        public void findSubtitles (int idx, int millisec) {
            //SubtitleLine n = null;
            SubtitleLine sl = null;
            int startTime = -1;
            int endTime = -1;
            //reset curIndex for backward seeking
            findSubtitle (millisec);
            idxlist.clear();
            try {
                while (idx < size()) {
                    sl = (SubtitleLine) get (idx);
                    startTime = sl.getBegin().getMilValue();
                    endTime = sl.getEnd().getMilValue();
                    if ( (millisec >= startTime) && (millisec <= endTime)) {
                        idxlist.add (idx);
                    }
                    idx ++;
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
        }

        public void matchSubtitles (int millisec) {
            findSubtitles (curIndex, millisec);
            if (idxlist.size() != 0) {
                if (idxlist.get (0) < size()) {
                    setCurSubtitleIndex (idxlist.get (0));
                }
            }
        }

        public int idxlistSize() {
            return idxlist.size();
        }

        public int getIdx (int i) {
            if (i <= idxlist.size() - 1) {
                return idxlist.get (i);
            }
            else {
                return -1;
            }
        }

        public void appendSubtitle (int index, int start, int end, String text) {
            //public int appendSubtitle(int index) {
            SubtitleLine sl = null;
            SubtitleTime startTime = null;
            SubtitleTime endTime = null;
            startTime = new SubtitleTime (start / 3600000, ( (start / 1000) % 3600) / 60, (start / 1000) % 60, start % 1000);
            endTime = new SubtitleTime (end / 3600000, ( (end / 1000) % 3600) / 60, (end / 1000) % 60, end % 1000);
            Log.d ("subtitleFile", "appendSubtitleFile:(" + start + "," + end + ")" + text);
            text = text.replaceAll ("\\{\\\\fn.*?\\}", "");
            text = text.replaceAll ("\\{\\\\r\\}", "");
            text = text.replaceAll ("\\{\\\\fs.*?\\}", "");
            sl = new SubtitleLine (index, startTime, endTime, text);
            add (sl);
            Log.d ("subtitleFile", "appendSubtitleFile" + index);
        }

        public  String bytesToHexString(byte[] bArray) {
            StringBuffer sb = new StringBuffer(bArray.length);
            String sTemp;
            for (int i = 0; i < bArray.length; i++) {
                sTemp = Integer.toHexString(0xFF & bArray[i]);
                if (sTemp.length() < 2)
                    sb.append(0);
                    sb.append(sTemp.toUpperCase());
            }
            return sb.toString();
        }

        public String stringToHexString(String s) {
            StringBuffer sb = new StringBuffer(s.length());
            String sTemp;
            for (int i = 0; i < s.length(); i++) {
                sTemp = Integer.toHexString(s.charAt(i));
                if (sTemp.length() < 2)
                    sb.append(0);
                    sb.append(sTemp.toUpperCase());
            }
            return sb.toString();
        }
        public boolean JudgeGBK(byte[] bArray) {
            boolean hasGBK = false;
            for (int i = 0; i < bArray.length-1; i++) {
                int bArray1 = bArray[i] & 0xff;
                int bArray2 = bArray[i+1]& 0xff;
                if (bArray1 > 128 && bArray1 <255 && bArray2 > 63 && bArray2 <255 && bArray2 != 127) {
                    hasGBK = true;
                }
            }
            return hasGBK;
        }
        public void appendSubtitle (int index, int start, int end, byte[] bytearray, String encode1) {
            Log.d ("subtitleFile", "[appendSubtitle]encode1:"+encode1);
            //public int appendSubtitle(int index) {
            String encode = encode1;
            if (encode.compareTo ("UTF-16LE") == 0 || encode.compareTo ("UTF-16BE") == 0) {
                encode = "UTF8";
            }
            SubtitleLine sl = null;
            SubtitleTime startTime = null;
            SubtitleTime endTime = null;
            String text = null;
            String texttemp = "";
            String ISO88591S = "iso88591";
            String GBKS = "GBK";
            String temp1 = "";
            String temp3 = "";
            if (encode.equals(GBKS) && !JudgeGBK(bytearray)) {
                texttemp = bytesToHexString(bytearray);
                try {
                    temp1 = new String(bytearray,ISO88591S);
                } catch (UnsupportedEncodingException e) {
                    Log.e("subtitleFile", "erro while decodeing with iso88591");
                }
                temp3 = stringToHexString(temp1);
                if (texttemp.equals(temp3)) {
                    encode = ISO88591S;
                }
            }
            try {
                text = new String (bytearray, encode);
            }
            catch (UnsupportedEncodingException e) {
                Log.d ("subtitleFile", "byte to string err!!!----------");
                e.printStackTrace();
            }
            startTime = new SubtitleTime (start / 3600000, ( (start / 1000) % 3600) / 60, (start / 1000) % 60, start % 1000);
            endTime = new SubtitleTime (end / 3600000, ( (end / 1000) % 3600) / 60, (end / 1000) % 60, end % 1000);
            Log.d ("subtitleFile", "appendSubtitleFile:(" + start + "," + end + ")" + text);
            text = text.replaceAll ("\\{\\\\fn.*?\\}", "");
            text = text.replaceAll ("\\{\\\\r\\}", "");
            text = text.replaceAll ("\\{\\\\fs.*?\\}", "");
            text = text.replaceAll ("\\{*?\\\\frx.*?\\}", "");
            text = text.replaceAll ("\\{*?\\\\fad.*?\\}", "");
            text = text.replaceAll ("\\{*?\\\\fs.*?\\}", "");
            int idx = text.indexOf (")}m");
            if (idx >= 0 && (text.substring (idx + 3)).startsWith (" ")) {
                sl = new SubtitleLine (index, startTime, endTime, "");
            }
            else {
                idx = text.indexOf ("0)}");
                if (idx >= 0) {
                    text = text.substring (idx + 3);
                }
                idx = text.indexOf ("500)}");
                if (idx >= 0) {
                    text = text.substring (idx + 5);
                }
                sl = new SubtitleLine (index, startTime, endTime, text);
            }
            try {
                if (size() == 0) {
                    add (sl);
                }
                else if (sl.getBegin().getMilValue() > ( (SubtitleLine) get (size() - 1)).getBegin().getMilValue()) {
                    add (sl);
                }
                else {
                    addSubtitleLine (sl);
                }
            }
            catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
        public void addSubtitleLine (SubtitleLine s) {
            int i = size();
            while (i > 0) {
                try {
                    if (s.getBegin().getMilValue() < ( (SubtitleLine) get (i - 1)).getBegin().getMilValue()) {
                        i--;
                    }
                    else {
                        break;
                    }
                }
                catch (Exception e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            add (i, s);
        }


}
