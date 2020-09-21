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
package com.android.tradefed.sandbox;

import com.android.tradefed.config.Configuration;
import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.ConfigurationFactory;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationFactory;
import com.android.tradefed.result.SubprocessResultsReporter;
import com.android.tradefed.util.StreamUtil;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Runner class that creates a {@link IConfiguration} based on a command line and dump it to a file.
 * args: <DumpCmd> <output File> <remaing command line>
 */
public class SandboxConfigDump {

    public enum DumpCmd {
        /** The full xml based on the command line will be outputted */
        FULL_XML,
        /** Only non-versioned element of the xml will be outputted */
        NON_VERSIONED_CONFIG,
        /** A run-ready config will be outputted */
        RUN_CONFIG
    }

    /**
     * We do not output the versioned elements to avoid causing the parent process to have issues
     * with them when trying to resolve them
     */
    private static final List<String> VERSIONED_ELEMENTS = new ArrayList<>();

    static {
        VERSIONED_ELEMENTS.add(Configuration.SYSTEM_STATUS_CHECKER_TYPE_NAME);
        VERSIONED_ELEMENTS.add(Configuration.DEVICE_METRICS_COLLECTOR_TYPE_NAME);
        VERSIONED_ELEMENTS.add(Configuration.MULTI_PREPARER_TYPE_NAME);
        VERSIONED_ELEMENTS.add(Configuration.TARGET_PREPARER_TYPE_NAME);
        VERSIONED_ELEMENTS.add(Configuration.TEST_TYPE_NAME);
    }

    /**
     * Parse the args and creates a {@link IConfiguration} from it then dumps it to the result file.
     */
    public int parse(String[] args) {
        // TODO: add some more checking
        List<String> argList = new ArrayList<>(Arrays.asList(args));
        DumpCmd cmd = DumpCmd.valueOf(argList.remove(0));
        File resFile = new File(argList.remove(0));
        IConfigurationFactory factory = ConfigurationFactory.getInstance();
        PrintWriter pw = null;
        try {
            // TODO: Handle keystore
            IConfiguration config =
                    factory.createConfigurationFromArgs(argList.toArray(new String[0]));
            if (DumpCmd.RUN_CONFIG.equals(cmd)) {
                config.getCommandOptions().setShouldUseSandboxing(false);
                config.getConfigurationDescription().setSandboxed(true);
                config.setTestInvocationListener(new SubprocessResultsReporter());
                // Turn off some of the invocation level options that would be duplicated in the
                // parent.
                config.getCommandOptions().setBugreportOnInvocationEnded(false);
                config.getCommandOptions().setBugreportzOnInvocationEnded(false);
            }
            pw = new PrintWriter(resFile);
            if (DumpCmd.NON_VERSIONED_CONFIG.equals(cmd)) {
                // Remove elements that are versioned.
                config.dumpXml(pw, VERSIONED_ELEMENTS);
            } else {
                // FULL_XML in that case.
                config.dumpXml(pw);
            }
        } catch (ConfigurationException | IOException e) {
            e.printStackTrace();
            return 1;
        } finally {
            StreamUtil.close(pw);
        }
        return 0;
    }

    public static void main(final String[] mainArgs) {
        try {
            GlobalConfiguration.createGlobalConfiguration(new String[] {});
        } catch (ConfigurationException e) {
            e.printStackTrace();
            System.exit(1);
        }
        SandboxConfigDump configDump = new SandboxConfigDump();
        int code = configDump.parse(mainArgs);
        System.exit(code);
    }
}
