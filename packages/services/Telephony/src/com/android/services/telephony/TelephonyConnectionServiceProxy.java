/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.services.telephony;

import android.telecom.Conference;
import android.telecom.Connection;
import android.telecom.PhoneAccountHandle;

import java.util.Collection;

/**
 * TelephonyConnectionService Interface for testing purpose
 */

public interface TelephonyConnectionServiceProxy {
    Collection<Connection> getAllConnections();
    void addConference(TelephonyConference mTelephonyConference);
    void addConference(ImsConference mImsConference);
    void removeConnection(Connection connection);
    void addExistingConnection(PhoneAccountHandle phoneAccountHandle,
            Connection connection);
    void addExistingConnection(PhoneAccountHandle phoneAccountHandle,
                               Connection connection, Conference conference);
    void addConnectionToConferenceController(TelephonyConnection connection);
}
