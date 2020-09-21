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
 * Copyright 2010 Giesecke & Devrient GmbH.
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

package com.android.se.security.ara;

import com.android.se.Channel;
import com.android.se.security.CommandApdu;
import com.android.se.security.ResponseApdu;
import com.android.se.security.gpac.BerTlv;
import com.android.se.security.gpac.ParserException;
import com.android.se.security.gpac.Response_DO_Factory;
import com.android.se.security.gpac.Response_RefreshTag_DO;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.security.AccessControlException;

/** Reads the ARA Rules from the Secure Element */
public class AccessRuleApplet {
    private static final int MAX_LEN = 0x00;
    private static final CommandApdu GET_ALL_CMD = new CommandApdu(0x80, 0xCA, 0xFF, 0x40, MAX_LEN);
    // MAX_LEN should be adapted by OEM, this is a defensive value since some devices/modems have
    // problems with Le=0x00 or 0xFF.
    private static final CommandApdu GET_NEXT_CMD = new CommandApdu(0x80, 0xCA, 0xFF, 0x60,
            MAX_LEN);
    private static final CommandApdu GET_REFRESH_TAG = new CommandApdu(0x80, 0xCA, 0xDF, 0x20,
            MAX_LEN);
    private final String mTag = "SecureElement-AccessRuleApplet";
    private Channel mChannel = null;

    public AccessRuleApplet(Channel channel) {
        mChannel = channel;
    }

    /** Reads all the access rules from the secure element */
    public byte[] readAllAccessRules() throws AccessControlException, IOException {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();
        int overallLen = 0;

        // send GET DATA (specific)
        CommandApdu apdu = (CommandApdu) GET_ALL_CMD.clone();
        ResponseApdu response = send(apdu);

        // OK
        if (response.isStatus(0x9000)) {
            // check if more data has to be fetched
            BerTlv tempTlv = null;
            try {
                tempTlv = BerTlv.decode(response.getData(), 0, false);
            } catch (ParserException e) {
                throw new AccessControlException(
                        "GET DATA (all) not successfull. Tlv encoding wrong.");
            }

            // the first data block contain the length of the TLV + Tag bytes + length bytes.
            overallLen = tempTlv.getValueLength() + tempTlv.getValueIndex();

            try {
                stream.write(response.getData());
            } catch (IOException e) {
                throw new AccessControlException("GET DATA (all) IO problem. " + e.getMessage());
            }

            int le;
            // send subsequent GET DATA (next) commands
            while (stream.size() < overallLen) {
                le = overallLen - stream.size();

                if (le > MAX_LEN) {
                    le = MAX_LEN;
                }
                // send GET DATA (next)
                apdu = (CommandApdu) GET_NEXT_CMD.clone();
                apdu.setLe(le);

                response = send(apdu);
                // OK
                if (response.isStatus(0x9000)) {
                    try {
                        stream.write(response.getData());
                    } catch (IOException e) {
                        throw new AccessControlException("GET DATA (next) IO problem. "
                                + e.getMessage());
                    }
                } else {
                    throw new AccessControlException("GET DATA (next) not successfull, SW1SW2="
                            + response.getSW1SW2());
                }
            }
            return stream.toByteArray();
            // referenced data not found
        } else if (response.isStatus(0x6A88)) {
            return null;
        } else {
            throw new AccessControlException("GET DATA (all) not successfull. SW1SW2="
                    + response.getSW1SW2());
        }
    }

    /** Fetches the Refresh Tag from the Secure Element */
    public byte[] readRefreshTag() throws AccessControlException, IOException {
        CommandApdu apdu = (CommandApdu) GET_REFRESH_TAG.clone();
        ResponseApdu response = send(apdu);
        // OK
        if (response.isStatus(0x9000)) {
            // check if more data has to be fetched
            BerTlv tempTlv = null;
            Response_RefreshTag_DO refreshDo;
            try {
                tempTlv = Response_DO_Factory.createDO(response.getData());
                if (tempTlv instanceof Response_RefreshTag_DO) {
                    refreshDo = (Response_RefreshTag_DO) tempTlv;
                    return refreshDo.getRefreshTagArray();
                } else {
                    throw new AccessControlException("GET REFRESH TAG returned invalid Tlv.");
                }
            } catch (ParserException e) {
                throw new AccessControlException(
                        "GET REFRESH TAG not successfull. Tlv encoding wrong.");
            }
        }
        throw new AccessControlException("GET REFRESH TAG not successfull.");
    }

    private ResponseApdu send(CommandApdu cmdApdu) throws IOException {
        byte[] response = mChannel.transmit(cmdApdu.toBytes());
        return new ResponseApdu(response);
    }
}
