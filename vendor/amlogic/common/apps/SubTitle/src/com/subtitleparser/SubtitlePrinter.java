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
 * a subtitle printer.
 *
 * @author
 */
public abstract class SubtitlePrinter {

        /**
        * @return a String with the entire file in the correct format.
        */
        public String print (SubtitleFile sf) throws Exception {

            SubtitleLine tmp = null;
            StringBuilder res = new StringBuilder();

            for (int i = 0; i < sf.size(); i++) {
                tmp = (SubtitleLine) sf.get (i);
                res.append (print (tmp) + System.getProperty ("line.separator"));
            }

            return res.toString();
        }

        public abstract String print (SubtitleLine sl) throws Exception;
        public abstract String print (SubtitleTime st) throws Exception;

}
