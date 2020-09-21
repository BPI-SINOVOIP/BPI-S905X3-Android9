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

import com.android.tradefed.log.LogUtil.CLog;

import java.io.IOException;
import java.io.InputStream;
import java.util.Properties;

/**
 * A class that resolves loading of build related metadata for test suite
 * <p>
 * To properly expose related info, a test suite must include a
 * <code>test-suite-info.properties</code> file in its jar resources
 */
public class TestSuiteInfo {

    /** expected property filename in jar resource */
    private static final String SUITE_INFO_PROPERTY = "/test-suite-info.properties";
    /** suite info keys */
    private static final String BUILD_NUMBER = "build_number";
    private static final String TARGET_ARCH = "target_arch";
    private static final String NAME = "name";
    private static final String FULLNAME = "fullname";
    private static final String VERSION = "version";

    private static TestSuiteInfo sInstance;
    private Properties mTestSuiteInfo;
    private boolean mLoaded = false;

    private TestSuiteInfo() {
        try (InputStream is = TestSuiteInfo.class.getResourceAsStream(SUITE_INFO_PROPERTY)) {
            if (is != null) {
                mTestSuiteInfo = loadSuiteInfo(is);
                mLoaded = true;
            } else {
                CLog.w("Unable to load suite info from jar resource %s, using stub info instead",
                        SUITE_INFO_PROPERTY);
                mTestSuiteInfo = new Properties();
                mTestSuiteInfo.setProperty(BUILD_NUMBER, "[stub build number]");
                mTestSuiteInfo.setProperty(TARGET_ARCH, "[stub target arch]");
                mTestSuiteInfo.setProperty(NAME, "[stub name]");
                mTestSuiteInfo.setProperty(FULLNAME, "[stub fullname]");
                mTestSuiteInfo.setProperty(VERSION, "[stub version]");
            }
        } catch (IOException ioe) {
            // rethrow as runtime exception
            throw new RuntimeException(String.format(
                    "error loading jar resource file \"%s\" for test suite info",
                    SUITE_INFO_PROPERTY));
        }
    }

    /** Performs the actual loading of properties */
    protected Properties loadSuiteInfo(InputStream is) throws IOException {
        Properties p = new Properties();
        p.load(is);
        return p;
    }

    /**
     * Retrieves the singleton instance, which also triggers loading of the related test suite info
     * from embedded resource files
     */
    public static TestSuiteInfo getInstance() {
        if (sInstance == null) {
            sInstance = new TestSuiteInfo();
        }
        return sInstance;
    }

    /** Gets the build number of the test suite */
    public String getBuildNumber() {
        return mTestSuiteInfo.getProperty(BUILD_NUMBER);
    }

    /** Gets the target archs supported by the test suite */
    public String getTargetArch() {
        return mTestSuiteInfo.getProperty(TARGET_ARCH);
    }

    /** Gets the short name of the test suite */
    public String getName() {
        return mTestSuiteInfo.getProperty(NAME);
    }

    /** Gets the full name of the test suite */
    public String getFullName() {
        return mTestSuiteInfo.getProperty(FULLNAME);
    }

    /** Gets the version name of the test suite */
    public String getVersion() {
        return mTestSuiteInfo.getProperty(VERSION);
    }

    /**
     * Retrieves test information keyed with the provided name. Or null if not property associated.
     */
    public String get(String name) {
        return mTestSuiteInfo.getProperty(name);
    }

    /** Returns true if the values were loaded from a property file, false otherwise. */
    public boolean didLoadFromProperties() {
        return mLoaded;
    }
}
