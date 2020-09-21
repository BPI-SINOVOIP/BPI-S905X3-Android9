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
/*
 * Copyright (c) 2017, The Linux Foundation.
 */

/*
 * Copyright 2012 Giesecke & Devrient GmbH.
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

package com.android.se.security;

/** Class for Storing the APDU and NFC Access for a particular Channel */
public class ChannelAccess {

    private final String mTag = "SecureElement-ChannelAccess";
    private String mPackageName = "";
    private ACCESS mAccess = ACCESS.UNDEFINED;
    private ACCESS mApduAccess = ACCESS.UNDEFINED;
    private boolean mUseApduFilter = false;
    private int mCallingPid = 0;
    private String mReason = "no access by default";
    private ACCESS mNFCEventAccess = ACCESS.UNDEFINED;
    private ApduFilter[] mApduFilter = null;

    /** Clones the ChannelAccess */
    public ChannelAccess clone() {
        ChannelAccess ca = new ChannelAccess();
        ca.setAccess(mAccess, mReason);
        ca.setPackageName(mPackageName);
        ca.setApduAccess(mApduAccess);
        ca.setCallingPid(mCallingPid);
        ca.setNFCEventAccess(mNFCEventAccess);
        ca.setUseApduFilter(mUseApduFilter);
        if (mApduFilter != null) {
            ApduFilter[] apduFilter = new ApduFilter[mApduFilter.length];
            int i = 0;
            for (ApduFilter filter : mApduFilter) {
                apduFilter[i++] = filter.clone();
            }
            ca.setApduFilter(apduFilter);
        } else {
            ca.setApduFilter(null);
        }
        return ca;
    }

    public String getPackageName() {
        return mPackageName;
    }

    public void setPackageName(String name) {
        mPackageName = name;
    }

    public ACCESS getApduAccess() {
        return mApduAccess;
    }

    public void setApduAccess(ACCESS apduAccess) {
        mApduAccess = apduAccess;
    }

    public ACCESS getAccess() {
        return mAccess;
    }

    /** Sets the Access for the ChannelAccess */
    public void setAccess(ACCESS access, String reason) {
        mAccess = access;
        mReason = reason;
    }

    public boolean isUseApduFilter() {
        return mUseApduFilter;
    }

    public void setUseApduFilter(boolean useApduFilter) {
        mUseApduFilter = useApduFilter;
    }

    public int getCallingPid() {
        return mCallingPid;
    }

    public void setCallingPid(int callingPid) {
        mCallingPid = callingPid;
    }

    public String getReason() {
        return mReason;
    }

    public ApduFilter[] getApduFilter() {
        return mApduFilter;
    }

    public void setApduFilter(ApduFilter[] accessConditions) {
        mApduFilter = accessConditions;
    }

    public ACCESS getNFCEventAccess() {
        return mNFCEventAccess;
    }

    public void setNFCEventAccess(ACCESS access) {
        mNFCEventAccess = access;
    }

    @Override
    public String toString() {
        StringBuilder sb = new StringBuilder();
        sb.append(this.getClass().getName());
        sb.append("\n [mPackageName=");
        sb.append(mPackageName);
        sb.append(", mAccess=");
        sb.append(mAccess);
        sb.append(", mApduAccess=");
        sb.append(mApduAccess);
        sb.append(", mUseApduFilter=");
        sb.append(mUseApduFilter);
        sb.append(", mApduFilter=");
        if (mApduFilter != null) {
            for (ApduFilter f : mApduFilter) {
                sb.append(f.toString());
                sb.append(" ");
            }
        } else {
            sb.append("null");
        }
        sb.append(", mCallingPid=");
        sb.append(mCallingPid);
        sb.append(", mReason=");
        sb.append(mReason);
        sb.append(", mNFCEventAllowed=");
        sb.append(mNFCEventAccess);
        sb.append("]\n");

        return sb.toString();
    }

    public enum ACCESS {
        ALLOWED,
        DENIED,
        UNDEFINED;
    }
}
