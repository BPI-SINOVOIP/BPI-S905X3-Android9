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
package com.android.tradefed.testtype.suite;

import com.android.tradefed.build.IDeviceBuildInfo;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.ConfigurationUtil;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.config.Option;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.IAbiReceiver;
import com.android.tradefed.testtype.IRemoteTest;
import com.android.tradefed.util.DirectedGraph;
import com.android.tradefed.util.StreamUtil;
import com.android.tradefed.util.ZipUtil2;

import org.apache.commons.compress.archivers.zip.ZipFile;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Set;

/**
 * Implementation of {@link ITestSuite} which will load tests from TF jars res/config/suite/
 * folder.
 */
public class TfSuiteRunner extends ITestSuite {

    private static final String CONFIG_EXT = ".config";

    @Option(name = "run-suite-tag", description = "The tag that must be run.",
            mandatory = true)
    private String mSuiteTag = null;

    @Option(
        name = "suite-config-prefix",
        description = "Search only configs with given prefix for suite tags."
    )
    private String mSuitePrefix = null;

    @Option(
        name = "additional-tests-zip",
        description = "Path to a zip file containing additional tests to be loaded."
    )
    private String mAdditionalTestsZip = null;

    private DirectedGraph<String> mLoadedConfigGraph = null;

    /** {@inheritDoc} */
    @Override
    public LinkedHashMap<String, IConfiguration> loadTests() {
        mLoadedConfigGraph = new DirectedGraph<>();
        return loadTests(null, mLoadedConfigGraph);
    }

    /**
     * Internal load configuration. Load configuration, expand sub suite runners and track cycle
     * inclusion to prevent infinite recursion.
     *
     * @param parentConfig the name of the config being loaded.
     * @param graph the directed graph tracking inclusion.
     * @return a map of module name and the configuration associated.
     */
    private LinkedHashMap<String, IConfiguration> loadTests(
            String parentConfig, DirectedGraph<String> graph) {
        LinkedHashMap <String, IConfiguration> configMap =
                new LinkedHashMap<String, IConfiguration>();
        IConfigurationFactory configFactory = ConfigurationFactory.getInstance();
        // TODO: Do a better job searching for configs.
        // We do not load config from environment, they should be inside the testsDir of the build
        // info.
        List<String> configs = configFactory.getConfigList(mSuitePrefix, false);
        Set<IAbi> abis = null;
        if (getBuildInfo() instanceof IDeviceBuildInfo) {
            IDeviceBuildInfo deviceBuildInfo = (IDeviceBuildInfo) getBuildInfo();
            File testsDir = deviceBuildInfo.getTestsDir();
            if (testsDir != null) {
                if (mAdditionalTestsZip != null) {
                    CLog.d(
                            "Extract general-tests.zip (%s) to tests directory.",
                            mAdditionalTestsZip);
                    ZipFile zip = null;
                    try {
                        zip = new ZipFile(mAdditionalTestsZip);
                        ZipUtil2.extractZip(zip, testsDir);
                    } catch (IOException e) {
                        RuntimeException runtimeException =
                                new RuntimeException(
                                        String.format(
                                                "IO error (%s) when unzipping general-tests.zip",
                                                e.toString()),
                                        e);
                        throw runtimeException;
                    } finally {
                        StreamUtil.close(zip);
                    }
                }

                CLog.d(
                        "Loading extra test configs from the tests directory: %s",
                        testsDir.getAbsolutePath());
                List<File> extraTestCasesDirs = Arrays.asList(testsDir);
                configs.addAll(
                        ConfigurationUtil.getConfigNamesFromDirs(mSuitePrefix, extraTestCasesDirs));
            }
        }
        // Sort configs to ensure they are always evaluated and added in the same order.
        Collections.sort(configs);
        for (String configName : configs) {
            try {
                IConfiguration testConfig =
                        configFactory.createConfigurationFromArgs(new String[]{configName});
                if (testConfig.getConfigurationDescription().getSuiteTags().contains(mSuiteTag)) {
                    // If this config supports running against different ABIs we need to queue up
                    // multiple instances of this config.
                    if (canRunMultipleAbis(testConfig)) {
                        if (abis == null) {
                            try {
                                abis = getAbis(getDevice());
                            } catch (DeviceNotAvailableException e) {
                                throw new RuntimeException(e);
                            }
                        }
                        for (IAbi abi : abis) {
                            testConfig =
                                    configFactory.createConfigurationFromArgs(
                                            new String[] {configName});
                            String configNameAbi = abi.getName() + " " + configName;
                            for (IRemoteTest test : testConfig.getTests()) {
                                if (test instanceof IAbiReceiver) {
                                    ((IAbiReceiver) test).setAbi(abi);
                                }
                            }
                            for (ITargetPreparer preparer : testConfig.getTargetPreparers()) {
                                if (preparer instanceof IAbiReceiver) {
                                    ((IAbiReceiver) preparer).setAbi(abi);
                                }
                            }
                            for (IMultiTargetPreparer preparer :
                                    testConfig.getMultiTargetPreparers()) {
                                if (preparer instanceof IAbiReceiver) {
                                    ((IAbiReceiver) preparer).setAbi(abi);
                                }
                            }
                            LinkedHashMap<String, IConfiguration> expandedConfig =
                                    expandTestSuites(
                                            configNameAbi, testConfig, parentConfig, graph);
                            configMap.putAll(expandedConfig);
                        }
                    } else {
                        LinkedHashMap<String, IConfiguration> expandedConfig =
                                expandTestSuites(configName, testConfig, parentConfig, graph);
                        configMap.putAll(expandedConfig);
                    }
                }
            } catch (ConfigurationException | NoClassDefFoundError e) {
                // Do not print the stack it's too verbose.
                CLog.e("Configuration '%s' cannot be loaded, ignoring.", configName);
            }
        }
        return configMap;
    }

    /** Helper to determine if a test configuration can be ran against multiple ABIs. */
    private boolean canRunMultipleAbis(IConfiguration testConfig) {
        for (IRemoteTest test : testConfig.getTests()) {
            if (test instanceof IAbiReceiver) {
                return true;
            }
        }
        for (ITargetPreparer preparer : testConfig.getTargetPreparers()) {
            if (preparer instanceof IAbiReceiver) {
                return true;
            }
        }
        for (IMultiTargetPreparer preparer : testConfig.getMultiTargetPreparers()) {
            if (preparer instanceof IAbiReceiver) {
                return true;
            }
        }
        return false;
    }

    /**
     * Helper to expand all TfSuiteRunner included in sub-configuration. Avoid having module inside
     * module if a suite is ran as part of another suite. Verifies against circular suite
     * dependencies.
     */
    private LinkedHashMap<String, IConfiguration> expandTestSuites(
            String configName,
            IConfiguration config,
            String parentConfig,
            DirectedGraph<String> graph) {
        LinkedHashMap<String, IConfiguration> configMap =
                new LinkedHashMap<String, IConfiguration>();
        List<IRemoteTest> tests = new ArrayList<>(config.getTests());
        // In case some sub-config are suite too, we expand them to avoid weirdness
        // of modules inside modules.
        if (parentConfig != null) {
            graph.addEdge(parentConfig, configName);
            if (!graph.isDag()) {
                CLog.e("%s", graph);
                throw new RuntimeException(
                        String.format(
                                "Circular configuration detected: %s has been included "
                                        + "several times.",
                                configName));
            }
        }
        for (IRemoteTest test : tests) {
            if (test instanceof TfSuiteRunner) {
                TfSuiteRunner runner = (TfSuiteRunner) test;
                // Suite runner can only load and run stuff, if it has a suite tag set.
                if (runner.getSuiteTag() != null) {
                    LinkedHashMap<String, IConfiguration> subConfigs =
                            runner.loadTests(configName, graph);
                    configMap.putAll(subConfigs);
                } else {
                    CLog.w(
                            "Config %s does not have a suite-tag it cannot run anything.",
                            configName);
                }
                config.getTests().remove(test);
            }
        }
        // If we have any IRemoteTests remaining in the base configuration, it will run.
        if (!config.getTests().isEmpty()) {
            configMap.put(sanitizeConfigName(configName), config);
        }
        return configMap;
    }

    private String getSuiteTag() {
        return mSuiteTag;
    }

    /**
     * Sanitize the config name by sanitizing the module name and reattaching the abi if it is part
     * of the config name.
     */
    private String sanitizeConfigName(String originalName) {
        String[] segments;
        if (originalName.contains(" ")) {
            segments = originalName.split(" ");
            return segments[0] + " " + sanitizeModuleName(segments[1]);
        }
        return sanitizeModuleName(originalName);
    }

    /**
     * Some module names are currently the absolute path name of some config files. We want to
     * sanitize that look more like a short included config name.
     */
    private String sanitizeModuleName(String originalName) {
        if (originalName.endsWith(CONFIG_EXT)) {
            originalName = originalName.substring(0, originalName.length() - CONFIG_EXT.length());
        }
        if (!originalName.startsWith("/")) {
            return originalName;
        }
        // if it's an absolute path
        String[] segments = originalName.split("/");
        if (segments.length < 3) {
            return originalName;
        }
        // return last two segments only
        return String.join("/", segments[segments.length - 2], segments[segments.length - 1]);
    }
}
