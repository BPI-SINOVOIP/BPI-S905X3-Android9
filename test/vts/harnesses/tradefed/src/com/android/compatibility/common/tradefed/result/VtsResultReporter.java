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
package com.android.compatibility.common.tradefed.result;

import com.android.tradefed.log.LogUtil.CLog;
import java.util.Map;

/** Override the result reporter to specify vendor device information in the VTS report. */
public class VtsResultReporter extends ResultReporter {
    private static final String BUILD_VENDOR_FINGERPRINT = "build_vendor_fingerprint";
    private static final String BUILD_VENDOR_MANUFACTURER = "build_vendor_manufacturer";
    private static final String BUILD_VENDOR_MODEL = "build_vendor_model";

    /** Determine if the property is empty or not. */
    private static boolean isEmptyProperty(String property) {
        return (property == null || property.trim().isEmpty() || property.equals("null"));
    }

    /** Override the vendor fingerprint and manufacturer in the report. */
    @Override
    protected void addDeviceBuildInfoToResult() {
        super.addDeviceBuildInfoToResult();
        Map<String, String> props = mapBuildInfo();
        String vendorFingerprint = props.get(BUILD_VENDOR_FINGERPRINT);
        String vendorManufacturer = props.get(BUILD_VENDOR_MANUFACTURER);
        String vendorModel = props.get(BUILD_VENDOR_MODEL);
        if (!isEmptyProperty(vendorFingerprint) && !isEmptyProperty(vendorManufacturer)
                && !isEmptyProperty(vendorModel)) {
            addDeviceBuildInfoToResult(vendorFingerprint, vendorManufacturer, vendorModel);
        } else {
            CLog.w(String.format(
                    "Vendor build information missing. Fingerprint: '%s', manufacturer: '%s', model: '%s'",
                    vendorFingerprint, vendorManufacturer, vendorModel));
        }
    }
}
