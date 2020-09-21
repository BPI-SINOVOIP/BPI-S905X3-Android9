/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.phone.testapps.imstestapp;

import android.telephony.SmsManager;
import android.telephony.SmsMessage;
import android.telephony.ims.stub.ImsSmsImplBase;

public class TestImsSmsImpl extends ImsSmsImplBase {

    @Override
    public void sendSms(int token, int messageRef, String format, String smsc, boolean isRetry,
            byte[] pdu) {
        // At this point, we will always fallback to CS if the phone tries to send an SMS with the
        // test app. Will expand in the future to include UI options for more testing.
        onSendSmsResult(token, messageRef, SEND_STATUS_ERROR_FALLBACK,
                SmsManager.RESULT_ERROR_NO_SERVICE);
    }

    @Override
    public void acknowledgeSms(int token, int messageRef, int result) {
        super.acknowledgeSms(token, messageRef, result);
    }

    @Override
    public void acknowledgeSmsReport(int token, int messageRef, int result) {
        super.acknowledgeSmsReport(token, messageRef, result);
    }

    @Override
    public String getSmsFormat() {
        return SmsMessage.FORMAT_3GPP;
    }
}
