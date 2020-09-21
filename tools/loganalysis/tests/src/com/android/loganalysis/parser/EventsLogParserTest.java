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

package com.android.loganalysis.parser;

import com.android.loganalysis.item.LatencyItem;
import com.android.loganalysis.item.TransitionDelayItem;

import junit.framework.TestCase;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileWriter;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link EventsLogParser}.
 */
public class EventsLogParserTest extends TestCase {

    private File mTempFile = null;

    @Override
    protected void tearDown() throws Exception {
        mTempFile.delete();
    }

    /**
     * Test for empty events logs passed to the transition delay parser
     */
    public void testEmptyEventsLog() throws IOException {
        List<String> lines = Arrays.asList("");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should be empty", 0,transitionItems.size());
    }

    /**
     * Test for no transition delay info in the events log
     */
    public void testNoTransitionDelayInfo() throws IOException {
        List<String> lines = Arrays
                .asList(
                        "08-25 12:56:15.850  1152  8968 I am_focused_stack: [0,0,1,appDied setFocusedActivity]",
                        "08-25 12:56:15.850  1152  8968 I wm_task_moved: [6,1,1]",
                        "08-25 12:56:15.852  1152  8968 I am_focused_activity: [0,com.google.android.apps.nexuslauncher/.NexusLauncherActivity,appDied]",
                        "08-25 12:56:15.852  1152  8968 I wm_task_removed: [27,removeTask]",
                        "08-25 12:56:15.852  1152  8968 I wm_stack_removed: 1");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should be empty", 0,
                transitionItems.size());
    }

    /**
     * Test for Cold launch transition delay and starting window delay info
     */
    public void testValidColdTransitionDelay() throws IOException {
        List<String> lines = Arrays
                .asList("09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,50,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,904,com.google.android.apps.nexuslauncher,905,0,945,41]",
                        "09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,50,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,905,0,945,41]");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Startinng Window Delay items list should have two item", 2,
                transitionItems.size());
        assertEquals("Component name not parsed correctly",
                "com.google.android.calculator/com.android.calculator2.Calculator",
                transitionItems.get(0).getComponentName());
        assertEquals("Transition delay is not parsed correctly", Long.valueOf(51),
                transitionItems.get(0).getTransitionDelay());
        assertEquals("Starting window delay is not parsed correctly", Long.valueOf(50),
                transitionItems.get(0).getStartingWindowDelay());
        assertEquals("Date and time is not parsed correctly", "09-18 23:56:19.376",
                transitionItems.get(0).getDateTime());
    }

    /**
     * Test for Hot launch transition delay and starting window delay info
     */
    public void testValidHotTransitionDelay() throws IOException {
        List<String> lines = Arrays
                .asList("09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,50,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,904,com.google.android.apps.nexuslauncher,905,0]",
                        "09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,50,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,905,0]",
                        "09-19 02:26:30.182  1143  1196 I sysui_multi_action: [319,87,322,75,325,212,757,761,758,9,759,2,806,com.google.android.apps.nexuslauncher,871,com.google.android.apps.nexuslauncher.NexusLauncherActivity,904,com.google.android.apps.nexuslauncher,905,0]",
                        "09-19 02:26:30.182  1143  1196 I sysui_multi_action: [319,87,322,75,325,212,757,761,758,9,759,2,806,com.google.android.apps.nexuslauncher,871,com.google.android.apps.nexuslauncher.NexusLauncherActivity,905,0]");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should have four item", 4,
                transitionItems.size());
        assertEquals("Component name not parsed correctly",
                "com.google.android.calculator/com.android.calculator2.Calculator",
                transitionItems.get(0).getComponentName());
        assertEquals("Transition delay is not parsed correctly", Long.valueOf(51),
                transitionItems.get(0).getTransitionDelay());
        assertEquals("Date is not parsed correctly", "09-18 23:56:19.376",
                transitionItems.get(0).getDateTime());
    }

    /**
     * Test for same app transition delay items order after parsing from the events log
     */
    public void testTransitionDelayOrder() throws IOException {
        List<String> lines = Arrays
                .asList("09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,59,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,904,com.google.android.apps.nexuslauncher,905,0,945,41]",
                        "09-18 23:59:18.380  1140  1221 I sysui_multi_action: [319,55,321,65,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,905,0,945,41]");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should have two items", 2,
                transitionItems.size());
        assertEquals("Transition delay for the first item is not set correct", Long.valueOf(59),
                transitionItems.get(0).getStartingWindowDelay());
        assertEquals("Transition delay for the second item is not set correct", Long.valueOf(65),
                transitionItems.get(1).getStartingWindowDelay());
    }

    /**
     * Test for two different different apps transition delay items
     */
    public void testDifferentAppTransitionDelay() throws IOException {
        List<String> lines = Arrays
                .asList("09-18 23:56:19.376  1140  1221 I sysui_multi_action: [319,51,321,50,322,190,325,670,757,761,758,7,759,1,806,com.google.android.calculator,871,com.android.calculator2.Calculator,904,com.google.android.apps.nexuslauncher,905,0]",
                        "09-19 02:26:30.182  1143  1196 I sysui_multi_action: [319,87,322,75,325,212,757,761,758,9,759,2,806,com.google.android.apps.nexuslauncher,871,com.google.android.apps.nexuslauncher.NexusLauncherActivity,904,com.google.android.apps.nexuslauncher,905,0]");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should have two items", 2,
                transitionItems.size());
        assertEquals("Calculator is not the first transition delay item",
                "com.google.android.calculator/com.android.calculator2.Calculator",
                transitionItems.get(0).getComponentName());
        assertEquals("Maps is not the second transition delay item",
                "com.google.android.apps.nexuslauncher/"
                + "com.google.android.apps.nexuslauncher.NexusLauncherActivity",
                transitionItems.get(1).getComponentName());
    }

    /**
     * Test for invalid transition delay items pattern having different code.
     */
    public void testInvalidTransitionPattern() throws IOException {
        List<String> lines = Arrays
                .asList("01-02 08:11:58.691   934   986 I sysui_multi_action: a[319,48,322,82,325,84088,757,761,758,9,759,4,807,com.google.android.calculator,871,com.android.calculator2.Calculator,905,0]",
                        "01-02 08:12:03.639   934   970 I sysui_multi_action: [757,803,799,window_time_0,802,5]",
                        "01-02 08:12:10.849   934   986 I sysui_multi_action: 319,42,321,59,322,208,325,84100,757,761,758,9,759,4,806,com.google.android.apps.maps,871,com.google.android.maps.MapsActivity,905,0]",
                        "01-02 08:12:16.895  1446  1446 I sysui_multi_action: [757,803,799,overview_trigger_nav_btn,802,1]",
                        "01-02 08:12:16.895  1446  1446 I sysui_multi_action: [757,803,799,overview_source_app,802,1]",
                        "01-02 08:12:16.895  1446  1446 I sysui_multi_action: [757,804,799,overview_source_app_index,801,8,802,1]");
        List<TransitionDelayItem> transitionItems = (new EventsLogParser()).
                parseTransitionDelayInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Transition Delay items list should be empty", 0,
                transitionItems.size());
    }

    /**
     * Test for valid latency item
     */
    public void testValidLatencyInfo() throws IOException {
        List<String> lines = Arrays
                .asList("08-25 13:01:19.412  1152  9031 I am_restart_activity: [com.google.android.gm/.ConversationListActivityGmail,0,85290699,38]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [321,85]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [320,1]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [319,85]",
                        "08-25 12:56:15.850  1152  8968 I am_focused_stack: [0,0,1,appDied setFocusedActivity]",
                        "09-19 11:53:16.893  1080  1160 I sysui_latency: [1,50]");
        List<LatencyItem> latencyItems = (new EventsLogParser()).
                parseLatencyInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("One latency item should present in the list", 1, latencyItems.size());
        assertEquals("Action Id is not correct", 1, latencyItems.get(0).getActionId());
        assertEquals("Delay is not correct", 50L, latencyItems.get(0).getDelay());
    }

    /**
     * Test for empty delay info
     */
    public void testInvalidLatencyInfo() throws IOException {
        List<String> lines = Arrays
                .asList("08-25 13:01:19.412  1152  9031 I am_restart_activity: [com.google.android.gm/.ConversationListActivityGmail,0,85290699,38]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [321,85]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [320,1]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [319,85]",
                        "08-25 12:56:15.850  1152  8968 I am_focused_stack: [0,0,1,appDied setFocusedActivity]",
                        "09-19 11:53:16.893  1080  1160 I sysui_latency: [1]");
        List<LatencyItem> latencyItems = (new EventsLogParser()).
                parseLatencyInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Latency items list should be empty", 0, latencyItems.size());
    }

    /**
     * Test for empty latency info
     */
    public void testEmptyLatencyInfo() throws IOException {
        List<String> lines = Arrays
                .asList("08-25 13:01:19.412  1152  9031 I am_restart_activity: [com.google.android.gm/.ConversationListActivityGmail,0,85290699,38]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [321,85]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [320,1]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [319,85]",
                        "08-25 12:56:15.850  1152  8968 I am_focused_stack: [0,0,1,appDied setFocusedActivity]",
                        "09-19 11:53:16.893  1080  1160 I sysui_latency: []");
        List<LatencyItem> latencyItems = (new EventsLogParser()).
                parseLatencyInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Latency items list should be empty", 0, latencyItems.size());
    }


    /**
     * Test for order of the latency items
     */
    public void testLatencyInfoOrder() throws IOException {
        List<String> lines = Arrays
                .asList("09-19 11:53:16.893  1080  1160 I sysui_latency: [1,50]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [321,85]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [320,1]",
                        "08-25 13:01:19.437  1152  1226 I sysui_action: [319,85]",
                        "08-25 12:56:15.850  1152  8968 I am_focused_stack: [0,0,1,appDied setFocusedActivity]",
                        "09-19 11:53:16.893  1080  1160 I sysui_latency: [2,100]");
        List<LatencyItem> latencyItems = (new EventsLogParser()).
                parseLatencyInfo(readInputBuffer(getTempFile(lines)));
        assertEquals("Latency list should have 2 items", 2, latencyItems.size());
        assertEquals("First latency id is not 1", 1, latencyItems.get(0).getActionId());
        assertEquals("Second latency id is not 2", 2, latencyItems.get(1).getActionId());
    }

    /**
     * Write list of strings to file and use it for testing.
     */
    public File getTempFile(List<String> sampleEventsLogs) throws IOException {
        mTempFile = File.createTempFile("events_logcat", ".txt");
        BufferedWriter out = new BufferedWriter(new FileWriter(mTempFile));
        for (String line : sampleEventsLogs) {
            out.write(line);
            out.newLine();
        }
        out.close();
        return mTempFile;
    }

    /**
     * Reader to read the input from the given temp file
     */
    public BufferedReader readInputBuffer(File tempFile) throws IOException {
        return (new BufferedReader(new InputStreamReader(new FileInputStream(tempFile))));
    }

}