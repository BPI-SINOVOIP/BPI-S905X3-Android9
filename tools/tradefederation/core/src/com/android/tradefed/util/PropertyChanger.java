/*
 * Copyright (C) 2013 The Android Open Source Project
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
package com.android.tradefed.util;

import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * A utility class for changing (or adding) items in an Android property file
 *
 */
public abstract class PropertyChanger {

    /**
     * A utility function to change (or add) items in an Android property file
     *
     * Properties that are in the original property file will have its value modified. New ones
     * will be appended. Caller is responsible for properly recycling temporary files, both original
     * and the modified.
     *
     * @param original the original property file
     * @param properties the properties to change or add
     * @return path to the new property file
     */
    public static File changeProperties(File original, Map<String, String> properties)
            throws IOException {
        Map<String, String> propsToAdd = new HashMap<String, String>(properties);
        File ret = FileUtil.createTempFile("chg_prop_", ".prop");
        BufferedReader br = null;
        BufferedWriter bw = null;
        try {
            br = new BufferedReader(new FileReader(original));
            bw = new BufferedWriter(new FileWriter(ret));
            String line = null;
            while ((line = br.readLine()) != null) {
                int pos = line.indexOf('=');
                if (pos != -1) {
                    String name = line.substring(0, pos);
                    if (propsToAdd.containsKey(name)) {
                        // modify the line if property needs to be changed
                        line = String.format("%s=%s", name, propsToAdd.get(name));
                        propsToAdd.remove(name);
                    }
                }
                // write line into new property file
                bw.write(line + '\n');
            }
            // write remaining properties (new ones)
            for (Entry<String, String> entry : propsToAdd.entrySet()) {
                bw.write(String.format("%s=%s\n", entry.getKey(), entry.getValue()));
            }
        } finally {
            StreamUtil.close(br);
            StreamUtil.close(bw);
        }
        return ret;
    }
}
