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

import java.io.FileNotFoundException;

import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Intent;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.SQLException;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.BaseColumns;
import android.provider.ContactsContract.Contacts;
import android.content.ComponentName;

import com.android.ims.internal.ContactNumberUtils;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.internal.EABContract;
import com.android.ims.internal.Logger;

/**
 * @author Vishal Patil (A22809)
 */

public class EABProvider extends DatabaseContentProvider{
    private Logger logger = Logger.getLogger(this.getClass().getName());

    private static final String EAB_DB_NAME = "rcseab.db";

    private static final int EAB_DB_VERSION = 4;

    private static final int EAB_TABLE = 1;

    private static final int EAB_TABLE_ID = 2;

    private static final int EAB_GROUPITEMS_TABLE = 3;

    private static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH) {
        {
            addURI(EABContract.AUTHORITY, EABContract.EABColumns.TABLE_NAME, EAB_TABLE);
            addURI(EABContract.AUTHORITY, EABContract.EABColumns.TABLE_NAME + "/#", EAB_TABLE_ID);
            addURI(EABContract.AUTHORITY, EABContract.EABColumns.GROUPITEMS_NAME,
                    EAB_GROUPITEMS_TABLE);
        }
    };

    /* Statement to create VMM Album table. */
    private static final String EAB_CREATE_STATEMENT = "create table if not exists "
            + EABContract.EABColumns.TABLE_NAME
            + "("
            + EABContract.EABColumns._ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
            + EABContract.EABColumns.CONTACT_NAME + " TEXT, "
            + EABContract.EABColumns.CONTACT_NUMBER + " TEXT, "
            + EABContract.EABColumns.RAW_CONTACT_ID + " LONG, "
            + EABContract.EABColumns.CONTACT_ID + " LONG, "
            + EABContract.EABColumns.DATA_ID + " LONG, "
            + EABContract.EABColumns.ACCOUNT_TYPE  + " TEXT, "
            + EABContract.EABColumns.VOLTE_CALL_SERVICE_CONTACT_ADDRESS + " TEXT, "
            + EABContract.EABColumns.VOLTE_CALL_CAPABILITY + " INTEGER, "
            + EABContract.EABColumns.VOLTE_CALL_CAPABILITY_TIMESTAMP + " LONG, "
            + EABContract.EABColumns.VOLTE_CALL_AVAILABILITY + " INTEGER, "
            + EABContract.EABColumns.VOLTE_CALL_AVAILABILITY_TIMESTAMP + " LONG, "
            + EABContract.EABColumns.VIDEO_CALL_SERVICE_CONTACT_ADDRESS + " TEXT, "
            + EABContract.EABColumns.VIDEO_CALL_CAPABILITY + " INTEGER, "
            + EABContract.EABColumns.VIDEO_CALL_CAPABILITY_TIMESTAMP + " LONG, "
            + EABContract.EABColumns.VIDEO_CALL_AVAILABILITY + " INTEGER, "
            + EABContract.EABColumns.VIDEO_CALL_AVAILABILITY_TIMESTAMP + " LONG, "
            + EABContract.EABColumns.CONTACT_LAST_UPDATED_TIMESTAMP + " LONG "
            + ");";

    private static final String EAB_DROP_STATEMENT = "drop table if exists "
            + EABContract.EABColumns.TABLE_NAME + ";";

    public EABProvider() {
        super(EAB_DB_NAME, EAB_DB_VERSION);
    }

    @Override
    public void bootstrapDatabase(SQLiteDatabase db) {
        logger.info("Enter: bootstrapDatabase() Creating new EAB database");
        upgradeDatabase(db, 0, EAB_DB_VERSION);
        logger.debug("Exit: bootstrapDatabase()");
    }

    /*
     * In upgradeDatabase,
     * oldVersion - the current version of Provider on the phone.
     * newVersion - the version of Provider where the phone will be upgraded to.
     */
    @Override
    public boolean upgradeDatabase(SQLiteDatabase db, int oldVersion, int newVersion) {
        logger.info("Enter: upgradeDatabase() - oldVersion = " + oldVersion +
                " newVersion = " + newVersion);

        boolean needsEabResetBroadcast = false;

        if (oldVersion == newVersion) {
            logger.info("upgradeDatabase oldVersion == newVersion, No Upgrade Required");
            return true;
        }

        try {
            if (oldVersion == 0) {
                db.execSQL(EAB_CREATE_STATEMENT);

                oldVersion++;
                logger.debug("upgradeDatabase : DB has been upgraded to " + oldVersion);
            }
            if (oldVersion == 1) {
                addColumn(db, EABContract.EABColumns.TABLE_NAME,
                        EABContract.EABColumns.VOLTE_STATUS, "INTEGER NOT NULL DEFAULT -1");

                oldVersion++;
                logger.debug("upgradeDatabase : DB has been upgraded to " + oldVersion);
            }
            if (oldVersion == 2) {
                // Add new column to EABPresence table to handle special numbers *67 and *82.
                addColumn(db, EABContract.EABColumns.TABLE_NAME,
                        EABContract.EABColumns.FORMATTED_NUMBER, "TEXT DEFAULT NULL");

                // Delete all records from EABPresence table.
                db.execSQL("DELETE FROM " + EABContract.EABColumns.TABLE_NAME);
                needsEabResetBroadcast = true;

                oldVersion++;
                logger.debug("upgradeDatabase : DB has been upgraded to " + oldVersion);
            }
            if (oldVersion == 3) {
                // Delete all records from EABPresence table. A bug in the previous version caused
                // invalid rows to be created. This version removes all rows to allow the DB to
                // repopulate.
                db.execSQL("DELETE FROM " + EABContract.EABColumns.TABLE_NAME);
                needsEabResetBroadcast = true;

                oldVersion++;
                logger.debug("upgradeDatabase : DB has been upgraded to " + oldVersion);
            }
            // add further upgrade code above this
        } catch (SQLException exception) {
            logger.error("Exception during upgradeDatabase. " + exception.getMessage());
            // If exception occured during upgrade database, then drop EABPresence
            // and recreate it. Please note in this case, all information stored in
            // table is lost.
            db.execSQL(EAB_DROP_STATEMENT);
            upgradeDatabase(db, 0, EAB_DB_VERSION);
            needsEabResetBroadcast = true;
            logger.debug("Dropped and created new EABPresence table.");
        }

        if (needsEabResetBroadcast) {
            sendEabResetBroadcast();
        }

        if (oldVersion == newVersion) {
            logger.debug("DB upgrade complete : to " + newVersion);
        }

        logger.debug("Exit: upgradeDatabase()");
        return true;
    }

    @Override
    protected boolean downgradeDatabase(SQLiteDatabase db, int oldVersion, int newVersion) {
        logger.info("Enter: downgradeDatabase()");
        // Drop and recreate EABPresence table as there should not be a scenario
        // where EAB table version is greater than EAB_DB_VERSION.
        db.execSQL(EAB_DROP_STATEMENT);
        upgradeDatabase(db, 0, EAB_DB_VERSION);
        sendEabResetBroadcast();
        logger.debug("Dropped and created new EABPresence table.");
        logger.debug("Exit: downgradeDatabase()");
        return true;
    }

    @Override
    protected int deleteInternal(SQLiteDatabase db, Uri uri, String selection,
            String[] selectionArgs) {
        logger.debug("Enter: deleteInternal()");
        final int match = URI_MATCHER.match(uri);
        String table = null;
        switch (match) {
            case EAB_TABLE_ID:
                if (selection == null) {
                    selection = "_id=?";
                    selectionArgs = new String[] {uri.getPathSegments().get(1)};
                }
            case EAB_TABLE:
                table = EABContract.EABColumns.TABLE_NAME;
                break;
            default:
                logger.debug("No match for " + uri);
                logger.debug("Exit: deleteInternal()");
                return 0;
        }
        logger.debug("Deleting from the table" + table + " selection= " + selection);
        printDeletingValues(uri, selection, selectionArgs);
        logger.debug("Exit: deleteInternal()");
        return db.delete(table, selection, selectionArgs);
    }

    @Override
    protected Uri insertInternal(SQLiteDatabase db, Uri uri, ContentValues values) {
        logger.debug("Enter: insertInternal()");
        final int match = URI_MATCHER.match(uri);
        String table = null;
        String nullColumnHack = null;
        switch (match) {
            case EAB_TABLE:
                table = EABContract.EABColumns.TABLE_NAME;
                break;
            default:
                logger.warn("No match for " + uri);
                logger.debug("Exit: insertInternal() with null");
                return null;
        }
        values = verifyIfMdnExists(values);
        // Do the insert.
        logger.debug("Inserting to the table" + table + " values=" + values.toString());

        final long id = db.insert(table, nullColumnHack, values);
        if (id > 0) {
            String contactNumber = values.getAsString(EABContract.EABColumns.CONTACT_NUMBER);
            sendInsertBroadcast(contactNumber);
            logger.debug("Exit: insertInternal()");
            return ContentUris.withAppendedId(uri, id);
        } else {
            logger.debug("Exit: insertInternal() with null");
            return null;
        }
    }

    @Override
    protected Cursor queryInternal(SQLiteDatabase db, Uri uri, String[] projection,
            String selection, String[] selectionArgs, String sortOrder) {
        logger.debug("Enter: queryInternal()");
        final int match = URI_MATCHER.match(uri);

        switch (match) {
            case EAB_TABLE_ID:
                long id = ContentUris.parseId(uri);
                logger.debug("queryInternal id=" + id);
                String idSelection = BaseColumns._ID + "=" + id;
                if(null != selection) {
                    selection = "((" + idSelection + ") AND (" + selection + "))";
                } else {
                    selection = idSelection;
                }
                break;
            case EAB_GROUPITEMS_TABLE:
                SQLiteQueryBuilder sqb = new SQLiteQueryBuilder();
                sqb.setTables(EABContract.EABColumns.TABLE_NAME);
                String rawquery = "select DISTINCT " + EABContract.EABColumns.CONTACT_ID
                        + " from " + EABContract.EABColumns.TABLE_NAME
                        + " where " + EABContract.EABColumns.VOLTE_CALL_CAPABILITY + " >'" +
                        RcsPresenceInfo.ServiceState.OFFLINE+"' AND "
                        + EABContract.EABColumns.VIDEO_CALL_CAPABILITY + " >'"
                        + RcsPresenceInfo.ServiceState.OFFLINE + "'";
                StringBuffer sb = new StringBuffer();
                Cursor cursor = db.rawQuery(rawquery, null);
                if (cursor != null && cursor.moveToFirst()) {
                    do {
                        if (sb.length() != 0) sb.append(",");
                        String contactId = cursor.getString(cursor
                                .getColumnIndex(EABContract.EABColumns.CONTACT_ID));
                        sb.append(contactId);
                    } while (cursor.moveToNext());
                }
                if (cursor != null) cursor.close();
                String contactSel = Contacts._ID + " IN ( " + sb.toString() + ")";
                return getContext().getContentResolver().query(Contacts.CONTENT_URI, projection,
                                contactSel, null, sortOrder);
        }

        SQLiteQueryBuilder qb = new SQLiteQueryBuilder();

        String groupBy = uri.getQueryParameter("groupby");
        String having = null;

        switch (match) {
            case EAB_TABLE:
            case EAB_TABLE_ID:
                qb.setTables(EABContract.EABColumns.TABLE_NAME);
                break;
            default:
                logger.warn("No match for " + uri);
                logger.debug("Exit: queryInternal()");
                return null;
        }
        logger.debug("Exit: queryInternal()");
        return qb.query(db, projection, selection, selectionArgs, groupBy, having, sortOrder);
    }

    @Override
    protected int updateInternal(SQLiteDatabase db, Uri uri, ContentValues values,
            String selection, String[] selectionArgs) {
        logger.debug("Enter: updateInternal()");
        int result = 0;
        final int match = URI_MATCHER.match(uri);

        switch (match) {
            case EAB_TABLE_ID:
                long id = ContentUris.parseId(uri);
                logger.debug("updateInternal id=" + id);
                String idSelection = BaseColumns._ID + "=" + id;
                if(null != selection){
                    selection = "((" + idSelection + ") AND (" + selection + "))";
                } else {
                    selection = idSelection;
                }
                break;
        }

        String table = null;
        switch (match) {
            case EAB_TABLE:
            case EAB_TABLE_ID:
                table = EABContract.EABColumns.TABLE_NAME;
                break;
            default:
                logger.warn("No match for " + uri);
                break;
        }

        if (table != null && values != null) {
            logger.debug("Updating the table " + table + " values= " + values.toString());
            result = db.update(table, values, selection, selectionArgs);
        }
        logger.debug("Exit: updateInternal()");
        return result;
    }

    @Override
    public String getType(Uri uri) {
        logger.debug("Enter: getType()");
        final int match = URI_MATCHER.match(uri);
        switch (match) {
            case EAB_TABLE:
                return EABContract.EABColumns.CONTENT_TYPE;
            case EAB_TABLE_ID:
                return EABContract.EABColumns.CONTENT_ITEM_TYPE;
            default:
                throw (new IllegalArgumentException("EABProvider URI: " + uri));
        }
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        return null;
    }

    private void sendInsertBroadcast(String contactNumber) {
        Intent intent = new Intent(com.android.service.ims.presence.Contacts
            .ACTION_NEW_CONTACT_INSERTED);
        ComponentName component = new ComponentName("com.android.service.ims.presence",
                "com.android.service.ims.presence.AlarmBroadcastReceiver");
        intent.setComponent(component);

        intent.putExtra(com.android.service.ims.presence.Contacts.NEW_PHONE_NUMBER, contactNumber);
        getContext().sendBroadcast(intent);
    }

    private void sendEabResetBroadcast() {
        logger.info("Enter: sendEabResetBroadcast()");
        Intent intent = new Intent(com.android.service.ims.presence.Contacts
                .ACTION_EAB_DATABASE_RESET);
        getContext().sendBroadcast(intent, "com.android.ims.permission.PRESENCE_ACCESS");
    }

    private ContentValues verifyIfMdnExists(ContentValues cvalues) {
        String formattedNumber = null;
        String phoneNumber = null;
        if (cvalues.containsKey(EABContract.EABColumns.CONTACT_NUMBER)
                && cvalues.containsKey(EABContract.EABColumns.FORMATTED_NUMBER)) {
            phoneNumber = cvalues.getAsString(EABContract.EABColumns.CONTACT_NUMBER);
            formattedNumber = cvalues.getAsString(EABContract.EABColumns.FORMATTED_NUMBER);
        } else {
            return cvalues;
        }
        if (null == formattedNumber) {
            return cvalues;
        }
        String[] projection = new String[] {
                EABContract.EABColumns.FORMATTED_NUMBER,
                EABContract.EABColumns.VOLTE_CALL_SERVICE_CONTACT_ADDRESS,
                EABContract.EABColumns.VOLTE_CALL_CAPABILITY,
                EABContract.EABColumns.VOLTE_CALL_CAPABILITY_TIMESTAMP,
                EABContract.EABColumns.VOLTE_CALL_AVAILABILITY,
                EABContract.EABColumns.VOLTE_CALL_AVAILABILITY_TIMESTAMP,
                EABContract.EABColumns.VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
                EABContract.EABColumns.VIDEO_CALL_CAPABILITY,
                EABContract.EABColumns.VIDEO_CALL_CAPABILITY_TIMESTAMP,
                EABContract.EABColumns.VIDEO_CALL_AVAILABILITY,
                EABContract.EABColumns.VIDEO_CALL_AVAILABILITY_TIMESTAMP};
        String whereClause = "PHONE_NUMBERS_EQUAL("
                + EABContract.EABColumns.FORMATTED_NUMBER + ", ?, 0)";
        String[] selectionArgs = new String[] { formattedNumber };
        Cursor cursor = getContext().getContentResolver().query(EABContract.EABColumns.CONTENT_URI,
                projection, whereClause, selectionArgs, null);
        if ((null != cursor) && (cursor.getCount() > 0)) {
            logger.debug("Cursor count is " + cursor.getCount());
            // Update data only from first valid cursor element.
            while (cursor.moveToNext()) {
                ContactNumberUtils contactNumberUtils = ContactNumberUtils.getDefault();
                String eabFormattedNumber = cursor.getString(
                        cursor.getColumnIndex(EABContract.EABColumns.FORMATTED_NUMBER));

                // Use contactNumberUtils.format() to format both formattedNumber and
                // eabFormattedNumber and then check if they are equal.
                if(contactNumberUtils.format(formattedNumber).equals(
                        contactNumberUtils.format(eabFormattedNumber))) {
                    logger.debug("phoneNumber : "+ phoneNumber +" is already stored in EAB DB. "
                            + " Hence inserting another copy.");
                    cvalues.put(EABContract.EABColumns.VOLTE_CALL_SERVICE_CONTACT_ADDRESS,
                            cursor.getString(cursor.getColumnIndex(
                                  EABContract.EABColumns.VOLTE_CALL_SERVICE_CONTACT_ADDRESS)));
                    cvalues.put(EABContract.EABColumns.VOLTE_CALL_CAPABILITY, cursor.getString(
                            cursor.getColumnIndex(
                                    EABContract.EABColumns.VOLTE_CALL_CAPABILITY)));
                    cvalues.put(EABContract.EABColumns.VOLTE_CALL_CAPABILITY_TIMESTAMP,
                            cursor.getLong(cursor.getColumnIndex(
                                    EABContract.EABColumns.VOLTE_CALL_CAPABILITY_TIMESTAMP)));
                    cvalues.put(EABContract.EABColumns.VOLTE_CALL_AVAILABILITY,
                            cursor.getString(cursor.getColumnIndex(
                                    EABContract.EABColumns.VOLTE_CALL_AVAILABILITY)));
                    cvalues.put(EABContract.EABColumns.VOLTE_CALL_AVAILABILITY_TIMESTAMP,
                            cursor.getLong(cursor.getColumnIndex(
                                   EABContract.EABColumns.VOLTE_CALL_AVAILABILITY_TIMESTAMP)));
                    cvalues.put(EABContract.EABColumns.VIDEO_CALL_SERVICE_CONTACT_ADDRESS,
                            cursor.getString(cursor.getColumnIndex(
                                  EABContract.EABColumns.VIDEO_CALL_SERVICE_CONTACT_ADDRESS)));
                    cvalues.put(EABContract.EABColumns.VIDEO_CALL_CAPABILITY,
                            cursor.getString(cursor.getColumnIndex(
                                    EABContract.EABColumns.VIDEO_CALL_CAPABILITY)));
                    cvalues.put(EABContract.EABColumns.VIDEO_CALL_CAPABILITY_TIMESTAMP,
                            cursor.getLong(cursor.getColumnIndex(
                                    EABContract.EABColumns.VIDEO_CALL_CAPABILITY_TIMESTAMP)));
                    cvalues.put(EABContract.EABColumns.VIDEO_CALL_AVAILABILITY,
                            cursor.getString(cursor.getColumnIndex(
                                    EABContract.EABColumns.VIDEO_CALL_AVAILABILITY)));
                    cvalues.put(EABContract.EABColumns.VIDEO_CALL_AVAILABILITY_TIMESTAMP,
                            cursor.getLong(cursor.getColumnIndex(
                                   EABContract.EABColumns.VIDEO_CALL_AVAILABILITY_TIMESTAMP)));
                    cvalues.put(EABContract.EABColumns.CONTACT_LAST_UPDATED_TIMESTAMP, 0);
                    break;
                }
            }
        }
        if (null != cursor) {
            cursor.close();
        }
        return cvalues;
    }

    private void printDeletingValues(Uri uri, String selection, String[] selectionArgs) {
        String[] projection = new String[] {
                EABContract.EABColumns.CONTACT_NUMBER,
                EABContract.EABColumns.CONTACT_NAME,
                EABContract.EABColumns.RAW_CONTACT_ID,
                EABContract.EABColumns.CONTACT_ID,
                EABContract.EABColumns.DATA_ID};
        Cursor cursor = getContext().getContentResolver().query(EABContract.EABColumns.CONTENT_URI,
                        projection, selection, selectionArgs, null);
        if ((null != cursor) && (cursor.getCount() > 0)) {
            logger.debug("Before deleting the cursor count is " + cursor.getCount());
            // Update data only from first cursor element.
            while (cursor.moveToNext()) {
                long dataId = cursor.getLong(cursor.getColumnIndex(
                        EABContract.EABColumns.DATA_ID));
                long contactId = cursor.getLong(cursor.getColumnIndex(
                        EABContract.EABColumns.CONTACT_ID));
                long rawContactId = cursor.getLong(cursor.getColumnIndex(
                        EABContract.EABColumns.RAW_CONTACT_ID));
                String phoneNumber = cursor.getString(cursor.getColumnIndex(
                        EABContract.EABColumns.CONTACT_NUMBER));
                String displayName = cursor.getString(cursor.getColumnIndex(
                        EABContract.EABColumns.CONTACT_NAME));
                logger.debug("Deleting : dataId : " + dataId + " contactId :"  + contactId +
                        " rawContactId :" + rawContactId + " phoneNumber :" + phoneNumber +
                        " displayName :" + displayName);
            }
        } else {
            logger.error("cursor is null!");
        }
        if (null != cursor) {
            cursor.close();
        }
    }
}
