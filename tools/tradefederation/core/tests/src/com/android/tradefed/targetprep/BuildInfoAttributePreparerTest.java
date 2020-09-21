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
package com.android.tradefed.targetprep;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.OptionSetter;

import junit.framework.TestCase;

import java.util.Map;

/**
 * Unit tests for {@link BuildInfoAttributePreparer}
 */
public class BuildInfoAttributePreparerTest extends TestCase {
    private BuildInfoAttributePreparer mPrep = null;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp() throws Exception {
        super.setUp();
        mPrep = new BuildInfoAttributePreparer();
    }

    public void testSimple() throws Exception {
        final IBuildInfo build = new BuildInfo();

        OptionSetter opt = new OptionSetter(mPrep);
        opt.setOptionValue("build-attribute", "key", "value");
        mPrep.setUp(null, build);

        Map<String, String> map = build.getBuildAttributes();
        assertTrue(map.containsKey("key"));
        assertEquals("value", map.get("key"));
    }
}
