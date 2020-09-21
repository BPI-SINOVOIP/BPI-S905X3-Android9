/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

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

public class TvControlDataManager {
    private static final String TAG = "TvControlDataManager";
    private static final boolean DEBUG = false;

    public static final String DB_NAME = "tv_control_data.db";
    public static final String TABLE_SCAN_NAME = "tv_control_scan";
    public static final String TABLE_SOUND_NAME = "tv_control_sound";
    public static final String TABLE_PPPOE_NAME = "tv_control_pppoe";
    public static final String TABLE_OTHERS_NAME = "tv_control_others";

    public static final int DB_VERSION = 1;
    public static final String KEY_INIT = "tv_control_init";

    public static final int TABLE_SCAN_CODE = 1;
    public static final int TABLE_SOUND_CODE = 2;
    public static final int TABLE_PPPOE_CODE = 3;
    public static final int TABLE_OTHERS_CODE = 4;

    public static final String PROPERTY = "property";
    public static final String VALUE = "value";
    public static final String DESCRIP = "description";
    public static final String TABLE_PROPERTY = "(_id INTEGER PRIMARY KEY AUTOINCREMENT, " + PROPERTY + " TEXT UNIQUE ON CONFLICT REPLACE, " + VALUE + " TEXT, " + DESCRIP + " TEXT);";

    public static final String AUTHORITY = "com.droidlogic.tv.settings";
    public static final String CONTENT_URI = "content://" + AUTHORITY + "/";

    private ContentResolver mContentResolver = null;
    private Context mContext = null;

    private static TvControlDataManager mInstance = null;

    /*private static class DbOpenHelper extends SQLiteOpenHelper {

        private final String SQL_CREATE_TABLE1 = "create table if not exists " + TABLE_SCAN_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE2 = "create table if not exists " + TABLE_SOUND_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE3 = "create table if not exists " + TABLE_PPPOE_NAME + TABLE_PROPERTY;
        private final String SQL_CREATE_TABLE4 = "create table if not exists " + TABLE_OTHERS_NAME + TABLE_PROPERTY;

        public DbOpenHelper(final Context context) {
            super(context, DB_NAME, null, DB_VERSION);

            Log.d(TAG, "DbOpenHelper");
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

    private static class TvContentProvider extends ContentProvider {
        private static final UriMatcher mUriMatcher = new UriMatcher(UriMatcher.NO_MATCH);

        private Context mContext = null;
        private DbOpenHelper mDbOpenHelper = null;

        static {
            mUriMatcher.addURI(AUTHORITY, TABLE_SCAN_NAME, TABLE_SCAN_CODE);
            mUriMatcher.addURI(AUTHORITY, TABLE_SOUND_NAME, TABLE_SOUND_CODE);
            mUriMatcher.addURI(AUTHORITY, TABLE_PPPOE_NAME, TABLE_PPPOE_CODE);
            mUriMatcher.addURI(AUTHORITY, TABLE_OTHERS_NAME, TABLE_OTHERS_CODE);
        }

        public TvContentProvider(Context context) {
            mContext = context;
            mDbOpenHelper = new DbOpenHelper(mContext);

            Log.d(TAG, "mContext == null " + (mContext == null));
            Log.d(TAG, "mDbOpenHelper == null " + (mDbOpenHelper == null));
            Log.d(TAG, "TvContentProvider");
        }

        @Override
        public boolean onCreate() {
            //mContext = getContext();
            //mDatabase = new DbOpenHelper(mContext);

            Log.d(TAG, "onCreate");

            return true;
        }

        @Override
        public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs, String sortOrder) {
            String table = getTableName(uri);
            SQLiteDatabase db = mDbOpenHelper.getReadableDatabase();
            return db.query(table, projection, selection, selectionArgs, null, sortOrder, null);
        }

        @Override
        public Uri insert(Uri uri, ContentValues values) {
            String table = getTableName(uri);
            SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
            db.insert(table, null, values);
            return null;
        }

        @Override
        public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
            String table = getTableName(uri);
            SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
            return db.update(table, values, selection, selectionArgs);
        }

        @Override
        public int delete(Uri uri, String selection, String[] selectionArgs) {
            String table = getTableName(uri);
            SQLiteDatabase db = mDbOpenHelper.getWritableDatabase();
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
    }*/

    private TvControlDataManager(Context context) {
        mContext = context;
        mContentResolver = mContext.getContentResolver();
    }

    public synchronized static TvControlDataManager getInstance(Context context) {
        if (null == mInstance) {
            mInstance = new TvControlDataManager(context);
        }

        return mInstance;
    }

    /*private Cursor isExist(final Uri uri, String name) {
        Cursor cursor = null;

        synchronized (TvControlDataManager.class) {
            cursor = mContentResolver.query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
        }
        if (cursor != null) {
            if (cursor.moveToNext() != null) {
                return cursor;
            }
            cursor.close();
        }
        cursor = null;
        return  null;
    }*/

    public boolean putString(ContentResolver resolver, String name, String value) {
        /*if (DEBUG) Log.d(TAG, "putString  name = " + name + ", value = " + value);
        Uri uri = Uri.parse(CONTENT_URI + TABLE_SCAN_NAME);
        ContentValues values = new ContentValues();
        values.put(PROPERTY, name);
        values.put(VALUE, value);
        synchronized (TvControlDataManager.class) {
            if (DEBUG) Log.d(TAG,"putString synchronized");
        }
        Cursor cursor = null;
        try {
            cursor = mContentResolver.query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                if (DEBUG) Log.d(TAG, "update row = " + mContentResolver.update(uri, values, PROPERTY + " = ?", new String[] { name }));
            } else {
                mContentResolver.insert(uri, values);
            }
        } catch (Exception e) {
            Log.e(TAG,"putString Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }*/

        return Settings.System.putString(resolver, name, value);
    }

    public String getString(ContentResolver resolver, String name) {
        /*String result = null;
        Uri uri = Uri.parse(CONTENT_URI + TABLE_SCAN_NAME);
        synchronized (TvControlDataManager.class) {
            if (DEBUG) Log.d(TAG,"getString synchronized");
        }
        Cursor cursor = null;
        try {
            cursor = mContentResolver.query(uri, new String[] { PROPERTY, VALUE}, PROPERTY + " = ?",
                    new String[]{ name }, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                result = cursor.getString(cursor.getColumnIndex(VALUE));
            }
        } catch (Exception e) {
            Log.e(TAG,"getString Exception  = " + e.getMessage());
            e.printStackTrace();
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DEBUG) Log.d(TAG,"getString  name = " + name + ", value = " + result);
        return result;*/

        return Settings.System.getString(resolver, name);

    }

    public boolean putInt(ContentResolver resolver, String name, int value) {
        //return putString(resolver, name, String.valueOf(value));
        return Settings.System.putInt(resolver, name, value);
    }

    public int getInt(ContentResolver resolver, String name, int def) {
        /*String value = getString(resolver, name);
        if (value != null) {
            return Integer.parseInt(value);
        }

        return def;*/
        return Settings.System.getInt(resolver, name, def);
    }

    public boolean putLong(ContentResolver resolver, String name, long value) {
        //return Settings.System.getInt(resolver, name, String.valueOf(value));
        return Settings.System.putLong(resolver, name, value);
    }

    public long getLong(ContentResolver resolver, String name, long def) {
        /*String value = getString(resolver, name);
        if (value != null) {
            return Integer.parseInt(value);
        }

        return def;*/

        return Settings.System.getLong(resolver, name, def);
    }

    public boolean putFloat(ContentResolver resolver, String name, float value) {
        //return putString(resolver, name, String.valueOf(value));
        return Settings.System.putFloat(resolver, name, value);
    }

    public float getFloat(ContentResolver resolver, String name, float def) {
        /*String value = getString(resolver, name);
        if (value != null) {
            return Float.parseFloat(value);
        }

        return def;*/

        return Settings.System.getFloat(resolver, name, def);
    }

    public boolean putBoolean(ContentResolver resolver, String name, boolean value) {
        //return putString(resolver, name, value ? "1" : "0");
        return Settings.System.putInt(resolver, name, value ? 1 : 0);
    }

    public boolean getBoolean(ContentResolver resolver, String name, boolean def) {
        /*String value = getString(resolver, name);
        if (value != null) {
            return (Integer.parseInt(value) != 0) ? true : false;
        }

        return def;*/
        int result = Settings.System.getInt(resolver, name, def ? 1 : 0);
        return result == 1 ;
    }
}
