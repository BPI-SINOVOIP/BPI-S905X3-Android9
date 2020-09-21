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

public class SubData {
    public SubData (String s, int start , int end) {
        substr = s; starttime = start; endtime = end;
    }

    public SubData (Bitmap bit, int start, int end) {
        bitmap = bit; starttime = start; endtime = end;
    }

    public SubData (String s, int start , int end, int size) {
        substr = s; starttime = start; endtime = end; subSize = size;
    }

    public SubData (Bitmap bit, int start, int end, int size, int o_w, int o_h) {
        bitmap = bit; starttime = start; endtime = end; subSize = size; originW=o_w; originH=o_h;
    }

    public int gettype() {
        if (bitmap != null) {
            return 1;
        }
        else {
            return 0;
        }
    }

    public String getSubString() {
        return substr;
    }

    public Bitmap getSubBitmap() {
        return bitmap;
    }

    public int beginTime() {
        return starttime;
    }

    public void setEndTime (int end) {
        endtime = end;
    }

    public int endTime() {
        return endtime;
    }

    public int subSize() {
        return subSize;
    }

    public int getOriginW() {
        return originW;
    }

    public int getOriginH() {
        return originH;
    }

    private String substr = null;
    private Bitmap bitmap = null;
    private int originW = 0;
    private int originH = 0;
    private int starttime = 0;
    private int endtime = 0;
    private int subSize = -1;
    int type = 0;//0-string,1-bitmap
}