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
package com.android.tv.dvr;

import android.content.ContentProviderOperation;
import android.content.ContentResolver;
import android.content.Context;
import android.content.OperationApplicationException;
import android.database.Cursor;
import android.media.tv.TvInputInfo;
import android.net.Uri;
import android.os.AsyncTask;
import android.os.RemoteException;
import android.support.media.tv.TvContractCompat;
import android.util.Log;
import android.media.tv.TvContract;
import android.text.TextUtils;
import android.content.ContentValues;
import android.os.StatFs;

import com.android.tv.TvSingletons;
import com.android.tv.common.recording.RecordingStorageStatusManager;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.common.util.SystemProperties;
import com.android.tv.dvr.data.RecordedProgram;
import com.android.tv.data.InternalDataUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Executor;

/** A class for extending TV app-specific function to {@link RecordingStorageStatusManager}. */
public class DvrStorageStatusManager extends RecordingStorageStatusManager {
    private static final String TAG = "DvrStorageStatusManager";
    private static final boolean DEBUG = false || SystemProperties.USE_DEBUG_PVR.getValue();

    private final Context mContext;
    private CleanUpDbTask mCleanUpDbTask;
    private UpdateDbTask mUpdateDbTask;
    private Executor mDbExecutor;

    private static final String[] PROJECTION = {
        TvContractCompat.RecordedPrograms._ID,
        TvContractCompat.RecordedPrograms.COLUMN_PACKAGE_NAME,
        TvContractCompat.RecordedPrograms.COLUMN_RECORDING_DATA_URI,
        TvContractCompat.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA
    };
    private static final int BATCH_OPERATION_COUNT = 100;

    public DvrStorageStatusManager(Context context) {
        super(context);
        mContext = context;
        mDbExecutor = TvSingletons.getSingletons(context).getDbExecutor();
    }

    @Override
    protected void cleanUpDbIfNeeded() {
        if (mCleanUpDbTask != null) {
            mCleanUpDbTask.cancel(true);
        }
        mCleanUpDbTask = new CleanUpDbTask();
        mCleanUpDbTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private class CleanUpDbTask extends AsyncTask<Void, Void, Boolean> {
        private final ContentResolver mContentResolver;

        private CleanUpDbTask() {
            mContentResolver = mContext.getContentResolver();
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            @StorageStatus int storageStatus = getDvrStorageStatus();
            if (storageStatus == STORAGE_STATUS_MISSING) {
                Log.i(TAG, "CleanUpDbTask doInBackground STORAGE_STATUS_MISSING");
                return null;
            }
            if (storageStatus == STORAGE_STATUS_TOTAL_CAPACITY_TOO_SMALL) {
                Log.i(TAG, "CleanUpDbTask doInBackground STORAGE_STATUS_TOTAL_CAPACITY_TOO_SMALL");
                return true;
            }
            List<ContentProviderOperation> ops = getDeleteOps();
            if (ops == null || ops.isEmpty()) {
                Log.i(TAG, "CleanUpDbTask doInBackground ops null");
                return null;
            }
            Log.i(
                    TAG,
                    "New device storage mounted. # of recordings to be forgotten : " + ops.size());
            for (int i = 0; i < ops.size() && !isCancelled(); i += BATCH_OPERATION_COUNT) {
                int toIndex =
                        (i + BATCH_OPERATION_COUNT) > ops.size()
                                ? ops.size()
                                : (i + BATCH_OPERATION_COUNT);
                ArrayList<ContentProviderOperation> batchOps =
                        new ArrayList<>(ops.subList(i, toIndex));
                try {
                    mContext.getContentResolver().applyBatch(TvContractCompat.AUTHORITY, batchOps);
                } catch (RemoteException | OperationApplicationException e) {
                    Log.e(TAG, "Failed to clean up  RecordedPrograms.", e);
                }
            }
            return null;
        }

        @Override
        protected void onPostExecute(Boolean forgetStorage) {
            if (forgetStorage != null && forgetStorage == true) {
                DvrManager dvrManager = TvSingletons.getSingletons(mContext).getDvrManager();
                TvInputManagerHelper tvInputManagerHelper =
                        TvSingletons.getSingletons(mContext).getTvInputManagerHelper();
                List<TvInputInfo> tvInputInfoList =
                        tvInputManagerHelper.getTvInputInfos(true, false);
                if (tvInputInfoList == null || tvInputInfoList.isEmpty()) {
                    return;
                }
                for (TvInputInfo info : tvInputInfoList) {
                    if (CommonUtils.isBundledInput(info.getId())) {
                        dvrManager.forgetStorage(info.getId());
                    }
                }
            }
            if (mCleanUpDbTask == this) {
                mCleanUpDbTask = null;
            }
        }

        private List<ContentProviderOperation> getDeleteOps() {
            List<ContentProviderOperation> ops = new ArrayList<>();

            try (Cursor c =
                    mContentResolver.query(
                            TvContractCompat.RecordedPrograms.CONTENT_URI,
                            PROJECTION,
                            null,
                            null,
                            null)) {
                if (c == null) {
                    return null;
                }
                while (c.moveToNext()) {
                    @StorageStatus int storageStatus = getDvrStorageStatus();
                    if (isCancelled() || storageStatus == STORAGE_STATUS_MISSING) {
                        ops.clear();
                        break;
                    }
                    String id = c.getString(0);
                    String packageName = c.getString(1);
                    String dataUriString = c.getString(2);
                    if (dataUriString == null) {
                        continue;
                    }
                    Uri dataUri = Uri.parse(dataUriString);
                    if (!CommonUtils.isInBundledPackageSet(packageName)
                            || dataUri == null
                            || dataUri.getPath() == null
                            || !ContentResolver.SCHEME_FILE.equals(dataUri.getScheme())) {
                        continue;
                    }
                    File recordedProgramDir = new File(dataUri.getPath());
                    if (!recordedProgramDir.exists()) {
                        ops.add(
                                ContentProviderOperation.newDelete(
                                                TvContractCompat.buildRecordedProgramUri(
                                                        Long.parseLong(id)))
                                        .build());
                    }
                }
                return ops;
            }
        }
    }

    protected void updateDbIfNeeded(boolean needDisplay, String path) {
        if (mUpdateDbTask != null) {
            mUpdateDbTask.cancel(true);
        }
        mUpdateDbTask = new UpdateDbTask(needDisplay, path);
        mUpdateDbTask.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private class UpdateDbTask extends AsyncTask<Void, Void, Boolean> {
        private ContentResolver mContentResolver;
        private boolean mNeedDisplay = true;
        private String mStoragePath = null;

        public UpdateDbTask(boolean needDisplay, String path) {
            this.mContentResolver = mContext.getContentResolver();
            this.mNeedDisplay = needDisplay;
            this.mStoragePath = path;
            Log.d(TAG, "UpdateDbTask needDisplay = " + needDisplay + " ,path = " + path);
        }

        @Override
        protected Boolean doInBackground(Void... params) {
            boolean result = false;
            List<ContentProviderOperation> ops = getUpdateOps(mNeedDisplay, mStoragePath);
            int size = ops.size();
            int count = 0;
            for (int i = 0; i < ops.size() && !isCancelled(); i += BATCH_OPERATION_COUNT) {
                int toIndex =
                        (i + BATCH_OPERATION_COUNT) > ops.size()
                                ? ops.size()
                                : (i + BATCH_OPERATION_COUNT);
                ArrayList<ContentProviderOperation> batchOps =
                        new ArrayList<>(ops.subList(i, toIndex));
                try {
                    mContext.getContentResolver().applyBatch(TvContractCompat.AUTHORITY, batchOps);
                    count++;
                } catch (RemoteException | OperationApplicationException e) {
                    Log.e(TAG, "Failed to UpdateDbTask Exception = " + e.getMessage());
                }
            }
            result = (size == count);
            return result;
        }

        @Override
        protected void onPostExecute(Boolean success) {
            if (success != null && success == true) {
                DvrManager dvrManager = TvSingletons.getSingletons(mContext).getDvrManager();
                if (dvrManager != null) {
                    dvrManager.updateDisplayforAddedOrRemovedStorage();
                } else {
                    Log.d(TAG, "onPostExecute dvrManager null");
                }
            }
            if (mUpdateDbTask == this) {
                mUpdateDbTask = null;
            }
        }

        private List<ContentProviderOperation> getUpdateOps(boolean needDisplay, String path) {
            List<ContentProviderOperation> ops = new ArrayList<>();

            try {
                Cursor c =
                    mContentResolver.query(
                            TvContractCompat.RecordedPrograms.CONTENT_URI,
                            RecordedProgram.PROJECTION,
                            null,
                            null,
                            null);
                if (c == null) {
                    Log.d(TAG, "getUpdateOps null Cursor");
                    return null;
                }
                RecordedProgram recordProgram = null;
                byte[] rawData = null;
                ContentValues values = null;
                boolean isNeedDisplay;
                while (c.moveToNext()) {
                    recordProgram = RecordedProgram.fromCursor(c);
                    InternalDataUtils.InternalProviderData providerData = recordProgram.getFormatInternalProviderData();
                    if (providerData == null || !TextUtils.equals(path, recordProgram.getRecordFilePath())) {
                        if (DEBUG) {
                            Log.d(TAG, "getUpdateOps diffrent record path");
                        }
                        continue;
                    }
                    isNeedDisplay = recordProgram.isRecordStorageExist();
                    if (DEBUG) {
                        Log.d(TAG, "getUpdateOps needDisplay " + needDisplay + ", path = " + path + ", saveExist = " + isNeedDisplay);
                    }
                    values = new ContentValues();
                    if (needDisplay != isNeedDisplay) {
                        providerData.put(RecordedProgram.RECORD_FILE_PATH, path);
                        providerData.put(RecordedProgram.RECORD_STORAGE_EXIST, needDisplay ? 1 : 0);
                        rawData = providerData.getByte();
                        if (rawData != null) {
                            values.put(TvContractCompat.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA, rawData);
                        } else {
                            values.putNull(TvContractCompat.RecordedPrograms.COLUMN_INTERNAL_PROVIDER_DATA);
                        }
                        ops.add(
                                ContentProviderOperation.newUpdate(
                                        TvContractCompat.buildRecordedProgramUri(recordProgram.getId()))
                                        .withValues(values)
                                        .build());
                    } else {
                        if (DEBUG) {
                            Log.d(TAG, "getUpdateOps same storage status");
                        }
                    }
                }
            } catch (Exception e) {
                Log.e(TAG, "Failed to getUpdateOps Exception = " + e.getMessage());
                e.printStackTrace();
            }
            return ops;
        }
    }
}
