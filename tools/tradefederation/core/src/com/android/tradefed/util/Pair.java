/*
 * Copyright (C) 2013 The Android Open Source Project
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
 * Define our own Pair class which contains two objects.
 */
public class Pair<A, B> {
    public final A first;
    public final B second;

    public Pair(A first, B second) {
        this.first = first;
        this.second = second;
    }

    /**
     * Checks whether two objects are equal
     */
    @Override
    public boolean equals(Object o) {
        if (!(o instanceof Pair)) {
            // o is not an instance of Pair object
            return false;
        }
        final Pair<?, ?> pair = (Pair<?, ?>) o;
        if (this == pair) {
            return true;
        }

        if (this.first == null) {
            if (pair.first != null) {
                return false;
            }
        } else if (!this.first.equals(pair.first)) {
            return false;
        }
        if (this.second == null) {
            if (pair.second != null) {
                return false;
            }
        } else if (!this.second.equals(pair.second)) {
            return false;
        }
        return true;
    }

    /**
     * Return a hashcode using hash codes from the inner objects
     */
    @Override
    public int hashCode() {
        return (first == null ? 0 : first.hashCode()) ^ (second == null ? 0 : second.hashCode());
    }
    /**
     * Convenience method to create a new pair.
     * @param a the first object in the Pair
     * @param b the second object in the Pair
     * @return a Pair contains a and b
     */
    public static <A, B> Pair<A, B> create(A a, B b) {
        return new Pair<A, B> (a, b);
    }

}
