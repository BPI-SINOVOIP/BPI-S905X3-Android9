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
 * Copyright (c) 2015-2017, The Linux Foundation.
 */

/*
 * Copyright 2012 Giesecke & Devrient GmbH.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy of
 * the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations under
 * the License.
 */
package com.android.se.security;

import com.android.se.internal.ByteArrayConverter;
import com.android.se.internal.Util;

/** Represents the masked APDU for the Access Rules */
public class ApduFilter {

    public static final int LENGTH = 8;
    protected byte[] mApdu;
    protected byte[] mMask;

    protected ApduFilter() {
    }

    public ApduFilter(byte[] apdu, byte[] mask) {
        if (apdu.length != 4) {
            throw new IllegalArgumentException("apdu length must be 4 bytes");
        }
        if (mask.length != 4) {
            throw new IllegalArgumentException("mask length must be 4 bytes");
        }
        mApdu = apdu;
        mMask = mask;
    }

    public ApduFilter(byte[] apduAndMask) {
        if (apduAndMask.length != 8) {
            throw new IllegalArgumentException("filter length must be 8 bytes");
        }
        mApdu = Util.getMid(apduAndMask, 0, 4);
        mMask = Util.getMid(apduAndMask, 4, 4);
    }

    /** Clones the APDU Filter */
    public ApduFilter clone() {
        ApduFilter apduFilter = new ApduFilter();
        apduFilter.setApdu(mApdu.clone());
        apduFilter.setMask(mMask.clone());
        return apduFilter;
    }

    public byte[] getApdu() {
        return mApdu;
    }

    /** Sets the APDU */
    public void setApdu(byte[] apdu) {
        if (apdu.length != 4) {
            throw new IllegalArgumentException("apdu length must be 4 bytes");
        }
        mApdu = apdu;
    }

    public byte[] getMask() {
        return mMask;
    }

    /** Sets the Mask */
    public void setMask(byte[] mask) {
        if (mask.length != 4) {
            throw new IllegalArgumentException("mask length must be 4 bytes");
        }
        mMask = mask;
    }

    /** Converts the filter into bytes */
    public byte[] toBytes() {
        return Util.mergeBytes(mApdu, mMask);
    }

    @Override
    public String toString() {
        return "APDU Filter [apdu="
                + ByteArrayConverter.byteArrayToHexString(mApdu)
                + ", mask="
                + ByteArrayConverter.byteArrayToHexString(mMask)
                + "]";
    }

    public int getLength() {
        return 8;
    }
}
