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

package android.os.cts;

import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AppModeInstant;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.ddmlib.testrunner.RemoteAndroidTestRunner;
import com.android.ddmlib.testrunner.TestResult.TestStatus;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.result.CollectingTestListener;
import com.android.tradefed.result.TestDescription;
import com.android.tradefed.result.TestResult;
import com.android.tradefed.result.TestRunResult;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IBuildReceiver;

import java.io.FileNotFoundException;
import java.util.Map;

public class StaticSharedLibsHostTests extends DeviceTestCase implements IBuildReceiver {
    private static final String ANDROID_JUNIT_RUNNER_CLASS =
            "android.support.test.runner.AndroidJUnitRunner";

    private static final String STATIC_LIB_PROVIDER_RECURSIVE_APK =
            "CtsStaticSharedLibProviderRecursive.apk";
    private static final String STATIC_LIB_PROVIDER_RECURSIVE_PKG =
            "android.os.lib.provider.recursive";

    private static final String STATIC_LIB_PROVIDER1_APK = "CtsStaticSharedLibProviderApp1.apk";
    private static final String STATIC_LIB_PROVIDER1_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER2_APK = "CtsStaticSharedLibProviderApp2.apk";
    private static final String STATIC_LIB_PROVIDER2_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER3_APK = "CtsStaticSharedLibProviderApp3.apk";
    private static final String STATIC_LIB_PROVIDER3_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER4_APK = "CtsStaticSharedLibProviderApp4.apk";
    private static final String STATIC_LIB_PROVIDER4_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER5_APK = "CtsStaticSharedLibProviderApp5.apk";
    private static final String STATIC_LIB_PROVIDER5_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER6_APK = "CtsStaticSharedLibProviderApp6.apk";
    private static final String STATIC_LIB_PROVIDER6_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_PROVIDER7_APK = "CtsStaticSharedLibProviderApp7.apk";
    private static final String STATIC_LIB_PROVIDER7_PKG = "android.os.lib.provider";

    private static final String STATIC_LIB_NATIVE_PROVIDER_APK =
            "CtsStaticSharedNativeLibProvider.apk";
    private static final String STATIC_LIB_NATIVE_PROVIDER_PKG =
            "android.os.lib.provider";

    private static final String STATIC_LIB_NATIVE_PROVIDER_APK1 =
            "CtsStaticSharedNativeLibProvider1.apk";
    private static final String STATIC_LIB_NATIVE_PROVIDER_PKG1 =
            "android.os.lib.provider";

    private static final String STATIC_LIB_CONSUMER1_APK = "CtsStaticSharedLibConsumerApp1.apk";
    private static final String STATIC_LIB_CONSUMER1_PKG = "android.os.lib.consumer1";

    private static final String STATIC_LIB_CONSUMER2_APK = "CtsStaticSharedLibConsumerApp2.apk";
    private static final String STATIC_LIB_CONSUMER2_PKG = "android.os.lib.consumer2";

    private static final String STATIC_LIB_CONSUMER3_APK = "CtsStaticSharedLibConsumerApp3.apk";
    private static final String STATIC_LIB_CONSUMER3_PKG = "android.os.lib.consumer3";

    private static final String STATIC_LIB_NATIVE_CONSUMER_APK
            = "CtsStaticSharedNativeLibConsumer.apk";
    private static final String STATIC_LIB_NATIVE_CONSUMER_PKG
            = "android.os.lib.consumer";

    private CompatibilityBuildHelper mBuildHelper;
    private boolean mInstantMode = false;

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildHelper = new CompatibilityBuildHelper(buildInfo);
    }

    @AppModeInstant
    public void testInstallSharedLibraryInstantMode() throws Exception {
        mInstantMode = true;
        doTestInstallSharedLibrary();
    }

    @AppModeFull
    public void testInstallSharedLibraryFullMode() throws Exception {
        doTestInstallSharedLibrary();
    }

    private void doTestInstallSharedLibrary() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install version 1
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install version 2
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Uninstall version 1
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG));
            // Uninstall version 2
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG));
            // Uninstall dependency
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testCannotInstallSharedLibraryWithMissingDependencyInstantMode() throws Exception {
        mInstantMode = true;
        doTestCannotInstallSharedLibraryWithMissingDependency();
    }

    @AppModeFull
    public void testCannotInstallSharedLibraryWithMissingDependencyFullMode() throws Exception {
        doTestCannotInstallSharedLibraryWithMissingDependency();
    }

    private void doTestCannotInstallSharedLibraryWithMissingDependency() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        try {
            // Install version 1 - should fail - no dependency
            assertNotNull(install(STATIC_LIB_PROVIDER1_APK));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        }
    }

    public void testLoadCodeAndResourcesFromSharedLibraryRecursively() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install the library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install the client
            assertNull(install(STATIC_LIB_CONSUMER1_APK));
            // Try to load code and resources
            runDeviceTests(STATIC_LIB_CONSUMER1_PKG,
                    "android.os.lib.consumer1.UseSharedLibraryTest",
                    "testLoadCodeAndResources");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testCannotUninstallUsedSharedLibrary1InstantMode() throws Exception {
        mInstantMode = true;
        doTestCannotUninstallUsedSharedLibrary1();
    }

    @AppModeFull
    public void testCannotUninstallUsedSharedLibrary1FullMode() throws Exception {
        doTestCannotUninstallUsedSharedLibrary1();
    }

    private void doTestCannotUninstallUsedSharedLibrary1() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install the library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // The library dependency cannot be uninstalled
            assertNotNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG));
            // Now the library dependency can be uninstalled
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG));
            // Uninstall dependency
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testCannotUninstallUsedSharedLibrary2InstantMode() throws Exception {
        mInstantMode = true;
        doTestCannotUninstallUsedSharedLibrary2();
    }

    @AppModeFull
    public void testCannotUninstallUsedSharedLibrary2FullMode() throws Exception {
        doTestCannotUninstallUsedSharedLibrary2();
    }

    private void doTestCannotUninstallUsedSharedLibrary2() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install the library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install the client
            assertNull(install(STATIC_LIB_CONSUMER1_APK));
            // The library cannot be uninstalled
            assertNotNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG));
            // Uninstall the client
            assertNull(getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG));
            // Now the library can be uninstalled
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG));
            // Uninstall dependency
            assertNull(getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testLibraryVersionsAndVersionCodesSameOrderInstantMode() throws Exception {
        mInstantMode = true;
        doTestLibraryVersionsAndVersionCodesSameOrder();
    }

    @AppModeFull
    public void testLibraryVersionsAndVersionCodesSameOrderFullMode() throws Exception {
        doTestLibraryVersionsAndVersionCodesSameOrder();
    }

    private void doTestLibraryVersionsAndVersionCodesSameOrder() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER3_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install library version 1 with version code 1
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install library version 2 with version code 4
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Shouldn't be able to install library version 3 with version code 3
            assertNotNull(install(STATIC_LIB_PROVIDER3_APK));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER3_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testCannotInstallAppWithMissingLibraryInstantMode() throws Exception {
        mInstantMode = true;
        doTestCannotInstallAppWithMissingLibrary();
    }

    @AppModeFull
    public void testCannotInstallAppWithMissingLibraryFullMode() throws Exception {
        doTestCannotInstallAppWithMissingLibrary();
    }

    private void doTestCannotInstallAppWithMissingLibrary() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        try {
            // Shouldn't be able to install an app if a dependency lib is missing
            assertNotNull(install(STATIC_LIB_CONSUMER1_APK));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        }
    }

    @AppModeInstant
    public void testCanReplaceLibraryIfVersionAndVersionCodeSameInstantMode() throws Exception {
        mInstantMode = true;
        doTestCanReplaceLibraryIfVersionAndVersionCodeSame();
    }

    @AppModeFull
    public void testCanReplaceLibraryIfVersionAndVersionCodeSameFullMode() throws Exception {
        doTestCanReplaceLibraryIfVersionAndVersionCodeSame();
    }

    private void doTestCanReplaceLibraryIfVersionAndVersionCodeSame() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install a library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Cannot install the library (need to reinstall)
            assertNotNull(install(STATIC_LIB_PROVIDER1_APK));
            // Can reinstall the library if version and version code same
            assertNull(getDevice().installPackage(mBuildHelper.getTestFile(
                    STATIC_LIB_PROVIDER1_APK), true, false));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testUninstallSpecificLibraryVersionInstantMode() throws Exception {
        mInstantMode = true;
        doTestUninstallSpecificLibraryVersion();
    }

    @AppModeFull
    public void testUninstallSpecificLibraryVersionFullMode() throws Exception {
        doTestUninstallSpecificLibraryVersion();
    }

    private void doTestUninstallSpecificLibraryVersion() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install library version 1 with version code 1
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install library version 2 with version code 4
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Uninstall the library package with version code 4 (version 2)
            assertTrue(getDevice().executeShellCommand("pm uninstall --versionCode 4 "
                    + STATIC_LIB_PROVIDER1_PKG).startsWith("Success"));
            // Uninstall the library package with version code 1 (version 1)
            assertTrue(getDevice().executeShellCommand("pm uninstall "
                    + STATIC_LIB_PROVIDER1_PKG).startsWith("Success"));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testKeyRotationInstantMode() throws Exception {
        mInstantMode = true;
        doTestKeyRotation();
    }

    @AppModeFull
    public void testKeyRotationFullMode() throws Exception {
        doTestKeyRotation();
    }

    private void doTestKeyRotation() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
        try {
            // Install a library version specifying an upgrade key set
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Install a newer library signed with the upgrade key set
            assertNull(install(STATIC_LIB_PROVIDER4_APK));
            // Install a client that depends on the upgraded key set
            assertNull(install(STATIC_LIB_CONSUMER2_APK));
            // Ensure code and resources can be loaded
            runDeviceTests(STATIC_LIB_CONSUMER2_PKG,
                    "android.os.lib.consumer2.UseSharedLibraryTest",
                    "testLoadCodeAndResources");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
        }
    }

    @AppModeInstant
    public void testCannotInstallIncorrectlySignedLibraryInstantMode() throws Exception {
        mInstantMode = true;
        doTestCannotInstallIncorrectlySignedLibrary();
    }

    @AppModeFull
    public void testCannotInstallIncorrectlySignedLibraryFullMode() throws Exception {
        doTestCannotInstallIncorrectlySignedLibrary();
    }

    private void doTestCannotInstallIncorrectlySignedLibrary() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install a library version not specifying an upgrade key set
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Shouldn't be able to install a newer version signed differently
            assertNotNull(install(STATIC_LIB_PROVIDER4_APK));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testLibraryAndPackageNameCanMatchInstantMode() throws Exception {
        mInstantMode = true;
        doTestLibraryAndPackageNameCanMatch();
    }

    @AppModeFull
    public void testLibraryAndPackageNameCanMatchFullMode() throws Exception {
        doTestLibraryAndPackageNameCanMatch();
    }

    private void doTestLibraryAndPackageNameCanMatch() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER5_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER6_PKG);
        try {
            // Install a library with same name as package should work.
            assertNull(install(STATIC_LIB_PROVIDER5_APK));
            // Install a library with same name as package should work.
            assertNull(getDevice().installPackage(mBuildHelper.getTestFile(
                    STATIC_LIB_PROVIDER6_APK), true, false));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER5_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER6_PKG);
        }
    }

    @AppModeInstant
    public void testGetSharedLibrariesInstantMode() throws Exception {
        mInstantMode = true;
        doTestGetSharedLibraries();
    }

    @AppModeFull
    public void testGetSharedLibrariesFullMode() throws Exception {
        doTestGetSharedLibraries();
    }

    private void doTestGetSharedLibraries() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install the first library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install the second library
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Install the third library
            assertNull(install(STATIC_LIB_PROVIDER4_APK));
            // Install the first client
            assertNull(install(STATIC_LIB_CONSUMER1_APK));
            // Install the second client
            assertNull(install(STATIC_LIB_CONSUMER2_APK));
            // Ensure libraries are properly reported
            runDeviceTests(STATIC_LIB_CONSUMER1_PKG,
                    "android.os.lib.consumer1.UseSharedLibraryTest",
                    "testSharedLibrariesProperlyReported");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER4_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testAppCanSeeOnlyLibrariesItDependOnInstantMode() throws Exception {
        mInstantMode = true;
        doTestAppCanSeeOnlyLibrariesItDependOn();
    }

    @AppModeFull
    public void testAppCanSeeOnlyLibrariesItDependOnFullMode() throws Exception {
        doTestAppCanSeeOnlyLibrariesItDependOn();
    }

    private void doTestAppCanSeeOnlyLibrariesItDependOn() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        try {
            // Install library dependency
            assertNull(install(STATIC_LIB_PROVIDER_RECURSIVE_APK));
            // Install the first library
            assertNull(install(STATIC_LIB_PROVIDER1_APK));
            // Install the second library
            assertNull(install(STATIC_LIB_PROVIDER2_APK));
            // Install the client
            assertNull(install(STATIC_LIB_CONSUMER1_APK));
            // Ensure the client can see only the lib it depends on
            runDeviceTests(STATIC_LIB_CONSUMER1_PKG,
                    "android.os.lib.consumer1.UseSharedLibraryTest",
                    "testAppCanSeeOnlyLibrariesItDependOn");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER1_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER2_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER_RECURSIVE_PKG);
        }
    }

    @AppModeInstant
    public void testLoadCodeFromNativeLibInstantMode() throws Exception {
        mInstantMode = true;
        doTestLoadCodeFromNativeLib();
    }

    @AppModeFull
    public void testLoadCodeFromNativeLibFullMode() throws Exception {
        doTestLoadCodeFromNativeLib();
    }

    private void doTestLoadCodeFromNativeLib() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_NATIVE_CONSUMER_PKG);
        getDevice().uninstallPackage(STATIC_LIB_NATIVE_PROVIDER_PKG);
        try {
            // Install library
            assertNull(install(STATIC_LIB_NATIVE_PROVIDER_APK));
            // Install the library client
            assertNull(install(STATIC_LIB_NATIVE_CONSUMER_APK));
            // Ensure the client can load native code from the library
            runDeviceTests(STATIC_LIB_NATIVE_CONSUMER_PKG,
                    "android.os.lib.consumer.UseSharedLibraryTest",
                    "testLoadNativeCode");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_NATIVE_CONSUMER_PKG);
            getDevice().uninstallPackage(STATIC_LIB_NATIVE_PROVIDER_PKG);
        }
    }

    @AppModeInstant
    public void testLoadCodeFromNativeLibMultiArchViolationInstantMode() throws Exception {
        mInstantMode = true;
        doTestLoadCodeFromNativeLibMultiArchViolation();
    }

    @AppModeFull
    public void testLoadCodeFromNativeLibMultiArchViolationFullMode() throws Exception {
        doTestLoadCodeFromNativeLibMultiArchViolation();
    }

    private void doTestLoadCodeFromNativeLibMultiArchViolation() throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_NATIVE_PROVIDER_PKG1);
        try {
            // Cannot install the library with native code if not multi-arch
            assertNotNull(install(STATIC_LIB_NATIVE_PROVIDER_APK1));
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_NATIVE_PROVIDER_PKG1);
        }
    }

    @AppModeInstant
    public void testLoadCodeAndResourcesFromSharedLibrarySignedWithTwoCertsInstantMode() throws Exception {
        mInstantMode = true;
        doTestLoadCodeAndResourcesFromSharedLibrarySignedWithTwoCerts();
    }

    @AppModeFull
    public void testLoadCodeAndResourcesFromSharedLibrarySignedWithTwoCertsFullMode() throws Exception {
        doTestLoadCodeAndResourcesFromSharedLibrarySignedWithTwoCerts();
    }

    private void doTestLoadCodeAndResourcesFromSharedLibrarySignedWithTwoCerts()
            throws Exception {
        getDevice().uninstallPackage(STATIC_LIB_CONSUMER3_PKG);
        getDevice().uninstallPackage(STATIC_LIB_PROVIDER7_PKG);
        try {
            // Install the library
            assertNull(install(STATIC_LIB_PROVIDER7_APK));
            // Install the client
            assertNull(install(STATIC_LIB_CONSUMER3_APK));
            // Try to load code and resources
            runDeviceTests(STATIC_LIB_CONSUMER3_PKG,
                    "android.os.lib.consumer3.UseSharedLibraryTest",
                    "testLoadCodeAndResources");
        } finally {
            getDevice().uninstallPackage(STATIC_LIB_CONSUMER3_PKG);
            getDevice().uninstallPackage(STATIC_LIB_PROVIDER7_PKG);
        }
    }

    private void runDeviceTests(String packageName, String testClassName,
            String testMethodName) throws DeviceNotAvailableException {
        RemoteAndroidTestRunner testRunner = new RemoteAndroidTestRunner(packageName,
                ANDROID_JUNIT_RUNNER_CLASS, getDevice().getIDevice());
        testRunner.setMethodName(testClassName, testMethodName);
        CollectingTestListener listener = new CollectingTestListener();

        getDevice().runInstrumentationTests(testRunner, listener);

        final TestRunResult result = listener.getCurrentRunResults();
        if (result.isRunFailure()) {
            throw new AssertionError("Failed to successfully run device tests for "
                    + result.getName() + ": " + result.getRunFailureMessage());
        }
        if (result.getNumTests() == 0) {
            throw new AssertionError("No tests were run on the device");
        }
        if (result.hasFailedTests()) {
            // build a meaningful error message
            StringBuilder errorBuilder = new StringBuilder("on-device tests failed:\n");
            for (Map.Entry<TestDescription, TestResult> resultEntry :
                    result.getTestResults().entrySet()) {
                if (!resultEntry.getValue().getStatus().equals(TestStatus.PASSED)) {
                    errorBuilder.append(resultEntry.getKey().toString());
                    errorBuilder.append(":\n");
                    errorBuilder.append(resultEntry.getValue().getStackTrace());
                }
            }
            throw new AssertionError(errorBuilder.toString());
        }
    }

    private String install(String apk) throws DeviceNotAvailableException, FileNotFoundException {
        return getDevice().installPackage(mBuildHelper.getTestFile(apk), false, false,
                apk.contains("consumer") && mInstantMode ? "--instant" : "");
    }
}
