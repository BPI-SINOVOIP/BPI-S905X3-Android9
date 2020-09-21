/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.ddmlib.Log;
import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.IConfigurationReceiver;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.ITestDevice;

/** Placeholder empty implementation of a {@link ITargetPreparer}. */
@OptionClass(alias = "stub-preparer")
public class StubTargetPreparer extends BaseTargetPreparer implements IConfigurationReceiver {

    @Option(name = "test-boolean-option", description = "test option, keep default to true.")
    private boolean mTestBooleanOption = true;

    @Option(name = "test-boolean-option-false", description = "test option, keep default to true.")
    private boolean mTestBooleanOptionFalse = false;

    private IConfiguration mConfig;

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError {
        Log.d("TargetPreparer", "skipping target prepare step");
    }

    /** {@inheritDoc} */
    @Override
    public void setConfiguration(IConfiguration configuration) {
        mConfig = configuration;
    }

    /** Returns the configuration received through {@link #setConfiguration(IConfiguration)}. */
    public IConfiguration getConfiguration() {
        return mConfig;
    }

    public boolean getTestBooleanOption() {
        return mTestBooleanOption;
    }

    public boolean getTestBooleanOptionFalse() {
        return mTestBooleanOptionFalse;
    }
}
