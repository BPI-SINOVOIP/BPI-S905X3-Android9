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
package com.android.car.vehiclehal.test;

import android.hardware.automotive.vehicle.V2_0.VehiclePropValue;
import android.os.ConditionVariable;

import java.util.ArrayList;
import java.util.List;

/**
 * The verifier class is used to verify received VHAL events against expected events on-the-fly.
 * It is initialized with a list of expected events and moving down the list to verify received
 * events. The verifier object not reusable and should be discarded once the verification is done.
 * The verifier will provide formatted result for all mismatched events in sequence.
 */
class VhalEventVerifier {
    private List<VehiclePropValue> mExpectedEvents;
    // A pointer to keep track of the next expected event in the list
    private int mIdx;
    private List<MismatchedEventPair> mMismatchedEvents;
    // Condition variable to notify waiting threads when verification is done or timeout.
    private ConditionVariable mCond;

    static class MismatchedEventPair {
        public final int idx;
        public final VehiclePropValue expectedEvent;
        public final VehiclePropValue mismatchedEvent;

        MismatchedEventPair(VehiclePropValue expectedEvent, VehiclePropValue mismatchedEvent,
                                   int idx) {
            this.idx = idx;
            this.expectedEvent = expectedEvent;
            this.mismatchedEvent = mismatchedEvent;
        }
    }

    VhalEventVerifier(List<VehiclePropValue> expectedEvents) {
        mExpectedEvents = expectedEvents;
        mIdx = 0;
        mMismatchedEvents = new ArrayList<>();
        mCond = new ConditionVariable(expectedEvents.isEmpty());
    }

    /**
     * Verification method that checks the equality of received event against expected event. Once
     * it reaches to the end of list, it will unblock the waiting threads. Note, the verification
     * method is not thread-safe. It assumes only a single thread is calling the method at all time.
     *
     * @param nextEvent to be verified
     */
    public void verify(VehiclePropValue nextEvent) {
        if (mIdx >= mExpectedEvents.size()) {
            return;
        }
        VehiclePropValue expectedEvent = mExpectedEvents.get(mIdx);
        if (!Utils.areVehiclePropValuesEqual(expectedEvent, nextEvent)) {
            mMismatchedEvents.add(new MismatchedEventPair(expectedEvent, nextEvent, mIdx));
        }
        if (++mIdx == mExpectedEvents.size()) {
            mCond.open();
        }
    }

    public List<MismatchedEventPair> getMismatchedEvents() {
        return mMismatchedEvents;
    }

    public void waitForEnd(long timeout) {
        mCond.block(timeout);
    }

    public String getResultString() {
        StringBuilder resultBuilder = new StringBuilder();
        for (MismatchedEventPair pair : mMismatchedEvents) {
            resultBuilder.append("Index " + pair.idx + ": Expected "
                    + Utils.vehiclePropValueToString(pair.expectedEvent) + ", Received "
                    + Utils.vehiclePropValueToString(pair.mismatchedEvent) + "\n");
        }
        return resultBuilder.toString();
    }
}
