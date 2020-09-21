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

package com.android.phone.testapps.imstestapp;

import android.telephony.ims.ImsService;
import android.telephony.ims.feature.ImsFeature;
import android.telephony.ims.feature.MmTelFeature;
import android.telephony.ims.feature.RcsFeature;
import android.telephony.ims.stub.ImsConfigImplBase;
import android.telephony.ims.stub.ImsFeatureConfiguration;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.util.Log;

/**
 * Creates a test ImsService, which is used for testing framework IMS.
 */

public class TestImsService extends ImsService {

    public static final String LOG_TAG = "ImsTestApp";

    public static TestImsService mInstance;

    public TestImsRegistrationImpl mImsRegistration;
    public TestMmTelFeatureImpl mTestMmTelFeature;
    public TestRcsFeatureImpl mTestRcsFeature;
    public TestImsConfigImpl mTestImsConfig;

    public static TestImsService getInstance() {
        return mInstance;
    }

    @Override
    public void onCreate() {
        Log.i(LOG_TAG, "TestImsService: onCreate");
        mImsRegistration = TestImsRegistrationImpl.getInstance();
        mTestMmTelFeature = TestMmTelFeatureImpl.getInstance();
        mTestRcsFeature = new TestRcsFeatureImpl();
        mTestImsConfig = TestImsConfigImpl.getInstance();

        mInstance = this;
    }

    @Override
    public ImsFeatureConfiguration querySupportedImsFeatures() {
        return new ImsFeatureConfiguration.Builder()
                .addFeature(0, ImsFeature.FEATURE_EMERGENCY_MMTEL)
                .addFeature(0, ImsFeature.FEATURE_MMTEL)
                .build();
    }

    @Override
    public MmTelFeature createMmTelFeature(int slotId) {
        Log.i(LOG_TAG, "TestImsService: onCreateMmTelImsFeature");
        return mTestMmTelFeature;
    }

    @Override
    public RcsFeature createRcsFeature(int slotId) {
        return mTestRcsFeature;
    }

    @Override
    public ImsRegistrationImplBase getRegistration(int slotId) {
        Log.i(LOG_TAG, "TestImsService: getRegistration");
        return mImsRegistration;
    }

    @Override
    public ImsConfigImplBase getConfig(int slotId) {
        return mTestImsConfig;
    }
}
