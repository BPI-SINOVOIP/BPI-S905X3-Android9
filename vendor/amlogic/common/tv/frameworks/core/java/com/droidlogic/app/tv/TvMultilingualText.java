/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.util.Locale;
import android.database.Cursor;
import android.content.Context;
import android.util.Log;

import org.json.JSONObject;
import org.json.JSONArray;
import org.json.JSONException;

/**
 *TvMultilingualText
 *multilingual text parsing
 */
public class TvMultilingualText{
    private static final String TAG="TvMultilingualText";

    private class MultilingualText{
        protected String language;
        protected String text;

        public MultilingualText(String formatString){
            if (formatString != null && formatString.length() >= 3) {
                language = formatString.substring(0, 3);
                /*there is no iso-descr in service_descr/SDT, xxx indicate it*/
                if (language.equalsIgnoreCase("xxx")) {
                    language = "eng";
                }
                if (formatString.length() > 3) {
                    text = formatString.substring(3, formatString.length());
                }
            } else {
                language = "";
            }
        }

        public MultilingualText() {}

        public String getLangage(){
            return language;
        }

        public String getText(){
            return text;
        }
    }


    private class MultilingualTextJ extends MultilingualText {
        //json coding format: {lng:"eng",txt:"string"}

        public MultilingualTextJ(JSONObject jsonObject) {
            parse(jsonObject);
        }

        public MultilingualTextJ(String jsonString) {
            if (jsonString != null && jsonString.length() >= 8) {
                JSONObject jsonObject;
                try {
                    jsonObject = new JSONObject(jsonString);
                } catch (JSONException e) {
                    throw new RuntimeException("Json parse fail: ["+jsonString+"]", e);
                }
                parse(jsonObject);
            }
        }

        public void parse(JSONObject jsonObject) {
            if (jsonObject != null) {
                language = jsonObject.optString("lng");
                text = jsonObject.optString("txt");
                if (language.equalsIgnoreCase("xxx"))
                    language = "eng";
            } else {
                language = "";
            }
        }
    }

    public static String getText(String formatText, String lang) {
        String ret = null;
        boolean useFirst = false;
        String split;

        if (formatText == null || lang == null || formatText.isEmpty())
            return ret;

        /* special case for 'local' and 'first' */
        if (lang.equalsIgnoreCase("local")) {
            lang = getLocalLang();
        }else if (lang.equalsIgnoreCase("first")) {
            useFirst = true;
        }

        if (formatText.contains(new String(new byte[]{(byte)0x80}))) {
            split = new String(new byte[]{(byte)0x80});
        }else{
            split = new String(new char[]{(char)0x80});
        }
        String[] langText = formatText.split(split);
        for (int i=0; langText!=null && i<langText.length; i++) {
            TvMultilingualText inst = new TvMultilingualText();
            MultilingualText text = inst.new MultilingualText(langText[i]);

            if (useFirst || text.getLangage().equalsIgnoreCase(lang)) {
                ret = text.getText();
                break;
            }
        }

        return ret;
    }

    public static String getText(String formatText) {
        String ret = null;
        String configLangs = "local first";

        if (configLangs == null || configLangs.isEmpty())
            return ret;

        String[] langs = configLangs.split(" ");
        if (langs != null && langs.length > 0) {
            for (int i=0; i<langs.length; i++) {
                ret = getText(formatText, langs[i]);
                if (ret != null && !ret.isEmpty()) {
                    break;
                }
            }
        }
        return ret;
    }

    public static String getText(String formatText, String[] langs) {
        String ret = null;
        String[] defaultLangs = {"local", "first"};

        if (langs == null || langs.length == 0)
            langs = defaultLangs;

        if (langs != null && langs.length > 0) {
            for (int i=0; i<langs.length; i++) {
                ret = getText(formatText, langs[i]);
                if (ret != null && !ret.isEmpty()) {
                    break;
                }
            }
        }
        return ret;
    }

    private static final Locale HK_LOCAL = new Locale("zh", "HK");
    public static String getLocalLang() {
        String lang;
        Locale defaultLocale = Locale.getDefault();
        /* recover lang by the current local Android language */
        if (defaultLocale.equals(Locale.SIMPLIFIED_CHINESE)) {
            lang = "chs";
        }else if (defaultLocale.equals(Locale.TRADITIONAL_CHINESE) ||
            defaultLocale.equals(HK_LOCAL)) {
            lang = "chi";
        }else{
            lang = Locale.getDefault().getISO3Language();
        }

        return lang;
    }

    public static String getTextJ(String formatText, String lang) {
        String ret = null;
        boolean useFirst = false;

        if (formatText == null || lang == null || formatText.isEmpty())
            return ret;

        /* special case for 'local' and 'first' */
        if (lang.equalsIgnoreCase("local")) {
            lang = getLocalLang();
        }else if (lang.equalsIgnoreCase("first")) {
            useFirst = true;
        }

        JSONArray jsonArray;
        try {
            jsonArray = new JSONArray(formatText);
        } catch (JSONException e) {
            throw new RuntimeException("Json parse fail: ["+formatText+"]", e);
        }
        for (int i=0; i<jsonArray.length(); i++) {
            JSONObject j = jsonArray.optJSONObject(i);
            TvMultilingualText inst = new TvMultilingualText();
            MultilingualTextJ text = inst.new MultilingualTextJ(j);
            if (useFirst || text.getLangage().equalsIgnoreCase(lang)) {
                ret = text.getText();
                break;
            }
        }

        return ret;
    }

    public static String getTextJ(String formatText, String[] langs) {
        String ret = null;
        String[] defaultLangs = {"local", "first"};

        if (langs == null || langs.length == 0)
            langs = defaultLangs;

        if (langs != null && langs.length > 0) {
            for (int i=0; i<langs.length; i++) {
                ret = getTextJ(formatText, langs[i]);
                if (ret != null && !ret.isEmpty()) {
                    break;
                }
            }
        }
        return ret;
    }

    public static String getTextJ(String formatText) {
        return getTextJ(formatText, new String[0]);
    }

}

