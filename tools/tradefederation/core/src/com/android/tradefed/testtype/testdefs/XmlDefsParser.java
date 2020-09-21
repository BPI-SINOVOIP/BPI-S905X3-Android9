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
package com.android.tradefed.testtype.testdefs;

import com.android.tradefed.util.xml.AbstractXmlParser;

import org.xml.sax.Attributes;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.util.Collection;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * Parses a test_defs.xml file.
 * <p/>
 * See development/testrunner/test_defs.xsd for expected format
 */
class XmlDefsParser extends AbstractXmlParser {

    private Map<String, InstrumentationTestDef> mTestDefsMap;

    /**
     * SAX callback object. Handles parsing data from the xml tags.
     */
    private class DefsHandler extends DefaultHandler {

        private static final String TEST_TAG = "test";

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            if (TEST_TAG.equals(localName)) {
                final String defName = attributes.getValue("name");
                InstrumentationTestDef def = new InstrumentationTestDef(defName,
                        attributes.getValue("package"));
                def.setClassName(attributes.getValue("class"));
                def.setRunner(attributes.getValue("runner"));
                def.setContinuous("true".equals(attributes.getValue("continuous")));
                def.setCoverageTarget(attributes.getValue("coverage_target"));
                mTestDefsMap.put(defName, def);
            }
        }
    }

    XmlDefsParser() {
        // Uses a LinkedHashmap to have predictable iteration order
        mTestDefsMap = new LinkedHashMap<String, InstrumentationTestDef>();
    }

    /**
     * Gets the list of parsed test definitions. The element order should be consistent with the
     * order of elements in the parsed input.
     */
    public Collection<InstrumentationTestDef> getTestDefs() {
        return mTestDefsMap.values();
    }

    /**
     * {@inheritDoc}
     */
    @Override
    protected DefaultHandler createXmlHandler() {
        return new DefsHandler();
    }
}
