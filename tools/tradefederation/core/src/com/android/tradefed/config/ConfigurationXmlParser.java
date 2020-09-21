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

import com.android.tradefed.log.LogUtil.CLog;

import org.xml.sax.Attributes;
import org.xml.sax.InputSource;
import org.xml.sax.SAXException;
import org.xml.sax.helpers.DefaultHandler;

import java.io.IOException;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;

import javax.xml.parsers.ParserConfigurationException;
import javax.xml.parsers.SAXParser;
import javax.xml.parsers.SAXParserFactory;

/**
 * Parses a configuration.xml file.
 * <p/>
 * See TODO for expected format
 */
class ConfigurationXmlParser {
    /**
     * SAX callback object. Handles parsing data from the xml tags.
     */
    static class ConfigHandler extends DefaultHandler {

        private static final String OBJECT_TAG = "object";
        private static final String OPTION_TAG = "option";
        private static final String INCLUDE_TAG = "include";
        private static final String TEMPLATE_INCLUDE_TAG = "template-include";
        private static final String CONFIG_TAG = "configuration";
        private static final String DEVICE_TAG = "device";
        private static final String IS_FAKE_ATTR = "isFake";

        /** Note that this simply hasn't been implemented; it is not intentionally forbidden. */
        static final String INNER_TEMPLATE_INCLUDE_ERROR =
                "Configurations which contain a <template-include> tag, not having a 'default' " +
                "attribute, may not be the target of any <include> or <template-include> tag. " +
                "However, configuration '%s' attempted to include configuration '%s', which " +
                "contains a <template-include> tag without a 'default' attribute.";

        // Settings
        private final IConfigDefLoader mConfigDefLoader;
        private final ConfigurationDef mConfigDef;
        private final Map<String, String> mTemplateMap;
        private final String mName;
        private final boolean mInsideParentDeviceTag;

        // State-holding members
        private String mCurrentConfigObject;
        private String mCurrentDeviceObject;
        private List<String> mListDevice = new ArrayList<String>();
        private List<String> mOutsideTag = new ArrayList<String>();

        private Boolean isLocalConfig = null;

        ConfigHandler(
                ConfigurationDef def,
                String name,
                IConfigDefLoader loader,
                String parentDeviceObject,
                Map<String, String> templateMap) {
            mName = name;
            mConfigDef = def;
            mConfigDefLoader = loader;
            mCurrentDeviceObject = parentDeviceObject;
            mInsideParentDeviceTag = (parentDeviceObject != null) ? true : false;

            if (templateMap == null) {
                mTemplateMap = Collections.<String, String>emptyMap();
            } else {
                mTemplateMap = templateMap;
            }
        }

        @Override
        public void startElement(String uri, String localName, String name, Attributes attributes)
                throws SAXException {
            if (OBJECT_TAG.equals(localName)) {
                final String objectTypeName = attributes.getValue("type");
                if (objectTypeName == null) {
                    throw new SAXException(new ConfigurationException(
                            "<object> must have a 'type' attribute"));
                }
                if (GlobalConfiguration.isBuiltInObjType(objectTypeName) ||
                        Configuration.isBuiltInObjType(objectTypeName)) {
                    throw new SAXException(new ConfigurationException(String.format("<object> "
                            + "cannot be type '%s' this is a reserved type.", objectTypeName)));
                }
                addObject(objectTypeName, attributes);
            } else if (DEVICE_TAG.equals(localName)) {
                if (mCurrentDeviceObject != null) {
                    throw new SAXException(new ConfigurationException(
                            "<device> tag cannot be included inside another device"));
                }
                // tag is a device tag (new format) for multi device definition.
                String deviceName = attributes.getValue("name");
                if (deviceName == null) {
                    throw new SAXException(
                            new ConfigurationException("device tag requires a name value"));
                }
                if (deviceName.equals(ConfigurationDef.DEFAULT_DEVICE_NAME)) {
                    throw new SAXException(new ConfigurationException(String.format("device name "
                            + "cannot be reserved name: '%s'",
                            ConfigurationDef.DEFAULT_DEVICE_NAME)));
                }
                if (deviceName.contains(String.valueOf(OptionSetter.NAMESPACE_SEPARATOR))) {
                    throw new SAXException(new ConfigurationException(String.format("device name "
                            + "cannot contain reserved character: '%s'",
                            OptionSetter.NAMESPACE_SEPARATOR)));
                }
                mConfigDef.setMultiDeviceMode(true);
                mCurrentDeviceObject = deviceName;
                addObject(localName, attributes);
            } else if (Configuration.isBuiltInObjType(localName)) {
                // tag is a built in local config object
                if (isLocalConfig == null) {
                    isLocalConfig = true;
                } else if (!isLocalConfig) {
                    throwException(String.format(
                            "Attempted to specify local object '%s' for global config!",
                            localName));
                }

                if (mCurrentDeviceObject == null &&
                        Configuration.doesBuiltInObjSupportMultiDevice(localName)) {
                    // Keep track of all the BuildInObj outside of device tag for final check
                    // if it turns out we are in multi mode, we will throw an exception.
                    mOutsideTag.add(localName);
                }
                // if we are inside a device object, some tags are not allowed.
                if (mCurrentDeviceObject != null) {
                    if (!Configuration.doesBuiltInObjSupportMultiDevice(localName)) {
                        // Prevent some tags to be inside of a device in multi device mode.
                        throw new SAXException(new ConfigurationException(
                                String.format("Tag %s should not be included in a <device> tag.",
                                        localName)));
                    }
                }
                addObject(localName, attributes);
            } else if (GlobalConfiguration.isBuiltInObjType(localName)) {
                // tag is a built in global config object
                if (isLocalConfig == null) {
                    // FIXME: config type should be explicit rather than inferred
                    isLocalConfig = false;
                } else if (isLocalConfig) {
                    throwException(String.format(
                            "Attempted to specify global object '%s' for local config!",
                            localName));
                }
                addObject(localName, attributes);
            } else if (OPTION_TAG.equals(localName)) {
                String optionName = attributes.getValue("name");
                if (optionName == null) {
                    throwException("Missing 'name' attribute for option");
                }

                String optionKey = attributes.getValue("key");
                // Key is optional at this stage.  If it's actually required, another stage in the
                // configuration validation will throw an exception.

                String optionValue = attributes.getValue("value");
                if (optionValue == null) {
                    throwException("Missing 'value' attribute for option '" + optionName + "'");
                }
                if (mCurrentConfigObject != null) {
                    // option is declared within a config object - namespace it with object class
                    // name
                    optionName = String.format("%s%c%s", mCurrentConfigObject,
                            OptionSetter.NAMESPACE_SEPARATOR, optionName);
                }
                if (mCurrentDeviceObject != null) {
                    // preprend the device name in extra if inside a device config object.
                    optionName = String.format("{%s}%s", mCurrentDeviceObject, optionName);
                }
                mConfigDef.addOptionDef(optionName, optionKey, optionValue, mName);
            } else if (CONFIG_TAG.equals(localName)) {
                String description = attributes.getValue("description");
                if (description != null) {
                    // Ensure that we only set the description the first time and not when it is
                    // loading the <include> configuration.
                    if (mConfigDef.getDescription() == null ||
                            mConfigDef.getDescription().isEmpty()) {
                        mConfigDef.setDescription(description);
                    }
                }
            } else if (INCLUDE_TAG.equals(localName)) {
                String includeName = attributes.getValue("name");
                if (includeName == null) {
                    throwException("Missing 'name' attribute for include");
                }
                if (attributes.getLength() > 1) {
                    throwException("<include> tag only expect a 'name' attribute.");
                }
                try {
                    mConfigDefLoader.loadIncludedConfiguration(
                            mConfigDef, mName, includeName, mCurrentDeviceObject, mTemplateMap);
                } catch (ConfigurationException e) {
                    if (e instanceof TemplateResolutionError) {
                        throwException(String.format(INNER_TEMPLATE_INCLUDE_ERROR,
                                mConfigDef.getName(), includeName));
                    }
                    throw new SAXException(e);
                }
            } else if (TEMPLATE_INCLUDE_TAG.equals(localName)) {
                final String templateName = attributes.getValue("name");
                if (templateName == null) {
                    throwException("Missing 'name' attribute for template-include");
                }
                if (mCurrentDeviceObject != null) {
                    // TODO: Add this use case.
                    throwException("<template> inside device object currently not supported.");
                }

                String includeName = mTemplateMap.get(templateName);
                if (includeName == null) {
                    includeName = attributes.getValue("default");
                }
                if (includeName == null) {
                    throwTemplateException(mConfigDef.getName(), templateName);
                }
                // Removing the used template from the map to avoid re-using it.
                mTemplateMap.remove(templateName);
                try {
                    mConfigDefLoader.loadIncludedConfiguration(
                            mConfigDef, mName, includeName, null, mTemplateMap);
                } catch (ConfigurationException e) {
                    if (e instanceof TemplateResolutionError) {
                        throwException(String.format(INNER_TEMPLATE_INCLUDE_ERROR,
                                mConfigDef.getName(), includeName));
                    }
                    throw new SAXException(e);
                }
            } else {
                throw new SAXException(String.format(
                        "Unrecognized tag '%s' in configuration", localName));
            }
        }

        @Override
        public void endElement (String uri, String localName, String qName) throws SAXException {
            if (OBJECT_TAG.equals(localName) || Configuration.isBuiltInObjType(localName)
                    || GlobalConfiguration.isBuiltInObjType(localName)) {
                mCurrentConfigObject = null;
            }
            if (DEVICE_TAG.equals(localName) && !mInsideParentDeviceTag) {
                // Only unset if it was not the parent device tag.
                mCurrentDeviceObject = null;
            }
        }

        void addObject(String objectTypeName, Attributes attributes) throws SAXException {
            if (Configuration.DEVICE_NAME.equals(objectTypeName)) {
                String isFakeString = attributes.getValue(IS_FAKE_ATTR);
                boolean isFake = false;
                if (isFakeString != null && Boolean.parseBoolean(isFakeString) == true) {
                    isFake = true;
                }
                // We still want to add a standalone device without any inner object.
                String deviceName = attributes.getValue("name");
                if (!mListDevice.contains(deviceName)) {
                    mListDevice.add(deviceName);
                    mConfigDef.addConfigObjectDef(objectTypeName,
                            DeviceConfigurationHolder.class.getCanonicalName());
                }
                String resp = mConfigDef.addExpectedDevice(deviceName, isFake);
                if (resp != null) {
                    throwException(resp);
                }
            } else {
                String className = attributes.getValue("class");
                if (className == null) {
                    throwException(String.format("Missing class attribute for object %s",
                            objectTypeName));
                }
                if (mCurrentDeviceObject != null) {
                    // Add the device name as a namespace to the type
                    objectTypeName = mCurrentDeviceObject + OptionSetter.NAMESPACE_SEPARATOR
                            + objectTypeName;
                }
                int classCount = mConfigDef.addConfigObjectDef(objectTypeName, className);
                mCurrentConfigObject = String.format("%s%c%d", className,
                        OptionSetter.NAMESPACE_SEPARATOR, classCount);
            }
        }

        private void throwException(String reason) throws SAXException {
            throw new SAXException(new ConfigurationException(String.format(
                    "Failed to parse config xml '%s'. Reason: %s", mConfigDef.getName(), reason)));
        }

        private void throwTemplateException(String configName, String templateName)
                throws SAXException {
            throw new SAXException(new TemplateResolutionError(configName, templateName));
        }
    }

    private final IConfigDefLoader mConfigDefLoader;
    /**
     * If we are loading a config from inside a <device> tag, this will contain the name of the
     * current device tag to properly load in context.
     */
    private final String mParentDeviceObject;

    ConfigurationXmlParser(IConfigDefLoader loader, String parentDeviceObject) {
        mConfigDefLoader = loader;
        mParentDeviceObject = parentDeviceObject;
    }

    /**
     * Parses out configuration data contained in given input into the given configdef.
     * <p/>
     * Currently performs limited error checking.
     *
     * @param configDef the {@link ConfigurationDef} to load data into
     * @param name the name of the configuration currently being loaded. Used for logging only.
     * Can be different than configDef.getName in cases of included configs
     * @param xmlInput the configuration xml to parse
     * @throws ConfigurationException if input could not be parsed or had invalid format
     */
    void parse(ConfigurationDef configDef, String name, InputStream xmlInput,
            Map<String, String> templateMap) throws ConfigurationException {
        try {
            SAXParserFactory parserFactory = SAXParserFactory.newInstance();
            parserFactory.setNamespaceAware(true);
            SAXParser parser = parserFactory.newSAXParser();
            ConfigHandler configHandler =
                    new ConfigHandler(
                            configDef, name, mConfigDefLoader, mParentDeviceObject, templateMap);
            parser.parse(new InputSource(xmlInput), configHandler);
            // ConfigurationDef holds whether or not the configs are multi-device or not.
            checkValidMultiConfiguration(configHandler, configDef);
        } catch (ParserConfigurationException e) {
            throwConfigException(name, e);
        } catch (SAXException e) {
            throwConfigException(name, e);
        } catch (IOException e) {
            throwConfigException(name, e);
        }
    }

    /**
     * Helper to encapsulate exceptions in a {@link ConfigurationException}
     */
    private void throwConfigException(String configName, Throwable e)
            throws ConfigurationException {
        if (e.getCause() instanceof ConfigurationException) {
            throw (ConfigurationException)e.getCause();
        }
        throw new ConfigurationException(String.format("Failed to parse config xml '%s' due to "
                + "'%s'", configName, e), e);
    }

    /**
     * Validate that the configuration is valid from a multi device configuration standpoint: Some
     * tags are not allowed outside the <device> tags.
     */
    private void checkValidMultiConfiguration(
            ConfigHandler configHandler, ConfigurationDef configDef) throws SAXException {
        Map<String, Boolean> expected = configDef.getExpectedDevices();
        Long numDut =
                expected.values()
                        .stream()
                        .filter(value -> (value == false))
                        .collect(Collectors.counting());
        Long numNonDut =
                expected.values()
                        .stream()
                        .filter(value -> (value == true))
                        .collect(Collectors.counting());
        if (numNonDut > 0 && numDut == 1) {
            // If we only have one DUT device and the rest are non-DUT devices. We need to consider
            // this has an hybrid use case since there is technically only one device. So we cannot
            // validate yet if objects are allowed to be outside <device> tags, it will be validated
            // later during the parsing when we have more information.
            CLog.d("Only one device under tests. Using hybrid handling.");
            return;
        }

        if (configDef.isMultiDeviceMode() && !configHandler.mOutsideTag.isEmpty()) {
            throw new SAXException(
                    new ConfigurationException(
                            String.format(
                                    "You seem to want a multi-devices configuration but you have "
                                            + "%s tags outside the <device> tags",
                                    configHandler.mOutsideTag)));
        }
    }
}
