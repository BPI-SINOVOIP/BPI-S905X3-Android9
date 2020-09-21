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

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.GlobalConfiguration;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.sandbox.SandboxConfigDump.DumpCmd;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.IRunUtil;
import com.android.tradefed.util.IRunUtil.EnvPriority;

import com.google.common.base.Joiner;
import com.google.common.base.Strings;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/** A utility class for managing {@link IConfiguration} when doing sandboxing. */
public class SandboxConfigUtil {

    private static final long DUMP_TIMEOUT = 2 * 60 * 1000; // 2min

    /**
     * Create a subprocess based on the Tf jars from any version, and dump the xml {@link
     * IConfiguration} based on the command line args.
     *
     * @param classpath the classpath to use to run the sandbox.
     * @param runUtil the {@link IRunUtil} to use to run the command.
     * @param args the command line args.
     * @param dump the {@link DumpCmd} driving some of the outputs.
     * @param globalConfig the file describing the global configuration to be used.
     * @return A {@link File} containing the xml dump from the command line.
     * @throws SandboxConfigurationException if the dump is not successful.
     */
    public static File dumpConfigForVersion(
            String classpath, IRunUtil runUtil, String[] args, DumpCmd dump, File globalConfig)
            throws SandboxConfigurationException, IOException {
        if (Strings.isNullOrEmpty(classpath)) {
            throw new SandboxConfigurationException(
                    "Something went wrong with the sandbox setup, classpath was empty.");
        }
        runUtil.unsetEnvVariable(GlobalConfiguration.GLOBAL_CONFIG_VARIABLE);
        File destination = null;
        try {
            destination = FileUtil.createTempFile("config-container", ".xml");
            if (globalConfig != null) {
                runUtil.setEnvVariable(
                        GlobalConfiguration.GLOBAL_CONFIG_VARIABLE, globalConfig.getAbsolutePath());
                runUtil.setEnvVariablePriority(EnvPriority.SET);
            }
        } catch (IOException e) {
            FileUtil.deleteFile(globalConfig);
            FileUtil.deleteFile(destination);
            throw e;
        }
        List<String> mCmdArgs = new ArrayList<>();
        mCmdArgs.add("java");
        mCmdArgs.add("-cp");
        mCmdArgs.add(classpath);
        mCmdArgs.add(SandboxConfigDump.class.getCanonicalName());
        mCmdArgs.add(dump.toString());
        mCmdArgs.add(destination.getAbsolutePath());
        for (String arg : args) {
            mCmdArgs.add(arg);
        }
        CommandResult result = runUtil.runTimedCmd(DUMP_TIMEOUT, mCmdArgs.toArray(new String[0]));
        if (CommandStatus.SUCCESS.equals(result.getStatus())) {
            return destination;
        }
        FileUtil.deleteFile(destination);
        // Do not delete the global configuration file here in this case, it might still be used.
        throw new SandboxConfigurationException(result.getStderr());
    }

    /**
     * Create a subprocess based on the Tf jars from any version, and dump the xml {@link
     * IConfiguration} based on the command line args.
     *
     * @param rootDir the directory containing all the jars from TF.
     * @param runUtil the {@link IRunUtil} to use to run the command.
     * @param args the command line args.
     * @param dump the {@link DumpCmd} driving some of the outputs.
     * @param globalConfig the file describing the global configuration to be used.
     * @return A {@link File} containing the xml dump from the command line.
     * @throws ConfigurationException if the dump is not successful.
     */
    public static File dumpConfigForVersion(
            File rootDir, IRunUtil runUtil, String[] args, DumpCmd dump, File globalConfig)
            throws ConfigurationException, IOException {
        // include all jars on the classpath
        String classpath = "";
        Set<String> jarFiles = FileUtil.findFiles(rootDir, ".*.jar");
        classpath = Joiner.on(":").join(jarFiles);
        return dumpConfigForVersion(classpath, runUtil, args, dump, globalConfig);
    }

    /** Create a global config with only the keystore to make it available in subprocess. */
    public static File dumpFilteredGlobalConfig() throws IOException {
        String[] configs =
                new String[] {
                    GlobalConfiguration.DEVICE_MANAGER_TYPE_NAME,
                    GlobalConfiguration.KEY_STORE_TYPE_NAME
                };
        File filteredGlobalConfig = FileUtil.createTempFile("filtered_global_config", ".config");
        GlobalConfiguration.getInstance().cloneConfigWithFilter(filteredGlobalConfig, configs);
        return filteredGlobalConfig;
    }
}
