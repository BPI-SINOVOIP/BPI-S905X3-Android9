/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.bips.util;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A lock that always allows higher-priority attempts to receive the lock first
 */
public class PriorityLock {
    private boolean mLocked = false;
    private List<Integer> mPriorities = new ArrayList<>();

    /**
     * Return when locked, always giving priority to the highest priority lock request
     */
    public synchronized void lock(int priority) throws InterruptedException {
        if (mLocked) {
            mPriorities.add(priority);
            Collections.sort(mPriorities);
            try {
                while (mLocked || priority < mPriorities.get(mPriorities.size() - 1)) {
                    wait();
                }
            } finally {
                mPriorities.remove((Integer) priority);
            }
        }
        mLocked = true;
    }

    /**
     * Unlock this object (when it is already locked)
     */
    public synchronized void unlock() {
        if (!mLocked) {
            throw new IllegalArgumentException("not locked");
        }
        mLocked = false;
        notifyAll();
    }
}
