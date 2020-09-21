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

package com.android.bluetooth.opp;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.doReturn;

import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;

import com.android.bluetooth.TestConfig;

import java.io.FileInputStream;
import java.io.IOException;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.robolectric.RobolectricTestRunner;
import org.robolectric.annotation.Config;

@RunWith(RobolectricTestRunner.class)
@Config(manifest = TestConfig.MANIFEST_PATH, sdk = TestConfig.SDK_VERSION)
public class OppSendFileInfoTest {

    // Constants for test file size.
    private static final int TEST_FILE_SIZE = 10;
    private static final int MAXIMUM_FILE_SIZE = 0xFFFFFFFF;

    @Mock Context mContext;
    @Mock ContentResolver mContentResolver;
    @Mock FileInputStream mFileInputStream;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
    }

    /**
     * Test a BluetoothOppSendFileInfo generated from a local file (MIME type: text/plain,
     * size: #TEST_FILE_SIZE).
     * Check whether the BluetoothOppSendFileInfo matches the input.
     */
    @Test
    public void testFileOpen() throws IOException {
        Uri uri = Uri.parse("file:///android_asset/opp/OppTestFile.txt");
        doReturn(mContentResolver).when(mContext).getContentResolver();
        doReturn(mFileInputStream).when(mContentResolver).openInputStream(uri);
        doReturn(TEST_FILE_SIZE, -1).when(mFileInputStream).read(any(), eq(0), eq(4096));
        BluetoothOppSendFileInfo sendFileInfo = BluetoothOppSendFileInfo.generateFileInfo(
                mContext, uri, "text/plain", false);
        assertThat(sendFileInfo).isNotEqualTo(BluetoothOppSendFileInfo.SEND_FILE_INFO_ERROR);
        assertThat(sendFileInfo.mFileName).isEqualTo("OppTestFile.txt");
        assertThat(sendFileInfo.mMimetype).isEqualTo("text/plain");
        assertThat(sendFileInfo.mLength).isEqualTo(TEST_FILE_SIZE);
        assertThat(sendFileInfo.mInputStream).isEqualTo(mFileInputStream);
        assertThat(sendFileInfo.mStatus).isEqualTo(0);
    }

    /**
     * Test a BluetoothOppSendFileInfo generated from a web page, which is not supported.
     * Should return an error BluetoothOppSendFileInfo.
     */
    @Test
    public void testInvalidUriScheme() {
        Uri webpage = Uri.parse("http://www.android.com/");
        BluetoothOppSendFileInfo sendFileInfo = BluetoothOppSendFileInfo.generateFileInfo(
                mContext, webpage, "html", false);
        assertThat(sendFileInfo).isEqualTo(BluetoothOppSendFileInfo.SEND_FILE_INFO_ERROR);
    }

    /**
     * Test a BluetoothOppSendFileInfo generated from a big local file (MIME type: text/plain,
     * size: 8GB). It should return an error BluetoothOppSendFileInfo because the maximum size of
     * file supported is #MAXIMUM_FILE_SIZE (4GB).
     */
    @Test
    public void testBigFileOpen() throws IOException {
        Uri uri = Uri.parse("file:///android_asset/opp/OppTestFile.txt");
        doReturn(mContentResolver).when(mContext).getContentResolver();
        doReturn(mFileInputStream).when(mContentResolver).openInputStream(uri);
        doReturn(MAXIMUM_FILE_SIZE, MAXIMUM_FILE_SIZE, -1).when(mFileInputStream).read(any(),
                eq(0), eq(4096));
        BluetoothOppSendFileInfo sendFileInfo = BluetoothOppSendFileInfo.generateFileInfo(
                mContext, uri, "text/plain", false);
        assertThat(sendFileInfo).isEqualTo(BluetoothOppSendFileInfo.SEND_FILE_INFO_ERROR);

    }

}
