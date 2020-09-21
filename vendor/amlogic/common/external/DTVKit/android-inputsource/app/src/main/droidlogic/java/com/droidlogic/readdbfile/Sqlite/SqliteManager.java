package com.droidlogic.readdbfile.Sqlite;

import android.content.ContentValues;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.util.Log;

import java.io.File;
import java.util.Arrays;

public class SqliteManager {

    private final static String TAG = SqliteManager.class.getSimpleName();
    public static final String DB_PATH = "/sdcard/tv.db";
    public static final String DESTINATION_DB_PATH = "/data/data/com.android.providers.tv/databases/tv.db";
    private SQLiteDatabase mSQLiteDatabase;

    public SqliteManager() {

    }

    public boolean setDb(SQLiteDatabase db) {
        Log.d(TAG, "setDb");
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen()) {
            mSQLiteDatabase.close();
        }
        mSQLiteDatabase = db;
        return mSQLiteDatabase != null;
    }

    public boolean openDb(String dbPath) {
        Log.d(TAG, "openDb " + dbPath);
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen()) {
            mSQLiteDatabase.close();
        }
        mSQLiteDatabase = null;
        if (dbPath != null && dbPath.endsWith(".db") && new File(dbPath).exists()) {
            mSQLiteDatabase = SQLiteDatabase.openOrCreateDatabase(dbPath, null);
        }
        return mSQLiteDatabase != null;
    }

    public void closeDb() {
        Log.d(TAG, "closeDb");
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen()) {
            mSQLiteDatabase.close();
            mSQLiteDatabase = null;
        }
    }

    public void createTable() {
        Log.d(TAG, "createTable");
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen()) {
            String stu_table="create table usertable(_id integer primary key autoincrement,name text,number text)";
            mSQLiteDatabase.execSQL(stu_table);
        }
    }

    public void dropTable(String table) {
        Log.d(TAG, "dropTable " + table);
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen() && table != null) {
            String sql ="DROP TABLE " + table;
            mSQLiteDatabase.execSQL(sql);
        }
    }

    public boolean insertData(String table, ContentValues values) {
        Log.d(TAG, "insertData " + table);
        long ret = -1;
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen() && table != null && values != null && values.size() > 0) {
            ret = mSQLiteDatabase.insert(table, null, values);
        }
        return ret != -1;
    }

    public boolean deleteData(String table, long id) {
        Log.d(TAG, "deleteData " + table + " " + id);
        long ret = -1;
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen() && table != null) {
            String whereClause = "id=?";
            String[] whereArgs = {String.valueOf(id)};
            ret = mSQLiteDatabase.delete(table, whereClause, whereArgs);
        }
        return ret >= 1;
    }

    /*String whereClause = "id=?";
    *String[] whereArgs={String.valuesOf(1)};
    */
    public boolean updateData(String table, ContentValues values, String whereClause, String[] whereArgs) {
        Log.d(TAG, "updateData " + table + " " + whereClause + "=" + Arrays.toString(whereArgs));
        long ret = -1;
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen() && table != null && values != null && values.size() > 0) {
            ret = mSQLiteDatabase.update(table,values,whereClause,whereArgs);
        }
        return ret >= 1;
    }

    public Cursor queryData(String table, String[] columns, String selection, String[] selectionArgs) {
        Log.d(TAG, "queryData " + table + " " + Arrays.toString(columns) + " " + selection + "=" + Arrays.toString(selectionArgs));
        return queryData(table, columns, selection, selectionArgs, null, null, null, null);
    }

    private Cursor queryData(String table, String[] columns, String selection, String[] selectionArgs, String groupBy, String having, String orderBy, String limit) {
        Cursor cursor = null;
        if (mSQLiteDatabase != null && mSQLiteDatabase.isOpen() && table != null) {
            cursor = mSQLiteDatabase.query (table,columns,selection,selectionArgs,groupBy,having,orderBy, limit);
        }
        return cursor;
    }
}
