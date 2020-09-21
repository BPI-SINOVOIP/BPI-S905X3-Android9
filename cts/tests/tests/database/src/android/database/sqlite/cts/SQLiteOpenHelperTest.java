/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.database.sqlite.cts;

import android.app.ActivityManager;
import android.content.Context;
import android.database.Cursor;
import android.database.DatabaseUtils;
import android.database.sqlite.SQLiteCursor;
import android.database.sqlite.SQLiteCursorDriver;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteDebug;
import android.database.sqlite.SQLiteGlobal;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQuery;
import android.database.sqlite.SQLiteDatabase.CursorFactory;
import android.support.test.InstrumentationRegistry;
import android.support.test.uiautomator.UiDevice;
import android.test.AndroidTestCase;
import android.util.Log;

import java.io.File;
import java.util.Arrays;

import static android.database.sqlite.cts.DatabaseTestUtils.getDbInfoOutput;
import static android.database.sqlite.cts.DatabaseTestUtils.waitForConnectionToClose;

/**
 * Test {@link SQLiteOpenHelper}.
 */
public class SQLiteOpenHelperTest extends AndroidTestCase {
    private static final String TAG = "SQLiteOpenHelperTest";
    private static final String TEST_DATABASE_NAME = "database_test.db";
    private static final int TEST_VERSION = 1;
    private static final int TEST_ILLEGAL_VERSION = 0;
    private MockOpenHelper mOpenHelper;
    private SQLiteDatabase.CursorFactory mFactory = MockCursor::new;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        SQLiteDatabase.deleteDatabase(mContext.getDatabasePath(TEST_DATABASE_NAME));
        mOpenHelper = getOpenHelper();
    }

    @Override
    protected void tearDown() throws Exception {
        mOpenHelper.close();
        SQLiteDatabase.deleteDatabase(mContext.getDatabasePath(TEST_DATABASE_NAME));
        super.tearDown();
    }

    public void testConstructor() {
        new MockOpenHelper(mContext, TEST_DATABASE_NAME, mFactory, TEST_VERSION);

        // Test with illegal version number.
        try {
            new MockOpenHelper(mContext, TEST_DATABASE_NAME, mFactory, TEST_ILLEGAL_VERSION);
            fail("Constructor of SQLiteOpenHelp should throws a IllegalArgumentException here.");
        } catch (IllegalArgumentException e) {
        }

        // Test with null factory
        new MockOpenHelper(mContext, TEST_DATABASE_NAME, null, TEST_VERSION);
    }

    public void testGetDatabase() {
        SQLiteDatabase database = null;
        assertFalse(mOpenHelper.hasCalledOnOpen());
        // Test getReadableDatabase.
        database = mOpenHelper.getReadableDatabase();
        assertNotNull(database);
        assertTrue(database.isOpen());
        assertTrue(mOpenHelper.hasCalledOnOpen());

        // Database has been opened, so onOpen can not be invoked.
        mOpenHelper.resetStatus();
        assertFalse(mOpenHelper.hasCalledOnOpen());
        // Test getWritableDatabase.
        SQLiteDatabase database2 = mOpenHelper.getWritableDatabase();
        assertSame(database, database2);
        assertTrue(database.isOpen());
        assertFalse(mOpenHelper.hasCalledOnOpen());

        mOpenHelper.close();
        assertFalse(database.isOpen());

        // After close(), onOpen() will be invoked by getWritableDatabase.
        mOpenHelper.resetStatus();
        assertFalse(mOpenHelper.hasCalledOnOpen());
        SQLiteDatabase database3 = mOpenHelper.getWritableDatabase();
        assertNotNull(database);
        assertNotSame(database, database3);
        assertTrue(mOpenHelper.hasCalledOnOpen());
        assertTrue(database3.isOpen());
        mOpenHelper.close();
        assertFalse(database3.isOpen());
    }

    public void testLookasideDefault() throws Exception {
        assertNotNull(mOpenHelper.getWritableDatabase());
        // Lookaside is always disabled on low-RAM devices
        boolean expectDisabled = mContext.getSystemService(ActivityManager.class).isLowRamDevice();
        verifyLookasideStats(mOpenHelper.getDatabaseName(), expectDisabled);
    }

    public void testLookasideDisabled() throws Exception {
        mOpenHelper.setLookasideConfig(0, 0);
        assertNotNull(mOpenHelper.getWritableDatabase());
        verifyLookasideStats(mOpenHelper.getDatabaseName(), true);
    }

    public void testLookasideCustom() throws Exception {
        mOpenHelper.setLookasideConfig(10000, 10);
        assertNotNull(mOpenHelper.getWritableDatabase());
        // Lookaside is always disabled on low-RAM devices
        boolean expectDisabled = mContext.getSystemService(ActivityManager.class).isLowRamDevice();
        verifyLookasideStats(mOpenHelper.getDatabaseName(), expectDisabled);
    }

    public void testSetLookasideConfigValidation() {
        try {
            mOpenHelper.setLookasideConfig(-1, 0);
            fail("Negative slot size should be rejected");
        } catch (IllegalArgumentException expected) {
        }
        try {
            mOpenHelper.setLookasideConfig(0, -10);
            fail("Negative slot count should be rejected");
        } catch (IllegalArgumentException expected) {
        }
        try {
            mOpenHelper.setLookasideConfig(1, 0);
            fail("Illegal config should be rejected");
        } catch (IllegalArgumentException expected) {
        }
        try {
            mOpenHelper.setLookasideConfig(0, 1);
            fail("Illegal config should be rejected");
        } catch (IllegalArgumentException expected) {
        }
    }

    private static void verifyLookasideStats(String dbName, boolean expectDisabled) {
        boolean dbStatFound = false;
        SQLiteDebug.PagerStats info = SQLiteDebug.getDatabaseInfo();
        for (SQLiteDebug.DbStats dbStat : info.dbStats) {
            if (dbStat.dbName.endsWith(dbName)) {
                dbStatFound = true;
                Log.i(TAG, "Lookaside for " + dbStat.dbName + " " + dbStat.lookaside);
                if (expectDisabled) {
                    assertTrue("lookaside slots count should be zero", dbStat.lookaside == 0);
                } else {
                    assertTrue("lookaside slots count should be greater than zero",
                            dbStat.lookaside > 0);
                }
            }
        }
        assertTrue("No dbstat found for " + dbName, dbStatFound);
    }

    public void testCloseIdleConnection() throws Exception {
        mOpenHelper.setIdleConnectionTimeout(1000);
        mOpenHelper.getReadableDatabase();
        // Wait a bit and check that connection is still open
        Thread.sleep(600);
        String output = getDbInfoOutput();
        assertTrue("Connection #0 should be open. Output: " + output,
                output.contains("Connection #0:"));

        // Now cause idle timeout and check that connection is closed
        // We wait up to 5 seconds, which is longer than required 1 s to accommodate for delays in
        // message processing when system is busy
        boolean connectionWasClosed = waitForConnectionToClose(10, 500);
        assertTrue("Connection #0 should be closed", connectionWasClosed);
    }

    public void testSetIdleConnectionTimeoutValidation() throws Exception {
        try {
            mOpenHelper.setIdleConnectionTimeout(-1);
            fail("Negative timeout should be rejected");
        } catch (IllegalArgumentException expected) {
        }
    }

    public void testCloseIdleConnectionDefaultDisabled() throws Exception {
        // Make sure system default timeout is not changed
        assertEquals(30000, SQLiteGlobal.getIdleConnectionTimeout());

        mOpenHelper.getReadableDatabase();
        // Wait past the timeout and verify that connection is still open
        Log.w(TAG, "Waiting for 35 seconds...");
        Thread.sleep(35000);
        String output = getDbInfoOutput();
        assertTrue("Connection #0 should be open. Output: " + output,
                output.contains("Connection #0:"));
    }

    public void testOpenParamsConstructor() {
        SQLiteDatabase.OpenParams params = new SQLiteDatabase.OpenParams.Builder()
                .build();

        MockOpenHelper helper = new MockOpenHelper(mContext, null, 1, params);
        SQLiteDatabase database = helper.getWritableDatabase();
        assertNotNull(database);
        helper.close();
    }

    /**
     * Test for {@link SQLiteOpenHelper#setOpenParams(SQLiteDatabase.OpenParams)}.
     * <p>Opens the database using the helper and verifies that params have been applied</p>
     */
    public void testSetOpenParams() {
        mOpenHelper.close();

        SQLiteDatabase.OpenParams.Builder paramsBuilder = new SQLiteDatabase.OpenParams.Builder();
        paramsBuilder.addOpenFlags(SQLiteDatabase.ENABLE_WRITE_AHEAD_LOGGING);

        MockOpenHelper helper = new MockOpenHelper(mContext, TEST_DATABASE_NAME, null, 1);
        helper.setOpenParams(paramsBuilder.build());
        assertTrue("database must be opened with ENABLE_WRITE_AHEAD_LOGGING flag",
                helper.getWritableDatabase().isWriteAheadLoggingEnabled());
    }

    /**
     * Verifies that {@link SQLiteOpenHelper#setOpenParams(SQLiteDatabase.OpenParams)} cannot be
     * called after opening the database.
     */
    public void testSetOpenParamsFailsIfDbIsOpen() {
        mOpenHelper.getWritableDatabase();
        try {
            mOpenHelper.setOpenParams(new SQLiteDatabase.OpenParams.Builder().build());
            fail("setOpenParams should fail if the database is open");
        } catch (IllegalStateException e) {
            // Expected
        }
    }

    /**
     * Tests a scenario in WAL mode with multiple connections, when a connection should see schema
     * changes made from another connection.
     */
    public void testWalSchemaChangeVisibilityOnUpgrade() {
        File dbPath = mContext.getDatabasePath(TEST_DATABASE_NAME);
        SQLiteDatabase.deleteDatabase(dbPath);
        SQLiteDatabase db = SQLiteDatabase.openOrCreateDatabase(dbPath, null);
        db.execSQL("CREATE TABLE test_table (_id INTEGER PRIMARY KEY AUTOINCREMENT)");
        db.setVersion(1);
        db.close();
        mOpenHelper = new MockOpenHelper(mContext, TEST_DATABASE_NAME, null, 2) {
            {
                setWriteAheadLoggingEnabled(true);
            }

            @Override
            public void onCreate(SQLiteDatabase db) {
            }

            @Override
            public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
                if (oldVersion == 1) {
                    db.execSQL("ALTER TABLE test_table ADD column2 INT DEFAULT 1234");
                    db.execSQL("CREATE TABLE test_table2 (_id INTEGER PRIMARY KEY AUTOINCREMENT)");
                }
            }
        };
        // Check if can see the new column
        try (Cursor cursor = mOpenHelper.getReadableDatabase()
                .rawQuery("select * from test_table", null)) {
            assertEquals("Newly added column should be visible. Returned columns: " + Arrays
                    .toString(cursor.getColumnNames()), 2, cursor.getColumnCount());
        }
        // Check if can see the new table
        try (Cursor cursor = mOpenHelper.getReadableDatabase()
                .rawQuery("select * from test_table2", null)) {
            assertEquals(1, cursor.getColumnCount());
        }
    }

    public void testSetWriteAheadLoggingDisablesCompatibilityWal() {
        // Verify that compatibility WAL is not enabled, if an application explicitly disables WAL

        mOpenHelper.setWriteAheadLoggingEnabled(false);
        String journalMode = DatabaseUtils
                .stringForQuery(mOpenHelper.getWritableDatabase(), "PRAGMA journal_mode", null);
        assertFalse("Default journal mode should not be WAL", "WAL".equalsIgnoreCase(journalMode));
    }

    private MockOpenHelper getOpenHelper() {
        return new MockOpenHelper(mContext, TEST_DATABASE_NAME, mFactory, TEST_VERSION);
    }

    private static class MockOpenHelper extends SQLiteOpenHelper {
        private boolean mHasCalledOnOpen = false;

        MockOpenHelper(Context context, String name, CursorFactory factory, int version) {
            super(context, name, factory, version);
        }

        MockOpenHelper(Context context, String name, int version,
                SQLiteDatabase.OpenParams openParams) {
            super(context, name, version, openParams);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
        }

        @Override
        public void onOpen(SQLiteDatabase db) {
            mHasCalledOnOpen = true;
        }

        public boolean hasCalledOnOpen() {
            return mHasCalledOnOpen;
        }

        public void resetStatus() {
            mHasCalledOnOpen = false;
        }
    }

    private static class MockCursor extends SQLiteCursor {
        MockCursor(SQLiteDatabase db, SQLiteCursorDriver driver, String editTable,
                SQLiteQuery query) {
            super(db, driver, editTable, query);
        }
    }
}
