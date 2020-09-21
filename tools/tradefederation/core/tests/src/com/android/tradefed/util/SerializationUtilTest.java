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
package com.android.tradefed.util;

import static org.junit.Assert.*;

import com.android.tradefed.build.BuildInfo;
import com.android.tradefed.build.BuildSerializedVersion;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.IOException;
import java.io.InputStream;
import java.io.NotSerializableException;
import java.io.Serializable;
import java.io.StreamCorruptedException;

/** Unit tests for {@link SerializationUtil}. */
@RunWith(JUnit4.class)
public class SerializationUtilTest {

    /**
     * Test class that implements {@link Serializable} but has an attribute that is not serializable
     */
    public static class SerialTestClass implements Serializable {
        private static final long serialVersionUID = BuildSerializedVersion.VERSION;
        public InputStream mStream;

        public SerialTestClass() {
            mStream = new ByteArrayInputStream("test".getBytes());
        }
    }

    /** Tests that serialization and deserialization creates a similar object from the original. */
    @Test
    public void testSerialize_Deserialize() throws Exception {
        BuildInfo b = new BuildInfo();
        File tmpSerialized = SerializationUtil.serialize(b);
        Object o = SerializationUtil.deserialize(tmpSerialized, true);
        assertTrue(o instanceof BuildInfo);
        BuildInfo test = (BuildInfo) o;
        // use the custom build info equals to check similar properties
        assertTrue(b.equals(test));
        assertTrue(!tmpSerialized.exists());
    }

    /**
     * Tests that serialization and deserialization creates a similar object from the original but
     * does not delete the temporary serial file.
     */
    @Test
    public void testSerialize_Deserialize_noDelete() throws Exception {
        BuildInfo b = new BuildInfo();
        File tmpSerialized = SerializationUtil.serialize(b);
        try {
            Object o = SerializationUtil.deserialize(tmpSerialized, false);
            assertTrue(o instanceof BuildInfo);
            BuildInfo test = (BuildInfo) o;
            // use the custom build info equals to check similar properties
            assertTrue(b.equals(test));
            assertTrue(tmpSerialized.exists());
        } finally {
            FileUtil.deleteFile(tmpSerialized);
        }
    }

    /** Test {@link SerializationUtil#deserialize(File, boolean)} for a non existing file. */
    @Test
    public void testDeserialize_noFile() {
        try {
            SerializationUtil.deserialize(new File("doesnotexists"), true);
            fail("Should have thrown an exception.");
        } catch (IOException expected) {
            // expected
        }
    }

    /**
     * Test {@link SerializationUtil#deserialize(File, boolean)} for a corrupted stream, it should
     * throw an exception.
     */
    @Test
    public void testSerialize_Deserialize_corrupted() throws Exception {
        BuildInfo b = new BuildInfo();
        File tmpSerialized = SerializationUtil.serialize(b);
        // add some extra data
        FileUtil.writeToFile("oh I am corrupted", tmpSerialized, false);
        try {
            SerializationUtil.deserialize(tmpSerialized, true);
            fail("Should have thrown an exception.");
        } catch (StreamCorruptedException expected) {
            // expected
        }
        // the file was still deleted as expected.
        assertTrue(!tmpSerialized.exists());
    }

    /**
     * Test {@link SerializationUtil#serialize(Serializable)} on an object that is not serializable
     * because of one of its attribute. Method should throw an exception.
     */
    @Test
    public void testSerialized_notSerializable() throws Exception {
        SerialTestClass test = new SerialTestClass();
        try {
            SerializationUtil.serialize(test);
            fail("Should have thrown an exception.");
        } catch (NotSerializableException expected) {
            // the ByteArrayInputStream attribute is not serializable.
            assertEquals("java.io.ByteArrayInputStream", expected.getMessage());
        }
    }
}
