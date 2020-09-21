/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;


import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;

import java.text.NumberFormat;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.graphics.Bitmap;
import android.graphics.Bitmap.Config;
import android.util.Log;

import java.io.RandomAccessFile;
import java.nio.channels.FileChannel;
import java.nio.ByteBuffer;

import com.subtitleparser.subtypes.*;


/**
 * Various static methods.
 *
 * @author
 */
public class Subtitle {

        public enum SUBTYPE {
            SUB_INVALID ,
            SUB_MICRODVD,
            SUB_SUBRIP,
            SUB_SUBVIEWER,
            SUB_SAMI   ,
            SUB_VPLAYER ,
            SUB_RT   ,
            SUB_SSA  ,
            SUB_PJS   ,
            SUB_MPSUB   ,
            SUB_AQTITLE   ,
            SUB_SUBVIEWER2 ,
            SUB_SUBVIEWER3 ,
            SUB_SUBRIP09  ,
            SUB_JACOSUB  ,
            SUB_MPL1  ,
            SUB_MPL2  ,
            SUB_DIVX  ,
            SUB_IDXSUB,
            SUB_COMMONTXT,
            SUB_LRC,
            SUB_XML,
            INSUB,
            SUB_XML2
        }
        private String filename = null;
        private String charset = null;
        private SUBTYPE subtype = SUBTYPE.SUB_INVALID;
        private static SubtitleApi subapi = null;
        private int index = 0;
        private static String systemCharset = "GBK";
        static {
                Log.i("Subtitle","[subtitle]-3-");
                System.loadLibrary ("subjni");
                Log.i("Subtitle","[subtitle]-4-");
               }

        public Subtitle() {  }
        public Subtitle (String name) {
            filename = name;
        }
        public void setSubname (String name) {
            filename = name;
            charset = null;
            subtype = SUBTYPE.SUB_INVALID;
            subapi = null;
            index = 0;
        }
        //add for idx sub , 1 idx file can have more than one subtitles;
        public void setSubID (SubID id) {
            filename = id.filename;
            charset = null;
            subtype = SUBTYPE.SUB_INVALID;
            subapi = null;
            index = id.index;
        }

        public static void setSystemCharset (String st) {
            systemCharset = st;
        }

        public String getCharset() {
            if (filename != null) {
                if (charset == null) {
                    charset = checkEncoding (filename , systemCharset);
                }
            }
            else {
                return null;
            }
            return charset;
        }

        public SubtitleApi parse() throws Exception {
            if (filename != null) {
                if (charset == null) {
                    getCharset();
                }
                if (subtype == null) {
                    getSubType();
                }
                subapi = parseSubtitleFile (filename);
            }
            return subapi;
        }

        public SubtitleApi getSubList() {
            if (subapi != null) {
                return subapi;
            }
            else {
                return null;
            }
        }
        public SUBTYPE getSubType() {
            if (filename != null) {
                if (filename.compareTo ("INSUB") == 0) {
                    return SUBTYPE.INSUB;
                }
                if (charset == null) {
                    getCharset();
                }
                subtype = Subtitle.fileType (filename, charset);
                return subtype;
            }
            else {
                return null;
            }
        }

        public void startSubThread() {
            startSubThreadByJni();
        }

        public void stopSubThread() {
            stopSubThreadByJni();
        }

        public void startSocketServer() {
            startSubServerByJni();
        }

        public void stopSocketServer() {
            stopSubServerByJni();
        }

        public void setIOType(int type) {
            setIOTypeByJni(type);
        }

        public String getPcrscr() {
            return getPcrscrByJni();
        }

        public void resetForSeek() {
            resetForSeekByjni();
        }

        public static native SubtitleFile parseSubtitleFileByJni (String fileName, String encode);
        native void startSubThreadByJni();
        native void stopSubThreadByJni();
        native void resetForSeekByjni();
        native void startSubServerByJni();
        native void stopSubServerByJni();
        native void setIOTypeByJni(int type);
        native String getPcrscrByJni();

        /**
         * Parse a known type subtitle file into a SubtitleFile object.
         *
         * @param fileName
         *            file name;
         * @return parsed SubtitleFile.
         */
        public SubtitleApi parseSubtitleFile (String fileName)
        throws Exception {
            SubtitleParser sp = null;
            String input = null;

            String encoding = getCharset();
            SUBTYPE type = getSubType();
            Log.i ("SubtitleFile", "type=" + type.toString() + "   encoding=" + encoding);

            switch (type) {
                    //      case TYPE_SRT:
                    //          Log.i("SubtitleFile", "------------TYPE_SRT-----------"+fileName );
                    //          input = FileIO.file2string(fileName, encoding);
                    //          sp = new SrtParser();
                    //          break;
                    //      case TYPE_SUB:
                    //          input = FileIO.file2string(fileName, encoding);
                    //          sp = new SubParser();
                    //          break;
                case SUB_SSA:
                    input = FileIO.file2string (fileName, encoding);
                    sp = new SsaParser();
                    return sp.parse (input, index);
                case SUB_LRC:
                    Log.i ("SubtitleFile", "--LrcSubParser--:" + fileName);
                    input = FileIO.file2string (fileName, encoding);
                    sp = new LrcSubParser();
                    return sp.parse (input, index);
                case SUB_XML:
                    Log.i ("SubtitleFile", "--XmlSubParser--:" + fileName);
                    //input = FileIO.file2string(fileName, encoding);
                    sp = new XmlSubParser (encoding);
                    return sp.parse (fileName, index);
                case SUB_XML2:
                    Log.i ("SubtitleFile", "--Xml2SubParser--:" + fileName);
                    //input = FileIO.file2string(fileName, encoding);
                    sp = new Xml2SubParser (encoding);
                    return sp.parse (fileName, index);
                case SUB_SAMI:
                    //          input = FileIO.file2string(fileName, encoding);
                    //          sp=new SamiParser();
                    //          return sp.parse(input);
                case SUB_MICRODVD:
                case SUB_SUBRIP:
                case SUB_SUBVIEWER:
                case SUB_VPLAYER :
                case SUB_RT   :
                case SUB_PJS   :
                case SUB_MPSUB   :
                case SUB_AQTITLE   :
                case SUB_SUBVIEWER2 :
                case SUB_SUBVIEWER3 :
                case SUB_SUBRIP09  :
                case SUB_JACOSUB  :
                case SUB_MPL1  :
                case SUB_MPL2  :
                case SUB_DIVX:
                    Log.i ("SubtitleFile", "------------parseSubtitleFileByJni-----------" + fileName);
                    sp = new TextSubParser();
                    return sp.parse (fileName, encoding);
                case SUB_IDXSUB:
                    sp = new IdxSubParser();
                    return sp.parse (fileName, index);
                case INSUB:
                    sp = new InSubParser();
                    return sp.parse (fileName, index);
                default:
                    sp = null;
                    throw new Exception ("Unknown File Extension.");
            }
        }



        /**
         * @return the file type analyzing only the extension.
         */
        public static SUBTYPE fileType (String file, String encoding) {
            if (file.endsWith (".idx")) {
                return SUBTYPE.SUB_IDXSUB;
            }
            if (file.compareTo ("INSUB") == 0) {
                return SUBTYPE.INSUB;
            }
            return FileIO.dectFileType (file, encoding);
        }

        public String getFont() {
            return FileIO.getFont();
        }

        private static String checkEncoding (String fileName, String enc) {
            if ("CP1256".equals(enc)) {
                return "CP1256";
            }
            BufferedInputStream bis = null;
            FileInputStream fin = null;
            byte[] first3Bytes = new byte[3];
            String charset = enc;
            int idx = fileName.indexOf ("INSUB");
            Log.i ("subtitle", "[checkEncoding]fileName:" + fileName + ",idx:" + idx);
            if (idx != -1) {
                return charset;
            }
            try {
                fin = new FileInputStream (new File (fileName));
                bis = new BufferedInputStream (fin);
                bis.mark (0);
                int r = bis.read (first3Bytes, 0, 3) ;
                if (r == -1) {
                    if (fin != null) {
                        fin.close();
                    }
                    if (bis != null) {
                        bis.close();
                    }
                    return charset;
                }
                if (first3Bytes[0] == (byte) 0xFF && first3Bytes[1] == (byte) 0xFE) {
                    charset = "UTF-16LE";
                }
                else if (first3Bytes[0] == (byte) 0xFE
                         && first3Bytes[1] == (byte) 0xFF) {
                    charset = "UTF-16BE";
                }
                else if (first3Bytes[0] == (byte) 0xEF
                         && first3Bytes[1] == (byte) 0xBB
                         && first3Bytes[2] == (byte) 0xBF) {
                    charset = "UTF8";
                }
                else {
                    if (1 > 0) { //whether need to probe the file charset
                        String probe = probe_utf8_utf16 (fileName, 1024);
                        if (probe != "") {
                            charset = probe;
                        }
                    }
                }
                if (fin != null) {
                    fin.close();
                }
                if (bis != null) {
                    bis.close();
                }
            }
            catch (FileNotFoundException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            Log.i ("checkEncoding", "--filename:--" + fileName + "-charset:" + charset);
            return charset;
        }


        private static String probe_utf8_utf16 (String fileName, int size) {
            char probe_find_time = 10;
            int utf8_count = 0;
            int little_utf16 = 0;
            int big_utf16 = 0;
            int i = 0;
            byte[] probeBytes = new byte[size];
            BufferedInputStream bis = null;
            FileInputStream fin = null;
            int r = 0;
            try {
                fin = new FileInputStream (new File (fileName));
                bis = new BufferedInputStream (fin);
                bis.mark (0);
                r = bis.read (probeBytes, 0, size);
                if (fin != null) {
                    fin.close();
                }
                if (bis != null) {
                    bis.close();
                }
            }
            catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            if (r == -1) {
                return "";
            }
            for (i = 0; i < (size - 5); i++) {
                if (probeBytes[i] == 0 && (probeBytes[i + 1] > 0x20 && probeBytes[i + 1] < 0x7d)) {
                    i++;
                    big_utf16++;
                }
                else if (probeBytes[i + 1] == 0 && (probeBytes[i] > 0x20 && probeBytes[i] < 0x7d)) {
                    i++;
                    little_utf16++;
                }
                else if ( ( (probeBytes[i] & 0xE0) == 0xE0) && ( (probeBytes[i + 1] & 0xC0) == 0x80) && ( (probeBytes[i + 2] & 0xC0) == 0x80)) {
                    i += 2;
                    utf8_count++;
                    if ( ( ( (probeBytes[i + 1] & 0x80) == 0x80) && ( (probeBytes[i + 1] & 0xE0) != 0xE0)) ||
                            ( ( (probeBytes[i + 2] & 0x80) == 0x80) && ( (probeBytes[i + 2] & 0xC0) != 0x80)) ||
                            ( ( (probeBytes[i + 3] & 0x80) == 0x80) && ( (probeBytes[i + 3] & 0xC0) != 0x80))) {
                        return "";
                    }
                }
                if (big_utf16 >= probe_find_time) {
                    return "UTF-16BE";
                }
                else if (little_utf16 >= probe_find_time) {
                    return "UTF-16LE";
                }
                else if (utf8_count >= probe_find_time) {
                    return "UTF-8";
                }
            }
            if (i == (size - 2)) {
                if (big_utf16 > 0) {
                    return "UTF-16BE";
                }
                else if (little_utf16 > 0) {
                    return "UTF-16LE";
                }
                else if (utf8_count > 0) {
                    return "UTF-8";
                }
            }
            return "";
        }

        /**
         * Simple integer formatter.
         *
         * @param n
         *            integer to format;
         * @param chars
         *            number of char on which represent n;
         * @return n on chars characters.
         */
        public static String format (int n, int chars) {
            NumberFormat numberFormatter;
            numberFormatter = NumberFormat.getNumberInstance();
            numberFormatter.setMinimumIntegerDigits (chars);
            numberFormatter.setMaximumIntegerDigits (chars);
            return numberFormatter.format (n);
        }

        /**
         * Parse shift time.
         *
         * @param shift
         *            String that contains a representation of the shift in seconds
         *            (e.g. "12","13.5");
         * @return milliseconds
         */
        public static int parseShiftTime (String shift) throws NumberFormatException {

            String tmp[] = shift.split ("\\.");

            if (tmp.length == 1) {
                return Integer.parseInt (tmp[0]) * 1000;
            }
            if (tmp.length == 2) {
                int dec = Integer.parseInt (tmp[1]);
                if ( (dec > 999) || (dec < 0)) {
                    throw new NumberFormatException ("Unpermitted shift value.");
                }
                if ( (dec > 0) && (dec <= 9)) {
                    dec = dec * 100;
                } // 1-9
                else {
                    if ( (dec >= 10) && (dec <= 99)) {
                        dec = dec * 10;
                    }// 10-99
                }
                if (Integer.parseInt (tmp[0]) < 0) {
                    return Integer.parseInt (tmp[0]) * 1000 - dec;
                }
                else {
                    return Integer.parseInt (tmp[0]) * 1000 + dec;
                }
            }
            throw new NumberFormatException ("Unpermitted shift value.");
        }

        /**
         * Remove hearing impaired subtitles.
         *
         * @param text
         *            with hearing impaired subtitles'
         * @param start
         *            char (e.g. '[');
         * @param end
         *            char (e.g. ']');
         * @return text without Hearing Impaired Subtitles
         */
        public static String removeHearImp (String text, String start, String end) {
            String res = text;
            Pattern p = Pattern.compile ("\\" + start + ".*?" + "\\" + end);
            Matcher m = p.matcher (res);
            while (m.find()) {
                res = res.substring (0, m.start())
                      + res.substring (m.end(), res.length());
                m = p.matcher (res);
            }
            return res;
        }

        /**
         * Frame/MilliSec Converter.
         *
         * @param frames
         *            n of frames
         * @param framerate
         *            framerate (frames per sec)
         * @return milliseconds
         */
        public static int frame2mil (int frames, float framerate) throws Exception {
            if (framerate <= 0)
                throw new Exception (
                    "frame2mil I need a positive framerate to perform this conversion!");

            Float fl = new Float (frames / framerate * 1000);
            return fl.intValue();
        }

        /**
         * MilliSec/Frame Converter.
         *
         * @param millisec
         *            n of millisec
         * @param framerate
         *            framerate (frames per sec)
         * @return frames
         */
        public static int mil2frame (int millisec, float framerate) throws Exception {
            if (framerate <= 0)
                throw new Exception (
                    " mil2frame I need a positive framerate to perform this conversion!");

            millisec = millisec / 1000;
            Float fl = new Float (millisec * framerate);
            return fl.intValue();
        }

        // Makes a non-format specific SubtitleFile
        public static SubtitleFile fillValues (SubtitleFile sf, float framerate)
        throws Exception {

        for (Object x : sf) {
                ( (SubtitleLine) x).getBegin().setAllValues (framerate);
                ( (SubtitleLine) x).getEnd().setAllValues (framerate);
            }

            return sf;
        }
}
