/*
 * Copyright (C) 2015 The Android Open Source Project
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

package com.android.tv.util;

import android.content.ContentResolver;
import android.database.Cursor;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Programs;
import android.net.Uri;
import android.os.AsyncTask;
import android.support.annotation.MainThread;
import android.support.annotation.Nullable;
import android.support.annotation.WorkerThread;
import android.util.Log;
import android.util.Range;

import com.android.tv.TvSingletons;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.data.ChannelImpl;
import com.android.tv.data.Program;
import com.android.tv.data.api.Channel;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.common.util.SystemProperties;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * {@link AsyncTask} that defaults to executing on its own single threaded Executor Service.
 *
 * @param <Params> the type of the parameters sent to the task upon execution.
 * @param <Progress> the type of the progress units published during the background computation.
 * @param <Result> the type of the result of the background computation.
 */
public abstract class AsyncDbTask<Params, Progress, Result>
        extends AsyncTask<Params, Progress, Result> {
    private static final String TAG = "AsyncDbTask";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_CHANNEL_UPDATE.getValue();

    private final Executor mExecutor;
    boolean mCalledExecuteOnDbThread;

    protected AsyncDbTask(Executor mExecutor) {
        this.mExecutor = mExecutor;
    }

    /**
     * Returns the result of a {@link ContentResolver#query(Uri, String[], String, String[],
     * String)}.
     *
     * <p>{@link #doInBackground(Void...)} executes the query on call {@link #onQuery(Cursor)} which
     * is implemented by subclasses.
     *
     * @param <Result> the type of result returned by {@link #onQuery(Cursor)}
     */
    public abstract static class AsyncQueryTask<Result> extends AsyncDbTask<Void, Void, Result> {
        private final ContentResolver mContentResolver;
        private final Uri mUri;
        private final String[] mProjection;
        private final String mSelection;
        private final String[] mSelectionArgs;
        private final String mOrderBy;

        public AsyncQueryTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String orderBy) {
            super(executor);
            mContentResolver = contentResolver;
            mUri = uri;
            mProjection = projection;
            mSelection = selection;
            mSelectionArgs = selectionArgs;
            mOrderBy = orderBy;
        }

        @Override
        protected final Result doInBackground(Void... params) {
            if (!mCalledExecuteOnDbThread) {
                IllegalStateException e =
                        new IllegalStateException(
                                this
                                        + " should only be executed using executeOnDbThread, "
                                        + "but it was called on thread "
                                        + Thread.currentThread());
                Log.w(TAG, e);
                if (BuildConfig.ENG) {
                    throw e;
                }
            }

            if (isCancelled()) {
                // This is guaranteed to never call onPostExecute because the task is canceled.
                return null;
            }
            if (DEBUG) {
                Log.v(TAG, "Starting query for " + this);
            }
            try (Cursor c =
                    mContentResolver.query(
                            mUri, mProjection, mSelection, mSelectionArgs, mOrderBy)) {
                if (c != null && !isCancelled()) {
                    Result result = onQuery(c);
                    if (DEBUG) {
                        Log.v(TAG, "Finished query for " + this);
                    }
                    return result;
                } else {
                    if (c == null) {
                        Log.e(TAG, "Unknown query error for " + this);
                    } else {
                        if (DEBUG) {
                            Log.d(TAG, "Canceled query for " + this);
                        }
                    }
                    return null;
                }
            } catch (Exception e) {
                SoftPreconditions.warn(TAG, null, e, "Error querying " + this);
                return null;
            }
        }

        /**
         * Return the result from the cursor.
         *
         * <p><b>Note</b> This is executed on the DB thread by {@link #doInBackground(Void...)}
         */
        @WorkerThread
        protected abstract Result onQuery(Cursor c);

        @Override
        public String toString() {
            return this.getClass().getName() + "(" + mUri + ")";
        }
    }

    /**
     * Returns the result of a query as an {@link List} of {@code T}.
     *
     * <p>Subclasses must implement {@link #fromCursor(Cursor)}.
     *
     * @param <T> the type of result returned in a list by {@link #onQuery(Cursor)}
     */
    public abstract static class AsyncQueryListTask<T> extends AsyncQueryTask<List<T>> {
        private final CursorFilter mFilter;

        public AsyncQueryListTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String orderBy) {
            this(
                    executor,
                    contentResolver,
                    uri,
                    projection,
                    selection,
                    selectionArgs,
                    orderBy,
                    null);
        }

        public AsyncQueryListTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String orderBy,
                CursorFilter filter) {
            super(executor, contentResolver, uri, projection, selection, selectionArgs, orderBy);
            mFilter = filter;
        }

        @Override
        protected final List<T> onQuery(Cursor c) {
            List<T> result = new ArrayList<>();
            while (c.moveToNext()) {
                if (isCancelled()) {
                    // This is guaranteed to never call onPostExecute because the task is canceled.
                    return null;
                }
                if (mFilter != null && !mFilter.filter(c)) {
                    continue;
                }
                T t = fromCursor(c);
                result.add(t);
            }
            if (DEBUG) {
                Log.v(TAG, "Found " + result.size() + " for  " + this);
            }
            return result;
        }

        /**
         * Return a single instance of {@code T} from the cursor.
         *
         * <p><b>NOTE</b> Do not move the cursor or close it, that is handled by {@link
         * #onQuery(Cursor)}.
         *
         * <p><b>Note</b> This is executed on the DB thread by {@link #onQuery(Cursor)}
         *
         * @param c The cursor with the values to create T from.
         */
        @WorkerThread
        protected abstract T fromCursor(Cursor c);
    }

    /**
     * Returns the result of a query as a single instance of {@code T}.
     *
     * <p>Subclasses must implement {@link #fromCursor(Cursor)}.
     */
    public abstract static class AsyncQueryItemTask<T> extends AsyncQueryTask<T> {

        public AsyncQueryItemTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String orderBy) {
            super(executor, contentResolver, uri, projection, selection, selectionArgs, orderBy);
        }

        @Override
        protected final T onQuery(Cursor c) {
            if (c.moveToNext()) {
                if (isCancelled()) {
                    // This is guaranteed to never call onPostExecute because the task is canceled.
                    return null;
                }
                T result = fromCursor(c);
                if (c.moveToNext()) {
                    Log.w(TAG, "More than one result for found for  " + this);
                }
                return result;
            } else {
                if (DEBUG) {
                    Log.v(TAG, "No result for found  for  " + this);
                }
                return null;
            }
        }

        /**
         * Return a single instance of {@code T} from the cursor.
         *
         * <p><b>NOTE</b> Do not move the cursor or close it, that is handled by {@link
         * #onQuery(Cursor)}.
         *
         * <p><b>Note</b> This is executed on the DB thread by {@link #onQuery(Cursor)}
         *
         * @param c The cursor with the values to create T from.
         */
        @WorkerThread
        protected abstract T fromCursor(Cursor c);
    }

    /** Gets an {@link List} of {@link Channel}s from {@link TvContract.Channels#CONTENT_URI}. */
    public abstract static class AsyncChannelQueryTask extends AsyncQueryListTask<Channel> {

        public AsyncChannelQueryTask(Executor executor, ContentResolver contentResolver) {
            super(
                    executor,
                    contentResolver,
                    TvContract.Channels.CONTENT_URI,
                    ChannelImpl.PROJECTION,
                    null,
                    null,
                    null);
        }

        @Override
        protected final Channel fromCursor(Cursor c) {
            return ChannelImpl.fromCursor(c);
        }
    }

    /** Gets an {@link List} of {@link Program}s from {@link TvContract.Programs#CONTENT_URI}. */
    public abstract static class AsyncProgramQueryTask extends AsyncQueryListTask<Program> {
        public AsyncProgramQueryTask(Executor executor, ContentResolver contentResolver) {
            super(
                    executor,
                    contentResolver,
                    Programs.CONTENT_URI,
                    Program.PROJECTION,
                    null,
                    null,
                    null);
        }

        public AsyncProgramQueryTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String selection,
                String[] selectionArgs,
                String sortOrder,
                CursorFilter filter) {
            super(
                    executor,
                    contentResolver,
                    uri,
                    Program.PROJECTION,
                    selection,
                    selectionArgs,
                    sortOrder,
                    filter);
        }

        @Override
        protected final Program fromCursor(Cursor c) {
            return Program.fromCursor(c);
        }
    }

    /** Gets an {@link List} of {@link TvContract.RecordedPrograms}s. */
    public abstract static class AsyncRecordedProgramQueryTask
            extends AsyncQueryListTask<RecordedProgram> {
        public AsyncRecordedProgramQueryTask(
                Executor executor, ContentResolver contentResolver, Uri uri) {
            super(executor, contentResolver, uri, RecordedProgram.PROJECTION, null, null, null);
        }

        @Override
        protected final RecordedProgram fromCursor(Cursor c) {
            return RecordedProgram.fromCursor(c);
        }
    }

    /** Execute the task on {@link TvSingletons#getDbExecutor()}. */
    @SafeVarargs
    @MainThread
    public final void executeOnDbThread(Params... params) {
        mCalledExecuteOnDbThread = true;
        executeOnExecutor(mExecutor, params);
    }

    /**
     * Gets an {@link List} of {@link Program}s for a given channel and period {@link
     * TvContract#buildProgramsUriForChannel(long, long, long)}. If the {@code period} is {@code
     * null}, then all the programs is queried.
     */
    public static class LoadProgramsForChannelTask extends AsyncProgramQueryTask {
        protected final Range<Long> mPeriod;
        protected final long mChannelId;

        public LoadProgramsForChannelTask(
                Executor executor,
                ContentResolver contentResolver,
                long channelId,
                @Nullable Range<Long> period) {
            super(
                    executor,
                    contentResolver,
                    period == null
                            ? TvContract.buildProgramsUriForChannel(channelId)
                            : TvContract.buildProgramsUriForChannel(
                                    channelId, period.getLower(), period.getUpper()),
                    null,
                    null,
                    null,
                    null);
            mPeriod = period;
            mChannelId = channelId;
        }

        public long getChannelId() {
            return mChannelId;
        }

        public final Range<Long> getPeriod() {
            return mPeriod;
        }
    }

    /** Gets a single {@link Program} from {@link TvContract.Programs#CONTENT_URI}. */
    public static class AsyncQueryProgramTask extends AsyncQueryItemTask<Program> {

        public AsyncQueryProgramTask(
                Executor executor, ContentResolver contentResolver, long programId) {
            super(
                    executor,
                    contentResolver,
                    TvContract.buildProgramUri(programId),
                    Program.PROJECTION,
                    null,
                    null,
                    null);
        }

        @Override
        protected Program fromCursor(Cursor c) {
            return Program.fromCursor(c);
        }
    }

    /** An interface which filters the row. */
    public interface CursorFilter extends Filter<Cursor> {}
}
