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
 * limitations under the License
 */

package com.android.server.wifi;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.any;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.net.MacAddress;
import android.net.wifi.WifiConfiguration;
import android.os.INetworkManagementService;
import android.support.test.filters.SmallTest;

import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashSet;
import java.util.Set;
import java.util.regex.Pattern;

/**
 * Unit tests for {@link com.android.server.wifi.WifiNative}.
 */
@SmallTest
public class WifiNativeTest {
    private static final String WIFI_IFACE_NAME = "mockWlan";
    private static final long FATE_REPORT_DRIVER_TIMESTAMP_USEC = 12345;
    private static final byte[] FATE_REPORT_FRAME_BYTES = new byte[] {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 0, 1, 2, 3, 4, 5, 6, 7};
    private static final WifiNative.TxFateReport TX_FATE_REPORT = new WifiNative.TxFateReport(
            WifiLoggerHal.TX_PKT_FATE_SENT,
            FATE_REPORT_DRIVER_TIMESTAMP_USEC,
            WifiLoggerHal.FRAME_TYPE_ETHERNET_II,
            FATE_REPORT_FRAME_BYTES
    );
    private static final WifiNative.RxFateReport RX_FATE_REPORT = new WifiNative.RxFateReport(
            WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID,
            FATE_REPORT_DRIVER_TIMESTAMP_USEC,
            WifiLoggerHal.FRAME_TYPE_ETHERNET_II,
            FATE_REPORT_FRAME_BYTES
    );
    private static final FrameTypeMapping[] FRAME_TYPE_MAPPINGS = new FrameTypeMapping[] {
            new FrameTypeMapping(WifiLoggerHal.FRAME_TYPE_UNKNOWN, "unknown", "N/A"),
            new FrameTypeMapping(WifiLoggerHal.FRAME_TYPE_ETHERNET_II, "data", "Ethernet"),
            new FrameTypeMapping(WifiLoggerHal.FRAME_TYPE_80211_MGMT, "802.11 management",
                    "802.11 Mgmt"),
            new FrameTypeMapping((byte) 42, "42", "N/A")
    };
    private static final FateMapping[] TX_FATE_MAPPINGS = new FateMapping[] {
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_ACKED, "acked"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_SENT, "sent"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_FW_QUEUED, "firmware queued"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_FW_DROP_INVALID,
                    "firmware dropped (invalid frame)"),
            new FateMapping(
                    WifiLoggerHal.TX_PKT_FATE_FW_DROP_NOBUFS,  "firmware dropped (no bufs)"),
            new FateMapping(
                    WifiLoggerHal.TX_PKT_FATE_FW_DROP_OTHER, "firmware dropped (other)"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_DRV_QUEUED, "driver queued"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_DRV_DROP_INVALID,
                    "driver dropped (invalid frame)"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_DRV_DROP_NOBUFS,
                    "driver dropped (no bufs)"),
            new FateMapping(WifiLoggerHal.TX_PKT_FATE_DRV_DROP_OTHER, "driver dropped (other)"),
            new FateMapping((byte) 42, "42")
    };
    private static final FateMapping[] RX_FATE_MAPPINGS = new FateMapping[] {
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_SUCCESS, "success"),
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_FW_QUEUED, "firmware queued"),
            new FateMapping(
                    WifiLoggerHal.RX_PKT_FATE_FW_DROP_FILTER, "firmware dropped (filter)"),
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID,
                    "firmware dropped (invalid frame)"),
            new FateMapping(
                    WifiLoggerHal.RX_PKT_FATE_FW_DROP_NOBUFS, "firmware dropped (no bufs)"),
            new FateMapping(
                    WifiLoggerHal.RX_PKT_FATE_FW_DROP_OTHER, "firmware dropped (other)"),
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_DRV_QUEUED, "driver queued"),
            new FateMapping(
                    WifiLoggerHal.RX_PKT_FATE_DRV_DROP_FILTER, "driver dropped (filter)"),
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_DRV_DROP_INVALID,
                    "driver dropped (invalid frame)"),
            new FateMapping(
                    WifiLoggerHal.RX_PKT_FATE_DRV_DROP_NOBUFS, "driver dropped (no bufs)"),
            new FateMapping(WifiLoggerHal.RX_PKT_FATE_DRV_DROP_OTHER, "driver dropped (other)"),
            new FateMapping((byte) 42, "42")
    };
    private static final WifiNative.SignalPollResult SIGNAL_POLL_RESULT =
            new WifiNative.SignalPollResult() {{
                currentRssi = -60;
                txBitrate = 12;
                associationFrequency = 5240;
            }};
    private static final WifiNative.TxPacketCounters PACKET_COUNTERS_RESULT =
            new WifiNative.TxPacketCounters() {{
                txSucceeded = 2000;
                txFailed = 120;
            }};

    private static final Set<Integer> SCAN_FREQ_SET =
            new HashSet<Integer>() {{
                add(2410);
                add(2450);
                add(5050);
                add(5200);
            }};
    private static final String TEST_QUOTED_SSID_1 = "\"testSsid1\"";
    private static final String TEST_QUOTED_SSID_2 = "\"testSsid2\"";
    private static final Set<String> SCAN_HIDDEN_NETWORK_SSID_SET =
            new HashSet<String>() {{
                add(TEST_QUOTED_SSID_1);
                add(TEST_QUOTED_SSID_2);
            }};

    private static final WifiNative.PnoSettings TEST_PNO_SETTINGS =
            new WifiNative.PnoSettings() {{
                isConnected = false;
                periodInMs = 6000;
                networkList = new WifiNative.PnoNetwork[2];
                networkList[0] = new WifiNative.PnoNetwork();
                networkList[1] = new WifiNative.PnoNetwork();
                networkList[0].ssid = TEST_QUOTED_SSID_1;
                networkList[1].ssid = TEST_QUOTED_SSID_2;
            }};
    private static final MacAddress TEST_MAC_ADDRESS = MacAddress.fromString("ee:33:a2:94:10:92");

    @Mock private WifiVendorHal mWifiVendorHal;
    @Mock private WificondControl mWificondControl;
    @Mock private SupplicantStaIfaceHal mStaIfaceHal;
    @Mock private HostapdHal mHostapdHal;
    @Mock private WifiMonitor mWifiMonitor;
    @Mock private INetworkManagementService mNwService;
    @Mock private PropertyService mPropertyService;
    @Mock private WifiMetrics mWifiMetrics;
    private WifiNative mWifiNative;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        when(mWifiVendorHal.isVendorHalSupported()).thenReturn(true);
        when(mWifiVendorHal.startVendorHalSta()).thenReturn(true);
        when(mWifiVendorHal.startVendorHalAp()).thenReturn(true);
        mWifiNative = new WifiNative(
                mWifiVendorHal, mStaIfaceHal, mHostapdHal, mWificondControl,
                mWifiMonitor, mNwService, mPropertyService, mWifiMetrics);
    }

    /**
     * Verifies that TxFateReport's constructor sets all of the TxFateReport fields.
     */
    @Test
    public void testTxFateReportCtorSetsFields() {
        WifiNative.TxFateReport fateReport = new WifiNative.TxFateReport(
                WifiLoggerHal.TX_PKT_FATE_SENT,  // non-zero value
                FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                WifiLoggerHal.FRAME_TYPE_ETHERNET_II,  // non-zero value
                FATE_REPORT_FRAME_BYTES
        );
        assertEquals(WifiLoggerHal.TX_PKT_FATE_SENT, fateReport.mFate);
        assertEquals(FATE_REPORT_DRIVER_TIMESTAMP_USEC, fateReport.mDriverTimestampUSec);
        assertEquals(WifiLoggerHal.FRAME_TYPE_ETHERNET_II, fateReport.mFrameType);
        assertArrayEquals(FATE_REPORT_FRAME_BYTES, fateReport.mFrameBytes);
    }

    /**
     * Verifies that RxFateReport's constructor sets all of the RxFateReport fields.
     */
    @Test
    public void testRxFateReportCtorSetsFields() {
        WifiNative.RxFateReport fateReport = new WifiNative.RxFateReport(
                WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID,  // non-zero value
                FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                WifiLoggerHal.FRAME_TYPE_ETHERNET_II,  // non-zero value
                FATE_REPORT_FRAME_BYTES
        );
        assertEquals(WifiLoggerHal.RX_PKT_FATE_FW_DROP_INVALID, fateReport.mFate);
        assertEquals(FATE_REPORT_DRIVER_TIMESTAMP_USEC, fateReport.mDriverTimestampUSec);
        assertEquals(WifiLoggerHal.FRAME_TYPE_ETHERNET_II, fateReport.mFrameType);
        assertArrayEquals(FATE_REPORT_FRAME_BYTES, fateReport.mFrameBytes);
    }

    /**
     * Verifies the hashCode methods for HiddenNetwork and PnoNetwork classes
     */
    @Test
    public void testHashCode() {
        WifiNative.HiddenNetwork hiddenNet1 = new WifiNative.HiddenNetwork();
        hiddenNet1.ssid = new String("sametext");

        WifiNative.HiddenNetwork hiddenNet2 = new WifiNative.HiddenNetwork();
        hiddenNet2.ssid = new String("sametext");

        assertTrue(hiddenNet1.equals(hiddenNet2));
        assertEquals(hiddenNet1.hashCode(), hiddenNet2.hashCode());

        WifiNative.PnoNetwork pnoNet1 = new WifiNative.PnoNetwork();
        pnoNet1.ssid = new String("sametext");
        pnoNet1.flags = 2;
        pnoNet1.auth_bit_field = 4;

        WifiNative.PnoNetwork pnoNet2 = new WifiNative.PnoNetwork();
        pnoNet2.ssid = new String("sametext");
        pnoNet2.flags = 2;
        pnoNet2.auth_bit_field = 4;

        assertTrue(pnoNet1.equals(pnoNet2));
        assertEquals(pnoNet1.hashCode(), pnoNet2.hashCode());
    }

    // Support classes for test{Tx,Rx}FateReportToString.
    private static class FrameTypeMapping {
        byte mTypeNumber;
        String mExpectedTypeText;
        String mExpectedProtocolText;
        FrameTypeMapping(byte typeNumber, String expectedTypeText, String expectedProtocolText) {
            this.mTypeNumber = typeNumber;
            this.mExpectedTypeText = expectedTypeText;
            this.mExpectedProtocolText = expectedProtocolText;
        }
    }
    private static class FateMapping {
        byte mFateNumber;
        String mExpectedText;
        FateMapping(byte fateNumber, String expectedText) {
            this.mFateNumber = fateNumber;
            this.mExpectedText = expectedText;
        }
    }

    /**
     * Verifies that FateReport.getTableHeader() prints the right header.
     */
    @Test
    public void testFateReportTableHeader() {
        final String header = WifiNative.FateReport.getTableHeader();
        assertEquals(
                "\nTime usec        Walltime      Direction  Fate                              "
                + "Protocol      Type                     Result\n"
                + "---------        --------      ---------  ----                              "
                + "--------      ----                     ------\n", header);
    }

    /**
     * Verifies that TxFateReport.toTableRowString() includes the information we care about.
     */
    @Test
    public void testTxFateReportToTableRowString() {
        WifiNative.TxFateReport fateReport = TX_FATE_REPORT;
        assertTrue(
                fateReport.toTableRowString().replaceAll("\\s+", " ").trim().matches(
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC + " "  // timestamp
                            + "\\d{2}:\\d{2}:\\d{2}\\.\\d{3} "  // walltime
                            + "TX "  // direction
                            + "sent "  // fate
                            + "Ethernet "  // type
                            + "N/A "  // protocol
                            + "N/A"  // result
                )
        );

        for (FrameTypeMapping frameTypeMapping : FRAME_TYPE_MAPPINGS) {
            fateReport = new WifiNative.TxFateReport(
                    WifiLoggerHal.TX_PKT_FATE_SENT,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    frameTypeMapping.mTypeNumber,
                    FATE_REPORT_FRAME_BYTES
            );
            assertTrue(
                    fateReport.toTableRowString().replaceAll("\\s+", " ").trim().matches(
                            FATE_REPORT_DRIVER_TIMESTAMP_USEC + " "  // timestamp
                                    + "\\d{2}:\\d{2}:\\d{2}\\.\\d{3} "  // walltime
                                    + "TX "  // direction
                                    + "sent "  // fate
                                    + frameTypeMapping.mExpectedProtocolText + " "  // type
                                    + "N/A "  // protocol
                                    + "N/A"  // result
                    )
            );
        }

        for (FateMapping fateMapping : TX_FATE_MAPPINGS) {
            fateReport = new WifiNative.TxFateReport(
                    fateMapping.mFateNumber,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    WifiLoggerHal.FRAME_TYPE_80211_MGMT,
                    FATE_REPORT_FRAME_BYTES
            );
            assertTrue(
                    fateReport.toTableRowString().replaceAll("\\s+", " ").trim().matches(
                            FATE_REPORT_DRIVER_TIMESTAMP_USEC + " "  // timestamp
                                    + "\\d{2}:\\d{2}:\\d{2}\\.\\d{3} "  // walltime
                                    + "TX "  // direction
                                    + Pattern.quote(fateMapping.mExpectedText) + " "  // fate
                                    + "802.11 Mgmt "  // type
                                    + "N/A "  // protocol
                                    + "N/A"  // result
                    )
            );
        }
    }

    /**
     * Verifies that TxFateReport.toVerboseStringWithPiiAllowed() includes the information we care
     * about.
     */
    @Test
    public void testTxFateReportToVerboseStringWithPiiAllowed() {
        WifiNative.TxFateReport fateReport = TX_FATE_REPORT;

        String verboseFateString = fateReport.toVerboseStringWithPiiAllowed();
        assertTrue(verboseFateString.contains("Frame direction: TX"));
        assertTrue(verboseFateString.contains("Frame timestamp: 12345"));
        assertTrue(verboseFateString.contains("Frame fate: sent"));
        assertTrue(verboseFateString.contains("Frame type: data"));
        assertTrue(verboseFateString.contains("Frame protocol: Ethernet"));
        assertTrue(verboseFateString.contains("Frame protocol type: N/A"));
        assertTrue(verboseFateString.contains("Frame length: 16"));
        assertTrue(verboseFateString.contains(
                "61 62 63 64 65 66 67 68 00 01 02 03 04 05 06 07")); // hex dump
        // TODO(quiche): uncomment this, once b/27975149 is fixed.
        // assertTrue(verboseFateString.contains("abcdefgh........"));  // hex dump

        for (FrameTypeMapping frameTypeMapping : FRAME_TYPE_MAPPINGS) {
            fateReport = new WifiNative.TxFateReport(
                    WifiLoggerHal.TX_PKT_FATE_SENT,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    frameTypeMapping.mTypeNumber,
                    FATE_REPORT_FRAME_BYTES
            );
            verboseFateString = fateReport.toVerboseStringWithPiiAllowed();
            assertTrue(verboseFateString.contains("Frame type: "
                    + frameTypeMapping.mExpectedTypeText));
        }

        for (FateMapping fateMapping : TX_FATE_MAPPINGS) {
            fateReport = new WifiNative.TxFateReport(
                    fateMapping.mFateNumber,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    WifiLoggerHal.FRAME_TYPE_80211_MGMT,
                    FATE_REPORT_FRAME_BYTES
            );
            verboseFateString = fateReport.toVerboseStringWithPiiAllowed();
            assertTrue(verboseFateString.contains("Frame fate: " + fateMapping.mExpectedText));
        }
    }

    /**
     * Verifies that RxFateReport.toTableRowString() includes the information we care about.
     */
    @Test
    public void testRxFateReportToTableRowString() {
        WifiNative.RxFateReport fateReport = RX_FATE_REPORT;
        assertTrue(
                fateReport.toTableRowString().replaceAll("\\s+", " ").trim().matches(
                        FATE_REPORT_DRIVER_TIMESTAMP_USEC + " "  // timestamp
                                + "\\d{2}:\\d{2}:\\d{2}\\.\\d{3} "  // walltime
                                + "RX "  // direction
                                + Pattern.quote("firmware dropped (invalid frame) ")  // fate
                                + "Ethernet "  // type
                                + "N/A "  // protocol
                                + "N/A"  // result
                )
        );

        // FrameTypeMappings omitted, as they're the same as for TX.

        for (FateMapping fateMapping : RX_FATE_MAPPINGS) {
            fateReport = new WifiNative.RxFateReport(
                    fateMapping.mFateNumber,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    WifiLoggerHal.FRAME_TYPE_80211_MGMT,
                    FATE_REPORT_FRAME_BYTES
            );
            assertTrue(
                    fateReport.toTableRowString().replaceAll("\\s+", " ").trim().matches(
                            FATE_REPORT_DRIVER_TIMESTAMP_USEC + " "  // timestamp
                                    + "\\d{2}:\\d{2}:\\d{2}\\.\\d{3} "  // walltime
                                    + "RX "  // direction
                                    + Pattern.quote(fateMapping.mExpectedText) + " " // fate
                                    + "802.11 Mgmt "  // type
                                    + "N/A " // protocol
                                    + "N/A"  // result
                    )
            );
        }
    }

    /**
     * Verifies that RxFateReport.toVerboseStringWithPiiAllowed() includes the information we care
     * about.
     */
    @Test
    public void testRxFateReportToVerboseStringWithPiiAllowed() {
        WifiNative.RxFateReport fateReport = RX_FATE_REPORT;

        String verboseFateString = fateReport.toVerboseStringWithPiiAllowed();
        assertTrue(verboseFateString.contains("Frame direction: RX"));
        assertTrue(verboseFateString.contains("Frame timestamp: 12345"));
        assertTrue(verboseFateString.contains("Frame fate: firmware dropped (invalid frame)"));
        assertTrue(verboseFateString.contains("Frame type: data"));
        assertTrue(verboseFateString.contains("Frame protocol: Ethernet"));
        assertTrue(verboseFateString.contains("Frame protocol type: N/A"));
        assertTrue(verboseFateString.contains("Frame length: 16"));
        assertTrue(verboseFateString.contains(
                "61 62 63 64 65 66 67 68 00 01 02 03 04 05 06 07")); // hex dump
        // TODO(quiche): uncomment this, once b/27975149 is fixed.
        // assertTrue(verboseFateString.contains("abcdefgh........"));  // hex dump

        // FrameTypeMappings omitted, as they're the same as for TX.

        for (FateMapping fateMapping : RX_FATE_MAPPINGS) {
            fateReport = new WifiNative.RxFateReport(
                    fateMapping.mFateNumber,
                    FATE_REPORT_DRIVER_TIMESTAMP_USEC,
                    WifiLoggerHal.FRAME_TYPE_80211_MGMT,
                    FATE_REPORT_FRAME_BYTES
            );
            verboseFateString = fateReport.toVerboseStringWithPiiAllowed();
            assertTrue(verboseFateString.contains("Frame fate: " + fateMapping.mExpectedText));
        }
    }

    /**
     * Verifies that startPktFateMonitoring returns false when HAL is not started.
     */
    @Test
    public void testStartPktFateMonitoringReturnsFalseWhenHalIsNotStarted() {
        assertFalse(mWifiNative.isHalStarted());
        assertFalse(mWifiNative.startPktFateMonitoring(WIFI_IFACE_NAME));
    }

    /**
     * Verifies that getTxPktFates returns error when HAL is not started.
     */
    @Test
    public void testGetTxPktFatesReturnsErrorWhenHalIsNotStarted() {
        WifiNative.TxFateReport[] fateReports = null;
        assertFalse(mWifiNative.isHalStarted());
        assertFalse(mWifiNative.getTxPktFates(WIFI_IFACE_NAME, fateReports));
    }

    /**
     * Verifies that getRxPktFates returns error when HAL is not started.
     */
    @Test
    public void testGetRxPktFatesReturnsErrorWhenHalIsNotStarted() {
        WifiNative.RxFateReport[] fateReports = null;
        assertFalse(mWifiNative.isHalStarted());
        assertFalse(mWifiNative.getRxPktFates(WIFI_IFACE_NAME, fateReports));
    }

    // TODO(quiche): Add tests for the success cases (when HAL has been started). Specifically:
    // - testStartPktFateMonitoringCallsHalIfHalIsStarted()
    // - testGetTxPktFatesCallsHalIfHalIsStarted()
    // - testGetRxPktFatesCallsHalIfHalIsStarted()
    //
    // Adding these tests is difficult to do at the moment, because we can't mock out the HAL
    // itself. Also, we can't mock out the native methods, because those methods are private.
    // b/28005116.

    /** Verifies that getDriverStateDumpNative returns null when HAL is not started. */
    @Test
    public void testGetDriverStateDumpReturnsNullWhenHalIsNotStarted() {
        assertEquals(null, mWifiNative.getDriverStateDump());
    }

    // TODO(b/28005116): Add test for the success case of getDriverStateDump().

    /**
     * Verifies that signalPoll() calls underlying WificondControl.
     */
    @Test
    public void testSignalPoll() throws Exception {
        when(mWificondControl.signalPoll(WIFI_IFACE_NAME))
                .thenReturn(SIGNAL_POLL_RESULT);

        assertEquals(SIGNAL_POLL_RESULT, mWifiNative.signalPoll(WIFI_IFACE_NAME));
        verify(mWificondControl).signalPoll(WIFI_IFACE_NAME);
    }

    /**
     * Verifies that getTxPacketCounters() calls underlying WificondControl.
     */
    @Test
    public void testGetTxPacketCounters() throws Exception {
        when(mWificondControl.getTxPacketCounters(WIFI_IFACE_NAME))
                .thenReturn(PACKET_COUNTERS_RESULT);

        assertEquals(PACKET_COUNTERS_RESULT, mWifiNative.getTxPacketCounters(WIFI_IFACE_NAME));
        verify(mWificondControl).getTxPacketCounters(WIFI_IFACE_NAME);
    }

    /**
     * Verifies that scan() calls underlying WificondControl.
     */
    @Test
    public void testScan() throws Exception {
        mWifiNative.scan(WIFI_IFACE_NAME, WifiNative.SCAN_TYPE_HIGH_ACCURACY, SCAN_FREQ_SET,
                SCAN_HIDDEN_NETWORK_SSID_SET);
        verify(mWificondControl).scan(
                WIFI_IFACE_NAME, WifiNative.SCAN_TYPE_HIGH_ACCURACY,
                SCAN_FREQ_SET, SCAN_HIDDEN_NETWORK_SSID_SET);
    }

    /**
     * Verifies that startPnoscan() calls underlying WificondControl.
     */
    @Test
    public void testStartPnoScan() throws Exception {
        mWifiNative.startPnoScan(WIFI_IFACE_NAME, TEST_PNO_SETTINGS);
        verify(mWificondControl).startPnoScan(
                WIFI_IFACE_NAME, TEST_PNO_SETTINGS);
    }

    /**
     * Verifies that stopPnoscan() calls underlying WificondControl.
     */
    @Test
    public void testStopPnoScan() throws Exception {
        mWifiNative.stopPnoScan(WIFI_IFACE_NAME);
        verify(mWificondControl).stopPnoScan(WIFI_IFACE_NAME);
    }

    /**
     * Verifies that connectToNetwork() calls underlying WificondControl and SupplicantStaIfaceHal.
     */
    @Test
    public void testConnectToNetwork() throws Exception {
        WifiConfiguration config = mock(WifiConfiguration.class);
        mWifiNative.connectToNetwork(WIFI_IFACE_NAME, config);
        // connectToNetwork() should abort ongoing scan before connection.
        verify(mWificondControl).abortScan(WIFI_IFACE_NAME);
        verify(mStaIfaceHal).connectToNetwork(WIFI_IFACE_NAME, config);
    }

    /**
     * Verifies that roamToNetwork() calls underlying WificondControl and SupplicantStaIfaceHal.
     */
    @Test
    public void testRoamToNetwork() throws Exception {
        WifiConfiguration config = mock(WifiConfiguration.class);
        mWifiNative.roamToNetwork(WIFI_IFACE_NAME, config);
        // roamToNetwork() should abort ongoing scan before connection.
        verify(mWificondControl).abortScan(WIFI_IFACE_NAME);
        verify(mStaIfaceHal).roamToNetwork(WIFI_IFACE_NAME, config);
    }

    /**
     * Verifies that setMacAddress() calls underlying WifiVendorHal.
     */
    @Test
    public void testSetMacAddress() throws Exception {
        mWifiNative.setMacAddress(WIFI_IFACE_NAME, TEST_MAC_ADDRESS);
        verify(mWifiVendorHal).setMacAddress(WIFI_IFACE_NAME, TEST_MAC_ADDRESS);
    }

    /**
     * Test to check if the softap start failure metrics are incremented correctly.
     */
    @Test
    public void testStartSoftApFailureIncrementsMetrics() throws Exception {
        when(mWificondControl.startHostapd(any(), any())).thenReturn(false);
        WifiNative.SoftApListener mockListener = mock(WifiNative.SoftApListener.class);
        mWifiNative.startSoftAp(WIFI_IFACE_NAME, new WifiConfiguration(), mockListener);
        verify(mWificondControl).startHostapd(WIFI_IFACE_NAME, mockListener);
        verify(mWifiMetrics).incrementNumSetupSoftApInterfaceFailureDueToHostapd();
    }
}
