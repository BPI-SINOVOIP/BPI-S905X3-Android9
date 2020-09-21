/*
 * Copyright (C) 2018 The Android Open Source Project
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
public class SysctlBaseTest extends AndroidTestCase {

    // Expected mode, UID, and GID of sysctl files.
    private static final int SYSCTL_MODE = 0100644;
    private static final int SYSCTL_UID = 0;
    private static final int SYSCTL_GID = 0;

    private void checkSysctlPermissions(String fileName) throws ErrnoException {
        StructStat stat = Os.stat(fileName);
        assertEquals("mode of " + fileName + ":", SYSCTL_MODE, stat.st_mode);
        assertEquals("UID of " + fileName + ":", SYSCTL_UID, stat.st_uid);
        assertEquals("GID of " + fileName + ":", SYSCTL_GID, stat.st_gid);
    }

    protected void assertLess(String sysctl, int a, int b) {
        assertTrue("value of " + sysctl + ": expected < " + b + " but was: " + a, a < b);
    }

    protected void assertAtLeast(String sysctl, int a, int b) {
        assertTrue("value of " + sysctl + ": expected >= " + b + " but was: " + a, a >= b);
    }

    private String readFile(String fileName) throws ErrnoException, IOException {
        byte[] buf = new byte[1024];
        FileDescriptor fd = Os.open(fileName, 0, OsConstants.O_RDONLY);
        int bytesRead = Os.read(fd, buf, 0, buf.length);
        assertLess("length of " + fileName + ":", bytesRead, buf.length);
        return new String(buf);
    }

    /*
     * Checks permissions and retrieves the sysctl's value. Retrieval of value should always use
     * this method
     */
    protected int getIntValue(String filename) throws ErrnoException, IOException {
        checkSysctlPermissions(filename);
        return Integer.parseInt(readFile(filename).trim());
    }
}
