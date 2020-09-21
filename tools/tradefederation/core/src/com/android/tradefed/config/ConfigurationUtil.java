/*
 * Copyright (C) 2017 The Android Open Source Project
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
import com.android.tradefed.util.FileUtil;
import com.android.tradefed.util.MultiMap;

import org.kxml2.io.KXmlSerializer;

import java.io.File;
import java.io.IOException;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.util.ArrayList;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;

/** Utility functions to handle configuration files. */
public class ConfigurationUtil {

    // Element names used for emitting the configuration XML.
    public static final String CONFIGURATION_NAME = "configuration";
    public static final String OPTION_NAME = "option";
    public static final String CLASS_NAME = "class";
    public static final String NAME_NAME = "name";
    public static final String KEY_NAME = "key";
    public static final String VALUE_NAME = "value";

    /**
     * Create a serializer to be used to create a new configuration file.
     *
     * @param outputXml the XML file to write to
     * @return a {@link KXmlSerializer}
     */
    static KXmlSerializer createSerializer(File outputXml) throws IOException {
        PrintWriter output = new PrintWriter(outputXml);
        KXmlSerializer serializer = new KXmlSerializer();
        serializer.setOutput(output);
        serializer.setFeature("http://xmlpull.org/v1/doc/features.html#indent-output", true);
        serializer.startDocument("UTF-8", null);
        return serializer;
    }

    /**
     * Add a class to the configuration XML dump.
     *
     * @param serializer a {@link KXmlSerializer} to create the XML dump
     * @param classTypeName a {@link String} of the class type's name
     * @param obj {@link Object} to be added to the XML dump
     * @param excludeClassFilter list of object configuration type or fully qualified class names to
     *     be excluded from the dump. for example: {@link Configuration#TARGET_PREPARER_TYPE_NAME}.
     *     com.android.tradefed.testtype.StubTest
     */
    static void dumpClassToXml(
            KXmlSerializer serializer,
            String classTypeName,
            Object obj,
            List<String> excludeClassFilter)
            throws IOException {
        if (excludeClassFilter.contains(classTypeName)) {
            return;
        }
        if (excludeClassFilter.contains(obj.getClass().getName())) {
            return;
        }
        serializer.startTag(null, classTypeName);
        serializer.attribute(null, CLASS_NAME, obj.getClass().getName());
        dumpOptionsToXml(serializer, obj);
        serializer.endTag(null, classTypeName);
    }

    /**
     * Add all the options of class to the command XML dump.
     *
     * @param serializer a {@link KXmlSerializer} to create the XML dump
     * @param obj {@link Object} to be added to the XML dump
     */
    @SuppressWarnings({"rawtypes", "unchecked"})
    private static void dumpOptionsToXml(KXmlSerializer serializer, Object obj) throws IOException {
        for (Field field : OptionSetter.getOptionFieldsForClass(obj.getClass())) {
            Option option = field.getAnnotation(Option.class);
            Object fieldVal = OptionSetter.getFieldValue(field, obj);
            if (fieldVal == null) {
                continue;
            } else if (fieldVal instanceof Collection) {
                for (Object entry : (Collection) fieldVal) {
                    dumpOptionToXml(serializer, option.name(), null, entry.toString());
                }
            } else if (fieldVal instanceof Map) {
                Map map = (Map) fieldVal;
                for (Object entryObj : map.entrySet()) {
                    Map.Entry entry = (Entry) entryObj;
                    dumpOptionToXml(
                            serializer,
                            option.name(),
                            entry.getKey().toString(),
                            entry.getValue().toString());
                }
            } else if (fieldVal instanceof MultiMap) {
                MultiMap multimap = (MultiMap) fieldVal;
                for (Object keyObj : multimap.keySet()) {
                    for (Object valueObj : multimap.get(keyObj)) {
                        dumpOptionToXml(
                                serializer, option.name(), keyObj.toString(), valueObj.toString());
                    }
                }
            } else {
                dumpOptionToXml(serializer, option.name(), null, fieldVal.toString());
            }
        }
    }

    /**
     * Add a single option to the command XML dump.
     *
     * @param serializer a {@link KXmlSerializer} to create the XML dump
     * @param name a {@link String} of the option's name
     * @param key a {@link String} of the option's key, used as name if param name is null
     * @param value a {@link String} of the option's value
     */
    private static void dumpOptionToXml(
            KXmlSerializer serializer, String name, String key, String value) throws IOException {
        serializer.startTag(null, OPTION_NAME);
        serializer.attribute(null, NAME_NAME, name);
        if (key != null) {
            serializer.attribute(null, KEY_NAME, key);
        }
        serializer.attribute(null, VALUE_NAME, value);
        serializer.endTag(null, OPTION_NAME);
    }

    /**
     * Helper to get the test config files from given directories.
     *
     * @param subPath where to look for configuration. Can be null.
     * @param dirs a list of {@link File} of extra directories to search for test configs
     */
    public static Set<String> getConfigNamesFromDirs(String subPath, List<File> dirs) {
        Set<File> res = getConfigNamesFileFromDirs(subPath, dirs);
        if (res.isEmpty()) {
            return new HashSet<>();
        }
        Set<String> files = new HashSet<>();
        res.forEach(file -> files.add(file.getAbsolutePath()));
        return files;
    }

    /**
     * Helper to get the test config files from given directories.
     *
     * @param subPath The location where to look for configuration. Can be null.
     * @param dirs A list of {@link File} of extra directories to search for test configs
     * @return the set of {@link File} that were found.
     */
    public static Set<File> getConfigNamesFileFromDirs(String subPath, List<File> dirs) {
        List<String> patterns = new ArrayList<>();
        patterns.add(".*.config");
        patterns.add(".*.xml");
        return getConfigNamesFileFromDirs(subPath, dirs, patterns);
    }

    /**
     * Search a particular pattern of in the given directories.
     *
     * @param subPath The location where to look for configuration. Can be null.
     * @param dirs A list of {@link File} of extra directories to search for test configs
     * @param configNamePatterns the list of patterns for files to be found.
     * @return the set of {@link File} that were found.
     */
    public static Set<File> getConfigNamesFileFromDirs(
            String subPath, List<File> dirs, List<String> configNamePatterns) {
        Set<File> configNames = new HashSet<>();
        for (File dir : dirs) {
            if (subPath != null) {
                dir = new File(dir, subPath);
            }
            if (!dir.isDirectory()) {
                CLog.d("%s doesn't exist or is not a directory.", dir.getAbsolutePath());
                continue;
            }
            try {
                for (String configNamePattern : configNamePatterns) {
                    configNames.addAll(FileUtil.findFilesObject(dir, configNamePattern));
                }
            } catch (IOException e) {
                CLog.w("Failed to get test config files from directory %s", dir.getAbsolutePath());
            }
        }
        return dedupFiles(configNames);
    }

    /**
     * From a same tests dir we only expect a single instance of each names, so we dedup the files
     * if that happens.
     */
    private static Set<File> dedupFiles(Set<File> origSet) {
        Set<String> tracker = new HashSet<>();
        Set<File> newSet = new HashSet<>();
        for (File f : origSet) {
            if (!tracker.contains(f.getName())) {
                tracker.add(f.getName());
                newSet.add(f);
            }
        }
        return newSet;
    }
}
