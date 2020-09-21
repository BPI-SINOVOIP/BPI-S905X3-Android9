/*
 * Copyright (C) 2008-2012  OMRON SOFTWARE Co., Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package jp.co.omronsoft.openwnn;

import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import org.xmlpull.v1.XmlPullParserException;
import android.content.SharedPreferences;
import android.content.res.XmlResourceParser;
import android.util.Log;

/**
 * The generator class of symbol list.
 * <br>
 * This class is used for generating lists of symbols.
 *
 * @author Copyright (C) 2009 OMRON SOFTWARE CO., LTD.  All Rights Reserved.
 */
public class SymbolList implements WnnEngine {
    /*
     * DEFINITION OF CONSTANTS
     */
    /** Language definition (English) */
    public static final int LANG_EN = 0;

    /** Language definition (Japanese) */
    public static final int LANG_JA = 1;

    /** Language definition (Chinese) */
    public static final int LANG_ZHCN = 2;

    
    /** Key string to get normal symbol list for Japanese */
    public static final String SYMBOL_JAPANESE = "j";

    /** Key string to get normal symbol list for English */
    public static final String SYMBOL_ENGLISH = "e";

    /** Key string to get normal symbol list for Chinese */
    public static final String SYMBOL_CHINESE = "c1";

    /** Key string to get face mark list for Japanese */
    public static final String SYMBOL_JAPANESE_FACE  = "j_face";

    /** The name of XML tag key */
    private static final String XMLTAG_KEY = "string";

    /*
     * DEFINITION OF VARIABLES
     */
    /** Symbols data */
    protected HashMap<String,ArrayList<String>> mSymbols;

    /** OpenWnn which has this instance */
    private OpenWnn mWnn;

    /** current list of symbols */
    private ArrayList<String> mCurrentList;

    /** Iterator for getting symbols from the list */
    private Iterator<String> mCurrentListIterator;

    /*
     * DEFINITION OF METHODS
     */
    /**
     * Constructor
     *
     * @param parent  OpenWnn instance which uses this.
     * @param lang    Language ({@code LANG_EN}, {@code LANG_JA} or {@code LANG_ZHCN})
     */
    public SymbolList(OpenWnn parent, int lang) {
        mWnn = parent;
        mSymbols = new HashMap<String, ArrayList<String>>();

        switch (lang) {
        case LANG_EN:
            /* symbols for English IME */
            mSymbols.put(SYMBOL_ENGLISH, getXmlfile(R.xml.symbols_latin12_list));
            mCurrentList = mSymbols.get(SYMBOL_ENGLISH);
            break;

        case LANG_JA:
            /* symbols for Japanese IME */
            mSymbols.put(SYMBOL_JAPANESE, getXmlfile(R.xml.symbols_japan_list));
            mSymbols.put(SYMBOL_JAPANESE_FACE, getXmlfile(R.xml.symbols_japan_face_list));
            mSymbols.put(SYMBOL_ENGLISH, getXmlfile(R.xml.symbols_latin1_list));
            mCurrentList = mSymbols.get(SYMBOL_JAPANESE);
            break;

        case LANG_ZHCN: 
            /* symbols for Chinese IME */
            mSymbols.put(SYMBOL_CHINESE, getXmlfile(R.xml.symbols_china_list));
            mSymbols.put(SYMBOL_ENGLISH, getXmlfile(R.xml.symbols_latin1_list));
            mCurrentList = mSymbols.get(SYMBOL_CHINESE);
            break;
        }        

        mCurrentList = null;
    }
    
    /**
     * Get a attribute value from a XML resource.
     *
     * @param xrp   XML resource
     * @param name  The attribute name
     *
     * @return  The value of the attribute
     */
    private String getXmlAttribute(XmlResourceParser xrp, String name) {
        int resId = xrp.getAttributeResourceValue(null, name, 0);
        if (resId == 0) {
            return xrp.getAttributeValue(null, name);
        } else {
            return mWnn.getString(resId);
        }
    }

    /**
     * Load a symbols list from XML resource.
     *
     * @param id    XML resource ID
     * @return      The symbols list
     */
    private ArrayList<String> getXmlfile(int id) {
        ArrayList<String> list = new ArrayList<String>();

        XmlResourceParser xrp = mWnn.getResources().getXml(id);
        try {
            int xmlEventType;
            while ((xmlEventType = xrp.next()) != XmlResourceParser.END_DOCUMENT) {
                if (xmlEventType == XmlResourceParser.START_TAG) {
                    String attribute = xrp.getName();
                    if (XMLTAG_KEY.equals(attribute)) {
                        String value = getXmlAttribute(xrp, "value");
                        if (value != null) {
                            list.add(value);
                        }
                    }
                }
            }
            xrp.close();
        } catch (XmlPullParserException e) {
            Log.e("OpenWnn", "Ill-formatted keybaord resource file");
        } catch (IOException e) {
            Log.e("OpenWnn", "Unable to read keyboard resource file");
        }

        return list;
    }

    /**
     * Set the dictionary
     *
     * @param listType  The list of symbol
     * @return          {@code true} if valid type is specified; {@code false} if not;
     */
    public boolean setDictionary(String listType) {
        mCurrentList = mSymbols.get(listType);
        return (mCurrentList != null);
    }

    /***********************************************************************
     * WnnEngine's interface
     **********************************************************************/
    /** @see jp.co.omronsoft.openwnn.WnnEngine#init */
    public void init() {}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#close */
    public void close() {}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#predict */
    public int predict(ComposingText text, int minLen, int maxLen) {
        /* ignore if there is no list for the type */
        if (mCurrentList == null) {
            mCurrentListIterator = null;
            return 0;
        }

        /* return the iterator of the list */
        mCurrentListIterator = mCurrentList.iterator();
        return 1;
    }
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#convert */
    public int convert(ComposingText text) {
        return 0;
    }
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#searchWords */
    public int searchWords(String key) {return 0;}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#searchWords */
    public int searchWords(WnnWord word) {return 0;}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#getNextCandidate */
    public WnnWord getNextCandidate() {
        if (mCurrentListIterator == null || !mCurrentListIterator.hasNext()) {
            return null;
        }
        String str = mCurrentListIterator.next();
        WnnWord word = new WnnWord(str, str);
        return word;
    }
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#learn */
    public boolean learn(WnnWord word) {return false;}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#addWord */
    public int addWord(WnnWord word) {return 0;}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#deleteWord */
    public boolean deleteWord(WnnWord word) {return false;}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#setPreferences */
    public void setPreferences(SharedPreferences pref) {}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#breakSequence */
    public void breakSequence() {}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#makeCandidateListOf */
    public int makeCandidateListOf(int clausePosition) {return 0;}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#initializeDictionary */
    public boolean initializeDictionary(int dictionary) {return true;}

    /** @see jp.co.omronsoft.openwnn.WnnEngine#initializeDictionary */
    public boolean initializeDictionary(int dictionary, int type) {return true;}
    
    /** @see jp.co.omronsoft.openwnn.WnnEngine#getUserDictionaryWords */
    public WnnWord[] getUserDictionaryWords() {return null;}
}
