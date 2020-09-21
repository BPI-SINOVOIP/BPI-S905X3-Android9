/*
 * Copyright (C) 2011 The Android Open Source Project
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

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;

/**
 * Interface for sending email.
 */
public interface IEmail {

    /**
     * A method to send a {@link Message}.  Verifies that the to, subject, and body fields of the
     * {@link Message} are not null, but does no verification beyond the null checks.
     *
     * Note that any SMTP-level errors are undetectable at this stage.  Because of the asynchronous
     * nature of email, they will generally be reported to the envelope-sender of the message.  In
     * that case, the envelope-sender will typically receive an email from MAILER-DAEMON with the
     * details of the error.
     *
     * @param msg The {@link IEmail.Message} to try to send
     * @throws IllegalArgumentException if any of the to, subject, or body fields of {@code msg} is
     *         null
     * @throws IOException if sending email failed in a synchronously-detectable way
     */
    public void send(Message msg) throws IllegalArgumentException, IOException;

    /**
     * Container for email message data.
     */
    public static class Message {
        static final String PLAIN = "text/plain";
        static final String HTML = "text/html";

        private Collection<String> mToAddrs = null;
        private Collection<String> mCcAddrs = null;
        private Collection<String> mBccAddrs = null;
        private String mSubject = null;
        private String mBody = null;
        private String mSender = null;
        private String mContentType = PLAIN;

        public Message() {}

        /**
         * Convenience constructor: create a simple message
         *
         * @param to Single destination address
         * @param subject Subject
         * @param body Message body
         */
        public Message(String to, String subject, String body) {
            addTo(to);
            setSubject(subject);
            setBody(body);
        }

        public void addTo(String address) {
            if (mToAddrs == null) {
                mToAddrs = new ArrayList<String>();
            }
            mToAddrs.add(address);
        }
        public void addCc(String address) {
            if (mCcAddrs == null) {
                mCcAddrs = new ArrayList<String>();
            }
            mCcAddrs.add(address);
        }
        public void addBcc(String address) {
            if (mBccAddrs == null) {
                mBccAddrs = new ArrayList<String>();
            }
            mBccAddrs.add(address);
        }
        public void setSubject(String subject) {
            mSubject = subject;
        }
        /**
         * Set the recipients. All previously added recipients will be replaced.
         * {@link #addTo(String)} to append to the recipients list.
         *
         * @param recipients an array of recipient email addresses
         */
        public void setTos(String[] recipients){
            mToAddrs = Arrays.asList(recipients);
        }
        public void setBody(String body) {
            mBody = body;
        }
        public void setSender(String sender) {
            mSender = sender;
        }
        public void setContentType(String contentType) {
            if (contentType == null) throw new NullPointerException();
            mContentType = contentType;
        }

        public void setHtml(boolean html) {
            if (html) {
                setContentType(HTML);
            } else {
                setContentType(PLAIN);
            }
        }

        public Collection<String> getTo() {
            return mToAddrs;
        }
        public Collection<String> getCc() {
            return mCcAddrs;
        }
        public Collection<String> getBcc() {
            return mBccAddrs;
        }
        public String getSubject() {
            return mSubject;
        }
        public String getBody() {
            return mBody;
        }

        public String getSender() {
            return mSender;
        }

        public String getContentType() {
            return mContentType;
        }

        public boolean isHtml() {
            return HTML.equals(mContentType);
        }
    }
}
