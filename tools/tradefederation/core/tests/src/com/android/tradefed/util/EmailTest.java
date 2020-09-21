/*
 * Copyright (C) 2010 The Android Open Source Project
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

import com.android.tradefed.util.IEmail.Message;

import junit.framework.TestCase;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.HashMap;
import java.util.Map;

public class EmailTest extends TestCase {
    private TestEmail mEmail = null;

    /**
     * A mock class to let us capture the data that is passed to the process on stdin
     */
    class TestProcess extends Process {
        OutputStream mOutputStream = null;

        TestProcess(OutputStream stream) {
            mOutputStream = stream;
        }

        @Override
        public OutputStream getOutputStream() {
            return mOutputStream;
        }

        // Add stubs for all the abstract methods
        class StubInputStream extends InputStream {
            @Override
            public int read() {return 0;}
        }

        @Override
        public void destroy() {}
        @Override
        public int exitValue() {return 0;}
        @Override
        public int waitFor() {return 0;}
        @Override
        public InputStream getInputStream() {
            return new StubInputStream();
        }
        @Override
        public InputStream getErrorStream() {
            return new StubInputStream();
        }
    }

    /**
     * A mock class that doesn't actually run anything, but instead returns a mock Process to
     * capture the data that would be passed _to_ the process on stdin.  Java's naming of these
     * methods is insane.
     */
    class TestEmail extends Email {
        ByteArrayOutputStream mOutputStream = null;

        TestEmail() {
            mOutputStream = new ByteArrayOutputStream();
        }

        public String getMailerData() {
            return mOutputStream.toString();
        }

        @Override
        Process run(String[] cmd) {
            return new TestProcess(mOutputStream);
        }
    }

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mEmail = new TestEmail();
    }

    /**
     * Ensure that IllegalArgumentException is thrown when a message without a destination address
     * is sent.  Note that the address is not validated in any way (it could even be null)
     */
    public void testSendInval_destination() throws IOException {
        Message msg = new Message();
        msg.setSubject("subject");
        msg.setBody("body");

        try {
            mEmail.send(msg);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Ensure that IllegalArgumentException is thrown when a message without a subject is sent.
     */
    public void testSendInval_subject() throws IOException {
        Message msg = new Message("dest@ination.com", null, "body");
        try {
            mEmail.send(msg);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Ensure that IllegalArgumentException is thrown when a message without a body is sent.
     */
    public void testSendInval_body() throws IOException {
        Message msg = new Message("dest@ination.com", "subject", null);
        try {
            mEmail.send(msg);
            fail("IllegalArgumentException not thrown");
        } catch (IllegalArgumentException e) {
            // expected
        }
    }

    /**
     * Ensure that the email body is passed correctly to the mailer program's standard input
     */
    public void testSend_simple() throws IOException {
        final Message msg = new Message("dest@ination.com", "subject", "body");
        msg.setSender("or@igin.com");
        mEmail.send(msg);

        final Map<String, String> headers = new HashMap<String, String>();
        final String body = extractBody(mEmail.getMailerData(), headers);

        assertEquals("body", body);
        assertEquals("or@igin.com", headers.get("From"));
        assertEquals("dest@ination.com", headers.get("To"));
        assertEquals("subject", headers.get("Subject"));
        assertEquals(Message.PLAIN, headers.get("Content-type"));
        assertEquals(4, headers.size());
    }

    /**
     * Make sure that the HTML flag is passed along correctly
     */
    public void testSend_htmlEmail() throws IOException {
        final String expectedBody = "<html><body>le body</body></html>";
        final Message msg = new Message("dest@ination.com", "subject", expectedBody);
        msg.setHtml(true);
        mEmail.send(msg);

        final Map<String, String> headers = new HashMap<String, String>();
        final String body = extractBody(mEmail.getMailerData(), headers);

        assertEquals(expectedBody, body);
        assertEquals(Message.HTML, headers.get("Content-type"));
    }

    /**
     * Ensure that the email is sent correctly even if the message has an empty (but present)
     * subject
     */
    public void testSend_emptySubject() throws IOException {
        Message msg = new Message("dest@ination.com", "", "body");
        mEmail.send(msg);
        assertTrue("body".equals(extractBody(mEmail.getMailerData())));
    }

    /**
     * Ensure that the email is sent correctly even if the message has an empty (but present) body.
     */
    public void testSend_emptyBody() throws IOException {
        Message msg = new Message("dest@ination.com", "subject", "");
        mEmail.send(msg);
        assertTrue("".equals(extractBody(mEmail.getMailerData())));
    }

    /**
     * A short functional test to send an email somewhere.  Intended to be manually enabled with
     * appropriate email addresses to verify that Email functionality is working after changes.
     * Not enabled by default because the particular addresses to use will depend on the environment
     */
    @SuppressWarnings("unused")
    public void _manual_testFuncSend() throws IOException {
        final String sender = null;
        final String[] to = {"RECIPIENT"};
        final String[] cc = {};
        final String[] bcc = {};

        final String subject = "This is a TF test email";
        final String body = "<html><body><h1>What do we want?!</h1><h2>Time travel!</h2>" +
                "When do we want it?<span style=\"display:block;color:blue;font-weight:bold\">" +
                "it's irrelevant!</span></body></html>";
        final String contentType = Message.HTML;

        final Message msg = new Message();
        msg.setSubject(subject);
        msg.setBody(body);
        msg.setContentType(contentType);
        if (sender != null) {
            msg.setSender(sender);
        }
        for (String addr : to) {
            msg.addTo(addr);
        }
        for (String addr : cc) {
            msg.addCc(addr);
        }
        for (String addr : bcc) {
            msg.addBcc(addr);
        }

        final IEmail mailer = new Email();
        mailer.send(msg);
    }

    private String extractBody(String data) {
        return extractBody(data, null);
    }

    /**
     * Helper function that takes a full email and splits it into the headers and the body.
     * Optionally returns the headers via the second arg
     */
    private String extractBody(String data, Map<String, String> headers) {
        final String[] pieces = data.split(Email.CRLF + Email.CRLF, 2);
        if (headers != null) {
            for (String header : pieces[0].split(Email.CRLF)) {
                final String[] halves = header.split(": ", 2);
                final String key = halves[0];
                final String val = halves.length == 2 ? halves[1] : null;

                headers.put(key, val);
            }
        }

        if (pieces.length < 2) {
            return null;
        } else {
            return pieces[1];
        }
    }
}

