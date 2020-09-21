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

import com.android.loganalysis.item.DmesgActionInfoItem;
import com.android.loganalysis.item.DmesgItem;
import com.android.loganalysis.item.DmesgServiceInfoItem;
import com.android.loganalysis.item.DmesgStageInfoItem;
import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import junit.framework.TestCase;

/**
 * Unit tests for {@link DmesgParser}.
 */
public class DmesgParserTest extends TestCase {

    private static final String BOOT_ANIMATION = "bootanim";
    private static final String NETD = "netd";
    private static final String[] LINES =
            new String[] {
                "[    3.786943] ueventd: Coldboot took 0.701291 seconds",
                "[   22.962730] init: starting service 'bootanim'...",
                "[   23.252321] init: starting service 'netd'...",
                "[   29.331069] ipa-wan ipa_wwan_ioctl:1428 dev(rmnet_data0) register to IPA",
                "[   32.182592] ueventd: fixup /sys/devices/virtual/input/poll_delay 0 1004 660",
                "[   35.642666] SELinux: initialized (dev fuse, type fuse), uses genfs_contexts",
                "[   39.855818] init: Service 'bootanim' (pid 588) exited with status 0",
                "[   41.665818] init: init first stage started!",
                "[   44.942872] init: processing action (early-init) from (/init.rc:13)",
                "[   47.233446] init: processing action (set_mmap_rnd_bits) from (<Builtin Action>:0)",
                "[   47.240083] init: processing action (set_kptr_restrict) from (<Builtin Action>:0)",
                "[   47.245778] init: processing action (keychord_init) from (<Builtin Action>:0)",
                "[   52.361049] init: processing action (persist.sys.usb.config=* boot) from (<Builtin Action>:0)",
                "[   52.361108] init: processing action (enable_property_trigger) from (<Builtin Action>:0)",
                "[   52.361313] init: processing action (security.perf_harden=1) from (/init.rc:677)",
                "[   52.361495] init: processing action (ro.debuggable=1) from (/init.rc:700)",
                "[   58.298293] init: processing action (sys.boot_completed=1)",
                "[   59.331069] ipa-wan ipa_wwan_ioctl:1428 dev(rmnet_data0) register to IPA",
                "[   62.182592] ueventd: fixup /sys/devices/virtual/input/poll_delay 0 1004 660",
                "[   65.642666] SELinux: initialized (dev fuse, type fuse), uses genfs_contexts",
                "[   69.855818] init: Service 'bootanim' (pid 588) exited with status 0"
            };

    private static final Map<String, DmesgServiceInfoItem> EXPECTED_SERVICE_INFO_ITEMS =
            getExpectedServiceInfoItems();

    private static final List<DmesgStageInfoItem> EXPECTED_STAGE_INFO_ITEMS =
            getExpectedStageInfoItems();

    private static final List<DmesgActionInfoItem> EXPECTED_ACTION_INFO_ITEMS =
            getExpectedActionInfoItems();

    /**
     * Test for empty dmesg logs passed to the DmesgParser
     */
    public void testEmptyDmesgLog() throws IOException {
        String[] lines = new String[] {""};
        try (BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(
                new ByteArrayInputStream(String.join("\n", lines).getBytes())))) {
            DmesgParser dmesgParser = new DmesgParser();
            dmesgParser.parseInfo(bufferedReader);
            assertEquals("Service info items list should be empty", 0,
                    dmesgParser.getServiceInfoItems().size());
        }
    }

    /**
     * Test for complete dmesg logs passed as list of strings
     */
    public void testCompleteDmesgLog_passedAsList() {

        DmesgParser dmesgParser = new DmesgParser();
        DmesgItem actualDmesgItem = dmesgParser.parse(Arrays.asList(LINES));

        assertEquals("Service info items list size should be 2", 2,
                dmesgParser.getServiceInfoItems().size());
        assertEquals("Stage info items list size should be 2",2,
                dmesgParser.getStageInfoItems().size());
        assertEquals("Action info items list size should be 9",9,
                dmesgParser.getActionInfoItems().size());

        assertEquals(EXPECTED_SERVICE_INFO_ITEMS, actualDmesgItem.getServiceInfoItems());
        assertEquals(EXPECTED_STAGE_INFO_ITEMS, actualDmesgItem.getStageInfoItems());
        assertEquals(EXPECTED_ACTION_INFO_ITEMS, actualDmesgItem.getActionInfoItems());
    }

    /**
     * Test for complete dmesg logs passed as buffered input
     */
    public void testCompleteDmesgLog_passedAsBufferedInput() throws IOException {
        try (BufferedReader bufferedReader = new BufferedReader(new InputStreamReader(
                new ByteArrayInputStream(String.join("\n", LINES).getBytes())))) {
            DmesgParser dmesgParser = new DmesgParser();
            dmesgParser.parseInfo(bufferedReader);
            assertEquals("Service info items list size should be 2", 2,
                    dmesgParser.getServiceInfoItems().size());
            assertEquals("Stage info items list size should be 2", 2,
                    dmesgParser.getStageInfoItems().size());
            assertEquals("Action info items list size should be 9",9,
                    dmesgParser.getActionInfoItems().size());
        }
    }

    /**
     * Test service which logs both the start and end time
     */
    public void testCompleteServiceInfo() {
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : LINES) {
            dmesgParser.parseServiceInfo(line);
        }

        assertEquals("There should be two service infos", 2,
                dmesgParser.getServiceInfoItems().size());
        assertEquals(EXPECTED_SERVICE_INFO_ITEMS, dmesgParser.getServiceInfoItems());
    }

    /**
     * Test service which logs only the start time
     */
    public void testStartServiceInfo() {
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : LINES) {
            dmesgParser.parseServiceInfo(line);
        }
        List<DmesgServiceInfoItem> serviceInfoItems = new ArrayList<>(
                dmesgParser.getServiceInfoItems().values());
        assertEquals("There should be exactly two service infos", 2, serviceInfoItems.size());
        assertEquals("Service name is not bootanim", BOOT_ANIMATION,
                serviceInfoItems.get(0).getServiceName());
        assertEquals("Service name is not netd", NETD, serviceInfoItems.get(1).getServiceName());
    }

    /**
     * Test multiple service info parsed correctly and stored in the same order logged in
     * the file.
     */
    public void testMultipleServiceInfo() {
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : LINES) {
            dmesgParser.parseServiceInfo(line);
        }
        assertEquals("There should be exactly two service info", 2,
                dmesgParser.getServiceInfoItems().size());
        assertEquals(EXPECTED_SERVICE_INFO_ITEMS, dmesgParser.getServiceInfoItems());
    }

    /**
     * Test invalid patterns on the start and exit service logs
     */
    public void testInvalidServiceLogs() {
        // Added space at the end of the start and exit of service logs to make it invalid
        String[] lines = new String[] {
                "[   22.962730] init: starting service 'bootanim'...  ",
                "[   23.252321] init: starting service 'netd'...  ",
                "[   29.331069] ipa-wan ipa_wwan_ioctl:1428 dev(rmnet_data0) register to IPA",
                "[   32.182592] ueventd: fixup /sys/devices/virtual/input/poll_delay 0 1004 660",
                "[   35.642666] SELinux: initialized (dev fuse, type fuse), uses genfs_contexts",
                "[   39.855818] init: Service 'bootanim' (pid 588) exited with status 0  "};
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : lines) {
            dmesgParser.parseServiceInfo(line);
        }
        List<DmesgServiceInfoItem> serviceInfoItems = new ArrayList<>(
                dmesgParser.getServiceInfoItems().values());
        assertEquals("No service info should be available", 0, serviceInfoItems.size());
    }

    /**
     * Test init stages' start time logs
     */
    public void testCompleteStageInfo() {
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : LINES) {
            dmesgParser.parseStageInfo(line);
        }
        List<DmesgStageInfoItem> stageInfoItems = dmesgParser.getStageInfoItems();
        assertEquals(2, stageInfoItems.size());
        assertEquals(EXPECTED_STAGE_INFO_ITEMS, stageInfoItems);
    }

    /** Test processing action start time logs */
    public void testCompleteActionInfo() {
        DmesgParser dmesgParser = new DmesgParser();
        for (String line : LINES) {
            dmesgParser.parseActionInfo(line);
        }
        List<DmesgActionInfoItem> actualActionInfoItems = dmesgParser.getActionInfoItems();
        assertEquals(9, actualActionInfoItems.size());
        assertEquals(EXPECTED_ACTION_INFO_ITEMS, actualActionInfoItems);
    }

    private static List<DmesgActionInfoItem> getExpectedActionInfoItems() {
        return Arrays.asList(
                new DmesgActionInfoItem("early-init", (long) (Double.parseDouble("44942.872"))),
                new DmesgActionInfoItem(
                        "set_mmap_rnd_bits", (long) (Double.parseDouble("47233.446"))),
                new DmesgActionInfoItem(
                        "set_kptr_restrict", (long) (Double.parseDouble("47240.083"))),
                new DmesgActionInfoItem("keychord_init", (long) (Double.parseDouble("47245.778"))),
                new DmesgActionInfoItem(
                        "persist.sys.usb.config=* boot", (long) (Double.parseDouble("52361.049"))),
                new DmesgActionInfoItem(
                        "enable_property_trigger", (long) (Double.parseDouble("52361.108"))),
                new DmesgActionInfoItem(
                        "security.perf_harden=1", (long) (Double.parseDouble("52361.313"))),
                new DmesgActionInfoItem(
                        "ro.debuggable=1", (long) (Double.parseDouble("52361.495"))),
                new DmesgActionInfoItem(
                        "sys.boot_completed=1", (long) (Double.parseDouble("58298.293"))));
    }

    private static List<DmesgStageInfoItem> getExpectedStageInfoItems() {
        return Arrays.asList(
                new DmesgStageInfoItem("ueventd_Coldboot", null, 701L),
                new DmesgStageInfoItem("first", 41665L, null));
    }

    private static Map<String, DmesgServiceInfoItem> getExpectedServiceInfoItems() {
        Map<String, DmesgServiceInfoItem> serviceInfoItemsMap = new HashMap<>();
        DmesgServiceInfoItem bootanimServiceInfoItem = new DmesgServiceInfoItem();
        bootanimServiceInfoItem.setServiceName(BOOT_ANIMATION);
        bootanimServiceInfoItem.setStartTime(22962L);
        bootanimServiceInfoItem.setEndTime(69855L);

        DmesgServiceInfoItem netdServiceInfoItem = new DmesgServiceInfoItem();
        netdServiceInfoItem.setServiceName(NETD);
        netdServiceInfoItem.setStartTime(23252L);

        serviceInfoItemsMap.put(BOOT_ANIMATION, bootanimServiceInfoItem);
        serviceInfoItemsMap.put(NETD, netdServiceInfoItem);

        return serviceInfoItemsMap;
    }

}
