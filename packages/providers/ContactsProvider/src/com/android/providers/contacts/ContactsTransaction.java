/*
 * Copyright (C) 2011 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.providers.contacts;

import android.database.sqlite.SQLiteDatabase;
import android.database.sqlite.SQLiteTransactionListener;
import android.util.Log;

import com.google.android.collect.Lists;
import com.google.android.collect.Maps;

import java.util.List;
import java.util.Map;

/**
 * A transaction for interacting with a Contacts provider.  This is used to pass state around
 * throughout the operations comprising the transaction, including which databases the overall
 * transaction is involved in, and whether the operation being performed is a batch operation.
 */
public class ContactsTransaction {

    /**
     * Whether this transaction is encompassing a batch of operations.  If we're in batch mode,
     * transactional operations from non-batch callers are ignored.
     */
    private final boolean mBatch;

    /**
     * The list of databases that have been enlisted in this transaction.
     *
     * Note we insert elements to the head of the list, so that we endTransaction() in the reverse
     * order.
     */
    private final List<SQLiteDatabase> mDatabasesForTransaction;

    /**
     * The mapping of tags to databases involved in this transaction.
     */
    private final Map<String, SQLiteDatabase> mDatabaseTagMap;

    /**
     * Whether any actual changes have been made successfully in this transaction.
     */
    private boolean mIsDirty;

    /**
     * Whether a yield operation failed with an exception.  If this occurred, we may not have a
     * lock on one of the databases that we started the transaction with (the yield code cleans
     * that up itself), so we should do an extra check before ending transactions.
     */
    private boolean mYieldFailed;

    /**
     * Creates a new transaction object, optionally marked as a batch transaction.
     * @param batch Whether the transaction is in batch mode.
     */
    public ContactsTransaction(boolean batch) {
        mBatch = batch;
        mDatabasesForTransaction = Lists.newArrayList();
        mDatabaseTagMap = Maps.newHashMap();
        mIsDirty = false;
    }

    public boolean isBatch() {
        return mBatch;
    }

    public boolean isDirty() {
        return mIsDirty;
    }

    public void markDirty() {
        mIsDirty = true;
    }

    public void markYieldFailed() {
        mYieldFailed = true;
    }

    /**
     * If the given database has not already been enlisted in this transaction, adds it to our
     * list of affected databases and starts a transaction on it.  If we already have the given
     * database in this transaction, this is a no-op.
     * @param db The database to start a transaction on, if necessary.
     * @param tag A constant that can be used to retrieve the DB instance in this transaction.
     * @param listener A transaction listener to attach to this transaction.  May be null.
     */
    public void startTransactionForDb(SQLiteDatabase db, String tag,
            SQLiteTransactionListener listener) {
        if (AbstractContactsProvider.ENABLE_TRANSACTION_LOG) {
            Log.i(AbstractContactsProvider.TAG, "startTransactionForDb: db=" + db.getPath() +
                    "  tag=" + tag + "  listener=" + listener +
                    "  startTransaction=" + !hasDbInTransaction(tag),
                    new RuntimeException("startTransactionForDb"));
        }
        if (!hasDbInTransaction(tag)) {
            // Insert a new db into the head of the list, so that we'll endTransaction() in
            // the reverse order.
            mDatabasesForTransaction.add(0, db);
            mDatabaseTagMap.put(tag, db);
            if (listener != null) {
                db.beginTransactionWithListenerNonExclusive(listener);
            } else {
                db.beginTransactionNonExclusive();
            }
        }
    }

    /**
     * Returns whether DB corresponding to the given tag is currently enlisted in this transaction.
     */
    public boolean hasDbInTransaction(String tag) {
        return mDatabaseTagMap.containsKey(tag);
    }

    /**
     * Retrieves the database enlisted in the transaction corresponding to the given tag.
     * @param tag The tag of the database to look up.
     * @return The database corresponding to the tag, or null if no database with that tag has been
     *     enlisted in this transaction.
     */
    public SQLiteDatabase getDbForTag(String tag) {
        return mDatabaseTagMap.get(tag);
    }

    /**
     * Removes the database corresponding to the given tag from this transaction.  It is now the
     * caller's responsibility to do whatever needs to happen with this database - it is no longer
     * a part of this transaction.
     * @param tag The tag of the database to remove.
     * @return The database corresponding to the tag, or null if no database with that tag has been
     *     enlisted in this transaction.
     */
    public SQLiteDatabase removeDbForTag(String tag) {
        SQLiteDatabase db = mDatabaseTagMap.get(tag);
        mDatabaseTagMap.remove(tag);
        mDatabasesForTransaction.remove(db);
        return db;
    }

    /**
     * Marks all active DB transactions as successful.
     * @param callerIsBatch Whether this is being performed in the context of a batch operation.
     *     If it is not, and the transaction is marked as batch, this call is a no-op.
     */
    public void markSuccessful(boolean callerIsBatch) {
        if (!mBatch || callerIsBatch) {
            for (SQLiteDatabase db : mDatabasesForTransaction) {
                db.setTransactionSuccessful();
            }
        }
    }

    /**
     * @return the tag for a database.  Only intended to be used for logging.
     */
    private String getTagForDb(SQLiteDatabase db) {
        for (String tag : mDatabaseTagMap.keySet()) {
            if (db == mDatabaseTagMap.get(tag)) {
                return tag;
            }
        }
        return null;
    }

    /**
     * Completes the transaction, ending the DB transactions for all associated databases.
     * @param callerIsBatch Whether this is being performed in the context of a batch operation.
     *     If it is not, and the transaction is marked as batch, this call is a no-op.
     */
    public void finish(boolean callerIsBatch) {
        if (AbstractContactsProvider.ENABLE_TRANSACTION_LOG) {
            Log.i(AbstractContactsProvider.TAG, "ContactsTransaction.finish  callerIsBatch=" +
                    callerIsBatch, new RuntimeException("ContactsTransaction.finish"));
        }
        if (!mBatch || callerIsBatch) {
            for (SQLiteDatabase db : mDatabasesForTransaction) {
                if (AbstractContactsProvider.ENABLE_TRANSACTION_LOG) {
                    Log.i(AbstractContactsProvider.TAG, "ContactsTransaction.finish: " +
                            "endTransaction for " + getTagForDb(db));
                }
                // If an exception was thrown while yielding, it's possible that we no longer have
                // a lock on this database, so we need to check before attempting to end its
                // transaction.  Otherwise, we should always expect to be in a transaction (and will
                // throw an exception if this is not the case).
                if (mYieldFailed && !db.isDbLockedByCurrentThread()) {
                    // We no longer hold the lock, so don't do anything with this database.
                    continue;
                }
                db.endTransaction();
            }
            mDatabasesForTransaction.clear();
            mDatabaseTagMap.clear();
            mIsDirty = false;
        }
    }
}
