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
 * limitations under the License
 */

package android.telephony.cts.embmstestapp;

import android.app.Activity;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.net.Uri;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.ParcelFileDescriptor;
import android.os.RemoteException;
import android.telephony.MbmsDownloadSession;
import android.telephony.mbms.DownloadProgressListener;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.DownloadStatusListener;
import android.telephony.mbms.FileInfo;
import android.telephony.mbms.FileServiceInfo;
import android.telephony.mbms.MbmsDownloadSessionCallback;
import android.telephony.mbms.MbmsErrors;
import android.telephony.mbms.UriPathPair;
import android.telephony.mbms.vendor.MbmsDownloadServiceBase;
import android.telephony.mbms.vendor.VendorUtils;
import android.util.Log;

import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

public class CtsDownloadService extends Service {
    private static final Set<String> ALLOWED_PACKAGES = new HashSet<String>() {{
        add("android.telephony.cts");
    }};
    private static final String TAG = "EmbmsTestDownload";

    public static final String METHOD_NAME = "method_name";
    public static final String METHOD_INITIALIZE = "initialize";
    public static final String METHOD_REQUEST_UPDATE_FILE_SERVICES =
            "requestUpdateFileServices";
    public static final String METHOD_SET_TEMP_FILE_ROOT = "setTempFileRootDirectory";
    public static final String METHOD_RESET_DOWNLOAD_KNOWLEDGE = "resetDownloadKnowledge";
    public static final String METHOD_GET_DOWNLOAD_STATUS = "getDownloadStatus";
    public static final String METHOD_CANCEL_DOWNLOAD = "cancelDownload";
    public static final String METHOD_CLOSE = "close";
    // Not a method call, but it's a form of communication to the middleware so it's included
    // here for convenience.
    public static final String METHOD_DOWNLOAD_RESULT_ACK = "downloadResultAck";

    public static final String ARGUMENT_SUBSCRIPTION_ID = "subscriptionId";
    public static final String ARGUMENT_SERVICE_CLASSES = "serviceClasses";
    public static final String ARGUMENT_ROOT_DIRECTORY_PATH = "rootDirectoryPath";
    public static final String ARGUMENT_DOWNLOAD_REQUEST = "downloadRequest";
    public static final String ARGUMENT_FILE_INFO = "fileInfo";
    public static final String ARGUMENT_RESULT_CODE = "resultCode";

    public static final String CONTROL_INTERFACE_ACTION =
            "android.telephony.cts.embmstestapp.ACTION_CONTROL_MIDDLEWARE";
    public static final ComponentName CONTROL_INTERFACE_COMPONENT =
            ComponentName.unflattenFromString(
                    "android.telephony.cts.embmstestapp/.CtsDownloadService");
    public static final ComponentName CTS_TEST_RECEIVER_COMPONENT =
            ComponentName.unflattenFromString(
                    "android.telephony.cts/android.telephony.mbms.MbmsDownloadReceiver");

    public static final Uri DOWNLOAD_SOURCE_URI_ROOT =
            Uri.parse("http://www.example.com/file_download");
    public static final FileServiceInfo FILE_SERVICE_INFO;
    public static final FileInfo FILE_INFO_1 = new FileInfo(
            DOWNLOAD_SOURCE_URI_ROOT.buildUpon().appendPath("file1.txt").build(),
            "text/plain");
    public static final FileInfo FILE_INFO_2 = new FileInfo(
            DOWNLOAD_SOURCE_URI_ROOT.buildUpon().appendPath("sub_dir1")
                    .appendPath("sub_dir2")
                    .appendPath("file2.txt")
                    .build(),
            "text/plain");
    public static final byte[] SAMPLE_FILE_DATA = "this is some sample file data".getBytes();

    // Define allowed source URIs so that we don't have to do the prefix matching calculation
    public static final Uri SOURCE_URI_1 = DOWNLOAD_SOURCE_URI_ROOT.buildUpon()
            .appendPath("file1.txt").build();
    public static final Uri SOURCE_URI_2 = DOWNLOAD_SOURCE_URI_ROOT.buildUpon()
            .appendPath("sub_dir1").appendPath("*").build();
    public static final Uri SOURCE_URI_3 = DOWNLOAD_SOURCE_URI_ROOT.buildUpon()
            .appendPath("*").build();

    static {
        String id = "urn:3GPP:service_0-0";
        Map<Locale, String> localeDict = new HashMap<Locale, String>() {{
            put(Locale.US, "Entertainment Source 1");
            put(Locale.CANADA, "Entertainment Source 1, eh?");
        }};
        List<Locale> locales = new ArrayList<Locale>() {{
            add(Locale.CANADA);
            add(Locale.US);
        }};
        List<FileInfo> files = new ArrayList<FileInfo>() {{
            add(FILE_INFO_1);
            add(FILE_INFO_2);
        }};
        FILE_SERVICE_INFO = new FileServiceInfo(localeDict, "class1", locales,
                id, new Date(2017, 8, 21, 18, 20, 29),
                new Date(2017, 8, 21, 18, 23, 9), files);
    }

    private MbmsDownloadSessionCallback mAppCallback;
    private DownloadStatusListener mDownloadStatusListener;
    private DownloadProgressListener mDownloadProgressListener;

    private HandlerThread mHandlerThread;
    private Handler mHandler;
    private List<Bundle> mReceivedCalls = new LinkedList<>();
    private int mErrorCodeOverride = MbmsErrors.SUCCESS;
    private List<DownloadRequest> mReceivedRequests = new LinkedList<>();
    private String mTempFileRootDirPath = null;

    private final MbmsDownloadServiceBase mDownloadServiceImpl = new MbmsDownloadServiceBase() {
        @Override
        public int initialize(int subscriptionId, MbmsDownloadSessionCallback callback) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_INITIALIZE);
            b.putInt(ARGUMENT_SUBSCRIPTION_ID, subscriptionId);
            mReceivedCalls.add(b);

            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }

            int packageUid = Binder.getCallingUid();
            String[] packageNames = getPackageManager().getPackagesForUid(packageUid);
            if (packageNames == null) {
                return MbmsErrors.InitializationErrors.ERROR_APP_PERMISSIONS_NOT_GRANTED;
            }
            boolean isUidAllowed = Arrays.stream(packageNames).anyMatch(ALLOWED_PACKAGES::contains);
            if (!isUidAllowed) {
                return MbmsErrors.InitializationErrors.ERROR_APP_PERMISSIONS_NOT_GRANTED;
            }

            mHandler.post(() -> {
                if (mAppCallback == null) {
                    mAppCallback = callback;
                } else {
                    callback.onError(
                            MbmsErrors.InitializationErrors.ERROR_DUPLICATE_INITIALIZE, "");
                    return;
                }
                callback.onMiddlewareReady();
            });
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int requestUpdateFileServices(int subscriptionId, List<String> serviceClasses) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_REQUEST_UPDATE_FILE_SERVICES);
            b.putInt(ARGUMENT_SUBSCRIPTION_ID, subscriptionId);
            b.putStringArrayList(ARGUMENT_SERVICE_CLASSES, new ArrayList<>(serviceClasses));
            mReceivedCalls.add(b);

            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }

            List<FileServiceInfo> serviceInfos = Collections.singletonList(FILE_SERVICE_INFO);

            mHandler.post(() -> {
                if (mAppCallback!= null) {
                    mAppCallback.onFileServicesUpdated(serviceInfos);
                }
            });

            return MbmsErrors.SUCCESS;
        }

        @Override
        public int download(DownloadRequest downloadRequest) {
            mReceivedRequests.add(downloadRequest);
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int setTempFileRootDirectory(int subscriptionId, String rootDirectoryPath) {
            if (mErrorCodeOverride != MbmsErrors.SUCCESS) {
                return mErrorCodeOverride;
            }

            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_SET_TEMP_FILE_ROOT);
            b.putInt(ARGUMENT_SUBSCRIPTION_ID, subscriptionId);
            b.putString(ARGUMENT_ROOT_DIRECTORY_PATH, rootDirectoryPath);
            mReceivedCalls.add(b);
            mTempFileRootDirPath = rootDirectoryPath;
            return 0;
        }

        @Override
        public int addProgressListener(DownloadRequest downloadRequest,
                DownloadProgressListener listener) throws RemoteException {
            mDownloadProgressListener = listener;
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int addStatusListener(DownloadRequest downloadRequest,
                DownloadStatusListener listener) throws RemoteException {
            mDownloadStatusListener = listener;
            return MbmsErrors.SUCCESS;
        }

        @Override
        public void dispose(int subscriptionId) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_CLOSE);
            b.putInt(ARGUMENT_SUBSCRIPTION_ID, subscriptionId);
            mReceivedCalls.add(b);
        }

        @Override
        public int requestDownloadState(DownloadRequest downloadRequest, FileInfo fileInfo) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_GET_DOWNLOAD_STATUS);
            b.putParcelable(ARGUMENT_DOWNLOAD_REQUEST, downloadRequest);
            b.putParcelable(ARGUMENT_FILE_INFO, fileInfo);
            mReceivedCalls.add(b);
            return MbmsDownloadSession.STATUS_ACTIVELY_DOWNLOADING;
        }

        @Override
        public int cancelDownload(DownloadRequest request) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_CANCEL_DOWNLOAD);
            b.putParcelable(ARGUMENT_DOWNLOAD_REQUEST, request);
            mReceivedCalls.add(b);
            mReceivedRequests.remove(request);
            return MbmsErrors.SUCCESS;
        }

        @Override
        public List<DownloadRequest> listPendingDownloads(int subscriptionId) {
            return mReceivedRequests;
        }

        @Override
        public int removeStatusListener(DownloadRequest downloadRequest,
                DownloadStatusListener callback) {
            mDownloadStatusListener = null;
            return MbmsErrors.SUCCESS;
        }

        @Override
        public int resetDownloadKnowledge(DownloadRequest downloadRequest) {
            Bundle b = new Bundle();
            b.putString(METHOD_NAME, METHOD_RESET_DOWNLOAD_KNOWLEDGE);
            b.putParcelable(ARGUMENT_DOWNLOAD_REQUEST, downloadRequest);
            mReceivedCalls.add(b);
            return MbmsErrors.SUCCESS;
        }

        @Override
        public void onAppCallbackDied(int uid, int subscriptionId) {
            mAppCallback = null;
        }
    };

    private final IBinder mControlInterface = new ICtsDownloadMiddlewareControl.Stub() {
        @Override
        public void reset() {
            mReceivedCalls.clear();
            mHandler.removeCallbacksAndMessages(null);
            mAppCallback = null;
            mErrorCodeOverride = MbmsErrors.SUCCESS;
            mReceivedRequests.clear();
            mDownloadStatusListener = null;
            mTempFileRootDirPath = null;
        }

        @Override
        public List<Bundle> getDownloadSessionCalls() {
            return mReceivedCalls;
        }

        @Override
        public void forceErrorCode(int error) {
            mErrorCodeOverride = error;
        }

        @Override
        public void fireErrorOnSession(int errorCode, String message) {
            mHandler.post(() -> mAppCallback.onError(errorCode, message));
        }

        @Override
        public void fireOnProgressUpdated(DownloadRequest request, FileInfo fileInfo,
                int currentDownloadSize, int fullDownloadSize,
                int currentDecodedSize, int fullDecodedSize) {
            if (mDownloadStatusListener == null) {
                return;
            }
            mHandler.post(() -> mDownloadProgressListener.onProgressUpdated(request, fileInfo,
                    currentDownloadSize, fullDownloadSize, currentDecodedSize, fullDecodedSize));
        }

        @Override
        public void fireOnStateUpdated(DownloadRequest request, FileInfo fileInfo, int state) {
            if (mDownloadStatusListener == null) {
                return;
            }
            mHandler.post(() -> mDownloadStatusListener.onStatusUpdated(request, fileInfo, state));
        }

        @Override
        public void actuallyStartDownloadFlow() {
            DownloadRequest request = mReceivedRequests.get(0);
            List<FileInfo> requestedFiles = getRequestedFiles(request);
            // Compose the FILE_DESCRIPTOR_REQUEST_INTENT to get some FDs to write to
            Intent requestIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
            requestIntent.putExtra(VendorUtils.EXTRA_SERVICE_ID, request.getFileServiceId());

            requestIntent.putExtra(VendorUtils.EXTRA_FD_COUNT, requestedFiles.size());
            requestIntent.putExtra(VendorUtils.EXTRA_TEMP_FILE_ROOT, mTempFileRootDirPath);
            requestIntent.setComponent(CTS_TEST_RECEIVER_COMPONENT);

            // Send as an ordered broadcast, using a BroadcastReceiver to capture the result
            // containing UriPathPairs.
            logd("Sending fd-request broadcast");
            sendOrderedBroadcast(requestIntent,
                    null, // receiverPermission
                    new BroadcastReceiver() {
                        @Override
                        public void onReceive(Context context, Intent intent) {
                            logd("Got file-descriptors");
                            Bundle extras = getResultExtras(false);
                            List<UriPathPair> tempFiles = extras.getParcelableArrayList(
                                    VendorUtils.EXTRA_FREE_URI_LIST);

                            for (int i = 0; i < tempFiles.size(); i++) {
                                UriPathPair tempFile = tempFiles.get(i);
                                FileInfo requestedFile = requestedFiles.get(i);
                                int result = writeContentsToTempFile(tempFile);

                                Intent downloadResultIntent = composeDownloadResultIntent(
                                        tempFile, request, result, requestedFile);

                                logd("Sending broadcast to app: "
                                        + downloadResultIntent.toString());
                                sendOrderedBroadcast(downloadResultIntent,
                                        null, // receiverPermission
                                        new BroadcastReceiver() {
                                            @Override
                                            public void onReceive(Context context, Intent intent) {
                                                Bundle b = new Bundle();
                                                b.putString(METHOD_NAME,
                                                        METHOD_DOWNLOAD_RESULT_ACK);
                                                b.putInt(ARGUMENT_RESULT_CODE, getResultCode());
                                                mReceivedCalls.add(b);
                                            }
                                        },
                                        null, // scheduler
                                        Activity.RESULT_OK,
                                        null, // initialData
                                        null /* initialExtras */);
                        }
                        }
                    },
                    mHandler, // scheduler
                    Activity.RESULT_OK,
                    null, // initialData
                    null /* initialExtras */);

        }
    };

    private List<FileInfo> getRequestedFiles(DownloadRequest request) {
        if (SOURCE_URI_1.equals(request.getSourceUri())) {
            return Collections.singletonList(FILE_INFO_1);
        }
        if (SOURCE_URI_2.equals(request.getSourceUri())) {
            return Collections.singletonList(FILE_INFO_2);
        }
        if (SOURCE_URI_3.equals(request.getSourceUri())) {
            return FILE_SERVICE_INFO.getFiles();
        }
        return Collections.emptyList();
    }

    private Intent composeDownloadResultIntent(UriPathPair tempFile, DownloadRequest request,
            int result, FileInfo downloadedFile) {
        Intent downloadResultIntent =
                new Intent(VendorUtils.ACTION_DOWNLOAD_RESULT_INTERNAL);
        downloadResultIntent.putExtra(
                MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST, request);
        downloadResultIntent.putExtra(VendorUtils.EXTRA_FINAL_URI,
                tempFile.getFilePathUri());
        downloadResultIntent.putExtra(
                MbmsDownloadSession.EXTRA_MBMS_FILE_INFO, downloadedFile);
        downloadResultIntent.putExtra(VendorUtils.EXTRA_TEMP_FILE_ROOT,
                mTempFileRootDirPath);
        downloadResultIntent.putExtra(
                MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_RESULT, result);
        downloadResultIntent.setComponent(CTS_TEST_RECEIVER_COMPONENT);
        return downloadResultIntent;
    }

    private int writeContentsToTempFile(UriPathPair tempFile) {
        int result = MbmsDownloadSession.RESULT_SUCCESSFUL;
        try {
            ParcelFileDescriptor tempFileFd =
                    getContentResolver().openFileDescriptor(
                            tempFile.getContentUri(), "rw");
            OutputStream destinationStream =
                    new ParcelFileDescriptor.AutoCloseOutputStream(tempFileFd);

            destinationStream.write(SAMPLE_FILE_DATA);
            destinationStream.flush();
        } catch (IOException e) {
            result = MbmsDownloadSession.RESULT_CANCELLED;
        }
        return result;
    }

    @Override
    public void onDestroy() {
        super.onCreate();
        mHandlerThread.quitSafely();
        logd("CtsDownloadService onDestroy");
    }

    @Override
    public IBinder onBind(Intent intent) {
        if (CONTROL_INTERFACE_ACTION.equals(intent.getAction())) {
            logd("CtsDownloadService control interface bind");
            return mControlInterface;
        }

        logd("CtsDownloadService onBind");
        if (mHandlerThread != null && mHandlerThread.isAlive()) {
            return mDownloadServiceImpl;
        }

        mHandlerThread = new HandlerThread("CtsDownloadServiceWorker");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());
        return mDownloadServiceImpl;
    }

    private static void logd(String s) {
        Log.d(TAG, s);
    }

    private void checkInitialized() {
        if (mAppCallback == null) {
            throw new IllegalStateException("Not yet initialized");
        }
    }
}
