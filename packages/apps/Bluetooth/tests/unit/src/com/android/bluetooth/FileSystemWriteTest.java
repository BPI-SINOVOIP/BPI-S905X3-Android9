/*
 * Copyright 2017 The Android Open Source Project
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
package com.android.bluetooth;

import android.support.test.filters.MediumTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.File;
import java.io.IOException;

/**
 * Test Bluetooth's ability to write to the different directories that it
 * is supposed to own
 */
@MediumTest
@RunWith(AndroidJUnit4.class)
public class FileSystemWriteTest {
    @Test
    public void testBluetoothDirWrite() {
        try {
            File file = new File("/data/misc/bluetooth/test.file");
            Assert.assertTrue("File not created", file.createNewFile());
            file.delete();
        } catch (IOException e) {
            Assert.fail("Exception creating file /data/misc/bluetooth/test.file: " + e);
        }
    }

    @Test
    public void testBluedroidDirWrite() {
        try {
            File file = new File("/data/misc/bluedroid/test.file");
            Assert.assertTrue("File not created", file.createNewFile());
            file.delete();
        } catch (IOException e) {
            Assert.fail("Exception creating file /data/misc/bluedroid/test.file: " + e);
        }
    }

    @Test
    public void testBluetoothLogsDirWrite() {
        try {
            File file = new File("/data/misc/bluetooth/logs/test.file");
            Assert.assertTrue("File not created", file.createNewFile());
            file.delete();
        } catch (IOException e) {
            Assert.fail("Exception creating file /data/misc/bluetooth/logs/test.file: " + e);
        }
    }
}
