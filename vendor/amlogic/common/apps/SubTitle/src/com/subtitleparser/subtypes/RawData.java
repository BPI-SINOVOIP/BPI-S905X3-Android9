/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser.subtypes;

import java.io.UnsupportedEncodingException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

import com.subtitleparser.SubtitleLine;
import com.subtitleparser.SubtitleTime;
public class RawData {
        public RawData (int[] data, int t, int w, int h, int delay, String st) { // ([BILjava/lang/String;)V
            rawdata = data; type = t; width = w; height = h; sub_delay = delay; codec = st; subtitlestring = null;
        }
        public RawData (int[] data, int t, int w, int h, int o_w, int o_h, int size, int start, int delay, String st) { // ([BILjava/lang/String;)V
            rawdata = data; type = t; width = w; height = h; origin_width=o_w; origin_height=o_h; sub_size = size; sub_start = start; sub_delay = delay; codec = st; subtitlestring = null;
        }

        public RawData(byte[] data,int t,int size,int start,int delay,String st){
            type=t;sub_size=size;sub_start=start;sub_delay=delay;codec=st;
            try {
                subtitlestring = new String((byte[])data,"UTF8");
            }catch (UnsupportedEncodingException e) {
                e.printStackTrace();
            }
        }

        public RawData (byte[] data, int delay, String st) {
            type = 0;
            sub_delay = delay;
            try {
                subtitlestring = new String ( (byte[]) data, "UTF8");
                subtitlestring = subtitlestring.replaceAll ("Dialogue:[^,]*,\\s*\\d:\\d\\d:\\d\\d.\\d\\d\\s*,\\s*\\d:\\d\\d:\\d\\d.\\d\\d[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,", "");
                subtitlestring = subtitlestring.replaceAll ("<(.*?)>", "");
                subtitlestring = subtitlestring.replaceAll ("\\{(.*?)\\}", "");
                subtitlestring = subtitlestring.replaceAll ("\\\\N", "\\\n");
                subtitlestring = subtitlestring.replaceAll ("\\\\n", "");
            }
            catch (UnsupportedEncodingException e) {
                e.printStackTrace();
            }
        }
        int[] rawdata;
        int type;
        int width;
        int height;
        int origin_width;
        int origin_height;
        int sub_size;
        int sub_start;    //ms
        int sub_delay;    //ms
        String subtitlestring;
        String codec;
}
