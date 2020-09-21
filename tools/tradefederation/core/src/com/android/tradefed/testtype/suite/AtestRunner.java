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
package com.android.tradefed.testtype.suite;

import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionCopier;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.result.SubprocessResultsReporter;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.testtype.InstrumentationTest;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.testtype.ITestFilterReceiver;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;


/** Implementation of {@link ITestSuite} */
public class AtestRunner extends BaseTestSuite {

    private static final Pattern CONFIG_RE =
            Pattern.compile(".*/(?<config>[^/]+).config", Pattern.CASE_INSENSITIVE);

    @Option(
        name = "wait-for-debugger",
        description = "For InstrumentationTests, we pass debug to the instrumentation run."
    )
    private boolean mDebug = false;

    @Option(
        name = "disable-target-preparers",
        description =
                "Skip the target preparer steps enumerated in test config. Skips the teardown step "
                        + "as well."
    )
    private boolean mSkipSetUp = false;

    @Option(name = "disable-teardown", description = "Skip the teardown of the target preparers.")
    private boolean mSkipTearDown = false;

    @Option(
        name = "subprocess-report-port",
        description = "the port where to connect to send the" + "events."
    )
    private Integer mReportPort = null;

    @Option(
        name = "atest-include-filter",
        description =
                "the include filters to pass to a module. The expected format is"
                        + "\"<module-name>:<include-filter-value>\"",
        importance = Importance.ALWAYS
    )
    private List<String> mIncludeFilters = new ArrayList<>();

    @Override
    public LinkedHashMap<String, IConfiguration> loadTests() {
        // atest only needs to run on primary or specified abi.
        if (getRequestedAbi() == null) {
            setPrimaryAbiRun(true);
        }
        LinkedHashMap<String, IConfiguration> configMap = super.loadTests();
        LinkedHashMap<String, HashSet<String>> includeFilters = getIncludeFilters();
        for (IConfiguration testConfig : configMap.values()) {
            if (mSkipSetUp || mSkipTearDown) {
                disableTargetPreparers(testConfig, mSkipSetUp, mSkipTearDown);
            }
            if (mDebug) {
                addDebugger(testConfig);
            }

            // Inject include-filter to test.
            HashSet<String> moduleFilters =
                    includeFilters.get(canonicalizeConfigName(testConfig.getName()));
            if (moduleFilters != null) {
                for (String filter : moduleFilters) {
                    addFilter(testConfig, filter);
                }
            }
        }

        return configMap;
    }

    /** Get a collection of include filters grouped by module name. */
    private LinkedHashMap<String, HashSet<String>> getIncludeFilters() {
        LinkedHashMap<String, HashSet<String>> includeFilters =
                new LinkedHashMap<String, HashSet<String>>();
        for (String filter : mIncludeFilters) {
            int moduleSep = filter.indexOf(":");
            if (moduleSep == -1) {
                throw new RuntimeException("Expected delimiter ':' for module or class.");
            }
            String moduleName = canonicalizeConfigName(filter.substring(0, moduleSep));
            String moduleFilter = filter.substring(moduleSep + 1);
            HashSet<String> moduleFilters = includeFilters.get(moduleName);
            if (moduleFilters == null) {
                moduleFilters = new HashSet<String>();
                includeFilters.put(moduleName, moduleFilters);
            }
            moduleFilters.add(moduleFilter);
        }
        return includeFilters;
    }

    /**
     * Non-integrated modules have full file paths as their name, .e.g /foo/bar/name.config, but all
     * we want is the name.
     */
    private String canonicalizeConfigName(String originalName) {
        Matcher match = CONFIG_RE.matcher(originalName);
        if (match.find()) {
            return match.group("config");
        }
        return originalName;
    }

    /**
     * Add filter to the tests in an IConfiguration.
     *
     * @param testConfig The configuration containing tests to filter.
     * @param filter The filter to add to the tests in the testConfig.
     */
    private void addFilter(IConfiguration testConfig, String filter) {
        List<IRemoteTest> tests = testConfig.getTests();
        for (IRemoteTest test : tests) {
            if (test instanceof ITestFilterReceiver) {
                CLog.d(
                        "%s:%s - Applying filter (%s)",
                        testConfig.getName(), test.getClass().getSimpleName(), filter);
                ((ITestFilterReceiver) test).addIncludeFilter(filter);
            } else {
                CLog.e(
                        "Test Class (%s) does not support filtering. Cannot apply filter: %s.\n"
                                + "Please update test to use a class that implements "
                                + "ITestFilterReceiver. Running entire test module instead.",
                        test.getClass().getSimpleName(), filter);
            }
        }
    }

    /** Return a ConfigurationFactory instance. Organized this way for testing purposes. */
    public IConfigurationFactory loadConfigFactory() {
        return ConfigurationFactory.getInstance();
    }

    /** {@inheritDoc} */
    @Override
    protected List<ITestInvocationListener> createModuleListeners() {
        List<ITestInvocationListener> listeners = super.createModuleListeners();
        if (mReportPort != null) {
            SubprocessResultsReporter subprocessResult = new SubprocessResultsReporter();
            OptionCopier.copyOptionsNoThrow(this, subprocessResult);
            listeners.add(subprocessResult);
        }
        return listeners;
    }

    /** Helper to attach the debugger to any Instrumentation tests in the config. */
    private void addDebugger(IConfiguration testConfig) {
        for (IRemoteTest test : testConfig.getTests()) {
            if (test instanceof InstrumentationTest) {
                ((InstrumentationTest) test).setDebug(true);
            }
        }
    }

    /** Helper to disable TargetPreparers of a test. */
    private void disableTargetPreparers(
            IConfiguration testConfig, boolean skipSetUp, boolean skipTearDown) {
        for (ITargetPreparer targetPreparer : testConfig.getTargetPreparers()) {
            if (skipSetUp) {
                CLog.d(
                        "%s: Disabling Target Preparer (%s)",
                        testConfig.getName(), targetPreparer.getClass().getSimpleName());
                targetPreparer.setDisable(true);
            } else if (skipTearDown) {
                CLog.d(
                        "%s: Disabling Target Preparer TearDown (%s)",
                        testConfig.getName(), targetPreparer.getClass().getSimpleName());
                targetPreparer.setDisableTearDown(true);
            }
        }
    }
}
