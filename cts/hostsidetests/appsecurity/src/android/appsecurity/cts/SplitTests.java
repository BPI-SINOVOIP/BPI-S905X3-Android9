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

package android.appsecurity.cts;

import android.platform.test.annotations.AppModeFull;
import android.platform.test.annotations.AppModeInstant;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.testtype.DeviceTestCase;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IBuildReceiver;

import java.util.HashMap;

/**
 * Tests that verify installing of various split APKs from host side.
 */
public class SplitTests extends DeviceTestCase implements IAbiReceiver, IBuildReceiver {
    static final String PKG_NO_RESTART = "com.android.cts.norestart";
    static final String APK_NO_RESTART_BASE = "CtsNoRestartBase.apk";
    static final String APK_NO_RESTART_FEATURE = "CtsNoRestartFeature.apk";

    static final String PKG = "com.android.cts.splitapp";
    static final String CLASS = PKG + ".SplitAppTest";
    static final String CLASS_NO_RESTART = PKG_NO_RESTART + ".NoRestartTest";

    static final String APK = "CtsSplitApp.apk";

    static final String APK_mdpi = "CtsSplitApp_mdpi-v4.apk";
    static final String APK_hdpi = "CtsSplitApp_hdpi-v4.apk";
    static final String APK_xhdpi = "CtsSplitApp_xhdpi-v4.apk";
    static final String APK_xxhdpi = "CtsSplitApp_xxhdpi-v4.apk";

    private static final String APK_v7 = "CtsSplitApp_v7.apk";
    private static final String APK_fr = "CtsSplitApp_fr.apk";
    private static final String APK_de = "CtsSplitApp_de.apk";

    private static final String APK_x86 = "CtsSplitApp_x86.apk";
    private static final String APK_x86_64 = "CtsSplitApp_x86_64.apk";
    private static final String APK_armeabi_v7a = "CtsSplitApp_armeabi-v7a.apk";
    private static final String APK_armeabi = "CtsSplitApp_armeabi.apk";
    private static final String APK_arm64_v8a = "CtsSplitApp_arm64-v8a.apk";
    private static final String APK_mips64 = "CtsSplitApp_mips64.apk";
    private static final String APK_mips = "CtsSplitApp_mips.apk";

    private static final String APK_DIFF_REVISION = "CtsSplitAppDiffRevision.apk";
    private static final String APK_DIFF_REVISION_v7 = "CtsSplitAppDiffRevision_v7.apk";

    private static final String APK_DIFF_VERSION = "CtsSplitAppDiffVersion.apk";
    private static final String APK_DIFF_VERSION_v7 = "CtsSplitAppDiffVersion_v7.apk";

    private static final String APK_DIFF_CERT = "CtsSplitAppDiffCert.apk";
    private static final String APK_DIFF_CERT_v7 = "CtsSplitAppDiffCert_v7.apk";

    private static final String APK_FEATURE = "CtsSplitAppFeature.apk";
    private static final String APK_FEATURE_v7 = "CtsSplitAppFeature_v7.apk";

    static final HashMap<String, String> ABI_TO_APK = new HashMap<>();

    static {
        ABI_TO_APK.put("x86", APK_x86);
        ABI_TO_APK.put("x86_64", APK_x86_64);
        ABI_TO_APK.put("armeabi-v7a", APK_armeabi_v7a);
        ABI_TO_APK.put("armeabi", APK_armeabi);
        ABI_TO_APK.put("arm64-v8a", APK_arm64_v8a);
        ABI_TO_APK.put("mips64", APK_mips64);
        ABI_TO_APK.put("mips", APK_mips);
    }

    private IAbi mAbi;
    private IBuildInfo mCtsBuild;

    @Override
    public void setAbi(IAbi abi) {
        mAbi = abi;
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        mCtsBuild = buildInfo;
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();

        Utils.prepareSingleUser(getDevice());
        assertNotNull(mAbi);
        assertNotNull(mCtsBuild);

        getDevice().uninstallPackage(PKG);
    }

    @Override
    protected void tearDown() throws Exception {
        super.tearDown();

        getDevice().uninstallPackage(PKG);
        getDevice().uninstallPackage(PKG_NO_RESTART);
    }

    @AppModeInstant
    public void testSingleBaseInstant() throws Exception {
        testSingleBase(true);
    }

    @AppModeFull
    public void testSingleBaseFull() throws Exception {
        testSingleBase(false);
    }

    private void testSingleBase(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testSingleBase");
    }

    @AppModeInstant
    public void testDensitySingleInstant() throws Exception {
        testDensitySingle(true);
    }

    @AppModeFull
    public void testDensitySingleFull() throws Exception {
        testDensitySingle(false);
    }

    private void testDensitySingle(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_mdpi)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testDensitySingle");
    }

    @AppModeInstant
    public void testDensityAllInstant() throws Exception {
        testDensityAll(true);
    }

    @AppModeFull
    public void testDensityAllFull() throws Exception {
        testDensityAll(false);
    }

    private void testDensityAll(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_mdpi).addApk(APK_hdpi).addApk(APK_xhdpi)
                .addApk(APK_xxhdpi).addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testDensityAll");
    }

    /**
     * Install first with low-resolution resources, then add a split that offers
     * higher-resolution resources.
     */
    @AppModeInstant
    public void testDensityBestInstant() throws Exception {
        testDensityBest(true);
    }

    /**
     * Install first with low-resolution resources, then add a split that offers
     * higher-resolution resources.
     */
    @AppModeFull
    public void testDensityBestFull() throws Exception {
        testDensityBest(false);
    }

    private void testDensityBest(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_mdpi)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testDensityBest1");

        // Now splice in an additional split which offers better resources
        new InstallMultiple().inheritFrom(PKG).addApk(APK_xxhdpi)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testDensityBest2");
    }

    /**
     * Verify that an API-based split can change enabled/disabled state of
     * manifest elements.
     */
    @AppModeInstant
    public void testApiInstant() throws Exception {
        testApi(true);
    }

    /**
     * Verify that an API-based split can change enabled/disabled state of
     * manifest elements.
     */
    @AppModeFull
    public void testApiFull() throws Exception {
        testApi(false);
    }

    private void testApi(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testApi");
    }

    @AppModeInstant
    public void testLocaleInstant() throws Exception {
        testLocale(true);
    }

    @AppModeFull
    public void testLocaleFull() throws Exception {
        testLocale(false);
    }

    private void testLocale(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_de).addApk(APK_fr)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testLocale");
    }

    /**
     * Install test app with <em>single</em> split that exactly matches the
     * currently active ABI. This also explicitly forces ABI when installing.
     */
    @AppModeInstant
    public void testNativeSingleInstant() throws Exception {
        testNativeSingle(true);
    }

    /**
     * Install test app with <em>single</em> split that exactly matches the
     * currently active ABI. This also explicitly forces ABI when installing.
     */
    @AppModeFull
    public void testNativeSingleFull() throws Exception {
        testNativeSingle(false);
    }

    private void testNativeSingle(boolean instant) throws Exception {
        final String abi = mAbi.getName();
        final String apk = ABI_TO_APK.get(abi);
        assertNotNull("Failed to find APK for ABI " + abi, apk);

        new InstallMultiple().addApk(APK).addApk(apk)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testNative");
    }

    /**
     * Install test app with <em>single</em> split that exactly matches the
     * currently active ABI. This variant <em>does not</em> force the ABI when
     * installing, instead exercising the system's ability to choose the ABI
     * through inspection of the installed app.
     */
    @AppModeInstant
    public void testNativeSingleNaturalInstant() throws Exception {
        testNativeSingleNatural(true);
    }

    /**
     * Install test app with <em>single</em> split that exactly matches the
     * currently active ABI. This variant <em>does not</em> force the ABI when
     * installing, instead exercising the system's ability to choose the ABI
     * through inspection of the installed app.
     */
    @AppModeFull
    public void testNativeSingleNaturalFull() throws Exception {
        testNativeSingleNatural(false);
    }

    private void testNativeSingleNatural(boolean instant) throws Exception {
        final String abi = mAbi.getName();
        final String apk = ABI_TO_APK.get(abi);
        assertNotNull("Failed to find APK for ABI " + abi, apk);

        new InstallMultiple().useNaturalAbi().addApk(APK).addApk(apk)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testNative");
    }

    /**
     * Install test app with <em>all</em> possible ABI splits. This also
     * explicitly forces ABI when installing.
     */
    @AppModeInstant
    public void testNativeAllInstant() throws Exception {
        testNativeAll(true, false);
    }

    /**
     * Install test app with <em>all</em> possible ABI splits. This also
     * explicitly forces ABI when installing.
     */
    @AppModeFull
    public void testNativeAllFull() throws Exception {
        testNativeAll(false, false);
    }

    /**
     * Install test app with <em>all</em> possible ABI splits. This variant
     * <em>does not</em> force the ABI when installing, instead exercising the
     * system's ability to choose the ABI through inspection of the installed
     * app.
     */
    @AppModeInstant
    public void testNativeAllNaturalInstant() throws Exception {
        testNativeAll(true, true);
    }

    /**
     * Install test app with <em>all</em> possible ABI splits. This variant
     * <em>does not</em> force the ABI when installing, instead exercising the
     * system's ability to choose the ABI through inspection of the installed
     * app.
     */
    @AppModeFull
    public void testNativeAllNaturalFull() throws Exception {
        testNativeAll(false, true);
    }

    private void testNativeAll(boolean instant, boolean natural) throws Exception {
        final InstallMultiple inst = new InstallMultiple().addApk(APK);
        for (String apk : ABI_TO_APK.values()) {
            inst.addApk(apk);
        }
        if (instant) {
            inst.addArg("--instant");
        }
        if (natural) {
            inst.useNaturalAbi();
        }
        inst.run();
        runDeviceTests(PKG, CLASS, "testNative");
    }

    @AppModeInstant
    public void testDuplicateBaseInstant() throws Exception {
        testDuplicateBase(true);
    }

    @AppModeFull
    public void testDuplicateBaseFull() throws Exception {
        testDuplicateBase(false);
    }

    private void testDuplicateBase(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDuplicateSplit() throws Exception {
        testDuplicateSplit(true);
    }

    @AppModeFull
    public void testDuplicateSplitFull() throws Exception {
        testDuplicateSplit(false);
    }

    private void testDuplicateSplit(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_v7).addApk(APK_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDiffCertInstant() throws Exception {
        testDiffCert(true);
    }

    @AppModeFull
    public void testDiffCertFull() throws Exception {
        testDiffCert(false);
    }

    private void testDiffCert(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_DIFF_CERT_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDiffCertInheritInstant() throws Exception {
        testDiffCertInherit(true);
    }

    @AppModeFull
    public void testDiffCertInheritFull() throws Exception {
        testDiffCertInherit(false);
    }

    private void testDiffCertInherit(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK) .addArg(instant ? "--instant" : "").run();
        new InstallMultiple().inheritFrom(PKG).addApk(APK_DIFF_CERT_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDiffVersionInstant() throws Exception {
        testDiffVersion(true);
    }

    @AppModeFull
    public void testDiffVersionFull() throws Exception {
        testDiffVersion(false);
    }

    private void testDiffVersion(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_DIFF_VERSION_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDiffVersionInheritInstant() throws Exception {
        testDiffVersionInherit(true);
    }

    @AppModeFull
    public void testDiffVersionInheritFull() throws Exception {
        testDiffVersionInherit(false);
    }

    private void testDiffVersionInherit(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addArg(instant ? "--instant" : "").run();
        new InstallMultiple().inheritFrom(PKG).addApk(APK_DIFF_VERSION_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testDiffRevisionInstant() throws Exception {
        testDiffRevision(true);
    }

    @AppModeFull
    public void testDiffRevisionFull() throws Exception {
        testDiffRevision(false);
    }

    private void testDiffRevision(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_DIFF_REVISION_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testRevision0_12");
    }

    @AppModeInstant
    public void testDiffRevisionInheritBaseInstant() throws Exception {
        testDiffRevisionInheritBase(true);
    }

    @AppModeFull
    public void testDiffRevisionInheritBaseFull() throws Exception {
        testDiffRevisionInheritBase(false);
    }

    private void testDiffRevisionInheritBase(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testRevision0_0");
        new InstallMultiple().inheritFrom(PKG).addApk(APK_DIFF_REVISION_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testRevision0_12");
    }

    @AppModeInstant
    public void testDiffRevisionInheritSplitInstant() throws Exception {
        testDiffRevisionInheritSplit(true);
    }

    @AppModeFull
    public void testDiffRevisionInheritSplitFull() throws Exception {
        testDiffRevisionInheritSplit(false);
    }

    private void testDiffRevisionInheritSplit(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testRevision0_0");
        new InstallMultiple().inheritFrom(PKG).addApk(APK_DIFF_REVISION)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testRevision12_0");
    }

    @AppModeInstant
    public void testDiffRevisionDowngradeInstant() throws Exception {
        testDiffRevisionDowngrade(true);
    }

    @AppModeFull
    public void testDiffRevisionDowngradeFull() throws Exception {
        testDiffRevisionDowngrade(false);
    }

    private void testDiffRevisionDowngrade(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_DIFF_REVISION_v7)
                .addArg(instant ? "--instant" : "").run();
        new InstallMultiple().inheritFrom(PKG).addApk(APK_v7)
                .addArg(instant ? "--instant" : "").runExpectingFailure();
    }

    @AppModeInstant
    public void testFeatureBaseInstant() throws Exception {
        testFeatureBase(true);
    }

    @AppModeFull
    public void testFeatureBaseFull() throws Exception {
        testFeatureBase(false);
    }

    private void testFeatureBase(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_FEATURE)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testFeatureBase");
    }

    @AppModeInstant
    public void testFeatureApiInstant() throws Exception {
        testFeatureApiInstant(true);
    }

    @AppModeFull
    public void testFeatureApiFull() throws Exception {
        testFeatureApiInstant(false);
    }

    private void testFeatureApiInstant(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).addApk(APK_FEATURE).addApk(APK_FEATURE_v7)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testFeatureApi");
    }

    @AppModeFull
    @AppModeInstant
    public void testInheritUpdatedBase() throws Exception {
        // TODO: flesh out this test
    }

    @AppModeFull
    @AppModeInstant
    public void testInheritUpdatedSplit() throws Exception {
        // TODO: flesh out this test
    }

    @AppModeInstant
    public void testFeatureWithoutRestartInstant() throws Exception {
        testFeatureWithoutRestart(true);
    }

    @AppModeFull
    public void testFeatureWithoutRestartFull() throws Exception {
        testFeatureWithoutRestart(false);
    }

    private void testFeatureWithoutRestart(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK).run();

        new InstallMultiple().addApk(APK_NO_RESTART_BASE)
                .addArg(instant ? "--instant" : "").run();

        if (instant) {
            // Poke the full app so it can see the instant app.
            runDeviceTests(PKG_NO_RESTART, CLASS_NO_RESTART, "testPokeFullApp");
        }

        runDeviceTests(PKG, CLASS, "testBaseInstalled");

        new InstallMultiple()
                .addArg(instant ? "--instant" : "")
                .addArg("--dont-kill")
                .inheritFrom(PKG_NO_RESTART)
                .addApk(APK_NO_RESTART_FEATURE)
                .run();
        runDeviceTests(PKG, CLASS, "testFeatureInstalled");
    }

    /**
     * Verify that installing a new version of app wipes code cache.
     */
    @AppModeInstant
    public void testClearCodeCacheInstant() throws Exception {
        testClearCodeCache(true);
    }

    /**
     * Verify that installing a new version of app wipes code cache.
     */
    @AppModeFull
    public void testClearCodeCacheFull() throws Exception {
        testClearCodeCache(false);
    }

    private void testClearCodeCache(boolean instant) throws Exception {
        new InstallMultiple().addApk(APK)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testCodeCacheWrite");
        new InstallMultiple().addArg("-r").addApk(APK_DIFF_VERSION)
                .addArg(instant ? "--instant" : "").run();
        runDeviceTests(PKG, CLASS, "testCodeCacheRead");
    }

    private class InstallMultiple extends BaseInstallMultiple<InstallMultiple> {
        public InstallMultiple() {
            super(getDevice(), mCtsBuild, mAbi);
        }
    }

    public void runDeviceTests(String packageName, String testClassName, String testMethodName)
            throws DeviceNotAvailableException {
        Utils.runDeviceTests(getDevice(), packageName, testClassName, testMethodName);
    }
}
