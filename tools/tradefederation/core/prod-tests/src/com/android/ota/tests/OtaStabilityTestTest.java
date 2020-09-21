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
package com.android.ota.tests;

import com.android.tradefed.testtype.IRemoteTest;

import junit.framework.TestCase;

import java.util.Collection;
import java.util.Iterator;

/**
 * Unit tests for {@link OtaStabilityTest}
 */
public class OtaStabilityTestTest extends TestCase {

    /**
     * Test basic case {@link OtaStabilityTest#split()}, where iterations divide evenly into each
     * shards.
     */
    public void testSplit_even() {
        OtaStabilityTest test = new OtaStabilityTest();
        test.setIterations(10);
        test.setShards(5);
        Collection<IRemoteTest> shards = test.split();
        assertEquals(5, shards.size());
        for (IRemoteTest shardTest : shards) {
            OtaStabilityTest otaShard = (OtaStabilityTest)shardTest;
            assertEquals(2, otaShard.getIterations());
        }
    }

    /**
     * Test {@link OtaStabilityTest#split()}, where there are more iterations than shards.
     */
    public void testSplit_shards() {
        OtaStabilityTest test = new OtaStabilityTest();
        test.setIterations(5);
        test.setShards(10);
        Collection<IRemoteTest> shards = test.split();
        assertEquals(5, shards.size());
        for (IRemoteTest shardTest : shards) {
            OtaStabilityTest otaShard = (OtaStabilityTest)shardTest;
            assertEquals(1, otaShard.getIterations());
        }
    }

    /**
     * Test {@link OtaStabilityTest#split()}, where iterations does not divide evenly
     */
    public void testSplit_remainder() {
        OtaStabilityTest test = new OtaStabilityTest();
        test.setIterations(10);
        test.setShards(3);
        Collection<IRemoteTest> shards = test.split();
        assertEquals(3, shards.size());
        Iterator<IRemoteTest> iterator = shards.iterator();
        IRemoteTest first = iterator.next();
        assertEquals(3, ((OtaStabilityTest)first).getIterations());

        IRemoteTest second = iterator.next();
        assertEquals(3, ((OtaStabilityTest)second).getIterations());

        IRemoteTest three = iterator.next();
        assertEquals(4, ((OtaStabilityTest)three).getIterations());
    }
}
