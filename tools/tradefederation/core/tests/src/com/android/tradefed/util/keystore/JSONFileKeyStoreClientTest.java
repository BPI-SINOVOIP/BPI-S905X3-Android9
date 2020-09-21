/*
 * Copyright (C) 2016 The Android Open Source Project
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

import com.android.tradefed.util.FileUtil;

import junit.framework.TestCase;

import org.json.JSONObject;

import java.io.File;

/**
 * Unit tests for JSON File Key Store Client test.
 */
public class JSONFileKeyStoreClientTest extends TestCase {
    final String mJsonData = new String("{\"key1\":\"value 1\",\"key2 \":\"foo\"}");
    JSONFileKeyStoreClient mKeyStore = null;

    @Override
    protected void setUp() throws Exception {
        mKeyStore = new JSONFileKeyStoreClient();
    }

    public void testKeyStoreNullFile() throws Exception {
        try {
            new JSONFileKeyStoreClient(null);
            fail("Key store should not be available for null file");
        } catch (KeyStoreException e) {
            // Expected.
        }
    }

    public void testKeyStoreFetchUnreadableFile() throws Exception {
        File test = FileUtil.createTempFile("keystore", "test");
        test.setReadable(false);
        try {
            new JSONFileKeyStoreClient(test);
            fail("Should have thrown an exception");
        } catch(KeyStoreException expected) {
            assertEquals(String.format("Unable to read the JSON key store file %s", test),
                    expected.getMessage());
        } finally {
            FileUtil.deleteFile(test);
        }
    }

    public void testKeyStoreFetchEmptyFile() throws Exception {
        File test = FileUtil.createTempFile("keystore", "test");
        try {
            new JSONFileKeyStoreClient(test);
            fail("Should have thrown an exception");
        } catch(KeyStoreException expected) {
            // expected
        } finally {
            FileUtil.deleteFile(test);
        }
    }

    public void testKeyStoreFetchFile() throws Exception {
        File test = FileUtil.createTempFile("keystore", "test");
        try {
            FileUtil.writeToFile(mJsonData, test);
            JSONFileKeyStoreClient keystore = new JSONFileKeyStoreClient(test);
            assertTrue(keystore.isAvailable());
            assertEquals("value 1", keystore.fetchKey("key1"));
        } finally {
            FileUtil.deleteFile(test);
        }
    }

    public void testContainsKeyinNullKeyStore() throws Exception {
        mKeyStore.setKeyStore(null);
        assertFalse("Key should not exist in null key store", mKeyStore.containsKey("test"));
    }

    public void testDoesNotContainMissingKey() throws Exception {
        JSONObject data = new JSONObject(mJsonData);
        mKeyStore.setKeyStore(data);
        assertFalse("Missing key should not exist in key store",
                mKeyStore.containsKey("invalid key"));
    }

    public void testContainsValidKey() throws Exception {
        JSONObject data = new JSONObject(mJsonData);
        mKeyStore.setKeyStore(data);
        assertTrue("Failed to fetch valid key in key store", mKeyStore.containsKey("key1"));
    }

    public void testFetchMissingKey() throws Exception {
        JSONObject data = new JSONObject(mJsonData);
        mKeyStore.setKeyStore(data);
        assertNull("Missing key should not exist in key store",
                mKeyStore.fetchKey("invalid key"));
    }

    public void testFetchNullKey() throws Exception {
        JSONObject data = new JSONObject(mJsonData);
        mKeyStore.setKeyStore(data);
        assertNull("Null key should not exist in key store",
                mKeyStore.fetchKey(null));
    }

    public void testFetchValidKey() throws Exception {
        JSONObject data = new JSONObject(mJsonData);
        mKeyStore.setKeyStore(data);
        assertEquals("value 1", mKeyStore.fetchKey("key1"));
    }
}
