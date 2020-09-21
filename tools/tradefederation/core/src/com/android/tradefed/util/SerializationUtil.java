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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;

/** Utility to serialize/deserialize an object that implements {@link Serializable}. */
public class SerializationUtil {

    /**
     * Serialize a object that implements {@link Serializable}.
     *
     * @param o the object to serialize.
     * @return the {@link File} where the object was serialized.
     * @throws IOException if serialization fails.
     */
    public static File serialize(Serializable o) throws IOException {
        File tmpSerialized = FileUtil.createTempFile("serial-util", ".ser");
        FileOutputStream fileOut = null;
        ObjectOutputStream out = null;
        try {
            fileOut = new FileOutputStream(tmpSerialized);
            out = new ObjectOutputStream(fileOut);
            out.writeObject(o);
        } catch (IOException e) {
            // catch in order to clean up the tmp file.
            FileUtil.deleteFile(tmpSerialized);
            throw e;
        } finally {
            StreamUtil.close(out);
            StreamUtil.close(fileOut);
        }
        return tmpSerialized;
    }

    /**
     * Deserialize an object that was serialized using {@link #serialize(Serializable)}.
     *
     * @param serializedFile the file where the object was serialized.
     * @param deleteFile true if the serialized file should be deleted once deserialized.
     * @return the Object deserialized.
     * @throws IOException if the deserialization fails.
     */
    public static Object deserialize(File serializedFile, boolean deleteFile) throws IOException {
        FileInputStream fileIn = null;
        ObjectInputStream in = null;
        try {
            fileIn = new FileInputStream(serializedFile);
            in = new ObjectInputStream(fileIn);
            return in.readObject();
        } catch (ClassNotFoundException cnfe) {
            throw new RuntimeException(cnfe);
        } finally {
            StreamUtil.close(in);
            StreamUtil.close(fileIn);
            if (deleteFile) {
                FileUtil.deleteFile(serializedFile);
            }
        }
    }
}
