/*
 * Copyright (C) 2014 The Android Open Source Project
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
package com.droidlogic;

import android.app.Service;
import android.content.Intent;
import android.os.Binder;
import android.os.Bundle;
import android.os.Debug;
import android.os.IBinder;
import android.os.Process;
import android.os.RemoteException;
import android.os.DeadObjectException;
import android.os.Trace;
import android.util.Log;

//import com.android.internal.policy.IKeyguardDismissCallback;
//import com.android.internal.policy.IKeyguardDrawnCallback;
//import com.android.internal.policy.IKeyguardExitCallback;
//import com.android.internal.policy.IKeyguardService;
//import com.android.internal.policy.IKeyguardStateCallback;
import java.util.ArrayList;

//import static android.content.pm.PackageManager.PERMISSION_GRANTED;

public class StubKeyguardService extends Service {
    static final String TAG = "KeyguardService";
    static final String PERMISSION = "android.permission.CONTROL_KEYGUARD";//android.Manifest.permission.CONTROL_KEYGUARD;
    //private final ArrayList<IKeyguardStateCallback> mKeyguardStateCallbacks = new ArrayList<>();

    @Override
    public void onCreate() {
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;//mBinder;
    }

    void checkPermission() {
        //Log.d("VERI", "here"+Log.getStackTraceString(new Throwable()));
        // Avoid deadlock by avoiding calling back into the system process.
        if (Binder.getCallingUid() == Process.SYSTEM_UID) return;

/*
        // Otherwise,explicitly check for caller permission ...
        if (getBaseContext().checkCallingOrSelfPermission(PERMISSION) != PERMISSION_GRANTED) {
            Log.w(TAG, "Caller needs permission '" + PERMISSION + "' to call " + Debug.getCaller());
            throw new SecurityException("Access denied to process: " + Binder.getCallingPid()
                    + ", must have permission " + PERMISSION);
        }*/
    }

    void stateCallback_l() {
        /*
        int size = mKeyguardStateCallbacks.size();
        for (int i = size - 1; i >= 0; i--) {
            final IKeyguardStateCallback callback = mKeyguardStateCallbacks.get(i);
            Log.d("VERI", "stateCallback_l:"+callback);
            try {
                // input restrict default is true, which will block system window showing
                callback.onInputRestrictedStateChanged(false);
                callback.onSimSecureStateChanged(false);
                callback.onShowingStateChanged(false);
                callback.onTrustedChanged(false);
                callback.onHasLockscreenWallpaperChanged(false);
            } catch (RemoteException e) {
                Log.d("VERI", "Failed to call onDeviceProvisioned", e);
                if (e instanceof DeadObjectException) {
                    mKeyguardStateCallbacks.remove(callback);
                }
            }
        }
        */
    }

/*
    private final IKeyguardService.Stub mBinder = new IKeyguardService.Stub() {

        @Override // Binder interface
        public void addStateMonitorCallback(IKeyguardStateCallback callback) {
            checkPermission();
            synchronized (this) {
                mKeyguardStateCallbacks.add(callback);
                stateCallback_l();
            }
        }

        @Override // Binder interface
        public void verifyUnlock(IKeyguardExitCallback callback) {
            checkPermission();
            try {
                    callback.onKeyguardExitResult(true);
            } catch (RemoteException e) {
                Log.w(TAG, "Failed to call onKeyguardExitResult(true)", e);
            }
             Log.d("VERI", "verifyUnlock");
            synchronized (this) {
                stateCallback_l();
            }
        }

        @Override // Binder interface
        public void setOccluded(boolean isOccluded, boolean animate) {
            checkPermission();
        }

        @Override // Binder interface
        public void dismiss(IKeyguardDismissCallback callback, CharSequence message) {
            checkPermission();
            if (callback != null) {
                Log.w(TAG, "dismiss success+"+callback);
                try {
                    callback.onDismissSucceeded();
                } catch (RemoteException e) {
                    Log.w(TAG, "Failed to call onKeyguardExitResult(true)", e);
                }
             }
            synchronized (this) {
                stateCallback_l();
            }
        }

        @Override // Binder interface
        public void onDreamingStarted() {
            checkPermission();
        }

        @Override // Binder interface
        public void onDreamingStopped() {
            checkPermission();
        }

        @Override // Binder interface
        public void onStartedGoingToSleep(int reason) {
            checkPermission();
        }

        @Override // Binder interface
        public void onFinishedGoingToSleep(int reason, boolean cameraGestureTriggered) {
            checkPermission();
        }

        @Override // Binder interface
        public void onStartedWakingUp() {
            checkPermission();
        }

        @Override // Binder interface
        public void onFinishedWakingUp() {
            checkPermission();
        }

        @Override // Binder interface
        public void onScreenTurningOff() {
            checkPermission();
        }

        @Override // Binder interface
        public void onScreenTurningOn(IKeyguardDrawnCallback callback) {
            checkPermission();
            if (callback == null) return;

            try {
                callback.onDrawn();
            } catch (RemoteException e) {
                Log.w(TAG, "Exception calling onDrawn():", e);
            }
            synchronized (this) {
                stateCallback_l();
            }
        }

        @Override // Binder interface
        public void onScreenTurnedOn() {
            checkPermission();
            synchronized (this) {
                stateCallback_l();
            }
        }

        @Override // Binder interface
        public void onScreenTurnedOff() {
            checkPermission();
        }

        @Override // Binder interface
        public void setKeyguardEnabled(boolean enabled) {
            checkPermission();
        }

        @Override // Binder interface
        public void onSystemReady() {
            checkPermission();
        }

        @Override // Binder interface
        public void doKeyguardTimeout(Bundle options) {
            checkPermission();
        }

        @Override // Binder interface
        public void setSwitchingUser(boolean switching) {
            checkPermission();
        }

        @Override // Binder interface
        public void setCurrentUser(int userId) {
            checkPermission();
        }

        @Override
        public void onBootCompleted() {
            checkPermission();
        }

        @Override
        public void startKeyguardExitAnimation(long startTime, long fadeoutDuration) {
            checkPermission();
        }

        @Override
        public void onShortPowerPressedGoHome() {
            checkPermission();
        }
    };
    */
}

