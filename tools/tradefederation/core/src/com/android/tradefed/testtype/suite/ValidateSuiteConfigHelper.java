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

import com.android.tradefed.build.StubBuildProvider;
import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IDeviceConfiguration;
import com.android.tradefed.device.metric.FilePullerLogCollector;
import com.android.tradefed.device.metric.IMetricCollector;
import com.android.tradefed.result.TextResultReporter;
import com.android.tradefed.targetprep.ITargetPreparer;
import com.android.tradefed.targetprep.multi.IMultiTargetPreparer;
import com.android.tradefed.testtype.suite.module.BaseModuleController;

import java.util.ArrayList;
import java.util.List;

/**
 * This class will help validating that the {@link IConfiguration} loaded for the suite are meeting
 * the expected requirements: - No Build providers - No Result reporters
 */
public class ValidateSuiteConfigHelper {

    /**
     * Special exemption list for some collectors. They would be overly cumbersome to be defined at
     * the suite level.
     */
    private static final List<String> ALLOWED_COLLECTOR_IN_MODULE = new ArrayList<>();

    static {
        // This collector simply pull and log file from the device. it is useful at the module level
        // so they can specify which 'key' is going to be watched to be pulled.
        ALLOWED_COLLECTOR_IN_MODULE.add(FilePullerLogCollector.class.getCanonicalName());
    }

    private ValidateSuiteConfigHelper() {}

    /**
     * Check that a configuration is properly built to run in a suite.
     *
     * @param config a {@link IConfiguration} to be checked if valide for suite.
     */
    public static void validateConfig(IConfiguration config) {
        if (config.getDeviceConfig().size() < 2) {
            // Special handling for single device objects.
            if (!config.getBuildProvider().getClass().isAssignableFrom(StubBuildProvider.class)) {
                throwRuntime(
                        config,
                        String.format(
                                "%s objects are not allowed in module.",
                                Configuration.BUILD_PROVIDER_TYPE_NAME));
            }
            checkTargetPrep(config, config.getTargetPreparers());
        }
        // if a multi device config is presented, ensure none of the devices define a build_provider
        for (IDeviceConfiguration deviceConfig : config.getDeviceConfig()) {
            if (!deviceConfig
                    .getBuildProvider()
                    .getClass()
                    .isAssignableFrom(StubBuildProvider.class)) {
                throwRuntime(
                        config,
                        String.format(
                                "%s objects are not allowed in module.",
                                Configuration.BUILD_PROVIDER_TYPE_NAME));
            }
            checkTargetPrep(config, deviceConfig.getTargetPreparers());
        }
        if (config.getTestInvocationListeners().size() != 1) {
            throwRuntime(
                    config,
                    String.format(
                            "%s objects are not allowed in module.",
                            Configuration.RESULT_REPORTER_TYPE_NAME));
        }
        if (!config.getTestInvocationListeners()
                .get(0)
                .getClass()
                .isAssignableFrom(TextResultReporter.class)) {
            throwRuntime(
                    config,
                    String.format(
                            "%s objects are not allowed in module.",
                            Configuration.RESULT_REPORTER_TYPE_NAME));
        }
        // For now we do not allow pre-multi target preparers in modules.
        if (config.getMultiPreTargetPreparers().size() > 0) {
            throwRuntime(
                    config,
                    String.format(
                            "%s objects are not allowed in module.",
                            Configuration.MULTI_PRE_TARGET_PREPARER_TYPE_NAME));
        }

        // Check multi target preparers
        checkTargetPrep(config, config.getMultiTargetPreparers());

        // Check metric collectors
        for (IMetricCollector collector : config.getMetricCollectors()) {
            // Only some collectors are allowed in the module
            if (!ALLOWED_COLLECTOR_IN_MODULE.contains(collector.getClass().getCanonicalName())) {
                throwRuntime(
                        config,
                        String.format(
                                "%s objects are not allowed in module. Except for: %s",
                                Configuration.DEVICE_METRICS_COLLECTOR_TYPE_NAME,
                                ALLOWED_COLLECTOR_IN_MODULE));
            }
        }

        // Check that we validate the module_controller.
        Object controller = config.getConfigurationObject(ModuleDefinition.MODULE_CONTROLLER);
        if (controller != null) {
            if (!(controller instanceof BaseModuleController)) {
                throwRuntime(
                        config,
                        String.format(
                                "%s object should be of type %s",
                                ModuleDefinition.MODULE_CONTROLLER, BaseModuleController.class));
            }
        }
    }

    /**
     * Check target_preparer and multi_target_preparer to ensure they do not extends each other as
     * it could lead to some issues.
     */
    private static boolean checkTargetPrep(IConfiguration config, List<?> targetPrepList) {
        for (Object o : targetPrepList) {
            if (o instanceof ITargetPreparer && o instanceof IMultiTargetPreparer) {
                throwRuntime(
                        config,
                        String.format(
                                "%s is extending both %s and " + "%s",
                                o.getClass().getCanonicalName(),
                                Configuration.TARGET_PREPARER_TYPE_NAME,
                                Configuration.MULTI_PREPARER_TYPE_NAME));
                return false;
            }
        }
        return true;
    }

    private static void throwRuntime(IConfiguration config, String msg) {
        throw new RuntimeException(
                String.format(
                        "Configuration %s cannot be run in a suite: %s", config.getName(), msg));
    }
}
