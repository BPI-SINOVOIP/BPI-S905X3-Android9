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

/** Matches the Tags for the data object */
public class Response_DO_Factory {

    /** Creates and interprets the bytes into the respective types */
    public static BerTlv createDO(byte[] data) throws ParserException {

        BerTlv tempTlv = BerTlv.decode(data, 0);

        BerTlv retTlv = null;

        switch (tempTlv.getTag()) {
            case Response_RefreshTag_DO.TAG:
                retTlv =
                        new Response_RefreshTag_DO(data, tempTlv.getValueIndex(),
                                tempTlv.getValueLength());
                break;
            case Response_ARAC_AID_DO.TAG:
                retTlv = new Response_ARAC_AID_DO(data, tempTlv.getValueIndex(),
                        tempTlv.getValueLength());
                break;

            case Response_ALL_AR_DO.TAG:
                retTlv = new Response_ALL_AR_DO(data, tempTlv.getValueIndex(),
                        tempTlv.getValueLength());
                break;
            case Response_AR_DO.TAG:
                retTlv = new Response_AR_DO(data, tempTlv.getValueIndex(),
                        tempTlv.getValueLength());
                break;
            default:
                retTlv = tempTlv;
        }

        retTlv.interpret();

        return retTlv;
    }
}
