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
 * limitations under the License.
 */
package com.android.cts.deviceowner;

import android.app.admin.ConnectEvent;
import android.app.admin.DnsEvent;
import android.app.admin.NetworkEvent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.support.test.InstrumentationRegistry;
import androidx.localbroadcastmanager.content.LocalBroadcastManager;
import android.util.Log;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.net.HttpURLConnection;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.ServerSocket;
import java.net.Socket;
import java.net.URL;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class NetworkLoggingTest extends BaseDeviceOwnerTest {

    private static final String TAG = "NetworkLoggingTest";
    private static final String ARG_BATCH_COUNT = "batchCount";
    private static final int FAKE_BATCH_TOKEN = -666; // real batch tokens are always non-negative
    private static final int FULL_LOG_BATCH_SIZE = 1200;
    private static final String CTS_APP_PACKAGE_NAME = "com.android.cts.deviceowner";
    private static final int MAX_IP_ADDRESSES_LOGGED = 10;

    private static final String[] NOT_LOGGED_URLS_LIST = {
            "wikipedia.org",
            "google.pl"
    };

    private static final String[] LOGGED_URLS_LIST = {
            "example.com",
            "example.net",
            "example.org",
            "example.edu",
            "ipv6.google.com",
            "google.co.jp",
            "google.fr",
            "google.com.br",
            "google.com.tr",
            "google.co.uk",
            "google.de"
    };

    private final BroadcastReceiver mNetworkLogsReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (BasicAdminReceiver.ACTION_NETWORK_LOGS_AVAILABLE.equals(intent.getAction())) {
                final long token =
                        intent.getLongExtra(BasicAdminReceiver.EXTRA_NETWORK_LOGS_BATCH_TOKEN,
                                FAKE_BATCH_TOKEN);
                // Retrieve network logs.
                final List<NetworkEvent> events = mDevicePolicyManager.retrieveNetworkLogs(getWho(),
                        token);
                if (events == null) {
                    fail("Failed to retrieve batch of network logs with batch token " + token);
                    return;
                }
                if (mBatchCountDown.getCount() > 0) {
                    mNetworkEvents.addAll(events);
                }
                mBatchCountDown.countDown();
            }
        }
    };

    private CountDownLatch mBatchCountDown;
    private final ArrayList<NetworkEvent> mNetworkEvents = new ArrayList<>();
    private int mBatchesRequested = 1;

    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.setNetworkLoggingEnabled(getWho(), false);
        assertFalse(mDevicePolicyManager.isNetworkLoggingEnabled(getWho()));

        super.tearDown();
    }

    /**
     * Test: retrieving network logs can only be done if there's one user on the device or all
     * secondary users / profiles are affiliated.
     */
    public void testRetrievingNetworkLogsThrowsSecurityException() {
        mDevicePolicyManager.setNetworkLoggingEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isNetworkLoggingEnabled(getWho()));
        try {
            mDevicePolicyManager.retrieveNetworkLogs(getWho(), FAKE_BATCH_TOKEN);
            fail("did not throw expected SecurityException");
        } catch (SecurityException expected) {
        }
    }

    /**
     * Test: when a wrong batch token id (not a token of the current batch) is provided, null should
     * be returned.
     */
    public void testProvidingWrongBatchTokenReturnsNull() {
        mDevicePolicyManager.setNetworkLoggingEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isNetworkLoggingEnabled(getWho()));
        assertNull(mDevicePolicyManager.retrieveNetworkLogs(getWho(), FAKE_BATCH_TOKEN));
    }

    /**
     * Test: test that the actual logging happens when the network logging is enabled and doesn't
     * happen before it's enabled; for this test to work we need to generate enough internet
     * traffic, so that the batch of logs is created
     */
    public void testNetworkLoggingAndRetrieval() throws Exception {
        mBatchesRequested =
                Integer.parseInt(
                        InstrumentationRegistry.getArguments().getString(ARG_BATCH_COUNT, "1"));
        mBatchCountDown = new CountDownLatch(mBatchesRequested);
        // register a receiver that listens for DeviceAdminReceiver#onNetworkLogsAvailable()
        final IntentFilter filterNetworkLogsAvailable = new IntentFilter(
                BasicAdminReceiver.ACTION_NETWORK_LOGS_AVAILABLE);
        LocalBroadcastManager.getInstance(mContext).registerReceiver(mNetworkLogsReceiver,
                filterNetworkLogsAvailable);

        // visit websites that shouldn't be logged as network logging isn't enabled yet
        for (final String url : NOT_LOGGED_URLS_LIST) {
            connectToWebsite(url);
        }

        // enable network logging and start the logging scenario
        mDevicePolicyManager.setNetworkLoggingEnabled(getWho(), true);
        assertTrue(mDevicePolicyManager.isNetworkLoggingEnabled(getWho()));

        // TODO: here test that facts about logging are shown in the UI

        // Fetch and verify the batches of events.
        generateBatches();
    }

    private void generateBatches() throws Exception {
        // visit websites to verify their dns lookups are logged
        for (final String url : LOGGED_URLS_LIST) {
            connectToWebsite(url);
        }

        // generate enough traffic to fill the batches.
        int dummyReqNo = 0;
        for (int i = 0; i < mBatchesRequested; i++) {
            dummyReqNo += generateDummyTraffic();
        }

        // if DeviceAdminReceiver#onNetworkLogsAvailable() hasn't been triggered yet, wait for up to
        // 3 minutes per batch just in case
        final int timeoutMins = 3 * mBatchesRequested;
        mBatchCountDown.await(timeoutMins, TimeUnit.MINUTES);
        LocalBroadcastManager.getInstance(mContext).unregisterReceiver(mNetworkLogsReceiver);
        if (mBatchCountDown.getCount() > 0) {
            fail("Generated events for " + mBatchesRequested + " batches and waited for "
                    + timeoutMins + " minutes, but still didn't get"
                    + " DeviceAdminReceiver#onNetworkLogsAvailable() callback");
        }

        // Verify network logs.
        assertEquals("First event has the wrong id.", 0L, mNetworkEvents.get(0).getId());
        // For each of the real URLs we have two events: one DNS and one connect. Dummy requests
        // don't require DNS queries.
        final int eventsExpected =
                Math.min(FULL_LOG_BATCH_SIZE * mBatchesRequested,
                        2 * LOGGED_URLS_LIST.length + dummyReqNo);
        verifyNetworkLogs(mNetworkEvents, eventsExpected);
    }

    private void verifyNetworkLogs(List<NetworkEvent> networkEvents, int eventsExpected) {
        // allow a batch to be slightly smaller or larger.
        assertTrue(Math.abs(eventsExpected - networkEvents.size()) <= 50);
        int ctsPackageNameCounter = 0;
        // allow a small down margin for verification, to avoid flakiness
        final int eventsExpectedWithMargin = eventsExpected - 50;
        final boolean[] visited = new boolean[LOGGED_URLS_LIST.length];

        for (int i = 0; i < networkEvents.size(); i++) {
            final NetworkEvent currentEvent = networkEvents.get(i);
            // verify that the events are in chronological order
            if (i > 0) {
                assertTrue(currentEvent.getTimestamp() >= networkEvents.get(i - 1).getTimestamp());
            }
            // verify that the event IDs are monotonically increasing
            if (i > 0) {
                assertTrue(currentEvent.getId() == (networkEvents.get(i - 1).getId() + 1));
            }
            // count how many events come from the CTS app
            if (CTS_APP_PACKAGE_NAME.equals(currentEvent.getPackageName())) {
                ctsPackageNameCounter++;
                if (currentEvent instanceof DnsEvent) {
                    final DnsEvent dnsEvent = (DnsEvent) currentEvent;
                    // verify that we didn't log a hostname lookup when network logging was disabled
                    if (dnsEvent.getHostname().contains(NOT_LOGGED_URLS_LIST[0])
                            || dnsEvent.getHostname().contains(NOT_LOGGED_URLS_LIST[1])) {
                        fail("A hostname that was looked-up when network logging was disabled"
                                + " was logged.");
                    }
                    // count the frequencies of LOGGED_URLS_LIST's hostnames that were looked up
                    for (int j = 0; j < LOGGED_URLS_LIST.length; j++) {
                        if (dnsEvent.getHostname().contains(LOGGED_URLS_LIST[j])) {
                            visited[j] = true;
                            break;
                        }
                    }
                    // verify that as many IP addresses were logged as were reported (max 10)
                    final List<InetAddress> ips = dnsEvent.getInetAddresses();
                    assertTrue(ips.size() <= MAX_IP_ADDRESSES_LOGGED);
                    final int expectedAddressCount = Math.min(MAX_IP_ADDRESSES_LOGGED,
                            dnsEvent.getTotalResolvedAddressCount());
                    assertEquals(expectedAddressCount, ips.size());
                    // verify the IP addresses are valid IPv4 or IPv6 addresses
                    for (final InetAddress ipAddress : ips) {
                        assertTrue(isIpv4OrIpv6Address(ipAddress));
                    }
                } else if (currentEvent instanceof ConnectEvent) {
                    final ConnectEvent connectEvent = (ConnectEvent) currentEvent;
                    // verify the IP address is a valid IPv4 or IPv6 address
                    assertTrue(isIpv4OrIpv6Address(connectEvent.getInetAddress()));
                    // verify that the port is a valid port
                    assertTrue(connectEvent.getPort() >= 0 && connectEvent.getPort() <= 65535);
                } else {
                    fail("An unknown NetworkEvent type logged: "
                            + currentEvent.getClass().getName());
                }
            }
        }

        // verify that each hostname from LOGGED_URLS_LIST was looked-up
        for (int i = 0; i < 10; i++) {
            assertTrue(LOGGED_URLS_LIST[i] + " wasn't visited", visited[i]);
        }
        // verify that sufficient iterations done by the CTS app were logged
        assertTrue(ctsPackageNameCounter >= eventsExpectedWithMargin);
    }

    private void connectToWebsite(String server) {
        HttpURLConnection urlConnection = null;
        try {
            final URL url = new URL("http://" + server);
            urlConnection = (HttpURLConnection) url.openConnection();
            urlConnection.setConnectTimeout(2000);
            urlConnection.setReadTimeout(2000);
            urlConnection.getResponseCode();
        } catch (IOException e) {
            Log.w(TAG, "Failed to connect to " + server, e);
        } finally {
            if (urlConnection != null) {
                urlConnection.disconnect();
            }
        }
    }

    /** Quickly generate loads of events by repeatedly connecting to a local server. */
    private int generateDummyTraffic() throws IOException, InterruptedException {
        final ServerSocket serverSocket = new ServerSocket(0);
        final Thread serverThread = startDummyServer(serverSocket);

        final int reqNo = makeDummyRequests(serverSocket.getLocalPort());

        serverSocket.close();
        serverThread.join();

        return reqNo;
    }

    private int makeDummyRequests(int port) {
        int reqNo;
        final String DUMMY_SERVER = "127.0.0.1:" + port;
        for (reqNo = 0; reqNo < FULL_LOG_BATCH_SIZE && mBatchCountDown.getCount() > 0; reqNo++) {
            connectToWebsite(DUMMY_SERVER);
            try {
                // Just to prevent choking the server.
                Thread.sleep(10);
            } catch (InterruptedException e) {
                Thread.currentThread().interrupt();
            }
        }
        return reqNo;
    }

    private Thread startDummyServer(ServerSocket serverSocket) throws InterruptedException {
        final Thread serverThread = new Thread(() -> {
            while (!serverSocket.isClosed()) {
                try {
                    final Socket socket = serverSocket.accept();
                    // Consume input.
                    final BufferedReader input =
                            new BufferedReader(new InputStreamReader(socket.getInputStream()));
                    String line;
                    do {
                        line = input.readLine();
                    } while (line != null && !line.equals(""));
                    // Return minimum valid response.
                    final PrintStream output = new PrintStream(socket.getOutputStream());
                    output.println("HTTP/1.0 200 OK");
                    output.println("Content-Length: 0");
                    output.println();
                    output.flush();
                    output.close();
                } catch (IOException e) {
                    if (!serverSocket.isClosed()) {
                        Log.w(TAG, "Failed to serve connection", e);
                    }
                }
            }
        });
        serverThread.start();

        // Allow the server to start accepting.
        Thread.sleep(1_000);

        return serverThread;
    }

    private boolean isIpv4OrIpv6Address(InetAddress addr) {
        return ((addr instanceof Inet4Address) || (addr instanceof Inet6Address));
    }
}
