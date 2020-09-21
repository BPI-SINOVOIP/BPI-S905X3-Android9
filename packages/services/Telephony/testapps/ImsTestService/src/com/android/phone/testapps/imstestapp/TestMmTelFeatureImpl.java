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

import android.telephony.ims.feature.CapabilityChangeRequest;
import android.telephony.ims.feature.MmTelFeature;
import android.telephony.ims.stub.ImsRegistrationImplBase;
import android.telephony.ims.stub.ImsSmsImplBase;
import android.util.ArraySet;
import android.util.SparseArray;
import android.widget.Toast;

import java.util.Set;

public class TestMmTelFeatureImpl extends MmTelFeature {

    public static TestMmTelFeatureImpl sTestMmTelFeatureImpl;
    public static TestImsSmsImpl sTestImsSmsImpl;
    private boolean mIsReady = false;
    // Enabled Capabilities - not status
    private SparseArray<MmTelCapabilities> mEnabledCapabilities = new SparseArray<>();
    private final Set<MmTelUpdateCallback> mCallbacks = new ArraySet<>();

    static class MmTelUpdateCallback {
        void onEnabledCapabilityChanged() {
        }
    }

    public TestMmTelFeatureImpl() {
        mEnabledCapabilities.append(ImsRegistrationImplBase.REGISTRATION_TECH_LTE,
                new MmTelCapabilities());
        mEnabledCapabilities.append(ImsRegistrationImplBase.REGISTRATION_TECH_IWLAN,
                new MmTelCapabilities());
        setFeatureState(STATE_READY);
    }

    public static TestMmTelFeatureImpl getInstance() {
        if (sTestMmTelFeatureImpl == null) {
            sTestMmTelFeatureImpl = new TestMmTelFeatureImpl();
        }
        return sTestMmTelFeatureImpl;
    }

    public static TestImsSmsImpl getSmsInstance() {
        if (sTestImsSmsImpl == null) {
            sTestImsSmsImpl = new TestImsSmsImpl();
        }
        return sTestImsSmsImpl;
    }

    public void addUpdateCallback(MmTelUpdateCallback callback) {
        mCallbacks.add(callback);
    }

    public boolean isReady() {
        return mIsReady;
    }

    @Override
    public boolean queryCapabilityConfiguration(int capability, int radioTech) {
        return mEnabledCapabilities.get(radioTech).isCapable(capability);
    }

    @Override
    public void changeEnabledCapabilities(CapabilityChangeRequest request,
            CapabilityCallbackProxy c) {
        for (CapabilityChangeRequest.CapabilityPair pair : request.getCapabilitiesToEnable()) {
            mEnabledCapabilities.get(pair.getRadioTech()).addCapabilities(pair.getCapability());
        }
        for (CapabilityChangeRequest.CapabilityPair pair : request.getCapabilitiesToDisable()) {
            mEnabledCapabilities.get(pair.getRadioTech()).removeCapabilities(pair.getCapability());
        }
        mCallbacks.forEach(callback->callback.onEnabledCapabilityChanged());
    }

    @Override
    public ImsSmsImplBase getSmsImplementation() {
        return getSmsInstance();
    }

    @Override
    public void onFeatureRemoved() {
        super.onFeatureRemoved();
    }

    public void sendCapabilitiesUpdate(MmTelFeature.MmTelCapabilities c) {
        Toast.makeText(mContext, "Sending Capabilities:{" + c + "}",
                Toast.LENGTH_LONG).show();

        notifyCapabilitiesStatusChanged(c);
    }

    public SparseArray<MmTelCapabilities> getEnabledCapabilities() {
        return mEnabledCapabilities;
    }

    @Override
    public void onFeatureReady() {
        mIsReady = true;
    }
}
