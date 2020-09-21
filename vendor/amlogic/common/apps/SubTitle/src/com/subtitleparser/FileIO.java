/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;

import java.io.*;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;

/**
* General File I/O methods.
*
* @author
*/
public class FileIO {
    static String mFont = null;
        /**
        * Fetch the entire contents of a text file, and return it in a String.
        * This style of implementation does not throw Exceptions to the caller.
        *
        * @param aFile is a file which already exists and can be read.
        * @param enc text encode in aFile
        */
        static private String getContents (File aFile, String enc) throws IOException {
            //...checks on aFile are elided
            StringBuffer contents = new StringBuffer();

            //declared here only to make visible to finally clause
            BufferedReader input = null;
            try {
                //use buffering
                //this implementation reads one line at a time
                input = new BufferedReader (new InputStreamReader (new FileInputStream (aFile), enc));
                String line = null; //not declared within while loop
                while ( (line = input.readLine()) != null) {
                    contents.append (line);
                    contents.append (System.getProperty ("line.separator"));
                }
            }
            catch (FileNotFoundException ex) {
                throw ex;
            }
            catch (IOException ex) {
                throw ex;
            }
            finally {
                try {
                    if (input != null) {
                        //flush and close both "input" and its underlying FileReader
                        input.close();
                    }
                }
                catch (IOException ex) {
                    throw ex;
                }
            }

            return contents.toString();
        }

        public static String getFont() {
            Log.i("getFont", "mFont:" + mFont);
            return mFont;
        }

        public static Subtitle.SUBTYPE dectFileType (String filePath, String encoding) {
            Log.i("FileIO", "[dectFileType]filePath:" + ",encoding:"+encoding);
            BufferedReader input = null;
            Subtitle.SUBTYPE type = Subtitle.SUBTYPE.SUB_INVALID;
            int testMaxLines = 60;
            Pattern MICRODVD_Pattern = Pattern.compile ("\\{\\d+\\}\\{\\d+\\}");
            Pattern MICRODVD_Pattern_2 = Pattern.compile ("\\{\\d+\\}\\{\\}");
            Pattern SUB_MPL1_Pattern = Pattern.compile ("\\d+,\\d+,\\d+,");
            Pattern SUB_MPL2_Pattern = Pattern.compile ("\\[\\d+\\]\\[\\d+\\]");
            Pattern SUBRIP_Pattern = Pattern.compile ("\\d+:\\d+:\\d+.\\d+,\\d+:\\d+:\\d+.\\d+");
            Pattern SUBVIEWER_Pattern = Pattern.compile ("\\d+:\\d+:\\d+[\\,\\.:]\\d+ ?--> ?\\d+:\\d+:\\d+[\\,\\.:]\\d+");
            Pattern SUBVIEWER2_Pattern = Pattern.compile ("\\{T \\d+:\\d+:\\d+:\\d+ ");
            Pattern SUBVIEWER3_Pattern = Pattern.compile ("\\d+:\\d+:\\d+,\\d+\\s+\\d+:\\d+:\\d+,\\d+");
            Pattern SAMI_Pattern = Pattern.compile ("<SAMI>");
            Pattern JACOSUB_Pattern = Pattern.compile ("\\d+:\\d+:\\d+.\\d+ \\d+:\\d+:\\d+.\\d+");
            Pattern JACOSUB_Pattern_2 = Pattern.compile ("@\\d+ @\\d+");
            Pattern VPLAYER_Pattern = Pattern.compile ("\\d+:\\d+:\\d+[: ]");
            Pattern VPLAYER_Pattern_2 = Pattern.compile ("\\d+:\\d+:\\d+.\\d+[: ]");
            Pattern PJS_Pattern = Pattern.compile ("\\d+\\d+,\"");
            Pattern PJS_Pattern_2 = Pattern.compile ("\\d+,+.*\\d+,");
            Pattern MPSUB_Pattern = Pattern.compile ("FORMAT=\\d+");
            Pattern MPSUB_Pattern_2 = Pattern.compile ("FORMAT=TIME");
            Pattern AQTITLE_Pattern = Pattern.compile ("-->>");
            Pattern SUBRIP9_Pattern = Pattern.compile ("\\[\\d+:\\d+:\\d+\\]");
            Pattern LRC_Pattern = Pattern.compile ("\\[\\d+:\\d+.\\d+\\]" + "(.*?)");
            Pattern XML_Pattern = Pattern.compile ("<Subtitle>");
            Pattern XML_Pattern2 = Pattern.compile ("<tt xml:lang=");
            Matcher matcher = null;
            int i = 0;
            int idx = 0;
            try {
                //use buffering
                //this implementation reads one line at a time
                //          input = new BufferedReader( new FileReader(filePath));
                input = new BufferedReader (new InputStreamReader (new FileInputStream (new File (filePath)), encoding), 1024);
                String line = null; //not declared within while loop
                mFont = null; //reset font
                try {
                    while ( (line = input.readLine()) != null && testMaxLines > 0) {
                        Log.v ("dectFileType", " -----new line--------" + (60 - testMaxLines) + "  " + line);

                        if (line.indexOf("Format:") >= 0) {
                            String[] propNames = line.split(", ");
                            for (String propName: propNames) {
                                if (propName.equals("Fontname")) {
                                    break;
                                }
                                idx++;
                            }
                        }
                        else if (line.indexOf("Style:") >= 0) {
                            String[] propTypes = line.split(",");
                            for (String propType: propTypes) {
                                if (i == idx) {
                                    mFont = propType;
                                    break;
                                }
                                i++;
                            }
                        }

                        if (line.length() > 3000) {
                            type = Subtitle.SUBTYPE.SUB_INVALID;
                            break;
                        }
                        testMaxLines--;
                        if (line == null) {
                            type = Subtitle.SUBTYPE.SUB_INVALID;
                            break;
                        }
                        if (line.startsWith ("Dialogue: ")) {
                            type = Subtitle.SUBTYPE.SUB_SSA;
                            break;
                        }
                        matcher = MICRODVD_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MICRODVD;
                            break;
                        }
                        matcher = MICRODVD_Pattern_2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MICRODVD ;
                            break;
                        }
                        matcher = SUB_MPL1_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MPL1;
                            break;
                        }
                        matcher = SUB_MPL2_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MPL2;
                            break;
                        }
                        matcher = SUBRIP_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SUBRIP;
                            break;
                        }
                        matcher = SUBVIEWER_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SUBVIEWER;
                            break;
                        }
                        matcher = SUBVIEWER2_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SUBVIEWER2;
                            break;
                        }
                        matcher = SUBVIEWER3_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SUBVIEWER3;
                            break;
                        }
                        matcher = SAMI_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SAMI;
                            break;
                        }
                        matcher = XML_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_XML;
                            break;
                        }
                        matcher = XML_Pattern2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_XML2;
                            break;
                        }
                        matcher = JACOSUB_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_JACOSUB;
                            break;
                        }
                        matcher = JACOSUB_Pattern_2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_JACOSUB;
                            break;
                        }
                        matcher = VPLAYER_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_VPLAYER;
                            break;
                        }
                        matcher = VPLAYER_Pattern_2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_VPLAYER;
                            break;
                        }
                        if (line.startsWith ("<")) {
                            type = Subtitle.SUBTYPE.SUB_RT;
                            break;
                        }
                        matcher = PJS_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_PJS;
                            break;
                        }
                        matcher = PJS_Pattern_2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_PJS;
                            break;
                        }
                        matcher = MPSUB_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MPSUB;
                            break;
                        }
                        matcher = MPSUB_Pattern_2.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_MPSUB;
                            break;
                        }
                        matcher = AQTITLE_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_AQTITLE;
                            break;
                        }
                        matcher = SUBRIP9_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_SUBRIP09;
                            break;
                        }
                        matcher = LRC_Pattern.matcher (line);
                        if (matcher.find()) {
                            type = Subtitle.SUBTYPE.SUB_LRC;
                            break;
                        }
                    }
                    if (input != null) {
                        input.close();
                    }
                    Log.v ("dectFileType", " type:" + type);
                    return type;
                }
                catch (IOException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
            }
            catch (FileNotFoundException e) {
                e.printStackTrace();
            }
            catch (UnsupportedEncodingException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            Log.v ("dectFileType", " type:Subtitle.SUBTYPE.SUB_INVALID");
            return Subtitle.SUBTYPE.SUB_INVALID;
        }

        /**
        * Fetch the entire contents of a text file, and return it in a String.
        *
        * @param filePath is a file which already exists and can be read.
        * @param enc
        */
        static public String file2string (String filePath, String enc) throws IOException {
            Log.i ("FileIO", "filename:" + filePath);
            File f = new File (filePath);
            if (!f.exists()) { throw new FileNotFoundException (filePath + " doesn't exist."); }
            if (f.isDirectory()) { throw new FileNotFoundException (filePath + " is a directory."); }

            return getContents (f, enc);
        }

        /**
        * Write a String into a text file.
        *
        * @param text the text to write in the file;
        * @param filePath is a file which already exists and can be read.
        * @param enc
        */
        static public void string2file (String text, String filePath, String enc) throws IOException {
            String s = "";
            File f = new File (filePath);
            f.createNewFile();
            int lineCount = 0;

            try {
                BufferedReader in = new BufferedReader (new StringReader (text));
                PrintWriter out = new PrintWriter (new BufferedWriter (new OutputStreamWriter (new FileOutputStream (filePath), enc)));

                while ( (s = in.readLine()) != null) {
                    out.println (s);
                    lineCount++;
                }
                out.close();
            }
            catch (IOException e) {
                throw new IOException ("Problem of writing at line " + lineCount + ": " + e);
            }
        }
}
