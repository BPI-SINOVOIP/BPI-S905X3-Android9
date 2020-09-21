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
package com.android.tradefed.util;

/**
 * A class with utility functions to help with dealing with <code>null</code>
 */
public class NullUtil {
    /**
     * Counts <code>null</code> objects in the passed set
     */
    public static int countNulls(Object... objs) {
        int count = 0;
        for (Object obj : objs) {
            if (obj == null) count++;
        }
        return count;
    }

    /**
     * Counts non-<code>null</code> objects in the passed set
     */
    public static int countNonNulls(Object... objs) {
        int count = 0;
        for (Object obj : objs) {
            if (obj != null) count++;
        }
        return count;
    }

    /**
     * Checks if all objects are null.  Uses short-circuit logic, so may be more efficient than
     * {@link #countNonNulls(Object...)} for sets of many objects.
     *
     * @return <code>false</code> if any passed objects are non-null.  <code>true</code> otherwise.
     *         In particular, returns true of no objects are passed.
     */
    public static boolean allNull(Object... objs) {
        for (Object obj : objs) {
            if (obj != null) return false;
        }
        return true;
    }

    /**
     * Checks if exactly one object is non-null.  Uses short-circuit logic, so may be more efficient
     * than {@link #countNonNulls(Object...)} for sets of many objects.
     *
     * @return <code>true</code> if there is exactly one non-<code>null</code> object in the list.
     *         <code>false</code> otherwise.
     */
    public static boolean singleNonNull(Object... objs) {
        int nonNullCount = 0;
        for (Object obj : objs) {
            if (obj != null) nonNullCount++;
            if (nonNullCount > 1) return false;
        }
        return nonNullCount == 1;
    }

    /**
     * Check if every object is null, or every object is non-null.  Uses short-circuit logic, so may
     * be more efficient than {@link #countNulls(Object...)} and {@link #countNonNulls(Object...)}
     * for sets of many objects.
     *
     * @return <code>true</code> if every object is <code>null</code> or if every object is
     *         non-<code>null</code>.  <code>false</code> otherwise.
     */
    public static boolean isHomogeneousSet(Object... objs) {
        if (objs.length == 0) return true;

        final boolean expectNull = (objs[0] == null);
        for (Object obj : objs) {
            if (expectNull ^ (obj == null)) {
                // xor tells us when the parities differ, which is only when objs[0] was null and
                // obj is non-null, or objs[0] was non-null and obj is null
                return false;
            }
        }
        return true;
    }
}
