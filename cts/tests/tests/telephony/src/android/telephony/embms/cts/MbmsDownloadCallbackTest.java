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

import android.telephony.MbmsDownloadSession;
import android.telephony.cts.embmstestapp.CtsDownloadService;
import android.telephony.mbms.DownloadProgressListener;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.DownloadStatusListener;
import android.telephony.mbms.FileInfo;

import java.util.concurrent.BlockingQueue;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;

public class MbmsDownloadCallbackTest extends MbmsDownloadTestBase {
    private static final long SHORT_TIMEOUT = 500;

    private class TestDSCallback extends DownloadStatusListener {
        private final BlockingQueue<SomeArgs> mCalls = new LinkedBlockingQueue<>();

        @Override
        public void onStatusUpdated(DownloadRequest request, FileInfo fileInfo,
                @MbmsDownloadSession.DownloadStatus int state) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = request;
            args.arg2 = fileInfo;
            args.arg3 = state;
            mCalls.add(args);
        }

        public SomeArgs waitOnStatusUpdated(long timeout) {
            try {
                return mCalls.poll(timeout, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }
    }

    private class TestDPCallback extends DownloadProgressListener {
        private final BlockingQueue<SomeArgs> mProgressUpdatedCalls = new LinkedBlockingQueue<>();

        @Override
        public void onProgressUpdated(DownloadRequest request, FileInfo fileInfo, int
                currentDownloadSize, int fullDownloadSize, int currentDecodedSize, int
                fullDecodedSize) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = request;
            args.arg2 = fileInfo;
            args.arg3 = currentDownloadSize;
            args.arg4 = fullDownloadSize;
            args.arg5 = currentDecodedSize;
            args.arg6 = fullDecodedSize;
            mProgressUpdatedCalls.add(args);
        }

        public SomeArgs waitOnProgressUpdated(long timeout) {
            try {
                return mProgressUpdatedCalls.poll(timeout, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }
    }

    public void testFullCallback() throws Exception {
        int sampleInt = 10;
        TestDSCallback statusCallback = new TestDSCallback();
        TestDPCallback progressCallback = new TestDPCallback();
        DownloadRequest request = downloadRequestTemplate.build();
        mDownloadSession.addStatusListener(request, mCallbackExecutor, statusCallback);
        mDownloadSession.addProgressListener(request, mCallbackExecutor, progressCallback);
        mMiddlewareControl.fireOnProgressUpdated(request, CtsDownloadService.FILE_INFO_1,
                sampleInt, sampleInt, sampleInt, sampleInt);
        SomeArgs progressArgs = progressCallback.waitOnProgressUpdated(ASYNC_TIMEOUT);
        assertEquals(request, progressArgs.arg1);
        assertEquals(CtsDownloadService.FILE_INFO_1, progressArgs.arg2);
        assertEquals(sampleInt, progressArgs.arg3);
        assertEquals(sampleInt, progressArgs.arg4);
        assertEquals(sampleInt, progressArgs.arg5);
        assertEquals(sampleInt, progressArgs.arg6);

        mMiddlewareControl.fireOnStateUpdated(request, CtsDownloadService.FILE_INFO_1, sampleInt);
        SomeArgs stateArgs = statusCallback.waitOnStatusUpdated(ASYNC_TIMEOUT);
        assertEquals(request, stateArgs.arg1);
        assertEquals(CtsDownloadService.FILE_INFO_1, stateArgs.arg2);
        assertEquals(sampleInt, stateArgs.arg3);
    }

    public void testDeregistration() throws Exception {
        TestDSCallback statusCallback = new TestDSCallback();
        TestDPCallback progressCallback = new TestDPCallback();
        DownloadRequest request = downloadRequestTemplate.build();
        mDownloadSession.addProgressListener(request, mCallbackExecutor, progressCallback);
        mDownloadSession.addStatusListener(request, mCallbackExecutor, statusCallback);
        mDownloadSession.removeProgressListener(request, progressCallback);
        mDownloadSession.removeStatusListener(request, statusCallback);

        mMiddlewareControl.fireOnStateUpdated(null, null, 0);
        assertNull(statusCallback.waitOnStatusUpdated(SHORT_TIMEOUT));
        mMiddlewareControl.fireOnProgressUpdated(null, null, 0, 0, 0, 0);
        assertNull(progressCallback.waitOnProgressUpdated(SHORT_TIMEOUT));
    }
}
