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

/**
 * REF-AR_DO: The REF-AR-DO contains access rules inclusively its corresponding references for the
 * SE application (AID reference) and device application (hash reference).
 */
public class REF_AR_DO extends BerTlv {

    public static final int TAG = 0xE2;

    private REF_DO mRefDo = null;
    private AR_DO mArDo = null;

    public REF_AR_DO(byte[] rawData, int valueIndex, int valueLength) {
        super(rawData, TAG, valueIndex, valueLength);
    }

    public REF_AR_DO() {
        super(null, TAG, 0, 0);
    }

    public REF_AR_DO(REF_DO refDo, AR_DO arDo) {
        super(null, TAG, 0, 0);
        mRefDo = refDo;
        mArDo = arDo;
    }

    public REF_DO getRefDo() {
        return mRefDo;
    }

    public AR_DO getArDo() {
        return mArDo;
    }

    /**
     * Interpret data.
     *
     * <p>Tags: E2 Length: n
     *
     * <p>Value: REF-DO | AR-DO: A concatenation of an REF-DO and an AR-DO. The REF-DO must
     * correspond
     * to the succeeding AR-DO.
     *
     * <p>Length: n bytes.
     */
    @Override
    public void interpret() throws ParserException {

        mRefDo = null;
        mArDo = null;

        byte[] data = getRawData();
        int index = getValueIndex();

        if (index + getValueLength() > data.length) {
            throw new ParserException("Not enough data for AR_DO!");
        }

        do {
            BerTlv temp = BerTlv.decode(data, index);
            if (temp.getTag() == REF_DO.TAG) { // REF-DO
                mRefDo = new REF_DO(data, temp.getValueIndex(), temp.getValueLength());
                mRefDo.interpret();
            } else if (temp.getTag() == AR_DO.TAG) { // AR-DO
                mArDo = new AR_DO(data, temp.getValueIndex(), temp.getValueLength());
                mArDo.interpret();
            } else {

                // uncomment following line if a more restrictive
                // behavior is necessary.
                // throw new ParserException("Invalid DO in REF-AR-DO!");
            }
            index = temp.getValueIndex() + temp.getValueLength();
        } while (getValueIndex() + getValueLength() > index);

        // check for mandatory TLVs.
        if (mRefDo == null) {
            throw new ParserException("Missing Ref-DO in REF-AR-DO!");
        }
        if (mArDo == null) {
            throw new ParserException("Missing AR-DO in REF-AR-DO!");
        }
    }

    /** Tag: E2 Length: n Value: REF-DO | AR-DO: A concatenation of an REF-DO and an AR-DO. */
    @Override
    public void build(ByteArrayOutputStream stream) throws DO_Exception {
        ByteArrayOutputStream temp = new ByteArrayOutputStream();

        if (mRefDo == null || mArDo == null) {
            throw new DO_Exception("REF-AR-DO: Required DO missing!");
        }
        stream.write(getTag());

        mRefDo.build(temp);
        mArDo.build(temp);

        byte[] data = temp.toByteArray();
        BerTlv.encodeLength(data.length, stream);
        try {
            stream.write(data);
        } catch (IOException e) {
            throw new DO_Exception("REF-AR-DO Memory IO problem! " + e.getMessage());
        }
    }
}
