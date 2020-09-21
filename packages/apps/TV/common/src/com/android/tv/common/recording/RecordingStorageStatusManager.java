/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.tv.common.recording;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Environment;
import android.os.Looper;
import android.os.StatFs;
import android.support.annotation.AnyThread;
import android.support.annotation.IntDef;
import android.support.annotation.WorkerThread;
import android.util.Log;
import android.text.TextUtils;
import android.net.Uri;

import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.util.SystemProperties;

import java.io.File;
import java.io.IOException;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.CopyOnWriteArraySet;

import java.lang.reflect.Method;

/** Signals DVR storage status change such as plugging/unplugging. */
public class RecordingStorageStatusManager {
    private static final String TAG = "RecordingStorageStatusManager";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_PVR.getValue();

    /** Minimum storage size to support DVR */
    public static final long MIN_STORAGE_SIZE_FOR_DVR_IN_BYTES = 50 * 1024 * 1024 * 1024L; // 50GB

    private static final long MIN_FREE_STORAGE_SIZE_FOR_DVR_IN_BYTES =
            10 * 1024 * 1024 * 1024L; // 10GB
    private static final String RECORDING_DATA_SUB_PATH = "/recording";

    /** Storage status constants. */
    @IntDef({
        STORAGE_STATUS_OK,
        STORAGE_STATUS_TOTAL_CAPACITY_TOO_SMALL,
        STORAGE_STATUS_FREE_SPACE_INSUFFICIENT,
        STORAGE_STATUS_MISSING
    })
    @Retention(RetentionPolicy.SOURCE)
    public @interface StorageStatus {}

    /** Current storage is OK to record a program. */
    public static final int STORAGE_STATUS_OK = 0;

    /** Current storage's total capacity is smaller than DVR requirement. */
    public static final int STORAGE_STATUS_TOTAL_CAPACITY_TOO_SMALL = 1;

    /** Current storage's free space is insufficient to record programs. */
    public static final int STORAGE_STATUS_FREE_SPACE_INSUFFICIENT = 2;

    /** Current storage is missing. */
    public static final int STORAGE_STATUS_MISSING = 3;

    private final Context mContext;
    private final Set<OnStorageMountChangedListener> mOnStorageMountChangedListeners =
            new CopyOnWriteArraySet<>();
    private MountedStorageStatus mMountedStorageStatus;
    private boolean mStorageValid;

    private class MountedStorageStatus {
        private final boolean mStorageMounted;
        private final File mStorageMountedDir;
        private final long mStorageMountedCapacity;

        private MountedStorageStatus(boolean mounted, File mountedDir, long capacity) {
            mStorageMounted = mounted;
            mStorageMountedDir = mountedDir;
            mStorageMountedCapacity = capacity;
        }

        private boolean isValidForDvr() {
            return mStorageMounted && mStorageMountedCapacity >= MIN_STORAGE_SIZE_FOR_DVR_IN_BYTES;
        }

        @Override
        public boolean equals(Object other) {
            if (!(other instanceof MountedStorageStatus)) {
                return false;
            }
            MountedStorageStatus status = (MountedStorageStatus) other;
            return mStorageMounted == status.mStorageMounted
                    && Objects.equals(mStorageMountedDir, status.mStorageMountedDir)
                    && mStorageMountedCapacity == status.mStorageMountedCapacity;
        }
    }

    public interface OnStorageMountChangedListener {

        /**
         * Listener for DVR storage status change.
         *
         * @param storageMounted {@code true} when DVR possible storage is mounted, {@code false}
         *     otherwise.
         */
        void onStorageMountChanged(boolean storageMounted, String path);
    }

    private final class StorageStatusBroadcastReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "StorageStatusBroadcastReceiver intent = " + intent);
            String path = null;
            boolean isMounted = true;
            String action = null;
            if (intent != null) {
                Uri data = intent.getData();
                action = intent.getAction();
                if (data != null) {
                    path = data.getPath();
                    Log.d(TAG, "StorageStatusBroadcastReceiver path = " + path);
                }
            }
            MountedStorageStatus result = getStorageStatusInternal();
            boolean isStoragePath = isStoragePath(path);
            if (mMountedStorageStatus.equals(result) && !isStoragePath) {
                return;
            }
            mMountedStorageStatus = result;
            if (result.mStorageMounted && !isStoragePath) {
                cleanUpDbIfNeeded();
            } else if (isStoragePath) {
                if (Intent.ACTION_MEDIA_MOUNTED.equals(intent.getAction())) {
                    isMounted = true;
                    updateDbIfNeeded(isMounted, path);
                } else if (Intent.ACTION_MEDIA_EJECT.equals(intent.getAction())) {
                    isMounted = false;
                    updateDbIfNeeded(isMounted, path);
                }
            }
            boolean valid = result.isValidForDvr();
            if (valid == mStorageValid && !isStoragePath) {
                return;
            }
            mStorageValid = valid;
            for (OnStorageMountChangedListener l : mOnStorageMountChangedListeners) {
                if (!isStoragePath) {
                    l.onStorageMountChanged(valid, path);
                } else {
                    if (Intent.ACTION_MEDIA_MOUNTED.equals(intent.getAction())
                            || Intent.ACTION_MEDIA_EJECT.equals(intent.getAction())) {
                        l.onStorageMountChanged(isMounted, path);
                    }
                }
            }
        }
    }

    /**
     * Creates RecordingStorageStatusManager.
     *
     * @param context {@link Context}
     */
    public RecordingStorageStatusManager(final Context context) {
        mContext = context;
        mMountedStorageStatus = getStorageStatusInternal();
        mStorageValid = mMountedStorageStatus.isValidForDvr();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_MEDIA_MOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        filter.addAction(Intent.ACTION_MEDIA_EJECT);
        filter.addAction(Intent.ACTION_MEDIA_REMOVED);
        filter.addAction(Intent.ACTION_MEDIA_BAD_REMOVAL);
        filter.addDataScheme(ContentResolver.SCHEME_FILE);
        mContext.registerReceiver(new StorageStatusBroadcastReceiver(), filter);
    }

    /**
     * Adds the listener for receiving storage status change.
     *
     * @param listener
     */
    public void addListener(OnStorageMountChangedListener listener) {
        mOnStorageMountChangedListeners.add(listener);
    }

    /** Removes the current listener. */
    public void removeListener(OnStorageMountChangedListener listener) {
        mOnStorageMountChangedListeners.remove(listener);
    }

    /** Returns true if a storage is mounted. */
    public boolean isStorageMounted() {
        return mMountedStorageStatus.mStorageMounted;
    }

    /** Returns the path to DVR recording data directory. This can take for a while sometimes. */
    @WorkerThread
    public File getRecordingRootDataDirectory() {
        SoftPreconditions.checkState(Looper.myLooper() != Looper.getMainLooper());
        if (mMountedStorageStatus.mStorageMountedDir == null) {
            return null;
        }
        File root = mContext.getExternalFilesDir(null);
        String rootPath;
        try {
            rootPath = root != null ? root.getCanonicalPath() : null;
        } catch (IOException | SecurityException e) {
            return null;
        }
        return rootPath == null ? null : new File(rootPath + RECORDING_DATA_SUB_PATH);
    }

    /**
     * Returns the current storage status for DVR recordings.
     *
     * @return {@link StorageStatus}
     */
    @AnyThread
    public @StorageStatus int getDvrStorageStatus() {
        MountedStorageStatus status = mMountedStorageStatus;
        if (status.mStorageMountedDir == null) {
            return STORAGE_STATUS_MISSING;
        }
        if (CommonFeatures.FORCE_RECORDING_UNTIL_NO_SPACE.isEnabled(mContext)) {
            return STORAGE_STATUS_OK;
        }
        if (status.mStorageMountedCapacity < MIN_STORAGE_SIZE_FOR_DVR_IN_BYTES) {
            return STORAGE_STATUS_TOTAL_CAPACITY_TOO_SMALL;
        }
        try {
            StatFs statFs = new StatFs(status.mStorageMountedDir.toString());
            if (statFs.getAvailableBytes() < MIN_FREE_STORAGE_SIZE_FOR_DVR_IN_BYTES) {
                return STORAGE_STATUS_FREE_SPACE_INSUFFICIENT;
            }
        } catch (IllegalArgumentException e) {
            // In rare cases, storage status change was not notified yet.
            SoftPreconditions.checkState(false);
            return STORAGE_STATUS_FREE_SPACE_INSUFFICIENT;
        }
        return STORAGE_STATUS_OK;
    }

    /**
     * Returns whether the storage has sufficient storage.
     *
     * @return {@code true} when there is sufficient storage, {@code false} otherwise
     */
    public boolean isStorageSufficient() {
        return getDvrStorageStatus() == STORAGE_STATUS_OK;
    }

    /** APPs that want to clean up DB for recordings should override this method to do the job. */
    protected void cleanUpDbIfNeeded() {}

    /** APPs that want to update DB for recordings should override this method to do the job. */
    protected void updateDbIfNeeded(boolean needDisplay, String path) {}

    private MountedStorageStatus getStorageStatusInternal() {
        boolean storageMounted =
                Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
        File storageMountedDir = storageMounted ? Environment.getExternalStorageDirectory() : null;
        storageMounted = storageMounted && storageMountedDir != null;
        long storageMountedCapacity = 0L;
        if (storageMounted) {
            try {
                StatFs statFs = new StatFs(storageMountedDir.toString());
                storageMountedCapacity = statFs.getTotalBytes();
            } catch (IllegalArgumentException e) {
                Log.e(TAG, "Storage mount status was changed.");
                storageMounted = false;
                storageMountedDir = null;
            }
        }
        Log.d(TAG, "getStorageStatusInternal storageMounted = " + storageMounted + ", storageMountedDir = " + storageMountedDir + ", storageMountedCapacity = " + storageMountedCapacity);
        return new MountedStorageStatus(storageMounted, storageMountedDir, storageMountedCapacity);
    }

    private String getStoragePathInternal() {
        String result = null;
        boolean storageMounted =
                Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED);
        File storageMountedDir = storageMounted ? Environment.getExternalStorageDirectory() : null;
        storageMounted = storageMounted && storageMountedDir != null;
        if (storageMounted) {
            result = storageMountedDir.getAbsolutePath();
        }
        Log.d(TAG, "getStorageStatusInternal path = " + result);
        return result;
    }

    //judge udisk or other disk
    private boolean isStoragePath(String path) {
        boolean result = false;
        String inetrnalPath = getStoragePathInternal();
        if (!TextUtils.isEmpty(path) && !TextUtils.equals(inetrnalPath, path)) {
            result = true;
        }
        return result;
    }

    private boolean isPathAvailable(String path) {
        boolean result = false;
        if (TextUtils.isEmpty(path)) {
            return result;
        }
        String volumeState = Environment.getExternalStorageState(new File(path));
        if (TextUtils.equals(volumeState, Environment.MEDIA_MOUNTED)) {
            result = true;
        }
        Log.d(TAG, "isPathAvailable path = " + path + ", volumeState = " + volumeState + ", result = " + result);
        return result;
    }

    public String getCurrentProcessName() {
        String result = null;
        try {
            Class<?> activityThreadClass = Class.forName("android.app.ActivityThread");
            Method method = activityThreadClass.getDeclaredMethod("currentProcessName", null);
            Object processName = method.invoke(null);
            result = processName.toString();
        } catch (Exception e) {
            e.printStackTrace();
            Log.i(TAG, "getCurrentProcessName Exception = " + e.getMessage());
        }
        Log.i(TAG, "getCurrentProcessName result = " + result);
        return result;
    }
}
