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

package android.view.inputmethod.cts.util;

import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.os.Bundle;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ResultReceiver;
import androidx.annotation.NonNull;

import java.util.concurrent.ArrayBlockingQueue;
import java.util.concurrent.BlockingQueue;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.TimeoutException;
import java.util.concurrent.atomic.AtomicBoolean;

/**
 * Helper class to trigger the situation where window in a different process gains focus.
 */
public class WindowFocusStealer implements AutoCloseable {

    private static final class MyResultReceiver extends ResultReceiver {
        final BlockingQueue<Integer> mQueue = new ArrayBlockingQueue<>(1);

        MyResultReceiver() {
            super(null);
        }

        @Override
        protected void onReceiveResult(int resultCode, Bundle resultData) {
            mQueue.add(resultCode);
        }

        public void waitResult(long timeout) throws TimeoutException {
            final Object result;
            try {
                result = mQueue.poll(timeout, TimeUnit.MILLISECONDS);
            } catch (InterruptedException e) {
                throw new IllegalStateException(e);
            }
            if (result == null) {
                throw new TimeoutException();
            }
        }
    }

    @NonNull
    private final IWindowFocusStealer mService;
    @NonNull
    private final Runnable mCloser;

    /**
     * Let a window in a different process gain the window focus.
     *
     * @param parentAppWindowToken Token returned from
     *                             {@link android.view.View#getApplicationWindowToken()}
     * @param timeout              timeout in millisecond
     * @throws TimeoutException when failed to have the window focused within {@code timeout}
     */
    public void stealWindowFocus(IBinder parentAppWindowToken, long timeout)
            throws TimeoutException {
        final MyResultReceiver resultReceiver = new MyResultReceiver();
        try {
            mService.stealWindowFocus(parentAppWindowToken, resultReceiver);
        } catch (RemoteException e) {
            throw new IllegalStateException(e);
        }
        resultReceiver.waitResult(timeout);
    }

    private WindowFocusStealer(@NonNull IWindowFocusStealer service, @NonNull Runnable closer) {
        mService = service;
        mCloser = closer;
    }

    /**
     * Establishes a connection to the service.
     *
     * @param context {@link Context} to which {@link WindowFocusStealerService} belongs
     * @param timeout timeout in millisecond
     * @throws TimeoutException when failed to establish the connection within {@code timeout}
     */
    public static WindowFocusStealer connect(Context context, long timeout)
            throws TimeoutException {
        final BlockingQueue<IWindowFocusStealer> queue = new ArrayBlockingQueue<>(1);

        final ServiceConnection connection = new ServiceConnection() {
            public void onServiceConnected(ComponentName className, IBinder service) {
                queue.add(IWindowFocusStealer.Stub.asInterface(service));
            }

            public void onServiceDisconnected(ComponentName className) {
            }
        };

        final Intent intent = new Intent();
        intent.setClass(context, WindowFocusStealerService.class);
        context.bindService(intent, connection, Context.BIND_AUTO_CREATE);

        final IWindowFocusStealer focusStealer;
        try {
            focusStealer = queue.poll(timeout, TimeUnit.MILLISECONDS);
        } catch (InterruptedException e) {
            throw new IllegalStateException(e);
        }

        if (focusStealer == null) {
            throw new TimeoutException();
        }

        final AtomicBoolean closed = new AtomicBoolean(false);
        return new WindowFocusStealer(focusStealer, () -> {
            if (closed.compareAndSet(false, true)) {
                context.unbindService(connection);
            }
        });
    }

    /**
     * Removes the temporary window and clean up everything.
     */
    @Override
    public void close() throws Exception {
        mCloser.run();
    }
}
