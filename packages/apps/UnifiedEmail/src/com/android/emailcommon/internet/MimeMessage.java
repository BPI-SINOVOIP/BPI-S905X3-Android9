/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.emailcommon.internet;

import com.android.emailcommon.mail.Address;
import com.android.emailcommon.mail.Body;
import com.android.emailcommon.mail.BodyPart;
import com.android.emailcommon.mail.Message;
import com.android.emailcommon.mail.MessagingException;
import com.android.emailcommon.mail.Multipart;
import com.android.emailcommon.mail.Part;
import com.android.mail.utils.LogUtils;

import org.apache.james.mime4j.BodyDescriptor;
import org.apache.james.mime4j.ContentHandler;
import org.apache.james.mime4j.EOLConvertingInputStream;
import org.apache.james.mime4j.MimeStreamParser;
import org.apache.james.mime4j.field.DateTimeField;
import org.apache.james.mime4j.field.Field;

import android.text.TextUtils;

import java.io.BufferedWriter;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Locale;
import java.util.Stack;
import java.util.regex.Pattern;

/**
 * An implementation of Message that stores all of its metadata in RFC 822 and
 * RFC 2045 style headers.
 *
 * NOTE:  Automatic generation of a local message-id is becoming unwieldy and should be removed.
 * It would be better to simply do it explicitly on local creation of new outgoing messages.
 */
public class MimeMessage extends Message {
    private MimeHeader mHeader;
    private MimeHeader mExtendedHeader;

    // NOTE:  The fields here are transcribed out of headers, and values stored here will supersede
    // the values found in the headers.  Use caution to prevent any out-of-phase errors.  In
    // particular, any adds/changes/deletes here must be echoed by changes in the parse() function.
    private Address[] mFrom;
    private Address[] mTo;
    private Address[] mCc;
    private Address[] mBcc;
    private Address[] mReplyTo;
    private Date mSentDate;
    private Body mBody;
    protected int mSize;
    private boolean mInhibitLocalMessageId = false;
    private boolean mComplete = true;

    // Shared random source for generating local message-id values
    private static final java.util.Random sRandom = new java.util.Random();

    // In MIME, en_US-like date format should be used. In other words "MMM" should be encoded to
    // "Jan", not the other localized format like "Ene" (meaning January in locale es).
    // This conversion is used when generating outgoing MIME messages. Incoming MIME date
    // headers are parsed by org.apache.james.mime4j.field.DateTimeField which does not have any
    // localization code.
    private static final SimpleDateFormat DATE_FORMAT =
        new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss Z", Locale.US);

    // regex that matches content id surrounded by "<>" optionally.
    private static final Pattern REMOVE_OPTIONAL_BRACKETS = Pattern.compile("^<?([^>]+)>?$");
    // regex that matches end of line.
    private static final Pattern END_OF_LINE = Pattern.compile("\r?\n");

    public MimeMessage() {
        mHeader = null;
    }

    /**
     * Generate a local message id.  This is only used when none has been assigned, and is
     * installed lazily.  Any remote (typically server-assigned) message id takes precedence.
     * @return a long, locally-generated message-ID value
     */
    private static String generateMessageId() {
        final StringBuilder sb = new StringBuilder();
        sb.append("<");
        for (int i = 0; i < 24; i++) {
            // We'll use a 5-bit range (0..31)
            final int value = sRandom.nextInt() & 31;
            final char c = "0123456789abcdefghijklmnopqrstuv".charAt(value);
            sb.append(c);
        }
        sb.append(".");
        sb.append(Long.toString(System.currentTimeMillis()));
        sb.append("@email.android.com>");
        return sb.toString();
    }

    /**
     * Parse the given InputStream using Apache Mime4J to build a MimeMessage.
     *
     * @param in InputStream providing message content
     * @throws IOException
     * @throws MessagingException
     */
    public MimeMessage(InputStream in) throws IOException, MessagingException {
        parse(in);
    }

    private MimeStreamParser init() {
        // Before parsing the input stream, clear all local fields that may be superceded by
        // the new incoming message.
        getMimeHeaders().clear();
        mInhibitLocalMessageId = true;
        mFrom = null;
        mTo = null;
        mCc = null;
        mBcc = null;
        mReplyTo = null;
        mSentDate = null;
        mBody = null;

        final MimeStreamParser parser = new MimeStreamParser();
        parser.setContentHandler(new MimeMessageBuilder());
        return parser;
    }

    protected void parse(InputStream in) throws IOException, MessagingException {
        final MimeStreamParser parser = init();
        parser.parse(new EOLConvertingInputStream(in));
        mComplete = !parser.getPrematureEof();
    }

    public void parse(InputStream in, EOLConvertingInputStream.Callback callback)
            throws IOException, MessagingException {
        final MimeStreamParser parser = init();
        parser.parse(new EOLConvertingInputStream(in, getSize(), callback));
        mComplete = !parser.getPrematureEof();
    }

    /**
     * Return the internal mHeader value, with very lazy initialization.
     * The goal is to save memory by not creating the headers until needed.
     */
    private MimeHeader getMimeHeaders() {
        if (mHeader == null) {
            mHeader = new MimeHeader();
        }
        return mHeader;
    }

    @Override
    public Date getReceivedDate() throws MessagingException {
        return null;
    }

    @Override
    public Date getSentDate() throws MessagingException {
        if (mSentDate == null) {
            try {
                DateTimeField field = (DateTimeField)Field.parse("Date: "
                        + MimeUtility.unfoldAndDecode(getFirstHeader("Date")));
                mSentDate = field.getDate();
                // TODO: We should make it more clear what exceptions can be thrown here,
                // and whether they reflect a normal or error condition.
            } catch (Exception e) {
                LogUtils.v(LogUtils.TAG, "Message missing Date header");
            }
        }
        if (mSentDate == null) {
            // If we still don't have a date, fall back to "Delivery-date"
            try {
                DateTimeField field = (DateTimeField)Field.parse("Date: "
                        + MimeUtility.unfoldAndDecode(getFirstHeader("Delivery-date")));
                mSentDate = field.getDate();
                // TODO: We should make it more clear what exceptions can be thrown here,
                // and whether they reflect a normal or error condition.
            } catch (Exception e) {
                LogUtils.v(LogUtils.TAG, "Message also missing Delivery-Date header");
            }
        }
        return mSentDate;
    }

    @Override
    public void setSentDate(Date sentDate) throws MessagingException {
        setHeader("Date", DATE_FORMAT.format(sentDate));
        this.mSentDate = sentDate;
    }

    @Override
    public String getContentType() throws MessagingException {
        final String contentType = getFirstHeader(MimeHeader.HEADER_CONTENT_TYPE);
        if (contentType == null) {
            return "text/plain";
        } else {
            return contentType;
        }
    }

    @Override
    public String getDisposition() throws MessagingException {
        return getFirstHeader(MimeHeader.HEADER_CONTENT_DISPOSITION);
    }

    @Override
    public String getContentId() throws MessagingException {
        final String contentId = getFirstHeader(MimeHeader.HEADER_CONTENT_ID);
        if (contentId == null) {
            return null;
        } else {
            // remove optionally surrounding brackets.
            return REMOVE_OPTIONAL_BRACKETS.matcher(contentId).replaceAll("$1");
        }
    }

    public boolean isComplete() {
        return mComplete;
    }

    @Override
    public String getMimeType() throws MessagingException {
        return MimeUtility.getHeaderParameter(getContentType(), null);
    }

    @Override
    public int getSize() throws MessagingException {
        return mSize;
    }

    /**
     * Returns a list of the given recipient type from this message. If no addresses are
     * found the method returns an empty array.
     */
    @Override
    public Address[] getRecipients(RecipientType type) throws MessagingException {
        if (type == RecipientType.TO) {
            if (mTo == null) {
                mTo = Address.parse(MimeUtility.unfold(getFirstHeader("To")));
            }
            return mTo;
        } else if (type == RecipientType.CC) {
            if (mCc == null) {
                mCc = Address.parse(MimeUtility.unfold(getFirstHeader("CC")));
            }
            return mCc;
        } else if (type == RecipientType.BCC) {
            if (mBcc == null) {
                mBcc = Address.parse(MimeUtility.unfold(getFirstHeader("BCC")));
            }
            return mBcc;
        } else {
            throw new MessagingException("Unrecognized recipient type.");
        }
    }

    @Override
    public void setRecipients(RecipientType type, Address[] addresses) throws MessagingException {
        final int TO_LENGTH = 4;  // "To: "
        final int CC_LENGTH = 4;  // "Cc: "
        final int BCC_LENGTH = 5; // "Bcc: "
        if (type == RecipientType.TO) {
            if (addresses == null || addresses.length == 0) {
                removeHeader("To");
                this.mTo = null;
            } else {
                setHeader("To", MimeUtility.fold(Address.toHeader(addresses), TO_LENGTH));
                this.mTo = addresses;
            }
        } else if (type == RecipientType.CC) {
            if (addresses == null || addresses.length == 0) {
                removeHeader("CC");
                this.mCc = null;
            } else {
                setHeader("CC", MimeUtility.fold(Address.toHeader(addresses), CC_LENGTH));
                this.mCc = addresses;
            }
        } else if (type == RecipientType.BCC) {
            if (addresses == null || addresses.length == 0) {
                removeHeader("BCC");
                this.mBcc = null;
            } else {
                setHeader("BCC", MimeUtility.fold(Address.toHeader(addresses), BCC_LENGTH));
                this.mBcc = addresses;
            }
        } else {
            throw new MessagingException("Unrecognized recipient type.");
        }
    }

    /**
     * Returns the unfolded, decoded value of the Subject header.
     */
    @Override
    public String getSubject() throws MessagingException {
        return MimeUtility.unfoldAndDecode(getFirstHeader("Subject"));
    }

    @Override
    public void setSubject(String subject) throws MessagingException {
        final int HEADER_NAME_LENGTH = 9;     // "Subject: "
        setHeader("Subject", MimeUtility.foldAndEncode2(subject, HEADER_NAME_LENGTH));
    }

    @Override
    public Address[] getFrom() throws MessagingException {
        if (mFrom == null) {
            String list = MimeUtility.unfold(getFirstHeader("From"));
            if (list == null || list.length() == 0) {
                list = MimeUtility.unfold(getFirstHeader("Sender"));
            }
            mFrom = Address.parse(list);
        }
        return mFrom;
    }

    @Override
    public void setFrom(Address from) throws MessagingException {
        final int FROM_LENGTH = 6;  // "From: "
        if (from != null) {
            setHeader("From", MimeUtility.fold(from.toHeader(), FROM_LENGTH));
            this.mFrom = new Address[] {
                    from
                };
        } else {
            this.mFrom = null;
        }
    }

    @Override
    public Address[] getReplyTo() throws MessagingException {
        if (mReplyTo == null) {
            mReplyTo = Address.parse(MimeUtility.unfold(getFirstHeader("Reply-to")));
        }
        return mReplyTo;
    }

    @Override
    public void setReplyTo(Address[] replyTo) throws MessagingException {
        final int REPLY_TO_LENGTH = 10;  // "Reply-to: "
        if (replyTo == null || replyTo.length == 0) {
            removeHeader("Reply-to");
            mReplyTo = null;
        } else {
            setHeader("Reply-to", MimeUtility.fold(Address.toHeader(replyTo), REPLY_TO_LENGTH));
            mReplyTo = replyTo;
        }
    }

    /**
     * Set the mime "Message-ID" header
     * @param messageId the new Message-ID value
     * @throws MessagingException
     */
    @Override
    public void setMessageId(String messageId) throws MessagingException {
        setHeader("Message-ID", messageId);
    }

    /**
     * Get the mime "Message-ID" header.  This value will be preloaded with a locally-generated
     * random ID, if the value has not previously been set.  Local generation can be inhibited/
     * overridden by explicitly clearing the headers, removing the message-id header, etc.
     * @return the Message-ID header string, or null if explicitly has been set to null
     */
    @Override
    public String getMessageId() throws MessagingException {
        String messageId = getFirstHeader("Message-ID");
        if (messageId == null && !mInhibitLocalMessageId) {
            messageId = generateMessageId();
            setMessageId(messageId);
        }
        return messageId;
    }

    @Override
    public void saveChanges() throws MessagingException {
        throw new MessagingException("saveChanges not yet implemented");
    }

    @Override
    public Body getBody() throws MessagingException {
        return mBody;
    }

    @Override
    public void setBody(Body body) throws MessagingException {
        this.mBody = body;
        if (body instanceof Multipart) {
            final Multipart multipart = ((Multipart)body);
            multipart.setParent(this);
            setHeader(MimeHeader.HEADER_CONTENT_TYPE, multipart.getContentType());
            setHeader("MIME-Version", "1.0");
        }
        else if (body instanceof TextBody) {
            setHeader(MimeHeader.HEADER_CONTENT_TYPE, String.format("%s;\n charset=utf-8",
                    getMimeType()));
            setHeader(MimeHeader.HEADER_CONTENT_TRANSFER_ENCODING, "base64");
        }
    }

    protected String getFirstHeader(String name) throws MessagingException {
        return getMimeHeaders().getFirstHeader(name);
    }

    @Override
    public void addHeader(String name, String value) throws MessagingException {
        getMimeHeaders().addHeader(name, value);
    }

    @Override
    public void setHeader(String name, String value) throws MessagingException {
        getMimeHeaders().setHeader(name, value);
    }

    @Override
    public String[] getHeader(String name) throws MessagingException {
        return getMimeHeaders().getHeader(name);
    }

    @Override
    public void removeHeader(String name) throws MessagingException {
        getMimeHeaders().removeHeader(name);
        if ("Message-ID".equalsIgnoreCase(name)) {
            mInhibitLocalMessageId = true;
        }
    }

    /**
     * Set extended header
     *
     * @param name Extended header name
     * @param value header value - flattened by removing CR-NL if any
     * remove header if value is null
     * @throws MessagingException
     */
    @Override
    public void setExtendedHeader(String name, String value) throws MessagingException {
        if (value == null) {
            if (mExtendedHeader != null) {
                mExtendedHeader.removeHeader(name);
            }
            return;
        }
        if (mExtendedHeader == null) {
            mExtendedHeader = new MimeHeader();
        }
        mExtendedHeader.setHeader(name, END_OF_LINE.matcher(value).replaceAll(""));
    }

    /**
     * Get extended header
     *
     * @param name Extended header name
     * @return header value - null if header does not exist
     * @throws MessagingException
     */
    @Override
    public String getExtendedHeader(String name) throws MessagingException {
        if (mExtendedHeader == null) {
            return null;
        }
        return mExtendedHeader.getFirstHeader(name);
    }

    /**
     * Set entire extended headers from String
     *
     * @param headers Extended header and its value - "CR-NL-separated pairs
     * if null or empty, remove entire extended headers
     * @throws MessagingException
     */
    public void setExtendedHeaders(String headers) throws MessagingException {
        if (TextUtils.isEmpty(headers)) {
            mExtendedHeader = null;
        } else {
            mExtendedHeader = new MimeHeader();
            for (final String header : END_OF_LINE.split(headers)) {
                final String[] tokens = header.split(":", 2);
                if (tokens.length != 2) {
                    throw new MessagingException("Illegal extended headers: " + headers);
                }
                mExtendedHeader.setHeader(tokens[0].trim(), tokens[1].trim());
            }
        }
    }

    /**
     * Get entire extended headers as String
     *
     * @return "CR-NL-separated extended headers - null if extended header does not exist
     */
    public String getExtendedHeaders() {
        if (mExtendedHeader != null) {
            return mExtendedHeader.writeToString();
        }
        return null;
    }

    /**
     * Write message header and body to output stream
     *
     * @param out Output steam to write message header and body.
     */
    @Override
    public void writeTo(OutputStream out) throws IOException, MessagingException {
        final BufferedWriter writer = new BufferedWriter(new OutputStreamWriter(out), 1024);
        // Force creation of local message-id
        getMessageId();
        getMimeHeaders().writeTo(out);
        // mExtendedHeader will not be write out to external output stream,
        // because it is intended to internal use.
        writer.write("\r\n");
        writer.flush();
        if (mBody != null) {
            mBody.writeTo(out);
        }
    }

    @Override
    public InputStream getInputStream() throws MessagingException {
        return null;
    }

    class MimeMessageBuilder implements ContentHandler {
        private final Stack<Object> stack = new Stack<Object>();

        public MimeMessageBuilder() {
        }

        private void expect(Class<?> c) {
            if (!c.isInstance(stack.peek())) {
                throw new IllegalStateException("Internal stack error: " + "Expected '"
                        + c.getName() + "' found '" + stack.peek().getClass().getName() + "'");
            }
        }

        @Override
        public void startMessage() {
            if (stack.isEmpty()) {
                stack.push(MimeMessage.this);
            } else {
                expect(Part.class);
                try {
                    final MimeMessage m = new MimeMessage();
                    ((Part)stack.peek()).setBody(m);
                    stack.push(m);
                } catch (MessagingException me) {
                    throw new Error(me);
                }
            }
        }

        @Override
        public void endMessage() {
            expect(MimeMessage.class);
            stack.pop();
        }

        @Override
        public void startHeader() {
            expect(Part.class);
        }

        @Override
        public void field(String fieldData) {
            expect(Part.class);
            try {
                final String[] tokens = fieldData.split(":", 2);
                ((Part)stack.peek()).addHeader(tokens[0], tokens[1].trim());
            } catch (MessagingException me) {
                throw new Error(me);
            }
        }

        @Override
        public void endHeader() {
            expect(Part.class);
        }

        @Override
        public void startMultipart(BodyDescriptor bd) {
            expect(Part.class);

            final Part e = (Part)stack.peek();
            try {
                final MimeMultipart multiPart = new MimeMultipart(e.getContentType());
                e.setBody(multiPart);
                stack.push(multiPart);
            } catch (MessagingException me) {
                throw new Error(me);
            }
        }

        @Override
        public void body(BodyDescriptor bd, InputStream in) throws IOException {
            expect(Part.class);
            final Body body = MimeUtility.decodeBody(in, bd.getTransferEncoding());
            try {
                ((Part)stack.peek()).setBody(body);
            } catch (MessagingException me) {
                throw new Error(me);
            }
        }

        @Override
        public void endMultipart() {
            stack.pop();
        }

        @Override
        public void startBodyPart() {
            expect(MimeMultipart.class);

            try {
                final MimeBodyPart bodyPart = new MimeBodyPart();
                ((MimeMultipart)stack.peek()).addBodyPart(bodyPart);
                stack.push(bodyPart);
            } catch (MessagingException me) {
                throw new Error(me);
            }
        }

        @Override
        public void endBodyPart() {
            expect(BodyPart.class);
            stack.pop();
        }

        @Override
        public void epilogue(InputStream is) throws IOException {
            expect(MimeMultipart.class);
            final StringBuilder sb = new StringBuilder();
            int b;
            while ((b = is.read()) != -1) {
                sb.append((char)b);
            }
            // TODO: why is this commented out?
            // ((Multipart) stack.peek()).setEpilogue(sb.toString());
        }

        @Override
        public void preamble(InputStream is) throws IOException {
            expect(MimeMultipart.class);
            final StringBuilder sb = new StringBuilder();
            int b;
            while ((b = is.read()) != -1) {
                sb.append((char)b);
            }
            try {
                ((MimeMultipart)stack.peek()).setPreamble(sb.toString());
            } catch (MessagingException me) {
                throw new Error(me);
            }
        }

        @Override
        public void raw(InputStream is) throws IOException {
            throw new UnsupportedOperationException("Not supported");
        }
    }
}
