/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser;
import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import android.util.Log;
import android.os.SystemProperties;

public class SubtitleUtils {
        public native int getInSubtitleTotalByJni();
        public native String getInSubtitleTitleByJni();
        public native String getInSubtitleLanguageByJni();
        public native String getInSubTypeStrByJni();
        public native String getInSubLanStrByJni();

        //public native int setInSubtitleNumberByJni(int  ms);
        public native void setSubtitleNumberByJni (int  idx);
        public native int getCurrentInSubtitleIndexByJni();
        public native void nativeDumpByJni (int fd);
        //  public native void FileChangedByJni(String name);

        private String filename = null;
        private File subfile = null;
        //  private List<String> strlist = new ArrayList<String>();
        private List<SubID> strlist = new ArrayList<SubID>();
        private int exSubtotle = 0;
        private boolean supportLrc = true;//false; //lrc support

        private String suffixExtSubtitleType = "";
        private String suffixInSubtitleType = "";

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
            if (filename != name) {
                strlist.clear();
                filename = name;
                //          FileChangedByJni(name);
                subfile = new File (filename);
                if (SystemProperties.getBoolean ("sys.extSubtitle.enable", true) && !name.startsWith ("/data/")) {
                    accountExSubtitleNumber();
                }
            }
        }

        public int getExSubTotal() {
            return exSubtotle;
        }

        public int getSubTotal() {
            for (int i = 0; i < exSubtotle; i++) {
                Log.v ("Subfile list ", i + ":" + getSubPath (i) );
            }
            return exSubtotle + accountInSubtitleNumber();
        }

        public int getInSubTotal() {
            return accountInSubtitleNumber();
        }
        public String getSubPath (int index) {
            if (subfile == null) {
                return null ;
            }
            if (index < getInSubTotal() ) {
                return "INSUB";
            } else if (index < (exSubtotle + accountInSubtitleNumber() ) ) {
                return strlist.get (index - getInSubTotal() ).filename;
            }
            return null;
            /*if(index<exSubtotle)
                return strlist.get(index).filename;
            else if(index<getSubTotal())
            {
            //           setInSubtitleNumber(0xff);
            //           setInSubtitleNumber(index-exSubtotle);
                return "INSUB";
            }
            return null;*/
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
            //if (subfile == null) {
                //return null ;
            //}
            if (index < getInSubTotal() ) {
                return new SubID ("INSUB", index);
            } else if (index < getSubTotal() ) {
                return strlist.get (index - getInSubTotal() );
            }
            return null;
            /*
            if (index < exSubtotle)
                return strlist.get(index);
            else if (index < getSubTotal())
            {
            //           setInSubtitleNumber(0xff);
            //           setInSubtitleNumber(index-exSubtotle);
                return new SubID("INSUB",index-exSubtotle);
            }
            return null; */
        }
        private void  accountExSubtitleNumber() {
            String tmp = subfile.getName();
            String prefix = tmp.substring (0, tmp.lastIndexOf ('.') + 1);
            Log.i ("SubtitleUtils","[accountExSubtitleNumber]prefix:" + prefix);
            File DirFile = subfile.getParentFile();
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
                                    strlist.add (new SubID (DirFile.getAbsolutePath() + "/" + file, exSubIndex) );
                                    exSubIndex++;
                                }
                                break;
                            }
                        }
                    }
                }
for (SubID file : strlist) {
                    if (file.filename.toLowerCase().endsWith ("idx") ) {
                        strlist.remove (idxindex);
                        Log.v ("before: ", "" + file);
                        String st = file.filename.substring (0, file.filename.length() - 3);
                        for (int i = 0; i < strlist.size(); i++) {
                            if (strlist.get (i).filename.toLowerCase().endsWith ("sub") &&
                                    strlist.get (i).filename.startsWith (st) &&
                                    strlist.get (i).filename.length() == file.filename.length() ) {
                                Log.v ("accountExSubtitleNumber: ", "clear " + strlist.get (i) );
                                strlist.remove (i);
                            }
                        }
                        accountIdxSubtitleNumber (file.filename);
                        exSubtotle = strlist.size();
                        break;
                    } else {
                        idxindex++;
                    }
                }
            }
            exSubtotle = strlist.size();
            Log.i("SubtitleUtils","accountExSubtitleNumber" + exSubtotle);
            String suffix = null;
            for (int i = 0; i < exSubtotle; i++) {
                suffix = strlist.get(i).filename.substring(strlist.get(i).filename.lastIndexOf ('.')+1, strlist.get(i).filename.length());
                suffix = suffix + ",";
                suffixExtSubtitleType += suffix;
                Log.i("SubtitleUtils","accountExSubtitleNumber suffix:" + suffixExtSubtitleType);
            }
        }

        public String getExtSubTypeStrAll() {
            //suffixInSubtitleType = getInSubTypeStrByJni();
            if (suffixExtSubtitleType != null && !suffixExtSubtitleType.equals("")) {
                return suffixExtSubtitleType;
            }
            return null;
        }

        public String getInSubLanStr() {
            return getInSubLanStrByJni();
        }

        public String getInBmpTxtType() {
            suffixInSubtitleType = getInSubTypeStrByJni();
            //Log.i("SubtitleUtils","getSubTypeStrAll suffixInSubtitleType:" + suffixInSubtitleType + ",suffixExtSubtitleType:" + suffixExtSubtitleType);
            if (suffixInSubtitleType.length() > 0) {
                String[] typeStrArray = suffixInSubtitleType.split(",");
                String bmpTxtType = "";
                int type = -1;
                for (int i = 0; i < typeStrArray.length; i++) {
                    if ((type = isInBmpTxtType(typeStrArray[i])) != -1) {
                        bmpTxtType += type;
                        bmpTxtType = bmpTxtType + ",";
                    }
                }
                return bmpTxtType;
            }
            return null;
        }

        public String getExtBmpTxtType() {
            if (suffixExtSubtitleType.length() > 0) {
                String[] typeStrArray = suffixExtSubtitleType.split(",");
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

        //wait to finish.
        private  int  accountInSubtitleNumber() {
            return getInSubtitleTotalByJni();
        }


        private String getInSubtitleTitle() {
            return getInSubtitleTitleByJni();
        }

        private String getInSubtitleLanguage() {
            return getInSubtitleLanguageByJni();
        }

        //wait to finish.
        public void setInSubtitleNumber (int index) {
            //setInSubtitleNumberByJni(index);
            return;
        }
        public void setSubtitleNumber (int index) {
            setSubtitleNumberByJni (index);
            return;
        }

        public void nativeDump (int fd) {
            nativeDumpByJni (fd);
            return;
        }

        private int accountIdxSubtitleNumber (String filename) {
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
                strlist.add (new SubID (filename, Integer.parseInt (m.group (2) ) ) );
            }
            return idxcount;
        }
}

