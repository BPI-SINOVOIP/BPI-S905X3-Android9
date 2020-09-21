/*
 * Copyright (c) 2017 Google Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you
 * may not use this file except in compliance with the License. You may
 * obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
 * implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

package com.android.vts.util;

import com.android.vts.entity.DeviceInfoEntity;
import com.android.vts.entity.TestRunEntity;
import com.google.gson.JsonObject;
import com.google.gson.JsonPrimitive;
import java.util.ArrayList;
import java.util.List;
import org.apache.commons.lang.StringUtils;

/** Helper object for describing test results data. */
public class TestRunMetadata {
    private static final String TEST_RUN = "testRun";
    private static final String TEST_DETAILS = "testDetails";
    private static final String DEVICE_INFO = "deviceInfo";
    private static final String ABI_INFO = "abiInfo";
    public final TestRunEntity testRun;

    private final List<DeviceInfoEntity> devices;
    private String deviceInfo;
    private String abiInfo;
    private TestRunDetails details;

    /**
     * Create a test metadata object.
     *
     * @param testName The name of the test.
     * @param testRun The test run entity storing run information.
     * @param devices The list of device entities describing the test run.
     */
    public TestRunMetadata(String testName, TestRunEntity testRun, List<DeviceInfoEntity> devices) {
        this.testRun = testRun;
        this.deviceInfo = "";
        this.abiInfo = "";
        this.details = null;
        this.devices = devices;
    }

    /**
     * Create a test metadata object.
     *
     * @param testName The name of the test.
     * @param testRun The test run entity storing run information.
     */
    public TestRunMetadata(String testName, TestRunEntity testRun) {
        this(testName, testRun, new ArrayList<DeviceInfoEntity>());
    }

    /**
     * Add a device info to the test run metadata.
     *
     * @param device The entity to add.
     */
    public void addDevice(DeviceInfoEntity device) {
        this.devices.add(device);
    }

    /** Get device information for the test run and add it to the metadata message. */
    private void processDeviceInfo() {
        List<String> deviceInfoList = new ArrayList<>();
        List<String> abiInfoList = new ArrayList<>();

        for (DeviceInfoEntity device : this.devices) {
            String abi = "";
            String abiName = device.abiName;
            String abiBitness = device.abiBitness;
            if (abiName.length() > 0) {
                abi += abiName;
                if (abiBitness.length() > 0) {
                    abi += " (" + abiBitness + " bit)";
                }
            }
            abiInfoList.add(abi);
            deviceInfoList.add(device.getFingerprint());
        }
        this.abiInfo = StringUtils.join(abiInfoList, ", ");
        this.deviceInfo = StringUtils.join(deviceInfoList, ", ");
    }

    /**
     * Get the device info string in the test metadata.
     *
     * @return The string descriptor of the devices used in the test run.
     */
    public String getDeviceInfo() {
        return this.deviceInfo;
    }

    /**
     * Get the test run details (e.g. test case information) for the test run.
     *
     * @return The TestRunDetails object stored in the metadata, or null if not set.
     */
    public TestRunDetails getDetails() {
        return this.details;
    }

    /**
     * Add test case details to the metadata object.
     *
     * <p>Used for prefetching details on initial page load.
     *
     * @param details The TestRunDetails object storing test case results for the test run.
     */
    public void addDetails(TestRunDetails details) {
        this.details = details;
    }

    /**
     * Serializes the test run metadata to json format.
     *
     * @return A JsonElement object representing the details object.
     */
    public JsonObject toJson() {
        processDeviceInfo();
        JsonObject json = new JsonObject();
        json.add(DEVICE_INFO, new JsonPrimitive(this.deviceInfo));
        json.add(ABI_INFO, new JsonPrimitive(this.abiInfo));
        json.add(TEST_RUN, this.testRun.toJson());
        if (this.details != null) {
            json.add(TEST_DETAILS, this.details.toJson());
        }
        return json;
    }
}
