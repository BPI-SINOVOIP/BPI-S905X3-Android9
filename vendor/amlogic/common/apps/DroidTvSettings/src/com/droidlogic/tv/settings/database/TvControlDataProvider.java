/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC TvControlDataProvider
 */

package com.droidlogic.tv.settings.database;

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

import com.droidlogic.app.tv.TvControlDataManager;

public class TvControlDataProvider extends ContentProvider {
    private static final String TAG = "TvControlDataProvider";
    private static final boolean DEBUG = true;

    private static final UriMatcher mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);
    private DbOpenHelper mDbOpenHelper = null;

    private static final String DB_NAME = TvControlDataManager.DB_NAME;
    private static final String TABLE_SCAN_NAME = TvControlDataManager.TABLE_SCAN_NAME;
    private static final String TABLE_SOUND_NAME = TvControlDataManager.TABLE_SCAN_NAME;
    private static final String TABLE_PPPOE_NAME = TvControlDataManager.TABLE_SCAN_NAME;
    private static final String TABLE_OTHERS_NAME = TvControlDataManager.TABLE_SCAN_NAME;

    private static final int DB_VERSION = TvControlDataManager.DB_VERSION;

    private static final int TABLE_SCAN_CODE = TvControlDataManager.TABLE_SCAN_CODE;
    private static final int TABLE_SOUND_CODE = TvControlDataManager.TABLE_SOUND_CODE;
    private static final int TABLE_PPPOE_CODE = TvControlDataManager.TABLE_PPPOE_CODE;
    private static final int TABLE_OTHERS_CODE = TvControlDataManager.TABLE_OTHERS_CODE;

    private static final String PROPERTY = TvControlDataManager.PROPERTY;
    private static final String VALUE = TvControlDataManager.VALUE;
    private static final String DESCRIP = TvControlDataManager.DESCRIP;
    private static final String TABLE_PROPERTY = TvControlDataManager.TABLE_PROPERTY;

    private static final String AUTHORITY = TvControlDataManager.AUTHORITY;
    private static final String CONTENT_URI = TvControlDataManager.CONTENT_URI;

    static {
        mUriMatcher.addURI(AUTHORITY, TABLE_SCAN_NAME, TABLE_SCAN_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_SOUND_NAME, TABLE_SOUND_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_PPPOE_NAME, TABLE_PPPOE_CODE);
        mUriMatcher.addURI(AUTHORITY, TABLE_OTHERS_NAME, TABLE_OTHERS_CODE);
    }

    private static class DbOpenHelper extends SQLiteOpenHelper {

        private final String SQL_CREATE_TABLE1 = "create table if not exists " + TABLE_SCAN_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE2 = "create table if not exists " + TABLE_SOUND_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE3 = "create table if not exists " + TABLE_PPPOE_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE4 = "create table if not exists " + TABLE_OTHERS_NAME + TABLE_PROPERTY;

        public DbOpenHelper(final Context context) {
            super(context, DB_NAME, null, DB_VERSION);
        }

        @Override
        public void onCreate(final SQLiteDatabase db) {
            db.execSQL(SQL_CREATE_TABLE1);
            db.execSQL(SQL_CREATE_TABLE2);
            db.execSQL(SQL_CREATE_TABLE3);
            db.execSQL(SQL_CREATE_TABLE4);

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
            if (DEBUG) Log.d(TAG, "query db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " projection:" + (projection != null ? projection.toString() : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? selectionArgs.toString() : "") + " sortOrder:" + sortOrder);
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
            if (DEBUG) Log.d(TAG, "update db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " values:" + (values != null ? values.toString() : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? selectionArgs.toString() : ""));
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
            if (DEBUG) Log.d(TAG, "delete db = " + db.getPath() + ", SQL uri:" + (uri != null ? uri.toString() : "") + " selection:" + selection + " selectionArgs:"  + (selectionArgs != null ? selectionArgs.toString() : ""));
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
        }

        return tableName;
    }
}