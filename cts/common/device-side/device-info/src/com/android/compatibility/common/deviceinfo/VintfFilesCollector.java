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
package com.android.compatibility.common.deviceinfo;

import android.test.InstrumentationTestCase;
import android.os.Environment;
import android.os.Build;
import android.os.VintfObject;
import com.google.common.collect.ImmutableMap;
import java.io.File;
import java.io.FileWriter;
import java.io.StringReader;
import java.io.Writer;
import java.util.Map;
import java.util.function.Supplier;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlPullParserFactory;

/**
 * Device-side VINTF files collector. Uses {@link android.os.VintfObject} to collect VINTF manifests
 * and compatibility matrices.
 */
public final class VintfFilesCollector extends InstrumentationTestCase {

    private static final String FRAMEWORK_MANIFEST_NAME = "framework_manifest.xml";
    private static final String FRAMEWORK_MATRIX_NAME = "framework_compatibility_matrix.xml";
    private static final String DEVICE_MANIFEST_NAME = "device_manifest.xml";
    private static final String DEVICE_MATRIX_NAME = "device_compatibility_matrix.xml";

    public void testCollectVintfFiles() throws Exception {

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            // No VINTF before O.
            return;
        }

        assertTrue("External storage is not mounted",
                Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED));
        File dir = DeviceInfo.makeResultDir();
        assertNotNull("Cannot create directory for device info files", dir);
        collect(dir);
    }


    // report() doesn't distinguish the four XML Strings, so we have to guess.
    private static void collect(File dir) throws Exception {
        for (String content : VintfObject.report()) {
            String fileName = guessFileName(content);
            if (fileName != null) {
                writeStringToFile(content, new File(dir, fileName));
            }
        }
    }

    private static void writeStringToFile(String content, File file) throws Exception {
        if (content == null || content.isEmpty()) {
            return;
        }
        try (Writer os = new FileWriter(file)) {
            os.write(content);
        }
    }

    // Guess a suitable file name for the given XML string. Return null if
    // it is not an XML string, or no suitable names can be provided.
    private static String guessFileName(String content) throws Exception {
        try {
            XmlPullParser parser = XmlPullParserFactory.newInstance().newPullParser();
            parser.setInput(new StringReader(content));

            for (int eventType = parser.getEventType(); eventType != XmlPullParser.END_DOCUMENT;
                 eventType = parser.next()) {

                if (eventType == XmlPullParser.START_TAG) {
                    String tag = parser.getName();
                    if (parser.getDepth() != 1) {
                        continue; // only parse top level tags
                    }

                    String type = parser.getAttributeValue(null, "type");
                    if ("manifest".equals(tag)) {
                        if ("framework".equals(type)) {
                            return FRAMEWORK_MANIFEST_NAME;
                        }
                        if ("device".equals(type)) {
                            return DEVICE_MANIFEST_NAME;
                        }
                    }
                    if ("compatibility-matrix".equals(tag)) {
                        if ("framework".equals(type)) {
                            return FRAMEWORK_MATRIX_NAME;
                        }
                        if ("device".equals(type)) {
                            return DEVICE_MATRIX_NAME;
                        }
                    }
                }
            }
        } catch (XmlPullParserException ex) {
            return null;
        }
        return null;
    }
}
