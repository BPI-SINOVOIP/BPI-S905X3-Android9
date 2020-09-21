/*
 * Copyright 2018 The Android Open Source Project
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
package com.android.bluetooth.hfp;

import java.util.Objects;

/**
 * A blob of data representing an overall call state on the phone
 */
class HeadsetCallState extends HeadsetMessageObject {
    /**
     * Number of active calls
     */
    int mNumActive;
    /**
     * Number of held calls
     */
    int mNumHeld;
    /**
     * Current call setup state as defined in bthf_call_state_t of bt_hf.h or
     * {@link com.android.server.telecom.BluetoothPhoneServiceImpl} or {@link HeadsetHalConstants}
     */
    int mCallState;
    /**
     * Currently active call's phone number
     */
    String mNumber;
    /**
     * Phone number type
     */
    int mType;

    HeadsetCallState(int numActive, int numHeld, int callState, String number, int type) {
        mNumActive = numActive;
        mNumHeld = numHeld;
        mCallState = callState;
        mNumber = number;
        mType = type;
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(this.getClass().getSimpleName())
                .append("[numActive=")
                .append(mNumActive)
                .append(", numHeld=")
                .append(mNumHeld)
                .append(", callState=")
                .append(mCallState)
                .append(", number=");
        if (mNumber == null) {
            builder.append("null");
        } else {
            builder.append("***");
        }
        builder.append(mNumber).append(", type=").append(mType).append("]");
    }

    @Override
    public boolean equals(Object object) {
        if (this == object) {
            return true;
        }
        if (!(object instanceof HeadsetCallState)) {
            return false;
        }
        HeadsetCallState that = (HeadsetCallState) object;
        return mNumActive == that.mNumActive && mNumHeld == that.mNumHeld
                && mCallState == that.mCallState && Objects.equals(mNumber, that.mNumber)
                && mType == that.mType;
    }

    @Override
    public int hashCode() {
        return Objects.hash(mNumActive, mNumHeld, mCallState, mNumber, mType);
    }
}
