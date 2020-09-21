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
package com.android.se.security.gpac;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.util.Arrays;

/**
 * The AID-REF-DO is used for retrieving and storing the corresponding access rules for an SE
 * application (which is identified by its AID) from and to the ARA. Two different AID reference
 * data objects exist and one of these can be chosen and applied for a GET DATA and STORE DATA
 * command
 */
public class AID_REF_DO extends BerTlv {

    public static final int TAG = 0x4F;
    public static final int TAG_DEFAULT_APPLICATION = 0xC0;

    private byte[] mAid = new byte[0];

    public AID_REF_DO(byte[] rawData, int tag, int valueIndex, int valueLength) {
        super(rawData, tag, valueIndex, valueLength);
    }

    public AID_REF_DO(int tag, byte[] aid) {
        super(aid, tag, 0, (aid == null ? 0 : aid.length));
        if (aid != null) mAid = aid;
    }

    public AID_REF_DO(int tag) {
        super(null, tag, 0, 0);
    }

    /**
     * Comapares two AID_REF_DO objects and returns true if they are equal
     */
    public static boolean equals(AID_REF_DO obj1, AID_REF_DO obj2) {
        if (obj1 == null) {
            return (obj2 == null) ? true : false;
        }
        return obj1.equals(obj2);
    }

    @Override
    public String toString() {
        StringBuilder b = new StringBuilder();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        b.append("AID_REF_DO: ");
        try {
            this.build(out);
            b.append(BerTlv.toHex(out.toByteArray()));
        } catch (Exception e) {
            b.append(e.getLocalizedMessage());
        }
        return b.toString();
    }

    public byte[] getAid() {
        return mAid;
    }

    /**
     * Tags: C0 -> Length: 0 -> Default selected application (all channels) 4F -> Length: 0 or 5 -
     * 16
     * bytes
     *
     * <p>Value: AID: identifies a specific application Empty: refers to all SE applications
     *
     * <p>Length: 5-16 for an AID according to ISO/IEC7816-5 0 for empty value field
     */
    @Override
    public void interpret() throws ParserException {

        mAid = null;

        byte[] data = getRawData();
        int index = getValueIndex();

        if (getTag() == TAG_DEFAULT_APPLICATION) {
            if (getValueLength() != 0) {
                throw new ParserException("Invalid value length for AID-REF-DO!");
            }
        } else if (getTag() == TAG) {

            // sanity checks
            if ((getValueLength() < 5 || getValueLength() > 16) && getValueLength() != 0) {
                throw new ParserException("Invalid value length for AID-REF-DO!");
            }

            if (index + getValueLength() > data.length) {
                throw new ParserException("Not enough data for AID-REF-DO!");
            }

            mAid = new byte[getValueLength()];
            System.arraycopy(data, index, mAid, 0, getValueLength());

        } else {
            throw new ParserException("Invalid Tag for AID-REF-DO!");
        }
    }

    /**
     * Tags: C0 -> Length: 0 -> Default selected application (all channels) 4F -> Length: 0 or 5 -
     * 16
     * bytes
     *
     * <p>Value: AID: identifies a specific application Empty: refers to all SE applications
     *
     * <p>Length: 5-16 for an AID according to ISO/IEC7816-5 0 for empty value field
     */
    @Override
    public void build(ByteArrayOutputStream stream) throws DO_Exception {

        if (getTag() == TAG_DEFAULT_APPLICATION) {
            if (mAid.length > 0) {
                throw new DO_Exception("No value allowed for default selected application!");
            }
            stream.write(getTag());
            stream.write(0x00);
        } else if (getTag() == TAG) {

            // sanity check
            if (getValueLength() != 0) {
                if (getValueLength() < 5 || getValueLength() > 16) {
                    throw new DO_Exception("Invalid length of AID!");
                }
            }

            stream.write(getTag());
            stream.write(mAid.length);
            try {
                stream.write(mAid);
            } catch (IOException ioe) {
                throw new DO_Exception("AID could not be written!");
            }
        } else {
            throw new DO_Exception("AID-REF-DO must either be C0 or 4F!");
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof AID_REF_DO) {
            AID_REF_DO aid_ref_do = (AID_REF_DO) obj;
            if (getTag() == aid_ref_do.getTag()) {
                return Arrays.equals(mAid, aid_ref_do.mAid);
            }
        }
        return false;
    }
}
