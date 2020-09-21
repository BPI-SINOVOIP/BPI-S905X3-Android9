/*
 * Copyright (C) 2016 The Android Open Source Project
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

package com.android.car.systeminterface;

import android.content.ContentResolver;
import android.content.Context;
import android.database.ContentObserver;
import android.hardware.display.DisplayManager;
import android.hardware.display.DisplayManager.DisplayListener;
import android.os.Handler;
import android.os.Looper;
import android.os.PowerManager;
import android.os.SystemClock;
import android.provider.Settings.SettingNotFoundException;
import android.provider.Settings.System;
import android.util.Log;
import android.view.Display;

import com.android.car.CarLog;
import com.android.car.CarPowerManagementService;

/**
 * Interface that abstracts display operations
 */
public interface DisplayInterface {
    /**
     * @param brightness Level from 0 to 100%
     */
    void setDisplayBrightness(int brightness);
    void setDisplayState(boolean on);
    void startDisplayStateMonitoring(CarPowerManagementService service);
    void stopDisplayStateMonitoring();

    class DefaultImpl implements DisplayInterface {
        private final ContentResolver mContentResolver;
        private final Context mContext;
        private final DisplayManager mDisplayManager;
        private final int mMaximumBacklight;
        private final int mMinimumBacklight;
        private final PowerManager mPowerManager;
        private final WakeLockInterface mWakeLockInterface;
        private CarPowerManagementService mService;
        private boolean mDisplayStateSet;

        private ContentObserver mBrightnessObserver =
                new ContentObserver(new Handler(Looper.getMainLooper())) {
                    @Override
                    public void onChange(boolean selfChange) {
                        int brightness = mMinimumBacklight;
                        int range = mMaximumBacklight - mMinimumBacklight;

                        try {
                            brightness = System.getInt(mContentResolver, System.SCREEN_BRIGHTNESS);
                        } catch (SettingNotFoundException e) {
                            Log.e(CarLog.TAG_POWER, "Could not get SCREEN_BRIGHTNESS:  " + e);
                        }
                        // Convert brightness from 0-255 to 0-100%
                        brightness -= mMinimumBacklight;
                        brightness *= 100;
                        brightness += (range + 1) / 2;
                        brightness /= range;
                        mService.sendDisplayBrightness(brightness);
                    }
                };

        private final DisplayManager.DisplayListener mDisplayListener = new DisplayListener() {
            @Override
            public void onDisplayAdded(int displayId) {
                //ignore
            }

            @Override
            public void onDisplayRemoved(int displayId) {
                //ignore
            }

            @Override
            public void onDisplayChanged(int displayId) {
                if (displayId == Display.DEFAULT_DISPLAY) {
                    handleMainDisplayChanged();
                }
            }
        };

        DefaultImpl(Context context, WakeLockInterface wakeLockInterface) {
            mContext = context;
            mContentResolver = mContext.getContentResolver();
            mDisplayManager = (DisplayManager) context.getSystemService(Context.DISPLAY_SERVICE);
            mPowerManager = (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            mMaximumBacklight = mPowerManager.getMaximumScreenBrightnessSetting();
            mMinimumBacklight = mPowerManager.getMinimumScreenBrightnessSetting();
            mWakeLockInterface = wakeLockInterface;
        }

        private void handleMainDisplayChanged() {
            boolean isOn = isMainDisplayOn();
            CarPowerManagementService service;
            synchronized (this) {
                if (mDisplayStateSet == isOn) { // same as what is set
                    return;
                }
                service = mService;
            }
            service.handleMainDisplayChanged(isOn);
        }

        private boolean isMainDisplayOn() {
            Display disp = mDisplayManager.getDisplay(Display.DEFAULT_DISPLAY);
            return disp.getState() == Display.STATE_ON;
        }

        @Override
        public void setDisplayBrightness(int brightness) {
            // Brightness is set in percent.  Need to convert this into 0-255 scale.  The actual
            //  brightness algorithm should look like this:
            //
            //      newBrightness = (brightness * (max - min)) + min
            //
            //  Since we're using integer arithmetic, do the multiplication first, then add 50 to
            //  round up as needed.
            brightness *= mMaximumBacklight - mMinimumBacklight;    // Multiply by full range
            brightness += 50;                                       // Integer rounding
            brightness /= 100;                                      // Divide by 100
            brightness += mMinimumBacklight;
            // Range checking
            if (brightness < mMinimumBacklight) {
                brightness = mMinimumBacklight;
            } else if (brightness > mMaximumBacklight) {
                brightness = mMaximumBacklight;
            }
            // Set the brightness
            System.putInt(mContentResolver, System.SCREEN_BRIGHTNESS, brightness);
        }

        @Override
        public void startDisplayStateMonitoring(CarPowerManagementService service) {
            synchronized (this) {
                mService = service;
                mDisplayStateSet = isMainDisplayOn();
            }
            mContentResolver.registerContentObserver(System.getUriFor(System.SCREEN_BRIGHTNESS),
                                                     false, mBrightnessObserver);
            mDisplayManager.registerDisplayListener(mDisplayListener, service.getHandler());
        }

        @Override
        public void stopDisplayStateMonitoring() {
            mDisplayManager.unregisterDisplayListener(mDisplayListener);
            mContentResolver.unregisterContentObserver(mBrightnessObserver);
        }

        @Override
        public void setDisplayState(boolean on) {
            synchronized (this) {
                mDisplayStateSet = on;
            }
            if (on) {
                mWakeLockInterface.switchToFullWakeLock();
                Log.i(CarLog.TAG_POWER, "on display");
                mPowerManager.wakeUp(SystemClock.uptimeMillis());
            } else {
                mWakeLockInterface.switchToPartialWakeLock();
                Log.i(CarLog.TAG_POWER, "off display");
                mPowerManager.goToSleep(SystemClock.uptimeMillis());
            }
        }
    }
}
