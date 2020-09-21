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

package com.android.tradefed.util.net;

import com.android.tradefed.util.xml.AbstractXmlParser;
import com.android.tradefed.util.xml.AbstractXmlParser.ParseException;

import org.kxml2.io.KXmlSerializer;
import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.IOException;
import java.io.InputStream;
import java.util.LinkedList;
import java.util.List;

/**
 * A mechanism to simplify writing XmlRpc.  Deals with XML and XmlRpc boilerplate.
 * <p/>
 * Call semantics:
 * <ol>
 * <li>Call an "Open" method</li>
 * <li>Construct the value on the serializer.  This may involve calling other helper methods,
 *     perhaps recursively.</li>
 * <li>Call a respective "Close" method</li>
 * </ol>
 * <p/>
 * It is the caller's responsibility to ensure that "Open" and "Close" calls are matched properly.
 * The helper methods do not check this.
 */
public class XmlRpcHelper {
    public static final String TRUE_VAL = "1";
    public static final String FALSE_VAL = "0";

    /**
     * Write the opening of a method call to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param name the name of the XmlRpc method to invoke
     */
    public static void writeOpenMethodCall(KXmlSerializer serializer, String ns, String name)
            throws IOException {
        serializer.startTag(ns, "methodCall");
        serializer.startTag(ns, "methodName");
        serializer.text(name);
        serializer.endTag(ns, "methodName");

        serializer.startTag(ns, "params");
    }

    /**
     * Write the end of a method call to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     */
    public static void writeCloseMethodCall(KXmlSerializer serializer, String ns)
            throws IOException {
        serializer.endTag(ns, "params");
        serializer.endTag(ns, "methodCall");
    }

    /**
     * Write the opening of a method argument to the serializer.  After calling this function, the
     * caller should send the argument value directly to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param valueType the XmlRpc type of the method argument
     */
    public static void writeOpenMethodArg(KXmlSerializer serializer, String ns, String valueType)
            throws IOException {
        serializer.startTag(ns, "param");
        serializer.startTag(ns, "value");
        serializer.startTag(ns, valueType);
    }

    /**
     * Write the end of a method argument to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param valueType the XmlRpc type of the method argument
     */
    public static void writeCloseMethodArg(KXmlSerializer serializer, String ns, String valueType)
            throws IOException {
        serializer.endTag(ns, valueType);
        serializer.endTag(ns, "value");
        serializer.endTag(ns, "param");
    }

    /**
     * Write a full method argument to the serializer.  This function is not paired with any other
     * function.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param valueType the XmlRpc type of the method argument
     * @param value the value of the method argument
     */
    public static void writeFullMethodArg(KXmlSerializer serializer, String ns, String valueType,
            String value) throws IOException {
        serializer.startTag(ns, "param");
        serializer.startTag(ns, valueType);
        serializer.text(value);
        serializer.endTag(ns, valueType);
        serializer.endTag(ns, "param");
    }

    /**
     * Write the opening of a struct member to the serializer.  After calling this function, the
     * caller should send the member value directly to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param name the name of the XmlRpc member
     * @param valueType the XmlRpc type of the member
     */
    public static void writeOpenStructMember(KXmlSerializer serializer, String ns, String name,
            String valueType) throws IOException {
        serializer.startTag(ns, "member");
        serializer.startTag(ns, "name");
        serializer.text(name);
        serializer.endTag(ns, "name");

        serializer.startTag(ns, "value");
        serializer.startTag(ns, valueType);
    }

    /**
     * Write the end of a struct member to the serializer.
     *
     * @param serializer the {@link KXmlSerializer}
     * @param ns the namespace
     * @param valueType the XmlRpc type of the member
     */
    public static void writeCloseStructMember(KXmlSerializer serializer, String ns,
            String valueType) throws IOException {
        serializer.endTag(ns, valueType);
        serializer.endTag(ns, "value");
        serializer.endTag(ns, "member");
    }

    private static class RpcResponseHandler extends DefaultHandler {
        private final List<String> mResponses = new LinkedList<String>();
        private String mType = null;
        private StringBuilder mValue = new StringBuilder();
        private boolean mInParams = false;
        private boolean mInValue = false;

        private static final String PARAMS_TAG = "params";
        private static final String VALUE_TAG = "value";
        private static final String PARAM_TAG = "param";

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            if (!mInParams && !PARAMS_TAG.equals(localName)) {
                // nothing to do yet
                return;
            } else if (PARAMS_TAG.equals(localName)) {
                mInParams = true;
                return;
            } else if (VALUE_TAG.equals(localName) || PARAM_TAG.equals(localName)) {
                // Treat <value> and <param> equivalently
                mInValue = true;
                return;
            } else if (mInParams && mInValue) {
                // expect this to be the data type
                mType = localName;
            }
        }

        @Override
        public void endElement(String uri, String localName, String qName) {
            if (PARAMS_TAG.equals(localName)) {
                mInParams = false;
            } else if (VALUE_TAG.equals(localName) || PARAM_TAG.equals(localName)) {
                // Treat <value> and <param> equivalently
                mInValue = false;
            } else if (mType != null && mType.equals(localName)) {
                // commit
                mResponses.add(mType);
                mResponses.add(mValue.toString());

                // clear
                mType = null;
                mValue.delete(0, mValue.length());
            }
        }

        @Override
        public void characters(char[] ch, int start, int length) throws SAXException {
            if (mType == null) {
                // nothing to do
                return;
            }

            mValue.append(ch, start, length);
        }

        public List<String> getResponses() {
            return mResponses;
        }
    }

    private static class XmlRpcResponseParser extends AbstractXmlParser {
        private RpcResponseHandler mHandler = new RpcResponseHandler();

        @Override
        protected DefaultHandler createXmlHandler() {
            return mHandler;
        }

        public List<String> getResponses() {
            return mHandler.getResponses();
        }
    }

    /**
     * Parses an XmlRpc response document.  Returns a flat list of pairs; the even elements are
     * datatype names, and the odds are string representations of the values, as passed over the
     * wire.
     *
     * @param input An {@link InputStream} from which the parser can read the XmlRpc response
     *        document.
     * @return A flat {@code List<String>} containing datatype/value pairs, or {@code null} if
     *         there was a parse error.
     */
    public static List<String> parseResponseTuple(InputStream input) {
        XmlRpcResponseParser parser = new XmlRpcResponseParser();
        try {
            parser.parse(input);
            return parser.getResponses();
        } catch (ParseException e) {
            System.err.format("got parse exception %s\n", e);
        }
        return null;
    }
}

