/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.contacts.util;

/**
 * Useful math functions that aren't in java.lang.Math
 */
public class MoreMath {
    /**
     * If the input value lies outside of the specified range, return the nearer
     * bound. Otherwise, return the input value, unchanged.
     */
    public static int clamp(int input, int lowerBound, int upperBound) {
        if (input < lowerBound) return lowerBound;
        if (input > upperBound) return upperBound;
        return input;
    }

    /**
     * If the input value lies outside of the specified range, return the nearer
     * bound. Otherwise, return the input value, unchanged.
     */
    public static float clamp(float input, float lowerBound, float upperBound) {
        if (input < lowerBound) return lowerBound;
        if (input > upperBound) return upperBound;
        return input;
    }

    /**
     * If the input value lies outside of the specified range, return the nearer
     * bound. Otherwise, return the input value, unchanged.
     */
    public static double clamp(double input, double lowerBound, double upperBound) {
        if (input < lowerBound) return lowerBound;
        if (input > upperBound) return upperBound;
        return input;
    }
}
