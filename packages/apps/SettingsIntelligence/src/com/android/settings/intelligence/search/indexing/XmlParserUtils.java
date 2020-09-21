/*
 * Copyright (C) 2017 The Android Open Source Project
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

package com.android.settings.intelligence.search.indexing;

import android.content.Context;
import android.content.res.TypedArray;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;
import android.util.TypedValue;

import com.android.settings.intelligence.R;

/**
 * Utility class to parse elements of XML preferences
 */
public class XmlParserUtils {

    private static final String TAG = "XmlParserUtils";
    private static final String NS_APP_RES_AUTO = "http://schemas.android.com/apk/res-auto";
    private static final String ENTRIES_SEPARATOR = "|";

    public static String getDataKey(Context context, AttributeSet attrs) {
        return getData(context, attrs,
                R.styleable.Preference,
                R.styleable.Preference_android_key);
    }

    public static String getDataTitle(Context context, AttributeSet attrs) {
        return getData(context, attrs,
                R.styleable.Preference,
                R.styleable.Preference_android_title);
    }

    public static String getDataSummary(Context context, AttributeSet attrs) {
        return getData(context, attrs,
                R.styleable.Preference,
                R.styleable.Preference_android_summary);
    }

    public static String getDataSummaryOn(Context context, AttributeSet attrs) {
        return getData(context, attrs,
                R.styleable.CheckBoxPreference,
                R.styleable.CheckBoxPreference_android_summaryOn);
    }

    public static String getDataSummaryOff(Context context, AttributeSet attrs) {
        return getData(context, attrs,
                R.styleable.CheckBoxPreference,
                R.styleable.CheckBoxPreference_android_summaryOff);
    }

    public static String getDataEntries(Context context, AttributeSet attrs) {
        return getDataEntries(context, attrs,
                R.styleable.ListPreference,
                R.styleable.ListPreference_android_entries);
    }

    public static String getDataKeywords(Context context, AttributeSet attrs) {
        final String keywordRes = attrs.getAttributeValue(NS_APP_RES_AUTO, "keywords");
        if (TextUtils.isEmpty(keywordRes)) {
            return null;
        }
        // The format of keyword is either a string, or @ followed by int (@123456).
        // When it's int, we need to look up the actual string from context.
        if (!keywordRes.startsWith("@")) {
            // It's a string.
            return keywordRes;
        } else {
            // It's a resource
            try  {
                final int resValue = Integer.parseInt(keywordRes.substring(1));
                return context.getString(resValue);
            } catch (NumberFormatException e) {
                Log.w(TAG, "Failed to parse keyword attribute, skipping " + keywordRes);
                return null;
            }
        }
    }

    public static int getDataIcon(Context context, AttributeSet attrs) {
        final TypedArray ta = context.obtainStyledAttributes(attrs, R.styleable.Preference);
        final int dataIcon = ta.getResourceId(R.styleable.Preference_android_icon, 0);
        ta.recycle();
        return dataIcon;
    }

    /**
     * Returns the fragment name if this preference launches a child fragment.
     */
    public static String getDataChildFragment(Context context, AttributeSet attrs) {
        return getData(context, attrs, R.styleable.Preference,
                R.styleable.Preference_android_fragment);
    }

    @Nullable
    private static String getData(Context context, AttributeSet set, int[] attrs, int resId) {
        final TypedArray ta = context.obtainStyledAttributes(set, attrs);
        String data = ta.getString(resId);
        ta.recycle();
        return data;
    }


    private static String getDataEntries(Context context, AttributeSet set, int[] attrs,
            int resId) {
        final TypedArray sa = context.obtainStyledAttributes(set, attrs);
        final TypedValue tv = sa.peekValue(resId);
        sa.recycle();
        String[] data = null;
        if (tv != null && tv.type == TypedValue.TYPE_REFERENCE) {
            if (tv.resourceId != 0) {
                data = context.getResources().getStringArray(tv.resourceId);
            }
        }
        final int count = (data == null) ? 0 : data.length;
        if (count == 0) {
            return null;
        }
        final StringBuilder result = new StringBuilder();
        for (int n = 0; n < count; n++) {
            result.append(data[n]);
            result.append(ENTRIES_SEPARATOR);
        }
        return result.toString();
    }
}
