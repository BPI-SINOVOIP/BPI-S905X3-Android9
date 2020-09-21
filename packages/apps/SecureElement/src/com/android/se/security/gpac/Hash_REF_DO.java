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
 * Hash-REF-DO: The Hash-REF-DO is used for retrieving and storing the corresponding access rules
 * for a device application (which is identified by the hash value of its certificate) from and to
 * the ARA
 */
public class Hash_REF_DO extends BerTlv {

    public static final int TAG = 0xC1;
    public static final int SHA1_LEN = 20;

    private byte[] mHash = new byte[0];

    public Hash_REF_DO(byte[] rawData, int valueIndex, int valueLength) {
        super(rawData, TAG, valueIndex, valueLength);
    }

    public Hash_REF_DO(byte[] hash) {
        super(hash, TAG, 0, (hash == null ? 0 : hash.length));
        if (hash != null) mHash = hash;
    }

    public Hash_REF_DO() {
        super(null, TAG, 0, 0);
    }

    /**
     * Comapares two Hash_REF_DO objects and returns true if they are equal
     */
    public static boolean equals(Hash_REF_DO obj1, Hash_REF_DO obj2) {
        if (obj1 == null) {
            return (obj2 == null) ? true : false;
        }
        return obj1.equals(obj2);
    }

    public byte[] getHash() {
        return mHash;
    }

    @Override
    public String toString() {
        StringBuilder b = new StringBuilder();
        ByteArrayOutputStream out = new ByteArrayOutputStream();
        b.append("Hash_REF_DO: ");
        try {
            this.build(out);
            b.append(BerTlv.toHex(out.toByteArray()));
        } catch (Exception e) {
            b.append(e.getLocalizedMessage());
        }
        return b.toString();
    }

    /**
     * Tags: C1 Length: 0 or SHA1_LEN bytes
     *
     * <p>Value: Hash: identifies a specific device application Empty: refers to all device
     * applications
     *
     * <p>Length: SHA1_LEN for 20 bytes SHA-1 hash value 0 for empty value field
     */
    @Override
    public void interpret() throws ParserException {

        mHash = new byte[0];

        byte[] data = getRawData();
        int index = getValueIndex();

        // sanity checks
        if (getValueLength() != 0 && getValueLength() != SHA1_LEN) {
            throw new ParserException("Invalid value length for Hash-REF-DO!");
        }

        if (getValueLength() == SHA1_LEN) {
            if (index + getValueLength() > data.length) {
                throw new ParserException("Not enough data for Hash-REF-DO!");
            }

            mHash = new byte[getValueLength()];
            System.arraycopy(data, index, mHash, 0, getValueLength());
        }
    }

    /**
     * Tags: C1 Length: 0 or 20 bytes
     *
     * <p>Value: Hash: identifies a specific device application Empty: refers to all device
     * applications
     *
     * <p>Length: SHA1_LEN for 20 bytes SHA-1 hash value 0 for empty value field
     */
    @Override
    public void build(ByteArrayOutputStream stream) throws DO_Exception {

        // sanity checks
        if (!(mHash.length != SHA1_LEN || mHash.length != 0)) {
            throw new DO_Exception("Hash value must be " + SHA1_LEN + " bytes in length!");
        }

        stream.write(getTag());

        try {
            stream.write(mHash.length);
            stream.write(mHash);
        } catch (IOException ioe) {
            throw new DO_Exception("Hash could not be written!");
        }
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof Hash_REF_DO) {
            Hash_REF_DO hash_ref_do = (Hash_REF_DO) obj;
            if (getTag() == hash_ref_do.getTag()) {
                return Arrays.equals(mHash, hash_ref_do.mHash);
            }
        }
        return false;
    }
}
