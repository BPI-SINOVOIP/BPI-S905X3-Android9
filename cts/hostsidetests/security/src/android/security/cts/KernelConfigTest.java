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

package android.security.cts;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;
import com.android.tradefed.testtype.IDeviceTest;
import com.android.compatibility.common.util.CddTest;
import com.android.compatibility.common.util.CpuFeatures;
import com.android.compatibility.common.util.PropertyUtil;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.lang.String;
import java.util.stream.Collectors;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.zip.GZIPInputStream;

/**
 * Host-side kernel config tests.
 *
 * These tests analyze /proc/config.gz to verify that certain kernel config options are set.
 */
public class KernelConfigTest extends DeviceTestCase implements IBuildReceiver, IDeviceTest {

    private static final Map<ITestDevice, HashSet<String>> cachedConfigGzSet = new HashMap<>(1);

    private HashSet<String> configSet;

    private ITestDevice mDevice;
    private IBuildInfo mBuild;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setBuild(IBuildInfo build) {
        mBuild = build;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setDevice(ITestDevice device) {
        super.setDevice(device);
        mDevice = device;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        configSet = getDeviceConfig(mDevice, cachedConfigGzSet);
    }

    /*
     * IMPLEMENTATION DETAILS: Cache the configurations from /proc/config.gz on per-device basis
     * in case CTS is being run against multiple devices at the same time. This speeds up testing
     * by avoiding pulling/parsing the config file for each individual test
     */
    private static HashSet<String> getDeviceConfig(ITestDevice device,
            Map<ITestDevice, HashSet<String>> cache) throws Exception {
        if (!device.doesFileExist("/proc/config.gz")){
            throw new Exception();
        }
        HashSet<String> set;
        synchronized (cache) {
            set = cache.get(device);
        }
        if (set != null) {
            return set;
        }
        File file = File.createTempFile("config.gz", ".tmp");
        file.deleteOnExit();
        device.pullFile("/proc/config.gz", file);

        BufferedReader reader = new BufferedReader(new InputStreamReader(new GZIPInputStream(new FileInputStream(file))));
        set = new HashSet<String>(reader.lines().collect(Collectors.toList()));

        synchronized (cache) {
            cache.put(device, set);
        }
        return set;
    }

    /**
     * Test that the kernel has Stack Protector Strong enabled.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testConfigStackProtectorStrong() throws Exception {
        assertTrue("Linux kernel must have Stack Protector enabled: " +
                "CONFIG_CC_STACKPROTECTOR_STRONG=y",
                configSet.contains("CONFIG_CC_STACKPROTECTOR_STRONG=y"));
    }

    /**
     * Test that the kernel's executable code is read-only, read-only data is non-executable and
     * non-writable, and writable data is non-executable.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testConfigROData() throws Exception {
        assertTrue("Linux kernel must have RO data enabled: " +
                "CONFIG_DEBUG_RODATA=y or CONFIG_STRICT_KERNEL_RWX=y",
                configSet.contains("CONFIG_DEBUG_RODATA=y") ||
                configSet.contains("CONFIG_STRICT_KERNEL_RWX=y"));

        if (configSet.contains("CONFIG_MODULES=y")) {
            assertTrue("Linux kernel modules must also have RO data enabled: " +
                    "CONFIG_DEBUG_SET_MODULE_RONX=y or CONFIG_STRICT_MODULE_RWX=y",
                    configSet.contains("CONFIG_DEBUG_SET_MODULE_RONX=y") ||
                    configSet.contains("CONFIG_STRICT_MODULE_RWX=y"));
        }
    }

    /**
     * Test that the kernel implements static and dynamic object size bounds checking of copies
     * between user-space and kernel-space.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testConfigHardenedUsercopy() throws Exception {
        if (PropertyUtil.getFirstApiLevel(mDevice) < 28) {
            return;
        }

        assertTrue("Linux kernel must have Hardened Usercopy enabled: CONFIG_HARDENED_USERCOPY=y",
                configSet.contains("CONFIG_HARDENED_USERCOPY=y"));
    }

    /**
     * Test that the kernel has PAN emulation enabled from architectures that support it.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testConfigPAN() throws Exception {
        if (PropertyUtil.getFirstApiLevel(mDevice) < 28) {
            return;
        }

        if (CpuFeatures.isArm64(mDevice)) {
            assertTrue("Linux kernel must have PAN emulation enabled: " +
                    "CONFIG_ARM64_SW_TTBR0_PAN=y or CONFIG_ARM64_PAN=y",
                    (configSet.contains("CONFIG_ARM64_SW_TTBR0_PAN=y") ||
                    configSet.contains("CONFIG_ARM64_PAN=y")));
        } else if (CpuFeatures.isArm32(mDevice)) {
            assertTrue("Linux kernel must have PAN emulation enabled: CONFIG_CPU_SW_DOMAIN_PAN=y",
                    configSet.contains("CONFIG_CPU_SW_DOMAIN_PAN=y"));
        }
    }

    /**
     * Test that the kernel has KTPI enabled for architectures and kernel versions that support it.
     *
     * @throws Exception
     */
    @CddTest(requirement="9.7")
    public void testConfigKTPI() throws Exception {
        if (PropertyUtil.getFirstApiLevel(mDevice) < 28) {
            return;
        }

        if (CpuFeatures.isArm64(mDevice) && !CpuFeatures.kernelVersionLessThan(mDevice, 4, 4)) {
            assertTrue("Linux kernel must have KPTI enabled: CONFIG_UNMAP_KERNEL_AT_EL0=y",
                    configSet.contains("CONFIG_UNMAP_KERNEL_AT_EL0=y"));
        } else if (CpuFeatures.isX86(mDevice)) {
            assertTrue("Linux kernel must have KPTI enabled: CONFIG_PAGE_TABLE_ISOLATION=y",
                    configSet.contains("CONFIG_PAGE_TABLE_ISOLATION=y"));
        }
    }
}
