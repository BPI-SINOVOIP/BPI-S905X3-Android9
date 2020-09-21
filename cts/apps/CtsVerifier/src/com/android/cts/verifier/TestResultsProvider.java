/*
 * Copyright (C) 2010 The Android Open Source Project
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

package com.android.cts.verifier;

import com.android.compatibility.common.util.ReportLog;

import android.app.backup.BackupManager;
import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.UriMatcher;
import android.database.Cursor;
import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteOpenHelper;
import android.database.sqlite.SQLiteQueryBuilder;
import android.net.Uri;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.ObjectOutputStream;

/** {@link ContentProvider} that provides read and write access to the test results. */
public class TestResultsProvider extends ContentProvider {

    private static final String RESULTS_PATH = "results";

    /**
     * Get the URI from the result content.
     * @param context
     * @return Uri
     */
    public static Uri getResultContentUri(Context context) {
        final String packageName = context.getPackageName();
        final Uri contentUri = Uri.parse("content://" + packageName + ".testresultsprovider");
        return Uri.withAppendedPath(contentUri, RESULTS_PATH);
    }

    /**
     * Get the URI from the test name.
     * @param context
     * @param testName
     * @return Uri
     */
    public static Uri getTestNameUri(Context context) {
        final String testName = context.getClass().getName();
        return Uri.withAppendedPath(getResultContentUri(context), testName);
    }

    static final String _ID = "_id";

    /** String name of the test like "com.android.cts.verifier.foo.FooTestActivity" */
    static final String COLUMN_TEST_NAME = "testname";

    /** Integer test result corresponding to constants in {@link TestResult}. */
    static final String COLUMN_TEST_RESULT = "testresult";

    /** Boolean indicating whether the test info has been seen. */
    static final String COLUMN_TEST_INFO_SEEN = "testinfoseen";

    /** String containing the test's details. */
    static final String COLUMN_TEST_DETAILS = "testdetails";

    /** ReportLog containing the test result metrics. */
    static final String COLUMN_TEST_METRICS = "testmetrics";

    private static final UriMatcher URI_MATCHER = new UriMatcher(UriMatcher.NO_MATCH);
    private static final int RESULTS_ALL = 1;
    private static final int RESULTS_ID = 2;
    private static final int RESULTS_TEST_NAME = 3;

    private static final String TABLE_NAME = "results";

    private SQLiteOpenHelper mOpenHelper;

    private BackupManager mBackupManager;

    @Override
    public boolean onCreate() {
        final String authority = getContext().getPackageName() + ".testresultsprovider";

        URI_MATCHER.addURI(authority, RESULTS_PATH, RESULTS_ALL);
        URI_MATCHER.addURI(authority, RESULTS_PATH + "/#", RESULTS_ID);
        URI_MATCHER.addURI(authority, RESULTS_PATH + "/*", RESULTS_TEST_NAME);

        mOpenHelper = new TestResultsOpenHelper(getContext());
        mBackupManager = new BackupManager(getContext());
        return false;
    }

    private static class TestResultsOpenHelper extends SQLiteOpenHelper {

        private static final String DATABASE_NAME = "results.db";

        private static final int DATABASE_VERSION = 6;

        TestResultsOpenHelper(Context context) {
            super(context, DATABASE_NAME, null, DATABASE_VERSION);
        }

        @Override
        public void onCreate(SQLiteDatabase db) {
            db.execSQL("CREATE TABLE " + TABLE_NAME + " ("
                    + _ID + " INTEGER PRIMARY KEY AUTOINCREMENT, "
                    + COLUMN_TEST_NAME + " TEXT, "
                    + COLUMN_TEST_RESULT + " INTEGER,"
                    + COLUMN_TEST_INFO_SEEN + " INTEGER DEFAULT 0,"
                    + COLUMN_TEST_DETAILS + " TEXT,"
                    + COLUMN_TEST_METRICS + " BLOB);");
        }

        @Override
        public void onUpgrade(SQLiteDatabase db, int oldVersion, int newVersion) {
            db.execSQL("DROP TABLE IF EXISTS " + TABLE_NAME);
            onCreate(db);
        }
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection, String[] selectionArgs,
            String sortOrder) {
        SQLiteQueryBuilder query = new SQLiteQueryBuilder();
        query.setTables(TABLE_NAME);

        int match = URI_MATCHER.match(uri);
        switch (match) {
            case RESULTS_ALL:
                break;

            case RESULTS_ID:
                query.appendWhere(_ID);
                query.appendWhere("=");
                query.appendWhere(uri.getPathSegments().get(1));
                break;

            case RESULTS_TEST_NAME:
                query.appendWhere(COLUMN_TEST_NAME);
                query.appendWhere("=");
                query.appendWhere("\"" + uri.getPathSegments().get(1) + "\"");
                break;

            default:
                throw new IllegalArgumentException("Unknown URI: " + uri);
        }

        SQLiteDatabase db = mOpenHelper.getReadableDatabase();
        return query.query(db, projection, selection, selectionArgs, null, null, sortOrder);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        long id = db.insert(TABLE_NAME, null, values);
        getContext().getContentResolver().notifyChange(uri, null);
        mBackupManager.dataChanged();
        return Uri.withAppendedPath(getResultContentUri(getContext()), "" + id);
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection, String[] selectionArgs) {
        int match = URI_MATCHER.match(uri);
        switch (match) {
            case RESULTS_ALL:
                break;

            case RESULTS_ID:
                String idSelection = _ID + "=" + uri.getPathSegments().get(1);
                if (selection != null && selection.length() > 0) {
                    selection = idSelection + " AND " + selection;
                } else {
                    selection = idSelection;
                }
                break;

            case RESULTS_TEST_NAME:
                String testNameSelection = COLUMN_TEST_NAME + "=\""
                        + uri.getPathSegments().get(1) + "\"";
                if (selection != null && selection.length() > 0) {
                    selection = testNameSelection + " AND " + selection;
                } else {
                    selection = testNameSelection;
                }
                break;

            default:
                throw new IllegalArgumentException("Unknown URI: " + uri);
        }

        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int numUpdated = db.update(TABLE_NAME, values, selection, selectionArgs);
        if (numUpdated > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
            mBackupManager.dataChanged();
        }
        return numUpdated;
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        SQLiteDatabase db = mOpenHelper.getWritableDatabase();
        int numDeleted = db.delete(TABLE_NAME, selection, selectionArgs);
        if (numDeleted > 0) {
            getContext().getContentResolver().notifyChange(uri, null);
            mBackupManager.dataChanged();
        }
        return numDeleted;
    }

    @Override
    public String getType(Uri uri) {
        return null;
    }

    static void setTestResult(Context context, String testName, int testResult,
            String testDetails, ReportLog reportLog) {
        ContentValues values = new ContentValues(2);
        values.put(TestResultsProvider.COLUMN_TEST_RESULT, testResult);
        values.put(TestResultsProvider.COLUMN_TEST_NAME, testName);
        values.put(TestResultsProvider.COLUMN_TEST_DETAILS, testDetails);
        values.put(TestResultsProvider.COLUMN_TEST_METRICS, serialize(reportLog));

        final Uri uri = getResultContentUri(context);
        ContentResolver resolver = context.getContentResolver();
        int numUpdated = resolver.update(uri, values,
                TestResultsProvider.COLUMN_TEST_NAME + " = ?",
                new String[] {testName});

        if (numUpdated == 0) {
            resolver.insert(uri, values);
        }
    }

    private static byte[] serialize(Object o) {
        ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
        ObjectOutputStream objectOutput = null;
        try {
            objectOutput = new ObjectOutputStream(byteStream);
            objectOutput.writeObject(o);
            return byteStream.toByteArray();
        } catch (IOException e) {
            return null;
        } finally {
            try {
                if (objectOutput != null) {
                    objectOutput.close();
                }
                byteStream.close();
            } catch (IOException e) {
                // Ignore close exception.
            }
        }
    }
}
