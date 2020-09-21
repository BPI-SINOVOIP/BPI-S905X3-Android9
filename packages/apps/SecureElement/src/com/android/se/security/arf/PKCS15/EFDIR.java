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

import com.android.se.security.arf.ASN1;
import com.android.se.security.arf.DERParser;
import com.android.se.security.arf.SecureElement;
import com.android.se.security.arf.SecureElementException;

import java.io.IOException;
import java.util.Arrays;

/** EF_DIR related features */
public class EFDIR extends EF {

    public static final String TAG = "ACE ARF EF_Dir";
    // Standardized ID for EF_DIR file
    public static final byte[] EFDIR_PATH = {0x3F, 0x00, 0x2F, 0x00};

    /**
     * Constructor
     *
     * @param secureElement SE on which ISO7816 commands are applied
     */
    public EFDIR(SecureElement handle) {
        super(handle);
    }

    /**
     * Decodes EF_DIR file
     *
     * @param buffer ASN.1 data
     * @param aid    Record key to search for
     * @return Path to "EF_ODF" when an expected record is found; <code>null</code> otherwise
     */
    private byte[] decodeDER(byte[] buffer, byte[] aid) throws PKCS15Exception {
        DERParser der = new DERParser(buffer);
        der.parseTLV(ASN1.TAG_ApplTemplate);
        // Application Identifier
        der.parseTLV(ASN1.TAG_ApplIdentifier);
        if (!Arrays.equals(der.getTLVData(), aid)) return null; // Record for another AID

        // Application Label or Application Path
        byte objectType = der.parseTLV();
        if (objectType == ASN1.TAG_ApplLabel) {
            // Application Label [Optional]
            der.getTLVData();
            der.parseTLV(ASN1.TAG_ApplPath);
        } else if (objectType != ASN1.TAG_ApplPath) {
            throw new PKCS15Exception("[Parser] Application Tag expected");
        }
        // Application Path
        return der.getTLVData();
    }

    /**
     * Analyses DIR file and lookups for AID record
     *
     * @param aid Record key to search for
     * @return Path to "EF_ODF" when an expected record is found; <code>null</code> otherwise
     */
    public byte[] lookupAID(byte[] aid) throws IOException, PKCS15Exception,
            SecureElementException {
        Log.i(TAG, "Analysing EF_DIR...");

        if (selectFile(EFDIR_PATH) != APDU_SUCCESS) throw new PKCS15Exception("EF_DIR not found!!");

        byte[] data, ODFPath = null;
        short index = 1;
        while (index <= getFileNbRecords()) {
            data = readRecord(index++);
            if ((ODFPath = decodeDER(data, aid)) != null) break;
        }
        return ODFPath;
    }
}
