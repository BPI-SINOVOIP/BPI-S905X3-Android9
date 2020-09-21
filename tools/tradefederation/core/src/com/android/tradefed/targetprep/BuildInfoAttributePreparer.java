/*
 * Copyright (C) 2013 The Android Open Source Project
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

import com.android.tradefed.build.IBuildInfo;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;

import java.util.HashMap;
import java.util.Map;

/** A {@link ITargetPreparer} that adds arbitrary attributes to the {@link IBuildInfo}. */
@OptionClass(alias = "buildinfo-preparer")
public class BuildInfoAttributePreparer extends BaseTargetPreparer {

    @Option(name = "build-attribute", description = "build attributes to add")
    private Map<String, String> mBuildAttributes = new HashMap<String, String>();

    /**
     * {@inheritDoc}
     */
    @Override
    public void setUp(ITestDevice device, IBuildInfo buildInfo) throws TargetSetupError,
            BuildError, DeviceNotAvailableException {
        for (Map.Entry<String, String> attr : mBuildAttributes.entrySet()) {
            String key = attr.getKey();
            String value = attr.getValue();
            buildInfo.addBuildAttribute(key, value);
        }
    }
}
