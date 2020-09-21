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

package com.android.tools.build.apkzlib.zip;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import com.android.tools.build.apkzlib.utils.ApkZFileTestUtils;
import java.io.File;
import org.junit.Test;

public class OldApkReadTest {

    @Test
    public void testReadOldApk() throws Exception {
        File apkFile = ApkZFileTestUtils.getResource("/testData/packaging/test.apk");
        assertTrue(apkFile.exists());

        try (ZFile zf = new ZFile(apkFile, new ZFileOptions())) {
            StoredEntry classesDex = zf.get("classes.dex");
            assertNotNull(classesDex);
        }
    }
}
