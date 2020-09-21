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

package com.android.car;

import static org.mockito.Mockito.any;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.res.Resources;
import android.os.ParcelUuid;
import android.support.test.filters.SmallTest;
import android.support.test.runner.AndroidJUnit4;

import org.junit.Assert;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.Arrays;

@SmallTest
@RunWith(AndroidJUnit4.class)
public class FastPairProviderTest {
    // Service ID assigned for FastPair.
    private static final ParcelUuid FastPairServiceUuid = ParcelUuid
            .fromString("0000FE2C-0000-1000-8000-00805f9b34fb");

    @Mock
    private BluetoothLeAdvertiser mBluetoothLeAdvertiser;
    @Mock
    private BluetoothManager mMockBluetoothManager;
    @Mock
    private BluetoothAdapter mMockBluetoothAdapter;
    @Mock
    private Context mMockContext;
    @Mock
    Resources mMockResources;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mMockContext.getSystemService(Context.BLUETOOTH_SERVICE)).thenReturn(
                mMockBluetoothManager);
        when(mMockBluetoothManager.getAdapter()).thenReturn(mMockBluetoothAdapter);
        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockResources.getInteger(R.integer.fastPairModelId)).thenReturn(0);
        when(mMockBluetoothAdapter.getBluetoothLeAdvertiser()).thenReturn(mBluetoothLeAdvertiser);
    }

    /**
     * Verify that when a model id is set it gets serialized correctly.
     */
    @Test
    public void enabledViaModelIdTest() {
        int modelId = 0xABCDEF;
        byte[] modelIdBytes = new byte[]{(byte) 0xEF, (byte) 0xCD, (byte) 0xAB};
        when(mMockResources.getInteger(R.integer.fastPairModelId)).thenReturn(modelId);
        ArgumentCaptor<AdvertiseData> advertiseDataCaptor = ArgumentCaptor.forClass(
                AdvertiseData.class);

        FastPairProvider fastPairProvider = new FastPairProvider(mMockContext);
        fastPairProvider.startAdvertising();

        verify(mBluetoothLeAdvertiser).startAdvertising(any(), advertiseDataCaptor.capture(),
                any());
        Assert.assertTrue(Arrays.equals(modelIdBytes,
                advertiseDataCaptor.getValue().getServiceData().get(FastPairServiceUuid)));
    }

    /**
     * Verify that when the model id is 0 Fast Pair is disabled.
     */
    @Test
    public void disabledViaModelIdTest() {
        FastPairProvider fastPairProvider = new FastPairProvider(mMockContext);
        fastPairProvider.startAdvertising();
        verify(mBluetoothLeAdvertiser, never()).startAdvertising(any(), any(), any());
    }
}
