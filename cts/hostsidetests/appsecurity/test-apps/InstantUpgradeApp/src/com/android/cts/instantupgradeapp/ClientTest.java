/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.cts.instantupgradeapp;

import static org.hamcrest.CoreMatchers.is;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertFalse;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.ResolveInfo;
import android.os.Bundle;
import android.preference.PreferenceManager;

import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.util.Log;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.util.List;

@RunWith(AndroidJUnit4.class)
public class ClientTest {
    private static final String TAG = "InstantUpgradeApp";

    @Test
    public void testInstantApplicationWritePreferences() throws Exception {
        // only done in the context of an instant application
        assertTrue(InstrumentationRegistry.getContext().getPackageManager().isInstantApp());
        final SharedPreferences prefWriter =
                InstrumentationRegistry.getContext().getSharedPreferences("preferences", 0);
        prefWriter.edit().putString("test", "value").commit();
    }
    @Test
    public void testFullApplicationReadPreferences() throws Exception {
        // only done in the context of an full application
        assertFalse(InstrumentationRegistry.getContext().getPackageManager().isInstantApp());
        final SharedPreferences prefReader =
                InstrumentationRegistry.getContext().getSharedPreferences("preferences", 0);
        assertThat(prefReader.getString("test", "default"), is("value"));
    }
    @Test
    public void testInstantApplicationWriteFile() throws Exception {
        // only done in the context of an instant application
        assertTrue(InstrumentationRegistry.getContext().getPackageManager().isInstantApp());
        try (FileOutputStream fos = InstrumentationRegistry
                .getContext().openFileOutput("test.txt", Context.MODE_PRIVATE)) {
            fos.write("MyTest".getBytes());
        }
    }
    @Test
    public void testFullApplicationReadFile() throws Exception {
        // only done in the context of an full application
        assertFalse(InstrumentationRegistry.getContext().getPackageManager().isInstantApp());
        try (FileInputStream fis = InstrumentationRegistry
                .getContext().openFileInput("test.txt")) {
            final StringBuffer buffer = new StringBuffer();
            final byte[] b = new byte[1024];
            int count;
            while ((count = fis.read(b)) != -1) {
                buffer.append(new String(b, 0, count));
            }
            assertThat(buffer.toString(), is("MyTest"));
        }
    }
}
