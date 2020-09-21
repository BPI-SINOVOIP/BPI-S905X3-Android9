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

import com.android.mail.providers.Folder;

import java.util.Set;

public final class SystemFolderSelectorAdapter extends FolderSelectorAdapter {

    public SystemFolderSelectorAdapter(Context context, Cursor folders,
            Set<String> initiallySelected, int layout) {
        super(context, folders, initiallySelected, layout);
    }

    public SystemFolderSelectorAdapter(Context context, Cursor folders,
            int layout, Folder excludedFolder) {
        super(context, folders, layout, excludedFolder);
    }

    /**
     * Return whether the supplied folder meets the requirements to be displayed
     * in the folder list.
     */
    @Override
    protected boolean meetsRequirements(Folder folder) {
        /*
         * TODO: Only show inboxes until we have a way to exclude things like STARRED and SPAM,
         * but allow other system folders.
         */
        return folder.isInbox();

        // We only want to show system folders.
        // return folder.supportsCapability(FolderCapabilities.CAN_ACCEPT_MOVED_MESSAGES)
        //             && folder.isProviderFolder();
    }
}
