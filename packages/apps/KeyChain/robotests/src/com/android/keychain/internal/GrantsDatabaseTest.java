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

package com.android.keychain.internal;

import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import android.content.ContentValues;
import android.content.Context;
import android.content.pm.PackageManager;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import com.android.keychain.TestConfig;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.RuntimeEnvironment;
import org.robolectric.annotation.Config;

/** Unit tests for {@link com.android.keychain.internal.GrantsDatabase}. */
@RunWith(RobolectricTestRunner.class)
@Config(manifest = TestConfig.MANIFEST_PATH, sdk = TestConfig.SDK_VERSION)
public final class GrantsDatabaseTest {
    private static final String DUMMY_ALIAS = "dummy_alias";
    private static final String DUMMY_ALIAS2 = "another_dummy_alias";
    private static final int DUMMY_UID = 1000;
    private static final int DUMMY_UID2 = 1001;
    // Constants duplicated from GrantsDatabase to make sure the upgrade tests catch if the
    // name of one of the fields in the DB changes.
    static final String DATABASE_NAME = "grants.db";
    static final String TABLE_GRANTS = "grants";
    static final String GRANTS_ALIAS = "alias";
    static final String GRANTS_GRANTEE_UID = "uid";
    static final String TABLE_SELECTABLE = "userselectable";
    static final String SELECTABLE_IS_SELECTABLE = "is_selectable";

    private GrantsDatabase mGrantsDB;

    @Before
    public void setUp() {
        mGrantsDB = new GrantsDatabase(RuntimeEnvironment.application);
    }

    @Test
    public void testSetGrant_notMixingUIDs() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID2, DUMMY_ALIAS));
    }

    @Test
    public void testSetGrant_notMixingAliases() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS2));
    }

    @Test
    public void testSetGrantTrue() {
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
    }

    @Test
    public void testSetGrantFalse() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, false);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
    }

    @Test
    public void testSetGrantTrueThenFalse() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, false);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
    }

    @Test
    public void testRemoveAliasInformation() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        mGrantsDB.setGrant(DUMMY_UID2, DUMMY_ALIAS, true);
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        mGrantsDB.removeAliasInformation(DUMMY_ALIAS);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID2, DUMMY_ALIAS));
        Assert.assertFalse(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
    }

    @Test
    public void testRemoveAllAliasesInformation() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        mGrantsDB.setGrant(DUMMY_UID2, DUMMY_ALIAS, true);
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS2, true);
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, true);
        mGrantsDB.removeAllAliasesInformation();
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID2, DUMMY_ALIAS));
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS2));
        Assert.assertFalse(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
    }

    @Test
    public void testPurgeOldGrantsDoesNotDeleteGrantsForExistingPackages() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        PackageManager pm = mock(PackageManager.class);
        when(pm.getPackagesForUid(DUMMY_UID)).thenReturn(new String[] {"p"});
        mGrantsDB.purgeOldGrants(pm);
        Assert.assertTrue(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
    }

    @Test
    public void testPurgeOldGrantsPurgesAllNonExistingPackages() {
        mGrantsDB.setGrant(DUMMY_UID, DUMMY_ALIAS, true);
        mGrantsDB.setGrant(DUMMY_UID2, DUMMY_ALIAS, true);
        PackageManager pm = mock(PackageManager.class);
        when(pm.getPackagesForUid(DUMMY_UID)).thenReturn(null);
        when(pm.getPackagesForUid(DUMMY_UID2)).thenReturn(null);
        mGrantsDB.purgeOldGrants(pm);
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID, DUMMY_ALIAS));
        Assert.assertFalse(mGrantsDB.hasGrant(DUMMY_UID2, DUMMY_ALIAS));
    }

    @Test
    public void testPurgeOldGrantsWorksOnEmptyDatabase() {
        // Check that NPE is not thrown.
        mGrantsDB.purgeOldGrants(null);
    }

    @Test
    public void testIsUserSelectable() {
        Assert.assertFalse(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
    }

    @Test
    public void testSetUserSelectable() {
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, false);
        Assert.assertFalse(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
        mGrantsDB.setIsUserSelectable(DUMMY_ALIAS, true);
        Assert.assertTrue(mGrantsDB.isUserSelectable(DUMMY_ALIAS));
    }

    private abstract class BaseGrantsDatabaseHelper extends SQLiteOpenHelper {
        private final boolean mCreateUserSelectableTable;

        public BaseGrantsDatabaseHelper(
                Context context, int dbVersion, boolean createUserSelectableTable) {
            super(context, DATABASE_NAME, null /* CursorFactory */, dbVersion);
            mCreateUserSelectableTable = createUserSelectableTable;
        }

        void createUserSelectableTable(final SQLiteDatabase db) {
            db.execSQL(
                    "CREATE TABLE "
                            + TABLE_SELECTABLE
                            + " (  "
                            + GRANTS_ALIAS
                            + " STRING NOT NULL,  "
                            + SELECTABLE_IS_SELECTABLE
                            + " STRING NOT NULL,  "
                            + "UNIQUE ("
                            + GRANTS_ALIAS
                            + "))");
        }

        @Override
        public void onCreate(final SQLiteDatabase db) {
            db.execSQL(
                    "CREATE TABLE "
                            + TABLE_GRANTS
                            + " (  "
                            + GRANTS_ALIAS
                            + " STRING NOT NULL,  "
                            + GRANTS_GRANTEE_UID
                            + " INTEGER NOT NULL,  "
                            + "UNIQUE ("
                            + GRANTS_ALIAS
                            + ","
                            + GRANTS_GRANTEE_UID
                            + "))");

            if (mCreateUserSelectableTable) {
                createUserSelectableTable(db);
            }
        }

        @Override
        public void onUpgrade(final SQLiteDatabase db, int oldVersion, final int newVersion) {
            throw new IllegalStateException("Existing DB must be dropped first.");
        }

        public void insertIntoGrantsTable(final SQLiteDatabase db, String alias, int uid) {
            final ContentValues values = new ContentValues();
            values.put(GRANTS_ALIAS, alias);
            values.put(GRANTS_GRANTEE_UID, uid);
            db.insert(TABLE_GRANTS, GRANTS_ALIAS, values);
        }

        public void insertIntoSelectableTable(
                final SQLiteDatabase db, String alias, boolean isSelectable) {
            final ContentValues values = new ContentValues();
            values.put(GRANTS_ALIAS, alias);
            values.put(SELECTABLE_IS_SELECTABLE, Boolean.toString(isSelectable));
            db.insert(TABLE_SELECTABLE, null, values);
        }
    }

    private class V1DatabaseHelper extends BaseGrantsDatabaseHelper {
        public V1DatabaseHelper(Context context) {
            super(context, 1, false);
        }
    }

    private class V2DatabaseHelper extends BaseGrantsDatabaseHelper {
        public V2DatabaseHelper(Context context) {
            super(context, 2, true);
        }
    }

    private class IncorrectlyVersionedV2DatabaseHelper extends BaseGrantsDatabaseHelper {
        public IncorrectlyVersionedV2DatabaseHelper(Context context) {
            super(context, 1, true);
        }
    }

    @Test
    public void testUpgradeDatabase() {
        // Close old DB
        mGrantsDB.destroy();
        // Create a new, V1 database.
        Context context = RuntimeEnvironment.application;
        context.deleteDatabase(DATABASE_NAME);
        V1DatabaseHelper v1DBHelper = new V1DatabaseHelper(context);
        // Fill it up with a few records
        final SQLiteDatabase db = v1DBHelper.getWritableDatabase();
        String[] aliases = {"alias-1", "alias-2", "alias-3"};
        for (String alias : aliases) {
            v1DBHelper.insertIntoGrantsTable(db, alias, 123456);
        }

        // Test that the aliases were made user-selectable during the upgrade.
        mGrantsDB = new GrantsDatabase(RuntimeEnvironment.application);
        for (String alias : aliases) {
            Assert.assertTrue(mGrantsDB.isUserSelectable(alias));
        }
    }

    @Test
    public void testSelectabilityInV2DatabaseNotChanged() {
        // Close old DB
        mGrantsDB.destroy();
        Context context = RuntimeEnvironment.application;
        context.deleteDatabase(DATABASE_NAME);
        // Create a new, V2 database.
        V2DatabaseHelper v2DBHelper = new V2DatabaseHelper(context);
        // Fill it up with a few records
        final SQLiteDatabase db = v2DBHelper.getWritableDatabase();
        String[] aliases = {"alias-1", "alias-2", "alias-3"};
        for (String alias : aliases) {
            v2DBHelper.insertIntoGrantsTable(db, alias, 123456);
            v2DBHelper.insertIntoSelectableTable(db, alias, false);
        }
        String selectableAlias = "alias-selectable-1";
        v2DBHelper.insertIntoGrantsTable(db, selectableAlias, 123457);
        v2DBHelper.insertIntoSelectableTable(db, selectableAlias, true);

        // Test that the aliases were made user-selectable during the upgrade.
        mGrantsDB = new GrantsDatabase(RuntimeEnvironment.application);
        for (String alias : aliases) {
            Assert.assertFalse(mGrantsDB.isUserSelectable(alias));
        }
        Assert.assertTrue(mGrantsDB.isUserSelectable(selectableAlias));
    }

    @Test
    public void testV1AndAHalfDBUpgradedCorrectly() {
        // Close old DB
        mGrantsDB.destroy();
        Context context = RuntimeEnvironment.application;
        context.deleteDatabase(DATABASE_NAME);
        // Create a new, V2 database that's incorrectly versioned as v1.
        IncorrectlyVersionedV2DatabaseHelper dbHelper =
                new IncorrectlyVersionedV2DatabaseHelper(context);
        // Fill it up with a few records
        final SQLiteDatabase db = dbHelper.getWritableDatabase();
        String[] aliases = {"alias-1", "alias-2", "alias-3"};
        for (String alias : aliases) {
            dbHelper.insertIntoGrantsTable(db, alias, 123456);
            dbHelper.insertIntoSelectableTable(db, alias, false);
        }

        // Insert one alias explicitly selectable
        String selectableAlias = "alias-selectable-1";
        dbHelper.insertIntoGrantsTable(db, selectableAlias, 123456);
        dbHelper.insertIntoSelectableTable(db, selectableAlias, true);

        // Insert one alias without explicitl user-selectability, which should
        // default to true when upgrading from V1 to V2.
        String defaultSelectableAlias = "alias-selectable-2";
        dbHelper.insertIntoGrantsTable(db, defaultSelectableAlias, 123456);

        // Test that the aliases were made user-selectable during the upgrade.
        mGrantsDB = new GrantsDatabase(RuntimeEnvironment.application);
        for (String alias : aliases) {
            Assert.assertFalse(mGrantsDB.isUserSelectable(alias));
        }
        Assert.assertTrue(mGrantsDB.isUserSelectable(selectableAlias));
        Assert.assertTrue(mGrantsDB.isUserSelectable(defaultSelectableAlias));
    }
}
