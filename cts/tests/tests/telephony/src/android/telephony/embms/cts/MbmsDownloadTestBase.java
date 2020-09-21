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

import android.annotation.Nullable;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.RemoteException;
import android.telephony.MbmsDownloadSession;
import android.telephony.cts.embmstestapp.CtsDownloadService;
import android.telephony.cts.embmstestapp.ICtsDownloadMiddlewareControl;
import android.telephony.mbms.DownloadRequest;
import android.telephony.mbms.FileServiceInfo;
import android.telephony.mbms.MbmsDownloadSessionCallback;
import android.test.InstrumentationTestCase;
import android.util.Log;

import com.android.internal.os.SomeArgs;

import java.io.File;
import java.util.List;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.Executor;
import java.util.concurrent.LinkedBlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.stream.Collectors;

public class MbmsDownloadTestBase extends InstrumentationTestCase {
    protected static final int ASYNC_TIMEOUT = 10000;

    protected static class TestCallback extends MbmsDownloadSessionCallback {
        private final BlockingQueue<SomeArgs> mErrorCalls = new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mFileServicesUpdatedCalls =
                new LinkedBlockingQueue<>();
        private final BlockingQueue<SomeArgs> mMiddlewareReadyCalls = new LinkedBlockingQueue<>();
        private int mNumErrorCalls = 0;

        @Override
        public void onError(int errorCode, @Nullable String message) {
            mNumErrorCalls += 1;
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = errorCode;
            args.arg2 = message;
            mErrorCalls.add(args);
            Log.i(MbmsDownloadTestBase.class.getSimpleName(),
                    "Got error: " + errorCode + ": " + message);
        }

        @Override
        public void onFileServicesUpdated(List<FileServiceInfo> services) {
            SomeArgs args = SomeArgs.obtain();
            args.arg1 = services;
            mFileServicesUpdatedCalls.add(args);
        }

        @Override
        public void onMiddlewareReady() {
            mMiddlewareReadyCalls.add(SomeArgs.obtain());
        }

        public SomeArgs waitOnError() {
            try {
                return mErrorCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public boolean waitOnMiddlewareReady() {
            try {
                return mMiddlewareReadyCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS) != null;
            } catch (InterruptedException e) {
                return false;
            }
        }

        public SomeArgs waitOnFileServicesUpdated() {
            try {
                return mFileServicesUpdatedCalls.poll(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                return null;
            }
        }

        public int getNumErrorCalls() {
            return mNumErrorCalls;
        }
    }

    DownloadRequest.Builder downloadRequestTemplate;
    Uri destinationDirectoryUri;

    Context mContext;
    HandlerThread mHandlerThread;
    Handler mHandler;
    Executor mCallbackExecutor;
    ICtsDownloadMiddlewareControl mMiddlewareControl;
    MbmsDownloadSession mDownloadSession;
    TestCallback mCallback = new TestCallback();

    @Override
    public void setUp() throws Exception {
        mContext = getInstrumentation().getContext();
        mHandlerThread = new HandlerThread("EmbmsCtsTestWorker");
        mHandlerThread.start();
        mHandler = new Handler(mHandlerThread.getLooper());
        mCallbackExecutor = mHandler::post;
        mCallback = new TestCallback();

        File destinationDirectory = new File(mContext.getFilesDir(), "downloads");
        destinationDirectory.mkdirs();
        destinationDirectoryUri = Uri.fromFile(destinationDirectory);
        downloadRequestTemplate = new DownloadRequest.Builder(
                CtsDownloadService.SOURCE_URI_1, destinationDirectoryUri)
                .setServiceInfo(CtsDownloadService.FILE_SERVICE_INFO);
        getControlBinder();
        setupDownloadSession();
    }

    @Override
    public void tearDown() throws Exception {
        mHandlerThread.quit();
        mDownloadSession.close();
        mMiddlewareControl.reset();
    }

    private void setupDownloadSession() throws Exception {
        mDownloadSession = MbmsDownloadSession.create(
                mContext, mCallbackExecutor, mCallback);
        assertNotNull(mDownloadSession);
        assertTrue(mCallback.waitOnMiddlewareReady());
        assertEquals(0, mCallback.getNumErrorCalls());
        Bundle initializeCall =  mMiddlewareControl.getDownloadSessionCalls().get(0);
        assertEquals(CtsDownloadService.METHOD_INITIALIZE,
                initializeCall.getString(CtsDownloadService.METHOD_NAME));
    }

    private void getControlBinder() throws InterruptedException {
        Intent bindIntent = new Intent(CtsDownloadService.CONTROL_INTERFACE_ACTION);
        bindIntent.setComponent(CtsDownloadService.CONTROL_INTERFACE_COMPONENT);
        final CountDownLatch bindLatch = new CountDownLatch(1);

        boolean success = mContext.bindService(bindIntent, new ServiceConnection() {
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                mMiddlewareControl = ICtsDownloadMiddlewareControl.Stub.asInterface(service);
                bindLatch.countDown();
            }

            @Override
            public void onServiceDisconnected(ComponentName name) {
                mMiddlewareControl = null;
            }
        }, Context.BIND_AUTO_CREATE);
        if (!success) {
            fail("Failed to get control interface -- bind error");
        }
        bindLatch.await(ASYNC_TIMEOUT, TimeUnit.MILLISECONDS);
    }

    protected List<Bundle> getMiddlewareCalls(String methodName) throws RemoteException {
        return (mMiddlewareControl.getDownloadSessionCalls()).stream()
                .filter((elem) -> elem.getString(CtsDownloadService.METHOD_NAME).equals(methodName))
                .collect(Collectors.toList());
    }

    protected static void recursiveDelete(File f) {
        if (f.isDirectory()) {
            for (File f1 : f.listFiles()) {
                recursiveDelete(f1);
            }
        }
        f.delete();
    }
}