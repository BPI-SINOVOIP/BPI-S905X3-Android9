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

package com.android.services.telephony;

import com.android.internal.telephony.Connection;

/**
 * Manages a single phone call handled by GSM.
 */
final class GsmConnection extends TelephonyConnection {
    GsmConnection(Connection connection, String telecomCallId, boolean isOutgoing) {
        super(connection, telecomCallId, isOutgoing);
    }

    /**
     * Clones the current {@link GsmConnection}.
     * <p>
     * Listeners are not copied to the new instance.
     *
     * @return The cloned connection.
     */
    @Override
    public TelephonyConnection cloneConnection() {
        GsmConnection gsmConnection = new GsmConnection(getOriginalConnection(),
                getTelecomCallId(), mIsOutgoing);
        return gsmConnection;
    }

    /** {@inheritDoc} */
    @Override
    public void onPlayDtmfTone(char digit) {
        if (getPhone() != null) {
            getPhone().startDtmf(digit);
        }
    }

    /** {@inheritDoc} */
    @Override
    public void onStopDtmfTone() {
        if (getPhone() != null) {
            getPhone().stopDtmf();
        }
    }

    @Override
    protected int buildConnectionProperties() {
        int properties = super.buildConnectionProperties();
        // PROPERTY_IS_DOWNGRADED_CONFERENCE is permanent on GSM connections -- once it is set, it
        // should be retained.
        if ((getConnectionProperties() & PROPERTY_IS_DOWNGRADED_CONFERENCE) != 0) {
            properties |= PROPERTY_IS_DOWNGRADED_CONFERENCE;
        }
        return properties;
    }

    @Override
    protected int buildConnectionCapabilities() {
        int capabilities = super.buildConnectionCapabilities();
        capabilities |= CAPABILITY_MUTE;
        // Overwrites TelephonyConnection.buildConnectionCapabilities() and resets the hold options
        // because all GSM calls should hold, even if the carrier config option is set to not show
        // hold for IMS calls.
        if (!shouldTreatAsEmergencyCall()) {
            capabilities |= CAPABILITY_SUPPORT_HOLD;
            if (isHoldable() && (getState() == STATE_ACTIVE || getState() == STATE_HOLDING)) {
                capabilities |= CAPABILITY_HOLD;
            }
        }

        return capabilities;
    }

    @Override
    void onRemovedFromCallService() {
        super.onRemovedFromCallService();
    }
}
