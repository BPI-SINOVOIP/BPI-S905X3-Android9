/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;



/**
 * a subtitle parser.
 *
 * @author
 */
public interface SubtitleParser {

    //  public SubtitleFile parse(String s ) throws MalformedSubException;
    public SubtitleApi parse (String s1, String s2) throws MalformedSubException ;
    public SubtitleApi parse (String s, int index) throws MalformedSubException;

    //add support pixmap: get w, h,source,and then change to Bitmap
    //public Bitmap curBitmap();
}
