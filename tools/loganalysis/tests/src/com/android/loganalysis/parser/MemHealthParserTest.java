/*
 * Copyright (C) 2015 The Android Open Source Project
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

import com.android.loganalysis.item.MemoryHealthItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;
import java.util.Map;

/**
 * Tests for memory health parser.
 */
public class MemHealthParserTest extends TestCase {
    public void testOneForegroundProc() {
        List<String> lines = Arrays.asList("Foreground",
                "com.google.android.gm",
                "Average Native Heap: 10910",
                "Average Dalvik Heap: 8011",
                "Average PSS: 90454",
                "Peak Native Heap: 11136",
                "Peak Dalvik Heap: 9812",
                "Peak PSS: 95161",
                "Average Summary Java Heap: 8223",
                "Average Summary Native Heap: 3852",
                "Average Summary Code: 1804",
                "Average Summary Stack: 246",
                "Average Summary Graphics: 0",
                "Average Summary Other: 855",
                "Average Summary System: 9151",
                "Average Summary Overall Pss: 24135",
                "Count 528"
                );

        MemoryHealthItem item = new MemoryHealthParser().parse(lines);
        Map<String, Map<String, Long>> processes = item.getForeground();
        assertNotNull(processes);
        assertNotNull(processes.get("com.google.android.gm"));
        Map<String, Long> process = processes.get("com.google.android.gm");
        assertEquals(10910, process.get("native_avg").longValue());
        assertEquals(8011, process.get("dalvik_avg").longValue());
        assertEquals(90454, process.get("pss_avg").longValue());
        assertEquals(11136, process.get("native_peak").longValue());
        assertEquals(9812, process.get("dalvik_peak").longValue());
        assertEquals(95161, process.get("pss_peak").longValue());
        assertEquals(8223, process.get("summary_java_heap_avg").longValue());
        assertEquals(3852, process.get("summary_native_heap_avg").longValue());
        assertEquals(1804, process.get("summary_code_avg").longValue());
        assertEquals(246, process.get("summary_stack_avg").longValue());
        assertEquals(0, process.get("summary_graphics_avg").longValue());
        assertEquals(855, process.get("summary_other_avg").longValue());
        assertEquals(9151, process.get("summary_system_avg").longValue());
        assertEquals(24135, process.get("summary_overall_pss_avg").longValue());
    }

    public void testTwoForegroundProc() {
        List<String> lines = Arrays.asList("Foreground",
                "com.google.android.gm",
                "Average Native Heap: 10910",
                "Average Dalvik Heap: 8011",
                "Average PSS: 90454",
                "Peak Native Heap: 11136",
                "Peak Dalvik Heap: 9812",
                "Peak PSS: 95161",
                "Average Summary Java Heap: 8223",
                "Average Summary Native Heap: 3852",
                "Average Summary Code: 1804",
                "Average Summary Stack: 246",
                "Average Summary Graphics: 0",
                "Average Summary Other: 855",
                "Average Summary System: 9151",
                "Average Summary Overall Pss: 24135",
                "Count 528",
                "com.google.android.music",
                "Average Native Heap: 1",
                "Average Dalvik Heap: 2",
                "Average PSS: 3",
                "Peak Native Heap: 4",
                "Peak Dalvik Heap: 5",
                "Peak PSS: 6",
                "Average Summary Java Heap: 7",
                "Average Summary Native Heap: 8",
                "Average Summary Code: 9",
                "Average Summary Stack: 10",
                "Average Summary Graphics: 11",
                "Average Summary Other: 12",
                "Average Summary System: 13",
                "Average Summary Overall Pss: 14",
                "Count 7"
                );

        MemoryHealthItem item = new MemoryHealthParser().parse(lines);
        Map<String, Map<String, Long>> processes = item.getForeground();
        assertEquals(2, processes.size());
        Map<String, Long> process = processes.get("com.google.android.music");
        assertEquals(1, process.get("native_avg").longValue());
        assertEquals(2, process.get("dalvik_avg").longValue());
        assertEquals(3, process.get("pss_avg").longValue());
        assertEquals(4, process.get("native_peak").longValue());
        assertEquals(5, process.get("dalvik_peak").longValue());
        assertEquals(6, process.get("pss_peak").longValue());
        assertEquals(7, process.get("summary_java_heap_avg").longValue());
        assertEquals(8, process.get("summary_native_heap_avg").longValue());
        assertEquals(9, process.get("summary_code_avg").longValue());
        assertEquals(10, process.get("summary_stack_avg").longValue());
        assertEquals(11, process.get("summary_graphics_avg").longValue());
        assertEquals(12, process.get("summary_other_avg").longValue());
        assertEquals(13, process.get("summary_system_avg").longValue());
        assertEquals(14, process.get("summary_overall_pss_avg").longValue());
    }

    public void testForegroundBackgroundProc() {
        List<String> lines = Arrays.asList("Foreground",
                "com.google.android.gm",
                "Average Native Heap: 10910",
                "Average Dalvik Heap: 8011",
                "Average PSS: 90454",
                "Peak Native Heap: 11136",
                "Peak Dalvik Heap: 9812",
                "Peak PSS: 95161",
                "Average Summary Java Heap: 8223",
                "Average Summary Native Heap: 3852",
                "Average Summary Code: 1804",
                "Average Summary Stack: 246",
                "Average Summary Graphics: 0",
                "Average Summary Other: 855",
                "Average Summary System: 9151",
                "Average Summary Overall Pss: 24135",
                "Count 528",
                "Background",
                "com.google.android.music",
                "Average Native Heap: 1",
                "Average Dalvik Heap: 2",
                "Average PSS: 3",
                "Peak Native Heap: 4",
                "Peak Dalvik Heap: 5",
                "Peak PSS: 6",
                "Average Summary Java Heap: 7",
                "Average Summary Native Heap: 8",
                "Average Summary Code: 9",
                "Average Summary Stack: 10",
                "Average Summary Graphics: 11",
                "Average Summary Other: 12",
                "Average Summary System: 13",
                "Average Summary Overall Pss: 14",
                "Count 7"
                );

        MemoryHealthItem item = new MemoryHealthParser().parse(lines);
        Map<String, Map<String, Long>> processes = item.getForeground();
        assertEquals(1, processes.size());
        assertEquals(1, item.getBackground().size());
        Map<String, Long> process = item.getBackground().get("com.google.android.music");
        assertEquals(1, process.get("native_avg").longValue());
        assertEquals(2, process.get("dalvik_avg").longValue());
        assertEquals(3, process.get("pss_avg").longValue());
        assertEquals(4, process.get("native_peak").longValue());
        assertEquals(5, process.get("dalvik_peak").longValue());
        assertEquals(6, process.get("pss_peak").longValue());
        assertEquals(7, process.get("summary_java_heap_avg").longValue());
        assertEquals(8, process.get("summary_native_heap_avg").longValue());
        assertEquals(9, process.get("summary_code_avg").longValue());
        assertEquals(10, process.get("summary_stack_avg").longValue());
        assertEquals(11, process.get("summary_graphics_avg").longValue());
        assertEquals(12, process.get("summary_other_avg").longValue());
        assertEquals(13, process.get("summary_system_avg").longValue());
        assertEquals(14, process.get("summary_overall_pss_avg").longValue());
    }
}
