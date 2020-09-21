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

package android.os.lib.consumer1;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertSame;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.SharedLibraryInfo;
import android.content.pm.VersionedPackage;
import android.os.lib.provider.StaticSharedLib;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.List;

import com.android.compatibility.common.util.SystemUtil;

@RunWith(AndroidJUnit4.class)
public class UseSharedLibraryTest {
    private static final String LIB_NAME = "foo.bar.lib";
    private static final String RECURSIVE_LIB_NAME = "foo.bar.lib.recursive";
    private static final String RECURSIVE_LIB_PROVIDER_NAME = "android.os.lib.provider.recursive";
    private static final String PLATFORM_PACKAGE = "android";

    private static final String STATIC_LIB_PROVIDER_PKG = "android.os.lib.provider";
    private static final String STATIC_LIB_CONSUMER1_PKG = "android.os.lib.consumer1";
    private static final String STATIC_LIB_CONSUMER2_PKG = "android.os.lib.consumer2";

    @Test
    public void testLoadCodeAndResources() {
        assertSame(1, StaticSharedLib.getVersion(InstrumentationRegistry.getContext()));
    }

    @Test
    public void testSharedLibrariesProperlyReported() throws Exception {
        SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), "appops set "
                + InstrumentationRegistry.getInstrumentation().getContext().getPackageName()
                + " REQUEST_INSTALL_PACKAGES allow");

        try {
            List<SharedLibraryInfo> sharedLibs = InstrumentationRegistry.getContext()
                    .getPackageManager().getSharedLibraries(0);

            assertNotNull(sharedLibs);

            boolean firstLibFound = false;
            boolean secondLibFound = false;
            boolean thirdLibFound = false;
            boolean fourthLibFound = false;

            for (SharedLibraryInfo sharedLib : sharedLibs) {
                assertNotNull(sharedLib.getName());

                final int type = sharedLib.getType();
                int typeCount = 0;
                typeCount += type == SharedLibraryInfo.TYPE_BUILTIN ? 1 : 0;
                typeCount += type == SharedLibraryInfo.TYPE_DYNAMIC ? 1 : 0;
                typeCount += type == SharedLibraryInfo.TYPE_STATIC ? 1 : 0;

                if (typeCount != 1) {
                    fail("Library " + sharedLib.getName()
                            + " must be either builtin or dynamic or static");
                }

                if (type == SharedLibraryInfo.TYPE_BUILTIN) {
                    assertSame((long) SharedLibraryInfo.VERSION_UNDEFINED, sharedLib.getLongVersion());
                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    assertEquals(PLATFORM_PACKAGE, declaringPackage.getPackageName());
                    assertSame(0L, declaringPackage.getLongVersionCode());
                }

                if (type == SharedLibraryInfo.TYPE_DYNAMIC) {
                    assertSame((long) SharedLibraryInfo.VERSION_UNDEFINED, sharedLib.getLongVersion());
                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    assertNotNull(declaringPackage.getPackageName());
                    assertTrue(declaringPackage.getLongVersionCode() >= 0);
                }

                if (type == SharedLibraryInfo.TYPE_STATIC) {
                    assertTrue(sharedLib.getLongVersion() >= 0);
                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    assertNotNull(declaringPackage.getPackageName());
                    assertTrue(declaringPackage.getLongVersionCode() >= 0);
                }

                boolean validLibName = false;
                if (LIB_NAME.equals(sharedLib.getName())) {
                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    assertEquals(STATIC_LIB_PROVIDER_PKG, declaringPackage.getPackageName());
                    validLibName = true;
                }
                if (RECURSIVE_LIB_NAME.equals(sharedLib.getName())) {
                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    assertEquals(RECURSIVE_LIB_PROVIDER_NAME, declaringPackage.getPackageName());
                    validLibName = true;
                }

                if (validLibName) {
                    assertTrue(type == SharedLibraryInfo.TYPE_STATIC);

                    VersionedPackage declaringPackage = sharedLib.getDeclaringPackage();
                    List<VersionedPackage> dependentPackages = sharedLib.getDependentPackages();

                    final long versionCode = sharedLib.getLongVersion();
                    if (versionCode == 1) {
                        firstLibFound = true;
                        assertSame(1L, declaringPackage.getLongVersionCode());
                        assertSame(1, dependentPackages.size());
                        VersionedPackage dependentPackage = dependentPackages.get(0);
                        assertEquals(STATIC_LIB_CONSUMER1_PKG, dependentPackage.getPackageName());
                        assertSame(1L, dependentPackage.getLongVersionCode());
                    } else if (versionCode == 2) {
                        secondLibFound = true;
                        assertSame(4L, declaringPackage.getLongVersionCode());
                        assertTrue(dependentPackages.isEmpty());
                    } else if (versionCode == 5) {
                        thirdLibFound = true;
                        assertSame(5L, declaringPackage.getLongVersionCode());
                        assertSame(1, dependentPackages.size());
                        VersionedPackage dependentPackage = dependentPackages.get(0);
                        assertEquals(STATIC_LIB_CONSUMER2_PKG, dependentPackage.getPackageName());
                        assertSame(2L, dependentPackage.getLongVersionCode());
                    } else if (versionCode == 6) {
                        fourthLibFound = true;
                        assertSame(1L, declaringPackage.getLongVersionCode());
                        assertSame(1, dependentPackages.size());
                        VersionedPackage dependentPackage = dependentPackages.get(0);
                        assertEquals(STATIC_LIB_PROVIDER_PKG, dependentPackage.getPackageName());
                        assertSame(1L, dependentPackage.getLongVersionCode());
                    }
                }
            }

            assertTrue("Did not find lib " + LIB_NAME + " version 1", firstLibFound);
            assertTrue("Did not find lib " + LIB_NAME + " version 4", secondLibFound);
            assertTrue("Did not find lib " + LIB_NAME + " version 5", thirdLibFound);
            assertTrue("Did not find lib " + RECURSIVE_LIB_NAME + " version 6", fourthLibFound);
        } finally {
            SystemUtil.runShellCommand(InstrumentationRegistry.getInstrumentation(), "appops set "
                    + InstrumentationRegistry.getInstrumentation().getContext().getPackageName()
                    + " REQUEST_INSTALL_PACKAGES default");
        }
    }

    @Test
    public void testAppCanSeeOnlyLibrariesItDependOn() throws Exception {
        // Make sure we see only the lib we depend on via getting its package info
        PackageInfo libPackageInfo = InstrumentationRegistry.getInstrumentation()
                .getContext().getPackageManager().getPackageInfo(STATIC_LIB_PROVIDER_PKG, 0);
        assertEquals(STATIC_LIB_PROVIDER_PKG, libPackageInfo.packageName);
        assertEquals(1L, libPackageInfo.getLongVersionCode());

        // Make sure we see the lib we depend on via getting installed packages
        List<PackageInfo> installedPackages = InstrumentationRegistry.getInstrumentation()
                .getContext().getPackageManager().getInstalledPackages(0);
        long usedLibraryVersionCode = -1;
        for (PackageInfo installedPackage : installedPackages) {
            if (STATIC_LIB_PROVIDER_PKG.equals(installedPackage.packageName)) {
                if (usedLibraryVersionCode != -1) {
                    fail("Should see only the lib it depends on");
                }
                usedLibraryVersionCode = installedPackage.getLongVersionCode();
            }
        }
        assertEquals(1L, usedLibraryVersionCode);

        // Make sure we see only the lib we depend on via getting its app info
        ApplicationInfo appInfo = InstrumentationRegistry.getInstrumentation()
                .getContext().getPackageManager().getApplicationInfo(STATIC_LIB_PROVIDER_PKG, 0);
        assertEquals(STATIC_LIB_PROVIDER_PKG, appInfo.packageName);
        assertEquals(1L, libPackageInfo.getLongVersionCode());

        // Make sure we see the lib we depend on via getting installed apps
        List<PackageInfo> installedApps = InstrumentationRegistry.getInstrumentation().getContext()
                .getPackageManager().getInstalledPackages(0);
        usedLibraryVersionCode = -1;
        for (PackageInfo installedApp : installedApps) {
            if (STATIC_LIB_PROVIDER_PKG.equals(installedApp.packageName)) {
                if (usedLibraryVersionCode != -1) {
                    fail("Should see only the lib it depends on");
                }
                usedLibraryVersionCode = installedApp.getLongVersionCode();
            }
        }
        assertEquals(1L, usedLibraryVersionCode);
    }
}
