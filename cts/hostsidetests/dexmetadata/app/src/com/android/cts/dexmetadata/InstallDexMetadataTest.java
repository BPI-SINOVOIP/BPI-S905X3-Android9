/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.cts.dexmetadata;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.os.Build.VERSION_CODES;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.ArrayMap;

import com.android.compatibility.common.util.ApiLevelUtil;

import java.nio.file.Files;
import java.nio.file.Paths;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Device-side test for verifying dex metadata installs.
 */
@RunWith(AndroidJUnit4.class)
public class InstallDexMetadataTest {

    private static final String sApkFileExtension = ".apk";
    private static final String sDexMetadataFileExtension = ".dm";

    private static String sSplitAppPackage = "com.android.cts.dexmetadata.splitapp";

    private boolean mShouldRunTest;

    private String mBaseCodePath;
    private String[] mSplitCodePaths;
    private ArrayMap<String, String> mDexMetadataMap;

    /**
     * Decide if we need to run the test.
     */
    @Before
    public void setUp() throws Exception {
        mShouldRunTest = (ApiLevelUtil.isAtLeast(VERSION_CODES.P)
                || ApiLevelUtil.codenameEquals("P"));

        if (mShouldRunTest) {
            ApplicationInfo info = getAppInfo(sSplitAppPackage);
            mBaseCodePath = info.sourceDir;
            mSplitCodePaths = info.splitSourceDirs;
            mDexMetadataMap = buildPackageApkToDexMetadataMap(info);
        }
        Assume.assumeTrue("Skip DexMetadata tests on releases before P.", mShouldRunTest);
    }


    /**
     * Verify that we installed the .dm files for base (no split install).
     */
    @Test
    public void testDmForBase() throws Exception {
        assertNull(mSplitCodePaths);
        assertEquals(1, mDexMetadataMap.size());
        assertTrue(mDexMetadataMap.containsKey(mBaseCodePath));
    }

    /**
     * Verify that we installed the .dm files for base and split.
     */
    @Test
    public void testDmForBaseAndSplit() throws Exception {
        assertNotNull(mSplitCodePaths);

        assertEquals(1 + mSplitCodePaths.length, mDexMetadataMap.size());
        assertTrue(mDexMetadataMap.containsKey(mBaseCodePath));
        for (String split : mSplitCodePaths) {
            assertTrue(mDexMetadataMap.containsKey(split));
        }
    }

    /**
     * Verify that we installed the .dm files for base but not for split.
     */
    @Test
    public void testDmForBaseButNoSplit() throws Exception {
        assertNotNull(mSplitCodePaths);
        assertEquals(1, mDexMetadataMap.size());
        assertTrue(mDexMetadataMap.containsKey(mBaseCodePath));
    }

    /**
     * Verify that we installed the .dm files for split but not for base.
     */
    @Test
    public void testDmForSplitButNoBase() throws Exception {
        assertNotNull(mSplitCodePaths);

        assertEquals(mSplitCodePaths.length, mDexMetadataMap.size());
        for (String split : mSplitCodePaths) {
            assertTrue(mDexMetadataMap.containsKey(split));
        }
    }

    /**
     * Verify that we did not install .dm files.
     */
    @Test
    public void testNoDm() throws Exception {
        assertTrue(mDexMetadataMap.isEmpty());
    }

    private ApplicationInfo getAppInfo(String pkg) {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        final PackageManager pm = context.getPackageManager();
        android.content.pm.PackageInfo packageInfo;
        try {
            packageInfo = pm.getPackageInfo(pkg, 0);
            return packageInfo.applicationInfo;
        } catch (PackageManager.NameNotFoundException e) {
            fail("NameNotFoundException");
        }
        return null;
    }

    private static String getDexMetadataFromApk(String codePath) {
        return codePath.substring(0, codePath.length() - sApkFileExtension.length())
                + sDexMetadataFileExtension;
    }

    private static ArrayMap<String, String> buildPackageApkToDexMetadataMap(ApplicationInfo info) {
        List<String> codePaths = new ArrayList<>();
        codePaths.add(info.sourceDir);
        if (info.splitSourceDirs != null) {
            codePaths.addAll(Arrays.asList(info.splitSourceDirs));
        }
        ArrayMap<String, String> result = new ArrayMap<>();
        for (String codePath : codePaths) {
            String dexMetadataPath = getDexMetadataFromApk(codePath);

            if (Files.exists(Paths.get(dexMetadataPath))) {
                result.put(codePath, dexMetadataPath);
            }
        }

        return result;
    }
}

