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
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.result.ITestInvocationListener;
import com.android.tradefed.util.CommandResult;
import com.android.tradefed.util.CommandStatus;
import com.android.tradefed.util.SerializationUtil;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.File;
import java.io.IOException;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/** Run the tests associated with the invocation in the sandbox. */
public class SandboxInvocationRunner {

    /** Do setup and run the tests */
    public static void prepareAndRun(
            IConfiguration config, IInvocationContext context, ITestInvocationListener listener)
            throws Throwable {
        // TODO: refactor TestInvocation to be more modular in the sandbox handling
        ISandbox sandbox =
                (ISandbox) config.getConfigurationObject(Configuration.SANDBOX_TYPE_NAME);
        Exception res = sandbox.prepareEnvironment(context, config, listener);
        if (res != null) {
            CLog.w("Sandbox prepareEnvironment threw an Exception.");
            sandbox.tearDown();
            throw res;
        }
        try {
            CommandResult result = sandbox.run(config, listener);
            if (!CommandStatus.SUCCESS.equals(result.getStatus())) {
                handleStderrException(result.getStderr());
            }
        } finally {
            sandbox.tearDown();
        }
    }

    /** Attempt to extract a proper exception from stderr, if not stick to RuntimeException. */
    private static void handleStderrException(String stderr) throws Throwable {
        Pattern pattern =
                Pattern.compile(String.format(".*%s.*", TradefedSandboxRunner.EXCEPTION_KEY));
        for (String line : stderr.split("\n")) {
            Matcher m = pattern.matcher(line);
            if (m.matches()) {
                try {
                    JSONObject json = new JSONObject(line);
                    String filePath = json.getString(TradefedSandboxRunner.EXCEPTION_KEY);
                    File exception = new File(filePath);
                    Throwable obj = (Throwable) SerializationUtil.deserialize(exception, true);
                    throw obj;
                } catch (JSONException | IOException e) {
                    // ignore
                    CLog.w(
                            "Could not parse the stderr as a particular exception. "
                                    + "Using RuntimeException instead.");
                }
            }
        }
        throw new RuntimeException(stderr);
    }
}
