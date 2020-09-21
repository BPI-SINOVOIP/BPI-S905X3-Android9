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
import com.android.se.security.arf.ASN1;
import com.android.se.security.arf.DERParser;
import com.android.se.security.arf.SecureElement;
import com.android.se.security.arf.SecureElementException;
import com.android.se.security.gpac.AID_REF_DO;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/** EF_ACRules related features */
public class EFACRules extends EF {

    public static final String TAG = "ACE ARF EF_ACRules";
    // AID used to store rules for default application
    public static final byte[] DEFAULT_APP = new byte[0];

    protected Map<String, byte[]> mAcConditionDataCache = new HashMap<String, byte[]>();

    /**
     * Constructor
     *
     * @param secureElement SE on which ISO7816 commands are applied
     */
    public EFACRules(SecureElement handle) {
        super(handle);
    }

    /**
     * Decodes EF_ACRules file
     *
     * @param buffer ASN.1 data
     */
    private void decodeDER(byte[] buffer) throws IOException, PKCS15Exception {
        byte[] aid = null;
        DERParser der = new DERParser(buffer);

        // mapping to GPAC data objects
        int tag = 0;

        while (!der.isEndofBuffer()) {
            der.parseTLV(ASN1.TAG_Sequence);
            switch (der.parseTLV()) {
                case (byte) 0xA0: // Restricted AID
                    der.parseTLV(ASN1.TAG_OctetString);
                    aid = der.getTLVData();
                    tag = AID_REF_DO.TAG;
                    break;
                case (byte) 0x81: // Rules for default Application
                    aid = null;
                    tag = AID_REF_DO.TAG_DEFAULT_APPLICATION;
                    break;
                case (byte) 0x82: // Rules for default case
                    aid = DEFAULT_APP;
                    tag = AID_REF_DO.TAG;
                    break;
                default:
                    throw new PKCS15Exception("[Parser] Unexpected ACRules entry");
            }
            byte[] path = der.parsePathAttributes();

            // 2012-09-04
            // optimization of reading EF ACCondition
            if (path != null) {
                String pathString = ByteArrayConverter.byteArrayToHexString(path);
                EFACConditions temp = new EFACConditions(mSEHandle, new AID_REF_DO(tag, aid));
                // check if EF was already read before
                if (this.mAcConditionDataCache.containsKey(pathString)) {
                    // yes, then reuse data
                    temp.addRestrictedHashesFromData(this.mAcConditionDataCache.get(pathString));
                } else {
                    // no, read EF and add to rules cache
                    temp.addRestrictedHashes(path);
                    if (temp.getData() != null) {
                        // if data are read the put it into cache.
                        this.mAcConditionDataCache.put(pathString, temp.getData());
                    }
                }
            }
        }
    }

    /**
     * Selects and Analyses EF_ACRules file
     *
     * @param path Path of the "EF_ACRules" file
     */
    public void analyseFile(byte[] path) throws IOException, PKCS15Exception,
            SecureElementException {
        Log.i(TAG, "Analysing EF_ACRules...");

        // clear EF AC Condition data cache.
        mAcConditionDataCache.clear();

        if (selectFile(path) != APDU_SUCCESS) throw new PKCS15Exception("EF_ACRules not found!!");

        try {
            decodeDER(readBinary(0, Util.END));
        } catch (PKCS15Exception e) {
            throw e;
        }
    }
}
