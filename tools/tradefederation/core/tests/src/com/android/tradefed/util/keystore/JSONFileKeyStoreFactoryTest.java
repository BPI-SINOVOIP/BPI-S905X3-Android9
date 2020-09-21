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
package com.android.tradefed.util.keystore;

import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertSame;

import com.android.tradefed.config.OptionSetter;
import com.android.tradefed.util.FileUtil;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.File;

/** Unit tests for {@link JSONFileKeyStoreFactory}. */
@RunWith(JUnit4.class)
public class JSONFileKeyStoreFactoryTest {

    private final String mJsonData = new String("{\"key1\":\"value 1\",\"key2 \":\"foo\"}");
    private JSONFileKeyStoreFactory mFactory;
    private File mJsonFile;

    @Before
    public void setUp() throws Exception {
        mFactory = new JSONFileKeyStoreFactory();
        mJsonFile = FileUtil.createTempFile("json-keystore", ".json");
        FileUtil.writeToFile(mJsonData, mJsonFile);
    }

    @After
    public void tearDown() {
        FileUtil.recursiveDelete(mJsonFile);
    }

    /**
     * Test that if the underlying JSON keystore has not changed, the client returned is the same.
     */
    @Test
    public void testLoadKeyStore_same() throws Exception {
        OptionSetter setter = new OptionSetter(mFactory);
        setter.setOptionValue("json-key-store-file", mJsonFile.getAbsolutePath());
        IKeyStoreClient client = mFactory.createKeyStoreClient();
        assertNotNull(client);
        IKeyStoreClient client2 = mFactory.createKeyStoreClient();
        assertSame(client, client2);
    }

    /** Test that if the last modified flag has changed, we reload a new client. */
    @Test
    public void testLoadKeyStore_modified() throws Exception {
        OptionSetter setter = new OptionSetter(mFactory);
        setter.setOptionValue("json-key-store-file", mJsonFile.getAbsolutePath());
        IKeyStoreClient client = mFactory.createKeyStoreClient();
        assertNotNull(client);
        mJsonFile.setLastModified(mJsonFile.lastModified() + 5000l);
        IKeyStoreClient client2 = mFactory.createKeyStoreClient();
        assertNotNull(client2);
        assertNotSame(client, client2);
    }

    /**
     * Test that if the underlying file is not accessible anymore for any reason, we rely on the
     * cache.
     */
    @Test
    public void testLoadKeyStore_null() throws Exception {
        OptionSetter setter = new OptionSetter(mFactory);
        setter.setOptionValue("json-key-store-file", mJsonFile.getAbsolutePath());
        IKeyStoreClient client = mFactory.createKeyStoreClient();
        assertNotNull(client);
        FileUtil.deleteFile(mJsonFile);
        IKeyStoreClient client2 = mFactory.createKeyStoreClient();
        assertNotNull(client2);
        assertSame(client, client2);
    }
}
