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

package com.android.services.telephony;

import android.telecom.Log;
import android.telecom.PhoneAccountHandle;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * @hide
 */
public class HoldTracker {
    private final Map<PhoneAccountHandle, List<Holdable>> mHoldables;

    public HoldTracker() {
        mHoldables = new HashMap<>();
    }

    /**
     * Adds the holdable associated with the {@code phoneAccountHandle}, this method may update
     * the hold state for all holdable associated with the {@code phoneAccountHandle}.
     */
    public void addHoldable(PhoneAccountHandle phoneAccountHandle, Holdable holdable) {
        if (!mHoldables.containsKey(phoneAccountHandle)) {
            mHoldables.put(phoneAccountHandle, new ArrayList<>(1));
        }
        List<Holdable> holdables = mHoldables.get(phoneAccountHandle);
        if (!holdables.contains(holdable)) {
            holdables.add(holdable);
            updateHoldCapability(phoneAccountHandle);
        }
    }

    /**
     * Removes the holdable associated with the {@code phoneAccountHandle}, this method may update
     * the hold state for all holdable associated with the {@code phoneAccountHandle}.
     */
    public void removeHoldable(PhoneAccountHandle phoneAccountHandle, Holdable holdable) {
        if (!mHoldables.containsKey(phoneAccountHandle)) {
            return;
        }

        if (mHoldables.get(phoneAccountHandle).remove(holdable)) {
            updateHoldCapability(phoneAccountHandle);
        }
    }

    /**
     * Updates the hold capability for all holdables associated with the {@code phoneAccountHandle}.
     */
    public void updateHoldCapability(PhoneAccountHandle phoneAccountHandle) {
        if (!mHoldables.containsKey(phoneAccountHandle)) {
            return;
        }

        List<Holdable> holdables = mHoldables.get(phoneAccountHandle);
        int topHoldableCount = 0;
        for (Holdable holdable : holdables) {
            if (!holdable.isChildHoldable()) {
                ++topHoldableCount;
            }
        }

        Log.d(this, "topHoldableCount = " + topHoldableCount);
        boolean isHoldable = topHoldableCount < 2;
        for (Holdable holdable : holdables) {
            holdable.setHoldable(holdable.isChildHoldable() ? false : isHoldable);
        }
    }
}
