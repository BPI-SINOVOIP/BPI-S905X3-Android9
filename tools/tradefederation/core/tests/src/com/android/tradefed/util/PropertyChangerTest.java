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

import junit.framework.TestCase;

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
 * Functional test for {@link PropertyChanger}
 */
public class PropertyChangerTest extends TestCase {

    private File mTmpInput;
    private File mTmpOutput;
    private Map<String, String> mOriginal, mChanges, mExpected;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTmpInput = FileUtil.createTempFile("prop_test_in", ".prop");
        mOriginal = new HashMap<String, String>();
        mOriginal.put("foo1", "bar1");
        mOriginal.put("foo2", "bar2");
        mOriginal.put("foo3", "bar3");
        mOriginal.put("foo4", "bar4");
        writeProperties(mTmpInput, mOriginal);
        mChanges = new HashMap<String, String>();
        mChanges.put("foo2", "bar2*");
        mChanges.put("foo3", "bar3*");
        mChanges.put("foo5", "bar5-new");
        mExpected = new HashMap<String, String>();
        mExpected.put("foo1", "bar1");
        mExpected.put("foo2", "bar2*");
        mExpected.put("foo3", "bar3*");
        mExpected.put("foo4", "bar4");
        mExpected.put("foo5", "bar5-new");
    }

    public void testChangeProperty() throws Exception {
        mTmpOutput = PropertyChanger.changeProperties(mTmpInput, mChanges);
        verifyProperties(mTmpOutput, mExpected);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mTmpInput != null && mTmpInput.exists()) {
            mTmpInput.delete();
        }
        if (mTmpOutput != null && mTmpOutput.exists()) {
            mTmpOutput.delete();
        }
        super.tearDown();
    }

    private void writeProperties(File output, Map<String, String> props) throws IOException {
        BufferedWriter bw = null;
        try {
        bw = new BufferedWriter(new FileWriter(output));
        for (Entry<String, String> entry : props.entrySet()) {
            bw.write(String.format("%s=%s\n", entry.getKey(), entry.getValue()));
        }
        } finally {
            if (bw != null) {
                bw.close();
            }
        }
    }

    private void verifyProperties(File source, Map<String, String> props) throws IOException {
        BufferedReader br = null;
        Map<String, String> verifyProps = new HashMap<String, String>(props);
        try {
            br = new BufferedReader(new FileReader(source));
            String line = null;
            while ((line = br.readLine()) != null) {
                int pos = line.indexOf('=');
                String name = line.substring(0, pos);
                String value = line.substring(pos + 1);
                assertTrue("extra property in file: " + name, verifyProps.containsKey(name));
                assertEquals("property value mismatch", verifyProps.get(name), value);
                verifyProps.remove(name);
            }
            assertTrue("missing properties in file", verifyProps.isEmpty());
        } finally {
            if (br != null) {
                br.close();
            }
        }
    }
}
