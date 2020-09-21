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

import android.content.Context;
import android.text.format.Formatter;
import android.util.AttributeSet;

import com.android.documentsui.DocumentsApplication;
import com.android.documentsui.R;
import com.android.documentsui.base.DocumentInfo;
import com.android.documentsui.base.Lookup;
import com.android.documentsui.inspector.InspectorController.DetailsDisplay;

/**
 * Displays the basic details about a file.
 */
public class DetailsView extends TableView implements DetailsDisplay {

    public DetailsView(Context context) {
        this(context, null);
    }

    public DetailsView(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public DetailsView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    @Override
    public void accept(DocumentInfo doc) {

        Lookup<String, String> fileTypeLookup =
                DocumentsApplication.getFileTypeLookup(getContext());

        String mimeType = fileTypeLookup.lookup(doc.mimeType);

        put(R.string.sort_dimension_file_type, mimeType);

        // TODO: Each of these rows need to be removed if the condition is false and previously
        // set.
        if (doc.size >= 0 && !doc.isDirectory()) {
            put(R.string.sort_dimension_size, Formatter.formatFileSize(getContext(), doc.size));
        }

        if (doc.lastModified > 0) {
            put(R.string.sort_dimension_date,
                    DateUtils.formatDate(this.getContext(), doc.lastModified));
        }

        // We only show summary field when doc is partial (meaning an active download).
        // The rest of the time "summary" tends to be less than useful. For example
        // after a download is completed DownloadsProvider include the orig filename
        // in the summary field. This is confusing to folks in-and-if-itself, but
        // after the file is renamed, it creates even more confusion (since it still
        // shows the original). For that reason, and others. We only display on partial files.
        if (doc.isPartial() && doc.summary != null) {
            put(R.string.sort_dimension_summary, doc.summary);
        }
    }

    @Override
    public void setChildrenCount(int count) {
        put(R.string.directory_items, String.valueOf(count));
    }
}
