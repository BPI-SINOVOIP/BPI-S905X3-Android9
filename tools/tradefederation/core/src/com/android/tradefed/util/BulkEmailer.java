/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.tradefed.util;

import com.android.tradefed.config.ConfigurationException;
import com.android.tradefed.config.IConfiguration;
import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.IEmail.Message;

import java.io.IOException;

/**
 * A email sender utility that allows the following configuration:
 *  sent interval,initial burst size, recipients and the total number messages.
 */
@OptionClass(alias = "emailer")
public class BulkEmailer {
    @Option(name = "total-emails", description = "total number of emails to send",
            importance = Importance.IF_UNSET)
    private int mEmails = 0;

    @Option(name = "recipients", description = "a comma separate list of recipient email addresses",
            importance = Importance.IF_UNSET)
    private String mRecipients = null;

    @Option(name = "send-interval", description = "email send interval in milliseconds",
            importance = Importance.IF_UNSET)
    private int mInterval = 0;

    @Option(name = "initial-burst", description = "initial burst of email to send",
            importance = Importance.IF_UNSET)
    private int mInitialBurst = 0;

    @Option(name = "sender", description = "the sender email.", importance = Importance.NEVER)
    private String mSender = "android-power-lab-external@google.com";

    private static final String SUBJECT = "No emails to send";
    private static final String MESSAGE = "This is a test message!";

    public void sendEmailsBg() {
        new Thread(new Runnable() {
                @Override
            public void run() {
                sendEmails();
            }
        }).start();
    }

    public void sendEmails() {
        if (mEmails == 0) {
            CLog.d("No emails to send.");
            return;
        }

        Message msg = new Message();
        msg.setTos(mRecipients.split(","));
        msg.setBody(MESSAGE);
        msg.setSender(mSender);

        Email email = new Email();
        try {
            CLog.d("Sending initial burst of %d emails\n", mInitialBurst);
            for (int i = 1; i <= mInitialBurst; i++){
                msg.setSubject(SUBJECT + i);
                email.send(msg);
                Thread.sleep(2000);
            }
            for (int i = mInitialBurst; i <= mEmails; i++){
                msg.setSubject(SUBJECT + i);
                email.send(msg);
                CLog.i("Sending email %d/%d", i, mEmails);
                Thread.sleep(mInterval);
            }
        } catch (IOException iox) {
            CLog.e("exception sending email");
            CLog.e(iox);
        } catch (InterruptedException ix) {
            CLog.e("sleep interrupted");
            CLog.e(ix);
        }
    }

    /**
     * Helper method to load BulkMailer from config.
     * The config must include the following tag
     * <object type="emailer" class="com.android.tradefed.util.BulkEmailer">
     *
     * @param config the config
     * @return an instance of BulkEmailer
     * @throws ConfigurationException
     */
    public static BulkEmailer loadMailer(IConfiguration config) throws ConfigurationException {
        Object o = config.getConfigurationObject("emailer");
        if (o instanceof BulkEmailer) {
            return (BulkEmailer) o;
        }
        throw new ConfigurationException("Missing or invalid emailer definition in config");
    }
}
