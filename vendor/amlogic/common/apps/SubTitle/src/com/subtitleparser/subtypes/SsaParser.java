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


class SsaApi extends SubtitleApi {
        private SubtitleFile SubFile = null;
        private SubtitleLine cur = null;
        private String st = null;
        private boolean isSubOverlap = false;

        public SsaApi (SubtitleFile sf) {
            SubFile = sf;
            isSubOverlap = checkSubOverlap (sf);
        }
        public void closeSubtitle() {}
        public int getSubTypeDetial() {
            return -1;
        }
        public Subtitle.SUBTYPE type() {
            return Subtitle.SUBTYPE.SUB_SSA;
        }

        private boolean checkSubOverlap (SubtitleFile sf) {
            boolean ret = false;
            SubtitleLine sli = null;
            SubtitleLine slj = null;
            //int beginTimei = -1;
            int beginTimej = -1;
            int endTimei = -1;
            //int endTimej = -1;
            try {
                for (int i = 0; i < sf.size(); i++) {
                    sli = (SubtitleLine) sf.get (i);
                    endTimei = sli.getEnd().getMilValue();
                    slj = (SubtitleLine) sf.get (i + 1);
                    beginTimej = slj.getBegin().getMilValue();
                    if (endTimei > beginTimej) {
                        ret = true;
                        break;
                    }
                    /*for (int j=i+1;j< sf.size();j++) {
                        slj = (SubtitleLine) sf.get(j);
                        beginTimej = slj.getBegin().getMilValue();
                        if (endTimei > beginTimej) {
                            ret = true;
                        }
                        else {
                            break;
                        }
                    }*/
                }
            }
            catch (Exception e) {
                e.printStackTrace();
            }
            return ret;
        }

        public SubData getdata (int millisec) {
            try {
                if (false == isSubOverlap) {
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
                }
                else {
                    cur = SubFile.curSubtitle();
                    SubFile.matchSubtitles (millisec);
                    String str = null;
                    st = "";
                    for (int i = 0; i < SubFile.idxlistSize(); i++) {
                        int idx = SubFile.getIdx (i);
                        str = SubFile.getSubtitle (idx).getText();
                        if (st != null) {
                            st = st + "\\\n" + str;
                        }
                        else {
                            st = str;
                        }
                    }
                    st=st.replaceAll( ".*\\{(.*?)\\}","");
                    st = st.replaceAll ("\\{(.*?)\\}", "");
                    st = st.replaceAll ("\\\\N", "\\\n");
                    st = st.replaceAll ("\\\\h", "");
                    st = st.replaceAll ("\\\\N", "");
                    st = st.replaceAll ("\\\\", "");
                    st = st.replaceAll ("\\{\\\\fad.*?\\}", "");
                    st = st.replaceAll ("\\{\\\\be.*?\\}", "");
                    return new SubData (st, 0, 0);
                }
                if (st.compareTo ("") != 0) {
                    st = st.replaceAll ("\\{(.*?)\\}", "");
                    st = st.replaceAll ("\\\\N", "\\\n");
                    return new SubData (st, cur.getBegin().getMilValue(), cur.getEnd().getMilValue());
                }
                else {
                    return new SubData (st, millisec, millisec + 30);
                }
            }
            catch (Exception e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
            return null;
        }

}

/**
* a .SUB subtitle parser.
*
* @author jeff.yang
*/
public class SsaParser implements SubtitleParser {

        public SubtitleApi parse (String inputString, int index) throws MalformedSubException {
            try{
                String n = "\\" + System.getProperty ("line.separator");
                String tmpText = "";
                SubtitleFile sf = new SubtitleFile();
                SubtitleLine sl = null;

                //SSA regexp
                Pattern p = Pattern.compile (
                    "Dialogue:[^,]*,\\s*" + "(\\d):(\\d\\d):(\\d\\d).(\\d\\d)\\s*,\\s*"
                    + "(\\d):(\\d\\d):(\\d\\d).(\\d\\d)" +
                    "[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*,[^,]*," +
                    "(.*?)" + n
                );

                Matcher m = p.matcher (inputString);

                int occ = 0;
                while (m.find()) {
                    occ++;
                    //              String tmp=m.group(9).replaceAll("\\{.*?\\}", "");
                    boolean shield = false;
                    if (m.group (9).indexOf ("pos") >= 0) {
                        int idx = m.group (9).indexOf ("}");
                        String tmp = m.group (9).substring (idx + 1);
                        if (tmp.startsWith ("m")) {
                            shield = true;
                        }
                    }
                    if ( (m.group (9).startsWith ("{\\pos(")) || shield) {
                        shield = false;
                        sl = new SubtitleLine (occ,
                                               new SubtitleTime (Integer.parseInt (m.group (1)), //start time
                                                       Integer.parseInt (m.group (2)),
                                                       Integer.parseInt (m.group (3)),
                                                       Integer.parseInt (m.group (4))),
                                               new SubtitleTime (Integer.parseInt (m.group (5)), //end time
                                                       Integer.parseInt (m.group (6)),
                                                       Integer.parseInt (m.group (7)),
                                                       Integer.parseInt (m.group (8))),
                                               "" //text = null
                                              );
                    }
                    else {
                        String tmp = m.group (9).replaceAll ("\\<font.*?\\>", "");
                        sl = new SubtitleLine (occ,
                                               new SubtitleTime (Integer.parseInt (m.group (1)), //start time
                                                       Integer.parseInt (m.group (2)),
                                                       Integer.parseInt (m.group (3)),
                                                       Integer.parseInt (m.group (4))),
                                               new SubtitleTime (Integer.parseInt (m.group (5)), //end time
                                                       Integer.parseInt (m.group (6)),
                                                       Integer.parseInt (m.group (7)),
                                                       Integer.parseInt (m.group (8))),
                                               tmp //text
                                              );
                    }
                    tmpText = "";
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
                Log.i ("SsaParser", "find" + sf.size());
                return new SsaApi (sf);
            }
            catch (Exception e) {
                Log.i ("SsaParser", "------------!!!!!!!parse file err!!!!!!!!");
                throw new MalformedSubException (e.toString());
            }
        };
        public SubtitleApi parse (String inputString, String st2) throws MalformedSubException {
            return null;
        };

}
