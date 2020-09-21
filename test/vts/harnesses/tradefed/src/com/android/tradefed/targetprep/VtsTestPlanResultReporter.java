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

package com.android.tradefed.targetprep;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.Base64;
import java.util.List;
import java.util.LinkedList;
import java.util.NoSuchElementException;

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.RunUtil;
import com.android.tradefed.util.VtsDashboardUtil;
import com.android.tradefed.util.VtsVendorConfigFileUtil;

import com.android.vts.proto.VtsReportMessage.DashboardPostMessage;
import com.android.vts.proto.VtsReportMessage.TestPlanReportMessage;

/**
 * Uploads the VTS test plan execution result to the web DB using a RESTful API and
 * an OAuth2 credential kept in a json file.
 */
@OptionClass(alias = "vts-plan-result")
public class VtsTestPlanResultReporter
        implements ITargetPreparer, ITargetCleaner, IMultiTargetPreparer {
    private static final String PLUS_ME = "https://www.googleapis.com/auth/plus.me";
    private static final String TEST_PLAN_EXECUTION_RESULT = "vts-test-plan-execution-result";
    private static final String TEST_PLAN_REPORT_FILE = "TEST_PLAN_REPORT_FILE";
    private static final String TEST_PLAN_REPORT_FILE_NAME = "status.json";
    private static VtsVendorConfigFileUtil configReader = null;
    private static VtsDashboardUtil dashboardUtil = null;
    private static final int BASE_TIMEOUT_MSECS = 1000 * 60;
    IRunUtil mRunUtil = new RunUtil();

    @Option(name = "plan-name", description = "The plan name") private String mPlanName = null;

    @Option(name = "file-path",
            description = "The path of a VTS vendor config file (format: json).")
    private String mVendorConfigFilePath = null;

    @Option(name = "default-type",
            description = "The default config file type, e.g., `prod` or `staging`.")
    private String mDefaultType = VtsVendorConfigFileUtil.VENDOR_TEST_CONFIG_DEFAULT_TYPE;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) {
        File mStatusDir = null;
        try {
            mStatusDir = FileUtil.createTempDir(TEST_PLAN_EXECUTION_RESULT);
            if (mStatusDir != null) {
                File statusFile = new File(mStatusDir, TEST_PLAN_REPORT_FILE_NAME);
                buildInfo.setFile(TEST_PLAN_REPORT_FILE, statusFile, buildInfo.getBuildId());
            }
        } catch (IOException ex) {
            CLog.e("Can't create a temp dir to store test execution results.");
            return;
        }

        // Use IBuildInfo to pass the uesd vendor config information to the rest of workflow.
        buildInfo.addBuildAttribute(
                VtsVendorConfigFileUtil.KEY_VENDOR_TEST_CONFIG_DEFAULT_TYPE, mDefaultType);
        buildInfo.addBuildAttribute(
                VtsVendorConfigFileUtil.KEY_VENDOR_TEST_CONFIG_FILE_PATH, mVendorConfigFilePath);
        configReader = new VtsVendorConfigFileUtil();
        configReader.LoadVendorConfig(buildInfo);
        dashboardUtil = new VtsDashboardUtil(configReader);
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(IInvocationContext context)
            throws TargetSetupError, DeviceNotAvailableException {
        setUp(context.getDevices().get(0), context.getBuildInfos().get(0));
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(ITestDevice device, IBuildInfo buildInfo, Throwable e) {
        File reportFile = buildInfo.getFile(TEST_PLAN_REPORT_FILE);
        String repotFilePath = reportFile.getAbsolutePath();
        DashboardPostMessage postMessage = new DashboardPostMessage();
        TestPlanReportMessage testPlanMessage = new TestPlanReportMessage();
        testPlanMessage.setTestPlanName(mPlanName);
        boolean found = false;
        try (BufferedReader br = new BufferedReader(new FileReader(repotFilePath))) {
            String currentLine;
            while ((currentLine = br.readLine()) != null) {
                String[] lineWords = currentLine.split("\\s+");
                if (lineWords.length != 2) {
                    CLog.e(String.format("Invalid keys %s", currentLine));
                    continue;
                }
                testPlanMessage.addTestModuleName(lineWords[0]);
                testPlanMessage.addTestModuleStartTimestamp(Long.parseLong(lineWords[1]));
                found = true;
            }
        } catch (IOException ex) {
            CLog.d(String.format("Can't read the test plan result file %s", repotFilePath));
            return;
        }
        File reportDir = reportFile.getParentFile();
        CLog.d(String.format("Delete report dir %s", reportDir.getAbsolutePath()));
        FileUtil.recursiveDelete(reportDir);
        postMessage.addTestPlanReport(testPlanMessage);
        if (found) {
            dashboardUtil.Upload(postMessage);
        }
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void tearDown(IInvocationContext context, Throwable e)
            throws DeviceNotAvailableException {
        tearDown(context.getDevices().get(0), context.getBuildInfos().get(0), e);
    }
}
