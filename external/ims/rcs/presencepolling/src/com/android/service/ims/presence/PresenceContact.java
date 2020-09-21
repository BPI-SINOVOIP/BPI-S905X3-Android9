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

public class PresenceContact {
    public static final int VIDEO_CALLING_NOT_AVAILABLE = 0;
    public static final int VIDEO_CALLING_AVAILABLE = 1;

    String mDisplayName = null;
    String mPhoneNumber = null;
    String mFormattedNumber = null;
    String mRawContactId = null;
    String mContactId = null;
    String mDataId = null;
    boolean mIsVolteCapable = false;
    boolean mIsVtCapable = false;
    int mVtStatus = VIDEO_CALLING_NOT_AVAILABLE;

    String mVtUri = null;

    public PresenceContact(String name, String number, String formattedNumber, String rawContactId,
            String contactId, String dataId) {
        mDisplayName = name;
        mPhoneNumber = number;
        mFormattedNumber = formattedNumber;
        mRawContactId = rawContactId;
        mContactId = contactId;
        mDataId = dataId;
    }

    public String getDisplayName() {
        return mDisplayName;
    }

    public String getPhoneNumber() {
        return mPhoneNumber;
    }

    public String getFormattedNumber() {
        return mFormattedNumber;
    }

    public String getRawContactId() {
        return mRawContactId;
    }

    public String getContactId() {
        return mContactId;
    }

    public String getDataId() {
        return mDataId;
    }

    public boolean isVolteCapable() {
        return mIsVolteCapable;
    }

    public void setIsVolteCapable(boolean isVolteCapable) {
        mIsVolteCapable = isVolteCapable;
    }

    public boolean isVtCapable() {
        return mIsVtCapable;
    }

    public void setIsVtCapable(boolean isVtCapable) {
        mIsVtCapable = isVtCapable;
    }

    public int getVtStatus() {
        return mVtStatus;
    }

    public void setVtStatus(int vtAvailable) {
        mVtStatus = vtAvailable;
    }

    public String getVtUri() {
        return mVtUri;
    }

    public void setVtUri(String vtUri) {
        mVtUri = vtUri;
    }
}
