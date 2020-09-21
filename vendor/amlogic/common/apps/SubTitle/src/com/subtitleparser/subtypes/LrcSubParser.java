/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser.subtypes;

import java.util.regex.Matcher;
import java.util.regex.Pattern;
import android.util.Log;
import com.subtitleparser.MalformedSubException;
import com.subtitleparser.SubData;
import com.subtitleparser.Subtitle;
import com.subtitleparser.SubtitleFile;
import com.subtitleparser.SubtitleLine;
import com.subtitleparser.SubtitleParser;
import com.subtitleparser.SubtitleTime;
import com.subtitleparser.SubtitleApi;

class LrcSubApi extends SubtitleApi {
        private static final String TAG = "LrcSubApi";
        private SubtitleFile SubFile = null;
        private SubtitleLine cur = null;
        private String st = null;

        public LrcSubApi (SubtitleFile sf) {
            SubFile = sf;
        }
        public void closeSubtitle() {}
        public int getSubTypeDetial() {
            return -1;
        }
        public Subtitle.SUBTYPE type() {
            return Subtitle.SUBTYPE.SUB_LRC;
        }

        public SubData getdata (int millisec) {
            try {
                cur = SubFile.curSubtitle();
                if (millisec >= cur.getBegin().getMilValue()
                        && millisec <= cur.getEnd().getMilValue()) {
                    st = SubFile.curSubtitle().getText();
                }
                else {
                    SubFile.matchSubtitle (millisec);
                    cur = SubFile.curSubtitle();
                    if (millisec >= cur.getBegin().getMilValue()
                            && millisec <= cur.getEnd().getMilValue()) {
                        st = SubFile.curSubtitle().getText();
                    }
                    else if (millisec < cur.getBegin().getMilValue()) {
                        st = "";
                    }
                    else {
                        SubFile.toNextSubtitle();
                        st = "";
                    }
                }
                if (st.compareTo ("") != 0) {
                    return new SubData (st, cur.getBegin().getMilValue(), cur.getEnd().getMilValue());
                }
                else {
                    return new SubData (st, millisec, millisec + 30);
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            return null;
        }
}

/**
* LrcSub subtitle parser.
*
* @author xiaoliang.wang
*/
public class LrcSubParser implements SubtitleParser {
        private static final String TAG = "LrcSubApi-LrcSubParser";
        private boolean DEBUG = false;
        public SubtitleApi parse (String inputString, int index) throws MalformedSubException {
            try{
                int h = 0;
                int min = 0;
                int sec = 0;
                int mil = 0;
                int hPre = 0;
                int minPre = 0;
                int secPre = 0;
                int milPre = 0;
                String subStr = null;
                String subStrPre = null;
                int occ = 0;
                SubtitleFile sf = new SubtitleFile();
                SubtitleLine sl = null;
                String n = "\\" + System.getProperty ("line.separator");

                //LRC regexp
                Pattern p = Pattern.compile ("\\[(\\d+):(\\d+).(\\d+)\\]" + "(.*?)" + n);
                Matcher m = p.matcher (inputString);
                Log.i (TAG, "[LrcSubParser]parse m:" + m);

                while (m.find()) {
                    min = Integer.parseInt (m.group (1));
                    sec = Integer.parseInt (m.group (2));
                    mil = Integer.parseInt (m.group (3));
                    h = min / 60;
                    if (h > 0) {
                        min = min - h * 60;
                    }
                    subStr = m.group (4);
                    //Log.i(TAG,"[LrcSubParser]parse 0occ:"+occ);
                    if (occ == 0) {
                        hPre = h;
                        minPre = min;
                        secPre = sec;
                        milPre = mil;
                        subStrPre = subStr;
                        occ++;
                        //Log.i(TAG,"occ=0;hPre:"+hPre+",minPre:"+minPre+",secPre:"+secPre+",milPre:"+milPre+",subStrPre:"+subStrPre);
                        continue;
                    }
                    else {
                        if (DEBUG) { Log.i (TAG, "occ:" + occ + ",start time:(" + hPre + ":" + minPre + ":" + secPre + ":" + milPre + "),end time:(" + h + ":" + min + ":" + sec + ":" + mil + "),subStrPre:" + subStrPre); }
                        sl = new SubtitleLine (occ,
                                               new SubtitleTime (hPre, minPre, secPre, milPre), //start time
                                               new SubtitleTime (h, min, sec, mil), //end time
                                               subStrPre //text
                                              );
                        hPre = h;
                        minPre = min;
                        secPre = sec;
                        milPre = mil;
                        subStrPre = subStr;
                    }
                    occ++;
                    //Log.i(TAG,"[LrcSubParser]parse 1occ:"+occ);
                    if (sf.size() == 0) {
                        sf.add (sl);
                    }
                    else if (sl.getBegin().getMilValue() > ( (SubtitleLine) sf.get (sf.size() - 1)).getBegin().getMilValue()) {
                        sf.add (sl);
                    }
                    else {
                        sf.addSubtitleLine (sl);
                    }
                }

                //add the last line
                occ++;
                if (DEBUG) { Log.i (TAG, "last line occ:" + occ + ",start time:(" + hPre + ":" + minPre + ":" + secPre + ":" + milPre + "),end time:(" + h + ":" + min + ":" + (sec + 1) + ":" + mil + "),subStrPre:" + subStrPre); }
                sl = new SubtitleLine (occ,
                                       new SubtitleTime (hPre, minPre, secPre, milPre), //start time
                                       new SubtitleTime (h, min, sec + 1, mil), //end time
                                       subStrPre //text
                                      );
                if (sf.size() == 0) {
                    sf.add (sl);
                }
                else if (sl.getBegin().getMilValue() > ( (SubtitleLine) sf.get (sf.size() - 1)).getBegin().getMilValue()) {
                    sf.add (sl);
                }
                else
                { sf.addSubtitleLine (sl); }
                Log.i (TAG, "find" + sf.size());
                return new LrcSubApi (sf);
            }
            catch (Exception e) {
                Log.i (TAG, "------------!!!!!!!LrcSub parse file err!!!!!!!!");
                throw new MalformedSubException (e.toString());
            }
        };

        public SubtitleApi parse (String inputString, String st2) throws MalformedSubException {
            return null;
        };
}
