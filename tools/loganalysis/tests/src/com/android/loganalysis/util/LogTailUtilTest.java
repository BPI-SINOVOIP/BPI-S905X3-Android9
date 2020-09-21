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
package com.android.loganalysis.util;

import junit.framework.TestCase;

/**
 * Unit tests for {@link LogTailUtil}.
 */
public class LogTailUtilTest extends TestCase {

    /**
     * Test that last and id tails of the log are returned correctly.
     */
    public void testGetPreambles() {
        LogTailUtil preambleUtil = new LogTailUtil(500, 3, 3);

        assertEquals("", preambleUtil.getLastTail());
        assertEquals("", preambleUtil.getIdTail(1));

        preambleUtil.addLine(1, "line 1");
        preambleUtil.addLine(2, "line 2");

        assertEquals("line 1\nline 2", preambleUtil.getLastTail());
        assertEquals("line 1", preambleUtil.getIdTail(1));

        preambleUtil.addLine(1, "line 3");
        preambleUtil.addLine(2, "line 4");
        preambleUtil.addLine(1, "line 5");
        preambleUtil.addLine(2, "line 6");
        preambleUtil.addLine(1, "line 7");
        preambleUtil.addLine(2, "line 8");

        assertEquals("line 6\nline 7\nline 8", preambleUtil.getLastTail());
        assertEquals("line 3\nline 5\nline 7", preambleUtil.getIdTail(1));
    }

    /**
     * Test that the ring buffer is limited to a certain size.
     */
    public void testRingBufferSize() {
        LogTailUtil preambleUtil = new LogTailUtil(5, 3, 3);
        preambleUtil.addLine(1, "line 1");
        preambleUtil.addLine(2, "line 2");
        preambleUtil.addLine(2, "line 3");
        preambleUtil.addLine(2, "line 4");
        preambleUtil.addLine(2, "line 5");
        preambleUtil.addLine(2, "line 6");

        // The first line should roll off the end of the buffer.
        assertEquals("", preambleUtil.getIdTail(1));
    }
}
