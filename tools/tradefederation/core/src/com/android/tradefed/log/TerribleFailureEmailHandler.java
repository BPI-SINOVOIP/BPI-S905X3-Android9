/*
 * Copyright (C) 2013 The Android Open Source Project
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

package com.android.tradefed.log;

import com.android.tradefed.config.Option;
import com.android.tradefed.config.Option.Importance;
import com.android.tradefed.config.OptionClass;
import com.android.tradefed.log.LogUtil.CLog;
import com.android.tradefed.util.Email;
import com.android.tradefed.util.IEmail;
import com.android.tradefed.util.IEmail.Message;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.Collection;
import java.util.HashSet;
import java.util.Iterator;

/**
 * A simple handler class that sends an email to interested people when a WTF
 * (What a Terrible Failure) error occurs within a Trade Federation instance.
 */
@OptionClass(alias = "wtf-email-handler")
public class TerribleFailureEmailHandler implements ITerribleFailureHandler {
    private static final String DEFAULT_SUBJECT_PREFIX = "WTF happened to tradefed";

    @Option(name = "sender",
            description = "The envelope-sender address to use for the messages.",
            importance = Importance.IF_UNSET)
    private String mSender = null;

    @Option(name = "destination",
            description = "One or more destination addresses.",
            importance = Importance.IF_UNSET)
    private Collection<String> mDestinations = new HashSet<String>();

    @Option(name = "subject-prefix",
            description = "The prefix to be added to the beginning of the email subject.")
    private String mSubjectPrefix = DEFAULT_SUBJECT_PREFIX;

    @Option(name = "min-email-interval",
            description = "The minimum interval between emails in ms. " +
                    "If a new WTF happens within this interval from the previous one, " +
                    "it will be ignored.")
    private long mMinEmailInterval = 5 * 60 * 1000;

    private IEmail mMailer;
    private long mLastEmailSentTime = 0;

    /**
     * Create a {@link TerribleFailureEmailHandler}
     */
    public TerribleFailureEmailHandler() {
        this(new Email());
    }

    /**
     * Create a {@link TerribleFailureEmailHandler} with a custom {@link IEmail}
     * instance to use.
     * <p/>
     * Exposed for unit testing.
     *
     * @param mailer the {@link IEmail} instance to use.
     */
    protected TerribleFailureEmailHandler(IEmail mailer) {
        mMailer = mailer;
    }

    /**
     * Adds an email destination address.
     *
     * @param dest
     */
    public void addDestination(String dest) {
        mDestinations.add(dest);
    }

    /**
     * Sets the email sender address.
     *
     * @param sender
     */
    public void setSender(String sender) {
        mSender = sender;
    }

    /**
     * Sets the minimum email interval.
     *
     * @param interval
     */
    public void setMinEmailInterval(long interval) {
        mMinEmailInterval = interval;
    }

    /**
     * Gets the local host name of the machine.
     *
     * @return the name of the host machine, or "unknown host" if unknown
     */
    protected String getLocalHostName() {
        try {
            return InetAddress.getLocalHost().getHostName();
        } catch (UnknownHostException e) {
            CLog.e(e);
            return "unknown host";
        }
    }

    /**
     * Gets the current time in milliseconds.
     */
    protected long getCurrentTimeMillis() {
        return System.currentTimeMillis();
    }

    /**
     * A method to generate the subject for email reports.
     * The subject will be formatted as:
     *     "<subject-prefix> on <local-host-name>"
     *
     * @return A {@link String} containing the subject to use for an email report
     */
    protected String generateEmailSubject() {
        final StringBuilder subj = new StringBuilder();

        subj.append(mSubjectPrefix);
        subj.append(" on ");
        subj.append(getLocalHostName());

        return subj.toString();
    }

    /**
     * A method to generate the body for WTF email reports.
     *
     * @param message summary of the terrible failure
     * @param cause throwable containing stack trace information
     * @return A {@link String} containing the body to use for an email report
     */
    protected String generateEmailBody(String message, Throwable cause) {
        StringBuilder bodyBuilder = new StringBuilder();

        bodyBuilder.append("host: ");
        bodyBuilder.append(getLocalHostName());
        bodyBuilder.append("\n\n");

        bodyBuilder.append("What a Terrible Failure!  ");
        bodyBuilder.append(message);
        bodyBuilder.append("\n\n");

        if (cause != null) {
            bodyBuilder.append("Invocation failed: ");
            bodyBuilder.append(getStackTraceString(cause));
            bodyBuilder.append("\n\n");
        }

        return bodyBuilder.toString();
    }

    /**
     * Generates a new email message based on the attributes already gathered
     * (subject, sender, destinations), as well as the description and cause (Optional)
     *
     * @param description Summary of the terrible failure
     * @param cause (Optional) Throwable that includes stack trace info
     * @return Message object with all email attributes populated
     */
    protected Message generateEmailMessage(String description, Throwable cause) {
        Message msg = new Message();
        msg.setSender(mSender);
        msg.setSubject(generateEmailSubject());
        msg.setBody(generateEmailBody(description, cause));
        msg.setHtml(false);
        Iterator<String> toAddress = mDestinations.iterator();
        while (toAddress.hasNext()) {
            msg.addTo(toAddress.next());
        }
        return msg;
    }

    /**
     * {@inheritDoc}
     */
    @Override
    public boolean onTerribleFailure(String description, Throwable cause) {
        if (mDestinations.isEmpty()) {
            CLog.e("Failed to send email because no destination addresses were set.");
            return false;
        }

        final long now = getCurrentTimeMillis();
        if (0 < mMinEmailInterval && now - mLastEmailSentTime < mMinEmailInterval) {
            // TODO: consider queuing up skipped failures and send it later.
            CLog.w("Skipped to send %s email: email interval %dms < %dms", DEFAULT_SUBJECT_PREFIX,
                    now - mLastEmailSentTime, mMinEmailInterval);
            return false;
        }

        Message msg = generateEmailMessage(description, cause);

        try {
            mMailer.send(msg);
        } catch (IllegalArgumentException e) {
            CLog.e("Failed to send %s email", DEFAULT_SUBJECT_PREFIX);
            CLog.e(e);
            return false;
        } catch (IOException e) {
            CLog.e("Failed to send %s email", DEFAULT_SUBJECT_PREFIX);
            CLog.e(e);
            return false;
        }

        mLastEmailSentTime = now;
        return true;
    }

    /**
     * A helper method that parses the stack trace string out of the throwable.
     *
     * @param t contains the stack trace information
     * @return A {@link String} containing the stack trace of the throwable.
     */
    private static String getStackTraceString(Throwable t) {
        if (t == null)
            return "";

        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);
        t.printStackTrace(pw);
        pw.flush();
        return sw.toString();
    }
}
