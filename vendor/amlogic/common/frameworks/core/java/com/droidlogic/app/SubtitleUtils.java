/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.app;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.io.BufferedInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.*;
import android.util.Log;
import android.os.SystemProperties;

public class SubtitleUtils {
        /*public native int getInSubtitleTotalByJni();
        public native String getInSubtitleTitleByJni();
        public native String getInSubtitleLanguageByJni();
        public native String getInSubTypeStrByJni();
        public native String getInSubLanStrByJni();

        //public native int setInSubtitleNumberByJni(int  ms);
        public native void setSubtitleNumberByJni (int  idx);
        public native int getCurrentInSubtitleIndexByJni();
        public native void nativeDumpByJni (int fd);*/
        //  public native void FileChangedByJni(String name);

        private String mFileName = null;
        private File mSubfile = null;
        private List<SubID> mStrlist = new ArrayList<SubID>();
        private int mExSubtotle = 0;
        private boolean supportLrc = true;//false; //lrc support

        private String mSuffixExtSubtitleType = "";
        private String mSuffixInSubtitleType = "";

        private String mCharset = null;
        private static String systemCharset = "GBK";

        public static final String[] extensions = {
            "txt",
            "srt",
            "smi",
            "sami",
            "rt",
            "ssa",
            "ass",
            "lrc",
            "xml",
            "idx",
            "sub",
            "pjs",
            "aqt",
            "mpl",
            "vtt",
            "js",
            "jss",
            /* "may be need add new types--------------" */
        };
        public  SubtitleUtils() {
        }

        public  SubtitleUtils (String name) {
            setFileName (name);
        }

        public void setFileName (String name) {
            if (mFileName != name) {
                mStrlist.clear();
                mFileName = name;
                mSubfile = new File (mFileName);
                if (SystemProperties.getBoolean ("sys.extSubtitle.enable", true) && !name.startsWith ("/data/")) {
                    accountExSubtitleNumber();
                }
            }
        }

        public int getExSubTotal() {
            return mExSubtotle;
        }

        public int getSubTotal() {
            for (int i = 0; i < mExSubtotle; i++) {
                Log.v ("Subfile list ", i + ":" + getSubPath (i) );
            }
            return mExSubtotle + accountInSubtitleNumber();
        }

        public int getInSubTotal() {
            return accountInSubtitleNumber();
        }
        public String getSubPath (int index) {
            if (mSubfile == null) {
                return null ;
            }
            if (index < getInSubTotal() ) {
                return "INSUB";
            } else if (index < (mExSubtotle + accountInSubtitleNumber() ) ) {
                return mStrlist.get (index - getInSubTotal() ).mFileName;
            }
            return null;
        }

        public String getInSubPath (int idx) {
            String titleTmp = getInSubtitleTitle();

            if (titleTmp != null) {
                String[] title = titleTmp.split (";");
                if (idx < title.length) {
                    return title[idx];
                }
            }

            return null;
        }
        // inner subtitle language and title string format:
        //1,eng;2,chi;
        //1,simplified chinese;2,english;3,xxx;
        private String parseInSubStr (int index, String string) {
            int idx = 0;
            String str = string;

            if (index > getInSubTotal() ) {
                return null;
            }

            for (int i = 0; i < index; i++) {
                idx = str.indexOf (";");
                if (idx >= 0) {
                    str = str.substring (idx + 1);
                }
            }
            idx = str.indexOf (";");
            if (idx >= 0) {
                str = str.substring (0, idx);
            }

            idx = str.indexOf (",");
            if (idx >= 0) {
                str = str.substring (idx + 1);
            }

            return str;
        }

        public String getInSubName (int index) {
            int idx = 0;
            String name = null;
            String str = getInSubtitleTitle();

            if (str != null) {
                name = parseInSubStr (index, str);
            }

            return name;
        }

        public String getInSubLanguage (int index) {
            int idx = 0;
            String language = null;
            String str = null;

            if (getInSubTotal() > 0) {
                str = getInSubtitleLanguage();
            }

            language = parseInSubStr (index, str);

            return language;
        }

        public SubID getSubID (int index) {
            if (index < getInSubTotal() ) {
                return new SubID ("INSUB", index);
            } else if (index < getSubTotal() ) {
                return mStrlist.get (index - getInSubTotal() );
            }
            return null;
        }
        private void  accountExSubtitleNumber() {
            String tmp = mSubfile.getName();
            String prefix = tmp.substring (0, tmp.lastIndexOf ('.') + 1);
            Log.i ("SubtitleUtils","[accountExSubtitleNumber]prefix:" + prefix);
            File DirFile = mSubfile.getParentFile();
            int idxindex = 0;
            boolean skipLrc = false;
            int exSubIndex = getInSubTotal();
            if (DirFile.isDirectory() ) {
                for (String file : DirFile.list() ) {
                    if ( (file.toLowerCase() ).startsWith (prefix.toLowerCase() ) ) {
                        for (String ext : extensions) {
                            if (file.toLowerCase().endsWith (ext) ) {
                                if (supportLrc == true) {
                                    skipLrc = false;
                                } else {
                                    skipLrc = file.toLowerCase().endsWith ("lrc"); //shield lrc file
                                }
                                if (!skipLrc) {
                                    mStrlist.add (new SubID (DirFile.getAbsolutePath() + "/" + file, exSubIndex) );
                                    exSubIndex++;
                                }
                                break;
                            }
                        }
                    }
                }
                for (SubID file : mStrlist) {
                    if (file.mFileName.toLowerCase().endsWith ("idx") ) {
                        mStrlist.remove (idxindex);
                        Log.v ("before: ", "" + file);
                        String st = file.mFileName.substring (0, file.mFileName.length() - 3);
                        for (int i = 0; i < mStrlist.size(); i++) {
                            if (mStrlist.get (i).mFileName.toLowerCase().endsWith ("sub") &&
                                    mStrlist.get (i).mFileName.startsWith (st) &&
                                    mStrlist.get (i).mFileName.length() == file.mFileName.length() ) {
                                Log.v ("accountExSubtitleNumber: ", "clear " + mStrlist.get (i) );
                                mStrlist.remove (i);
                            }
                        }
                        //accountIdxSubtitleNumber (file.mFileName);
                        mExSubtotle = mStrlist.size();
                        break;
                    } else {
                        idxindex++;
                    }
                }
            }
            mExSubtotle = mStrlist.size();
            Log.i("SubtitleUtils","accountExSubtitleNumber" + mExSubtotle);
            String suffix = null;
            for (int i = 0; i < mExSubtotle; i++) {
                suffix = mStrlist.get(i).mFileName.substring(mStrlist.get(i).mFileName.lastIndexOf ('.')+1, mStrlist.get(i).mFileName.length());
                suffix = suffix + ",";
                mSuffixExtSubtitleType += suffix;
                Log.i("SubtitleUtils","accountExSubtitleNumber suffix:" + mSuffixExtSubtitleType);
            }
        }

        public String getExtSubTypeStrAll() {
            //mSuffixInSubtitleType = getInSubTypeStrByJni();
            if (mSuffixExtSubtitleType != null && !mSuffixExtSubtitleType.equals("")) {
                return mSuffixExtSubtitleType;
            }
            return null;
        }

        public String getInSubLanStr() {
            return null;//getInSubLanStrByJni();
        }

        public String getInBmpTxtType() {
            /*mSuffixInSubtitleType = getInSubTypeStrByJni();
            //Log.i("SubtitleUtils","getSubTypeStrAll mSuffixInSubtitleType:" + mSuffixInSubtitleType + ",mSuffixExtSubtitleType:" + mSuffixExtSubtitleType);
            if (mSuffixInSubtitleType.length() > 0) {
                String[] typeStrArray = mSuffixInSubtitleType.split(",");
                String bmpTxtType = "";
                int type = -1;
                for (int i = 0; i < typeStrArray.length; i++) {
                    if ((type = isInBmpTxtType(typeStrArray[i])) != -1) {
                        bmpTxtType += type;
                        bmpTxtType = bmpTxtType + ",";
                    }
                }
                return bmpTxtType;
            }*/
            return null;
        }

        public String getExtBmpTxtType() {
            if (mSuffixExtSubtitleType.length() > 0) {
                String[] typeStrArray = mSuffixExtSubtitleType.split(",");
                String bmpTxtType = "";
                for (int i = 0; i < typeStrArray.length; i++) {
                    bmpTxtType += isExtBmpTxtType(typeStrArray[i]);
                    bmpTxtType = bmpTxtType + ",";
                }
                return bmpTxtType;
            }
            return null;
        }

        public int isInBmpTxtType(String subType) {
            Log.i("SubtitleUtils","isInBmpTxtType subType:" + subType);
            if (subType != null) {
                if (subType.equals("ssa") || subType.equals("subrip") ) {
                    return 0;
                } else if (subType.equals("teletext") || subType.equals("dvb") || subType.equals("pgs") || subType.equals("dvd")) {
                    return 1;
                }
            }
            return -1;
        }

        public int isExtBmpTxtType(String subType) {
            Log.i("SubtitleUtils","isExtBmpTxtType subType:" + subType);
            if (subType != null) {
                if (subType.equals("idx")) {
                    return 1;
                } else {
                    return 0;
                }
            }
            return -1;
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

         public String checkEncoding (String fileName, String defaultEncoder) {
             if ("CP1256".equals(defaultEncoder)) {
                 return "CP1256";
             }
             BufferedInputStream bis = null;
             FileInputStream fin = null;
             byte[] first3Bytes = new byte[3];
             String charset = defaultEncoder;
             int idx = fileName.indexOf ("INSUB");
             //Log.i ("subtitle", "[checkEncoding]fileName:" + fileName + ",idx:" + idx);
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
                     if (1 < 0) { //whether need to probe the file charset
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
             //Log.i ("checkEncoding", "--filename:--" + fileName + "-charset:" + charset);
             return charset;
         }

         public void setSystemCharset (String st) {
             systemCharset = st;
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

         public String getExtSubCharset() {
             if (mFileName != null) {
                 if (mCharset == null) {
                     mCharset = checkEncoding (mFileName , systemCharset);
                 }
             } else {
                 return null;
             }
             return mCharset;
         }


        //wait to finish.
        private  int  accountInSubtitleNumber() {
            return 0;//getInSubtitleTotalByJni();
        }


        private String getInSubtitleTitle() {
            return null;//getInSubtitleTitleByJni();
        }

        private String getInSubtitleLanguage() {
            return null;//getInSubtitleLanguageByJni();
        }

        //wait to finish.
        public void setInSubtitleNumber (int index) {
            //setInSubtitleNumberByJni(index);
            return;
        }
        public void setSubtitleNumber (int index) {
            //setSubtitleNumberByJni (index);
            return;
        }

        public void nativeDump (int fd) {
            //nativeDumpByJni (fd);
            return;
        }

        /*private int accountIdxSubtitleNumber (String filename) {
            int idxcount = 0;
            String inputString = null;
            try {
                inputString = FileIO.file2string (filename, "GBK");
            } catch (IOException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
                return idxcount;
            }
            String n = "\\" + System.getProperty ("line.separator");
            Pattern p = Pattern.compile ("id:(.*?),\\s*" + "index:\\s*(\\d*)");
            Matcher m = p.matcher (inputString);
            while (m.find() ) {
                idxcount++;
                Log.v ("accountIdxSubtitleNumber", "id:" + m.group (1) + " index:" + m.group (2) );
                mStrlist.add (new SubID (filename, Integer.parseInt (m.group (2) ) ) );
            }
            return idxcount;
        }*/
}

class SubID {
        public SubID (String file, int i) {
            mFileName = file;
            mIndex = i;
        }
        public String mFileName;
        public int mIndex;
}

