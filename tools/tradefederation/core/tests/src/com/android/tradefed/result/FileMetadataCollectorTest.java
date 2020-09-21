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

package com.android.tradefed.result;

import com.android.test.metrics.proto.FileMetadataProto.FileMetadata;
import com.android.test.metrics.proto.FileMetadataProto.LogFile;
import com.android.test.metrics.proto.FileMetadataProto.LogType;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

/** Unit tests for {@link com.android.tradefed.result.FileMetadataCollector}. */
@RunWith(JUnit4.class)
public class FileMetadataCollectorTest {
    private FileMetadataCollector mCollector = null;

    private static final LogDataType DATA_TYPE_BR = LogDataType.BUGREPORT;
    private static final LogDataType DATA_TYPE_LC = LogDataType.LOGCAT;
    private static final LogType LOG_TYPE_BR = LogType.BUGREPORT;
    private static final LogType LOG_TYPE_LC = LogType.LOGCAT;
    private static final String NAME_BR1 = "br1";
    private static final String NAME_BR2 = "br2";
    private static final String NAME_LC1 = "lc1";
    private static final String NAME_LC2 = "lc2";

    @Before
    public void setUp() {
        mCollector = new FileMetadataCollector();
    }

    /** A simple test to ensure expected output is generated for test run with no logs. */
    @Test
    public void testNoElements() throws Exception {
        // Generate actual value
        FileMetadata actual = mCollector.getMetadataContents();
        // Generate expected value
        FileMetadata expected = FileMetadata.newBuilder().build();
        // Assert equality
        Assert.assertEquals(actual, expected);
    }

    /**
     * A simple test to ensure expected output is generated for test run with a single log type with a
     * single log.
     */
    @Test
    public void testSingleTypeSingleElement() throws Exception {
        // Generate actual value
        mCollector.testLogSaved(NAME_BR1, DATA_TYPE_BR, null, null);
        FileMetadata actual = mCollector.getMetadataContents();
        // Generate expected value
        LogFile log = LogFile.newBuilder().setLogType(LOG_TYPE_BR).setName(NAME_BR1).build();
        FileMetadata expected = FileMetadata.newBuilder().addLogFiles(log).build();
        // Assert equality
        Assert.assertEquals(actual, expected);
    }

    /**
     * A simple test to ensure expected output is generated for test run with a single log type, but
     * multiple log files.
     */
    @Test
    public void testSingleTypeMultipleElements() throws Exception {
        // Generate actual value
        mCollector.testLogSaved(NAME_BR1, DATA_TYPE_BR, null, null);
        mCollector.testLogSaved(NAME_BR2, DATA_TYPE_BR, null, null);
        FileMetadata actual = mCollector.getMetadataContents();
        // Generate expected value
        LogFile log1 = LogFile.newBuilder().setLogType(LOG_TYPE_BR).setName(NAME_BR1).build();
        LogFile log2 = LogFile.newBuilder().setLogType(LOG_TYPE_BR).setName(NAME_BR2).build();
        FileMetadata expected = FileMetadata.newBuilder()
                .addLogFiles(log1).addLogFiles(log2).build();
        // Assert equality
        Assert.assertEquals(actual, expected);
    }

    /**
     * A simple test to ensure expected output is generated for test run with multiple log types and
     * multiple log files.
     */
    @Test
    public void testMultipleTypesMultipleElements() throws Exception {
        // Generate actual value
        mCollector.testLogSaved(NAME_BR1, DATA_TYPE_BR, null, null);
        mCollector.testLogSaved(NAME_BR2, DATA_TYPE_BR, null, null);
        mCollector.testLogSaved(NAME_LC1, DATA_TYPE_LC, null, null);
        mCollector.testLogSaved(NAME_LC2, DATA_TYPE_LC, null, null);
        FileMetadata actual = mCollector.getMetadataContents();
        // Generate expected value
        LogFile log1 = LogFile.newBuilder().setLogType(LOG_TYPE_BR).setName(NAME_BR1).build();
        LogFile log2 = LogFile.newBuilder().setLogType(LOG_TYPE_BR).setName(NAME_BR2).build();
        LogFile log3 = LogFile.newBuilder().setLogType(LOG_TYPE_LC).setName(NAME_LC1).build();
        LogFile log4 = LogFile.newBuilder().setLogType(LOG_TYPE_LC).setName(NAME_LC2).build();
        FileMetadata expected = FileMetadata.newBuilder()
                .addLogFiles(log1).addLogFiles(log2).addLogFiles(log3).addLogFiles(log4).build();
        // Assert equality
        Assert.assertEquals(actual, expected);
    }
}
