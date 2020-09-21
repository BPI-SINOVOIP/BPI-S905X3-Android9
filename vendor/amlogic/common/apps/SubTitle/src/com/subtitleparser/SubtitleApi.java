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

public abstract class SubtitleApi {
        //protected SubtitleFile SubFile =null;
        protected int begingtime = 0 , endtime = 0 ;
        abstract public SubData getdata (int ms);
        abstract public void closeSubtitle();
        abstract public int getSubTypeDetial();
        abstract public Subtitle.SUBTYPE type();
}



