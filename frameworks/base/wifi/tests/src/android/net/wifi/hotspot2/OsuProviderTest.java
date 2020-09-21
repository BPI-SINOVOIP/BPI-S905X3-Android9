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
 * limitations under the License
 */

package android.net.wifi.hotspot2;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.graphics.drawable.Icon;
import android.net.Uri;
import android.net.wifi.WifiSsid;
import android.os.Parcel;
import android.support.test.filters.SmallTest;

import org.junit.Test;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Unit tests for {@link android.net.wifi.hotspot2.OsuProvider}.
 */
@SmallTest
public class OsuProviderTest {
    private static final WifiSsid TEST_SSID =
            WifiSsid.createFromByteArray("TEST SSID".getBytes(StandardCharsets.UTF_8));
    private static final String TEST_FRIENDLY_NAME = "Friendly Name";
    private static final String TEST_SERVICE_DESCRIPTION = "Dummy Service";
    private static final Uri TEST_SERVER_URI = Uri.parse("https://test.com");
    private static final String TEST_NAI = "test.access.com";
    private static final List<Integer> TEST_METHOD_LIST =
            Arrays.asList(OsuProvider.METHOD_SOAP_XML_SPP);
    private static final Icon TEST_ICON = Icon.createWithData(new byte[10], 0, 10);

    /**
     * Verify parcel write and read consistency for the given {@link OsuProvider}.
     *
     * @param writeInfo The {@link OsuProvider} to verify
     * @throws Exception
     */
    private static void verifyParcel(OsuProvider writeInfo) throws Exception {
        Parcel parcel = Parcel.obtain();
        writeInfo.writeToParcel(parcel, 0);

        parcel.setDataPosition(0);    // Rewind data position back to the beginning for read.
        OsuProvider readInfo = OsuProvider.CREATOR.createFromParcel(parcel);
        assertEquals(writeInfo, readInfo);
    }

    /**
     * Verify parcel read/write for an OSU provider containing no information.
     *
     * @throws Exception
     */
    @Test
    public void verifyParcelWithEmptyProviderInfo() throws Exception {
        verifyParcel(new OsuProvider(null, null, null, null, null, null, null));
    }

    /**
     * Verify parcel read/write for an OSU provider containing full information.
     *
     * @throws Exception
     */
    @Test
    public void verifyParcelWithFullProviderInfo() throws Exception {
        verifyParcel(new OsuProvider(TEST_SSID, TEST_FRIENDLY_NAME, TEST_SERVICE_DESCRIPTION,
                TEST_SERVER_URI, TEST_NAI, TEST_METHOD_LIST, TEST_ICON));
    }

    /**
     * Verify copy constructor with a null source.
     * @throws Exception
     */
    @Test
    public void verifyCopyConstructorWithNullSource() throws Exception {
        OsuProvider expected = new OsuProvider(null, null, null, null, null, null, null);
        assertEquals(expected, new OsuProvider(null));
    }

    /**
     * Verify copy constructor with a valid source.
     *
     * @throws Exception
     */
    @Test
    public void verifyCopyConstructorWithValidSource() throws Exception {
        OsuProvider source = new OsuProvider(TEST_SSID, TEST_FRIENDLY_NAME, TEST_SERVICE_DESCRIPTION,
                TEST_SERVER_URI, TEST_NAI, TEST_METHOD_LIST, TEST_ICON);
        assertEquals(source, new OsuProvider(source));
    }

    /**
     * Verify getter methods.
     *
     * @throws Exception
     */
    @Test
    public void verifyGetters() throws Exception {
        OsuProvider provider = new OsuProvider(TEST_SSID, TEST_FRIENDLY_NAME,
                TEST_SERVICE_DESCRIPTION, TEST_SERVER_URI, TEST_NAI, TEST_METHOD_LIST, TEST_ICON);
        assertTrue(TEST_SSID.equals(provider.getOsuSsid()));
        assertTrue(TEST_FRIENDLY_NAME.equals(provider.getFriendlyName()));
        assertTrue(TEST_SERVICE_DESCRIPTION.equals(provider.getServiceDescription()));
        assertTrue(TEST_SERVER_URI.equals(provider.getServerUri()));
        assertTrue(TEST_NAI.equals(provider.getNetworkAccessIdentifier()));
        assertTrue(TEST_METHOD_LIST.equals(provider.getMethodList()));
        assertTrue(TEST_ICON.sameAs(provider.getIcon()));
    }
}
