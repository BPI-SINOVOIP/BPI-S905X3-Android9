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

package android.app.cts.android.app.cts.tools;

import android.app.ActivityManager;
import android.content.Context;
import android.os.SystemClock;
import android.util.Log;

/**
 * Helper for monitoring the importance state of a uid.
 */
public final class UidImportanceListener implements ActivityManager.OnUidImportanceListener {
    final ActivityManager mAm;
    final int mUid;
    final int mCutPoint;
    final long mDefaultWaitTime;

    int mLastValue = -1;

    public UidImportanceListener(Context context, int uid, int cutPoint) {
        this(context, uid, cutPoint, 5*1000);
    }

    public UidImportanceListener(Context context, int uid, int cutPoint, long defaultWaitTime) {
        mAm = context.getSystemService(ActivityManager.class);
        mUid = uid;
        mCutPoint = cutPoint;
        mDefaultWaitTime = defaultWaitTime;
    }

    @Override
    public void onUidImportance(int uid, int importance) {
        synchronized (this) {
            Log.d("XXXXX", "Got importance for uid " + uid + " (cut " + mCutPoint
                    + "): " + importance);
            if (uid == mUid) {
                mLastValue = importance;
                notifyAll();
            }
        }
    }

    public void register() {
        mAm.addOnUidImportanceListener(this, mCutPoint);
    }

    public void unregister() {
        mAm.removeOnUidImportanceListener(this);
    }

    public int waitForValue(int minValue, int maxValue) {
        return waitForValue(minValue, maxValue, mDefaultWaitTime);
    }

    public int waitForValue(int minValue, int maxValue, long timeout) {
        final long endTime = SystemClock.uptimeMillis()+timeout;

        synchronized (this) {
            while (mLastValue < minValue || mLastValue > maxValue) {
                final long now = SystemClock.uptimeMillis();
                if (now >= endTime) {
                    throw new IllegalStateException("Timed out waiting for importance "
                            + minValue + "-" + maxValue + " (cut "
                            + mCutPoint + "), last was " + mLastValue);
                }
                try {
                    wait(endTime-now);
                } catch (InterruptedException e) {
                }
            }
            Log.d("XXXX", "waitForValue " + minValue + "-" + maxValue + " (cut "
                    + mCutPoint + "): " + mLastValue);
            return mLastValue;
        }
    }
}
