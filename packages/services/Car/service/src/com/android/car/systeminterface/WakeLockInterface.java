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

import android.content.Context;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import com.android.car.CarLog;

/**
 * Interface that abstracts wake lock operations
 */
public interface WakeLockInterface {
    void releaseAllWakeLocks();
    void switchToPartialWakeLock();
    void switchToFullWakeLock();

    class DefaultImpl implements WakeLockInterface {
        private final WakeLock mPartialWakeLock;
        private final WakeLock mFullWakeLock;

        DefaultImpl(Context context) {
            PowerManager powerManager =
                    (PowerManager) context.getSystemService(Context.POWER_SERVICE);
            mFullWakeLock = powerManager.newWakeLock(PowerManager.SCREEN_DIM_WAKE_LOCK,
                CarLog.TAG_POWER);
            mPartialWakeLock = powerManager.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK,
                CarLog.TAG_POWER);
        }

        @Override
        public void switchToPartialWakeLock() {
            if (!mPartialWakeLock.isHeld()) {
                mPartialWakeLock.acquire();
            }
            if (mFullWakeLock.isHeld()) {
                mFullWakeLock.release();
            }
        }

        @Override
        public void switchToFullWakeLock() {
            if (!mFullWakeLock.isHeld()) {
                mFullWakeLock.acquire();
            }
            if (mPartialWakeLock.isHeld()) {
                mPartialWakeLock.release();
            }
        }

        @Override
        public void releaseAllWakeLocks() {
            if (mPartialWakeLock.isHeld()) {
                mPartialWakeLock.release();
            }
            if (mFullWakeLock.isHeld()) {
                mFullWakeLock.release();
            }
        }
    }
}
