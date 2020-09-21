/*
* Copyright (C) 2013 Samsung System LSI
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
package com.android.bluetooth.map;

import android.text.util.Rfc822Token;
import android.text.util.Rfc822Tokenizer;
import android.util.Base64;
import android.util.Log;

import java.io.UnsupportedEncodingException;
import java.nio.charset.Charset;
import java.nio.charset.IllegalCharsetNameException;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.Locale;
import java.util.UUID;

public class BluetoothMapbMessageMime extends BluetoothMapbMessage {

    public static class MimePart {
        public long mId = INVALID_VALUE;   /* The _id from the content provider, can be used to
                                            * sort the parts if needed */
        public String mContentType = null; /* The mime type, e.g. text/plain */
        public String mContentId = null;
        public String mContentLocation = null;
        public String mContentDisposition = null;
        public String mPartName = null;    /* e.g. text_1.txt*/
        public String mCharsetName = null; /* This seems to be a number e.g. 106 for UTF-8
                                              CharacterSets holds a method for the mapping. */
        public String mFileName = null;     /* Do not seem to be used */
        public byte[] mData = null;        /* The raw un-encoded data e.g. the raw
                                            * jpeg data or the text.getBytes("utf-8") */


        String getDataAsString() {
            String result = null;
            String charset = mCharsetName;
            // Figure out if we support the charset, else fall back to UTF-8, as this is what
            // the MAP specification suggest to use, and is compatible with US-ASCII.
            if (charset == null) {
                charset = "UTF-8";
            } else {
                charset = charset.toUpperCase();
                try {
                    if (!Charset.isSupported(charset)) {
                        charset = "UTF-8";
                    }
                } catch (IllegalCharsetNameException e) {
                    Log.w(TAG, "Received unknown charset: " + charset + " - using UTF-8.");
                    charset = "UTF-8";
                }
            }
            try {
                result = new String(mData, charset);
            } catch (UnsupportedEncodingException e) {
                /* This cannot happen unless Charset.isSupported() is out of sync with String */
                try {
                    result = new String(mData, "UTF-8");
                } catch (UnsupportedEncodingException e2) {
                    Log.e(TAG, "getDataAsString: " + e);
                }
            }
            return result;
        }

        public void encode(StringBuilder sb, String boundaryTag, boolean last)
                throws UnsupportedEncodingException {
            sb.append("--").append(boundaryTag).append("\r\n");
            if (mContentType != null) {
                sb.append("Content-Type: ").append(mContentType);
            }
            if (mCharsetName != null) {
                sb.append("; ").append("charset=\"").append(mCharsetName).append("\"");
            }
            sb.append("\r\n");
            if (mContentLocation != null) {
                sb.append("Content-Location: ").append(mContentLocation).append("\r\n");
            }
            if (mContentId != null) {
                sb.append("Content-ID: ").append(mContentId).append("\r\n");
            }
            if (mContentDisposition != null) {
                sb.append("Content-Disposition: ").append(mContentDisposition).append("\r\n");
            }
            if (mData != null) {
                /* TODO: If errata 4176 is adopted in the current form (it is not in either 1.1
                or 1.2),
                the below use of UTF-8 is not allowed, Base64 should be used for text. */

                if (mContentType != null && (mContentType.toUpperCase().contains("TEXT")
                        || mContentType.toUpperCase().contains("SMIL"))) {
                    String text = new String(mData, "UTF-8");
                    if (text.getBytes().length == text.getBytes("UTF-8").length) {
                        /* Add the header split empty line */
                        sb.append("Content-Transfer-Encoding: 8BIT\r\n\r\n");
                    } else {
                        /* Add the header split empty line */
                        sb.append("Content-Transfer-Encoding: Quoted-Printable\r\n\r\n");
                        text = BluetoothMapUtils.encodeQuotedPrintable(mData);
                    }
                    sb.append(text).append("\r\n");
                } else {
                    /* Add the header split empty line */
                    sb.append("Content-Transfer-Encoding: Base64\r\n\r\n");
                    sb.append(Base64.encodeToString(mData, Base64.DEFAULT)).append("\r\n");
                }
            }
            if (last) {
                sb.append("--").append(boundaryTag).append("--").append("\r\n");
            }
        }

        public void encodePlainText(StringBuilder sb) throws UnsupportedEncodingException {
            if (mContentType != null && mContentType.toUpperCase().contains("TEXT")) {
                String text = new String(mData, "UTF-8");
                if (text.getBytes().length != text.getBytes("UTF-8").length) {
                    text = BluetoothMapUtils.encodeQuotedPrintable(mData);
                }
                sb.append(text).append("\r\n");
            } else if (mContentType != null && mContentType.toUpperCase().contains("/SMIL")) {
                /* Skip the smil.xml, as no-one knows what it is. */
            } else {
                /* Not a text part, just print the filename or part name if they exist. */
                if (mPartName != null) {
                    sb.append("<").append(mPartName).append(">\r\n");
                } else {
                    sb.append("<").append("attachment").append(">\r\n");
                }
            }
        }
    }

    private long mDate = INVALID_VALUE;
    private String mSubject = null;
    private ArrayList<Rfc822Token> mFrom = null;   // Shall not be empty
    private ArrayList<Rfc822Token> mSender = null;   // Shall not be empty
    private ArrayList<Rfc822Token> mTo = null;     // Shall not be empty
    private ArrayList<Rfc822Token> mCc = null;     // Can be empty
    private ArrayList<Rfc822Token> mBcc = null;    // Can be empty
    private ArrayList<Rfc822Token> mReplyTo = null; // Can be empty
    private String mMessageId = null;
    private ArrayList<MimePart> mParts = null;
    private String mContentType = null;
    private String mBoundary = null;
    private boolean mTextonly = false;
    private boolean mIncludeAttachments;
    private boolean mHasHeaders = false;
    private String mMyEncoding = null;

    private String getBoundary() {
        // Include "=_" as these cannot occur in quoted printable text
        if (mBoundary == null) {
            mBoundary = "--=_" + UUID.randomUUID();
        }
        return mBoundary;
    }

    /**
     * @return the parts
     */
    public ArrayList<MimePart> getMimeParts() {
        return mParts;
    }

    public String getMessageAsText() {
        StringBuilder sb = new StringBuilder();
        if (mSubject != null && !mSubject.isEmpty()) {
            sb.append("<Sub:").append(mSubject).append("> ");
        }
        if (mParts != null) {
            for (MimePart part : mParts) {
                if (part.mContentType.toUpperCase().contains("TEXT")) {
                    sb.append(new String(part.mData));
                }
            }
        }
        return sb.toString();
    }

    public MimePart addMimePart() {
        if (mParts == null) {
            mParts = new ArrayList<BluetoothMapbMessageMime.MimePart>();
        }
        MimePart newPart = new MimePart();
        mParts.add(newPart);
        return newPart;
    }

    public String getDateString() {
        SimpleDateFormat format = new SimpleDateFormat("EEE, dd MMM yyyy HH:mm:ss Z", Locale.US);
        Date dateObj = new Date(mDate);
        return format.format(dateObj); // Format according to RFC 2822 page 14
    }

    public long getDate() {
        return mDate;
    }

    public void setDate(long date) {
        this.mDate = date;
    }

    public String getSubject() {
        return mSubject;
    }

    public void setSubject(String subject) {
        this.mSubject = subject;
    }

    public ArrayList<Rfc822Token> getFrom() {
        return mFrom;
    }

    public void setFrom(ArrayList<Rfc822Token> from) {
        this.mFrom = from;
    }

    public void addFrom(String name, String address) {
        if (this.mFrom == null) {
            this.mFrom = new ArrayList<Rfc822Token>(1);
        }
        this.mFrom.add(new Rfc822Token(name, address, null));
    }

    public ArrayList<Rfc822Token> getSender() {
        return mSender;
    }

    public void setSender(ArrayList<Rfc822Token> sender) {
        this.mSender = sender;
    }

    public void addSender(String name, String address) {
        if (this.mSender == null) {
            this.mSender = new ArrayList<Rfc822Token>(1);
        }
        this.mSender.add(new Rfc822Token(name, address, null));
    }

    public ArrayList<Rfc822Token> getTo() {
        return mTo;
    }

    public void setTo(ArrayList<Rfc822Token> to) {
        this.mTo = to;
    }

    public void addTo(String name, String address) {
        if (this.mTo == null) {
            this.mTo = new ArrayList<Rfc822Token>(1);
        }
        this.mTo.add(new Rfc822Token(name, address, null));
    }

    public ArrayList<Rfc822Token> getCc() {
        return mCc;
    }

    public void setCc(ArrayList<Rfc822Token> cc) {
        this.mCc = cc;
    }

    public void addCc(String name, String address) {
        if (this.mCc == null) {
            this.mCc = new ArrayList<Rfc822Token>(1);
        }
        this.mCc.add(new Rfc822Token(name, address, null));
    }

    public ArrayList<Rfc822Token> getBcc() {
        return mBcc;
    }

    public void setBcc(ArrayList<Rfc822Token> bcc) {
        this.mBcc = bcc;
    }

    public void addBcc(String name, String address) {
        if (this.mBcc == null) {
            this.mBcc = new ArrayList<Rfc822Token>(1);
        }
        this.mBcc.add(new Rfc822Token(name, address, null));
    }

    public ArrayList<Rfc822Token> getReplyTo() {
        return mReplyTo;
    }

    public void setReplyTo(ArrayList<Rfc822Token> replyTo) {
        this.mReplyTo = replyTo;
    }

    public void addReplyTo(String name, String address) {
        if (this.mReplyTo == null) {
            this.mReplyTo = new ArrayList<Rfc822Token>(1);
        }
        this.mReplyTo.add(new Rfc822Token(name, address, null));
    }

    public void setMessageId(String messageId) {
        this.mMessageId = messageId;
    }

    public String getMessageId() {
        return mMessageId;
    }

    public void setContentType(String contentType) {
        this.mContentType = contentType;
    }

    public String getContentType() {
        return mContentType;
    }

    public void setTextOnly(boolean textOnly) {
        this.mTextonly = textOnly;
    }

    public boolean getTextOnly() {
        return mTextonly;
    }

    public void setIncludeAttachments(boolean includeAttachments) {
        this.mIncludeAttachments = includeAttachments;
    }

    public boolean getIncludeAttachments() {
        return mIncludeAttachments;
    }

    public void updateCharset() {
        if (mParts != null) {
            mCharset = null;
            for (MimePart part : mParts) {
                if (part.mContentType != null && part.mContentType.toUpperCase().contains("TEXT")) {
                    mCharset = "UTF-8";
                    if (V) {
                        Log.v(TAG, "Charset set to UTF-8");
                    }
                    break;
                }
            }
        }
    }

    public int getSize() {
        int messageSize = 0;
        if (mParts != null) {
            for (MimePart part : mParts) {
                messageSize += part.mData.length;
            }
        }
        return messageSize;
    }

    /**
     * Encode an address header, and perform folding if needed.
     * @param sb The stringBuilder to write to
     * @param headerName The RFC 2822 header name
     * @param addresses the reformatted address substrings to encode.
     */
    public void encodeHeaderAddresses(StringBuilder sb, String headerName,
            ArrayList<Rfc822Token> addresses) {
        /* TODO: Do we need to encode the addresses if they contain illegal characters?
         * This depends of the outcome of errata 4176. The current spec. states to use UTF-8
         * where possible, but the RFCs states to use US-ASCII for the headers - hence encoding
         * would be needed to support non US-ASCII characters. But the MAP spec states not to
         * use any encoding... */
        int partLength, lineLength = 0;
        lineLength += headerName.getBytes().length;
        sb.append(headerName);
        for (Rfc822Token address : addresses) {
            partLength = address.toString().getBytes().length + 1;
            // Add folding if needed
            if (lineLength + partLength >= 998 /* max line length in RFC2822 */) {
                sb.append("\r\n "); // Append a FWS (folding whitespace)
                lineLength = 0;
            }
            sb.append(address.toString()).append(";");
            lineLength += partLength;
        }
        sb.append("\r\n");
    }

    public void encodeHeaders(StringBuilder sb) throws UnsupportedEncodingException {
        /* TODO: From RFC-4356 - about the RFC-(2)822 headers:
         *    "Current Internet Message format requires that only 7-bit US-ASCII
         *     characters be present in headers.  Non-7-bit characters in an address
         *     domain must be encoded with [IDN].  If there are any non-7-bit
         *     characters in the local part of an address, the message MUST be
         *     rejected.  Non-7-bit characters elsewhere in a header MUST be encoded
         *     according to [Hdr-Enc]."
         *    We need to add the address encoding in encodeHeaderAddresses, but it is not
         *    straight forward, as it is unclear how to do this.  */
        if (mDate != INVALID_VALUE) {
            sb.append("Date: ").append(getDateString()).append("\r\n");
        }
        /* According to RFC-2822 headers must use US-ASCII, where the MAP specification states
         * UTF-8 should be used for the entire <bmessage-body-content>. We let the MAP specification
         * take precedence above the RFC-2822.
         */
        /* If we are to use US-ASCII anyway, here is the code for it for base64.
          if (subject != null){
            // Use base64 encoding for the subject, as it may contain non US-ASCII characters or
            // other illegal (RFC822 header), and android do not seem to have encoders/decoders
            // for quoted-printables
            sb.append("Subject:").append("=?utf-8?B?");
            sb.append(Base64.encodeToString(subject.getBytes("utf-8"), Base64.DEFAULT));
            sb.append("?=\r\n");
        }*/
        if (mSubject != null) {
            sb.append("Subject: ").append(mSubject).append("\r\n");
        }
        if (mFrom == null) {
            sb.append("From: \r\n");
        }
        if (mFrom != null) {
            encodeHeaderAddresses(sb, "From: ", mFrom); // This includes folding if needed.
        }
        if (mSender != null) {
            encodeHeaderAddresses(sb, "Sender: ", mSender); // This includes folding if needed.
        }
        /* For MMS one recipient(to, cc or bcc) must exists, if none: 'To:  undisclosed-
         * recipients:;' could be used.
         */
        if (mTo == null && mCc == null && mBcc == null) {
            sb.append("To:  undisclosed-recipients:;\r\n");
        }
        if (mTo != null) {
            encodeHeaderAddresses(sb, "To: ", mTo); // This includes folding if needed.
        }
        if (mCc != null) {
            encodeHeaderAddresses(sb, "Cc: ", mCc); // This includes folding if needed.
        }
        if (mBcc != null) {
            encodeHeaderAddresses(sb, "Bcc: ", mBcc); // This includes folding if needed.
        }
        if (mReplyTo != null) {
            encodeHeaderAddresses(sb, "Reply-To: ", mReplyTo); // This includes folding if needed.
        }
        if (mIncludeAttachments) {
            if (mMessageId != null) {
                sb.append("Message-Id: ").append(mMessageId).append("\r\n");
            }
            if (mContentType != null) {
                sb.append("Content-Type: ")
                        .append(mContentType)
                        .append("; boundary=")
                        .append(getBoundary())
                        .append("\r\n");
            }
        }
        // If no headers exists, we still need two CRLF, hence keep it out of the if above.
        sb.append("\r\n");
    }

    /* Notes on MMS
     * ------------
     * According to rfc4356 all headers of a MMS converted to an E-mail must use
     * 7-bit encoding. According the the MAP specification only 8-bit encoding is
     * allowed - hence the bMessage-body should contain no SMTP headers. (Which makes
     * sense, since the info is already present in the bMessage properties.)
     * The result is that no information from RFC4356 is needed, since it does not
     * describe any mapping between MMS content and E-mail content.
     * Suggestion:
     * Clearly state in the MAP specification that
     * only the actual message content should be included in the <bmessage-body-content>.
     * Correct the Example to not include the E-mail headers, and in stead show how to
     * include a picture or another binary attachment.
     *
     * If the headers should be included, clearly state which, as the example clearly shows
     * that some of the headers should be excluded.
     * Additionally it is not clear how to handle attachments. There is a parameter in the
     * get message to include attachments, but since only 8-bit encoding is allowed,
     * (hence neither base64 nor binary) there is no mechanism to embed the attachment in
     * the <bmessage-body-content>.
     *
     * UPDATE: Errata 4176 allows the needed encoding typed inside the <bmessage-body-content>
     * including Base64 and Quoted Printables - hence it is possible to encode non-us-ascii
     * messages - e.g. pictures and utf-8 strings with non-us-ascii content.
     * It have not yet been adopted, but since the comments clearly suggest that it is allowed
     * to use encoding schemes for non-text parts, it is still not clear what to do about non
     * US-ASCII text in the headers.
     * */

    /**
     * Encode the bMessage as a Mime message(MMS/IM)
     * @return
     * @throws UnsupportedEncodingException
     */
    public byte[] encodeMime() throws UnsupportedEncodingException {
        ArrayList<byte[]> bodyFragments = new ArrayList<byte[]>();
        StringBuilder sb = new StringBuilder();
        int count = 0;
        String mimeBody;

        mEncoding = "8BIT"; // The encoding used

        encodeHeaders(sb);
        if (mParts != null) {
            if (!getIncludeAttachments()) {
                for (MimePart part : mParts) {
                    /* We call encode on all parts, to include a tag,
                     * where an attachment is missing. */
                    part.encodePlainText(sb);
                }
            } else {
                for (MimePart part : mParts) {
                    count++;
                    part.encode(sb, getBoundary(), (count == mParts.size()));
                }
            }
        }

        mimeBody = sb.toString();

        if (mimeBody != null) {
            // Replace any occurrences of END:MSG with \END:MSG
            String tmpBody = mimeBody.replaceAll("END:MSG", "/END\\:MSG");
            bodyFragments.add(tmpBody.getBytes("UTF-8"));
        } else {
            bodyFragments.add(new byte[0]);
        }

        return encodeGeneric(bodyFragments);
    }


    /**
     * Try to parse the hdrPart string as e-mail headers.
     * @param hdrPart The string to parse.
     * @return Null if the entire string were e-mail headers. The part of the string in which
     * no headers were found.
     */
    private String parseMimeHeaders(String hdrPart) {
        String[] headers = hdrPart.split("\r\n");
        if (D) {
            Log.d(TAG, "Header count=" + headers.length);
        }
        String header;
        mHasHeaders = false;

        for (int i = 0, c = headers.length; i < c; i++) {
            header = headers[i];
            if (D) {
                Log.d(TAG, "Header[" + i + "]: " + header);
            }
            /* We need to figure out if any headers are present, in cases where devices do
             * not follow the e-mail RFCs.
             * Skip empty lines, and then parse headers until a non-header line is found,
             * at which point we treat the remaining as plain text.
             */
            if (header.trim().isEmpty()) {
                continue;
            }
            String[] headerParts = header.split(":", 2);
            if (headerParts.length != 2) {
                // We treat the remaining content as plain text.
                StringBuilder remaining = new StringBuilder();
                for (; i < c; i++) {
                    remaining.append(headers[i]);
                }

                return remaining.toString();
            }

            String headerType = headerParts[0].toUpperCase();
            String headerValue = headerParts[1].trim();

            // Address headers
            /* If this is empty, the MSE needs to fill it in before sending the message.
             * This happens when sending the MMS.
             */
            if (headerType.contains("FROM")) {
                headerValue = BluetoothMapUtils.stripEncoding(headerValue);
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(headerValue);
                mFrom = new ArrayList<Rfc822Token>(Arrays.asList(tokens));
            } else if (headerType.contains("TO")) {
                headerValue = BluetoothMapUtils.stripEncoding(headerValue);
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(headerValue);
                mTo = new ArrayList<Rfc822Token>(Arrays.asList(tokens));
            } else if (headerType.contains("CC")) {
                headerValue = BluetoothMapUtils.stripEncoding(headerValue);
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(headerValue);
                mCc = new ArrayList<Rfc822Token>(Arrays.asList(tokens));
            } else if (headerType.contains("BCC")) {
                headerValue = BluetoothMapUtils.stripEncoding(headerValue);
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(headerValue);
                mBcc = new ArrayList<Rfc822Token>(Arrays.asList(tokens));
            } else if (headerType.contains("REPLY-TO")) {
                headerValue = BluetoothMapUtils.stripEncoding(headerValue);
                Rfc822Token[] tokens = Rfc822Tokenizer.tokenize(headerValue);
                mReplyTo = new ArrayList<Rfc822Token>(Arrays.asList(tokens));
            } else if (headerType.contains("SUBJECT")) { // Other headers
                mSubject = BluetoothMapUtils.stripEncoding(headerValue);
            } else if (headerType.contains("MESSAGE-ID")) {
                mMessageId = headerValue;
            } else if (headerType.contains("DATE")) {
                /* The date is not needed, as the time stamp will be set in the DB
                 * when the message is send. */
            } else if (headerType.contains("MIME-VERSION")) {
                /* The mime version is not needed */
            } else if (headerType.contains("CONTENT-TYPE")) {
                String[] contentTypeParts = headerValue.split(";");
                mContentType = contentTypeParts[0];
                // Extract the boundary if it exists
                for (int j = 1, n = contentTypeParts.length; j < n; j++) {
                    if (contentTypeParts[j].contains("boundary")) {
                        mBoundary = contentTypeParts[j].split("boundary[\\s]*=", 2)[1].trim();
                        // removing quotes from boundary string
                        if ((mBoundary.charAt(0) == '\"') && (
                                mBoundary.charAt(mBoundary.length() - 1) == '\"')) {
                            mBoundary = mBoundary.substring(1, mBoundary.length() - 1);
                        }
                        if (D) {
                            Log.d(TAG, "Boundary tag=" + mBoundary);
                        }
                    } else if (contentTypeParts[j].contains("charset")) {
                        mCharset = contentTypeParts[j].split("charset[\\s]*=", 2)[1].trim();
                    }
                }
            } else if (headerType.contains("CONTENT-TRANSFER-ENCODING")) {
                mMyEncoding = headerValue;
            } else {
                if (D) {
                    Log.w(TAG, "Skipping unknown header: " + headerType + " (" + header + ")");
                }
            }
        }
        return null;
    }

    private void parseMimePart(String partStr) {
        String[] parts = partStr.split("\r\n\r\n", 2); // Split the header from the body
        MimePart newPart = addMimePart();
        String partEncoding = mMyEncoding; /* Use the overall encoding as default */
        String body;

        String[] headers = parts[0].split("\r\n");
        if (D) {
            Log.d(TAG, "parseMimePart: headers count=" + headers.length);
        }

        if (parts.length != 2) {
            body = partStr;
        } else {
            for (String header : headers) {
                // Skip empty lines(the \r\n after the boundary tag) and endBoundary tags
                if ((header.length() == 0) || (header.trim().isEmpty()) || header.trim()
                        .equals("--")) {
                    continue;
                }

                String[] headerParts = header.split(":", 2);
                if (headerParts.length != 2) {
                    if (D) {
                        Log.w(TAG, "part-Header not formatted correctly: ");
                    }
                    continue;
                }
                if (D) {
                    Log.d(TAG, "parseMimePart: header=" + header);
                }
                String headerType = headerParts[0].toUpperCase();
                String headerValue = headerParts[1].trim();
                if (headerType.contains("CONTENT-TYPE")) {
                    String[] contentTypeParts = headerValue.split(";");
                    newPart.mContentType = contentTypeParts[0];
                    // Extract the boundary if it exists
                    for (int j = 1, n = contentTypeParts.length; j < n; j++) {
                        String value = contentTypeParts[j].toLowerCase();
                        if (value.contains("charset")) {
                            newPart.mCharsetName = value.split("charset[\\s]*=", 2)[1].trim();
                        }
                    }
                } else if (headerType.contains("CONTENT-LOCATION")) {
                    // This is used if the smil refers to a file name in its src
                    newPart.mContentLocation = headerValue;
                    newPart.mPartName = headerValue;
                } else if (headerType.contains("CONTENT-TRANSFER-ENCODING")) {
                    partEncoding = headerValue;
                } else if (headerType.contains("CONTENT-ID")) {
                    // This is used if the smil refers to a cid:<xxx> in it's src
                    newPart.mContentId = headerValue;
                } else if (headerType.contains("CONTENT-DISPOSITION")) {
                    // This is used if the smil refers to a cid:<xxx> in it's src
                    newPart.mContentDisposition = headerValue;
                } else {
                    if (D) {
                        Log.w(TAG, "Skipping unknown part-header: " + headerType + " (" + header
                                + ")");
                    }
                }
            }
            body = parts[1];
            if (body.length() > 2) {
                if (body.charAt(body.length() - 2) == '\r'
                        && body.charAt(body.length() - 2) == '\n') {
                    body = body.substring(0, body.length() - 2);
                }
            }
        }
        // Now for the body
        newPart.mData = decodeBody(body, partEncoding, newPart.mCharsetName);
    }

    private void parseMimeBody(String body) {
        MimePart newPart = addMimePart();
        newPart.mCharsetName = mCharset;
        newPart.mData = decodeBody(body, mMyEncoding, mCharset);
    }

    private byte[] decodeBody(String body, String encoding, String charset) {
        if (encoding != null && encoding.toUpperCase().contains("BASE64")) {
            return Base64.decode(body, Base64.DEFAULT);
        } else if (encoding != null && encoding.toUpperCase().contains("QUOTED-PRINTABLE")) {
            return BluetoothMapUtils.quotedPrintableToUtf8(body, charset);
        } else {
            // TODO: handle other encoding types? - here we simply store the string data as bytes
            try {

                return body.getBytes("UTF-8");
            } catch (UnsupportedEncodingException e) {
                // This will never happen, as UTF-8 is mandatory on Android platforms
            }
        }
        return null;
    }

    private void parseMime(String message) {
        /* Overall strategy for decoding:
         * 1) split on first empty line to extract the header
         * 2) unfold and parse headers
         * 3) split on boundary to split into parts (or use the remaining as a part,
         *    if part is not found)
         * 4) parse each part
         * */
        String[] messageParts;
        String[] mimeParts;
        String remaining = null;
        String messageBody = null;
        message = message.replaceAll("\\r\\n[ \\\t]+", ""); // Unfold
        messageParts = message.split("\r\n\r\n", 2); // Split the header from the body
        if (messageParts.length != 2) {
            // Handle entire message as plain text
            messageBody = message;
        } else {
            remaining = parseMimeHeaders(messageParts[0]);
            // If we have some text not being a header, add it to the message body.
            if (remaining != null) {
                messageBody = remaining + messageParts[1];
                if (D) {
                    Log.d(TAG, "parseMime remaining=" + remaining);
                }
            } else {
                messageBody = messageParts[1];
            }
        }

        if (mBoundary == null) {
            // If the boundary is not set, handle as non-multi-part
            parseMimeBody(messageBody);
            setTextOnly(true);
            if (mContentType == null) {
                mContentType = "text/plain";
            }
            mParts.get(0).mContentType = mContentType;
        } else {
            mimeParts = messageBody.split("--" + mBoundary);
            if (D) {
                Log.d(TAG, "mimePart count=" + mimeParts.length);
            }
            // Part 0 is the message to clients not capable of decoding MIME
            for (int i = 1; i < mimeParts.length - 1; i++) {
                String part = mimeParts[i];
                if (part != null && (part.length() > 0)) {
                    parseMimePart(part);
                }
            }
        }
    }

    /* Notes on SMIL decoding (from http://tools.ietf.org/html/rfc2557):
     * src="filename.jpg" refers to a part with Content-Location: filename.jpg
     * src="cid:1234@hest.net" refers to a part with Content-ID:<1234@hest.net>*/
    @Override
    public void parseMsgPart(String msgPart) {
        parseMime(msgPart);

    }

    @Override
    public void parseMsgInit() {
        // Not used for e-mail

    }

    @Override
    public byte[] encode() throws UnsupportedEncodingException {
        return encodeMime();
    }

}
