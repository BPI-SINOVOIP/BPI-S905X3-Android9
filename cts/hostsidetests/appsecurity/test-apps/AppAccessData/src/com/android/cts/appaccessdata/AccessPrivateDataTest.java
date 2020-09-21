/*
 * Copyright (C) 2009 The Android Open Source Project
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

package com.android.cts.appaccessdata;

import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.Bundle;
import android.os.ParcelFileDescriptor;
import android.os.SystemClock;
import android.test.AndroidTestCase;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;

/**
 * Test that another app's private data cannot be accessed, while its public data can.
 *
 * Assumes that {@link #APP_WITH_DATA_PKG} has already created the private and public data.
 */
public class AccessPrivateDataTest extends AndroidTestCase {

    /**
     * The Android package name of the application that owns the data
     */
    private static final String APP_WITH_DATA_PKG = "com.android.cts.appwithdata";

    /**
     * Name of private file to access. This must match the name of the file created by
     * {@link #APP_WITH_DATA_PKG}.
     */
    private static final String PRIVATE_FILE_NAME = "private_file.txt";
    /**
     * Name of public file to access. This must match the name of the file created by
     * {@link #APP_WITH_DATA_PKG}.
     */
    private static final String PUBLIC_FILE_NAME = "public_file.txt";

    private static final String QTAGUID_STATS_FILE = "/proc/net/xt_qtaguid/stats";

    private static final Uri PRIVATE_TARGET = Uri.parse("content://com.android.cts.appwithdata/");

    /**
     * Tests that another app's private data cannot be accessed. It includes file
     * and detailed traffic stats.
     * @throws IOException
     */
    public void testAccessPrivateData() throws IOException {
        try {
            // construct the absolute file path to the app's private file
            ApplicationInfo applicationInfo = getApplicationInfo(APP_WITH_DATA_PKG);
            File privateFile = new File(applicationInfo.dataDir, "files/" + PRIVATE_FILE_NAME);
            FileInputStream inputStream = new FileInputStream(privateFile);
            inputStream.read();
            inputStream.close();
            fail("Was able to access another app's private data");
        } catch (FileNotFoundException | SecurityException e) {
            // expected
        }
    }

    private ApplicationInfo getApplicationInfo(String packageName) {
        try {
            return mContext.getPackageManager().getApplicationInfo(packageName, 0);
        } catch (PackageManager.NameNotFoundException e) {
            throw new IllegalStateException("Expected package not found: " + e);
        }
    }

    /**
     * Tests that another app's public file can be accessed
     * @throws IOException
     */
    public void testAccessPublicData() throws IOException {
        try {
            // construct the absolute file path to the other app's public file
            ApplicationInfo applicationInfo = getApplicationInfo(APP_WITH_DATA_PKG);
            File publicFile = new File(applicationInfo.dataDir, "files/" + PUBLIC_FILE_NAME);
            FileInputStream inputStream = new FileInputStream(publicFile);
            inputStream.read();
            inputStream.close();
            fail("Was able to access another app's public file");
        } catch (FileNotFoundException | SecurityException e) {
            // expected
        }
    }

    public void testAccessProcQtaguidTrafficStatsFailed() {
        // For untrusted app with SDK P or above, proc/net/xt_qtaguid files are no long readable.
        // They can only read their own stats from TrafficStats API. The test for TrafficStats API
        // will ensure they cannot read other apps stats.
        assertFalse("untrusted app should not be able to read qtaguid profile",
            new File(QTAGUID_STATS_FILE).canRead());
    }

    public void testAccessPrivateTrafficStats() {
        int otherAppUid = -1;
        try {
            otherAppUid = getContext()
                    .createPackageContext(APP_WITH_DATA_PKG, 0 /*flags*/)
                    .getApplicationInfo().uid;
        } catch (NameNotFoundException e) {
            fail("Was not able to find other app");
        }

        final int UNSUPPORTED = -1;
        assertEquals(UNSUPPORTED, TrafficStats.getUidRxBytes(otherAppUid));
        assertEquals(UNSUPPORTED, TrafficStats.getUidRxPackets(otherAppUid));
        assertEquals(UNSUPPORTED, TrafficStats.getUidTxBytes(otherAppUid));
        assertEquals(UNSUPPORTED, TrafficStats.getUidTxPackets(otherAppUid));
    }

    public void testTrafficStatsStatsUidSelf() throws Exception {
        final int uid = android.os.Process.myUid();
        final long rxb = TrafficStats.getUidRxBytes(uid);
        final long rxp = TrafficStats.getUidRxPackets(uid);
        final long txb = TrafficStats.getUidTxBytes(uid);
        final long txp = TrafficStats.getUidTxPackets(uid);

        // Start remote server
        final int port = getContext().getContentResolver().call(PRIVATE_TARGET, "start", null, null)
                .getInt("port");

        // Try talking to them, but shift blame
        try {
            final Socket socket = new Socket();
            socket.setTcpNoDelay(true);

            Bundle extras = new Bundle();
            extras.putParcelable("fd", ParcelFileDescriptor.fromSocket(socket));
            getContext().getContentResolver().call(PRIVATE_TARGET, "tag", null, extras);

            socket.connect(new InetSocketAddress("localhost", port));

            socket.getOutputStream().write(42);
            socket.getOutputStream().flush();
            final int val = socket.getInputStream().read();
            assertEquals(42, val);
            socket.close();
        } catch (IOException e) {
            throw new RuntimeException(e);
        } finally {
            getContext().getContentResolver().call(PRIVATE_TARGET, "stop", null, null);
        }

        SystemClock.sleep(1000);

        // Since we shifted blame, our stats shouldn't have changed
        assertEquals(rxb, TrafficStats.getUidRxBytes(uid));
        assertEquals(rxp, TrafficStats.getUidRxPackets(uid));
        assertEquals(txb, TrafficStats.getUidTxBytes(uid));
        assertEquals(txp, TrafficStats.getUidTxPackets(uid));
    }
}
