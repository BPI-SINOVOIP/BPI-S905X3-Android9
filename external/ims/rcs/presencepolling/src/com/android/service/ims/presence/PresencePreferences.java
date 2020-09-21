/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import android.content.Context;
import android.content.SharedPreferences;
import android.content.SharedPreferences.Editor;

public class PresencePreferences {
    private static final String PREFERENCES = "PresencePolling";
    private static PresencePreferences sInstance;

    private Context mContext = null;
    private SharedPreferences mCommonPref = null;

    private PresencePreferences() {
    }

    public void setContext(Context context) {
        if (mContext == null) {
            mContext = context;
            mCommonPref = mContext.getSharedPreferences(PREFERENCES, Context.MODE_PRIVATE);
        }
    }

    public static PresencePreferences getInstance(){
        if (null == sInstance) {
            sInstance = new PresencePreferences();
        }
        return sInstance;
    }

    private static final String CAPABILITY_DISCOVERY_TIME = "CapabilityDiscoveryTime";

    public long lastCapabilityDiscoveryTime() {
        if (mCommonPref == null) {
            return 0;
        }

        return mCommonPref.getLong(CAPABILITY_DISCOVERY_TIME, 0);
    }

    public void updateCapabilityDiscoveryTime() {
        if (mCommonPref == null) {
            return;
        }

        Editor editor = mCommonPref.edit();
        long time = System.currentTimeMillis();
        editor.putLong(CAPABILITY_DISCOVERY_TIME, time);
        editor.commit();
    }

    private static final String PHONE_SUBSCRIBER_ID = "PhoneSubscriberId";
    public String getSubscriberId() {
        if (mCommonPref == null) {
            return null;
        }

        return mCommonPref.getString(PHONE_SUBSCRIBER_ID, null);
    }

    public void setSubscriberId(String id) {
        if (mCommonPref == null) {
            return;
        }

        Editor editor = mCommonPref.edit();
        editor.putString(PHONE_SUBSCRIBER_ID, id);
        editor.commit();
    }

    private static final String PHONE_LINE1_NUMBER = "PhoneLine1Number";
    public String getLine1Number() {
        if (mCommonPref == null) {
            return null;
        }

        return mCommonPref.getString(PHONE_LINE1_NUMBER, null);
    }

    public void setLine1Number(String number) {
        if (mCommonPref == null) {
            return;
        }

        Editor editor = mCommonPref.edit();
        editor.putString(PHONE_LINE1_NUMBER, number);
        editor.commit();
    }

    private static final String RCS_TEST_MODE = "RcsTestMode";
    public boolean getRcsTestMode() {
        if (mCommonPref == null) {
            return false;
        }

        return mCommonPref.getInt(RCS_TEST_MODE, 0) == 1;
    }

    public void setRcsTestMode(boolean test) {
        if (mCommonPref == null) {
            return;
        }

        Editor editor = mCommonPref.edit();
        editor.putInt(RCS_TEST_MODE, test ? 1 : 0);
        editor.commit();
    }
}
