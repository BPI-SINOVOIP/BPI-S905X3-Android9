/*
 * Copyright (C) 2012 Google Inc.
 * Licensed to The Android Open Source Project.
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

package com.android.mail.ui;

import android.os.Parcel;
import android.os.Parcelable;

import com.android.mail.providers.Folder;
import com.google.common.base.Objects;
import com.google.common.collect.ImmutableList;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Collections;
import java.util.HashSet;

public class FolderOperation implements Parcelable {
    /** An immutable, empty conversation list */
    public static final Collection<FolderOperation> EMPTY = Collections.emptyList();
    public final Folder mFolder;
    public final boolean mAdd;

    public FolderOperation(Folder folder, Boolean operation) {
        mAdd = operation;
        mFolder = folder;
    }

    /**
     * Get all the unique folders associated with a set of folder operations.
     * @param ops Operations to inspect
     * @return ArrayList of Folder objects
     */
    public static ArrayList<Folder> getFolders(Collection<FolderOperation> ops) {
        HashSet<Folder> folders = new HashSet<Folder>();
        for (FolderOperation op : ops) {
            folders.add(op.mFolder);
        }
        return new ArrayList<Folder>(folders);
    }

    /**
     * Returns a collection of a single FolderOperation. This method always
     * returns a valid collection even if the input folder is null.
     *
     * @param in a FolderOperation, possibly null.
     * @return a collection of the folder.
     */
    public static Collection<FolderOperation> listOf(FolderOperation in) {
        final Collection<FolderOperation> target = (in == null) ? EMPTY : ImmutableList.of(in);
        return target;
    }

    /**
     * Return if a set of folder operations removes the specified folder or adds
     * inbox to a trashed conversation, making it a destructive operation.
     */
    public static boolean isDestructive(Collection<FolderOperation> folderOps, Folder folder) {
        for (FolderOperation op : folderOps) {
            if (Objects.equal(op.mFolder, folder) && !op.mAdd) {
                return true;
            }
            if (folder.isTrash() && op.mFolder.isInbox()) {
                return true;
            }
        }
        return false;
    }

    // implements Parcelable

    FolderOperation(Parcel in) {
        mAdd = in.readByte() != 0;
        mFolder = in.readParcelable(getClass().getClassLoader());
    }

    @Override
    public int describeContents() {
        return 0;
    }

    @Override
    public void writeToParcel(Parcel dest, int flags) {
        dest.writeByte((byte)(mAdd ? 1 : 0));
        dest.writeParcelable(mFolder, 0);
    }

    public static final Creator<FolderOperation> CREATOR = new Creator<FolderOperation>() {
        @Override
        public FolderOperation createFromParcel(Parcel source) {
            return new FolderOperation(source);
        }

        @Override
        public FolderOperation[] newArray(int size) {
            return new FolderOperation[size];
        }
    };
}
