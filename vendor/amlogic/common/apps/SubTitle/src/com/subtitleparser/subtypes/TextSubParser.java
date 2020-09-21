/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.subtitleparser.subtypes;

import android.util.Log;

import com.subtitleparser.MalformedSubException;
import com.subtitleparser.SubData;
import com.subtitleparser.SubtitleApi;
import com.subtitleparser.SubtitleFile;
import com.subtitleparser.SubtitleLine;
import com.subtitleparser.SubtitleParser;
import com.subtitleparser.Subtitle;


class TextSubApi extends SubtitleApi {
        private SubtitleFile SubFile = null;
        private SubtitleLine cur = null;
        private String st = null;
        private boolean isSubOverlap = false;
        public TextSubApi (SubtitleFile sf) {
            SubFile = sf;
            isSubOverlap = checkSubOverlap (sf);
        }
        public void closeSubtitle() {
        }
        public int getSubTypeDetial() {
            return -1;
        }
        public Subtitle.SUBTYPE type() {
            return Subtitle.SUBTYPE.SUB_COMMONTXT;
        }
        private boolean checkSubOverlap (SubtitleFile sf) {
            boolean ret = false;
            SubtitleLine sli = null;
            SubtitleLine slj = null;
            int beginTimej = -1;
            int endTimei = -1;
            try {
                for (int i = 0; i < sf.size() - 1; i++) {
                    sli = (SubtitleLine) sf.get (i);
                    endTimei = sli.getEnd().getMilValue();
                    slj = (SubtitleLine) sf.get (i + 1);
                    beginTimej = slj.getBegin().getMilValue();
                    if (endTimei > beginTimej) {
                        ret = true;
                        break;
                    }
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
                    Log.i("TextSubApi","[getdata]millisec:"+millisec+",cur.getBegin().getMilValue():"+cur.getBegin().getMilValue()+",cur.getEnd().getMilValue():"+cur.getEnd().getMilValue());
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
                    Log.i("TextSubApi","[getdata]st:"+st);
                }
                else {
                    cur = SubFile.curSubtitle();
                    SubFile.matchSubtitles (millisec);
                    String str = null;
                    st = null;
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
                    if (st == null) {
                        return null;
                    }
                    st = st.replaceAll ("\\{(.*?)\\}", "");
                    st = st.replaceAll ("\\\\N", "\\\n");
                    st = st.replaceAll ("\\\\h", "");
                    st = st.replaceAll ("\\\\N", "");
                    st = st.replaceAll ("\\\\", "");
                    st = st.replaceAll ("\\{\\\\fad.*?\\}", "");
                    st = st.replaceAll ("\\{\\\\be.*?\\}", "");
                    st = st.replaceAll ("\\{\\\\pos.*?\\}", "");
                    return new SubData (st, 0, 0);
                }
                if (st.compareTo ("") != 0) {
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
* a .SRT subtitle parser.
*
* @author
*/
public class TextSubParser implements SubtitleParser {

        public SubtitleApi parse (String filename, String encode) throws MalformedSubException {

            SubtitleFile file = new SubtitleFile();
            file = Subtitle.parseSubtitleFileByJni (filename, encode);
            if (file == null) {
                Log.e("TextSubParser", "------------err-----------");
                //throw new MalformedSubException ("text sub parser return NULL!");
                return null;
            }
            else
            {
                return new TextSubApi (file);

            }
        };
        public SubtitleApi parse (String inputstring, int index) throws MalformedSubException {
            return null;
        };

}
