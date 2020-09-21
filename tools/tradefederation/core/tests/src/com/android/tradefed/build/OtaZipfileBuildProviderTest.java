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

package com.android.tradefed.build;

import junit.framework.TestCase;

import java.io.File;

/**
 * Tests for {@link OtaZipfileBuildProvider}
 */
public class OtaZipfileBuildProviderTest extends TestCase {
    private static final String SOME_BAD_PATH = "/some/inexistent/path.zip";
    private static final String BUILD_ID = "123456";
    private static final String MOCK_BUILD_PROP =
        "some.fake.property=some_value\n" +
        "ro.build.version.incremental=" + BUILD_ID + "\n" +
        "some.other.fake.property=some_value";

    private static class OtaZipfileBuildProviderUnderTest extends OtaZipfileBuildProvider {
        @Override
        String getOtaPath() {
            return SOME_BAD_PATH;
        }
    }

    public void testGetBuild_Success() throws BuildRetrievalError {
        OtaZipfileBuildProviderUnderTest providerAllGood = new OtaZipfileBuildProviderUnderTest() {
            @Override
            String getBuildPropContents() {
                return MOCK_BUILD_PROP;
            }
        };
        IBuildInfo buildInfo = providerAllGood.getBuild();
        assertEquals(BUILD_ID, buildInfo.getBuildId());
        assertTrue(buildInfo instanceof IDeviceBuildInfo);
        assertEquals(new File(SOME_BAD_PATH), ((IDeviceBuildInfo) buildInfo).getOtaPackageFile());
    }

    public void testGetBuild_NoPath() throws BuildRetrievalError {
        OtaZipfileBuildProvider providerNoPath = new OtaZipfileBuildProvider();
        try {
            providerNoPath.getBuild();
            fail("Expected to get an IllegalArgumentException when no path was passed");
        } catch (IllegalArgumentException e) {
            assertTrue(true);
        }
    }

    public void testGetBuild_BadPath() {
        OtaZipfileBuildProviderUnderTest providerBadPath = new OtaZipfileBuildProviderUnderTest();
        try {
            providerBadPath.getBuild();
            fail("Expected to get an BuildRetrievalError when a bad path was passed");
        } catch (BuildRetrievalError e) {
            assertTrue(true);
        }
    }
}
