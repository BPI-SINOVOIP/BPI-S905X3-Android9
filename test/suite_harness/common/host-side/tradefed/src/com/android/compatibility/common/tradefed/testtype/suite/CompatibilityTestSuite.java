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
package com.android.compatibility.common.tradefed.testtype.suite;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.testtype.ISubPlan;
import com.android.compatibility.common.tradefed.testtype.SubPlan;
import com.android.compatibility.common.tradefed.testtype.retry.RetryFactoryTest;
import com.android.ddmlib.Log.LogLevel;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.suite.BaseTestSuite;
import com.android.tradefed.testtype.suite.SuiteModuleLoader;
import com.android.tradefed.testtype.suite.SuiteTestFilter;
import com.android.tradefed.testtype.suite.TestSuiteInfo;
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.InputStream;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * A Test for running Compatibility Test Suite with new suite system.
 */
@OptionClass(alias = "compatibility")
public class CompatibilityTestSuite extends BaseTestSuite {

    public static final String SUBPLAN_OPTION = "subplan";

    // TODO: remove this option when CompatibilityTest goes away
    @Option(name = RetryFactoryTest.RETRY_OPTION,
            shortName = 'r',
            description = "Copy of --retry from CompatibilityTest to prevent using it.")
    private Integer mRetrySessionId = null;

    @Option(name = SUBPLAN_OPTION,
            description = "the subplan to run",
            importance = Importance.IF_UNSET)
    private String mSubPlan;

    private CompatibilityBuildHelper mBuildHelper;
    /** Tag if the current instance is running as a retry from RetryFactory */
    private boolean mIsRetry = false;

    /**
     * Ctor that sets some default for Compatibility runs.
     */
    public CompatibilityTestSuite() {
        try {
            OptionSetter setter = new OptionSetter(this);
            setter.setOptionValue("config-patterns", ".*\\.config");
            setter.setOptionValue("skip-loading-config-jar", "true");
        } catch (ConfigurationException e) {
            // Should not happen
            throw new RuntimeException(e);
        }
    }

    @Override
    public void setBuild(IBuildInfo buildInfo) {
        super.setBuild(buildInfo);
        mBuildHelper = new CompatibilityBuildHelper(buildInfo);
    }

    @Override
    public File getTestsDir() throws FileNotFoundException {
        return mBuildHelper.getTestsDir();
    }

    @Override
    public SuiteModuleLoader createModuleLoader(
            Map<String, List<SuiteTestFilter>> includeFiltersFormatted,
            Map<String, List<SuiteTestFilter>> excludeFiltersFormatted,
            List<String> testArgs,
            List<String> moduleArgs) {
        return new CompatibilitySuiteModuleLoader(includeFiltersFormatted,
                excludeFiltersFormatted, testArgs, moduleArgs);
    }

    @Override
    public LinkedHashMap<String, IConfiguration> loadTests() {
        if (mRetrySessionId != null) {
            throw new IllegalArgumentException(
                    String.format("--retry cannot be specified with %s[*].xml. "
                            + "Use 'run retry --retry <session id>' instead.",
                            TestSuiteInfo.getInstance().getName().toLowerCase()));
        }
        return super.loadTests();
    }

    /**
     * Sets the include/exclude filters up based on if a module name was given or whether this is a
     * retry run.
     */
    @Override
    final protected void setupFilters(File testDir) throws FileNotFoundException {
        if (mSubPlan != null) {
            try {
                File subPlanFile = new File(mBuildHelper.getSubPlansDir(), mSubPlan + ".xml");
                if (!subPlanFile.exists()) {
                    throw new IllegalArgumentException(
                            String.format("Could not retrieve subplan \"%s\"", mSubPlan));
                }
                InputStream subPlanInputStream = new FileInputStream(subPlanFile);
                ISubPlan subPlan = new SubPlan();
                subPlan.parse(subPlanInputStream);
                // Set include/exclude filter is additive
                setIncludeFilter(subPlan.getIncludeFilters());
                setExcludeFilter(subPlan.getExcludeFilters());
            } catch (ParseException e) {
                throw new RuntimeException(
                        String.format("Unable to find or parse subplan %s", mSubPlan), e);
            }
        }
        super.setupFilters(testDir);
    }

    /**
     * Allow to reset the requested session id for retry.
     */
    public final void resetRetryId() {
        mRetrySessionId = null;
    }

    /**
     * Mark the instance of CompatibilityTestSuite as a retry.
     */
    public final void isRetry() {
        mIsRetry = true;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public LinkedHashMap<String, IConfiguration> loadingStrategy(
            Set<IAbi> abis, File testsDir, String suitePrefix, String suiteTag) {
        LinkedHashMap<String, IConfiguration> loadedConfigs =
                super.loadingStrategy(abis, testsDir, suitePrefix, suiteTag);
        // Add an extra check in CTS since we never expect the config folder to be empty.
        if (loadedConfigs.size() == 0) {
            if (mIsRetry) {
                // Only log if it's a retry
                CLog.logAndDisplay(LogLevel.DEBUG,
                        "No module that needed to run in retry were found. nothing to do.");
            } else {
                throw new IllegalArgumentException(
                        String.format("No config files found in %s or in resources.",
                                testsDir.getAbsolutePath()));
            }
        }
        return loadedConfigs;
    }
}
