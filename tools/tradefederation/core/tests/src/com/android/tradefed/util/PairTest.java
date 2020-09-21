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

import junit.framework.TestCase;

import org.junit.Assert;

public class PairTest extends TestCase {

    public void testPairs() throws Exception {
        Object obj1 = new Object();
        Pair<Object, Object> p1 = Pair.create(obj1, obj1);
        // case 1: One object is not an instance of Pair
        Assert.assertFalse(p1.equals(obj1));

        // case 2: one pair is null
        Pair<Object, Object> p2 = null;
        Assert.assertFalse(p1.equals(p2));

        // case 3: two pairs with both objects equal to null;
        p1 = Pair.create(null, null);
        p2 = Pair.create(null, null);
        Assert.assertTrue(p1.equals(p2));

        // case 4: a Pair with both elements null, not equal to null object
        Assert.assertFalse(p1.equals(null));

        // case 5: two pairs with different object for the second element
        p1 = Pair.create(null, new Object());
        p2 = Pair.create(null, new Object());
        Assert.assertFalse(p1.equals(p2));

        // case 6: two equal pairs
        p1 = Pair.create(null, obj1);
        p2 = Pair.create(null, obj1);
        Assert.assertTrue(p1.equals(p2));

        // case 7: two pairs with both elements objects
        Object obj2 = new Object();
        p1 = Pair.create(obj1, obj2);
        p2 = Pair.create(obj1, obj2);
        Assert.assertTrue(p1.equals(p2));

        Pair<String, String> p3 = Pair.create(new String("abc"), new String("2.4"));
        Pair<String, String> p4 = Pair.create(new String("abc"), new String("2.4"));
        Assert.assertTrue(p3.equals(p4));
    }
}
