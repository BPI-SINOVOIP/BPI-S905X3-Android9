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

package android.net.cts;

import android.system.ErrnoException;
import android.system.Os;
import android.system.OsConstants;
import android.system.StructStat;
import android.test.AndroidTestCase;

import java.io.File;
import java.io.FileDescriptor;
import java.io.IOException;

/**
 * Tests for multinetwork sysctl functionality.
 */
public class MultinetworkSysctlTest extends SysctlBaseTest {

    // Global sysctls. Must be present and set to 1.
    private static final String[] GLOBAL_SYSCTLS = {
        "/proc/sys/net/ipv4/fwmark_reflect",
        "/proc/sys/net/ipv6/fwmark_reflect",
        "/proc/sys/net/ipv4/tcp_fwmark_accept",
    };

    // Per-interface IPv6 autoconf sysctls.
    private static final String IPV6_SYSCTL_DIR = "/proc/sys/net/ipv6/conf";
    private static final String AUTOCONF_SYSCTL = "accept_ra_rt_table";

    /**
     * Checks that the sysctls for multinetwork kernel features are present and
     * enabled. The necessary kernel commits are:
     *
     * Mainline Linux:
     *   e110861 net: add a sysctl to reflect the fwmark on replies
     *   1b3c61d net: Use fwmark reflection in PMTU discovery.
     *   84f39b0 net: support marking accepting TCP sockets
     *
     * Common Android tree (e.g., 3.10):
     *   a03f539 net: ipv6: autoconf routes into per-device tables
     */
     public void testProcFiles() throws ErrnoException, IOException, NumberFormatException {
         for (String sysctl : GLOBAL_SYSCTLS) {
             int value = getIntValue(sysctl);
             assertEquals(sysctl, 1, value);
         }

         File[] interfaceDirs = new File(IPV6_SYSCTL_DIR).listFiles();
         for (File interfaceDir : interfaceDirs) {
             if (interfaceDir.getName().equals("all") || interfaceDir.getName().equals("lo")) {
                 continue;
             }
             String sysctl = new File(interfaceDir, AUTOCONF_SYSCTL).getAbsolutePath();
             int value = getIntValue(sysctl);
             assertLess(sysctl, value, 0);
         }
     }
}
