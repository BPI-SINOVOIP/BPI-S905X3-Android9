/*
 * Copyright (C) 2016 The Android Open Source Project
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

import java.util.ArrayList;
import java.util.List;

/**
 * Unit tests for {@link ConfigCompletor}
 */
public class ConfigCompletorTest extends TestCase {

    private ConfigCompletor mConfigCompletor;
    private List<?> mCandidates;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        List<String> fakeConfigs = new ArrayList<String>();
        fakeConfigs.add("util/timewaster");
        fakeConfigs.add("util/wifi");
        fakeConfigs.add("util/wipe");
        mConfigCompletor = new ConfigCompletor(fakeConfigs);
        mCandidates = new ArrayList<>();
    }

    /**
     * Test that when the buffer has no start of configuration it returns all the possibilities.
     */
    public void testFullList() {
        final String buffer = "run ";
        mConfigCompletor.complete(buffer, buffer.length(), mCandidates);
        assertEquals(3, mCandidates.size());
    }

    /**
     * Test that when a partial match is happening, it returns only the concerned matching entries.
     */
    public void testPartialMatch() {
        final String buffer = "run  util/wi";
        mConfigCompletor.complete(buffer, buffer.length(), mCandidates);
        assertEquals(2, mCandidates.size());
        assertTrue(mCandidates.contains("util/wifi"));
        assertTrue(mCandidates.contains("util/wipe"));
    }
}
