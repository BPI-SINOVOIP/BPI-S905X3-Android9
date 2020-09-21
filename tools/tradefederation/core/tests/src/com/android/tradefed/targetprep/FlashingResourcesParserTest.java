/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.targetprep.FlashingResourcesParser.AndroidInfo;
import com.android.tradefed.targetprep.FlashingResourcesParser.Constraint;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.MultiMap;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link FlashingResourcesParser}.
 */
public class FlashingResourcesParserTest extends TestCase {

    private static class Under3Chars implements Constraint {
        @Override
        public boolean shouldAccept(String item) {
            return item.length() < 3;
        }
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     */
    public void testParseAndroidInfo() throws IOException {
        final String validInfoData = "require board=board1|board2\n" + // valid
                "require version-bootloader=1.0.1\n" + // valid
                "cylon=blah\n" + // valid
                "blah"; // not valid
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));
        AndroidInfo fullInfo = FlashingResourcesParser.parseAndroidInfo(reader, null);
        MultiMap<String, String> result = fullInfo.get(null);

        assertEquals(3, result.size());
        List<String> boards = result.get(FlashingResourcesParser.BOARD_KEY);
        assertEquals(2, boards.size());
        assertEquals("board1", boards.get(0));
        assertEquals("board2", boards.get(1));
        List<String> bootloaders = result.get(FlashingResourcesParser.BOOTLOADER_VERSION_KEY);
        assertEquals("1.0.1", bootloaders.get(0));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     */
    public void testParseAndroidInfo_withConstraint() throws IOException {
        final String validInfoData = "require board=board1|board2\n" +
                "require version-bootloader=1.0.1\n" +
                "require version-baseband=abcde|fg|hijkl|m\n";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));
        Map<String, Constraint> constraintMap = new HashMap<String, Constraint>(1);
        constraintMap.put(FlashingResourcesParser.BASEBAND_VERSION_KEY, new Under3Chars());

        AndroidInfo fullInfo = FlashingResourcesParser.parseAndroidInfo(reader, constraintMap);
        MultiMap<String, String> result = fullInfo.get(null);

        assertEquals(3, result.size());
        List<String> boards = result.get(FlashingResourcesParser.BOARD_KEY);
        assertEquals(2, boards.size());
        assertEquals("board1", boards.get(0));
        assertEquals("board2", boards.get(1));
        List<String> bootloaders = result.get(FlashingResourcesParser.BOOTLOADER_VERSION_KEY);
        assertEquals("1.0.1", bootloaders.get(0));
        List<String> radios = result.get(FlashingResourcesParser.BASEBAND_VERSION_KEY);
        assertEquals(2, radios.size());
        assertEquals("fg", radios.get(0));
        assertEquals("m", radios.get(1));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     *
     * When both 'require board=foo' and 'require product=bar' lines are present, the board line
     * should supercede the product line
     */
    public void testParseAndroidInfo_boardAndProduct() throws Exception {
        final String validInfoData = "require product=alpha|beta\n" +
                "require board=gamma|delta";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));

        IFlashingResourcesParser parser = new FlashingResourcesParser(reader);
        Collection<String> reqBoards = parser.getRequiredBoards();
        assertEquals(2, reqBoards.size());
        assertTrue(reqBoards.contains("gamma"));
        assertTrue(reqBoards.contains("delta"));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     */
    public void testParseAndroidInfo_onlyBoard() throws Exception {
        final String validInfoData = "require board=gamma|delta";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));

        IFlashingResourcesParser parser = new FlashingResourcesParser(reader);
        Collection<String> reqBoards = parser.getRequiredBoards();
        assertEquals(2, reqBoards.size());
        assertTrue(reqBoards.contains("gamma"));
        assertTrue(reqBoards.contains("delta"));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     *
     * When only 'require product=bar' line is present, it should be passed out in lieu of the
     * (missing) board line.
     */
    public void testParseAndroidInfo_onlyProduct() throws Exception {
        final String validInfoData = "require product=alpha|beta";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));

        IFlashingResourcesParser parser = new FlashingResourcesParser(reader);
        Collection<String> reqBoards = parser.getRequiredBoards();
        assertEquals(2, reqBoards.size());
        assertTrue(reqBoards.contains("alpha"));
        assertTrue(reqBoards.contains("beta"));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     *
     * In particular, this tests that the "require-for-product:(productName)" requirement is parsed
     * properly and causes the expected internal state.
     */
    public void testRequireForProduct_internalState() throws Exception {
        final String validInfoData =
                "require product=alpha|beta|gamma\n" +
                "require version-bootloader=1234\n" +
                "require-for-product:gamma " +
                    "version-bootloader=istanbul|constantinople\n";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));

        // Verify parsing for the first line
        AndroidInfo fullInfo = FlashingResourcesParser.parseAndroidInfo(reader, null);
        // 1 for global reqs, 1 for gamma-specific reqs
        assertEquals(2, fullInfo.size());

        MultiMap<String, String> globalReqs = fullInfo.get(null);
        assertEquals(2, globalReqs.size());
        List<String> products = globalReqs.get(FlashingResourcesParser.PRODUCT_KEY);
        assertEquals(3, products.size());
        assertEquals("alpha", products.get(0));
        assertEquals("beta", products.get(1));
        assertEquals("gamma", products.get(2));
        List<String> bootloaders = globalReqs.get(FlashingResourcesParser.BOOTLOADER_VERSION_KEY);
        assertEquals("1234", bootloaders.get(0));

        MultiMap<String, String> gammaReqs = fullInfo.get("gamma");
        assertNotNull(gammaReqs);
        assertEquals(1, gammaReqs.size());
        List<String> gammaBoot = gammaReqs.get("version-bootloader");
        assertEquals(2, gammaBoot.size());
        assertEquals("istanbul", gammaBoot.get(0));
        assertEquals("constantinople", gammaBoot.get(1));
    }

    /**
     * Test that {@link FlashingResourcesParser#parseAndroidInfo(BufferedReader, Map)} parses valid
     * data correctly.
     *
     * In particular, this tests that the "require-for-product:(productName)" requirement is parsed
     * properly and causes the expected internal state.
     */
    public void testRequireForProduct_api() throws Exception {
        final String validInfoData =
                "require product=alpha|beta|gamma\n" +
                "require version-bootloader=1234\n" +
                "require-for-product:gamma " +
                    "version-bootloader=istanbul|constantinople\n";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));
        IFlashingResourcesParser parser = new FlashingResourcesParser(reader);
        assertEquals("1234", parser.getRequiredImageVersion("version-bootloader"));
        assertEquals("1234", parser.getRequiredImageVersion("version-bootloader", null));
        assertEquals("1234", parser.getRequiredImageVersion("version-bootloader", "alpha"));
        assertEquals("istanbul", parser.getRequiredImageVersion("version-bootloader", "gamma"));
    }

    /**
     * Test {@link FlashingResourcesParser#getBuildRequirements(File, Map)} when passed a
     * file that is not a zip.
     */
    public void testGetBuildRequirements_notAZip() throws IOException {
        File badFile = FileUtil.createTempFile("foo", ".zip");
        try {
            new FlashingResourcesParser(badFile);
            fail("TargetSetupError not thrown");
        } catch (TargetSetupError e) {
            // expected
        } finally {
            badFile.delete();
        }
    }

    /**
     * Test {@link FlashingResourcesParser#getRequiredImageVersion(String, String)} to make sure
     * the latest version is returned when multiple valid version exist.
     */
    public void testGetRequiredImageVersion() throws Exception {
        final String validInfoData =
            "require product=alpha|beta|gamma\n" +
            "require version-bootloader=1234|5678|3456\n" +
            "require-for-product:beta " +
                "version-bootloader=ABCD|CDEF|EFGH\n" +
            "require-for-product:gamma " +
                "version-bootloader=efgh|cdef|abcd\n";
        BufferedReader reader = new BufferedReader(new StringReader(validInfoData));
        IFlashingResourcesParser parser = new FlashingResourcesParser(reader);
        assertEquals("5678", parser.getRequiredImageVersion("version-bootloader"));
        assertEquals("5678", parser.getRequiredImageVersion("version-bootloader", null));
        assertEquals("5678", parser.getRequiredImageVersion("version-bootloader", "alpha"));
        assertEquals("EFGH", parser.getRequiredImageVersion("version-bootloader", "beta"));
        assertEquals("efgh", parser.getRequiredImageVersion("version-bootloader", "gamma"));
        assertNull(parser.getRequiredImageVersion("version-baseband"));
    }
}
