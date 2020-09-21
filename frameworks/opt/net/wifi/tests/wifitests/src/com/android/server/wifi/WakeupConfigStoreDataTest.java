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

package com.android.server.wifi;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.util.Xml;

import com.android.internal.util.FastXmlSerializer;

import com.google.android.collect.Sets;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.xmlpull.v1.XmlPullParser;
import org.xmlpull.v1.XmlPullParserException;
import org.xmlpull.v1.XmlSerializer;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.nio.charset.StandardCharsets;
import java.util.Collections;
import java.util.Set;

/**
 * Unit tests for {@link WakeupConfigStoreData}.
 */
public class WakeupConfigStoreDataTest {

    @Mock private WakeupConfigStoreData.DataSource<Boolean> mActiveDataSource;
    @Mock private WakeupConfigStoreData.DataSource<Boolean> mIsOnboardedDataSource;
    @Mock private WakeupConfigStoreData.DataSource<Integer> mNotificationsDataSource;
    @Mock private WakeupConfigStoreData.DataSource<Set<ScanResultMatchInfo>> mNetworkDataSource;

    private WakeupConfigStoreData mWakeupConfigData;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);

        mWakeupConfigData = new WakeupConfigStoreData(mActiveDataSource, mIsOnboardedDataSource,
                mNotificationsDataSource, mNetworkDataSource);
    }

    /**
     * Helper function for serializing configuration data to a XML block.
     *
     * @param shared Flag indicating serializing shared or user configurations
     * @return byte[] of the XML data
     */
    private byte[] serializeData(boolean shared) throws Exception {
        final XmlSerializer out = new FastXmlSerializer();
        final ByteArrayOutputStream outputStream = new ByteArrayOutputStream();
        out.setOutput(outputStream, StandardCharsets.UTF_8.name());
        mWakeupConfigData.serializeData(out, shared);
        out.flush();
        return outputStream.toByteArray();
    }

    /**
     * Helper function for parsing configuration data from a XML block.
     *
     * @param data XML data to parse from
     * @param shared Flag indicating parsing of shared or user configurations
     */
    private void deserializeData(byte[] data, boolean shared) throws Exception {
        final XmlPullParser in = Xml.newPullParser();
        final ByteArrayInputStream inputStream = new ByteArrayInputStream(data);
        in.setInput(inputStream, StandardCharsets.UTF_8.name());
        mWakeupConfigData.deserializeData(in, in.getDepth(), shared);
    }

    /**
     * Verify that a XmlPullParserException will be thrown when attempting to serialize data
     * to the share store.
     */
    @Test(expected = XmlPullParserException.class)
    public void serializeShareDataThrowsException() throws Exception {
        serializeData(true /* shared */);
    }

    /**
     * Verify that a XmlPullParserException will be thrown when attempting to deserialize
     * data from the share store.
     */
    @Test(expected = XmlPullParserException.class)
    public void deserializeShareDataThrowsException() throws Exception {
        deserializeData(new byte[0], true /* shared */);
    }

    /**
     * Can correctly serialize and deserialize the empty set case.
     */
    @Test
    public void deserializeSerializedData_emptySet() throws Exception {
        Set<ScanResultMatchInfo> networks = Collections.emptySet();
        boolean isActive = false;
        boolean isOnboarded = false;
        int notificationsShown = 0;

        when(mActiveDataSource.getData()).thenReturn(isActive);
        when(mIsOnboardedDataSource.getData()).thenReturn(isOnboarded);
        when(mNotificationsDataSource.getData()).thenReturn(notificationsShown);
        when(mNetworkDataSource.getData()).thenReturn(networks);

        byte[] bytes = serializeData(false /* shared */);
        deserializeData(bytes, false /* shared */);

        verify(mActiveDataSource).setData(eq(isActive));
        verify(mIsOnboardedDataSource).setData(eq(isOnboarded));
        verify(mNotificationsDataSource).setData(notificationsShown);
        verify(mNetworkDataSource).setData(eq(networks));
    }

    /**
     * Can correctly serialize and deserialize a standard working case.
     */
    @Test
    public void deserializeSerializedData() throws Exception {
        ScanResultMatchInfo network1 = new ScanResultMatchInfo();
        network1.networkSsid = "ssid 1";
        network1.networkType = 0;

        ScanResultMatchInfo network2 = new ScanResultMatchInfo();
        network2.networkSsid = ",.23;4@, .#,%(,";
        network2.networkType = 1;

        ScanResultMatchInfo network3 = new ScanResultMatchInfo();
        network3.networkSsid = "";
        network3.networkType = 2;

        Set<ScanResultMatchInfo> networks = Sets.newArraySet(network1, network2, network3);
        boolean isActive = true;
        boolean isOnboarded = false;
        int notificationsShown = 1;

        when(mActiveDataSource.getData()).thenReturn(isActive);
        when(mIsOnboardedDataSource.getData()).thenReturn(isOnboarded);
        when(mNotificationsDataSource.getData()).thenReturn(notificationsShown);
        when(mNetworkDataSource.getData()).thenReturn(networks);

        byte[] bytes = serializeData(false /* shared */);
        deserializeData(bytes, false /* shared */);

        verify(mActiveDataSource).setData(eq(isActive));
        verify(mIsOnboardedDataSource).setData(eq(isOnboarded));
        verify(mNotificationsDataSource).setData(notificationsShown);
        verify(mNetworkDataSource).setData(eq(networks));
    }

    /**
     * Verify that reset data wipes the data sources.
     */
    @Test
    public void resetDataWipesDataSources() {
        mWakeupConfigData.resetData(false /* shared */);

        verify(mActiveDataSource).setData(false);
        verify(mIsOnboardedDataSource).setData(false);
        verify(mNotificationsDataSource).setData(0);
        verify(mNetworkDataSource).setData(eq(Collections.emptySet()));
    }

    /**
     * Verify that hasBeenRead returns false on newly instantiated WakeupConfigStoreData.
     */
    @Test
    public void hasBeenReadIsFalseWhenInitialized() {
        assertFalse(mWakeupConfigData.hasBeenRead());
    }

    /**
     * Verify that hasBeenRead returns true after the user store has been read.
     */
    @Test
    public void hasBeenReadIsTrueWhenUserStoreIsLoaded() throws Exception {
        mWakeupConfigData.deserializeData(null /* in */, 0 /* outerTagDepth */, false /* shared */);
        assertTrue(mWakeupConfigData.hasBeenRead());
    }

    /**
     * Verify that hasBeenRead returns false if only the shared store has been read.
     */
    @Test
    public void hasBeenReadIsFalseWhenSharedStoreIsLoaded() throws Exception {
        mWakeupConfigData.deserializeData(null /* in */, 0 /* outerTagDepth */, true /* shared */);
        assertFalse(mWakeupConfigData.hasBeenRead());
    }
}
