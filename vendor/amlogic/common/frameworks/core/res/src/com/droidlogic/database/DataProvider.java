/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DataProvider
 */

package com.droidlogic.database;

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

import java.util.Arrays;

public class DataProvider extends ContentProvider {
    private static final String TAG = "DataProvider";
    private static final boolean DEBUG = true;

    private static final UriMatcher mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    private DbOpenHelper mDbOpenHelper = null;

    public static final String DB_NAME = "database.db";
    public static final int DB_VERSION = 1;
    public static final String TABLE_SCAN_NAME = "tv_control_scan";
    public static final String TABLE_SOUND_NAME = "tv_control_sound";
    public static final String TABLE_PPPOE_NAME = "tv_control_pppoe";
    public static final String TABLE_OTHERS_NAME = "tv_control_others";
    public static final String TABLE_PROP_NAME = "prop_table";
    public static final String TABLE_STRING_NAME = "string_table";

    public static final int TABLE_SCAN_CODE = 1;
    public static final int TABLE_SOUND_CODE = 2;
    public static final int TABLE_PPPOE_CODE = 3;
    public static final int TABLE_OTHERS_CODE = 4;
    public static final int TABLE_PROP_CODE = 5;
    public static final int TABLE_STRING_CODE = 6;

    public static final String PROPERTY = "property";
    public static final String VALUE = "value";
    public static final String DESCRIP = "description";
    public static final String TABLE_PROPERTY = "(_id INTEGER PRIMARY KEY AUTOINCREMENT, " + PROPERTY + " TEXT UNIQUE ON CONFLICT REPLACE, " + VALUE + " TEXT, " + DESCRIP + " TEXT);";

    public static final String AUTHORITY = "com.droidlogic.database";
    public static final String CONTENT_URI = "content://" + AUTHORITY + "/";

    static {
        mUriMatcher.addURI(AUTHORITY, TABLE_SCAN_NAME, TABLE_SCAN_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_SOUND_NAME, TABLE_SOUND_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_PPPOE_NAME, TABLE_PPPOE_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_OTHERS_NAME, TABLE_OTHERS_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_PROP_NAME, TABLE_PROP_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_STRING_NAME, TABLE_STRING_CODE);
    }

    private static class DbOpenHelper extends SQLiteOpenHelper {

        private final String SQL_CREATE_TABLE1 = "create table if not exists " + TABLE_SCAN_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE2 = "create table if not exists " + TABLE_SOUND_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE3 = "create table if not exists " + TABLE_PPPOE_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE4 = "create table if not exists " + TABLE_OTHERS_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE5 = "create table if not exists " + TABLE_PROP_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE6 = "create table if not exists " + TABLE_STRING_NAME + TABLE_PROPERTY;

        public DbOpenHelper(final Context context) {
            super(context, DB_NAME, null, DB_VERSION);
        }

        @Override
        public void onCreate(final SQLiteDatabase db) {
            db.execSQL(SQL_CREATE_TABLE1);
            db.execSQL(SQL_CREATE_TABLE2);
            db.execSQL(SQL_CREATE_TABLE3);
            db.execSQL(SQL_CREATE_TABLE4);
            db.execSQL(SQL_CREATE_TABLE5);
            db.execSQL(SQL_CREATE_TABLE6);
            Log.d(TAG, "onCreate tables");
        }

        @Override
        public void onUpgrade(final SQLiteDatabase db, final int oldVersion, final int newVersion) {

        }
    }

    @Override
    public boolean onCreate() {
        mDbOpenHelper = new DbOpenHelper(getContext());
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
        String table = getTableName(uri);
        SQLiteDatabase db = mDbOpenHelper.getReadableDatabase();
        if (db != null) {
            if (DEBUG) Log.d(TAG, "query db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " projection:" + (projection != null ? Arrays.toString(projection) : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? Arrays.toString(selectionArgs) : "") + " sortOrder:" + sortOrder);
        } else {
            Log.d(TAG, "query db null");
        }
        return db.query(table, projection, selection, selectionArgs, null, null, sortOrder);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        String table = getTableName(uri);
        SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
        if (db != null) {
            if (DEBUG) Log.d(TAG, "insert db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " values:" + (values != null ? values.toString() : ""));
        } else {
            Log.d(TAG, "insert db null");
        }
        db.insert(table, null, values);
        return null;
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        String table = getTableName(uri);
        SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
        if (db != null) {
            if (DEBUG) Log.d(TAG, "update db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " values:" + (values != null ? values.toString() : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? Arrays.toString(selectionArgs) : ""));
        } else {
            Log.d(TAG, "update db null");
        }
        return db.update(table, values, selection, selectionArgs);
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        String table = getTableName(uri);
        SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
        if (db != null) {
            if (DEBUG) Log.d(TAG, "delete db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? Arrays.toString(selectionArgs) : ""));
        } else {
            Log.d(TAG, "delete db null");
        }
        return db.delete(table, selection, selectionArgs);
    }

    @Override
    public String getType(final Uri uri) {
        return null;
    }

    private String getTableName(final Uri uri) {
        String tableName = "";
        int match = mUriMatcher.match(uri);
        switch (match) {
            case TABLE_SCAN_CODE:
                tableName = TABLE_SCAN_NAME;
                break;
            case TABLE_SOUND_CODE:
                tableName = TABLE_SOUND_NAME;
                break;
            case TABLE_PPPOE_CODE:
                tableName = TABLE_PPPOE_NAME;
                break;
            case TABLE_OTHERS_CODE:
                tableName = TABLE_OTHERS_NAME;
                break;
            case TABLE_PROP_CODE:
                tableName = TABLE_PROP_NAME;
                break;
            case TABLE_STRING_CODE:
                tableName = TABLE_STRING_NAME;
                break;
        }

        return tableName;
    }
}