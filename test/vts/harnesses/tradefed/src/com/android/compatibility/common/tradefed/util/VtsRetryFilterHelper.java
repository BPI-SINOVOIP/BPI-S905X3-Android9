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

package com.android.compatibility.common.tradefed.util;

import com.android.compatibility.common.tradefed.build.CompatibilityBuildHelper;
import com.android.compatibility.common.tradefed.util.RetryFilterHelper;
import com.android.compatibility.common.tradefed.util.RetryType;
import com.android.compatibility.common.util.IInvocationResult;
import com.android.tradefed.device.DeviceNotAvailableException;
import com.android.tradefed.device.ITestDevice;
import java.util.Set;

/**
 * Helper for generating --include-filter and --exclude-filter values on compatibility retry.
 */
public class VtsRetryFilterHelper extends RetryFilterHelper {
    /* Instance variables for retrieving results to be retried. */
    private int mSessionId;

    /**
     * Constructor for a {@link VtsRetryFilterHelper}.
     *
     * @param build a {@link CompatibilityBuildHelper} describing the build.
     * @param sessionId The ID of the session to retry.
     * @param subPlan The name of a subPlan to be used. Can be null.
     * @param includeFilters The include module filters to apply
     * @param excludeFilters The exclude module filters to apply
     * @param abiName The name of abi to use. Can be null.
     * @param moduleName The name of the module to run. Can be null.
     * @param testName The name of the test to run. Can be null.
     * @param retryType The type of results to retry. Can be null.
     */
    public VtsRetryFilterHelper(CompatibilityBuildHelper build, int sessionId, String subPlan,
            Set<String> includeFilters, Set<String> excludeFilters, String abiName,
            String moduleName, String testName, RetryType retryType) {
        super(build, sessionId, subPlan, includeFilters, excludeFilters, abiName, moduleName,
                testName, retryType);
        mSessionId = sessionId;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public void validateBuildFingerprint(ITestDevice device) throws DeviceNotAvailableException {
        IInvocationResult result = getResult();
        String oldVendorFingerprint = result.getBuildFingerprint();
        String currentVendorFingerprint = device.getProperty("ro.vendor.build.fingerprint");
        if (!oldVendorFingerprint.equals(currentVendorFingerprint)) {
            throw new IllegalArgumentException(
                    String.format("Device vendor fingerprint must match %s to retry session %d",
                            oldVendorFingerprint, mSessionId));
        }
    }
}
