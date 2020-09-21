/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tradefed.testtype;

import com.android.tradefed.device.ITestDevice;
import com.android.tradefed.targetprep.companion.CompanionAllocator;
import com.android.tradefed.targetprep.companion.CompanionDeviceTracker;

/**
 * Base test class that encapsulates boilerpate of getting and checking companion device
 * <p>
 * Subclass may call {@link #getCompanion()} to retrieve the allocated companion. Must be used
 * in conjunction with an implementation of {@link CompanionAllocator}
 */
public abstract class CompanionAwareTest implements IRemoteTest, IDeviceTest {

    private ITestDevice mCompanionDevice;

    /**
     * Fetches the companion device allocated for the primary device
     * @return the allocated companion device
     * @throws RuntimeException if no companion device has been allocated
     */
    protected ITestDevice getCompanion() {
        if (mCompanionDevice == null) {
            mCompanionDevice = CompanionDeviceTracker.getInstance().getCompanionDevice(getDevice());
            if (mCompanionDevice == null) {
                throw new RuntimeException("no companion device allocated, "
                        + "use appropriate ITargetPreparer");
            }
        }
        return mCompanionDevice;
    }
}
