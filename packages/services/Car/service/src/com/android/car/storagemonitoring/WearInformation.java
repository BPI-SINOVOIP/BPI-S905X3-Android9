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
package com.android.car.storagemonitoring;

import static java.util.Objects.hash;

import android.car.storagemonitoring.WearEstimate;
import java.time.Instant;

public final class WearInformation {
    public static final int UNKNOWN_LIFETIME_ESTIMATE = -1;

    public static final int UNKNOWN_PRE_EOL_INFO = 0;
    public static final int PRE_EOL_INFO_NORMAL = 1;
    public static final int PRE_EOL_INFO_WARNING = 2;
    public static final int PRE_EOL_INFO_URGENT = 3;

    private static final String UNKNOWN = "unknown";
    private static final String[] PRE_EOL_STRINGS = new String[] {
        UNKNOWN, "normal", "warning", "urgent"
    };

    /**
     * A lower bound on the lifetime estimate for "type A" memory cells, expressed as a percentage.
     */
    public final int lifetimeEstimateA;

    /**
     * A lower bound on the lifetime estimate for "type B" memory cells, expressed as a percentage.
     */
    public final int lifetimeEstimateB;

    /**
     * An estimate of the lifetime based on reserved block consumption.
     */
    public final int preEolInfo;

    public WearInformation(int lifetimeA, int lifetimeB, int preEol) {
        lifetimeEstimateA = lifetimeA;
        lifetimeEstimateB = lifetimeB;
        preEolInfo = preEol;
    }

    @Override
    public int hashCode() {
        return hash(lifetimeEstimateA, lifetimeEstimateB, preEolInfo);
    }

    @Override
    public boolean equals(Object other) {
        if (other instanceof WearInformation) {
            WearInformation wi = (WearInformation)other;
            return (wi.lifetimeEstimateA == lifetimeEstimateA) &&
                (wi.lifetimeEstimateB == lifetimeEstimateB) &&
                (wi.preEolInfo == preEolInfo);
        } else {
            return false;
        }
    }

    private String lifetimeToString(int lifetime) {
        if (lifetime == UNKNOWN_LIFETIME_ESTIMATE) return UNKNOWN;

        return lifetime + "%";
    }

    @Override
    public String toString() {
        return String.format("lifetime estimate: A = %s, B = %s; pre EOL info: %s",
                lifetimeToString(lifetimeEstimateA),
                lifetimeToString(lifetimeEstimateB),
                PRE_EOL_STRINGS[preEolInfo]);
    }

    public WearEstimate toWearEstimate() {
        return new WearEstimate(lifetimeEstimateA, lifetimeEstimateB);
    }
}
