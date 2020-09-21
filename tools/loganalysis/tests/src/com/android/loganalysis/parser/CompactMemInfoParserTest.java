/*
 * Copyright (C) 2014 The Android Open Source Project
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

import com.android.loganalysis.item.CompactMemInfoItem;

import junit.framework.TestCase;

import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;

import java.util.Arrays;
import java.util.List;

public class CompactMemInfoParserTest extends TestCase {

    public void testSingleProcLineWithSwapHasActivities() {
        List<String> input = Arrays.asList("proc,cached,com.google.android.youtube1,2964,19345,1005,a");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(1, item.getPids().size());
        assertEquals("com.google.android.youtube1", item.getName(2964));
        assertEquals(19345, item.getPss(2964));
        assertEquals(1005, item.getSwap(2964));
        assertEquals("cached", item.getType(2964));
        assertEquals(true, item.hasActivities(2964));
    }

    public void testSingleProcLineWithoutSwapHasActivities() {
        List<String> input = Arrays.asList("proc,cached,com.google.android.youtube,2964,19345,a");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(1, item.getPids().size());
        assertEquals("com.google.android.youtube", item.getName(2964));
        assertEquals(19345, item.getPss(2964));
        assertEquals(0, item.getSwap(2964));
        assertEquals("cached", item.getType(2964));
        assertEquals(true, item.hasActivities(2964));
    }


    public void testSingleProcLineWithoutSwapNoActivities() {
        List<String> input = Arrays.asList("proc,cached,com.google.android.youtube,2964,19345,e");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(1, item.getPids().size());
        assertEquals("com.google.android.youtube", item.getName(2964));
        assertEquals(19345, item.getPss(2964));
        assertEquals(0, item.getSwap(2964));
        assertEquals("cached", item.getType(2964));
        assertEquals(false, item.hasActivities(2964));
    }

    public void testSingleLostRamLine() {
        List<String> input = Arrays.asList("lostram,1005");
        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);
        assertEquals(1005, item.getLostRam());
    }

    public void testSingleRamLine() {
        List<String> input = Arrays.asList("ram,2866484,1221694,1112777");
        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);
        assertEquals(1221694, item.getFreeRam());
    }

    public void testSingleZramLine() {
        List<String> input = Arrays.asList("zram,5800,520908,491632");
        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);
        assertEquals(5800, item.getTotalZram());
        assertEquals(491632, item.getFreeSwapZram());
    }

    public void testSingleTuningLine() {
        // With specific configuration
        List<String> input1 = Arrays.asList("tuning,192,512,322560,high-end-gfx");
        CompactMemInfoItem item1 = new CompactMemInfoParser().parse(input1);
        assertEquals(322560, item1.getTuningLevel());
        // Without specific configuration
        List<String> input2 = Arrays.asList("tuning,193,513,322561");
        CompactMemInfoItem item2 = new CompactMemInfoParser().parse(input2);
        assertEquals(322561, item2.getTuningLevel());
    }

    public void testSomeMalformedLines() {
        List<String> input =
                Arrays.asList(
                        "proc,cached,com.google.android.youtube,a,b,e",
                        "proc,cached,com.google.android.youtube,2964,c,e",
                        "proc,cached,com.google.android.youtube,2964,e",
                        "proc,cached,com.google.android.youtube,2964,19345,a,e",
                        "lostram,a,1000",
                        "lostram,1000,a",
                        "ram,123,345",
                        "ram,123,345,abc",
                        "ram,123,345,456,678",
                        "ram,123,345,456,abc",
                        "zram,123,345",
                        "zram,123,345,abc",
                        "zram,123,345,456,678",
                        "zram,123,345,456,abc",
                        "tuning,123,345",
                        "tuning,123,345,abc");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(0, item.getPids().size());
    }

    public void testMultipleLines() {
        List<String> input =
                Arrays.asList(
                        "proc,cached,com.google.android.youtube,2964,19345,123,e",
                        "proc,cached,com.google.android.apps.plus,2877,9604,N/A,e",
                        "proc,cached,com.google.android.apps.magazines,2009,20111,N/A,e",
                        "proc,cached,com.google.android.apps.walletnfcrel,10790,11164,100,e",
                        "proc,cached,com.google.android.incallui,3410,9491,N/A,e",
                        "lostram,1005",
                        "ram,2866484,1221694,1112777",
                        "zram,5800,520908,491632",
                        "tuning,193,513,322561");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(5, item.getPids().size());
        assertEquals("com.google.android.youtube", item.getName(2964));
        assertEquals(19345, item.getPss(2964));
        assertEquals(123, item.getSwap(2964));
        assertEquals("cached", item.getType(2964));
        assertEquals(false, item.hasActivities(2964));

        assertEquals(1005, item.getLostRam());
        assertEquals(1221694, item.getFreeRam());
        assertEquals(5800, item.getTotalZram());
        assertEquals(491632, item.getFreeSwapZram());
        assertEquals(322561, item.getTuningLevel());
    }

    public void testSkipNonProcLines() {
        // Skip lines which does not start with proc

        List<String> input = Arrays.asList(
                "oom,cached,141357",
                "proc,cached,com.google.android.youtube,2964,19345,54321,e",
                "proc,cached,com.google.android.apps.plus,2877,9604,4321,e",
                "proc,cached,com.google.android.apps.magazines,2009,20111,321,e",
                "proc,cached,com.google.android.apps.walletnfcrel,10790,11164,21,e",
                "proc,cached,com.google.android.incallui,3410,9491,1,e",
                "cat,Native,63169");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);

        assertEquals(5, item.getPids().size());
        assertEquals("com.google.android.youtube", item.getName(2964));
        assertEquals(19345, item.getPss(2964));
        assertEquals(54321, item.getSwap(2964));
        assertEquals("cached", item.getType(2964));
        assertEquals(false, item.hasActivities(2964));
    }

    public void testJson() throws JSONException {
        List<String> input =
                Arrays.asList(
                        "oom,cached,141357",
                        "proc,cached,com.google.android.youtube,2964,19345,N/A,e",
                        "proc,cached,com.google.android.apps.plus,2877,9604,50,e",
                        "proc,cached,com.google.android.apps.magazines,2009,20111,100,e",
                        "proc,cached,com.google.android.apps.walletnfcrel,10790,11164,0,e",
                        "proc,cached,com.google.android.incallui,3410,9491,500,e",
                        "lostram,1005",
                        "ram,2866484,1221694,1112777",
                        "zram,5800,520908,491632",
                        "tuning,193,513,322561",
                        "cat,Native,63169");

        CompactMemInfoItem item = new CompactMemInfoParser().parse(input);
        JSONObject json = item.toJson();
        assertNotNull(json);

        JSONArray processes = json.getJSONArray("processes");
        assertEquals(5, processes.length());

        assertEquals(1005, (long)json.get("lostRam"));
        assertEquals(1221694, (long) json.get("freeRam"));
        assertEquals(5800, (long) json.get("totalZram"));
        assertEquals(491632, (long) json.get("freeSwapZram"));
        assertEquals(322561, (long) json.get("tuningLevel"));
    }
}
