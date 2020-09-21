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
package com.android.tradefed.util.xml;

import com.android.ddmlib.Log;

import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.IOException;
import java.io.InputStream;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

/**
 * Helper base class for parsing xml files
 */
public abstract class AbstractXmlParser {

    private static final String LOG_TAG = "XmlDefsParser";

    /**
     * Thrown if XML input could not be parsed
     */
    @SuppressWarnings("serial")
    public static class ParseException extends Exception {
        public ParseException(Throwable cause) {
            super(cause);
        }
    }

    /**
     * Parses out xml data contained in given input.
     *
     * @param xmlInput
     * @throws ParseException if input could not be parsed
     */
    public void parse(InputStream xmlInput) throws ParseException  {
        try {
            SAXParserFactory parserFactory = SAXParserFactory.newInstance();
            parserFactory.setNamespaceAware(true);
            SAXParser parser;
            parser = parserFactory.newSAXParser();

            DefaultHandler handler = createXmlHandler();
            parser.parse(new InputSource(xmlInput), handler);
        } catch (ParserConfigurationException | SAXException | IOException e) {
            Log.e(LOG_TAG, e);
            throw new ParseException(e);
        }
    }

    /**
     * Creates a {@link DefaultHandler} to process the xml
     */
    protected abstract DefaultHandler createXmlHandler();

}
