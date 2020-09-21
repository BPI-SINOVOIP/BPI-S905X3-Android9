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

import android.annotation.StringRes;
import android.content.Context;
import android.content.res.Resources;
import android.os.AsyncTask;
import android.os.Bundle;
import android.provider.DocumentsContract;
import android.util.AttributeSet;
import android.widget.TextView;

import com.android.documentsui.R;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.DummyLookup;
import com.android.documentsui.base.Lookup;
import com.android.documentsui.inspector.InspectorController.DebugDisplay;

import java.text.NumberFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.Executor;

/**
 * Organizes and Displays the debug information about a file. This view
 * should only be made visible when build is debuggable and system policies
 * allow debug "stuff".
 */
public class DebugView extends TableView implements DebugDisplay {

    private final Context mContext;
    private final Resources mRes;
    private Lookup<String, Executor> mExecutors = new DummyLookup<>();

    public DebugView(Context context) {
        this(context, null);
    }

    public DebugView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DebugView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        mContext = context;
        mRes = context.getResources();
    }

    void init(Lookup<String, Executor> executors) {
        assert executors != null;
        setBackgroundColor(0xFFFFFFFF);  // it's just debug. We do what we want!
        mExecutors = executors;
    }

    @Override
    public void accept(DocumentInfo info) {
        setTitle(R.string.inspector_debug_section, false);

        put(R.string.debug_content_uri, info.derivedUri.toString());
        put(R.string.debug_document_id, info.documentId);
        put(R.string.debug_raw_mimetype, info.mimeType);
        put(R.string.debug_stream_types, "-");
        put(R.string.debug_raw_size, NumberFormat.getInstance().format(info.size));
        put(R.string.debug_is_archive, info.isArchive());
        put(R.string.debug_is_container, info.isContainer());
        put(R.string.debug_is_partial, info.isPartial());
        put(R.string.debug_is_virtual, info.isVirtual());
        put(R.string.debug_supports_create, info.isCreateSupported());
        put(R.string.debug_supports_delete, info.isDeleteSupported());
        put(R.string.debug_supports_metadata, info.isMetadataSupported());
        put(R.string.debug_supports_remove, info.isRemoveSupported());
        put(R.string.debug_supports_rename, info.isRenameSupported());
        put(R.string.debug_supports_settings, info.isSettingsSupported());
        put(R.string.debug_supports_thumbnail, info.isThumbnailSupported());
        put(R.string.debug_supports_weblink, info.isWeblinkSupported());
        put(R.string.debug_supports_write, info.isWriteSupported());

        // Load Document stream types of the file. For virtual files, this should be
        // something other than the primary type of the file.
        Executor executor = mExecutors.lookup(info.derivedUri.getAuthority());
        if (executor != null) {
            new AsyncTask<Void, Void, String[]>() {
                @Override
                protected String[] doInBackground(Void... params) {
                    return mContext.getContentResolver().getStreamTypes(info.derivedUri, "*/*");
                }

                @Override
                protected void onPostExecute(String[] streamTypes) {
                    put(R.string.debug_stream_types,
                            streamTypes != null ? Arrays.toString(streamTypes) : "[]");
                }
            }.executeOnExecutor(executor, (Void[]) null);
        }
    }

    @Override
    public void accept(Bundle metadata) {
        if (metadata == null) {
            return;
        }

        String[] types = metadata.getStringArray(DocumentsContract.METADATA_TYPES);
        if (types == null) {
            return;
        }

        for (String type : types) {
            dumpMetadata(type, metadata.getBundle(type));
        }
    }

    private void dumpMetadata(String type, Bundle bundle) {
        String title = mContext.getResources().getString(
                R.string.inspector_debug_metadata_section);
        putTitle(String.format(title, type), true);
        List<String> keys = new ArrayList<>(bundle.keySet());
        Collections.sort(keys);
        for (String key : keys) {
            put(key, String.valueOf(bundle.get(key)));
        }
    }

    private void put(@StringRes int key, boolean value) {
        KeyValueRow row = put(mRes.getString(key), String.valueOf(value));
        TextView valueView = ((TextView) row.findViewById(R.id.table_row_value));
        valueView.setTextColor(value ? 0xFF006400 : 0xFF9A2020);
    }
}
