/*******************************************************************************
 *      Copyright (C) 2012 Google Inc.
 *      Licensed to The Android Open Source Project.
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *           http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 *******************************************************************************/

package com.android.mail.ui;

import android.content.Context;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;

import com.android.mail.providers.Folder;
import com.android.mail.providers.UIProvider;
import com.android.mail.utils.FolderUri;
import com.android.mail.utils.MatrixCursorWithCachedColumns;

import java.util.Set;

public class AddableFolderSelectorAdapter extends FolderSelectorAdapter {

    public AddableFolderSelectorAdapter(Context context, Cursor folders,
            Set<String> selected, int layout) {
        super(context, folders, selected, layout);
    }

    /**
     * Essentially uses filterFolders with no filter to convert the cursors
     */
    public static Cursor filterFolders(final Cursor folderCursor) {
        return filterFolders(folderCursor, null /* excludedTypes */, null /* initiallySelected */,
                true /* includeInitiallySelected, useless in this case */);
    }

    /**
     * @param folderCursor
     * @param excludedTypes folder types that we want to filter out.
     * @param initiallySelected set of folder uris that are previously selected.
     * @param includeOnlyInitiallySelected if we want to ONLY include or exclude initiallySelected,
     *   doesn't do anything if initiallySelected is null.
     * @return
     */
    public static Cursor filterFolders(final Cursor folderCursor,
            final Set<Integer> excludedTypes, final Set<String> initiallySelected,
            final boolean includeOnlyInitiallySelected) {
        final int projectionSize = UIProvider.FOLDERS_PROJECTION.length;
        final MatrixCursor cursor =
                new MatrixCursorWithCachedColumns(UIProvider.FOLDERS_PROJECTION);
        final Object[] folder = new Object[projectionSize];
        if (folderCursor.moveToFirst()) {
            do {
                final int type = folderCursor.getInt(UIProvider.FOLDER_TYPE_COLUMN);

                // Check for excluded types
                boolean exclude = false;
                if (excludedTypes != null) {
                    for (final int excludedType : excludedTypes) {
                        if (Folder.isType(type, excludedType)) {
                            exclude = true;
                            break;
                        }
                    }
                }

                if (initiallySelected != null) {
                    // TODO: there has to be a better way to get this value...
                    String uri = new FolderUri(Uri.parse(folderCursor.getString(
                            UIProvider.FOLDER_URI_COLUMN))).getComparisonUri().toString();
                    // Check if the folder is already selected and if we are trying to include only
                    // the ones that were initially selected or only the ones that aren't.
                    // Xor is what we want, since we essentially want the following:
                    // (includeOnlyInitial && !contains) || (!includeOnlyInitial && contains)
                    exclude |= includeOnlyInitiallySelected ^ initiallySelected.contains(uri);
                }

                if (exclude) {
                    continue;
                }

                if (Folder.isType(type, UIProvider.FolderType.INBOX)
                        || Folder.isType(type, UIProvider.FolderType.DEFAULT)) {
                    folder[UIProvider.FOLDER_ID_COLUMN] = folderCursor
                            .getLong(UIProvider.FOLDER_ID_COLUMN);
                    folder[UIProvider.FOLDER_PERSISTENT_ID_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_PERSISTENT_ID_COLUMN);
                    folder[UIProvider.FOLDER_URI_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_URI_COLUMN);
                    folder[UIProvider.FOLDER_NAME_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_NAME_COLUMN);
                    folder[UIProvider.FOLDER_HAS_CHILDREN_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_HAS_CHILDREN_COLUMN);
                    folder[UIProvider.FOLDER_CAPABILITIES_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_CAPABILITIES_COLUMN);
                    folder[UIProvider.FOLDER_SYNC_WINDOW_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_SYNC_WINDOW_COLUMN);
                    folder[UIProvider.FOLDER_CONVERSATION_LIST_URI_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_CONVERSATION_LIST_URI_COLUMN);
                    folder[UIProvider.FOLDER_CHILD_FOLDERS_LIST_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_CHILD_FOLDERS_LIST_COLUMN);
                    folder[UIProvider.FOLDER_UNSEEN_COUNT_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_UNSEEN_COUNT_COLUMN);
                    folder[UIProvider.FOLDER_UNREAD_COUNT_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_UNREAD_COUNT_COLUMN);
                    folder[UIProvider.FOLDER_TOTAL_COUNT_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_TOTAL_COUNT_COLUMN);
                    folder[UIProvider.FOLDER_REFRESH_URI_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_REFRESH_URI_COLUMN);
                    folder[UIProvider.FOLDER_SYNC_STATUS_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_SYNC_STATUS_COLUMN);
                    folder[UIProvider.FOLDER_LAST_SYNC_RESULT_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_LAST_SYNC_RESULT_COLUMN);
                    folder[UIProvider.FOLDER_TYPE_COLUMN] = type;
                    folder[UIProvider.FOLDER_ICON_RES_ID_COLUMN] = folderCursor
                            .getInt(UIProvider.FOLDER_ICON_RES_ID_COLUMN);
                    folder[UIProvider.FOLDER_BG_COLOR_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_BG_COLOR_COLUMN);
                    folder[UIProvider.FOLDER_FG_COLOR_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_FG_COLOR_COLUMN);
                    folder[UIProvider.FOLDER_LOAD_MORE_URI_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_LOAD_MORE_URI_COLUMN);
                    folder[UIProvider.FOLDER_HIERARCHICAL_DESC_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_HIERARCHICAL_DESC_COLUMN);
                    folder[UIProvider.FOLDER_LAST_MESSAGE_TIMESTAMP_COLUMN] = folderCursor
                            .getLong(UIProvider.FOLDER_LAST_MESSAGE_TIMESTAMP_COLUMN);
                    folder[UIProvider.FOLDER_PARENT_URI_COLUMN] = folderCursor
                            .getString(UIProvider.FOLDER_PARENT_URI_COLUMN);
                    cursor.addRow(folder);
                }
            } while (folderCursor.moveToNext());
        }
        return cursor;
    }
}
