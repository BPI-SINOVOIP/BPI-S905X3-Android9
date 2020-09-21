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

package android.webkit.cts;


import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Messenger;
import android.os.RemoteException;

import com.android.internal.annotations.GuardedBy;

import junit.framework.Assert;
import junit.framework.AssertionFailedError;

class TestProcessClient extends Assert implements AutoCloseable, ServiceConnection {
    private Context mContext;

    private static final long CONNECT_TIMEOUT_MS = 5000;

    private Object mLock = new Object();
    @GuardedBy("mLock")
    private Messenger mService;
    @GuardedBy("mLock")
    private Integer mLastResult;
    @GuardedBy("mLock")
    private Throwable mLastException;

    private final Messenger mReplyHandler = new Messenger(new ReplyHandler(Looper.getMainLooper()));

    public static TestProcessClient createProcessA(Context context) throws Throwable {
        return new TestProcessClient(context, TestProcessServiceA.class);
    }

    public static TestProcessClient createProcessB(Context context) throws Throwable {
        return new TestProcessClient(context, TestProcessServiceB.class);
    }

    /**
     * Subclass this to implement test code to run on the service side.
     */
    static abstract class TestRunnable extends Assert {
        public abstract void run(Context ctx);
    }

    static class ProcessFreshChecker extends TestRunnable {
        private static Object sFreshLock = new Object();
        @GuardedBy("sFreshLock")
        private static boolean sFreshProcess = true;

        @Override
        public void run(Context ctx) {
            synchronized (sFreshLock) {
                if (!sFreshProcess) {
                    fail("Service process was unexpectedly reused");
                }
                sFreshProcess = true;
            }
        }

    }

    private TestProcessClient(Context context, Class service) throws Throwable {
        mContext = context;
        Intent i = new Intent(context, service);
        context.bindService(i, this, Context.BIND_AUTO_CREATE);
        synchronized (mLock) {
            if (mService == null) {
                mLock.wait(CONNECT_TIMEOUT_MS);
                if (mService == null) {
                    fail("Timeout waiting for connection");
                }
            }
        }

        // Check that we're using an actual fresh process.
        // 1000ms timeout is plenty since the service is already running.
        run(ProcessFreshChecker.class, 1000);
    }

    public void run(Class runnableClass, long timeoutMs) throws Throwable {
        Message m = Message.obtain(null, TestProcessService.MSG_RUN_TEST);
        m.replyTo = mReplyHandler;
        m.getData().putString(TestProcessService.TEST_CLASS_KEY, runnableClass.getName());
        int result;
        Throwable exception;
        synchronized (mLock) {
            mService.send(m);
            if (mLastResult == null) {
                mLock.wait(timeoutMs);
                if (mLastResult == null) {
                    fail("Timeout waiting for result");
                }
            }
            result = mLastResult;
            mLastResult = null;
            exception = mLastException;
            mLastException = null;
        }
        if (result == TestProcessService.REPLY_EXCEPTION) {
            throw exception;
        } else if (result != TestProcessService.REPLY_OK) {
            fail("Unknown result from service: " + result);
        }
    }

    public void close() {
        synchronized (mLock) {
            if (mService != null) {
                try {
                    mService.send(Message.obtain(null, TestProcessService.MSG_EXIT_PROCESS));
                } catch (RemoteException e) {}
                mService = null;
                mContext.unbindService(this);
            }
        }
    }

    @Override
    public void onServiceConnected(ComponentName className, IBinder service) {
        synchronized (mLock) {
            mService = new Messenger(service);
            mLock.notify();
        }
    }

    @Override
    public void onServiceDisconnected(ComponentName className) {
        synchronized (mLock) {
            mService = null;
            mContext.unbindService(this);
            mLastResult = TestProcessService.REPLY_EXCEPTION;
            mLastException = new AssertionFailedError("Service disconnected unexpectedly");
            mLock.notify();
        }
    }

    private class ReplyHandler extends Handler {
        ReplyHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            synchronized (mLock) {
                mLastResult = msg.what;
                if (msg.what == TestProcessService.REPLY_EXCEPTION) {
                    mLastException = (Throwable) msg.getData().getSerializable(
                            TestProcessService.REPLY_EXCEPTION_KEY);
                }
                mLock.notify();
            }
        }
    }
}
