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
package com.android.tv.tuner.util;

import java.util.ArrayList;
import java.util.List;

/** Static utility methods pertaining to int primitives. (Referred Guava's Ints class) */
public class Ints {
    private Ints() {}

    public static int[] toArray(List<Integer> integerList) {
        int[] intArray = new int[integerList.size()];
        int i = 0;
        for (Integer data : integerList) {
            intArray[i++] = data;
        }
        return intArray;
    }

    public static List<Integer> asList(int[] intArray) {
        List<Integer> integerList = new ArrayList<>(intArray.length);
        for (int data : intArray) {
            integerList.add(data);
        }
        return integerList;
    }
}
