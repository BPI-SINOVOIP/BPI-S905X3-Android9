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
package com.android.tradefed.testtype.suite.module;

import com.android.tradefed.config.Option;
import com.android.tradefed.invoker.IInvocationContext;
import com.android.tradefed.testtype.Abi;
import com.android.tradefed.testtype.IAbi;
import com.android.tradefed.testtype.suite.ModuleDefinition;
import com.android.tradefed.util.AbiUtils;

/**
 * Basic implementation of {@link IModuleController} that should be implemented for checking if a
 * module should run or not.
 */
public abstract class BaseModuleController implements IModuleController {

    @Option(
        name = "bugreportz-on-failure",
        description = "Module option to capture a bugreportz on its test failure."
    )
    private boolean mBugReportOnFailure = true;

    @Option(
        name = "logcat-on-failure",
        description = "Module option to capture a logcat on its test failure."
    )
    private boolean mLogcatOnFailure = true;

    @Option(
        name = "screenshot-on-failure",
        description = "Module option to capture a screenshot on its test failure."
    )
    private boolean mScreenshotOnFailure = true;

    private IInvocationContext mContext;

    @Override
    public final RunStrategy shouldRunModule(IInvocationContext context) {
        mContext = context;
        return shouldRun(context);
    }

    /**
     * Method to decide if the module should run or not.
     *
     * @param context the {@link IInvocationContext} of the module
     * @return True if the module should run, false otherwise.
     */
    public abstract RunStrategy shouldRun(IInvocationContext context);

    /** Helper method to get the module abi. */
    public final IAbi getModuleAbi() {
        String abi = mContext.getAttributes().get(ModuleDefinition.MODULE_ABI).get(0);
        return new Abi(abi, AbiUtils.getBitness(abi));
    }

    /** Helper method to get the module name. */
    public final String getModuleName() {
        return mContext.getAttributes().get(ModuleDefinition.MODULE_NAME).get(0);
    }

    /** Returns whether of not the module wants to capture the logcat on test failure. */
    public final boolean shouldCaptureLogcat() {
        return mLogcatOnFailure;
    }

    /** Returns whether of not the module wants to capture the screenshot on test failure. */
    public final boolean shouldCaptureScreenshot() {
        return mScreenshotOnFailure;
    }

    /** Returns whether of not the module wants to capture the bugreport on test failure. */
    public final boolean shouldCaptureBugreport() {
        return mBugReportOnFailure;
    }
}
