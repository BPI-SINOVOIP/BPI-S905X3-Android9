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
package com.android.tradefed.testtype.suite;

import static org.junit.Assert.*;

import com.android.tradefed.invoker.shard.StrictShardHelperTest.SplitITestSuite;
import com.android.tradefed.testtype.IRemoteTest;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.util.Collection;
import java.util.Iterator;

/** Unit tests for {@link ModuleMerger}. */
@RunWith(JUnit4.class)
public class ModuleMergerTest {

    /**
     * Test that {@link ModuleMerger#arePartOfSameSuite(ITestSuite, ITestSuite)} returns false when
     * the first suite is not splitted yet.
     */
    @Test
    public void testPartOfSameSuite_notSplittedYet() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        assertFalse(ModuleMerger.arePartOfSameSuite(suite1, suite2));
    }

    /**
     * Test that {@link ModuleMerger#arePartOfSameSuite(ITestSuite, ITestSuite)} returns false when
     * the second suite is not splitted yet.
     */
    @Test
    public void testPartOfSameSuite_notSplittedYet2() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        assertFalse(ModuleMerger.arePartOfSameSuite((ITestSuite) res1.iterator().next(), suite2));
    }

    /**
     * Test that {@link ModuleMerger#arePartOfSameSuite(ITestSuite, ITestSuite)} returns true when
     * the two suites are splitted and from the same module.
     */
    @Test
    public void testPartOfSameSuite_sameSuite() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        Iterator<IRemoteTest> ite = res1.iterator();
        assertTrue(
                ModuleMerger.arePartOfSameSuite((ITestSuite) ite.next(), (ITestSuite) ite.next()));
    }

    /**
     * Test that {@link ModuleMerger#arePartOfSameSuite(ITestSuite, ITestSuite)} returns false when
     * the two suites are splitted but from different modules.
     */
    @Test
    public void testPartOfSameSuite_notSameSuite() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        Collection<IRemoteTest> res2 = suite2.split(2);
        assertFalse(
                ModuleMerger.arePartOfSameSuite(
                        (ITestSuite) res1.iterator().next(), (ITestSuite) res2.iterator().next()));
    }

    /**
     * Test that {@link ModuleMerger#mergeSplittedITestSuite(ITestSuite, ITestSuite)} throws an
     * exception when the first suite is not splitted yet.
     */
    @Test
    public void testMergeSplittedITestSuite_notSplittedYet() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        try {
            ModuleMerger.mergeSplittedITestSuite(suite1, suite2);
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    /**
     * Test that {@link ModuleMerger#mergeSplittedITestSuite(ITestSuite, ITestSuite)} throws an
     * exception when the second suite is not splitted yet.
     */
    @Test
    public void testMergeSplittedITestSuite_notSplittedYet2() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        try {
            ModuleMerger.mergeSplittedITestSuite((ITestSuite) res1.iterator().next(), suite2);
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    /**
     * Test that {@link ModuleMerger#mergeSplittedITestSuite(ITestSuite, ITestSuite)} throws an
     * exception when the two suites are splitted but coming from different modules.
     */
    @Test
    public void testMergeSplittedITestSuite_splittedSuiteFromDifferentModules() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        SplitITestSuite suite2 = new SplitITestSuite("module2");
        Collection<IRemoteTest> res2 = suite2.split(2);
        try {
            ModuleMerger.mergeSplittedITestSuite(
                    (ITestSuite) res1.iterator().next(), (ITestSuite) res2.iterator().next());
            fail("Should have thrown an exception.");
        } catch (IllegalArgumentException expected) {
            // expected
        }
    }

    /**
     * Test that {@link ModuleMerger#mergeSplittedITestSuite(ITestSuite, ITestSuite)} properly
     * assigns tests from the second suite to the first since part of same module.
     */
    @Test
    public void testMergeSplittedITestSuite() {
        SplitITestSuite suite1 = new SplitITestSuite("module1");
        Collection<IRemoteTest> res1 = suite1.split(2);
        Iterator<IRemoteTest> ite = res1.iterator();
        ITestSuite split1 = (ITestSuite) ite.next();
        ITestSuite split2 = (ITestSuite) ite.next();
        assertEquals(2, split1.getDirectModule().numTests());
        assertEquals(2, split2.getDirectModule().numTests());
        ModuleMerger.mergeSplittedITestSuite(split1, split2);
        assertEquals(4, split1.getDirectModule().numTests());
    }
}
