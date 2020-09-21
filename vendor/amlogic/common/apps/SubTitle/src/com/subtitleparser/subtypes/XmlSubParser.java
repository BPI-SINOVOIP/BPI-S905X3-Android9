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
import org.xmlpull.v1.XmlPullParser;
import java.io.FileInputStream;
import android.util.Xml;

class XmlSubApi extends SubtitleApi {
        private static final String TAG = "XmlSubApi";
        private SubtitleFile SubFile = null;
        private SubtitleLine cur = null;
        private String st = null;

        public XmlSubApi (SubtitleFile sf) {
            SubFile = sf;
        }
        public void closeSubtitle() {}
        public int getSubTypeDetial() {
            return -1;
        }
        public Subtitle.SUBTYPE type() {
            return Subtitle.SUBTYPE.SUB_XML;
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
* XmlSub subtitle parser.
*
* @author xiaoliang.wang
*/
public class XmlSubParser implements SubtitleParser {
        private static final String TAG = "XmlSubApi-XmlSubParser";
        private boolean DEBUG = false;
        private String mEncoding;
        public XmlSubParser (String enc) {
            mEncoding = enc;
        }
        public SubtitleApi parse (String fileName, int index) throws MalformedSubException {
            int hStart = 0;
            int minStart = 0;
            int secStart = 0;
            int milStart = 0;
            int hEnd = 0;
            int minEnd = 0;
            int secEnd = 0;
            int milEnd = 0;
            int subNum = 0;
            SubtitleFile sf = new SubtitleFile();
            SubtitleLine sl = null;

            try {
                FileInputStream fin = new FileInputStream (fileName);
                XmlPullParser parser = Xml.newPullParser();
                parser.setInput (fin, mEncoding);
                int eventType = parser.getEventType();

                while (eventType != XmlPullParser.END_DOCUMENT) {
                    switch (eventType) {
                        case XmlPullParser.START_DOCUMENT:
                            //Log.i(TAG,"[XmlSubParser]START_DOCUMENT");
                            break;
                        case XmlPullParser.START_TAG:
                            //Log.i(TAG,"[XmlSubParser]START_TAG");
                            String name = parser.getName();
                            if (name.equalsIgnoreCase ("Number")) {
                                subNum = Integer.parseInt (parser.nextText());
                                //Log.i(TAG,"[XmlSubParser]subNum:"+subNum);
                            }
                            else if (name.equalsIgnoreCase ("StartMilliseconds")) {
                                int timeStart = Integer.parseInt (parser.nextText());
                                hStart = timeStart % 3600000;
                                minStart = timeStart % 60000 - hStart * 60;
                                secStart = timeStart % 1000 - hStart * 3600 - minStart * 60;
                                milStart = timeStart - hStart * 3600000 - minStart * 60000 - secStart * 1000;
                                //Log.i(TAG,"[XmlSubParser]hStart:"+hStart+",minStart:"+minStart+",secStart:"+secStart+", milStart:"+milStart);
                            }
                            else if (name.equalsIgnoreCase ("EndMilliseconds")) {
                                int timeEnd = Integer.parseInt (parser.nextText());
                                hEnd = timeEnd % 3600000;
                                minEnd = timeEnd % 60000 - hEnd * 60;
                                secEnd = timeEnd % 1000 - hEnd * 3600 - minEnd * 60;
                                milEnd = timeEnd - hEnd * 3600000 - minEnd * 60000 - secEnd * 1000;
                                //Log.i(TAG,"[XmlSubParser]hEnd:"+hEnd+",minEnd:"+minEnd+",secEnd:"+secEnd+", milEnd:"+milEnd);
                            }
                            else if (name.equalsIgnoreCase ("Text")) {
                                sl = new SubtitleLine (subNum,
                                                       new SubtitleTime (hStart, minStart, secStart, milStart), //start time
                                                       new SubtitleTime (hEnd, minEnd, secEnd, milEnd), //end time
                                                       parser.nextText() //text
                                                      );
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
                            break;
                        case XmlPullParser.END_TAG:
                            //Log.i(TAG,"[XmlSubParser]END_TAG");
                            break;
                    }
                    eventType = parser.next();
                }

                fin.close();
                return new XmlSubApi (sf);
            }
            catch (Exception e) {
                e.printStackTrace();
                throw new MalformedSubException (e.toString());
            }
        };

        public SubtitleApi parse (String inputString, String st2) throws MalformedSubException {
            return null;
        };
}
