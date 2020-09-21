/*
 * Copyright (C) 2008 The Android Open Source Project
 *
 * Licensed under the Eclipse Public License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.eclipse.org/org/documents/epl-v10.php
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.ide.common.resources.platform;


import com.android.ide.common.api.IAttributeInfo.Format;
import com.android.ide.eclipse.mock.TestLogger;
import com.android.ide.eclipse.tests.AdtTestData;

import java.util.Map;

import junit.framework.TestCase;

public class AttrsXmlParserTest extends TestCase {

    private AttrsXmlParser mParser;
    private String mFilePath;

    private static final String MOCK_DATA_PATH =
        "com/android/ide/eclipse/testdata/mock_attrs.xml"; //$NON-NLS-1$

    @Override
    public void setUp() throws Exception {
        mFilePath = AdtTestData.getInstance().getTestFilePath(MOCK_DATA_PATH);
        mParser = new AttrsXmlParser(mFilePath, new TestLogger(), 100);
    }

    @Override
    public void tearDown() throws Exception {
    }

    public void testGetOsAttrsXmlPath() throws Exception {
        assertEquals(mFilePath, mParser.getOsAttrsXmlPath());
    }

    public final void testPreload() throws Exception {
        assertSame(mParser, mParser.preload());
    }


    public final void testLoadViewAttributes() throws Exception {
        mParser.preload();
        ViewClassInfo info = new ViewClassInfo(
                false /* isLayout */,
                "mock_android.something.Theme",      //$NON-NLS-1$
                "Theme");                            //$NON-NLS-1$
        mParser.loadViewAttributes(info);

        assertEquals("These are the standard attributes that make up a complete theme.", //$NON-NLS-1$
                info.getJavaDoc());
        AttributeInfo[] attrs = info.getAttributes();
        assertEquals(1, attrs.length);
        assertEquals("scrollbarSize", info.getAttributes()[0].getName());
        assertEquals(1, info.getAttributes()[0].getFormats().size());
        assertEquals(Format.DIMENSION, info.getAttributes()[0].getFormats().iterator().next());
    }

    public final void testEnumFlagValues() throws Exception {
        /* The XML being read contains:
            <!-- Standard orientation constant. -->
            <attr name="orientation">
                <!-- Defines an horizontal widget. -->
                <enum name="horizontal" value="0" />
                <!-- Defines a vertical widget. -->
                <enum name="vertical" value="1" />
            </attr>
         */

        mParser.preload();
        Map<String, Map<String, Integer>> attrMap = mParser.getEnumFlagValues();
        assertTrue(attrMap.containsKey("orientation"));

        Map<String, Integer> valueMap = attrMap.get("orientation");
        assertTrue(valueMap.containsKey("horizontal"));
        assertTrue(valueMap.containsKey("vertical"));
        assertEquals(Integer.valueOf(0), valueMap.get("horizontal"));
        assertEquals(Integer.valueOf(1), valueMap.get("vertical"));
    }

    public final void testDeprecated() throws Exception {
        mParser.preload();

        DeclareStyleableInfo dep = mParser.getDeclareStyleableList().get("DeprecatedTest");
        assertNotNull(dep);

        AttributeInfo[] attrs = dep.getAttributes();
        assertEquals(4, attrs.length);

        assertEquals("deprecated-inline", attrs[0].getName());
        assertEquals("In-line deprecated.", attrs[0].getDeprecatedDoc());
        assertEquals("Deprecated comments using delimiters.", attrs[0].getJavaDoc());

        assertEquals("deprecated-multiline", attrs[1].getName());
        assertEquals("Multi-line version of deprecated that works till the next tag.",
                attrs[1].getDeprecatedDoc());
        assertEquals("Deprecated comments on their own line.", attrs[1].getJavaDoc());

        assertEquals("deprecated-not", attrs[2].getName());
        assertEquals(null, attrs[2].getDeprecatedDoc());
        assertEquals("This attribute is not deprecated.", attrs[2].getJavaDoc());

        assertEquals("deprecated-no-javadoc", attrs[3].getName());
        assertEquals("There is no other javadoc here.", attrs[3].getDeprecatedDoc());
        assertEquals("", attrs[3].getJavaDoc());
    }
}
