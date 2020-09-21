/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.loganalysis.item.TopItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link ProcrankParser}
 */
public class TopParserTest extends TestCase {

    /**
     * Test that the output of the top command is parsed.
     */
    public void testTopParser() {
        List<String> inputBlock = Arrays.asList(
                "User 20%, System 20%, IOW 5%, IRQ 3%",
                "User 150 + Nice 50 + Sys 200 + Idle 510 + IOW 60 + IRQ 5 + SIRQ 25 = 1000",
                "",
                "  PID   TID PR CPU% S     VSS     RSS PCY UID      Thread          Proc",
                " 4474  4474  0   2% R   1420K    768K     shell    top             top");

        TopItem item = new TopParser().parse(inputBlock);

        assertEquals(150, item.getUser());
        assertEquals(50, item.getNice());
        assertEquals(200, item.getSystem());
        assertEquals(510, item.getIdle());
        assertEquals(60, item.getIow());
        assertEquals(5, item.getIrq());
        assertEquals(25, item.getSirq());
        assertEquals(1000, item.getTotal());
        assertEquals(ArrayUtil.join("\n", inputBlock), item.getText());
    }

    /**
     * Test that the last output is stored.
     */
    public void testLastTop() {
        List<String> inputBlock = Arrays.asList(
                "User 0 + Nice 0 + Sys 0 + Idle 1000 + IOW 0 + IRQ 0 + SIRQ 0 = 1000",
                "User 0 + Nice 0 + Sys 0 + Idle 1000 + IOW 0 + IRQ 0 + SIRQ 0 = 1000",
                "User 150 + Nice 50 + Sys 200 + Idle 510 + IOW 60 + IRQ 5 + SIRQ 25 = 1000");

        TopItem item = new TopParser().parse(inputBlock);

        assertEquals(150, item.getUser());
        assertEquals(50, item.getNice());
        assertEquals(200, item.getSystem());
        assertEquals(510, item.getIdle());
        assertEquals(60, item.getIow());
        assertEquals(5, item.getIrq());
        assertEquals(25, item.getSirq());
        assertEquals(1000, item.getTotal());
    }

    /**
     * Test that an empty input returns {@code null}.
     */
    public void testEmptyInput() {
        TopItem item = new TopParser().parse(Arrays.asList(""));
        assertNull(item);
    }
}
