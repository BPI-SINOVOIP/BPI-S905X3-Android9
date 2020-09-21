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
package com.android.compatibility.common.util;

import static org.junit.Assert.fail;

import com.android.compatibility.common.util.HostInfoStore;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.FileInputStreamSource;
import com.android.tradefed.result.LogDataType;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner.TestLogData;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.StreamUtil;

import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.EOFException;

/**
 * Collect device information from host and write to a JSON file.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public abstract class DeviceInfo extends BaseHostJUnit4Test {

    // Default name of the directory for storing device info files within the result directory
    public static final String RESULT_DIR_NAME = "device-info-files";

    public static final String FILE_SUFFIX = ".deviceinfo.json";

    @Rule
    public TestLogData mLogger = new TestLogData();

    @Test
    public void testCollectDeviceInfo() throws Exception {
        String deviceInfoName = getClass().getSimpleName() + FILE_SUFFIX;
        File jsonFile = null;
        FileInputStreamSource source = null;
        try {
            jsonFile = FileUtil.createTempFile(getClass().getSimpleName(), FILE_SUFFIX);
            try (HostInfoStore store = new HostInfoStore(jsonFile)) {
                store.open();
                collectDeviceInfo(store);
            } finally {
                // If file is empty throw exception so it is not copied to the results.
                if (jsonFile != null && jsonFile.exists() &&
                        jsonFile.length() == 0) {
                    throw new EOFException(String.format("File is empty: %s", deviceInfoName));
                }
            }
            source = new FileInputStreamSource(jsonFile);
            mLogger.addTestLog(deviceInfoName, LogDataType.TEXT, source);
        } catch (Exception e) {
            CLog.e(e);
            fail(String.format("Failed to collect device info (%s): %s",
                    deviceInfoName, e.getMessage()));
        } finally {
            FileUtil.deleteFile(jsonFile);
            StreamUtil.close(source);
        }
    }

    /**
     * Method to collect device information.
     */
    protected abstract void collectDeviceInfo(HostInfoStore store) throws Exception;
}
