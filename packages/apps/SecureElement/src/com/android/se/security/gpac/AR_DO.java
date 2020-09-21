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
 * This class represents the Access rule data object (AR-DO), according to GP Secure Element Control
 * Access.
 *
 * <p>The AR-DO contains one or two access rules of type APDU or NFC.
 */
public class AR_DO extends BerTlv {

    public static final int TAG = 0xE3;

    private APDU_AR_DO mApduAr = null;
    private NFC_AR_DO mNfcAr = null;

    public AR_DO(byte[] rawData, int valueIndex, int valueLength) {
        super(rawData, TAG, valueIndex, valueLength);
    }

    public AR_DO(APDU_AR_DO apduArDo, NFC_AR_DO nfcArDo) {
        super(null, TAG, 0, 0);
        mApduAr = apduArDo;
        mNfcAr = nfcArDo;
    }

    public APDU_AR_DO getApduArDo() {
        return mApduAr;
    }

    public NFC_AR_DO getNfcArDo() {
        return mNfcAr;
    }

    @Override
    /**
     * Interpret value.
     *
     * <p>Tag: E3
     *
     * <p>Value: Value can contain APDU-AR-DO or NFC-AR-DO or APDU-AR-DO | NFC-AR-DO A concatenation
     * of one or two AR-DO(s). If two AR-DO(s) are present these must have different types.
     */
    public void interpret() throws ParserException {

        this.mApduAr = null;
        this.mNfcAr = null;

        byte[] data = getRawData();
        int index = getValueIndex();

        if (index + getValueLength() > data.length) {
            throw new ParserException("Not enough data for AR_DO!");
        }

        do {
            BerTlv temp = BerTlv.decode(data, index);

            if (temp.getTag() == APDU_AR_DO.TAG) { // APDU-AR-DO
                mApduAr = new APDU_AR_DO(data, temp.getValueIndex(), temp.getValueLength());
                mApduAr.interpret();
            } else if (temp.getTag() == NFC_AR_DO.TAG) { // NFC-AR-DO
                mNfcAr = new NFC_AR_DO(data, temp.getValueIndex(), temp.getValueLength());
                mNfcAr.interpret();
            } else {
                // un-comment following line if a more restrictive
                // behavior is necessary.
                // throw new ParserException("Invalid DO in AR-DO!");
            }
            index = temp.getValueIndex() + temp.getValueLength();
        } while (getValueIndex() + getValueLength() > index);

        if (mApduAr == null && mNfcAr == null) {
            throw new ParserException("No valid DO in AR-DO!");
        }
    }

    @Override
    /**
     * Interpret value.
     *
     * <p>Tag: E3
     *
     * <p>Value: Value can contain APDU-AR-DO or NFC-AR-DO or APDU-AR-DO | NFC-AR-DO A concatenation
     * of one or two AR-DO(s). If two AR-DO(s) are present these must have different types.
     */
    public void build(ByteArrayOutputStream stream) throws DO_Exception {

        // write tag
        stream.write(getTag());

        ByteArrayOutputStream temp = new ByteArrayOutputStream();
        if (mApduAr != null) {
            mApduAr.build(temp);
        }

        if (mNfcAr != null) {
            mNfcAr.build(temp);
        }

        BerTlv.encodeLength(temp.size(), stream);
        try {
            stream.write(temp.toByteArray());
        } catch (IOException e) {
            throw new DO_Exception("AR-DO Memory IO problem! " + e.getMessage());
        }
    }
}
