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
 * limitations under the License
 */

package com.android.providers.blockednumber;

import static android.provider.BlockedNumberContract.SystemContract
        .ENHANCED_SETTING_KEY_BLOCK_PAYPHONE;

import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.when;
import static org.mockito.MockitoAnnotations.initMocks;

import android.content.ContentProvider;
import android.content.ContentResolver;
import android.content.Context;
import android.net.Uri;
import android.os.Bundle;
import android.provider.BlockedNumberContract;
import android.test.AndroidTestCase;
import android.test.mock.MockContentResolver;
import android.test.suitebuilder.annotation.SmallTest;

import junit.framework.TestCase;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.ArgumentMatchers;
import org.mockito.Mock;
import org.mockito.Mockito;

/**
 * Tests for {@link android.provider.BlockedNumberContract}.
 */
@RunWith(JUnit4.class)
public class BlockedNumberContractTest extends AndroidTestCase {
    private static final String TEST_NUMBER = "650-555-1212";

    @Mock
    private Context mMockContext;

    @Mock
    private ContentProvider mMockProvider;

    private MockContentResolver mMockContentResolver = new MockContentResolver();

    @Before
    @Override
    public void setUp() throws Exception {
        super.setUp();
        initMocks(this);
        mMockContentResolver.addProvider(BlockedNumberContract.AUTHORITY_URI.toString(),
                mMockProvider);
        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);
        when(mMockProvider.call(any(), any(), any()))
                .thenThrow(new NullPointerException());
    }

    /**
     * Ensures a content provider crash results in a "false" response from
     * {@link BlockedNumberContract#isBlocked(Context, String)}.
     */
    @SmallTest
    @Test
    public void testIsBlockedException() {
        assertFalse(BlockedNumberContract.isBlocked(mMockContext, TEST_NUMBER));
    }

    /**
     * Ensures a content provider crash results in a "false" response from
     * {@link BlockedNumberContract#canCurrentUserBlockNumbers(Context)}.
     */
    @SmallTest
    @Test
    public void testCanUserBlockNumbersException() {
        assertFalse(BlockedNumberContract.canCurrentUserBlockNumbers(mMockContext));
    }

    /**
     * Ensures a content provider crash results in a "false" response from
     * {@link BlockedNumberContract.SystemContract#shouldSystemBlockNumber(Context, String, Bundle)}
     */
    @SmallTest
    @Test
    public void testShouldSystemBlockNumberException() {
        assertFalse(BlockedNumberContract.SystemContract.shouldSystemBlockNumber(mMockContext,
                TEST_NUMBER, null));
    }

    /**
     * Ensures a content provider crash results in a "false" response from
     * {@link BlockedNumberContract.SystemContract#shouldShowEmergencyCallNotification(Context)}.
     */
    @SmallTest
    @Test
    public void testShouldShowEmergencyCallNotificationException() {
        assertFalse(BlockedNumberContract.SystemContract.shouldShowEmergencyCallNotification(
                mMockContext));
    }

    /**
     * Ensures a content provider crash results in a "false" response from
     * {@link BlockedNumberContract.SystemContract#getEnhancedBlockSetting(Context, String)}.
     */
    @SmallTest
    @Test
    public void testGetEnhancedBlockSettingException() {
        assertFalse(BlockedNumberContract.SystemContract.getEnhancedBlockSetting(
                mMockContext, ENHANCED_SETTING_KEY_BLOCK_PAYPHONE));
    }
}
