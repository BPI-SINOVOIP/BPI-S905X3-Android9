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
package com.android.tradefed.util;

import static org.junit.Assert.*;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;
import java.util.Map;

/** Unit tests for {@link HprofAllocSiteParser}. */
@RunWith(JUnit4.class)
public class HprofAllocSiteParserTest {

    private static final String TEST_STRING =
            "java.util.WeakHashMap$KeySet.iterator\n"
                    + "        java.util.AbstractCollection.toArray(AbstractCollection.java:180)\n"
                    + "        com.sun.imageio.stream.StreamCloser$1.run(StreamCloser.java:70)\n"
                    + "        java.lang.Thread.run(Thread.java:745)\n"
                    + "SITES BEGIN (ordered by live bytes) Mon Jun  5 04:35:20 2017\n"
                    + "          percent          live          alloc'ed  stack class\n"
                    + " rank   self  accum     bytes objs     bytes  objs trace name\n"
                    + "    1 12.24% 12.24%  12441616    1  12441616     1 586322 byte[]\n"
                    + "    2  6.87% 19.12%   6983280 145485  10509264 218943 977169 HeapChaBuffer\n"
                    + "    3  6.12% 25.24%   6220816    1   6220816     1 586500 byte[]\n"
                    + "    4  4.60% 29.84%   4676976 97437   6912816 144017 977186 HeapChaBuffer\n"
                    + "    5  3.44% 33.28%   3491640 145485   5254632 218943 977168 char[]\n"
                    + "    6  2.68% 35.96%   2727808 17547   2831208 18167 303028 char[]\n"
                    + "    7  2.30% 38.26%   2338488 97437   3456408 144017 977185 char[]\n"
                    + "    8  2.26% 40.52%   2294880    6   2294880     6 1069475 char[]\n"
                    + "    9  2.06% 42.59%   2097168    1   2097168     1 1063791 byte[]\n"
                    + "   10  1.82% 44.41%   1853888 33022   1881488 33455 303005 char[]\n"
                    + "   11  1.21% 45.62%   1227208 15226   2047424 33452 303003 char[]\n"
                    + "   12  1.11% 46.72%   1123248    1   1123248     1 1063811 byte[]\n"
                    + "   13  1.04% 47.76%   1056704 33022   1070560 33455 303051 OptionDef\n"
                    + "   14  1.03% 48.79%   1048592    1   1048592     1 970204 byte[]\n"
                    + "   15  1.03% 49.83%   1048592    1   2080976    13 975125 byte[]\n"
                    + "SITES END";

    private HprofAllocSiteParser mParser;

    @Before
    public void setUp() {
        mParser = new HprofAllocSiteParser();
    }

    /** Test that {@link HprofAllocSiteParser#parse(File)} returns correctly a map of results. */
    @Test
    public void testParse() throws Exception {
        File f = FileUtil.createTempFile("hprof", ".test");
        try {
            FileUtil.writeToFile(TEST_STRING, f);
            Map<String, String> results = mParser.parse(f);
            assertFalse(results.isEmpty());
            assertEquals(15, results.size());
            assertEquals("2294880", results.get("Rank8"));
        } finally {
            FileUtil.deleteFile(f);
        }
    }

    /** Test that when the parsing does not find any valid pattern, we return no results. */
    @Test
    public void testParse_invalidContent() throws Exception {
        File f = FileUtil.createTempFile("hprof", ".test");
        try {
            FileUtil.writeToFile("SITES BEGIN\nugh, we are in big trouble", f);
            Map<String, String> results = mParser.parse(f);
            assertTrue(results.isEmpty());
        } finally {
            FileUtil.deleteFile(f);
        }
    }

    /** Assert that if the file passed for parsing is null we return empty results. */
    @Test
    public void testParse_noFile() throws Exception {
        Map<String, String> results = mParser.parse(null);
        assertNotNull(results);
        assertTrue(results.isEmpty());
    }

    /** Assert that if the file passed for parsing does not exists we return empty results. */
    @Test
    public void testParse_fileDoesNotExists() throws Exception {
        Map<String, String> results = mParser.parse(new File("thisdoesnotexistsatall"));
        assertNotNull(results);
        assertTrue(results.isEmpty());
    }
}
