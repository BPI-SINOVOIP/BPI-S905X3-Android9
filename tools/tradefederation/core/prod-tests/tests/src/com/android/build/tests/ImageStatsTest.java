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

package com.android.build.tests;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Unit tests for {@link ImageStats}
 */
@RunWith(JUnit4.class)
public class ImageStatsTest {

    // data obtained from build 4597696, taimen-userdebug_fastbuild_linux
    private static final String TEST_DATA =
            "   164424453  /system/app/WallpapersBReel2017/WallpapersBReel2017.apk\n"
            + "   124082279  /system/app/Chrome/Chrome.apk\n"
            + "    92966112  /system/priv-app/Velvet/Velvet.apk\n"
            + "     1790897  /system/framework/ext.jar\n"
            + "      505436  /system/fonts/NotoSansEgyptianHieroglyphs-Regular.ttf\n"
            + "      500448  /system/bin/ip6tables\n"
            + "      500393  /system/usr/share/zoneinfo/tzdata\n"
            + "      500380  /system/fonts/NotoSansCuneiform-Regular.ttf\n"
            + "      126391  /system/framework/core-oj.jar\n"
            + "      122641  /system/framework/com.quicinc.cne.jar\n";
    private static final Map<String, Long> PARSED_TEST_DATA = new HashMap<>();
    static {
        PARSED_TEST_DATA.put("/system/app/WallpapersBReel2017/WallpapersBReel2017.apk",
                164424453L);
        PARSED_TEST_DATA.put("/system/app/Chrome/Chrome.apk",
                124082279L);
        PARSED_TEST_DATA.put("/system/priv-app/Velvet/Velvet.apk",
                92966112L);
        PARSED_TEST_DATA.put("/system/framework/ext.jar",
                1790897L);
        PARSED_TEST_DATA.put("/system/fonts/NotoSansEgyptianHieroglyphs-Regular.ttf",
                505436L);
        PARSED_TEST_DATA.put("/system/bin/ip6tables",
                500448L);
        PARSED_TEST_DATA.put("/system/usr/share/zoneinfo/tzdata",
                500393L);
        PARSED_TEST_DATA.put("/system/fonts/NotoSansCuneiform-Regular.ttf",
                500380L);
        PARSED_TEST_DATA.put("/system/framework/core-oj.jar",
                126391L);
        PARSED_TEST_DATA.put("/system/framework/com.quicinc.cne.jar",
                122641L);
    }

    private ImageStats mImageStats = null;

    @Before
    public void setup() throws Exception {
        mImageStats = new ImageStats();
    }

    @Test
    public void testParseFileSizes() throws Exception {
        Map<String, Long> ret = mImageStats.parseFileSizes(
                new ByteArrayInputStream(TEST_DATA.getBytes()));
        Assert.assertEquals("parsed test file sizes mismatches expectations",
                PARSED_TEST_DATA, ret);
    }

    /** Verifies that regular matching pattern without capturing group works as expected */
    @Test
    public void testGetAggregationLabel_regular() throws Exception {
        String fileName = "/system/app/WallpapersBReel2017/WallpapersBReel2017.apk";
        Pattern pattern = Pattern.compile("^.+\\.apk$");
        final String label = "foobar";
        Assert.assertEquals("unexpected label transformation output",
                label,  mImageStats.getAggregationLabel(pattern.matcher(fileName), label));
    }

    /** Verifies that matching pattern with corresponding capturing groups works as expected */
    @Test
    public void testGetAggregationLabel_capturingGroups() throws Exception {
        String fileName = "/system/app/WallpapersBReel2017/WallpapersBReel2017.apk";
        Pattern pattern = Pattern.compile("^/system/(.+?)/.+\\.(.+)$");
        final String label = "folder-\\1-ext-\\2";
        Matcher m = pattern.matcher(fileName);
        Assert.assertTrue("this shouldn't fail unless test case isn't written correctly",
                m.matches());
        Assert.assertEquals("unexpected label transformation output",
                "folder-app-ext-apk",
                mImageStats.getAggregationLabel(m, label));
    }

    /**
     * Verifies that matching pattern with capturing groups but partial back references works
     * as expected
     **/
    @Test
    public void testGetAggregationLabel_capturingGroups_partialBackReference() throws Exception {
        String fileName = "/system/app/WallpapersBReel2017/WallpapersBReel2017.apk";
        Pattern pattern = Pattern.compile("^/system/(.+?)/.+\\.(.+)$");
        final String label = "ext-\\2";
        Matcher m = pattern.matcher(fileName);
        Assert.assertTrue("this shouldn't fail unless test case isn't written correctly",
                m.matches());
        Assert.assertEquals("unexpected label transformation output",
                "ext-apk",
                mImageStats.getAggregationLabel(m, label));
    }

    /**
     * Verifies that aggregating the sample input with patterns works as expected
     */
    @Test
    public void testPerformAggregation() throws Exception {
        Map<Pattern, String> mapping = new HashMap<>();
        mapping.put(Pattern.compile("^.+\\.(.+)"), "ext-\\1"); // aggregate by extension
        mapping.put(Pattern.compile("^/system/(.+?)/.+$"), "folder-\\1"); // aggregate by folder
        Map<String, String> ret = mImageStats.performAggregation(PARSED_TEST_DATA, mapping);
        Assert.assertEquals("failed to verify aggregated size for category 'ext-apk'",
                "381472844", ret.get("ext-apk"));
        Assert.assertEquals("failed to verify aggregated size for category 'ext-jar'",
                "2039929", ret.get("ext-jar"));
        Assert.assertEquals("failed to verify aggregated size for category 'ext-ttf'",
                "1005816", ret.get("ext-ttf"));
        Assert.assertEquals("failed to verify aggregated size for category 'uncategorized'",
                "0", ret.get("uncategorized"));
        Assert.assertEquals("failed to verify aggregated size for category 'total'",
                "385519430", ret.get("total"));
    }
}
