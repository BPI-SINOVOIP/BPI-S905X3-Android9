/*
 * Copyright (C) 2018 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.ArrayUtil;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/** A Test for running Compatibility Test Suite with new suite system. */
@OptionClass(alias = "base-suite")
public class BaseTestSuite extends ITestSuite {

    public static final String INCLUDE_FILTER_OPTION = "include-filter";
    public static final String EXCLUDE_FILTER_OPTION = "exclude-filter";
    public static final String MODULE_OPTION = "module";
    public static final String TEST_ARG_OPTION = "test-arg";
    public static final String TEST_OPTION = "test";
    public static final char TEST_OPTION_SHORT_NAME = 't';
    private static final String MODULE_ARG_OPTION = "module-arg";

    @Option(
        name = INCLUDE_FILTER_OPTION,
        description = "the include module filters to apply.",
        importance = Importance.ALWAYS
    )
    private Set<String> mIncludeFilters = new HashSet<>();

    @Option(
        name = EXCLUDE_FILTER_OPTION,
        description = "the exclude module filters to apply.",
        importance = Importance.ALWAYS
    )
    private Set<String> mExcludeFilters = new HashSet<>();

    @Option(
        name = MODULE_OPTION,
        shortName = 'm',
        description = "the test module to run. Only works for configuration in the tests dir.",
        importance = Importance.IF_UNSET
    )
    private String mModuleName = null;

    @Option(
        name = TEST_OPTION,
        shortName = TEST_OPTION_SHORT_NAME,
        description = "the test to run.",
        importance = Importance.IF_UNSET
    )
    private String mTestName = null;

    @Option(
        name = MODULE_ARG_OPTION,
        description =
                "the arguments to pass to a module. The expected format is"
                        + "\"<module-name>:<arg-name>:[<arg-key>:=]<arg-value>\"",
        importance = Importance.ALWAYS
    )
    private List<String> mModuleArgs = new ArrayList<>();

    @Option(
        name = TEST_ARG_OPTION,
        description =
                "the arguments to pass to a test. The expected format is"
                        + "\"<test-class>:<arg-name>:[<arg-key>:=]<arg-value>\"",
        importance = Importance.ALWAYS
    )
    private List<String> mTestArgs = new ArrayList<>();

    @Option(
        name = "run-suite-tag",
        description =
                "The tag that must be run. If specified, only configurations containing the "
                        + "matching suite tag will be able to run."
    )
    private String mSuiteTag = null;

    @Option(
        name = "suite-config-prefix",
        description = "Search only configs with given prefix for suite tags."
    )
    private String mSuitePrefix = null;

    @Option(
        name = "skip-loading-config-jar",
        description = "Whether or not to skip loading configurations from the JAR on the classpath."
    )
    private boolean mSkipJarLoading = false;

    @Option(
        name = "config-patterns",
        description =
                "The pattern(s) of the configurations that should be loaded from a directory."
                        + " If none is explicitly specified, .*.xml and .*.config will be used."
                        + " Can be repeated."
    )
    private List<String> mConfigPatterns = new ArrayList<>();

    private SuiteModuleLoader mModuleRepo;
    private Map<String, List<SuiteTestFilter>> mIncludeFiltersParsed = new HashMap<>();
    private Map<String, List<SuiteTestFilter>> mExcludeFiltersParsed = new HashMap<>();

    /** {@inheritDoc} */
    @Override
    public LinkedHashMap<String, IConfiguration> loadTests() {
        try {
            File testsDir = getTestsDir();
            setupFilters(testsDir);
            Set<IAbi> abis = getAbis(getDevice());

            // Create and populate the filters here
            SuiteModuleLoader.addFilters(mIncludeFilters, mIncludeFiltersParsed, abis);
            SuiteModuleLoader.addFilters(mExcludeFilters, mExcludeFiltersParsed, abis);

            CLog.d(
                    "Initializing ModuleRepo\nABIs:%s\n"
                            + "Test Args:%s\nModule Args:%s\nIncludes:%s\nExcludes:%s",
                    abis, mTestArgs, mModuleArgs, mIncludeFiltersParsed, mExcludeFiltersParsed);
            mModuleRepo =
                    createModuleLoader(
                            mIncludeFiltersParsed, mExcludeFiltersParsed, mTestArgs, mModuleArgs);
            // Actual loading of the configurations.
            return loadingStrategy(abis, testsDir, mSuitePrefix, mSuiteTag);
        } catch (DeviceNotAvailableException | FileNotFoundException e) {
            throw new RuntimeException(e);
        }
    }

    /**
     * Default loading strategy will load from the resources and the tests directory. Can be
     * extended or replaced.
     *
     * @param abis The set of abis to run against.
     * @param testsDir The tests directory.
     * @param suitePrefix A prefix to filter the resource directory.
     * @param suiteTag The suite tag a module should have to be included. Can be null.
     * @return A list of loaded configuration for the suite.
     */
    public LinkedHashMap<String, IConfiguration> loadingStrategy(
            Set<IAbi> abis, File testsDir, String suitePrefix, String suiteTag) {
        LinkedHashMap<String, IConfiguration> loadedConfigs = new LinkedHashMap<>();
        // Load configs that are part of the resources
        if (!mSkipJarLoading) {
            loadedConfigs.putAll(
                    getModuleLoader().loadConfigsFromJars(abis, suitePrefix, suiteTag));
        }

        // Load the configs that are part of the tests dir
        if (mConfigPatterns.isEmpty()) {
            // If no special pattern was configured, use the default configuration patterns we know
            mConfigPatterns.add(".*\\.config");
            mConfigPatterns.add(".*\\.xml");
        }
        loadedConfigs.putAll(
                getModuleLoader()
                        .loadConfigsFromDirectory(
                                testsDir, abis, suitePrefix, suiteTag, mConfigPatterns));
        return loadedConfigs;
    }

    public File getTestsDir() throws FileNotFoundException {
        IBuildInfo build = getBuildInfo();
        if (build instanceof IDeviceBuildInfo) {
            return ((IDeviceBuildInfo) build).getTestsDir();
        }
        // TODO: handle multi build?
        throw new FileNotFoundException("Could not found a tests dir folder.");
    }

    /** {@inheritDoc} */
    @Override
    public void setBuild(IBuildInfo buildInfo) {
        super.setBuild(buildInfo);
    }

    /** Sets include-filters for the compatibility test */
    public void setIncludeFilter(Set<String> includeFilters) {
        mIncludeFilters.addAll(includeFilters);
    }

    /** Gets a copy of include-filters for the compatibility test */
    protected Set<String> getIncludeFilter() {
        return new HashSet<String>(mIncludeFilters);
    }

    /** Sets exclude-filters for the compatibility test */
    public void setExcludeFilter(Set<String> excludeFilters) {
        mExcludeFilters.addAll(excludeFilters);
    }

    /** Returns the current {@link SuiteModuleLoader}. */
    public SuiteModuleLoader getModuleLoader() {
        return mModuleRepo;
    }

    /** Adds module args */
    public void addModuleArgs(Set<String> moduleArgs) {
        mModuleArgs.addAll(moduleArgs);
    }

    /**
     * Create the {@link SuiteModuleLoader} responsible to load the {@link IConfiguration} and
     * assign them some of the options.
     *
     * @param includeFiltersFormatted The formatted and parsed include filters.
     * @param excludeFiltersFormatted The formatted and parsed exclude filters.
     * @param testArgs the list of test ({@link IRemoteTest}) arguments.
     * @param moduleArgs the list of module arguments.
     * @return the created {@link SuiteModuleLoader}.
     */
    public SuiteModuleLoader createModuleLoader(
            Map<String, List<SuiteTestFilter>> includeFiltersFormatted,
            Map<String, List<SuiteTestFilter>> excludeFiltersFormatted,
            List<String> testArgs,
            List<String> moduleArgs) {
        return new SuiteModuleLoader(
                includeFiltersFormatted, excludeFiltersFormatted, testArgs, moduleArgs);
    }

    /**
     * Sets the include/exclude filters up based on if a module name was given.
     *
     * @throws FileNotFoundException if any file is not found.
     */
    protected void setupFilters(File testsDir) throws FileNotFoundException {
        if (mModuleName != null) {
            // If this option (-m / --module) is set only the matching unique module should run.
            Set<File> modules =
                    SuiteModuleLoader.getModuleNamesMatching(
                            testsDir, mSuitePrefix, String.format("%s.*.config", mModuleName));
            // If multiple modules match, do exact match.
            if (modules.size() > 1) {
                Set<File> newModules = new HashSet<>();
                String exactModuleName = String.format("%s.config", mModuleName);
                for (File module : modules) {
                    if (module.getName().equals(exactModuleName)) {
                        newModules.add(module);
                        modules = newModules;
                        break;
                    }
                }
            }
            if (modules.size() == 0) {
                throw new IllegalArgumentException(
                        String.format("No modules found matching %s", mModuleName));
            } else if (modules.size() > 1) {
                throw new IllegalArgumentException(
                        String.format(
                                "Multiple modules found matching %s:\n%s\nWhich one did you "
                                        + "mean?\n",
                                mModuleName, ArrayUtil.join("\n", modules)));
            } else {
                File mod = modules.iterator().next();
                String moduleName = mod.getName().replace(".config", "");
                checkFilters(mIncludeFilters, moduleName);
                checkFilters(mExcludeFilters, moduleName);
                mIncludeFilters.add(
                        new SuiteTestFilter(getRequestedAbi(), moduleName, mTestName).toString());
            }
        } else if (mTestName != null) {
            throw new IllegalArgumentException(
                    "Test name given without module name. Add --module <module-name>");
        }
    }

    /* Helper method designed to remove filters in a list not applicable to the given module */
    private static void checkFilters(Set<String> filters, String moduleName) {
        Set<String> cleanedFilters = new HashSet<String>();
        for (String filter : filters) {
            if (moduleName.equals(SuiteTestFilter.createFrom(filter).getName())) {
                cleanedFilters.add(filter); // Module name matches, filter passes
            }
        }
        filters.clear();
        filters.addAll(cleanedFilters);
    }
}
