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

package android.telephony.embms.cts;

import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.telephony.MbmsDownloadSession;
import android.telephony.cts.embmstestapp.CtsDownloadService;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.MbmsDownloadReceiver;
import android.telephony.mbms.UriPathPair;
import android.telephony.mbms.vendor.VendorUtils;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class MbmsDownloadReceiverTest extends MbmsDownloadTestBase {
    private static final String CTS_BROADCAST_PERMISSION =
            "android.telephony.embms.cts.permission.TEST_BROADCAST";
    private static final String TEST_SERVICE_ID = "service_id";

    public static final String APP_INTENT_ACTION =
            "android.telephony.embms.cts.ACTION_TEST_DOWNLOAD_COMPLETE";

    public static class AppIntentCapture {
        private final BlockingQueue<Intent> mReceivedIntent = new LinkedBlockingQueue<>();
        private final BroadcastReceiver mAppIntentReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                mReceivedIntent.add(intent);
            }
        };
        private Context mContext;

        public AppIntentCapture(Context context, Handler handler) {
            mContext = context;
            IntentFilter filter = new IntentFilter(APP_INTENT_ACTION);
            mContext.registerReceiver(mAppIntentReceiver, filter, null, handler);
        }

        public Intent getIntent() {
            return getIntent(true);
        }

        public Intent getIntent(boolean unregister) {
            try {
                Intent result = mReceivedIntent.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
                if (unregister) {
                    mContext.unregisterReceiver(mAppIntentReceiver);
                }
                return result;
            } catch (InterruptedException e) {
                fail("test was interrupted");
                return null;
            }
        }

        public List<Intent> getIntents(int numExpected) {
            ArrayList<Intent> result = new ArrayList<>(numExpected);
            for (int i = 0; i < numExpected; i++) {
                result.add(getIntent(false));
            }
            mContext.unregisterReceiver(mAppIntentReceiver);
            return result;
        }
    }

    private MbmsDownloadReceiver mReceiver;
    private File tempFileRootDir;
    private String tempFileRootDirPath;
    private DownloadRequest testDownloadRequest;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        testDownloadRequest = downloadRequestTemplate
                .setAppIntent(new Intent(APP_INTENT_ACTION))
                .build();
        mReceiver = new MbmsDownloadReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(VendorUtils.ACTION_DOWNLOAD_RESULT_INTERNAL);
        filter.addAction(VendorUtils.ACTION_CLEANUP);
        filter.addAction(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        mContext.registerReceiver(mReceiver, filter);
        tempFileRootDir = new File(mContext.getFilesDir(), "CtsTestDir");
        tempFileRootDir.mkdir();
        tempFileRootDirPath = tempFileRootDir.getCanonicalPath();
        try {
            mDownloadSession.setTempFileRootDirectory(tempFileRootDir);
        } catch (IllegalStateException e) {
            tearDown();
            throw e;
        }
    }

    @Override
    public void tearDown() throws Exception {
        recursiveDelete(tempFileRootDir);
        tempFileRootDir = null;
        super.tearDown();
    }

    public void testMalformedIntents() throws Exception {
        Intent downloadCompleteIntent = new Intent(VendorUtils.ACTION_DOWNLOAD_RESULT_INTERNAL);
        sendBroadcastAndValidate(downloadCompleteIntent,
                MbmsDownloadReceiver.RESULT_MALFORMED_INTENT);

        Intent fdRequestIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        sendBroadcastAndValidate(fdRequestIntent,
                MbmsDownloadReceiver.RESULT_MALFORMED_INTENT);

        Intent cleanupIntent = new Intent(VendorUtils.ACTION_CLEANUP);
        sendBroadcastAndValidate(cleanupIntent,
                MbmsDownloadReceiver.RESULT_MALFORMED_INTENT);
    }

    public void testBadTempFileDirectory() throws Exception {
        Intent cleanupIntent = new Intent(VendorUtils.ACTION_CLEANUP);
        populateIntentWithCommonFields(cleanupIntent);
        cleanupIntent.putParcelableArrayListExtra(VendorUtils.EXTRA_TEMP_FILES_IN_USE,
                new ArrayList<>(0));
        cleanupIntent.putExtra(VendorUtils.EXTRA_TEMP_FILE_ROOT, "this is not a directory path");
        sendBroadcastAndValidate(cleanupIntent,
                MbmsDownloadReceiver.RESULT_BAD_TEMP_FILE_ROOT);
    }

    public void testDownloadFailureIntent() throws Exception {
        Intent intentForReceiverTest = new Intent(VendorUtils.ACTION_DOWNLOAD_RESULT_INTERNAL);
        populateIntentWithCommonFields(intentForReceiverTest);
        intentForReceiverTest.putExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_RESULT,
                MbmsDownloadSession.RESULT_CANCELLED);
        intentForReceiverTest.putExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST,
                testDownloadRequest);

        AppIntentCapture intentCaptor = new AppIntentCapture(mContext, mHandler);

        sendBroadcastAndValidate(intentForReceiverTest, MbmsDownloadReceiver.RESULT_OK);
        Intent receivedIntent = intentCaptor.getIntent();

        assertEquals(MbmsDownloadSession.RESULT_CANCELLED,
                receivedIntent.getIntExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_RESULT, -1));

        assertEquals(testDownloadRequest,
                receivedIntent.getParcelableExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST));
    }

    public void testBadDownloadToken() {
        // Set up a perfectly valid download completion intent, and expect it to fail because the
        // download token hasn't been written.
        Intent intentForReceiverTest = new Intent(VendorUtils.ACTION_DOWNLOAD_RESULT_INTERNAL);
        populateIntentWithCommonFields(intentForReceiverTest);
        intentForReceiverTest.putExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_RESULT,
                MbmsDownloadSession.RESULT_SUCCESSFUL);
        intentForReceiverTest.putExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST,
                testDownloadRequest);
        intentForReceiverTest.putExtra(MbmsDownloadSession.EXTRA_MBMS_FILE_INFO,
                CtsDownloadService.FILE_INFO_1);
        intentForReceiverTest.putExtra(VendorUtils.EXTRA_FINAL_URI,
                Uri.fromFile(new File(new File(tempFileRootDir, TEST_SERVICE_ID), "file1")));

        sendBroadcastAndValidate(intentForReceiverTest,
                MbmsDownloadReceiver.RESULT_MALFORMED_INTENT);
    }

    public void testRequestNoFileDescriptors() throws Exception {
        Intent fdRequestIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        populateIntentWithCommonFields(fdRequestIntent);

        Bundle b = sendBroadcastAndValidate(fdRequestIntent, MbmsDownloadReceiver.RESULT_OK);
        assertTrue(b == null || b.isEmpty());
    }

    public void testRequestNewFileDescriptors() throws Exception {
        Intent fdRequestIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        populateIntentWithCommonFields(fdRequestIntent);
        fdRequestIntent.putExtra(VendorUtils.EXTRA_FD_COUNT, 5);

        Bundle result = sendBroadcastAndValidate(fdRequestIntent, MbmsDownloadReceiver.RESULT_OK);
        List<UriPathPair> freeUris = result.getParcelableArrayList(VendorUtils.EXTRA_FREE_URI_LIST);
        assertNotNull(freeUris);
        assertEquals(5, freeUris.size());
        for (UriPathPair pathPair : freeUris) {
            assertEquals(ContentResolver.SCHEME_CONTENT, pathPair.getContentUri().getScheme());
            assertEquals(ContentResolver.SCHEME_FILE, pathPair.getFilePathUri().getScheme());
        }
    }

    public void testRequestRefreshedFileDescriptors() throws Exception {
        // Set up a few temp files that we can request again
        Intent fdRequestIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        populateIntentWithCommonFields(fdRequestIntent);
        fdRequestIntent.putExtra(VendorUtils.EXTRA_FD_COUNT, 2);

        Bundle result = sendBroadcastAndValidate(fdRequestIntent, MbmsDownloadReceiver.RESULT_OK);
        List<UriPathPair> freeUris = result.getParcelableArrayList(VendorUtils.EXTRA_FREE_URI_LIST);

        Intent fdRefreshIntent = new Intent(VendorUtils.ACTION_FILE_DESCRIPTOR_REQUEST);
        populateIntentWithCommonFields(fdRefreshIntent);
        fdRefreshIntent.putParcelableArrayListExtra(VendorUtils.EXTRA_PAUSED_LIST,
                new ArrayList<>(freeUris.stream().map(UriPathPair::getFilePathUri)
                        .collect(Collectors.toList())));
        Bundle result2 = sendBroadcastAndValidate(fdRefreshIntent, MbmsDownloadReceiver.RESULT_OK);
        List<UriPathPair> refreshUris =
                result2.getParcelableArrayList(VendorUtils.EXTRA_PAUSED_URI_LIST);
        assertEquals(freeUris.size(), refreshUris.size());
        for (UriPathPair pathPair : refreshUris) {
            assertTrue(freeUris.stream()
                    .anyMatch((originalPair) ->
                            originalPair.getFilePathUri().equals(pathPair.getFilePathUri())));
        }
    }

    private Bundle sendBroadcastAndValidate(Intent intent, int expectedCode) {
        BlockingQueue<Bundle> receivedExtras = new LinkedBlockingQueue<>();
        BlockingQueue<Integer> receivedCode = new LinkedBlockingQueue<>();
        mContext.sendOrderedBroadcast(intent, CTS_BROADCAST_PERMISSION,
                new BroadcastReceiver() {
                    @Override
                    public void onReceive(Context context, Intent intent) {
                        receivedExtras.add(getResultExtras(true));
                        receivedCode.add(getResultCode());
                    }
                }, mHandler, -1, null, null);

        try {
            assertEquals(expectedCode,
                    (int) receivedCode.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS));
            return receivedExtras.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            fail("Test interrupted");
            return null;
        }
    }

    private boolean bundleEquals(Bundle a, Bundle b) {
        if (a == null && b == null) {
            return true;
        }
        if (a == null || b == null) {
            return false;
        }
        for (String aKey : a.keySet()) {
            if (!Objects.equals(a.get(aKey), b.get(aKey))) {
                return false;
            }
        }

        for (String bKey : b.keySet()) {
            if (!Objects.equals(b.get(bKey), a.get(bKey))) {
                return false;
            }
        }

        return true;
    }

    private void populateIntentWithCommonFields(Intent intent) {
        intent.putExtra(VendorUtils.EXTRA_SERVICE_ID, TEST_SERVICE_ID);
        intent.putExtra(VendorUtils.EXTRA_TEMP_FILE_ROOT, tempFileRootDirPath);
    }

}
