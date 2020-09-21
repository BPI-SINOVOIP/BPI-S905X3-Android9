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

package com.android.se.security.arf.pkcs15;

import android.util.Log;

import com.android.se.internal.Util;
import com.android.se.security.arf.ASN1;
import com.android.se.security.arf.DERParser;
import com.android.se.security.arf.SecureElement;
import com.android.se.security.arf.SecureElementException;

import java.io.IOException;

/** EF_DODF related features */
public class EFDODF extends EF {

    public static final String TAG = "ACE ARF EF_DODF";
    // OID defined by Global Platform for the "Access Control"
    public static final String AC_OID = "1.2.840.114283.200.1.1";

    /**
     * Constructor
     *
     * @param secureElement SE on which ISO7816 commands are applied
     */
    public EFDODF(SecureElement handle) {
        super(handle);
    }

    /**
     * Decodes EF_DODF file
     *
     * @param buffer ASN.1 data
     * @return Path to "Access Control Main" from "Access Control" OID; <code>null</code> otherwise
     */
    private byte[] decodeDER(byte[] buffer) throws PKCS15Exception {
        byte objectType;
        short[] context = null;
        DERParser der = new DERParser(buffer);

        while (!der.isEndofBuffer()) {
            if (der.parseTLV() == (byte) 0xA1) { // OidDO Data Object
                // Common Object Attributes
                der.parseTLV(ASN1.TAG_Sequence);
                der.skipTLVData();
                // Common Data Object Attributes
                der.parseTLV(ASN1.TAG_Sequence);
                der.skipTLVData();

                objectType = der.parseTLV();
                if (objectType == (byte) 0xA0) { // SubClassAttributes [Optional]
                    der.skipTLVData();
                    objectType = der.parseTLV();
                }
                if (objectType == (byte) 0xA1) { // OidDO
                    der.parseTLV(ASN1.TAG_Sequence);
                    context = der.saveContext();
                    if (der.parseOID().compareTo(AC_OID) != 0) {
                        der.restoreContext(context);
                        der.skipTLVData();
                    } else {
                        return der.parsePathAttributes();
                    }
                } else {
                    throw new PKCS15Exception("[Parser] OID Tag expected");
                }
            } else {
                der.skipTLVData();
            }
        }
        return null; // No "Access Control" OID found
    }

    /**
     * Selects and Analyses EF_DODF file
     *
     * @param path Path of the "EF_DODF" file
     * @return Path to "EF_ACMain" from "Access Control" OID; <code>null</code> otherwise
     */
    public byte[] analyseFile(byte[] path) throws IOException, PKCS15Exception,
            SecureElementException {
        Log.i(TAG, "Analysing EF_DODF...");

        if (selectFile(path) != APDU_SUCCESS) throw new PKCS15Exception("EF_DODF not found!");

        return decodeDER(readBinary(0, Util.END));
    }
}
