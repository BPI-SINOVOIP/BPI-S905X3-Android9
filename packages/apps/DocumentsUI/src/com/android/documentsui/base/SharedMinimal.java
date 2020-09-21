/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.documentsui.base;

import static android.os.Environment.isStandardDirectory;

import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_ERROR;
import static com.android.documentsui.ScopedAccessMetrics.SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY;
import static com.android.documentsui.ScopedAccessMetrics.logInvalidScopedAccessRequest;

import android.annotation.Nullable;
import android.content.ContentProviderClient;
import android.content.Context;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.RemoteException;
import android.os.storage.StorageManager;
import android.os.storage.StorageVolume;
import android.os.storage.VolumeInfo;
import android.provider.DocumentsContract;
import android.text.TextUtils;
import android.util.Log;

import java.io.File;
import java.io.IOException;
import java.util.List;

/**
 * Contains the minimum number of utilities (contants, helpers, etc...) that can be used by both the
 * main package and the minimal APK that's used by Android TV (and other devices).
 *
 * <p>In other words, it should not include any external dependency that would increase the APK
 * size.
 */
public final class SharedMinimal {

    public static final String TAG = "Documents";

    public static final boolean DEBUG = Build.IS_DEBUGGABLE;
    public static final boolean VERBOSE = DEBUG && Log.isLoggable(TAG, Log.VERBOSE);

    /**
     * Special directory name representing the full volume of a scoped directory request.
     */
    public static final String DIRECTORY_ROOT = "ROOT_DIRECTORY";

    /**
     * Callback for {@link SharedMinimal#getUriPermission(Context, ContentProviderClient,
     * StorageVolume, String, int, boolean, GetUriPermissionCallback)}.
     */
    public static interface GetUriPermissionCallback {

        /**
         * Evaluates the result of the request.
         *
         * @param file the path of the requested URI.
         * @param volumeLabel user-friendly label of the volume.
         * @param isRoot whether the requested directory is the root directory.
         * @param isPrimary whether the requested volume is the primary storage volume.
         * @param requestedUri the requested URI.
         * @param rootUri the URI for the volume's root directory.
         * @return whethe the result was sucessfully.
         */
        boolean onResult(File file, String volumeLabel, boolean isRoot, boolean isPrimary,
                Uri requestedUri, Uri rootUri);
    }

    /**
     * Gets the name of a directory name in the format that's used internally by the app
     * (i.e., mapping {@code null} to {@link #DIRECTORY_ROOT});
     * if necessary.
     */
    public static String getInternalDirectoryName(@Nullable String name) {
        return name == null ? DIRECTORY_ROOT : name;
    }

    /**
     * Gets the name of a directory name in the format that is used externally
     * (i.e., mapping {@link #DIRECTORY_ROOT} to {@code null} if necessary);
     */
    @Nullable
    public static String getExternalDirectoryName(String name) {
        return name.equals(DIRECTORY_ROOT) ? null : name;
    }

    /**
     * Gets the URI permission for the given volume and directory.
     *
     * @param context caller's context.
     * @param storageClient storage provider client.
     * @param storageVolume volume.
     * @param directoryName directory name, or {@link #DIRECTORY_ROOT} for full volume.
     * @param userId caller's user handle.
     * @param logMetrics whether intermediate errors should be logged.
     * @param callback callback that receives the results.
     *
     * @return whether the call was succesfull or not.
     */
    public static boolean getUriPermission(Context context,
            ContentProviderClient storageClient, StorageVolume storageVolume,
            String directoryName, int userId, boolean logMetrics,
            GetUriPermissionCallback callback) {
        if (DEBUG) {
            Log.d(TAG, "getUriPermission() for volume " + storageVolume.dump() + ", directory "
                    + directoryName + ", and user " + userId);
        }
        final boolean isRoot = directoryName.equals(DIRECTORY_ROOT);
        final boolean isPrimary = storageVolume.isPrimary();

        if (isRoot && isPrimary) {
            if (DEBUG) Log.d(TAG, "root access requested on primary volume");
            return false;
        }

        final File volumeRoot = storageVolume.getPathFile();
        File file;
        try {
            file = isRoot ? volumeRoot : new File(volumeRoot, directoryName).getCanonicalFile();
        } catch (IOException e) {
            Log.e(TAG, "Could not get canonical file for volume " + storageVolume.dump()
                    + " and directory " + directoryName);
            if (logMetrics) logInvalidScopedAccessRequest(context, SCOPED_DIRECTORY_ACCESS_ERROR);
            return false;
        }
        final StorageManager sm = context.getSystemService(StorageManager.class);

        final String root, directory;
        if (isRoot) {
            root = volumeRoot.getAbsolutePath();
            directory = ".";
        } else {
            root = file.getParent();
            directory = file.getName();
            // Verify directory is valid.
            if (TextUtils.isEmpty(directory) || !isStandardDirectory(directory)) {
                if (DEBUG) {
                    Log.d(TAG, "Directory '" + directory + "' is not standard (full path: '"
                            + file.getAbsolutePath() + "')");
                }
                if (logMetrics) {
                    logInvalidScopedAccessRequest(context,
                            SCOPED_DIRECTORY_ACCESS_INVALID_DIRECTORY);
                }
                return false;
            }
        }

        // Gets volume label and converted path.
        String volumeLabel = null;
        final List<VolumeInfo> volumes = sm.getVolumes();
        if (DEBUG) Log.d(TAG, "Number of volumes: " + volumes.size());
        File internalRoot = null;
        for (VolumeInfo volume : volumes) {
            if (isRightVolume(volume, root, userId)) {
                internalRoot = volume.getInternalPathForUser(userId);
                // Must convert path before calling getDocIdForFileCreateNewDir()
                if (DEBUG) Log.d(TAG, "Converting " + root + " to " + internalRoot);
                file = isRoot ? internalRoot : new File(internalRoot, directory);
                volumeLabel = sm.getBestVolumeDescription(volume);
                if (TextUtils.isEmpty(volumeLabel)) {
                    volumeLabel = storageVolume.getDescription(context);
                }
                if (TextUtils.isEmpty(volumeLabel)) {
                    volumeLabel = context.getString(android.R.string.unknownName);
                    Log.w(TAG, "No volume description  for " + volume + "; using " + volumeLabel);
                }
                break;
            }
        }
        if (internalRoot == null) {
            // Should not happen on normal circumstances, unless app crafted an invalid volume
            // using reflection or the list of mounted volumes changed.
            Log.e(TAG, "Didn't find right volume for '" + storageVolume.dump() + "' on " + volumes);
            return false;
        }

        final Uri requestedUri = getUriPermission(context, storageClient, file);
        final Uri rootUri = internalRoot.equals(file) ? requestedUri
                : getUriPermission(context, storageClient, internalRoot);

        return callback.onResult(file, volumeLabel, isRoot, isPrimary, requestedUri, rootUri);
    }

    /**
     * Creates an URI permission for the given file.
     */
    public static Uri getUriPermission(Context context, ContentProviderClient storageProvider,
            File file) {
        // Calls ExternalStorageProvider to get the doc id for the file
        final Bundle bundle;
        try {
            bundle = storageProvider.call("getDocIdForFileCreateNewDir", file.getPath(), null);
        } catch (RemoteException e) {
            Log.e(TAG, "Did not get doc id from External Storage provider for " + file, e);
            logInvalidScopedAccessRequest(context, SCOPED_DIRECTORY_ACCESS_ERROR);
            return null;
        }
        final String docId = bundle == null ? null : bundle.getString("DOC_ID");
        if (docId == null) {
            Log.e(TAG, "Did not get doc id from External Storage provider for " + file);
            logInvalidScopedAccessRequest(context, SCOPED_DIRECTORY_ACCESS_ERROR);
            return null;
        }
        if (DEBUG) Log.d(TAG, "doc id for " + file + ": " + docId);

        final Uri uri = DocumentsContract.buildTreeDocumentUri(Providers.AUTHORITY_STORAGE, docId);
        if (uri == null) {
            Log.e(TAG, "Could not get URI for doc id " + docId);
            return null;
        }
        if (DEBUG) Log.d(TAG, "URI for " + file + ": " + uri);
        return uri;
    }

    private static boolean isRightVolume(VolumeInfo volume, String root, int userId) {
        final File userPath = volume.getPathForUser(userId);
        final String path = userPath == null ? null : volume.getPathForUser(userId).getPath();
        final boolean isMounted = volume.isMountedReadable();
        if (DEBUG)
            Log.d(TAG, "Volume: " + volume
                    + "\n\tuserId: " + userId
                    + "\n\tuserPath: " + userPath
                    + "\n\troot: " + root
                    + "\n\tpath: " + path
                    + "\n\tisMounted: " + isMounted);

        return isMounted && root.equals(path);
    }

    private SharedMinimal() {
        throw new UnsupportedOperationException("provides static fields only");
    }
}
