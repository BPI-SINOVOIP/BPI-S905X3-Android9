/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.droidlogic.app.tv;

import android.content.ContentUris;
import android.content.Context;
//import android.content.pm.PackageManager.NameNotFoundException;
//import android.content.res.Resources;
//import android.content.res.XmlResourceParser;
import android.media.tv.TvContentRatingSystemInfo;
//import android.net.Uri;
import android.util.Log;

//import com.android.tv.parental.ContentRatingSystem.Order;
//import com.android.tv.parental.ContentRatingSystem.Rating;
//import com.android.tv.parental.ContentRatingSystem.SubRating;

import java.io.FileOutputStream;
import java.io.InputStream;
import java.util.List;
import android.app.Activity;
import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;


import android.content.Context;
import android.content.Intent;
//import android.media.tv.TvContentRating;
//import android.media.tv.TvInputManager;
//import android.os.Environment;
import android.os.Handler;
import android.os.UserHandle;
import android.text.TextUtils;
import android.util.AtomicFile;
import android.util.Xml;

//import com.android.internal.util.FastXmlSerializer;
//import com.android.internal.util.XmlUtils;
//import libcore.io.IoUtils;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import java.lang.Boolean;
import java.lang.reflect.AccessibleObject;
import java.lang.reflect.Field;
import java.lang.reflect.Method;

public class DroidContentRatingsParser {
    private static final String TAG = "DroidContentRatingsParser";
    private static final boolean DEBUG = false;

    public static final String DOMAIN_RRT_RATINGS = "com.droidlogic.app.tv";
    public static final int FIXED_REGION_lEVEL_2 = 2;

    private static final String TAG_RATING_SYSTEM_DEFINITIONS = "rating-system-definitions";
    private static final String TAG_RATING_SYSTEM_DEFINITION = "rating-system-definition";
    private static final String TAG_RATING_DEFINITION = "rating-definition";

    private static final String ATTR_STRING = "string";
    private static final String ATTR_ENABLED = "enabled";

    private static final String ATTR_NAME = "name";
    private static final String ATTR_RATING = "rating";
    private static final String ATTR_COUNTRY = "country";
    private static final String ATTR_DIMENSION_ID = "dimension_id";
    private static final String ATTR_TITLE = "title";
    private static final String ATTR_DESCRIPTION = "description";
    private static final String ATTR_RADING_ID = "rating_id";

    //private final Context mContext;
    //private Resources mResources;
    private String mXmlVersionCode;
    private final Object mLock = new Object();

    private AtomicFile mAtomicFile_t;
    public DroidContentRatingsParser() {
        //mContext = context;
        File userDir = new File("/mnt/vendor/param");//Environment.getDataSystemDirectory();
        if (!userDir.exists()) {
            if (!userDir.mkdirs()) {
                throw new IllegalStateException("User dir cannot be created: " + userDir);
            }
         }
         mAtomicFile_t = new AtomicFile(new File(userDir, "tv_rrt_define.xml"));
    }

    private void doUtilscloseQuietly (InputStream string) {
        /*try {
            Class clazz = ClassLoader.getSystemClassLoader().loadClass("libcore.io.IoUtils");
            Method method = clazz.getMethod("closeQuietly", InputStream.class);
            method.invoke(clazz, string);
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();s
        }*/
        //function closeQuietly defined in IoUtils doesn't have InputStream.class parameter
        try {
            if (string != null) {
                string.close();
            }
        } catch (IOException e) {
            Log.e(TAG, "Failed to close rrt xml file", e);
            e.printStackTrace();
        }
    }

    private void doXmlUtilsbeginDocument (XmlPullParser parser, String string) {
        try {
            Class clazz = ClassLoader.getSystemClassLoader().loadClass("com.android.internal.util.XmlUtils");
            Method method = clazz.getMethod("beginDocument", XmlPullParser.class, String.class);
            method.invoke(clazz, parser, string);
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
    }

    private boolean doXmlUtilsnextElementWithin (XmlPullParser parser, int outerDepth) {
        try {
            Class clazz = ClassLoader.getSystemClassLoader().loadClass("com.android.internal.util.XmlUtils");
            Method method = clazz.getMethod("nextElementWithin", XmlPullParser.class, int.class);
            Object objBoolean = method.invoke(clazz, parser, outerDepth);
            return Boolean.parseBoolean(objBoolean.toString());
        } catch (Exception e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        return false;
    }

    public List<ContentRatingSystemT> load_t() {
        //clearState();
        synchronized (mLock) {
            final InputStream is;
            Log.d(TAG, "==== start load_t====");
            try {
                is = mAtomicFile_t.openRead();
            } catch (FileNotFoundException ex) {
                Log.d(TAG, "==== load FileNotFoundException====");
                return null;
            }

            XmlPullParser parser;
            try {
                parser = Xml.newPullParser();
                parser.setInput(new BufferedInputStream(is), StandardCharsets.UTF_8.name());
                return loadFromXml_t(parser);
            } catch (IOException | XmlPullParserException ex) {
                Log.w(TAG, "Failed to load tv input manager persistent store data.", ex);
                //clearState();
            } finally {
                doUtilscloseQuietly(is);
            }
            return null;
        }
    }
    private List<ContentRatingSystemT> loadFromXml_t(XmlPullParser parser)
            throws IOException, XmlPullParserException {
        List<ContentRatingSystemT> ratingSystems_t = new ArrayList<>();

        doXmlUtilsbeginDocument(parser, TAG_RATING_SYSTEM_DEFINITIONS);
        final int outerDepth = parser.getDepth();
        //Log.w(TAG, "loadFromXml_t,outerDepth:"+outerDepth);
        while (doXmlUtilsnextElementWithin(parser, outerDepth)) {
           // Log.w(TAG, "-----name:"+parser.getName());
            if (parser.getName().equals(TAG_RATING_SYSTEM_DEFINITION)) {
                ratingSystems_t.add(parseRatingSystemDefinition_t(parser));
            }
        }
        return ratingSystems_t;
    }
    private ContentRatingSystemT parseRatingSystemDefinition_t(XmlPullParser parser)
            throws IOException, XmlPullParserException {
        ContentRatingSystemT builder = new ContentRatingSystemT();
        //builder.setDomain(domain);
        final int outerDepth = parser.getDepth();
        int i = 0;
        //Log.w(TAG, "     parseRatingSystemDefinition_t:"+outerDepth);
        int attr_num = parser.getAttributeCount();
       // Log.w(TAG, "     attr_num:"+attr_num);

        for (i = 0; i< attr_num; i++) {
            String attr = parser.getAttributeName(i);
            switch (attr) {
                case ATTR_NAME:
                    //Log.w(TAG, "         attr:"+attr+", value:"+parser.getAttributeValue(i));
                    builder.setName(parser.getAttributeValue(i));
                    break;
                case ATTR_RATING:
                    //Log.w(TAG, "         attr:"+attr+", value:"+parser.getAttributeValue(i));
                    builder.setRegion(StringToInt(parser.getAttributeValue(i)));
                    break;
                case ATTR_COUNTRY:
                   // Log.w(TAG, "         attr:"+attr+", value:"+parser.getAttributeValue(i));
                    builder.setCountry(parser.getAttributeValue(i));
                    break;
                case ATTR_DIMENSION_ID:
                   // Log.w(TAG, "         attr:"+attr+", value:"+parser.getAttributeValue(i));
                    break;
            }
        }


        while (doXmlUtilsnextElementWithin(parser, outerDepth)) {
            i = 0;
           // Log.w(TAG, "         ---tag:"+parser.getName()+"i:"+i);
            if (parser.getName().equals(TAG_RATING_DEFINITION)) {
                builder.addRatingBuilder(parseRatingDefinition_t(parser));
            }
        }
        return builder;
    }

    private int StringToInt(String value) {
        int getvalue = -1;
        try {
            getvalue = Integer.valueOf(value);
        } catch (NumberFormatException e){
            throw new NumberFormatException("string is not integer: " + value);
        }
        return getvalue;
    }

    private RatingDefinition parseRatingDefinition_t(XmlPullParser parser)
            throws XmlPullParserException, IOException {
        RatingDefinition builder = new RatingDefinition();
        //Log.w(TAG, "             parseRatingDefinition_t:"+parser.getAttributeCount());
        for (int i = 0; i < parser.getAttributeCount(); i++) {
            String attr = parser.getAttributeName(i);
            switch (attr) {
                /*case ATTR_NAME:
                    builder.setName(parser.getAttributeValue(i));
                    break;*/
                case ATTR_TITLE:
                case ATTR_NAME:
                    //Log.w(TAG, "                 attr:"+attr+", value:"+parser.getAttributeValue(i));
                    builder.setTitle(parser.getAttributeValue(i));
                    break;
                case ATTR_DESCRIPTION:
                   // Log.w(TAG, "                 attr:"+attr+", value:"+parser.getAttributeValue(i));
                    builder.setDescription(parser.getAttributeValue(i));
                    break;
                case ATTR_RADING_ID:
                    //Log.w(TAG, "                 attr:"+attr+", value:"+parser.getAttributeValue(i));
                    break;
            }
        }
        return builder;
    }

    private void clearState() {
        /*mBlockedRatings.clear();
        mParentalControlsEnabled = false;*/
    }

    public class ContentRatingSystemT {
        private String mName;
        private String mCountry;
        private int mRegion;
        private final List<RatingDefinition> mRatings =  new ArrayList<>();

        public void ContentRatingSystemT(){
        }

        public void setName(String name) {
        if (name == null || name.equals(""))
             mName = "NULL";
             else
             mName = name;
        }

        public void setCountry(String country) {
        if (country == null || country.equals(""))
            mCountry = "NULL";
        else
            mCountry = country;
        }

        public void setRegion(int region) {
            mRegion = region;
        }

        public int getRegion() {
            return mRegion;
        }

        public String getName() {
            return mName;
        }
        public String getCountry() {
            return mCountry;
        }
        public void addRatingBuilder(RatingDefinition rating) {
            if (mRatings == null) {
                Log.d(TAG,"addRatingBuilder,mRating is NULL");
                return;
            }
            mRatings.add(rating);
        }
        public List<RatingDefinition> getRatingDefinitions() {
            return mRatings;
        }
        public String toString(){
        String s = "Name:"+mName+",Country:" + mCountry + "\n RatingDefinition : \n";
        for (RatingDefinition rating : mRatings)
            s = s + rating.toString();
            return s;
        }

        public void clear() {
        mRatings.clear();
        }
    }

    public class RatingDefinition {
        private String mTitle;
        private String mDescription;
        public void RatingDefinition(){
        }

        public void setTitle(String name) {
            if (name == null || name.equals(""))
                mTitle = "NULL";
            else
                mTitle = name;
        }
        public void setDescription(String description) {
            if (description == null || description.equals(""))
                mDescription = "NULL";
            else
                mDescription = description;
        }
            public String getTitle() {
                return mTitle;
            }
        public String getDescription() {
            return mDescription;
        }

        public String toString(){
            String s = "    Title:" + mTitle + ",Description:" + mDescription+"\n";
            return s;
        }
    }
}
