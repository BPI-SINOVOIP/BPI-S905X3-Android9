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

/**
 * A blob of response data to AT+CLCC command from HF
 *
 * Example:
 *   AT+CLCC
 *   +CLCC:[index],[direction],[status],[mode],[mpty][,[number],[type]]
 */
class HeadsetClccResponse extends HeadsetMessageObject {
    /**
     * Index of the call, starting with 1, by the sequence of setup or receiving the calls
     */
    int mIndex;
    /**
     * Direction of the call, 0 is outgoing, 1 is incoming
     */
    int mDirection;
    /**
     * Status of the call, currently defined in bthf_call_state_t of bt_hf.h or
     * {@link com.android.server.telecom.BluetoothPhoneServiceImpl} or {@link HeadsetHalConstants}
     *
     * 0 - Active
     * 1 - Held
     * 2 - Dialing
     * 3 - Alerting
     * 4 - Incoming
     * 5 - Waiting
     * 6 - Call held by response and hold
     */
    int mStatus;
    /**
     * Call mode, 0 is Voice, 1 is Data, 2 is Fax
     */
    int mMode;
    /**
     * Multi-party indicator
     *
     * 0 - this call is NOT a member of a multi-party (conference) call
     * 1 - this call IS a multi-party (conference) call
     */
    boolean mMpty;
    /**
     * Phone number of the call (optional)
     */
    String mNumber;
    /**
     * Phone number type (optional)
     */
    int mType;

    HeadsetClccResponse(int index, int direction, int status, int mode, boolean mpty, String number,
            int type) {
        mIndex = index;
        mDirection = direction;
        mStatus = status;
        mMode = mode;
        mMpty = mpty;
        mNumber = number;
        mType = type;
    }

    @Override
    public void buildString(StringBuilder builder) {
        if (builder == null) {
            return;
        }
        builder.append(this.getClass().getSimpleName())
                .append("[index=")
                .append(mIndex)
                .append(", direction=")
                .append(mDirection)
                .append(", status=")
                .append(mStatus)
                .append(", callMode=")
                .append(mMode)
                .append(", isMultiParty=")
                .append(mMpty)
                .append(", number=");
        if (mNumber == null) {
            builder.append("null");
        } else {
            builder.append("***");
        }
        builder.append(", type=").append(mType).append("]");
    }
}
