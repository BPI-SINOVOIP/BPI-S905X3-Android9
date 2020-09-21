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

import com.android.loganalysis.item.GfxInfoItem;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

public class GfxInfoParserTest extends TestCase {

    /**
     * Tests gfxinfo output from M with single process.
     */
    public void testSingleProcess() {
        List<String> input = Arrays.asList(
            "** Graphics info for pid 853 [com.google.android.leanbacklauncher] **",
            "",
            "Stats since: 13370233957ns",
            "Total frames rendered: 20391",
            "Janky frames: 785 (3.85%)",
            "90th percentile: 9ms",
            "95th percentile: 14ms",
            "99th percentile: 32ms",
            "Number Missed Vsync: 155",
            "Number High input latency: 0",
            "Number Slow UI thread: 469",
            "Number Slow bitmap uploads: 65",
            "Number Slow issue draw commands: 153",
            "",
            "Caches:",
            "Current memory usage / total memory usage (bytes):",
            "  TextureCache         16055224 / 50331648",
            "  LayerCache                  0 / 33554432 (numLayers = 0)",
            "  Layers total          0 (numLayers = 0)",
            "  RenderBufferCache           0 /  2097152",
            "  GradientCache               0 /   838860",
            "  PathCache                   0 / 25165824",
            "  TessellationCache     1048296 /  1048576",
            "  TextDropShadowCache         0 /  4194304",
            "  PatchCache                  0 /   131072",
            "  FontRenderer 0 A8      524288 /   524288",
            "  FontRenderer 0 RGBA         0 /        0",
            "  FontRenderer 0 total   524288 /   524288",
            "Other:",
            "  FboCache                    0 /        0",
            "Total memory usage:",
            "  17627808 bytes, 16.81 MB",
            "",
            "Profile data in ms:",
            "",
            "\tcom.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@8dc465 (visibility=8)",
            "Stats since: 13370233957ns",
            "Total frames rendered: 20391",
            "Janky frames: 785 (3.85%)",
            "90th percentile: 9ms",
            "95th percentile: 14ms",
            "99th percentile: 32ms",
            "Number Missed Vsync: 155",
            "Number High input latency: 0",
            "Number Slow UI thread: 469",
            "Number Slow bitmap uploads: 65",
            "Number Slow issue draw commands: 153",
            "",
            "View hierarchy:",
            "",
            "  com.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@8dc465",
            "  220 views, 177.61 kB of display lists",
            "",
            "",
            "Total ViewRootImpl: 1",
            "Total Views:        220",
            "Total DisplayList:  177.61 kB");

        GfxInfoItem item = new GfxInfoParser().parse(input);

        assertEquals(1, item.getPids().size());
        assertEquals("com.google.android.leanbacklauncher", item.getName(853));
        assertEquals(20391, item.getTotalFrames(853));
        assertEquals(785, item.getJankyFrames(853));
        assertEquals(9, item.getPrecentile90(853));
        assertEquals(14, item.getPrecentile95(853));
        assertEquals(32, item.getPrecentile99(853));
    }

    /**
     * Test gfxinfo output from M with multiple processes.
     */
    public void testMultipleProcesses() {
        List<String> input = Arrays.asList(
                "Applications Graphics Acceleration Info:",
                "Uptime: 6127679 Realtime: 6127679",
                "",
                "** Graphics info for pid 844 [com.google.android.leanbacklauncher] **",
                "",
                "Stats since: 12167093145ns",
                "Total frames rendered: 1690",
                "Janky frames: 125 (7.40%)",
                "90th percentile: 13ms",
                "95th percentile: 19ms",
                "99th percentile: 48ms",
                "Number Missed Vsync: 17",
                "Number High input latency: 0",
                "Number Slow UI thread: 32",
                "Number Slow bitmap uploads: 20",
                "Number Slow issue draw commands: 67",
                "",
                "Caches:",
                "Current memory usage / total memory usage (bytes):",
                "  TextureCache         16550096 / 50331648",
                "  LayerCache                  0 / 33554432 (numLayers = 0)",
                "  Layers total          0 (numLayers = 0)",
                "  RenderBufferCache           0 /  2097152",
                "  GradientCache               0 /   838860",
                "  PathCache                   0 / 25165824",
                "  TessellationCache      350424 /  1048576",
                "  TextDropShadowCache         0 /  4194304",
                "  PatchCache                  0 /   131072",
                "  FontRenderer 0 A8      524288 /   524288",
                "  FontRenderer 0 RGBA         0 /        0",
                "  FontRenderer 0 total   524288 /   524288",
                "Other:",
                "  FboCache                    0 /        0",
                "Total memory usage:",
                "  17424808 bytes, 16.62 MB",
                "",
                "Profile data in ms:",
                "",
                "\tcom.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@178d02b (visibility=0)",
                "Stats since: 12167093145ns",
                "Total frames rendered: 1690",
                "Janky frames: 125 (7.40%)",
                "90th percentile: 13ms",
                "95th percentile: 19ms",
                "99th percentile: 48ms",
                "Number Missed Vsync: 17",
                "Number High input latency: 0",
                "Number Slow UI thread: 32",
                "Number Slow bitmap uploads: 20",
                "Number Slow issue draw commands: 67",
                "",
                "View hierarchy:",
                "",
                "  com.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@178d02b",
                "  221 views, 207.24 kB of display lists",
                "",
                "",
                "Total ViewRootImpl: 1",
                "Total Views:        221",
                "Total DisplayList:  207.24 kB",
                "",
                "",
                "** Graphics info for pid 1881 [com.android.vending] **",
                "",
                "Stats since: 6092969986095ns",
                "Total frames rendered: 693",
                "Janky frames: 62 (8.95%)",
                "90th percentile: 16ms",
                "95th percentile: 26ms",
                "99th percentile: 81ms",
                "Number Missed Vsync: 17",
                "Number High input latency: 0",
                "Number Slow UI thread: 30",
                "Number Slow bitmap uploads: 4",
                "Number Slow issue draw commands: 13",
                "",
                "Caches:",
                "Current memory usage / total memory usage (bytes):",
                "  TextureCache          7369504 / 50331648",
                "  LayerCache                  0 / 33554432 (numLayers = 0)",
                "  Layers total          0 (numLayers = 0)",
                "  RenderBufferCache           0 /  2097152",
                "  GradientCache               0 /   838860",
                "  PathCache                   0 / 25165824",
                "  TessellationCache           0 /  1048576",
                "  TextDropShadowCache         0 /  4194304",
                "  PatchCache                  0 /   131072",
                "  FontRenderer 0 A8      524288 /   524288",
                "  FontRenderer 0 RGBA         0 /        0",
                "  FontRenderer 0 total   524288 /   524288",
                "Other:",
                "  FboCache                    0 /        0",
                "Total memory usage:",
                "  7893792 bytes, 7.53 MB",
                "",
                "Profile data in ms:",
                "",
                "\tcom.android.vending/com.google.android.finsky.activities.MainActivity/android.view.ViewRootImpl@5bd0cb2 (visibility=8)",
                "Stats since: 6092969986095ns",
                "Total frames rendered: 693",
                "Janky frames: 62 (8.95%)",
                "90th percentile: 16ms",
                "95th percentile: 26ms",
                "99th percentile: 81ms",
                "Number Missed Vsync: 17",
                "Number High input latency: 0",
                "Number Slow UI thread: 30",
                "Number Slow bitmap uploads: 4",
                "Number Slow issue draw commands: 13",
                "",
                "View hierarchy:",
                "",
                "  com.android.vending/com.google.android.finsky.activities.MainActivity/android.view.ViewRootImpl@5bd0cb2",
                "  195 views, 157.71 kB of display lists",
                "",
                "",
                "Total ViewRootImpl: 1",
                "Total Views:        195",
                "Total DisplayList:  157.71 kB",
                "",
                "",
                "** Graphics info for pid 2931 [com.google.android.videos] **",
                "",
                "Stats since: 6039768250261ns",
                "Total frames rendered: 107",
                "Janky frames: 42 (39.25%)",
                "90th percentile: 48ms",
                "95th percentile: 65ms",
                "99th percentile: 113ms",
                "Number Missed Vsync: 9",
                "Number High input latency: 0",
                "Number Slow UI thread: 28",
                "Number Slow bitmap uploads: 8",
                "Number Slow issue draw commands: 20",
                "",
                "Caches:",
                "Current memory usage / total memory usage (bytes):",
                "  TextureCache          7880000 / 50331648",
                "  LayerCache                  0 / 33554432 (numLayers = 0)",
                "  Layers total          0 (numLayers = 0)",
                "  RenderBufferCache           0 /  2097152",
                "  GradientCache               0 /   838860",
                "  PathCache                   0 / 25165824",
                "  TessellationCache           0 /  1048576",
                "  TextDropShadowCache         0 /  4194304",
                "  PatchCache                  0 /   131072",
                "  FontRenderer 0 A8      524288 /   524288",
                "  FontRenderer 0 RGBA         0 /        0",
                "  FontRenderer 0 total   524288 /   524288",
                "Other:",
                "  FboCache                    0 /        0",
                "Total memory usage:",
                "  8404288 bytes, 8.01 MB",
                "",
                "Profile data in ms:",
                "",
                "\tcom.google.android.videos/com.google.android.videos.pano.activity.PanoHomeActivity/android.view.ViewRootImpl@3d96e69 (visibility=8)",
                "Stats since: 6039768250261ns",
                "Total frames rendered: 107",
                "Janky frames: 42 (39.25%)",
                "90th percentile: 48ms",
                "95th percentile: 65ms",
                "99th percentile: 113ms",
                "Number Missed Vsync: 9",
                "Number High input latency: 0",
                "Number Slow UI thread: 28",
                "Number Slow bitmap uploads: 8",
                "Number Slow issue draw commands: 20",
                "",
                "View hierarchy:",
                "",
                "  com.google.android.videos/com.google.android.videos.pano.activity.PanoHomeActivity/android.view.ViewRootImpl@3d96e69",
                "  219 views, 173.57 kB of display lists",
                "",
                "",
                "Total ViewRootImpl: 1",
                "Total Views:        219",
                "Total DisplayList:  173.57 kB");

        GfxInfoItem item = new GfxInfoParser().parse(input);

        assertEquals(3, item.getPids().size());
        assertEquals("com.google.android.leanbacklauncher", item.getName(844));
        assertEquals(1690, item.getTotalFrames(844));
        assertEquals(125, item.getJankyFrames(844));
        assertEquals(13, item.getPrecentile90(844));
        assertEquals(19, item.getPrecentile95(844));
        assertEquals(48, item.getPrecentile99(844));
        assertEquals("com.android.vending", item.getName(1881));
        assertEquals(693, item.getTotalFrames(1881));
        assertEquals(62, item.getJankyFrames(1881));
        assertEquals(16, item.getPrecentile90(1881));
        assertEquals(26, item.getPrecentile95(1881));
        assertEquals(81, item.getPrecentile99(1881));
        assertEquals("com.google.android.videos", item.getName(2931));
        assertEquals(107, item.getTotalFrames(2931));
        assertEquals(42, item.getJankyFrames(2931));
        assertEquals(48, item.getPrecentile90(2931));
        assertEquals(65, item.getPrecentile95(2931));
        assertEquals(113, item.getPrecentile99(2931));
    }

    /**
     * Test gfxinfo output from L with single process.
     * In L, gfxinfo does not contain Jank number information.
     * This method tests that GfxInfoParser silently ignores such outputs.
     */
    public void testSingleProcessInL() {
        List<String> input = Arrays.asList(
                "** Graphics info for pid 1924 [com.google.android.leanbacklauncher] **",
                "",
                "Caches:",
                "Current memory usage / total memory usage (bytes):",
                "  TextureCache         19521592 / 50331648",
                "  LayerCache           14745600 / 50331648 (numLayers = 5)",
                "    Layer size 512x512; isTextureLayer()=0; texid=392 fbo=0; refs=1",
                "    Layer size 512x448; isTextureLayer()=0; texid=377 fbo=0; refs=1",
                "    Layer size 1920x832; isTextureLayer()=0; texid=360 fbo=0; refs=1",
                "    Layer size 1920x512; isTextureLayer()=0; texid=14 fbo=0; refs=1",
                "    Layer size 1920x320; isTextureLayer()=0; texid=393 fbo=0; refs=1",
                "  Layers total   14745600 (numLayers = 5)",
                "  RenderBufferCache           0 /  8388608",
                "  GradientCache               0 /  1048576",
                "  PathCache             3370264 / 33554432",
                "  TessellationCache      194928 /  1048576",
                "  TextDropShadowCache         0 /  6291456",
                "  PatchCache                  0 /   131072",
                "  FontRenderer 0 A8     1048576 /  1048576",
                "  FontRenderer 0 RGBA         0 /        0",
                "  FontRenderer 0 total  1048576 /  1048576",
                "Other:",
                "  FboCache                    0 /        0",
                "Total memory usage:",
                "  38880960 bytes, 37.08 MB",
                "",
                "Profile data in ms:",
                "",
                "\tcom.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@355a7923 (visibility=0)",
                "View hierarchy:",
                "",
                "  com.google.android.leanbacklauncher/com.google.android.leanbacklauncher.MainActivity/android.view.ViewRootImpl@355a7923",
                "  142 views, 136.96 kB of display lists",
                "",
                "",
                "Total ViewRootImpl: 1",
                "Total Views:        142",
                "Total DisplayList:  136.96 kB");

        GfxInfoItem item = new GfxInfoParser().parse(input);

        assertEquals(0, item.getPids().size());
    }
}
