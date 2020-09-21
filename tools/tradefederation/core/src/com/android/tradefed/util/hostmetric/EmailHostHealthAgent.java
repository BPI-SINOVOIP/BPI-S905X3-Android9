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
package com.android.tradefed.util.hostmetric;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.Email;
import com.android.tradefed.util.IEmail;
import com.android.tradefed.util.IEmail.Message;

import org.json.JSONException;
import org.json.JSONObject;

import java.io.IOException;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

/** A {@link IHostHealthAgent} implementation to send email from Host monitor reports */
@OptionClass(alias = "host_metric_agent_email", global_namespace = false)
public class EmailHostHealthAgent implements IHostHealthAgent {

    @Option(
        name = "sender",
        description = "The envelope-sender address to use for the messages.",
        importance = Importance.IF_UNSET
    )
    private String mSender = null;

    @Option(
        name = "destination",
        description = "The destination email addresses. Can be repeated.",
        importance = Importance.IF_UNSET
    )
    private Collection<String> mDestinations = new HashSet<String>();

    private List<HostMetric> mMetrics = new LinkedList<>();

    /** {@inheritDoc} */
    @Override
    public void emitValue(String name, long value, Map<String, String> data) {
        mMetrics.add(new HostMetric(name, System.currentTimeMillis(), value, data));
    }

    /** {@inheritDoc} */
    @Override
    public void flush() {
        if (mMetrics.isEmpty()) {
            CLog.i("No metric to send, skipping HostMetricAgentEmail.");
            return;
        }
        if (mDestinations.isEmpty()) {
            CLog.w("No email sent because no destination addresses were set.");
            return;
        }
        if (mSender == null) {
            CLog.w("No email sent because no sender addresse was set.");
            return;
        }
        IEmail mMailer = new Email();
        Message msg = new Message();
        msg.setSender(mSender);
        msg.setSubject(generateEmailSubject());
        msg.setBody(generateEmailBody());
        msg.setHtml(false);
        Iterator<String> toAddress = mDestinations.iterator();
        while (toAddress.hasNext()) {
            msg.addTo(toAddress.next());
        }

        try {
            CLog.d(msg.getSubject());
            CLog.d(msg.getBody());
            mMailer.send(msg);
        } catch (IllegalArgumentException e) {
            CLog.e("Failed to send email");
            CLog.e(e);
        } catch (IOException e) {
            CLog.e("Failed to send email");
            CLog.e(e);
        }
    }

    private String generateEmailSubject() {
        final StringBuilder subj = new StringBuilder("Tradefed host: ");
        try {
            subj.append(InetAddress.getLocalHost().getHostName());
        } catch (UnknownHostException e) {
            subj.append("UNKNOWN");
        }
        subj.append(" - Health Report");
        return subj.toString();
    }

    private String generateEmailBody() {
        StringBuilder bodyBuilder = new StringBuilder();
        bodyBuilder.append("Reporting Event from Host Health Monitor:\n\n");

        while (!mMetrics.isEmpty()) {
            HostMetric tmp = mMetrics.remove(0);
            try {
                JSONObject metric = tmp.toJson();
                Iterator<?> i = metric.keys();
                while (i.hasNext()) {
                    String key = (String) i.next();
                    bodyBuilder.append(key).append(": ").append(metric.get(key)).append("\n");
                }
            } catch (JSONException e) {
                CLog.e(e);
            }
        }
        return bodyBuilder.toString();
    }
}
