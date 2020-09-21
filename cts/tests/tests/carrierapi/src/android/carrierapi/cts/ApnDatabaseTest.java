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
package android.carrierapi.cts;

import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.database.sqlite.SQLiteException;
import android.net.Uri;
import android.provider.Telephony.Carriers;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.HashMap;
import java.util.Map;

import static junit.framework.TestCase.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.fail;

/**
 * Build, install and run the tests by running the commands below:
 *  make cts -j64
 *  cts-tradefed run cts -m CtsCarrierApiTestCases --test android.carrierapi.cts.ApnDatabaseTest
 */
@RunWith(AndroidJUnit4.class)
public class ApnDatabaseTest {
    private static final String TAG = "ApnDatabaseTest";

    private ContentResolver mContentResolver;
    private PackageManager mPackageManager;
    private boolean mHasCellular;

    private static final String NAME = "carrierName";
    private static final String APN = "apn";
    private static final String PROXY = "proxy";
    private static final String PORT = "port";
    private static final String MMSC = "mmsc";
    private static final String MMSPROXY = "mmsproxy";
    private static final String MMSPORT = "mmsport";
    private static final String NUMERIC = "numeric";
    private static final String USER = "user";
    private static final String PASSWORD = "password";
    private static final String AUTH_TYPE = "auth_type";
    private static final String TYPE = "type";
    private static final String PROTOCOL = "protocol";
    private static final String ROAMING_PROTOCOL = "roaming_protocol";
    private static final String CARRIER_ENABLED = "true";
    private static final String NETWORK_TYPE_BITMASK = "0";
    private static final String BEARER = "0";

    private static final Map<String, String> APN_MAP = new HashMap<String,String>() {{
        put(Carriers.NAME, NAME);
        put(Carriers.APN, APN);
        put(Carriers.PROXY, PROXY);
        put(Carriers.PORT, PORT);
        put(Carriers.MMSC, MMSC);
        put(Carriers.MMSPROXY, MMSPROXY);
        put(Carriers.MMSPORT, MMSPORT);
        put(Carriers.NUMERIC, NUMERIC);
        put(Carriers.USER, USER);
        put(Carriers.PASSWORD, PASSWORD);
        put(Carriers.AUTH_TYPE, AUTH_TYPE);
        put(Carriers.TYPE, TYPE);
        put(Carriers.PROTOCOL, PROTOCOL);
        put(Carriers.ROAMING_PROTOCOL, ROAMING_PROTOCOL);
        put(Carriers.CARRIER_ENABLED, CARRIER_ENABLED);
        put(Carriers.NETWORK_TYPE_BITMASK, NETWORK_TYPE_BITMASK);
        put(Carriers.BEARER, BEARER);
    }};

    // Faked network type bitmask and its compatible bearer bitmask.
    private static final int NETWORK_TYPE_BITMASK_NUMBER = 1 << (13 - 1);
    private static final int RIL_RADIO_TECHNOLOGY_BITMASK_NUMBER = 1 << (14 - 1);

    @Before
    public void setUp() throws Exception {
        mContentResolver = InstrumentationRegistry.getContext().getContentResolver();
        mPackageManager = InstrumentationRegistry.getContext().getPackageManager();
        // Checks whether the cellular stack should be running on this device.
        mHasCellular = mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY);
        if (!mHasCellular) {
            Log.e(TAG, "No cellular support, all tests will be skipped.");
        }
    }

    private void failMessage() {
        fail("This test requires a SIM card with carrier privilege rule on it.\n" +
                "Visit https://source.android.com/devices/tech/config/uicc.html");
    }

    /**
     * Test inserting, querying, updating and deleting values in carriers table.
     * Verify that the inserted values match the result of the query and are deleted.
     */
    @Test
    public void testValidCase() {
        if (!mHasCellular) return;
        Uri uri = Carriers.CONTENT_URI;
        // CONTENT_URI = Uri.parse("content://telephony/carriers");
        // Create A set of column_name/value pairs to add to the database.
        ContentValues contentValues = makeDefaultContentValues();

        try {
            // Insert the value into database.
            Log.d(TAG, "testInsertCarriers Inserting contentValues: " + contentValues.toString());
            Uri newUri = mContentResolver.insert(uri, contentValues);
            assertNotNull("Failed to insert to table", newUri);

            // Get the values in table.
            final String selection = Carriers.NUMERIC + "=?";
            String[] selectionArgs = { NUMERIC };
            String[] apnProjection = APN_MAP.keySet().toArray(new String[APN_MAP.size()]);
            Log.d(TAG, "testInsertCarriers query projection: " + Arrays.toString(apnProjection)
                    + "\ntestInsertCarriers selection: " + selection
                    + "\ntestInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            Cursor cursor = mContentResolver.query(
                    uri, apnProjection, selection, selectionArgs, null);

            // Verify that the inserted value match the results of the query
            assertNotNull("Failed to query the table", cursor);
            assertEquals("Unexpected number of APNs returned by cursor",
                    1, cursor.getCount());
            cursor.moveToFirst();
            for (Map.Entry<String, String> entry: APN_MAP.entrySet()) {
                assertEquals(
                        "Unexpected value returned by cursor",
                        cursor.getString(cursor.getColumnIndex(entry.getKey())), entry.getValue());
            }

            // update the apn
            final String newApn = "newapn";
            Log.d(TAG, "Update the APN field to: " + newApn);
            contentValues.put(Carriers.APN, newApn);
            final int updateCount = mContentResolver.update(uri, contentValues, selection,
                    selectionArgs);
            assertEquals("Unexpected number of rows updated", 1, updateCount);

            // Verify the updated value
            cursor = mContentResolver.query(uri, apnProjection, selection, selectionArgs, null);
            assertNotNull("Failed to query the table", cursor);
            assertEquals("Unexpected number of APNs returned by cursor", 1, cursor.getCount());
            cursor.moveToFirst();
            assertEquals("Unexpected value returned by cursor",
                    cursor.getString(cursor.getColumnIndex(Carriers.APN)), newApn);

            // delete test content
            final String selectionToDelete = Carriers.NUMERIC + "=?";
            String[] selectionArgsToDelete = { NUMERIC };
            Log.d(TAG, "testInsertCarriers deleting selection: " + selectionToDelete
                    + "testInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            int numRowsDeleted = mContentResolver.delete(
                    uri, selectionToDelete, selectionArgsToDelete);
            assertEquals("Unexpected number of rows deleted",1, numRowsDeleted);

            // verify that deleted values are gone
            cursor = mContentResolver.query(uri, apnProjection, selection, selectionArgs, null);
            assertEquals("Unexpected number of rows deleted", 0, cursor.getCount());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    @Test
    public void testQueryConflictCase() {
        if (!mHasCellular) return;
        String invalidColumn = "random";
        Uri uri = Carriers.CONTENT_URI;
        // CONTENT_URI = Uri.parse("content://telephony/carriers");
        // Create A set of column_name/value pairs to add to the database.
        ContentValues contentValues = new ContentValues();
        contentValues.put(Carriers.NAME, NAME);
        contentValues.put(Carriers.APN, APN);
        contentValues.put(Carriers.PORT, PORT);
        contentValues.put(Carriers.PROTOCOL, PROTOCOL);
        contentValues.put(Carriers.NUMERIC, NUMERIC);

        try {
            // Insert the value into database.
            Log.d(TAG, "testInsertCarriers Inserting contentValues: " + contentValues.toString());
            Uri newUri = mContentResolver.insert(uri, contentValues);
            assertNotNull("Failed to insert to table", newUri);

            // Try to get the value with invalid selection
            final String[] testProjection =
                    {
                            Carriers.NAME,
                            Carriers.APN,
                            Carriers.PORT,
                            Carriers.PROTOCOL,
                            Carriers.NUMERIC,
                    };
            final String selection = invalidColumn + "=?";
            String[] selectionArgs = { invalidColumn };
            Log.d(TAG, "testInsertCarriers query projection: " + Arrays.toString(testProjection)
                    + "\ntestInsertCarriers selection: " + selection
                    + "\ntestInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            Cursor cursor = mContentResolver.query(
                    uri, testProjection, selection, selectionArgs, null);
            assertNull("Failed to query the table",cursor);

            // delete test content
            final String selectionToDelete = Carriers.NAME + "=?";
            String[] selectionArgsToDelete = { NAME };
            Log.d(TAG, "testInsertCarriers deleting selection: " + selectionToDelete
                    + "testInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            int numRowsDeleted = mContentResolver.delete(
                    uri, selectionToDelete, selectionArgsToDelete);
            assertEquals("Unexpected number of rows deleted", 1, numRowsDeleted);

            // verify that deleted values are gone
            cursor = mContentResolver.query(
                    uri, testProjection, selectionToDelete, selectionArgsToDelete, null);
            assertEquals("Unexpected number of rows deleted", 0, cursor.getCount());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    @Test
    public void testUpdateConflictCase() {
        if (!mHasCellular) return;
        Uri uri = Carriers.CONTENT_URI;
        // CONTENT_URI = Uri.parse("content://telephony/carriers");
        // Create A set of column_name/value pairs to add to the database.
        ContentValues contentValues = new ContentValues();
        contentValues.put(Carriers.NAME, NAME);
        contentValues.put(Carriers.APN, APN);
        contentValues.put(Carriers.PORT, PORT);
        contentValues.put(Carriers.PROTOCOL, PROTOCOL);
        contentValues.put(Carriers.NUMERIC, NUMERIC);

        try {
            // Insert the value into database.
            Log.d(TAG, "testInsertCarriers Inserting contentValues: " + contentValues.toString());
            Uri newUri = mContentResolver.insert(uri, contentValues);
            assertNotNull("Failed to insert to table", newUri);

            // Try to get the value with invalid selection
            final String[] testProjection =
                    {
                            Carriers.NAME,
                            Carriers.APN,
                            Carriers.PORT,
                            Carriers.PROTOCOL,
                            Carriers.NUMERIC,
                    };
            String selection = Carriers.NAME + "=?";
            String[] selectionArgs = { NAME };
            Log.d(TAG, "testInsertCarriers query projection: " + Arrays.toString(testProjection)
                    + "\ntestInsertCarriers selection: " + selection
                    + "\ntestInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            Cursor cursor = mContentResolver.query(
                    uri, testProjection, selection, selectionArgs, null);
            assertEquals("Unexpected number of APNs returned by cursor",
                    1, cursor.getCount());

            // Update the table with invalid column
            String invalidColumn = "random";
            contentValues.put(invalidColumn, invalidColumn);
            try {
                mContentResolver.update(uri, contentValues, selection, selectionArgs);
                fail();
            } catch (SQLiteException e) {
                // Expected: If there's no such a column, an exception will be thrown and the
                // Activity Manager will kill this process shortly.
            }

            // delete test content
            final String selectionToDelete = Carriers.NAME + "=?";
            String[] selectionArgsToDelete = { NAME };
            Log.d(TAG, "testInsertCarriers deleting selection: " + selectionToDelete
                    + "testInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            int numRowsDeleted = mContentResolver.delete(
                    uri, selectionToDelete, selectionArgsToDelete);
            assertEquals("Unexpected number of rows deleted", 1, numRowsDeleted);

            // verify that deleted values are gone
            cursor = mContentResolver.query(
                    uri, testProjection, selectionToDelete, selectionArgsToDelete, null);
            assertEquals("Unexpected number of rows deleted", 0, cursor.getCount());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    @Test
    public void testDeleteConflictCase() {
        if (!mHasCellular) return;
        String invalidColumn = "random";
        Uri uri = Carriers.CONTENT_URI;
        // CONTENT_URI = Uri.parse("content://telephony/carriers");
        // Create A set of column_name/value pairs to add to the database.
        ContentValues contentValues = new ContentValues();
        contentValues.put(Carriers.NAME, NAME);
        contentValues.put(Carriers.APN, APN);
        contentValues.put(Carriers.PORT, PORT);
        contentValues.put(Carriers.PROTOCOL, PROTOCOL);
        contentValues.put(Carriers.NUMERIC, NUMERIC);

        try {
            // Insert the value into database.
            Log.d(TAG, "testInsertCarriers Inserting contentValues: " + contentValues.toString());
            Uri newUri = mContentResolver.insert(uri, contentValues);
            assertNotNull("Failed to insert to table", newUri);

            // Get the values in table.
            final String[] testProjection =
                    {
                            Carriers.NAME,
                            Carriers.APN,
                            Carriers.PORT,
                            Carriers.PROTOCOL,
                            Carriers.NUMERIC,
                    };
            String selection = Carriers.NAME + "=?";
            String[] selectionArgs = { NAME };
            Log.d(TAG, "testInsertCarriers query projection: " + Arrays.toString(testProjection)
                    + "\ntestInsertCarriers selection: " + selection
                    + "\ntestInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            Cursor cursor = mContentResolver.query(
                    uri, testProjection, selection, selectionArgs, null);
            assertEquals("Unexpected number of APNs returned by cursor", 1, cursor.getCount());

            // try to delete with invalid selection
            String selectionToDelete = invalidColumn + "=?";
            String[] selectionArgsToDelete = { invalidColumn };
            Log.d(TAG, "testInsertCarriers deleting selection: " + selectionToDelete
                    + "testInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));

            try {
                mContentResolver.delete(uri, selectionToDelete, selectionArgsToDelete);
                fail();
            } catch (SQLiteException e) {
                // Expected: If there's no such a column, an exception will be thrown and the
                // Activity Manager will kill this process shortly.
            }

            // verify that deleted value is still there
            selection = Carriers.NAME + "=?";
            selectionArgs[0] = NAME;
            cursor = mContentResolver.query(uri, testProjection, selection, selectionArgs, null);
            assertEquals("Unexpected number of APNs returned by cursor", 1, cursor.getCount());

            // delete test content
            selectionToDelete = Carriers.NAME + "=?";
            selectionArgsToDelete[0] = NAME;
            Log.d(TAG, "testInsertCarriers deleting selection: " + selectionToDelete
                    + "testInsertCarriers selectionArgs: " + Arrays.toString(selectionArgs));
            int numRowsDeleted = mContentResolver.delete(
                    uri, selectionToDelete, selectionArgsToDelete);
            assertEquals("Unexpected number of rows deleted", 1, numRowsDeleted);

            // verify that deleted values are gone
            cursor = mContentResolver.query(uri, testProjection, selection, selectionArgs, null);
            assertEquals("Unexpected number of rows deleted", 0, cursor.getCount());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    private ContentValues makeDefaultContentValues() {
        ContentValues contentValues = new ContentValues();
        for (Map.Entry<String, String> entry: APN_MAP.entrySet()) {
            contentValues.put(entry.getKey(), entry.getValue());
        }
        return contentValues;
    }
}