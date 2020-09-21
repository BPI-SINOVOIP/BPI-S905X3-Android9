/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import java.io.File;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteException;
import android.database.sqlite.SQLiteFullException;
import android.database.sqlite.SQLiteOpenHelper;
import android.net.Uri;

import com.android.ims.internal.Logger;

public abstract class DatabaseContentProvider extends ContentProvider {
    static private Logger logger = Logger.getLogger("DatabaseContentProvider");

    //Constants
    public static final String ACTION_DEVICE_STORAGE_FULL = "com.android.vmm.DEVICE_STORAGE_FULL";

    //Fields
    protected SQLiteOpenHelper mDbHelper;
    /*package*/final int mDbVersion;
    private final String mDbName;

    /**
     * Initializes the DatabaseContentProvider
     * @param dbName the filename of the database
     * @param dbVersion the current version of the database schema
     * @param contentUri The base Uri of the syncable content in this provider
     */
    public DatabaseContentProvider(String dbName, int dbVersion) {
        super();
        mDbName = dbName;
        mDbVersion = dbVersion;
    }

    /**
     * bootstrapDatabase() allows the implementer to set up their database
     * after it is opened for the first time.  this is a perfect place
     * to create tables and triggers :)
     * @param db
     */
    protected void bootstrapDatabase(SQLiteDatabase db) {
    }

    /**
     * updgradeDatabase() allows the user to do whatever they like
     * when the database is upgraded between versions.
     * @param db - the SQLiteDatabase that will be upgraded
     * @param oldVersion - the old version number as an int
     * @param newVersion - the new version number as an int
     * @return
     */
    protected abstract boolean upgradeDatabase(SQLiteDatabase db, int oldVersion, int newVersion);

    /**
     * downgradeDatabase() allows the user to do whatever they like when the
     * database is downgraded between versions.
     *
     * @param db - the SQLiteDatabase that will be downgraded
     * @param oldVersion - the old version number as an int
     * @param newVersion - the new version number as an int
     * @return
     */
    protected abstract boolean downgradeDatabase(SQLiteDatabase db, int oldVersion, int newVersion);

    /**
     * Safely wraps an ALTER TABLE table ADD COLUMN columnName columnType
     * If columnType == null then it's set to INTEGER DEFAULT 0
     * @param db - db to alter
     * @param table - table to alter
     * @param columnDef
     * @return
     */
    protected static boolean addColumn(SQLiteDatabase db, String table, String columnName,
            String columnType) {
        StringBuilder sb = new StringBuilder();
        sb.append("ALTER TABLE ").append(table).append(" ADD COLUMN ").append(columnName).append(
                ' ').append(columnType == null ? "INTEGER DEFAULT 0" : columnType).append(';');
        try {
            db.execSQL(sb.toString());
        } catch (SQLiteException e) {
                logger.debug("Alter table failed : "+ e.getMessage());
            return false;
        }
        return true;
    }

    /**
     * onDatabaseOpened() allows the user to do whatever they might
     * need to do whenever the database is opened
     * @param db - SQLiteDatabase that was just opened
     */
    protected void onDatabaseOpened(SQLiteDatabase db) {
    }

    private class DatabaseHelper extends SQLiteOpenHelper {
        private File mDatabaseFile = null;

        DatabaseHelper(Context context, String name) {
            // Note: context and name may be null for temp providers
            super(context, name, null, mDbVersion);
            mDatabaseFile = context.getDatabasePath(name);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            bootstrapDatabase(db);
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            upgradeDatabase(db, oldVersion, newVersion);
        }

        @Override
        public void onDowngrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            logger.debug("Enter: onDowngrade() - oldVersion = " + oldVersion + " newVersion = "
                    + newVersion);
            downgradeDatabase(db, oldVersion, newVersion);
        }

        @Override
        public void onOpen(SQLiteDatabase db) {
            onDatabaseOpened(db);
        }

        @Override
        public synchronized SQLiteDatabase getWritableDatabase() {
            try {
                return super.getWritableDatabase();
            } catch (Exception e) {
                logger.error("getWritableDatabase exception " + e);
            }

            // try to delete the database file
            if (null != mDatabaseFile) {
                logger.error("deleting mDatabaseFile.");
                mDatabaseFile.delete();
            }

            // Return a freshly created database.
            return super.getWritableDatabase();
        }

        @Override
        public synchronized SQLiteDatabase getReadableDatabase() {
            try {
                return super.getReadableDatabase();
            } catch (Exception e) {
                logger.error("getReadableDatabase exception " + e);
            }

            // try to delete the database file
            if (null != mDatabaseFile) {
                logger.error("deleting mDatabaseFile.");
                mDatabaseFile.delete();
            }

            // Return a freshly created database.
            return super.getReadableDatabase();
        }
    }

    /**
     * deleteInternal allows getContentResolver().delete() to occur atomically
     * via transactions and notify the uri automatically upon completion (provided
     * rows were deleted) - otherwise, it functions exactly as getContentResolver.delete()
     * would on a regular ContentProvider
     * @param uri - uri to delete from
     * @param selection - selection used for the uri
     * @param selectionArgs - selection args replacing ?'s in the selection
     * @return returns the number of rows deleted
     */
    protected abstract int deleteInternal(final SQLiteDatabase db, Uri uri, String selection,
            String[] selectionArgs);

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        int result = 0;
        SQLiteDatabase db = mDbHelper.getWritableDatabase();
        if (isClosed(db)) {
            return result;
        }
        try {
            //acquire reference to prevent from garbage collection
            db.acquireReference();
            //beginTransaction can throw a runtime exception
            //so it needs to be moved into the try
            db.beginTransaction();
            result = deleteInternal(db, uri, selection, selectionArgs);
            db.setTransactionSuccessful();
        } catch (SQLiteFullException fullEx) {
            logger.error("" + fullEx);
            sendStorageFullIntent(getContext());
        } catch (Exception e) {
            logger.error("" + e);
        } finally {
            try {
                db.endTransaction();
            } catch (SQLiteFullException fullEx) {
                logger.error("" + fullEx);
                sendStorageFullIntent(getContext());
            } catch (Exception e) {
                logger.error("" + e);
            }
            //release reference
            db.releaseReference();
        }
        // don't check return value because it may be 0 if all rows deleted
        getContext().getContentResolver().notifyChange(uri, null);
        return result;
    }

    /**
     * insertInternal allows getContentResolver().insert() to occur atomically
     * via transactions and notify the uri automatically upon completion (provided
     * rows were added to the db) - otherwise, it functions exactly as getContentResolver().insert()
     * would on a regular ContentProvider
     * @param uri - uri on which to insert
     * @param values - values to insert
     * @return returns the uri of the row added
     */
    protected abstract Uri insertInternal(final SQLiteDatabase db, Uri uri, ContentValues values);

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        Uri result = null;
        SQLiteDatabase db = mDbHelper.getWritableDatabase();
        if (isClosed(db)) {
            return result;
        }
        try {
            db.acquireReference();
            //beginTransaction can throw a runtime exception
            //so it needs to be moved into the try
            db.beginTransaction();
            result = insertInternal(db, uri, values);
            db.setTransactionSuccessful();
        } catch (SQLiteFullException fullEx) {
            logger.warn("" + fullEx);
            sendStorageFullIntent(getContext());
        } catch (Exception e) {
            logger.warn("" + e);
        } finally {
            try {
                db.endTransaction();
            } catch (SQLiteFullException fullEx) {
                logger.warn("" + fullEx);
                sendStorageFullIntent(getContext());
            } catch (Exception e) {
                logger.warn("" + e);
            }
            db.releaseReference();
        }
        if (result != null) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return result;
    }

    @Override
    public boolean onCreate() {
        mDbHelper = new DatabaseHelper(getContext(), mDbName);
        return onCreateInternal();
    }

    /**
     * Called by onCreate.  Should be overridden by any subclasses
     * to handle the onCreate lifecycle event.
     *
     * @return
     */
    protected boolean onCreateInternal() {
        return true;
    }

    /**
     * queryInternal allows getContentResolver().query() to occur
     * @param uri
     * @param projection
     * @param selection
     * @param selectionArgs
     * @param sortOrder
     * @return Cursor holding the contents of the requested query
     */
    protected abstract Cursor queryInternal(final SQLiteDatabase db, Uri uri, String[] projection,
            String selection, String[] selectionArgs, String sortOrder);

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        SQLiteDatabase db = mDbHelper.getReadableDatabase();
        if (isClosed(db)) {
            return null;
        }

        try {
            db.acquireReference();
            return queryInternal(db, uri, projection, selection, selectionArgs, sortOrder);
        } finally {
            db.releaseReference();
        }
    }

    protected abstract int updateInternal(final SQLiteDatabase db, Uri uri, ContentValues values,
            String selection, String[] selectionArgs);

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        int result = 0;
        SQLiteDatabase db = mDbHelper.getWritableDatabase();
        if (isClosed(db)) {
            return result;
        }
        try {
            db.acquireReference();
            //beginTransaction can throw a runtime exception
            //so it needs to be moved into the try
            db.beginTransaction();
            result = updateInternal(db, uri, values, selection, selectionArgs);
            db.setTransactionSuccessful();
        } catch (SQLiteFullException fullEx) {
            logger.error("" + fullEx);
            sendStorageFullIntent(getContext());
        } catch (Exception e) {
            logger.error("" + e);
        } finally {
            try {
                db.endTransaction();
            } catch (SQLiteFullException fullEx) {
                logger.error("" + fullEx);
                sendStorageFullIntent(getContext());
            } catch (Exception e) {
                logger.error("" + e);
            }
            db.releaseReference();
        }
        if (result > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
        }
        return result;
    }

    @Override
    public int bulkInsert(Uri uri, ContentValues[] values) {
        int added = 0;
        if (values != null) {
            int numRows = values.length;
            SQLiteDatabase db = mDbHelper.getWritableDatabase();
            if (isClosed(db)) {
                return added;
            }
            try {
                db.acquireReference();
                //beginTransaction can throw a runtime exception
                //so it needs to be moved into the try
                db.beginTransaction();

                for (int i = 0; i < numRows; i++) {
                    if (insertInternal(db, uri, values[i]) != null) {
                        added++;
                    }
                }
                db.setTransactionSuccessful();
                if (added > 0) {
                    getContext().getContentResolver().notifyChange(uri, null);
                }
            } catch (SQLiteFullException fullEx) {
                logger.error("" + fullEx);
                sendStorageFullIntent(getContext());
            } catch (Exception e) {
                logger.error("" + e);
            } finally {
                try {
                    db.endTransaction();
                } catch (SQLiteFullException fullEx) {
                    logger.error("" + fullEx);
                    sendStorageFullIntent(getContext());
                } catch (Exception e) {
                    logger.error("" + e);
                }
                db.releaseReference();
            }
        }
        return added;
    }

    private void sendStorageFullIntent(Context context) {
        Intent fullStorageIntent = new Intent(ACTION_DEVICE_STORAGE_FULL);
        context.sendBroadcast(fullStorageIntent);
    }

    private boolean isClosed(SQLiteDatabase db) {
        if (db == null || !db.isOpen()) {
            logger.warn("Null DB returned from DBHelper for a writable/readable database.");
            return true;
        }
        return false;
    }

}
