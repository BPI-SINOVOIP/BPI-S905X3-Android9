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
 * Copyright (C) 2011 Deutsche Telekom, A.G.
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
 * Contributed by: Giesecke & Devrient GmbH.
 */

package com.android.se.security.arf.pkcs15;

import android.util.Log;

import com.android.se.internal.ByteArrayConverter;
import com.android.se.internal.Util;
import com.android.se.security.arf.DERParser;
import com.android.se.security.arf.SecureElement;
import com.android.se.security.arf.SecureElementException;

import java.io.IOException;

/** EF_ODF related features */
public class EFODF extends EF {

    // Standardized ID for EF_ODF file
    public static final byte[] EFODF_PATH = {0x50, 0x31};
    public final String mTag = "SecureElement-ARF-EFODF";
    byte[] mCDFPath = null;

    /**
     * Constructor
     *
     * @param secureElement SE on which ISO7816 commands are applied
     */
    public EFODF(SecureElement handle) {
        super(handle);
    }

    /**
     * Decodes EF_ODF file
     *
     * @param buffer ASN.1 data
     * @return Path to "EF_DODF" from "DODF Tag" entry; <code>null</code> otherwise
     */
    private byte[] decodeDER(byte[] buffer) throws PKCS15Exception {
        DERParser der = new DERParser(buffer);
        while (!der.isEndofBuffer()) {
            if (der.parseTLV() == (byte) 0xA5) { // CDF
                mCDFPath = der.parsePathAttributes();
                Log.i(
                        mTag,
                        "ODF content found mCDFPath :" + ByteArrayConverter.byteArrayToHexString(
                                mCDFPath));
            } else {
                der.skipTLVData();
            }
        }

        DERParser der2 = new DERParser(buffer);
        while (!der2.isEndofBuffer()) {
            if (der2.parseTLV() == (byte) 0xA7) { // DODF
                return der2.parsePathAttributes();
            } else {
                der2.skipTLVData();
            }
        }
        return null;
    }

    public byte[] getCDFPath() {
        return mCDFPath;
    }

    /**
     * Selects and Analyses EF_ODF file
     *
     * @return Path to "EF_DODF" from "DODF Tag" entry; <code>null</code> otherwise
     */
    public byte[] analyseFile(byte[] pkcs15Path) throws IOException, PKCS15Exception,
            SecureElementException {
        Log.i(mTag, "Analysing EF_ODF...");

        // 2012-04-12
        // extend path if ODF path was determined from EF DIR.
        byte[] path = null;
        if (pkcs15Path != null) {
            path = new byte[pkcs15Path.length + EFODF_PATH.length];
            System.arraycopy(pkcs15Path, 0, path, 0, pkcs15Path.length);
            System.arraycopy(EFODF_PATH, 0, path, pkcs15Path.length, EFODF_PATH.length);
        } else {
            path = EFODF_PATH;
        }
        // ---

        if (selectFile(path) != APDU_SUCCESS) {
            // let's give it another try..
            try {
                synchronized (this) {
                    wait(1000);
                }
            } catch (InterruptedException e) {
                Log.e(mTag, "Interupted while waiting for EF_ODF : " + e);
            }
            if (selectFile(path) != APDU_SUCCESS) throw new PKCS15Exception("EF_ODF not found!!");
        }
        return decodeDER(readBinary(0, Util.END));
    }
}
