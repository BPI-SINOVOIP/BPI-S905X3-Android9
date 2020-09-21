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
package com.android.documentsui.inspector;

import static com.android.internal.util.Preconditions.checkArgument;

import android.app.LoaderManager;
import android.app.LoaderManager.LoaderCallbacks;
import android.content.Context;
import android.content.CursorLoader;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.provider.DocumentsContract;
import android.support.annotation.Nullable;

import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.inspector.InspectorController.DataSupplier;

import java.util.ArrayList;
import java.util.List;
import java.util.function.Consumer;

/**
 * Asynchronously loads a document data for the inspector.
 *
 * <p>This loader is not a Loader! Its our own funky loader.
 */
public class RuntimeDataSupplier implements DataSupplier {

    private final Context mContext;
    private final LoaderManager mLoaderMgr;
    private final List<Integer> loaderIds = new ArrayList<>();
    private @Nullable Callbacks mDocCallbacks;
    private @Nullable Callbacks mDirCallbacks;
    private @Nullable LoaderCallbacks<Bundle> mMetadataCallbacks;

    public RuntimeDataSupplier(Context context, LoaderManager loaderMgr) {
        checkArgument(context != null);
        checkArgument(loaderMgr != null);
        mContext = context;
        mLoaderMgr = loaderMgr;
    }

    /**
     * Loads documents metadata.
     */
    @Override
    public void loadDocInfo(Uri uri, Consumer<DocumentInfo> updateView) {
        //Check that we have correct Uri type and that the loader is not already created.
        checkArgument(uri.getScheme().equals("content"));

        Consumer<Cursor> callback = new Consumer<Cursor>() {
            @Override
            public void accept(Cursor cursor) {

                if (cursor == null || !cursor.moveToFirst()) {
                    updateView.accept(null);
                } else {
                    DocumentInfo docInfo = DocumentInfo.fromCursor(cursor, uri.getAuthority());
                    updateView.accept(docInfo);
                }
            }
        };

        mDocCallbacks = new Callbacks(mContext, uri, callback);
        mLoaderMgr.restartLoader(getNextLoaderId(), null, mDocCallbacks);
    }

    /**
     * Loads a directories item count.
     */
    @Override
    public void loadDirCount(DocumentInfo directory, Consumer<Integer> updateView) {
        checkArgument(directory.isDirectory());
        Uri children = DocumentsContract.buildChildDocumentsUri(
                directory.authority, directory.documentId);

        Consumer<Cursor> callback = new Consumer<Cursor>() {
            @Override
            public void accept(Cursor cursor) {
                if(cursor != null && cursor.moveToFirst()) {
                    updateView.accept(cursor.getCount());
                }
            }
        };

        mDirCallbacks = new Callbacks(mContext, children, callback);
        mLoaderMgr.restartLoader(getNextLoaderId(), null, mDirCallbacks);
    }

    @Override
    public void getDocumentMetadata(Uri uri, Consumer<Bundle> callback) {
        mMetadataCallbacks = new LoaderCallbacks<Bundle>() {
            @Override
            public android.content.Loader<Bundle> onCreateLoader(int id, Bundle unused) {
                return new MetadataLoader(mContext, uri);
            }

            @Override
            public void onLoadFinished(android.content.Loader<Bundle> loader, Bundle data) {
                callback.accept(data);
            }

            @Override
            public void onLoaderReset(android.content.Loader<Bundle> loader) {
            }
        };

        // TODO: Listen for changes on content URI.
        mLoaderMgr.restartLoader(getNextLoaderId(), null, mMetadataCallbacks);
    }

    @Override
    public void reset() {
        for (Integer id : loaderIds) {
            mLoaderMgr.destroyLoader(id);
        }
        loaderIds.clear();

        if (mDocCallbacks != null && mDocCallbacks.getObserver() != null) {
            mContext.getContentResolver().unregisterContentObserver(mDocCallbacks.getObserver());
        }

        if (mDirCallbacks != null && mDirCallbacks.getObserver() != null) {
            mContext.getContentResolver().unregisterContentObserver(mDirCallbacks.getObserver());
        }
    }

    private int getNextLoaderId() {
        int id = 0;
        while(mLoaderMgr.getLoader(id) != null) {
            id++;
            checkArgument(id <= Integer.MAX_VALUE);
        }
        loaderIds.add(id);
        return id;
    }

    /**
     * Implements the callback interface for cursor loader.
     */
    static final class Callbacks implements LoaderCallbacks<Cursor> {

        private final Context mContext;
        private final Uri mUri;
        private final Consumer<Cursor> mCallback;
        private ContentObserver mObserver;

        Callbacks(Context context, Uri uri, Consumer<Cursor> callback) {
            checkArgument(context != null);
            checkArgument(uri != null);
            checkArgument(callback != null);
            mContext = context;
            mUri = uri;
            mCallback = callback;
        }

        @Override
        public android.content.Loader<Cursor> onCreateLoader(int id, Bundle args) {
            return new CursorLoader(mContext, mUri, null, null, null, null);
        }

        @Override
        public void onLoadFinished(android.content.Loader<Cursor> loader, Cursor cursor) {

            if (cursor != null) {
                mObserver = new InspectorContentObserver(loader::onContentChanged);
                cursor.registerContentObserver(mObserver);
            }

            mCallback.accept(cursor);
        }

        @Override
        public void onLoaderReset(android.content.Loader<Cursor> loader) {
            if (mObserver != null) {
                mContext.getContentResolver().unregisterContentObserver(mObserver);
            }
        }

        public ContentObserver getObserver() {
            return mObserver;
        }
    }

    private static final class InspectorContentObserver extends ContentObserver {
        private final Runnable mContentChangedCallback;

        public InspectorContentObserver(Runnable contentChangedCallback) {
            super(new Handler(Looper.getMainLooper()));
            mContentChangedCallback = contentChangedCallback;
        }

        @Override
        public void onChange(boolean selfChange) {
            mContentChangedCallback.run();
        }
    }
}
