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

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.telephony.MbmsDownloadSession;
import android.telephony.cts.embmstestapp.CtsDownloadService;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.FileInfo;
import android.telephony.mbms.MbmsDownloadReceiver;

import java.io.File;
import java.io.InputStream;
import java.util.Collections;
import java.util.List;

public class MbmsDownloadFlowTest extends MbmsDownloadTestBase {
    private File tempFileRootDir;

    @Override
    public void setUp() throws Exception {
        super.setUp();
        tempFileRootDir = new File(mContext.getFilesDir(), "CtsTestDir");
        tempFileRootDir.mkdir();
        mDownloadSession.setTempFileRootDirectory(tempFileRootDir);
    }

    @Override
    public void tearDown() throws Exception {
        recursiveDelete(tempFileRootDir);
        tempFileRootDir = null;
        super.tearDown();
    }

    public void testSingleFileDownloadFlow() throws Exception {
        MbmsDownloadReceiverTest.AppIntentCapture captor =
                new MbmsDownloadReceiverTest.AppIntentCapture(mContext, mHandler);
        DownloadRequest request = downloadRequestTemplate
                .setAppIntent(new Intent(MbmsDownloadReceiverTest.APP_INTENT_ACTION))
                .build();
        mDownloadSession.download(request);
        mMiddlewareControl.actuallyStartDownloadFlow();

        Uri fileUri = checkReceivedDownloadCompleteIntent(captor.getIntent(), request,
                CtsDownloadService.FILE_INFO_1);
        checkFileContentIntegrity(CtsDownloadService.FILE_INFO_1, fileUri);
        checkDownloadResultAck(1);
    }

    public void testFileInSubdirectoryDownloadFlow() throws Exception {
        MbmsDownloadReceiverTest.AppIntentCapture captor =
                new MbmsDownloadReceiverTest.AppIntentCapture(mContext, mHandler);
        DownloadRequest request = new DownloadRequest.Builder(
                CtsDownloadService.SOURCE_URI_2, destinationDirectoryUri)
                .setServiceInfo(CtsDownloadService.FILE_SERVICE_INFO)
                .setAppIntent(new Intent(MbmsDownloadReceiverTest.APP_INTENT_ACTION))
                .build();

        mDownloadSession.download(request);
        mMiddlewareControl.actuallyStartDownloadFlow();

        Uri fileUri = checkReceivedDownloadCompleteIntent(captor.getIntent(), request,
                CtsDownloadService.FILE_INFO_2);
        // Make sure that the received file is placed in the proper subdirectory.
        String file2RelativePath = CtsDownloadService.FILE_INFO_2.getUri().getPath().substring(
                CtsDownloadService.SOURCE_URI_2.getPath().length());
        assertTrue("got path: " + fileUri.getPath() + ", should end with: " + file2RelativePath,
                fileUri.getPath().endsWith(file2RelativePath));
        checkFileContentIntegrity(CtsDownloadService.FILE_INFO_2, fileUri);
        checkDownloadResultAck(1);
    }

    public void testMultiFileDownloadFlow() throws Exception {
        MbmsDownloadReceiverTest.AppIntentCapture captor =
                new MbmsDownloadReceiverTest.AppIntentCapture(mContext, mHandler);
        DownloadRequest request = new DownloadRequest.Builder(
                CtsDownloadService.SOURCE_URI_3, destinationDirectoryUri)
                .setServiceInfo(CtsDownloadService.FILE_SERVICE_INFO)
                .setAppIntent(new Intent(MbmsDownloadReceiverTest.APP_INTENT_ACTION))
                .build();

        mDownloadSession.download(request);
        mMiddlewareControl.actuallyStartDownloadFlow();

        for (Intent i : captor.getIntents(2)) {
            FileInfo fileInfo = i.getParcelableExtra(MbmsDownloadSession.EXTRA_MBMS_FILE_INFO);
            Uri fileUri = null;
            if (CtsDownloadService.FILE_INFO_1.equals(fileInfo)) {
                fileUri = checkReceivedDownloadCompleteIntent(
                        i, request, CtsDownloadService.FILE_INFO_1);
            } else if (CtsDownloadService.FILE_INFO_2.equals(fileInfo)) {
                fileUri = checkReceivedDownloadCompleteIntent(
                        i, request, CtsDownloadService.FILE_INFO_2);
            } else {
                fail("Got unknown file info: " + fileInfo);
            }
            String relativePath = fileInfo.getUri().getPath().substring(
                    CtsDownloadService.SOURCE_URI_3.getPath().length());
            assertTrue("got path: " + fileUri.getPath() + ", should end with: " + relativePath,
                    fileUri.getPath().endsWith(relativePath));
            checkFileContentIntegrity(fileInfo, fileUri);
        }
        checkDownloadResultAck(2);
    }


    private Uri checkReceivedDownloadCompleteIntent(Intent downloadDoneIntent,
            DownloadRequest downloadRequest, FileInfo expectedFileInfo) {
        assertEquals(MbmsDownloadReceiverTest.APP_INTENT_ACTION, downloadDoneIntent.getAction());
        assertEquals(MbmsDownloadSession.RESULT_SUCCESSFUL,
                downloadDoneIntent.getIntExtra(MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_RESULT, -1));
        assertEquals(downloadRequest,
                downloadDoneIntent.getParcelableExtra(
                        MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST));
        assertEquals(downloadRequest.getSubscriptionId(),
                ((DownloadRequest) downloadDoneIntent.getParcelableExtra(
                        MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST)).getSubscriptionId());
        assertEquals(downloadRequest.getDestinationUri(),
                ((DownloadRequest) downloadDoneIntent.getParcelableExtra(
                        MbmsDownloadSession.EXTRA_MBMS_DOWNLOAD_REQUEST)).getDestinationUri());
        assertEquals(expectedFileInfo,
                downloadDoneIntent.getParcelableExtra(MbmsDownloadSession.EXTRA_MBMS_FILE_INFO));
        return downloadDoneIntent.getParcelableExtra(
                MbmsDownloadSession.EXTRA_MBMS_COMPLETED_FILE_URI);
    }

    private void checkFileContentIntegrity(FileInfo fileInfo, Uri completedFileUri)
            throws Exception {
        assertEquals(fileInfo.getUri().getLastPathSegment(),
                completedFileUri.getLastPathSegment());
        InputStream is = mContext.getContentResolver().openInputStream(completedFileUri);
        byte[] contents = new byte[CtsDownloadService.SAMPLE_FILE_DATA.length];
        is.read(contents);
        for (int i = 0; i < contents.length; i++) {
            assertEquals(contents[i], CtsDownloadService.SAMPLE_FILE_DATA[i]);
        }
    }

    private void checkDownloadResultAck(int numAcks) throws Exception {
        // Poll until we get to the right number.
        List<Bundle> downloadResultAck = Collections.emptyList();
        long currentTime = System.currentTimeMillis();
        while (System.currentTimeMillis() < currentTime + ASYNC_TIMEOUT) {
            downloadResultAck = getMiddlewareCalls(CtsDownloadService.METHOD_DOWNLOAD_RESULT_ACK);
            if (numAcks == downloadResultAck.size()) {
                break;
            }
            Thread.sleep(200);
        }
        assertEquals(numAcks, downloadResultAck.size());
        downloadResultAck.forEach((ack) -> assertEquals(MbmsDownloadReceiver.RESULT_OK,
                ack.getInt(CtsDownloadService.ARGUMENT_RESULT_CODE, -1)));
    }
}
