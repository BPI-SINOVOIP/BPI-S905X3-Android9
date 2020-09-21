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
package com.android.compatibility.common.tradefed.build;

import com.android.annotations.VisibleForTesting;
import com.android.tradefed.build.BuildRetrievalError;
import com.android.tradefed.build.DeviceBuildInfo;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IBuildInfo.BuildInfoProperties;
import com.android.tradefed.build.IBuildProvider;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.build.IDeviceBuildProvider;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.testtype.IInvocationContextReceiver;
import com.android.tradefed.testtype.suite.TestSuiteInfo;
import com.android.tradefed.util.FileUtil;

import java.io.File;
import java.io.IOException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.regex.Pattern;
/**
 * A simple {@link IBuildProvider} that uses a pre-existing Compatibility install.
 */
@OptionClass(alias="compatibility-build-provider")
public class CompatibilityBuildProvider implements IDeviceBuildProvider, IInvocationContextReceiver {

    private static final Pattern RELEASE_BUILD = Pattern.compile("^[A-Z]{3}\\d{2}[A-Z]{0,1}$");
    private static final String ROOT_DIR = "ROOT_DIR";
    private static final String SUITE_BUILD = "SUITE_BUILD";
    private static final String SUITE_NAME = "SUITE_NAME";
    private static final String SUITE_FULL_NAME = "SUITE_FULL_NAME";
    private static final String SUITE_VERSION = "SUITE_VERSION";
    private static final String SUITE_PLAN = "SUITE_PLAN";
    private static final String RESULT_DIR = "RESULT_DIR";
    private static final String START_TIME_MS = "START_TIME_MS";
    public static final String DYNAMIC_CONFIG_OVERRIDE_URL = "DYNAMIC_CONFIG_OVERRIDE_URL";

    /* API Key for compatibility test project, used for dynamic configuration */
    private static final String API_KEY = "AIzaSyAbwX5JRlmsLeygY2WWihpIJPXFLueOQ3U";

    @Option(name="branch", description="build branch name to supply.")
    private String mBranch = null;

    @Option(name = "build-id",
            description =
                    "build version number to supply. Override the default cts version number.")
    private String mBuildId = null;

    @Option(name="build-flavor", description="build flavor name to supply.")
    private String mBuildFlavor = null;

    @Option(name="build-target", description="build target name to supply.")
    private String mBuildTarget = null;

    @Option(name="build-attribute", description="build attributes to supply.")
    private Map<String, String> mBuildAttributes = new HashMap<String,String>();

    @Option(name="use-device-build-info", description="Bootstrap build info from device")
    private boolean mUseDeviceBuildInfo = false;

    @Option(name = "dynamic-config-url",
            description = "Specify the url for override config")
    private String mURL = "https://androidpartner.googleapis.com/v1/dynamicconfig/"
            + "suites/{suite-name}/modules/{module}/version/{version}?key=" + API_KEY;

    @Option(name = "url-suite-name-override",
            description = "Override the name that should used to replace the {suite-name} "
                    + "pattern in the dynamic-config-url.")
    private String mUrlSuiteNameOverride = null;

    @Option(name = "plan",
            description = "the test suite plan to run, such as \"everything\" or \"cts\"",
            importance = Importance.ALWAYS)
    private String mSuitePlan;

    private String mTestTag;
    private File mArtificialRootDir;

    /**
     * Util method to inject build attributes into supplied {@link IBuildInfo}
     * @param buildInfo
     */
    private void injectBuildAttributes(IBuildInfo buildInfo) {
        for (Map.Entry<String, String> entry : mBuildAttributes.entrySet()) {
            buildInfo.addBuildAttribute(entry.getKey(), entry.getValue());
        }
        if (mTestTag != null) {
            buildInfo.setTestTag(mTestTag);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setInvocationContext(IInvocationContext invocationContext) {
        mTestTag = invocationContext.getTestTag();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild() throws BuildRetrievalError {
        // Create a blank BuildInfo which will get populated later.
        String version = null;
        if (mBuildId != null) {
            version = mBuildId;
        } else {
            version = getSuiteInfoBuildNumber();
            if (version == null) {
                version = IBuildInfo.UNKNOWN_BUILD_ID;
            }
        }
        IBuildInfo ctsBuild = new DeviceBuildInfo(version, mBuildTarget);
        if (mBranch  != null) {
            ctsBuild.setBuildBranch(mBranch);
        }
        if (mBuildFlavor != null) {
            ctsBuild.setBuildFlavor(mBuildFlavor);
        }
        injectBuildAttributes(ctsBuild);
        addCompatibilitySuiteInfo(ctsBuild);
        return ctsBuild;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public IBuildInfo getBuild(ITestDevice device)
            throws BuildRetrievalError, DeviceNotAvailableException {
        if (!mUseDeviceBuildInfo) {
            // return a regular build info without extracting device attributes into standard
            // build info fields
            return getBuild();
        } else {
            if (mBuildId == null) {
                mBuildId = device.getBuildId();
            }
            if (mBuildFlavor == null) {
                mBuildFlavor = device.getBuildFlavor();
            }
            if (mBuildTarget == null) {
                String name = device.getProperty("ro.product.name");
                String variant = device.getProperty("ro.build.type");
                mBuildTarget = name + "-" + variant;
            }
            IBuildInfo info = new DeviceBuildInfo(mBuildId, mBuildTarget);
            if (mBranch == null) {
                // if branch is not specified via param, make a pseudo branch name based on platform
                // version and product info from device
                mBranch = String.format("%s-%s-%s-%s",
                        device.getProperty("ro.product.brand"),
                        device.getProperty("ro.product.name"),
                        device.getProductVariant(),
                        device.getProperty("ro.build.version.release"));
            }
            info.setBuildBranch(mBranch);
            info.setBuildFlavor(mBuildFlavor);
            String buildAlias = device.getBuildAlias();
            if (RELEASE_BUILD.matcher(buildAlias).matches()) {
                info.addBuildAttribute("build_alias", buildAlias);
            }
            injectBuildAttributes(info);
            addCompatibilitySuiteInfo(info);
            return info;
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void buildNotTested(IBuildInfo info) {
        // ignore
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void cleanUp(IBuildInfo info) {
        // Everything should have been copied properly to result folder, we clean up
        if (info instanceof IDeviceBuildInfo) {
            List<File> doNotDelete = new ArrayList<>();
            // Clean up everything except the tests dir
            doNotDelete.add(((IDeviceBuildInfo) info).getTestsDir());
            info.cleanUp(doNotDelete);
        } else {
            info.cleanUp();
        }
        FileUtil.recursiveDelete(mArtificialRootDir);
    }

    private void addCompatibilitySuiteInfo(IBuildInfo info) {
        long startTimeMs = System.currentTimeMillis();
        info.addBuildAttribute(SUITE_BUILD, getSuiteInfoBuildNumber());
        info.addBuildAttribute(SUITE_NAME, getSuiteInfoName());
        info.addBuildAttribute(SUITE_FULL_NAME, getSuiteInfoFullname());
        info.addBuildAttribute(SUITE_VERSION, getSuiteInfoVersion());
        info.addBuildAttribute(SUITE_PLAN, mSuitePlan);
        info.addBuildAttribute(START_TIME_MS, Long.toString(startTimeMs));
        info.addBuildAttribute(RESULT_DIR, getDirSuffix(startTimeMs));
        String rootDirPath = getRootDirPath();
        if (rootDirPath == null || rootDirPath.trim().equals("")) {
            throw new IllegalArgumentException(
                    String.format("Missing install path property %s_ROOT", getSuiteInfoName()));
        }
        File rootDir = new File(rootDirPath);
        if (!rootDir.exists()) {
            throw new IllegalArgumentException(
                    String.format("Root directory doesn't exist %s", rootDir.getAbsolutePath()));
        }
        info.addBuildAttribute(ROOT_DIR, rootDir.getAbsolutePath());
        // For DeviceBuildInfo we populate the testsDir folder of the build info.
        if (info instanceof IDeviceBuildInfo) {
            if (mArtificialRootDir == null) {
                // If the real CTS directory is used, do not copy it again.
                info.setProperties(
                        BuildInfoProperties.DO_NOT_LINK_TESTS_DIR,
                        BuildInfoProperties.DO_NOT_COPY_ON_SHARDING);
            } else {
                info.setProperties(BuildInfoProperties.DO_NOT_COPY_ON_SHARDING);
            }
            File testDir = new File(rootDir, String.format("android-%s/testcases/",
                    getSuiteInfoName().toLowerCase()));
            ((IDeviceBuildInfo) info).setTestsDir(testDir, "0");
        }
        if (mURL != null && !mURL.isEmpty()) {
            String suiteName = mUrlSuiteNameOverride;
            if (suiteName == null) {
                suiteName = getSuiteInfoName();
            }
            info.addBuildAttribute(DYNAMIC_CONFIG_OVERRIDE_URL,
                    mURL.replace("{suite-name}", suiteName));
        }
    }

    /**
     * Returns the CTS_ROOT variable that the harness was started with.
     */
    @VisibleForTesting
    String getRootDirPath() {
        String varName = String.format("%s_ROOT", getSuiteInfoName());
        String rootDirVariable = System.getProperty(varName);
        if (rootDirVariable != null) {
            return rootDirVariable;
        }
        // Create an artificial root dir, we are most likely running from Tradefed directly.
        try {
            mArtificialRootDir = FileUtil.createTempDir(
                    String.format("%s-root-dir", getSuiteInfoName()));
            new File(mArtificialRootDir, String.format("android-%s/testcases",
                    getSuiteInfoName().toLowerCase())).mkdirs();
            return mArtificialRootDir.getAbsolutePath();
        } catch (IOException e) {
            throw new RuntimeException(
                    String.format("%s was not set, and couldn't create an artificial one.",
                            varName));
        }
    }

    /**
     * Return the SuiteInfo name generated at build time. Exposed for testing.
     */
    protected String getSuiteInfoName() {
        return TestSuiteInfo.getInstance().getName();
    }

    /**
     * Return the SuiteInfo build number generated at build time. Exposed for testing.
     */
    protected String getSuiteInfoBuildNumber() {
        return TestSuiteInfo.getInstance().getBuildNumber();
    }

    /**
     * Return the SuiteInfo fullname generated at build time. Exposed for testing.
     */
    protected String getSuiteInfoFullname() {
        return TestSuiteInfo.getInstance().getFullName();
    }

    /**
     * Return the SuiteInfo version generated at build time. Exposed for testing.
     */
    protected String getSuiteInfoVersion() {
        return TestSuiteInfo.getInstance().getVersion();
    }

    /**
     * @return a {@link String} to use for directory suffixes created from the given time.
     */
    private String getDirSuffix(long millis) {
        return new SimpleDateFormat("yyyy.MM.dd_HH.mm.ss").format(new Date(millis));
    }
}
