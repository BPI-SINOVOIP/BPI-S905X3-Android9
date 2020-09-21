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

package android.appsecurity.cts;

import static android.appsecurity.cts.SplitTests.ABI_TO_APK;
import static android.appsecurity.cts.SplitTests.APK;
import static android.appsecurity.cts.SplitTests.APK_mdpi;
import static android.appsecurity.cts.SplitTests.APK_xxhdpi;
import static android.appsecurity.cts.SplitTests.CLASS;
import static android.appsecurity.cts.SplitTests.PKG;

import static org.junit.Assert.fail;

import android.platform.test.annotations.AppModeFull;
import com.android.tradefed.device.CollectingOutputReceiver;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;

import org.junit.After;
import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.concurrent.TimeUnit;

/**
 * Set of tests that verify behavior of adopted storage media, if supported.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class AdoptableHostTest extends BaseHostJUnit4Test {

    public static final String FEATURE_ADOPTABLE_STORAGE = "feature:android.software.adoptable_storage";

    @Before
    public void setUp() throws Exception {
        // Start all possible users to make sure their storage is unlocked
        Utils.prepareMultipleUsers(getDevice(), Integer.MAX_VALUE);

        getDevice().uninstallPackage(PKG);

        // Enable a virtual disk to give us the best shot at being able to pass
        // the various tests below. This helps verify devices that may not
        // currently have an SD card inserted.
        if (isSupportedDevice()) {
            getDevice().executeShellCommand("sm set-virtual-disk true");
        }
    }

    @After
    public void tearDown() throws Exception {
        getDevice().uninstallPackage(PKG);

        if (isSupportedDevice()) {
            getDevice().executeShellCommand("sm set-virtual-disk false");
        }
    }

    /**
     * Ensure that we have consistency between the feature flag and what we
     * sniffed from the underlying fstab.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testFeatureConsistent() throws Exception {
        final boolean hasFeature = hasFeature();
        final boolean hasFstab = hasFstab();
        if (hasFeature != hasFstab) {
            fail("Inconsistent adoptable storage status; feature claims " + hasFeature
                    + " but fstab claims " + hasFstab);
        }
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testApps() throws Exception {
        if (!isSupportedDevice()) return;
        final String diskId = getAdoptionDisk();
        try {
            final String abi = getAbi().getName();
            final String apk = ABI_TO_APK.get(abi);
            Assert.assertNotNull("Failed to find APK for ABI " + abi, apk);

            // Install simple app on internal
            new InstallMultiple().useNaturalAbi().addApk(APK).addApk(apk).run();
            runDeviceTests(PKG, CLASS, "testDataInternal");
            runDeviceTests(PKG, CLASS, "testDataWrite");
            runDeviceTests(PKG, CLASS, "testDataRead");
            runDeviceTests(PKG, CLASS, "testNative");

            // Adopt that disk!
            assertEmpty(getDevice().executeShellCommand("sm partition " + diskId + " private"));
            final LocalVolumeInfo vol = getAdoptionVolume();

            // Move app and verify
            assertSuccess(getDevice().executeShellCommand(
                    "pm move-package " + PKG + " " + vol.uuid));
            runDeviceTests(PKG, CLASS, "testDataNotInternal");
            runDeviceTests(PKG, CLASS, "testDataRead");
            runDeviceTests(PKG, CLASS, "testNative");

            // Unmount, remount and verify
            getDevice().executeShellCommand("sm unmount " + vol.volId);
            getDevice().executeShellCommand("sm mount " + vol.volId);
            runDeviceTests(PKG, CLASS, "testDataNotInternal");
            runDeviceTests(PKG, CLASS, "testDataRead");
            runDeviceTests(PKG, CLASS, "testNative");

            // Move app back and verify
            assertSuccess(getDevice().executeShellCommand("pm move-package " + PKG + " internal"));
            runDeviceTests(PKG, CLASS, "testDataInternal");
            runDeviceTests(PKG, CLASS, "testDataRead");
            runDeviceTests(PKG, CLASS, "testNative");

            // Un-adopt volume and app should still be fine
            getDevice().executeShellCommand("sm partition " + diskId + " public");
            runDeviceTests(PKG, CLASS, "testDataInternal");
            runDeviceTests(PKG, CLASS, "testDataRead");
            runDeviceTests(PKG, CLASS, "testNative");

        } finally {
            cleanUp(diskId);
        }
    }

    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testPrimaryStorage() throws Exception {
        if (!isSupportedDevice()) return;
        final String diskId = getAdoptionDisk();
        try {
            final String originalVol = getDevice()
                    .executeShellCommand("sm get-primary-storage-uuid").trim();

            if ("null".equals(originalVol)) {
                verifyPrimaryInternal(diskId);
            } else if ("primary_physical".equals(originalVol)) {
                verifyPrimaryPhysical(diskId);
            }
        } finally {
            cleanUp(diskId);
        }
    }

    private void verifyPrimaryInternal(String diskId) throws Exception {
        // Write some data to shared storage
        new InstallMultiple().addApk(APK).run();
        runDeviceTests(PKG, CLASS, "testPrimaryOnSameVolume");
        runDeviceTests(PKG, CLASS, "testPrimaryInternal");
        runDeviceTests(PKG, CLASS, "testPrimaryDataWrite");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // Adopt that disk!
        assertEmpty(getDevice().executeShellCommand("sm partition " + diskId + " private"));
        final LocalVolumeInfo vol = getAdoptionVolume();

        // Move storage there and verify that data went along for ride
        CollectingOutputReceiver out = new CollectingOutputReceiver();
        getDevice().executeShellCommand("pm move-primary-storage " + vol.uuid, out, 2,
                TimeUnit.HOURS, 1);
        assertSuccess(out.getOutput());
        runDeviceTests(PKG, CLASS, "testPrimaryAdopted");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // Unmount and verify
        getDevice().executeShellCommand("sm unmount " + vol.volId);
        runDeviceTests(PKG, CLASS, "testPrimaryUnmounted");
        getDevice().executeShellCommand("sm mount " + vol.volId);
        runDeviceTests(PKG, CLASS, "testPrimaryAdopted");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // Move app and verify backing storage volume is same
        assertSuccess(getDevice().executeShellCommand("pm move-package " + PKG + " " + vol.uuid));
        runDeviceTests(PKG, CLASS, "testPrimaryOnSameVolume");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // And move back to internal
        out = new CollectingOutputReceiver();
        getDevice().executeShellCommand("pm move-primary-storage internal", out, 2,
                TimeUnit.HOURS, 1);
        assertSuccess(out.getOutput());

        runDeviceTests(PKG, CLASS, "testPrimaryInternal");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        assertSuccess(getDevice().executeShellCommand("pm move-package " + PKG + " internal"));
        runDeviceTests(PKG, CLASS, "testPrimaryOnSameVolume");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");
    }

    private void verifyPrimaryPhysical(String diskId) throws Exception {
        // Write some data to shared storage
        new InstallMultiple().addApk(APK).run();
        runDeviceTests(PKG, CLASS, "testPrimaryPhysical");
        runDeviceTests(PKG, CLASS, "testPrimaryDataWrite");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // Adopt that disk!
        assertEmpty(getDevice().executeShellCommand("sm partition " + diskId + " private"));
        final LocalVolumeInfo vol = getAdoptionVolume();

        // Move primary storage there, but since we just nuked primary physical
        // the storage device will be empty
        assertSuccess(getDevice().executeShellCommand("pm move-primary-storage " + vol.uuid));
        runDeviceTests(PKG, CLASS, "testPrimaryAdopted");
        runDeviceTests(PKG, CLASS, "testPrimaryDataWrite");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // Unmount and verify
        getDevice().executeShellCommand("sm unmount " + vol.volId);
        runDeviceTests(PKG, CLASS, "testPrimaryUnmounted");
        getDevice().executeShellCommand("sm mount " + vol.volId);
        runDeviceTests(PKG, CLASS, "testPrimaryAdopted");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");

        // And move to internal
        assertSuccess(getDevice().executeShellCommand("pm move-primary-storage internal"));
        runDeviceTests(PKG, CLASS, "testPrimaryOnSameVolume");
        runDeviceTests(PKG, CLASS, "testPrimaryInternal");
        runDeviceTests(PKG, CLASS, "testPrimaryDataRead");
    }

    /**
     * Verify that we can install both new and inherited packages directly on
     * adopted volumes.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testPackageInstaller() throws Exception {
        if (!isSupportedDevice()) return;
        final String diskId = getAdoptionDisk();
        try {
            assertEmpty(getDevice().executeShellCommand("sm partition " + diskId + " private"));
            final LocalVolumeInfo vol = getAdoptionVolume();

            // Install directly onto adopted volume
            new InstallMultiple().locationAuto().forceUuid(vol.uuid)
                    .addApk(APK).addApk(APK_mdpi).run();
            runDeviceTests(PKG, CLASS, "testDataNotInternal");
            runDeviceTests(PKG, CLASS, "testDensityBest1");

            // Now splice in an additional split which offers better resources
            new InstallMultiple().locationAuto().inheritFrom(PKG)
                    .addApk(APK_xxhdpi).run();
            runDeviceTests(PKG, CLASS, "testDataNotInternal");
            runDeviceTests(PKG, CLASS, "testDensityBest2");

        } finally {
            cleanUp(diskId);
        }
    }

    /**
     * Verify behavior when changes occur while adopted device is ejected and
     * returned at a later time.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testEjected() throws Exception {
        if (!isSupportedDevice()) return;
        final String diskId = getAdoptionDisk();
        try {
            assertEmpty(getDevice().executeShellCommand("sm partition " + diskId + " private"));
            final LocalVolumeInfo vol = getAdoptionVolume();

            // Install directly onto adopted volume, and write data there
            new InstallMultiple().locationAuto().forceUuid(vol.uuid).addApk(APK).run();
            runDeviceTests(PKG, CLASS, "testDataNotInternal");
            runDeviceTests(PKG, CLASS, "testDataWrite");
            runDeviceTests(PKG, CLASS, "testDataRead");

            // Now unmount and uninstall; leaving stale package on adopted volume
            getDevice().executeShellCommand("sm unmount " + vol.volId);
            getDevice().uninstallPackage(PKG);

            // Install second copy on internal, but don't write anything
            new InstallMultiple().locationInternalOnly().addApk(APK).run();
            runDeviceTests(PKG, CLASS, "testDataInternal");

            // Kick through a remount cycle, which should purge the adopted app
            getDevice().executeShellCommand("sm mount " + vol.volId);
            runDeviceTests(PKG, CLASS, "testDataInternal");
            boolean didThrow = false;
            try {
                runDeviceTests(PKG, CLASS, "testDataRead");
            } catch (AssertionError expected) {
                didThrow = true;
            }
            if (!didThrow) {
                fail("Unexpected data from adopted volume picked up");
            }
            getDevice().executeShellCommand("sm unmount " + vol.volId);

            // Uninstall the internal copy and remount; we should have no record of app
            getDevice().uninstallPackage(PKG);
            getDevice().executeShellCommand("sm mount " + vol.volId);

            assertEmpty(getDevice().executeShellCommand("pm list packages " + PKG));
        } finally {
            cleanUp(diskId);
        }
    }

    private boolean isSupportedDevice() throws Exception {
        return hasFeature() || hasFstab();
    }

    private boolean hasFeature() throws Exception {
        return getDevice().hasFeature(FEATURE_ADOPTABLE_STORAGE);
    }

    private boolean hasFstab() throws Exception {
        return Boolean.parseBoolean(getDevice().executeShellCommand("sm has-adoptable").trim());
    }

    private String getAdoptionDisk() throws Exception {
        // In the case where we run multiple test we cleanup the state of the device. This
        // results in the execution of sm forget all which causes the MountService to "reset"
        // all its knowledge about available drives. This can cause the adoptable drive to
        // become temporarily unavailable.
        int attempt = 0;
        String disks = getDevice().executeShellCommand("sm list-disks adoptable");
        while ((disks == null || disks.isEmpty()) && attempt++ < 15) {
            Thread.sleep(1000);
            disks = getDevice().executeShellCommand("sm list-disks adoptable");
        }

        if (disks == null || disks.isEmpty()) {
            throw new AssertionError("Devices that claim to support adoptable storage must have "
                    + "adoptable media inserted during CTS to verify correct behavior");
        }
        return disks.split("\n")[0].trim();
    }

    private LocalVolumeInfo getAdoptionVolume() throws Exception {
        String[] lines = null;
        int attempt = 0;
        while (attempt++ < 15) {
            lines = getDevice().executeShellCommand("sm list-volumes private").split("\n");
            for (String line : lines) {
                final LocalVolumeInfo info = new LocalVolumeInfo(line.trim());
                if (!"private".equals(info.volId) && "mounted".equals(info.state)) {
                    return info;
                }
            }
            Thread.sleep(1000);
        }
        throw new AssertionError("Expected private volume; found " + Arrays.toString(lines));
    }

    private void cleanUp(String diskId) throws Exception {
        getDevice().executeShellCommand("sm partition " + diskId + " public");
        getDevice().executeShellCommand("sm forget all");
    }

    private static void assertSuccess(String str) {
        if (str == null || !str.startsWith("Success")) {
            throw new AssertionError("Expected success string but found " + str);
        }
    }

    private static void assertEmpty(String str) {
        if (str != null && str.trim().length() > 0) {
            throw new AssertionError("Expected empty string but found " + str);
        }
    }

    private static class LocalVolumeInfo {
        public String volId;
        public String state;
        public String uuid;

        public LocalVolumeInfo(String line) {
            final String[] split = line.split(" ");
            volId = split[0];
            state = split[1];
            uuid = split[2];
        }
    }

    private class InstallMultiple extends BaseInstallMultiple<InstallMultiple> {
        public InstallMultiple() {
            super(getDevice(), getBuild(), getAbi());
        }
    }
}
