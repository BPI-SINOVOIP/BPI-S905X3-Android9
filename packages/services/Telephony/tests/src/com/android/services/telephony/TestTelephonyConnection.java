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
 * limitations under the License.
 */

package com.android.services.telephony;

import android.content.Context;
import android.os.Bundle;
import android.telecom.PhoneAccountHandle;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.doNothing;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.when;

import com.android.internal.telephony.Call;
import com.android.internal.telephony.Connection;
import com.android.internal.telephony.Phone;

import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.List;

/**
 * Mock Telephony Connection used in TelephonyConferenceController.java for testing purpose
 */

public class TestTelephonyConnection extends TelephonyConnection {

    @Mock
    com.android.internal.telephony.Connection mMockRadioConnection;

    @Mock
    Call mMockCall;

    @Mock
    Context mMockContext;

    private Phone mMockPhone;
    private int mNotifyPhoneAccountChangedCount = 0;
    private List<String> mLastConnectionEvents = new ArrayList<>();
    private List<Bundle> mLastConnectionEventExtras = new ArrayList<>();

    @Override
    public com.android.internal.telephony.Connection getOriginalConnection() {
        return mMockRadioConnection;
    }

    public TestTelephonyConnection() {
        super(null, null, false);
        MockitoAnnotations.initMocks(this);

        mMockPhone = mock(Phone.class);
        mMockContext = mock(Context.class);
        // Set up mMockRadioConnection and mMockPhone to contain an active call
        when(mMockRadioConnection.getState()).thenReturn(Call.State.ACTIVE);
        when(mMockRadioConnection.getCall()).thenReturn(mMockCall);
        doNothing().when(mMockRadioConnection).addListener(any(Connection.Listener.class));
        doNothing().when(mMockRadioConnection).addPostDialListener(
                any(Connection.PostDialListener.class));
        when(mMockPhone.getRingingCall()).thenReturn(mMockCall);
        when(mMockPhone.getContext()).thenReturn(null);
        when(mMockCall.getState()).thenReturn(Call.State.ACTIVE);
        when(mMockCall.getPhone()).thenReturn(mMockPhone);
    }

    @Override
    public boolean isConferenceSupported() {
        return true;
    }

    public void setMockPhone(Phone newPhone) {
        mMockPhone = newPhone;
    }

    @Override
    public Phone getPhone() {
        return mMockPhone;
    }

    public TelephonyConnection cloneConnection() {
        return this;
    }

    @Override
    public void notifyPhoneAccountChanged(PhoneAccountHandle pHandle) {
        mNotifyPhoneAccountChangedCount++;
    }

    @Override
    public void sendConnectionEvent(String event, Bundle extras) {
        mLastConnectionEvents.add(event);
        mLastConnectionEventExtras.add(extras);
    }

    public int getNotifyPhoneAccountChangedCount() {
        return mNotifyPhoneAccountChangedCount;
    }

    public List<String> getLastConnectionEvents() {
        return mLastConnectionEvents;
    }

    public List<Bundle> getLastConnectionEventExtras() {
        return mLastConnectionEventExtras;
    }
}
