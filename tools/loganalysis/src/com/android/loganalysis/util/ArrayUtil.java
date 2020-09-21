/*
 * Copyright (C) 2011 The Android Open Source Project
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
package com.android.loganalysis.util;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;

/**
 * Utility methods for arrays
 */
// TODO: Use libTF once this is copied over.
public class ArrayUtil {

    private ArrayUtil() {
    }

    /**
     * Build an array from the provided contents.
     *
     * <p>
     * The resulting array will be the concatenation of <var>arrays</var> input arrays, in their
     * original order.
     * </p>
     *
     * @param arrays the arrays to concatenate
     * @return the newly constructed array
     */
    public static String[] buildArray(String[]... arrays) {
        int length = 0;
        for (String[] array : arrays) {
            length += array.length;
        }
        String[] newArray = new String[length];
        int offset = 0;
        for (String[] array : arrays) {
            System.arraycopy(array, 0, newArray, offset, array.length);
            offset += array.length;
        }
        return newArray;
    }

    /**
     * Convert a varargs list/array to an {@link List}.  This is useful for building instances of
     * {@link List} by hand.  Note that this differs from {@link java.util.Arrays#asList} in that
     * the returned array is mutable.
     *
     * @param inputAry an array, or a varargs list
     * @return a {@link List} instance with the identical contents
     */
    @SafeVarargs
    public static <T> List<T> list(T... inputAry) {
        List<T> retList = new ArrayList<T>(inputAry.length);
        for (T item : inputAry) {
            retList.add(item);
        }
        return retList;
    }

    private static String internalJoin(String sep, Collection<Object> pieces) {
        StringBuilder sb = new StringBuilder();
        boolean skipSep = true;
        Iterator<Object> iter = pieces.iterator();
        while (iter.hasNext()) {
            if (skipSep) {
                skipSep = false;
            } else {
                sb.append(sep);
            }

            Object obj = iter.next();
            if (obj == null) {
                sb.append("null");
            } else {
                sb.append(obj.toString());
            }
        }
        return sb.toString();
    }

    /**
     * Turns a sequence of objects into a string, delimited by {@code sep}.  If a single
     * {@code Collection} is passed, it is assumed that the elements of that Collection are to be
     * joined.  Otherwise, wraps the passed {@link Object}(s) in a {@link List} and joins the
     * generated list.
     *
     * @param sep the string separator to delimit the different output segments.
     * @param pieces A {@link Collection} or a varargs {@code Array} of objects.
     */
    @SuppressWarnings("unchecked")
    public static String join(String sep, Object... pieces) {
        if ((pieces.length == 1) && (pieces[0] instanceof Collection)) {
            // Don't re-wrap the Collection
            return internalJoin(sep, (Collection<Object>) pieces[0]);
        } else {
            return internalJoin(sep, Arrays.asList(pieces));
        }
    }
}
