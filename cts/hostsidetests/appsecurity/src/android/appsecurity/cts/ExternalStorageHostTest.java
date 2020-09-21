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

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import android.platform.test.annotations.AppModeFull;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.Log;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceJUnit4ClassRunner;
import com.android.tradefed.testtype.junit4.BaseHostJUnit4Test;
import com.android.tradefed.util.AbiUtils;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;

/**
 * Set of tests that verify behavior of external storage devices.
 */
@RunWith(DeviceJUnit4ClassRunner.class)
public class ExternalStorageHostTest extends BaseHostJUnit4Test {
    private static final String TAG = "ExternalStorageHostTest";

    private static final String COMMON_CLASS =
            "com.android.cts.externalstorageapp.CommonExternalStorageTest";

    private static final String NONE_APK = "CtsExternalStorageApp.apk";
    private static final String NONE_PKG = "com.android.cts.externalstorageapp";
    private static final String NONE_CLASS = NONE_PKG + ".ExternalStorageTest";
    private static final String READ_APK = "CtsReadExternalStorageApp.apk";
    private static final String READ_PKG = "com.android.cts.readexternalstorageapp";
    private static final String READ_CLASS = READ_PKG + ".ReadExternalStorageTest";
    private static final String WRITE_APK = "CtsWriteExternalStorageApp.apk";
    private static final String WRITE_PKG = "com.android.cts.writeexternalstorageapp";
    private static final String WRITE_CLASS = WRITE_PKG + ".WriteExternalStorageTest";
    private static final String MULTIUSER_APK = "CtsMultiUserStorageApp.apk";
    private static final String MULTIUSER_PKG = "com.android.cts.multiuserstorageapp";
    private static final String MULTIUSER_CLASS = MULTIUSER_PKG + ".MultiUserStorageTest";

    private int[] mUsers;

    private File getTestAppFile(String fileName) throws FileNotFoundException {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(getBuild());
        return buildHelper.getTestFile(fileName);
    }

    @Before
    public void setUp() throws Exception {
        mUsers = Utils.prepareMultipleUsers(getDevice());
        assertNotNull(getAbi());
        assertNotNull(getBuild());
    }

    /**
     * Verify that app with no external storage permissions works correctly.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testExternalStorageNone() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};
            assertNull(getDevice().installPackage(getTestAppFile(NONE_APK), false, options));

            for (int user : mUsers) {
                runDeviceTests(NONE_PKG, COMMON_CLASS, user);
                runDeviceTests(NONE_PKG, NONE_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(NONE_PKG);
        }
    }

    /**
     * Verify that app with
     * {@link android.Manifest.permission#READ_EXTERNAL_STORAGE} works
     * correctly.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testExternalStorageRead() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(READ_PKG);
            String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};
            assertNull(getDevice().installPackage(getTestAppFile(READ_APK), false, options));

            for (int user : mUsers) {
                runDeviceTests(READ_PKG, COMMON_CLASS, user);
                runDeviceTests(READ_PKG, READ_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(READ_PKG);
        }
    }

    /**
     * Verify that app with
     * {@link android.Manifest.permission#WRITE_EXTERNAL_STORAGE} works
     * correctly.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testExternalStorageWrite() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(WRITE_PKG);
            String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};
            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));

            for (int user : mUsers) {
                runDeviceTests(WRITE_PKG, COMMON_CLASS, user);
                runDeviceTests(WRITE_PKG, WRITE_CLASS, user);
            }
        } finally {
            getDevice().uninstallPackage(WRITE_PKG);
        }
    }

    /**
     * Verify that app with WRITE_EXTERNAL can leave gifts in external storage
     * directories belonging to other apps, and those apps can read.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testExternalStorageGifts() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
            final String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};

            // We purposefully delay the installation of the reading apps to
            // verify that the daemon correctly invalidates any caches.
            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));
            for (int user : mUsers) {
                runDeviceTests(WRITE_PKG, WRITE_PKG + ".WriteGiftTest", user);
            }

            assertNull(getDevice().installPackage(getTestAppFile(NONE_APK), false, options));
            assertNull(getDevice().installPackage(getTestAppFile(READ_APK), false, options));
            for (int user : mUsers) {
                runDeviceTests(READ_PKG, READ_PKG + ".ReadGiftTest", user);
                runDeviceTests(NONE_PKG, NONE_PKG + ".GiftTest", user);
            }
        } finally {
            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
        }
    }

    /**
     * Test multi-user emulated storage environment, ensuring that each user has
     * isolated storage.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testMultiUserStorageIsolated() throws Exception {
        try {
            if (mUsers.length == 1) {
                Log.d(TAG, "Single user device; skipping isolated storage tests");
                return;
            }

            final int owner = mUsers[0];
            final int secondary = mUsers[1];

            // Install our test app
            getDevice().uninstallPackage(MULTIUSER_PKG);
            String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};
            final String installResult = getDevice()
                    .installPackage(getTestAppFile(MULTIUSER_APK), false, options);
            assertNull("Failed to install: " + installResult, installResult);

            // Clear data from previous tests
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testCleanIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testCleanIsolatedStorage", secondary);

            // Have both users try writing into isolated storage
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testWriteIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testWriteIsolatedStorage", secondary);

            // Verify they both have isolated view of storage
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testReadIsolatedStorage", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testReadIsolatedStorage", secondary);

            // Verify they can't poke at each other
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testUserIsolation", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testUserIsolation", secondary);

            // Verify they can't access other users' content using media provider
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testMediaProviderUserIsolation", owner);
            runDeviceTests(MULTIUSER_PKG, MULTIUSER_CLASS, "testMediaProviderUserIsolation", secondary);
        } finally {
            getDevice().uninstallPackage(MULTIUSER_PKG);
        }
    }

    /**
     * Test that apps with read permissions see the appropriate permissions
     * when apps with r/w permission levels move around their files.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testMultiViewMoveConsistency() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
            final String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};

            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));
            assertNull(getDevice().installPackage(getTestAppFile(READ_APK), false, options));

            for (int user : mUsers) {
                runDeviceTests(READ_PKG, READ_PKG + ".ReadMultiViewTest", "testFolderSetup", user);
            }
            for (int user : mUsers) {
                runDeviceTests(READ_PKG, READ_PKG + ".ReadMultiViewTest", "testRWAccess", user);
            }

            for (int user : mUsers) {
                runDeviceTests(WRITE_PKG, WRITE_PKG + ".WriteMultiViewTest", "testMoveAway", user);
            }
            for (int user : mUsers) {
                runDeviceTests(READ_PKG, READ_PKG + ".ReadMultiViewTest", "testROAccess", user);
            }

            // for fuse file system
            Thread.sleep(10000);
            for (int user : mUsers) {
                runDeviceTests(WRITE_PKG, WRITE_PKG + ".WriteMultiViewTest", "testMoveBack", user);
            }
            for (int user : mUsers) {
                runDeviceTests(READ_PKG, READ_PKG + ".ReadMultiViewTest", "testRWAccess", user);
            }
        } finally {
            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(READ_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
        }
    }

    /** Verify that app without READ_EXTERNAL can play default URIs in external storage. */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testExternalStorageReadDefaultUris() throws Exception {
        try {
            wipePrimaryExternalStorage();

            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
            final String[] options = {AbiUtils.createAbiFlag(getAbi().getName())};

            assertNull(getDevice().installPackage(getTestAppFile(WRITE_APK), false, options));
            assertNull(getDevice().installPackage(getTestAppFile(NONE_APK), false, options));

            for (int user : mUsers) {
                enableWriteSettings(WRITE_PKG, user);
                runDeviceTests(
                        WRITE_PKG, WRITE_PKG + ".ChangeDefaultUris", "testChangeDefaultUris", user);

                runDeviceTests(
                        NONE_PKG, NONE_PKG + ".ReadDefaultUris", "testPlayDefaultUris", user);
            }
        } finally {
            // Make sure the provider and uris are reset on failure.
            for (int user : mUsers) {
                runDeviceTests(
                        WRITE_PKG, WRITE_PKG + ".ChangeDefaultUris", "testResetDefaultUris", user);
            }
            getDevice().uninstallPackage(NONE_PKG);
            getDevice().uninstallPackage(WRITE_PKG);
        }
    }

    /**
     * For security reasons, the shell user cannot access the shared storage of
     * secondary users. Instead, developers should use the {@code content} shell
     * tool to read/write files in those locations.
     */
    @Test
    @AppModeFull // TODO: Needs porting to instant
    public void testSecondaryUsersInaccessible() throws Exception {
        List<String> mounts = new ArrayList<>();
        for (String line : getDevice().executeShellCommand("cat /proc/mounts").split("\n")) {
            String[] split = line.split(" ");
            if (split[1].startsWith("/storage/") || split[1].startsWith("/mnt/")) {
                mounts.add(split[1]);
            }
        }

        for (int user : mUsers) {
            String probe = "/sdcard/../" + user;
            if (user == Utils.USER_SYSTEM) {
                // Primary user should always be visible. Skip checking raw
                // mount points, since we'd get false-positives for physical
                // devices that aren't multi-user aware.
                assertTrue(probe, access(probe));
            } else {
                // Secondary user should never be visible.
                assertFalse(probe, access(probe));
                for (String mount : mounts) {
                    probe = mount + "/" + user;
                    assertFalse(probe, access(probe));
                }
            }
        }
    }

    private boolean access(String path) throws DeviceNotAvailableException {
        final long nonce = System.nanoTime();
        return getDevice().executeShellCommand("ls -la " + path + " && echo " + nonce)
                .contains(Long.toString(nonce));
    }

    private void enableWriteSettings(String packageName, int userId)
            throws DeviceNotAvailableException {
        StringBuilder cmd = new StringBuilder();
        cmd.append("appops set --user ");
        cmd.append(userId);
        cmd.append(" ");
        cmd.append(packageName);
        cmd.append(" android:write_settings allow");
        getDevice().executeShellCommand(cmd.toString());
        try {
            Thread.sleep(2200);
        } catch (InterruptedException e) {
        }
    }

    private void wipePrimaryExternalStorage() throws DeviceNotAvailableException {
        getDevice().executeShellCommand("rm -rf /sdcard/Android");
        getDevice().executeShellCommand("rm -rf /sdcard/DCIM");
        getDevice().executeShellCommand("rm -rf /sdcard/MUST_*");
    }

    private void runDeviceTests(String packageName, String testClassName, int userId)
            throws DeviceNotAvailableException {
        runDeviceTests(getDevice(), packageName, testClassName, null, userId, null);
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName,
            int userId) throws DeviceNotAvailableException {
        runDeviceTests(getDevice(), packageName, testClassName, testMethodName, userId, null);
    }
}
