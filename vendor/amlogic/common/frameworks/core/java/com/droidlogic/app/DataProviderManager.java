/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app;

import android.content.Context;
import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteDatabase;
import android.database.Cursor;
import android.net.Uri;
import android.content.UriMatcher;
import android.util.Log;
import android.provider.Settings;

public class DataProviderManager {
    private static final String TAG = "DataProviderManager";
    private static final boolean DEBUG = true;

    public static final String TABLE_SCAN_NAME = "tv_control_scan";
    public static final String TABLE_SOUND_NAME = "tv_control_sound";
    public static final String TABLE_PPPOE_NAME = "tv_control_pppoe";
    public static final String TABLE_OTHERS_NAME = "tv_control_others";
    public static final String TABLE_PROP_NAME = "prop_table";
    public static final String TABLE_STRING_NAME = "string_table";
    public static final String PROPERTY = "property";
    public static final String VALUE = "value";
    public static final String DESCRIP = "description";
    public static final String AUTHORITY = "com.droidlogic.database";
    public static final String CONTENT_URI = "content://" + AUTHORITY + "/";

    public static boolean putStringValue(Context context, String name, String value) {
        if (DEBUG) Log.d(TAG, "putStringValue  name = " + name + ", value = " + value);
        boolean result = false;
        if (context == null) {
            Log.d(TAG, "putStringValue null context");
            return result;
        }
        Uri uri = Uri.parse(CONTENT_URI + TABLE_STRING_NAME);
        ContentValues values = new ContentValues();
        values.put(PROPERTY, name);
        values.put(VALUE, value);
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int row = context.getContentResolver().update(uri, values, PROPERTY + " = ?", new String[] { name });
                if (row != -1) {
                    result = true;
                }
                if (DEBUG) {
                    Log.d(TAG, "putStringValue update row = " + row);
                }
            } else {
                Uri resultUri = context.getContentResolver().insert(uri, values);
                if (resultUri != null) {
                    result = true;
                }
                if (DEBUG) {
                    Log.d(TAG, "putStringValue insert resultUri = " + resultUri);
                }
            }
        } catch (Exception e) {
            Log.e(TAG,"putStringValue putString Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DEBUG) {
            Log.d(TAG, "putStringValue result = " + result);
        }
        return result;
    }

    public static String getStringValue(Context context, String name, String def) {
        String result = def;
        if (context == null) {
            Log.d(TAG, "getStringValue null context");
            return result;
        }
        Uri uri = Uri.parse(CONTENT_URI + TABLE_STRING_NAME);
        Cursor cursor = null;
        try {
            cursor = context.getContentResolver().query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                result = cursor.getString(cursor.getColumnIndex(VALUE));
            }
        } catch (Exception e) {
            Log.e(TAG,"getStringValue Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DEBUG) Log.d(TAG,"getStringValue  name = " + name + ", value = " + result);
        return result;
    }

    public static boolean putBooleanValue(Context context, String name, boolean value) {
        boolean result = false;
        result = putStringValue(context, name, value ? "true" : "false");
        return result;
    }

    public static boolean getBooleanValue(Context context, String name, boolean def) {
        boolean result = def;
        String stringValue = getStringValue(context, name, def ? "true" : "false");
        if ("true".equals(stringValue)) {
            result = true;
        } else {
            result = false;
        }
        return result;
    }

    public static boolean putIntValue(Context context, String name, int value) {
        boolean result = false;
        result = putStringValue(context, name, String.valueOf(value));
        return result;
    }

    public static int getIntValue(Context context, String name, int def) {
        int result = def;
        String stringValue = getStringValue(context, name, String.valueOf(def));
        try {
            result = Integer.valueOf(stringValue);
        } catch (Exception e) {
            Log.d(TAG, "getIntValue Exception = " + e.getMessage());
        }
        return result;
    }

    public static boolean putLongValue(Context context, String name, long value) {
        boolean result = false;
        result = putStringValue(context, name, String.valueOf(value));
        return result;
    }

    public static long getLongValue(Context context, String name, long def) {
        long result = def;
        String stringValue = getStringValue(context, name, String.valueOf(def));
        try {
            result = Long.valueOf(stringValue);
        } catch (Exception e) {
            Log.d(TAG, "getLongValue Exception = " + e.getMessage());
        }
        return result;
    }
}
