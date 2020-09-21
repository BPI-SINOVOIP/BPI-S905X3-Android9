/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser.subtypes;

import java.util.StringTokenizer;

import com.subtitleparser.SubtitleLine;
import com.subtitleparser.SubtitlePrinter;
import com.subtitleparser.SubtitleTime;

/**
 * a .SRT subtitle printer.
 *
 * @author
 */
public class SubPrinter extends SubtitlePrinter {

        @Override
        public String print (SubtitleLine sl) throws Exception {

            String newLine = System.getProperty ("line.separator");
            String tmpText = "";
            int i = 0;
            //text formatting

            StringTokenizer st = new StringTokenizer (sl.getText(), newLine);
            while (st.hasMoreTokens()) {
                i++;
                if (i > 1) {
                    tmpText = tmpText + "|" + st.nextToken();
                }
                else {tmpText = st.nextToken();}
            }

            return "{" + print (sl.getBegin()) + "}{" + print (sl.getEnd()) + "}" + tmpText;
        }


        @Override
        public String print (SubtitleTime st) throws Exception {
            if (!st.isFramesValid()) { throw new Exception ("Time is not correct. I cannot write a correct .SUB subtitle."); }

            return String.valueOf (st.getFrameValue());
        }

}
