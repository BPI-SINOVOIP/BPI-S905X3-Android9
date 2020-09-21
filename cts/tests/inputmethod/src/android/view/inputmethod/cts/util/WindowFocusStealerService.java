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

import android.app.Service;
import android.content.Intent;
import android.graphics.Color;
import android.graphics.PixelFormat;
import android.graphics.drawable.ColorDrawable;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.ResultReceiver;
import androidx.annotation.BinderThread;
import androidx.annotation.MainThread;
import androidx.annotation.Nullable;
import android.view.WindowManager;
import android.widget.TextView;

public final class WindowFocusStealerService extends Service {

    @Nullable
    private TextView mCustomView;

    private final class BinderService extends IWindowFocusStealer.Stub {
        @BinderThread
        @Override
        public void stealWindowFocus(IBinder parentAppWindowToken, ResultReceiver resultReceiver) {
            mMainHandler.post(() -> handleStealWindowFocus(parentAppWindowToken, resultReceiver));
        }
    }

    private final Handler mMainHandler = new Handler(Looper.getMainLooper());

    @Override
    public IBinder onBind(Intent intent) {
        return new BinderService();
    }

    @Override
    public boolean onUnbind(Intent intent) {
        handleReset();
        return false;
    }

    @MainThread
    public void handleStealWindowFocus(
            IBinder parentAppWindowToken, ResultReceiver resultReceiver) {
        handleReset();

        mCustomView = new TextView(this) {
            private boolean mWindowInitiallyFocused = false;
            /**
             * {@inheritDoc}
             */
            @Override
            public void onWindowFocusChanged(boolean hasWindowFocus) {
                if (!mWindowInitiallyFocused && hasWindowFocus) {
                    mWindowInitiallyFocused = true;
                    resultReceiver.send(0, null);
                }
            }
        };
        mCustomView.setText("Popup");
        mCustomView.setBackground(new ColorDrawable(Color.CYAN));
        mCustomView.setElevation(0.5f);

        WindowManager mWm = getSystemService(WindowManager.class);
        WindowManager.LayoutParams params = new WindowManager.LayoutParams(
                150, 150, 10, 10,
                WindowManager.LayoutParams.TYPE_APPLICATION_PANEL,
                WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM
                        | WindowManager.LayoutParams.FLAG_NOT_TOUCH_MODAL,
                PixelFormat.OPAQUE);
        params.packageName = getPackageName();
        params.token = parentAppWindowToken;

        mWm.addView(mCustomView, params);
    }

    @MainThread
    public void handleReset() {
        if (mCustomView != null) {
            getSystemService(WindowManager.class).removeView(mCustomView);
            mCustomView = null;
        }
    }

    @MainThread
    public void onDestroy() {
        super.onDestroy();
        handleReset();
    }

}
