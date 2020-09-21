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
 * limitations under the License
 */

package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Tests for ephemeral packages.
 */
@AppModeFull // Already handles instant installs when needed.
public class EphemeralTest extends DeviceTestCase
        implements IAbiReceiver, IBuildReceiver {

    // a normally installed application
    private static final String NORMAL_APK = "CtsEphemeralTestsNormalApp.apk";
    private static final String NORMAL_PKG = "com.android.cts.normalapp";

    // the first ephemerally installed application
    private static final String EPHEMERAL_1_APK = "CtsEphemeralTestsEphemeralApp1.apk";
    private static final String EPHEMERAL_1_PKG = "com.android.cts.ephemeralapp1";

    // the second ephemerally installed application
    private static final String EPHEMERAL_2_APK = "CtsEphemeralTestsEphemeralApp2.apk";
    private static final String EPHEMERAL_2_PKG = "com.android.cts.ephemeralapp2";

    // a normally installed application with implicitly exposed components
    private static final String IMPLICIT_APK = "CtsEphemeralTestsImplicitApp.apk";
    private static final String IMPLICIT_PKG = "com.android.cts.implicitapp";

    // a normally installed application with no exposed components
    private static final String UNEXPOSED_APK = "CtsEphemeralTestsUnexposedApp.apk";
    private static final String UNEXPOSED_PKG = "com.android.cts.unexposedapp";

    // an application that gets upgraded from 'instant' to 'full'
    private static final String UPGRADED_APK = "CtsInstantUpgradeApp.apk";
    private static final String UPGRADED_PKG = "com.android.cts.instantupgradeapp";

    private static final String TEST_CLASS = ".ClientTest";
    private static final String WEBVIEW_TEST_CLASS = ".WebViewTest";

    private static final List<Map<String, String>> EXPECTED_EXPOSED_INTENTS =
            new ArrayList<>();
    static {
        // Framework intents we expect the system to expose to instant applications
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.CHOOSER",
                null, null));
        // Contact intents we expect the system to expose to instant applications
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.PICK", null, "vnd.android.cursor.dir/contact"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.PICK", null, "vnd.android.cursor.dir/phone_v2"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.PICK", null, "vnd.android.cursor.dir/email_v2"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.PICK", null, "vnd.android.cursor.dir/postal-address_v2"));
        // Storage intents we expect the system to expose to instant applications
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.OPEN_DOCUMENT",
                "android.intent.category.OPENABLE", "\\*/\\*"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.OPEN_DOCUMENT", null, "\\*/\\*"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.GET_CONTENT",
                "android.intent.category.OPENABLE", "\\*/\\*"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.GET_CONTENT", null, "\\*/\\*"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.OPEN_DOCUMENT_TREE", null, null));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.CREATE_DOCUMENT",
                "android.intent.category.OPENABLE", "text/plain"));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.intent.action.CREATE_DOCUMENT", null, "text/plain"));
        /** Camera intents we expect the system to expose to instant applications */
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.media.action.IMAGE_CAPTURE", null, null));
        EXPECTED_EXPOSED_INTENTS.add(makeArgs(
                "android.media.action.VIDEO_CAPTURE", null, null));
    }

    private String mOldVerifierValue;
    private IAbi mAbi;
    private IBuildInfo mBuildInfo;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mBuildInfo = buildInfo;
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        if (isDeviceUnsupported()) {
            return;
        }

        Utils.prepareSingleUser(getDevice());
        assertNotNull(mAbi);
        assertNotNull(mBuildInfo);

        uninstallTestPackages();
        installTestPackages();
    }

    public void tearDown() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        uninstallTestPackages();
        super.tearDown();
    }

    public void testNormalQuery() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(NORMAL_PKG, TEST_CLASS, "testQuery");
    }

    public void testNormalStartNormal() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(NORMAL_PKG, TEST_CLASS, "testStartNormal");
    }

    public void testNormalStartEphemeral() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(NORMAL_PKG, TEST_CLASS, "testStartEphemeral");
    }

    public void testEphemeralQuery() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testQuery");
    }

    public void testEphemeralStartNormal() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartNormal");
    }

    // each connection to an exposed component needs to run in its own test to
    // avoid sharing state. once an instant app is exposed to a component, it's
    // exposed until the device restarts or the instant app is removed.
    public void testEphemeralStartExposed01() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed01");
    }
    public void testEphemeralStartExposed02() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed02");
    }
    public void testEphemeralStartExposed03() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed03");
    }
    public void testEphemeralStartExposed04() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed04");
    }
    public void testEphemeralStartExposed05() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed05");
    }
    public void testEphemeralStartExposed06() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed06");
    }
    public void testEphemeralStartExposed07() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed07");
    }
    public void testEphemeralStartExposed08() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed08");
    }
    public void testEphemeralStartExposed09() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed09");
    }
    public void testEphemeralStartExposed10() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartExposed10");
    }

    public void testEphemeralStartEphemeral() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartEphemeral");
    }

    public void testEphemeralGetInstaller01() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        installEphemeralApp(EPHEMERAL_1_APK, "com.android.cts.normalapp");
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testGetInstaller01");
    }
    public void testEphemeralGetInstaller02() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        installApp(NORMAL_APK, "com.android.cts.normalapp");
        installEphemeralApp(EPHEMERAL_1_APK, "com.android.cts.normalapp");
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testGetInstaller02");
    }
    public void testEphemeralGetInstaller03() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        installApp(NORMAL_APK, "com.android.cts.normalapp");
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testGetInstaller03");
    }

    public void testExposedSystemActivities() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        for (Map<String, String> testArgs : EXPECTED_EXPOSED_INTENTS) {
            final boolean exposed = isIntentExposed(testArgs);
            if (exposed) {
                runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testExposedActivity", testArgs);
            } else {
                CLog.w("Skip intent; " + dumpArgs(testArgs));
            }
        }
    }

    public void testBuildSerialUnknown() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testBuildSerialUnknown");
    }

    public void testPackageInfo() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testPackageInfo");
    }

    public void testActivityInfo() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testActivityInfo");
    }

    public void testWebViewLoads() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, WEBVIEW_TEST_CLASS, "testWebViewLoads");
    }

    public void testInstallPermissionNotGranted() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testInstallPermissionNotGranted");
    }

    public void testInstallPermissionGranted() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testInstallPermissionGranted");
    }

    /** Test for android.permission.INSTANT_APP_FOREGROUND_SERVICE */
    public void testStartForegrondService() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        // Make sure the test package does not have INSTANT_APP_FOREGROUND_SERVICE
        getDevice().executeShellCommand("cmd package revoke " + EPHEMERAL_1_PKG
                        + " android.permission.INSTANT_APP_FOREGROUND_SERVICE");
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testStartForegroundService");
    }

    /** Test for android.permission.RECORD_AUDIO */
    public void testRecordAudioPermission() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testRecordAudioPermission");
    }

    /** Test for android.permission.CAMERA */
    public void testCameraPermission() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testCameraPermission");
    }

    /** Test for android.permission.READ_PHONE_NUMBERS */
    public void testReadPhoneNumbersPermission() throws Exception {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testReadPhoneNumbersPermission");
    }

    /** Test for android.permission.ACCESS_COARSE_LOCATION */
    public void testAccessCoarseLocationPermission() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testAccessCoarseLocationPermission");
    }

    /** Test for android.permission.NETWORK */
    public void testInternetPermission() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testInternetPermission");
    }

    /** Test for android.permission.VIBRATE */
    public void testVibratePermission() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testVibratePermission");
    }

    /** Test for android.permission.WAKE_LOCK */
    public void testWakeLockPermission() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testWakeLockPermission");
    }

    /** Test for search manager */
    public void testGetSearchableInfo() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        runDeviceTests(EPHEMERAL_1_PKG, TEST_CLASS, "testGetSearchableInfo");
    }

    /** Test for upgrade from instant --> full */
    public void testInstantAppUpgrade() throws Throwable {
        if (isDeviceUnsupported()) {
            return;
        }
        installEphemeralApp(UPGRADED_APK);
        runDeviceTests(UPGRADED_PKG, TEST_CLASS, "testInstantApplicationWritePreferences");
        runDeviceTests(UPGRADED_PKG, TEST_CLASS, "testInstantApplicationWriteFile");
        installFullApp(UPGRADED_APK);
        runDeviceTests(UPGRADED_PKG, TEST_CLASS, "testFullApplicationReadPreferences");
        runDeviceTests(UPGRADED_PKG, TEST_CLASS, "testFullApplicationReadFile");
    }

    private static final HashMap<String, String> makeArgs(
            String action, String category, String mimeType) {
        if (action == null || action.length() == 0) {
            fail("action not specified");
        }
        final HashMap<String, String> testArgs = new HashMap<>();
        testArgs.put("action", action);
        if (category != null && category.length() > 0) {
            testArgs.put("category", category);
        }
        if (mimeType != null && mimeType.length() > 0) {
            testArgs.put("mime_type", mimeType);
        }
        return testArgs;
    }

    private static final String[] sUnsupportedFeatures = {
            "feature:android.hardware.type.watch",
            "android.hardware.type.embedded",
    };
    private boolean isDeviceUnsupported() throws Exception {
        for (String unsupportedFeature : sUnsupportedFeatures) {
            if (getDevice().hasFeature(unsupportedFeature)) {
                return true;
            }
        }
        return false;
    }

    private boolean isIntentExposed(Map<String, String> testArgs)
            throws DeviceNotAvailableException {
        final StringBuffer command = new StringBuffer();
        command.append("cmd package query-activities");
        command.append(" -a ").append(testArgs.get("action"));
        if (testArgs.get("category") != null) {
            command.append(" -c ").append(testArgs.get("category"));
        }
        if (testArgs.get("mime_type") != null) {
            command.append(" -t ").append(testArgs.get("mime_type"));
        }
        final String output = getDevice().executeShellCommand(command.toString()).trim();
        return !"No activities found".equals(output);
    }

    private static final String dumpArgs(Map<String, String> testArgs) {
        final StringBuffer dump = new StringBuffer();
        dump.append("action: ").append(testArgs.get("action"));
        if (testArgs.get("category") != null) {
            dump.append(", category: ").append(testArgs.get("category"));
        }
        if (testArgs.get("mime_type") != null) {
            dump.append(", type: ").append(testArgs.get("mime_type"));
        }
        return dump.toString();
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName);
    }

    private void runDeviceTests(String packageName, String testClassName, String testMethodName,
            Map<String, String> testArgs) throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName, testArgs);
    }

    private void installApp(String apk) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        assertNull(getDevice().installPackage(buildHelper.getTestFile(apk), false));
    }

    private void installApp(String apk, String installer) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        assertNull(getDevice().installPackage(buildHelper.getTestFile(apk), false,
                "-i", installer));
    }

    private void installEphemeralApp(String apk) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        assertNull(getDevice().installPackage(buildHelper.getTestFile(apk), false, "--ephemeral"));
    }

    private void installEphemeralApp(String apk, String installer) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        assertNull(getDevice().installPackage(buildHelper.getTestFile(apk), false,
                "--ephemeral", "-i", installer));
    }

    private void installFullApp(String apk) throws Exception {
        CompatibilityBuildHelper buildHelper = new CompatibilityBuildHelper(mBuildInfo);
        assertNull(getDevice().installPackage(buildHelper.getTestFile(apk), true, "--full"));
    }

    private void installTestPackages() throws Exception {
        installApp(NORMAL_APK);
        installApp(UNEXPOSED_APK);
        installApp(IMPLICIT_APK);
        installEphemeralApp(EPHEMERAL_1_APK);
        installEphemeralApp(EPHEMERAL_2_APK);
    }

    private void uninstallTestPackages() throws Exception {
        getDevice().uninstallPackage(NORMAL_PKG);
        getDevice().uninstallPackage(UNEXPOSED_PKG);
        getDevice().uninstallPackage(IMPLICIT_PKG);
        getDevice().uninstallPackage(EPHEMERAL_1_PKG);
        getDevice().uninstallPackage(EPHEMERAL_2_PKG);
        getDevice().uninstallPackage(UPGRADED_PKG);
    }
}
