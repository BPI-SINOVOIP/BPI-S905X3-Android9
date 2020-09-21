/*
 * Copyright (C) 2012 The Android Open Source Project
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

import com.android.loganalysis.item.NativeCrashItem;
import com.android.loganalysis.util.ArrayUtil;

import junit.framework.TestCase;

import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link NativeCrashParser}.
 */
public class NativeCrashParserTest extends TestCase {

    /**
     * Test that native crashes are parsed.
     */
    public void testParseage() {
        List<String> lines = Arrays.asList(
                "Build fingerprint: 'google/soju/crespo:4.0.4/IMM76D/299849:userdebug/test-keys'",
                "pid: 2058, tid: 2523  >>> com.google.android.browser <<<",
                "signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000",
                " r0 00000000  r1 007d9064  r2 007d9063  r3 00000004",
                " r4 006bf518  r5 0091e3b0  r6 00000000  r7 9e3779b9",
                " r8 000006c1  r9 000006c3  10 00000000  fp 67d246c1",
                " ip d2363b58  sp 50ed71d8  lr 4edfc89b  pc 4edfc6a0  cpsr 20000030",
                " d0  00640065005f0065  d1  0072006f00740069",
                " d2  00730075006e006b  d3  0066006900670000",
                " d4  00e6d48800e6d3b8  d5  02d517a000e6d518",
                " d6  0000270f02d51860  d7  0000000002d51a80",
                " d8  41d3dc5261e7893b  d9  3fa999999999999a",
                " d10 0000000000000000  d11 0000000000000000",
                " d12 0000000000000000  d13 0000000000000000",
                " d14 0000000000000000  d15 0000000000000000",
                " d16 4070000000000000  d17 40c3878000000000",
                " d18 412310f000000000  d19 3f91800dedacf040",
                " d20 0000000000000000  d21 0000000000000000",
                " d22 4010000000000000  d23 0000000000000000",
                " d24 3ff0000000000000  d25 0000000000000000",
                " d26 0000000000000000  d27 8000000000000000",
                " d28 0000000000000000  d29 3ff0000000000000",
                " d30 0000000000000000  d31 3ff0000000000000",
                " scr 20000013",
                "",
                "         #00  pc 001236a0  /system/lib/libwebcore.so",
                "         #01  pc 00123896  /system/lib/libwebcore.so",
                "         #02  pc 00123932  /system/lib/libwebcore.so",
                "         #03  pc 00123e3a  /system/lib/libwebcore.so",
                "         #04  pc 00123e84  /system/lib/libwebcore.so",
                "         #05  pc 003db92a  /system/lib/libwebcore.so",
                "         #06  pc 003dd01c  /system/lib/libwebcore.so",
                "         #07  pc 002ffb92  /system/lib/libwebcore.so",
                "         #08  pc 0031c120  /system/lib/libwebcore.so",
                "         #09  pc 0031c134  /system/lib/libwebcore.so",
                "         #10  pc 0013fb98  /system/lib/libwebcore.so",
                "         #11  pc 0015b026  /system/lib/libwebcore.so",
                "         #12  pc 0015b164  /system/lib/libwebcore.so",
                "         #13  pc 0015f4cc  /system/lib/libwebcore.so",
                "         #14  pc 00170472  /system/lib/libwebcore.so",
                "         #15  pc 0016ecb6  /system/lib/libwebcore.so",
                "         #16  pc 0027120e  /system/lib/libwebcore.so",
                "         #17  pc 0026efec  /system/lib/libwebcore.so",
                "         #18  pc 0026fcd8  /system/lib/libwebcore.so",
                "         #19  pc 00122efa  /system/lib/libwebcore.so",
                "",
                "code around pc:",
                "4edfc680 4a14b5f7 0601f001 23000849 3004f88d  ...J....I..#...0",
                "4edfc690 460a9200 3006f8ad e00e4603 3a019f00  ...F...0.F.....:",
                "4edfc6a0 5c04f833 f83319ed 042c7c02 2cc7ea84  3..\\..3..|,....,",
                "4edfc6b0 0405ea8c 24d4eb04 33049400 d1ed2a00  .......$...3.*..",
                "4edfc6c0 f830b126 46681021 ff72f7ff f7ff4668  &.0.!.hF..r.hF..",
                "",
                "code around lr:",
                "4edfc878 f9caf7ff 60209e03 9605e037 5b04f856  ...... `7...V..[",
                "4edfc888 d0302d00 d13b1c6b 68a8e02d f7ff6869  .-0.k.;.-..hih..",
                "4edfc898 6128fef3 b010f8d5 99022500 ea0146aa  ..(a.....%...F..",
                "4edfc8a8 9b01080b 0788eb03 3028f853 b9bdb90b  ........S.(0....",
                "4edfc8b8 3301e015 4638d005 f7ff9905 b970ff15  ...3..8F......p.",
                "",
                "stack:",
                "    50ed7198  01d02c08  [heap]",
                "    50ed719c  40045881  /system/lib/libc.so",
                "    50ed71a0  400784c8",
                "    50ed71a4  400784c8",
                "    50ed71a8  02b40c68  [heap]",
                "    50ed71ac  02b40c90  [heap]",
                "    50ed71b0  50ed7290",
                "    50ed71b4  006bf518  [heap]",
                "    50ed71b8  00010000",
                "    50ed71bc  50ed72a4",
                "    50ed71c0  7da5a695",
                "    50ed71c4  50ed7290",
                "    50ed71c8  00000000",
                "    50ed71cc  00000008",
                "    50ed71d0  df0027ad",
                "    50ed71d4  00000000",
                "#00 50ed71d8  9e3779b9",
                "    50ed71dc  00002000",
                "    50ed71e0  00004000",
                "    50ed71e4  006bf518  [heap]",
                "    50ed71e8  0091e3b0  [heap]",
                "    50ed71ec  01d72588  [heap]",
                "    50ed71f0  00000000",
                "    50ed71f4  4edfc89b  /system/lib/libwebcore.so",
                "#01 50ed71f8  01d70a78  [heap]",
                "    50ed71fc  02b6afa8  [heap]",
                "    50ed7200  00003fff",
                "    50ed7204  01d70a78  [heap]",
                "    50ed7208  00004000",
                "    50ed720c  01d72584  [heap]",
                "    50ed7210  00000000",
                "    50ed7214  00000006",
                "    50ed7218  006bf518  [heap]",
                "    50ed721c  50ed72a4",
                "    50ed7220  7da5a695",
                "    50ed7224  50ed7290",
                "    50ed7228  000016b8",
                "    50ed722c  00000008",
                "    50ed7230  01d70a78  [heap]",
                "    50ed7234  4edfc937  /system/lib/libwebcore.so",
                "debuggerd committing suicide to free the zombie!",
                "debuggerd");

        NativeCrashItem nc = new NativeCrashParser().parse(lines);
        assertNotNull(nc);
        assertEquals(2058, nc.getPid().intValue());
        assertEquals(2523, nc.getTid().intValue());
        assertEquals("com.google.android.browser", nc.getApp());
        assertEquals("google/soju/crespo:4.0.4/IMM76D/299849:userdebug/test-keys",
                nc.getFingerprint());
        assertEquals(ArrayUtil.join("\n", lines), nc.getStack());
    }

    /**
     * Test that both types of native crash app lines are parsed.
     */
    public void testParseApp() {
        List<String> lines = Arrays.asList(
                "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "Build fingerprint: 'google/soju/crespo:4.0.4/IMM76D/299849:userdebug/test-keys'",
                "pid: 2058, tid: 2523  >>> com.google.android.browser <<<",
                "signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        NativeCrashItem nc = new NativeCrashParser().parse(lines);
        assertNotNull(nc);
        assertEquals(2058, nc.getPid().intValue());
        assertEquals(2523, nc.getTid().intValue());
        assertEquals("com.google.android.browser", nc.getApp());

        lines = Arrays.asList(
                "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "Build fingerprint: 'google/soju/crespo:4.0.4/IMM76D/299849:userdebug/test-keys'",
                "pid: 2058, tid: 2523, name: com.google.android.browser  >>> com.google.android.browser <<<",
                "signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        nc = new NativeCrashParser().parse(lines);
        assertNotNull(nc);

        assertEquals(2058, nc.getPid().intValue());
        assertEquals(2523, nc.getTid().intValue());
        assertEquals("com.google.android.browser", nc.getApp());

        lines = Arrays.asList(
                "*** *** *** *** *** *** *** *** *** *** *** *** *** *** *** ***",
                "Build fingerprint: 'google/soju/crespo:4.0.4/IMM76D/299849:userdebug/test-keys'",
                "pid: 2058, tid: 2523, name: Atlas Worker #1  >>> com.google.android.browser <<<",
                "signal 11 (SIGSEGV), code 1 (SEGV_MAPERR), fault addr 00000000");

        nc = new NativeCrashParser().parse(lines);
        assertNotNull(nc);

        assertEquals(2058, nc.getPid().intValue());
        assertEquals(2523, nc.getTid().intValue());
        assertEquals("com.google.android.browser", nc.getApp());
    }
}
