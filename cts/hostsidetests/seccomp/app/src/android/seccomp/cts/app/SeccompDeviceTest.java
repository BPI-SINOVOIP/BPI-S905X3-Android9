/*
 * Copyright (C) 2017 The Android Open Source Project
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

package android.seccomp.cts.app;

import java.io.IOException;
import java.io.InputStream;
import java.util.Iterator;

import android.content.Context;
import android.content.res.AssetManager;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import com.android.compatibility.common.util.CpuFeatures;
import org.json.JSONObject;
import org.json.JSONException;

import org.junit.After;
import org.junit.Assert;
import org.junit.Assume;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

/**
 * Device-side tests for CtsSeccompHostTestCases
 */
@RunWith(AndroidJUnit4.class)
public class SeccompDeviceTest {
    static {
        System.loadLibrary("ctsseccomp_jni");
    }

    private JSONObject mAllowedSyscallMap;
    private JSONObject mBlockedSyscallMap;

    @Before
    public void initializeSyscallMap() throws IOException, JSONException {
        final Context context = InstrumentationRegistry.getInstrumentation().getTargetContext();
        AssetManager manager = context.getAssets();
        try (InputStream is = manager.open("syscalls_allowed.json")) {
            mAllowedSyscallMap = new JSONObject(readInputStreamFully(is));
        }
        try (InputStream is = manager.open("syscalls_blocked.json")) {
            mBlockedSyscallMap = new JSONObject(readInputStreamFully(is));
        }
    }

    @Test
    public void testCTSSyscallAllowed() throws JSONException {
        JSONObject map = mAllowedSyscallMap.getJSONObject(getCurrentArch());
        Iterator<String> iter = map.keys();
        while (iter.hasNext()) {
            String syscallName = iter.next();
            testAllowed(map.getInt(syscallName));
        }
    }

    @Test
    public void testCTSSyscallBlocked() throws JSONException {
        JSONObject map = mBlockedSyscallMap.getJSONObject(getCurrentArch());
        Iterator<String> iter = map.keys();
        while (iter.hasNext()) {
            String syscallName = iter.next();
            testBlocked(map.getInt(syscallName));
        }
    }

    private static String getCurrentArch() {
        if (CpuFeatures.isArm64Cpu()) {
            return "arm64";
        } else if (CpuFeatures.isArmCpu()) {
            return "arm";
        } else if (CpuFeatures.isX86_64Cpu()) {
            return "x86_64";
        } else if (CpuFeatures.isX86Cpu()) {
            return "x86";
        } else if (CpuFeatures.isMips64Cpu()) {
            return "mips64";
        } else if (CpuFeatures.isMipsCpu()) {
            return "mips";
        } else {
            Assert.fail("Unsupported OS");
            return null;
        }
    }

    private String readInputStreamFully(InputStream is) throws IOException {
        StringBuilder sb = new StringBuilder();
        byte[] buffer = new byte[4096];
        while (is.available() > 0) {
            int size = is.read(buffer);
            if (size < 0) {
                break;
            }
            sb.append(new String(buffer, 0, size));
        }
        return sb.toString();
    }

    private void testBlocked(int nr) {
        Assert.assertTrue("Syscall " + nr + " not blocked", testSyscallBlocked(nr));
    }

    private void testAllowed(int nr) {
        Assert.assertFalse("Syscall " + nr + " blocked", testSyscallBlocked(nr));
    }

    private static final native boolean testSyscallBlocked(int nr);
}
