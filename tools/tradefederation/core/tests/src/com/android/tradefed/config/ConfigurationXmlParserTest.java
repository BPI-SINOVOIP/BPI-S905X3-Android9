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
package com.android.tradefed.config;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import org.easymock.EasyMock;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.io.InputStream;
import java.util.Collections;
import java.util.Map;

/** Unit tests for {@link ConfigurationXmlParser}. */
@RunWith(JUnit4.class)
public class ConfigurationXmlParserTest {

    private ConfigurationXmlParser xmlParser;
    private IConfigDefLoader mMockLoader;

    @Before
    public void setUp() throws Exception {
        mMockLoader = EasyMock.createMock(IConfigDefLoader.class);
        xmlParser = new ConfigurationXmlParser(mMockLoader, null);
    }

    /**
     * Normal case test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String,
     * InputStream, Map)}.
     */
    @Test
    public void testParse() throws ConfigurationException {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <test class=\"junit.framework.TestCase\">\n" +
            "    <option name=\"opName\" value=\"val\" />\n" +
            "  </test>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
        assertEquals(configName, configDef.getName());
        assertEquals("desc", configDef.getDescription());
        assertEquals(
                "junit.framework.TestCase",
                configDef.getObjectClassMap().get("test").get(0).mClassName);
        assertEquals("junit.framework.TestCase:1:opName", configDef.getOptionList().get(0).name);
        assertEquals("val", configDef.getOptionList().get(0).value);
    }

    /** Test parsing xml with a global option */
    @Test
    public void testParse_globalOption() throws ConfigurationException {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <option name=\"opName\" value=\"val\" />\n" +
            "  <test class=\"junit.framework.TestCase\">\n" +
            "  </test>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
        assertEquals(configName, configDef.getName());
        assertEquals("desc", configDef.getDescription());
        assertEquals(
                "junit.framework.TestCase",
                configDef.getObjectClassMap().get("test").get(0).mClassName);
        // the non-namespaced option value should be used
        assertEquals("opName", configDef.getOptionList().get(0).name);
        assertEquals("val", configDef.getOptionList().get(0).value);
    }

    /** Test parsing xml with repeated type/class pairs */
    @Test
    public void testParse_multiple() throws ConfigurationException {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <test class=\"com.android.tradefed.testtype.HostTest\">\n" +
            "    <option name=\"class\" value=\"val1\" />\n" +
            "  </test>\n" +
            "  <test class=\"com.android.tradefed.testtype.HostTest\">\n" +
            "    <option name=\"class\" value=\"val2\" />\n" +
            "  </test>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);

        assertEquals(configName, configDef.getName());
        assertEquals("desc", configDef.getDescription());

        assertEquals(
                "com.android.tradefed.testtype.HostTest",
                configDef.getObjectClassMap().get("test").get(0).mClassName);
        assertEquals("com.android.tradefed.testtype.HostTest:1:class", configDef.getOptionList().get(0).name);
        assertEquals("val1", configDef.getOptionList().get(0).value);

        assertEquals(
                "com.android.tradefed.testtype.HostTest",
                configDef.getObjectClassMap().get("test").get(1).mClassName);
        assertEquals("com.android.tradefed.testtype.HostTest:2:class", configDef.getOptionList().get(1).name);
        assertEquals("val2", configDef.getOptionList().get(1).value);
    }

    /** Test parsing a object tag missing a attribute. */
    @Test
    public void testParse_objectMissingAttr() {
        final String config =
            "<object name=\"foo\" />";
        try {
            xmlParser.parse(new ConfigurationDef("foo"), "foo", getStringAsStream(config), null);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /** Test parsing a option tag missing a attribute. */
    @Test
    public void testParse_optionMissingAttr() {
        final String config =
            "<option name=\"foo\" />";
        try {
            xmlParser.parse(new ConfigurationDef("name"), "name", getStringAsStream(config), null);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /** Test parsing a object tag. */
    @Test
    public void testParse_object() throws ConfigurationException {
        final String config =
            "<object type=\"foo\" class=\"junit.framework.TestCase\" />";
        ConfigurationDef configDef = new ConfigurationDef("name");
        xmlParser.parse(configDef, "name", getStringAsStream(config), null);
        assertEquals(
                "junit.framework.TestCase",
                configDef.getObjectClassMap().get("foo").get(0).mClassName);
    }

    /** Test parsing a include tag. */
    @Test
    public void testParse_include() throws ConfigurationException {
        String includedName = "includeme";
        ConfigurationDef configDef = new ConfigurationDef("foo");
        mMockLoader.loadIncludedConfiguration(
                EasyMock.eq(configDef),
                EasyMock.eq("foo"),
                EasyMock.eq(includedName),
                EasyMock.anyObject(),
                EasyMock.anyObject());
        EasyMock.replay(mMockLoader);
        final String config = "<include name=\"includeme\" />";
        xmlParser.parse(configDef, "foo", getStringAsStream(config), null);
    }

    /** Test parsing a include tag where named config does not exist */
    @Test
    public void testParse_includeMissing() throws ConfigurationException {
        String includedName = "non-existent";
        ConfigurationDef parent = new ConfigurationDef("name");
        ConfigurationException exception = new ConfigurationException("I don't exist");
        mMockLoader.loadIncludedConfiguration(
                parent, "name", includedName, null, Collections.<String, String>emptyMap());
        EasyMock.expectLastCall().andThrow(exception);
        EasyMock.replay(mMockLoader);
        final String config = String.format("<include name=\"%s\" />", includedName);
        try {
            xmlParser.parse(parent, "name", getStringAsStream(config), null);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /** Test parsing a tag whose name is not recognized. */
    @Test
    public void testParse_badTag() {
        final String config = "<blah name=\"foo\" />";
        try {
            xmlParser.parse(new ConfigurationDef("name"), "name", getStringAsStream(config), null);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    /** Test parsing invalid xml. */
    @Test
    public void testParse_xml() {
        final String config = "blah";
        try {
            xmlParser.parse(new ConfigurationDef("name"), "name", getStringAsStream(config), null);
            fail("ConfigurationException not thrown");
        } catch (ConfigurationException e) {
            // expected
        }
    }

    private InputStream getStringAsStream(String input) {
        return new ByteArrayInputStream(input.getBytes());
    }

    /**
     * Normal case test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String,
     * InputStream, Map)}. when presented a device tag.
     */
    @Test
    public void testParse_deviceTag() throws ConfigurationException {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"device1\">\n" +
            "    <option name=\"opName\" value=\"val\" />\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
        assertEquals(configName, configDef.getName());
        assertEquals("desc", configDef.getDescription());
        // Option is preprended with the device name.
        assertEquals("{device1}opName", configDef.getOptionList().get(0).name);
        assertEquals("val", configDef.getOptionList().get(0).value);
    }

    /**
     * Test case for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream,
     * Map)}. when presented a device tag with no name.
     */
    @Test
    public void testParse_deviceTagNoName() {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device>\n" +
            "    <option name=\"opName\" value=\"val\" />\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        String expectedException = "device tag requires a name value";
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch (ConfigurationException expected) {
            assertEquals(expectedException, expected.getMessage());
        }
    }

    /**
     * Test case for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream,
     * Map)}. when presented a device tag with a used name, should merge them.
     */
    @Test
    public void testParse_deviceTagSameName() {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"device1\">\n" +
            "    <option name=\"opName\" value=\"val\" />\n" +
            "  </device>\n" +
            "  <device name=\"device2\">\n" +
            "    <option name=\"opName3\" value=\"val3\" />\n" +
            "  </device>\n" +
            "  <device name=\"device1\">\n" +
            "    <option name=\"opName2\" value=\"val2\" />\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            assertTrue(configDef.getObjectClassMap().get(Configuration.DEVICE_NAME).size() == 2);
            assertTrue("{device1}opName".equals(configDef.getOptionList().get(0).name));
            assertEquals("{device2}opName3", configDef.getOptionList().get(1).name);
            assertTrue("{device1}opName2".equals(configDef.getOptionList().get(2).name));
        } catch(ConfigurationException unExpected) {
            fail("No exception should have been thrown.");
        }
    }

    /**
     * Test case for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream,
     * Map)}. when presented an object tag outside of the device tag but should be inside.
     */
    @Test
    public void testParse_deviceTagAndObjectOutside() {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"device1\">\n" +
            "    <option name=\"opName\" value=\"val\" />\n" +
            "  </device>\n" +
            "  <target_preparer class=\"com.targetprep.class\">\n" +
            "    <option name=\"opName2\" value=\"val2\" />\n" +
            "  </target_preparer>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        String expectedException =
                "You seem to want a multi-devices configuration but you have "
                        + "[target_preparer] tags outside the <device> tags";
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch(ConfigurationException expected) {
            assertEquals(expectedException, expected.getMessage());
        }
    }

    /**
     * Test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream, Map)}.
     * with a test tag inside a device where it should not be.
     */
    @Test
    public void testParse_withDeviceTag() {
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"device1\">\n" +
            "    <option name=\"deviceOp\" value=\"val2\" />\n" +
            "    <test class=\"junit.framework.TestCase\">\n" +
            "        <option name=\"opName\" value=\"val\" />\n" +
            "    </test>\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
        } catch(ConfigurationException expected) {
            return;
        }
        fail("An exception should have been thrown.");
    }

    /**
     * Test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream, Map)}.
     * with an invalid name
     */
    @Test
    public void testParse_withDeviceInvalidName() {
        String expectedException = "device name cannot contain reserved character: ':'";
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"device:1\">\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch(ConfigurationException expected) {
            assertEquals(expectedException, expected.getMessage());
        }
    }

    /**
     * Test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream, Map)}.
     * with a reserved name.
     */
    @Test
    public void testParse_withDeviceReservedName() {
        String expectedException = "device name cannot be reserved name: '" +
                ConfigurationDef.DEFAULT_DEVICE_NAME + "'";
        final String normalConfig =
            "<configuration description=\"desc\" >\n" +
            "  <device name=\"" + ConfigurationDef.DEFAULT_DEVICE_NAME + "\">\n" +
            "  </device>\n" +
            "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch(ConfigurationException expected) {
            assertEquals(expectedException, expected.getMessage());
        }
    }

    /**
     * Test for {@link ConfigurationXmlParser#parse(ConfigurationDef, String, InputStream, Map)}.
     * with a include that has extra attributes.
     */
    @Test
    public void testParse_includeWithExtraAttributes() {
        String expectedException =
                "Failed to parse config xml 'config'. Reason: <include> tag only expect a 'name' "
                        + "attribute.";
        final String normalConfig =
                "<configuration description=\"desc\" >\n"
                        + "  <include name=\"default\" other=\"test\">\n"
                        + "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch (ConfigurationException expected) {
            assertEquals(expectedException, expected.getMessage());
        }
    }

    /**
     * Test that if an object is left at the root of the config but with one real and one fake
     * device, we do not reject the xml. Object will be associated later to the real device.
     */
    @Test
    public void testParse_multiDevice_fakeMulti() throws Exception {
        final String normalConfig =
                "<configuration description=\"desc\" >\n"
                        + "  <device name=\"device1\">\n"
                        + "    <option name=\"opName\" value=\"val\" />\n"
                        + "  </device>\n"
                        + "  <device name=\"device2\" isFake=\"true\">\n"
                        + "    <option name=\"opName3\" value=\"val3\" />\n"
                        + "  </device>\n"
                        + "  <target_preparer class=\"com.targetprep.class\">\n"
                        + "    <option name=\"opName2\" value=\"val2\" />\n"
                        + "  </target_preparer>\n"
                        + "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);

        xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
        assertEquals(2, configDef.getObjectClassMap().get(Configuration.DEVICE_NAME).size());
        assertTrue("{device1}opName".equals(configDef.getOptionList().get(0).name));
        assertEquals("{device2}opName3", configDef.getOptionList().get(1).name);
    }

    /**
     * Test that if we have only fake devices, an object cannot be left at the root because we only
     * consider it properly formatted if at least one real device exists.
     */
    @Test
    public void testParse_multiDevice_fakeMulti_noReal() throws Exception {
        final String normalConfig =
                "<configuration description=\"desc\" >\n"
                        + "  <device name=\"device1\" isFake=\"true\">\n"
                        + "    <option name=\"opName\" value=\"val\" />\n"
                        + "  </device>\n"
                        + "  <device name=\"device2\" isFake=\"true\">\n"
                        + "    <option name=\"opName3\" value=\"val3\" />\n"
                        + "  </device>\n"
                        + "  <target_preparer class=\"com.targetprep.class\">\n"
                        + "    <option name=\"opName2\" value=\"val2\" />\n"
                        + "  </target_preparer>\n"
                        + "</configuration>";
        final String configName = "config";
        ConfigurationDef configDef = new ConfigurationDef(configName);
        try {
            xmlParser.parse(configDef, configName, getStringAsStream(normalConfig), null);
            fail("An exception should have been thrown.");
        } catch (ConfigurationException expected) {
            assertEquals(
                    "You seem to want a multi-devices configuration but you have [target_preparer] "
                            + "tags outside the <device> tags",
                    expected.getMessage());
        }
    }
}
