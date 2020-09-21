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
package com.android.loganalysis.parser;

import com.android.loganalysis.item.SystemPropsItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link SystemPropsParser}
 */
public class SystemPropsParserTest extends TestCase {

    /**
     * Test that normal input is parsed.
     */
    public void testSimpleParse() {
        List<String> inputBlock = Arrays.asList(
                "[dalvik.vm.dexopt-flags]: [m=y]",
                "[dalvik.vm.heapgrowthlimit]: [48m]",
                "[dalvik.vm.heapsize]: [256m]",
                "[gsm.version.ril-impl]: [android moto-ril-multimode 1.0]");

        SystemPropsItem map = new SystemPropsParser().parse(inputBlock);

        assertEquals(4, map.size());
        assertEquals("m=y", map.get("dalvik.vm.dexopt-flags"));
        assertEquals("48m", map.get("dalvik.vm.heapgrowthlimit"));
        assertEquals("256m", map.get("dalvik.vm.heapsize"));
        assertEquals("android moto-ril-multimode 1.0", map.get("gsm.version.ril-impl"));
    }

    /**
     * Make sure that a parse error on one line doesn't prevent the rest of the lines from being
     * parsed
     */
    public void testParseError() {
        List<String> inputBlock = Arrays.asList(
                "[dalvik.vm.dexopt-flags]: [m=y]",
                "[ends with newline]: [yup",
                "]",
                "[dalvik.vm.heapsize]: [256m]");

        SystemPropsItem map = new SystemPropsParser().parse(inputBlock);

        assertEquals(2, map.size());
        assertEquals("m=y", map.get("dalvik.vm.dexopt-flags"));
        assertEquals("256m", map.get("dalvik.vm.heapsize"));
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        SystemPropsItem item = new SystemPropsParser().parse(Arrays.asList(""));
        assertNull(item);
    }
}

