/**
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License. You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
 * License for the specific language governing permissions and limitations
 * under the License.
 */

package android.app.usage.cts;

import android.app.AppOpsManager;
import android.app.usage.NetworkStatsManager;
import android.app.usage.NetworkStats;
import android.content.Context;
import android.content.pm.PackageManager;
import android.net.ConnectivityManager;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.TrafficStats;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.ParcelFileDescriptor;
import android.os.Process;
import android.os.RemoteException;
import android.platform.test.annotations.AppModeFull;
import android.telephony.TelephonyManager;
import android.test.InstrumentationTestCase;
import android.util.Log;
import com.android.compatibility.common.util.SystemUtil;

import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.URL;
import java.text.MessageFormat;
import java.util.ArrayList;
import java.util.Scanner;
import java.net.HttpURLConnection;

import libcore.io.IoUtils;
import libcore.io.Streams;

import static android.app.usage.NetworkStats.Bucket.DEFAULT_NETWORK_ALL;
import static android.app.usage.NetworkStats.Bucket.DEFAULT_NETWORK_NO;
import static android.app.usage.NetworkStats.Bucket.DEFAULT_NETWORK_YES;
import static android.app.usage.NetworkStats.Bucket.METERED_ALL;
import static android.app.usage.NetworkStats.Bucket.METERED_YES;
import static android.app.usage.NetworkStats.Bucket.METERED_NO;
import static android.app.usage.NetworkStats.Bucket.STATE_ALL;
import static android.app.usage.NetworkStats.Bucket.STATE_DEFAULT;
import static android.app.usage.NetworkStats.Bucket.STATE_FOREGROUND;
import static android.app.usage.NetworkStats.Bucket.TAG_NONE;
import static android.app.usage.NetworkStats.Bucket.UID_ALL;

public class NetworkUsageStatsTest extends InstrumentationTestCase {
    private static final String LOG_TAG = "NetworkUsageStatsTest";
    private static final String APPOPS_SET_SHELL_COMMAND = "appops set {0} {1} {2}";
    private static final String APPOPS_GET_SHELL_COMMAND = "appops get {0} {1}";

    private static final long MINUTE = 1000 * 60;
    private static final int TIMEOUT_MILLIS = 15000;

    private static final String CHECK_CONNECTIVITY_URL = "http://www.265.com/";
    private static final String CHECK_CALLBACK_URL = CHECK_CONNECTIVITY_URL;

    private static final int NETWORK_TAG = 0xf00d;
    private static final long THRESHOLD_BYTES = 2 * 1024 * 1024;  // 2 MB

    private abstract class NetworkInterfaceToTest {
        private boolean mMetered;
        private boolean mIsDefault;

        abstract int getNetworkType();
        abstract int getTransportType();

        public boolean getMetered() {
            return mMetered;
        }

        public void setMetered(boolean metered) {
            this.mMetered = metered;
        }

        public boolean getIsDefault() {
            return mIsDefault;
        }

        public void setIsDefault(boolean isDefault) {
            mIsDefault = isDefault;
        }

        abstract String getSystemFeature();
        abstract String getErrorMessage();
    }

    private final NetworkInterfaceToTest[] mNetworkInterfacesToTest =
            new NetworkInterfaceToTest[] {
                    new NetworkInterfaceToTest() {
                        @Override
                        public int getNetworkType() {
                            return ConnectivityManager.TYPE_WIFI;
                        }

                        @Override
                        public int getTransportType() {
                            return NetworkCapabilities.TRANSPORT_WIFI;
                        }

                        @Override
                        public String getSystemFeature() {
                            return PackageManager.FEATURE_WIFI;
                        }

                        @Override
                        public String getErrorMessage() {
                            return " Please make sure you are connected to a WiFi access point.";
                        }
                    },
                    new NetworkInterfaceToTest() {
                        @Override
                        public int getNetworkType() {
                            return ConnectivityManager.TYPE_MOBILE;
                        }

                        @Override
                        public int getTransportType() {
                            return NetworkCapabilities.TRANSPORT_CELLULAR;
                        }

                        @Override
                        public String getSystemFeature() {
                            return PackageManager.FEATURE_TELEPHONY;
                        }

                        @Override
                        public String getErrorMessage() {
                            return " Please make sure you have added a SIM card with data plan to" +
                                    " your phone, have enabled data over cellular and in case of" +
                                    " dual SIM devices, have selected the right SIM " +
                                    "for data connection.";
                        }
                    }
    };

    private String mPkg;
    private NetworkStatsManager mNsm;
    private ConnectivityManager mCm;
    private PackageManager mPm;
    private long mStartTime;
    private long mEndTime;

    private long mBytesRead;
    private String mWriteSettingsMode;
    private String mUsageStatsMode;

    private void exerciseRemoteHost(Network network, URL url) throws Exception {
        NetworkInfo networkInfo = mCm.getNetworkInfo(network);
        if (networkInfo == null) {
            Log.w(LOG_TAG, "Network info is null");
        } else {
            Log.w(LOG_TAG, "Network: " + networkInfo.toString());
        }
        InputStreamReader in = null;
        HttpURLConnection urlc = null;
        String originalKeepAlive = System.getProperty("http.keepAlive");
        System.setProperty("http.keepAlive", "false");
        try {
            TrafficStats.setThreadStatsTag(NETWORK_TAG);
            urlc = (HttpURLConnection) network.openConnection(url);
            urlc.setConnectTimeout(TIMEOUT_MILLIS);
            urlc.setUseCaches(false);
            // Disable compression so we generate enough traffic that assertWithinPercentage will
            // not be affected by the small amount of traffic (5-10kB) sent by the test harness.
            urlc.setRequestProperty("Accept-Encoding", "identity");
            urlc.connect();
            boolean ping = urlc.getResponseCode() == 200;
            if (ping) {
                in = new InputStreamReader(
                        (InputStream) urlc.getContent());

                mBytesRead = 0;
                while (in.read() != -1) ++mBytesRead;
            }
        } catch (Exception e) {
            Log.i(LOG_TAG, "Badness during exercising remote server: " + e);
        } finally {
            TrafficStats.clearThreadStatsTag();
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    // don't care
                }
            }
            if (urlc != null) {
                urlc.disconnect();
            }
            if (originalKeepAlive == null) {
                System.clearProperty("http.keepAlive");
            } else {
                System.setProperty("http.keepAlive", originalKeepAlive);
            }
        }
    }

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mNsm = (NetworkStatsManager) getInstrumentation().getContext()
                .getSystemService(Context.NETWORK_STATS_SERVICE);
        mNsm.setPollForce(true);

        mCm = (ConnectivityManager) getInstrumentation().getContext()
                .getSystemService(Context.CONNECTIVITY_SERVICE);

        mPm = getInstrumentation().getContext().getPackageManager();

        mPkg = getInstrumentation().getContext().getPackageName();

        mWriteSettingsMode = getAppOpsMode(AppOpsManager.OPSTR_WRITE_SETTINGS);
        setAppOpsMode(AppOpsManager.OPSTR_WRITE_SETTINGS, "allow");
        mUsageStatsMode = getAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS);
    }

    @Override
    protected void tearDown() throws Exception {
        if (mWriteSettingsMode != null) {
            setAppOpsMode(AppOpsManager.OPSTR_WRITE_SETTINGS, mWriteSettingsMode);
        }
        if (mUsageStatsMode != null) {
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, mUsageStatsMode);
        }
        super.tearDown();
    }

    private void setAppOpsMode(String appop, String mode) throws Exception {
        final String command = MessageFormat.format(APPOPS_SET_SHELL_COMMAND, mPkg, appop, mode);
        SystemUtil.runShellCommand(command);
    }

    private String getAppOpsMode(String appop) throws Exception {
        final String command = MessageFormat.format(APPOPS_GET_SHELL_COMMAND, mPkg, appop);
        String result = SystemUtil.runShellCommand(command);
        if (result == null) {
            Log.w(LOG_TAG, "App op " + appop + " could not be read.");
        }
        return result;
    }

    private boolean isInForeground() throws IOException {
        String result = SystemUtil.runShellCommand(getInstrumentation(),
                "cmd activity get-uid-state " + Process.myUid());
        return result.contains("FOREGROUND");
    }

    private class NetworkCallback extends ConnectivityManager.NetworkCallback {
        private long mTolerance;
        private URL mUrl;
        public boolean success;
        public boolean metered;
        public boolean isDefault;

        NetworkCallback(long tolerance, URL url) {
            mTolerance = tolerance;
            mUrl = url;
            success = false;
            metered = false;
            isDefault = false;
        }

        @Override
        public void onAvailable(Network network) {
            try {
                mStartTime = System.currentTimeMillis() - mTolerance;
                isDefault = network.equals(mCm.getActiveNetwork());
                exerciseRemoteHost(network, mUrl);
                mEndTime = System.currentTimeMillis() + mTolerance;
                success = true;
                metered = !mCm.getNetworkCapabilities(network)
                        .hasCapability(NetworkCapabilities.NET_CAPABILITY_NOT_METERED);
                synchronized(NetworkUsageStatsTest.this) {
                    NetworkUsageStatsTest.this.notify();
                }
            } catch (Exception e) {
                Log.w(LOG_TAG, "exercising remote host failed.", e);
                success = false;
            }
        }
    }

    private boolean shouldTestThisNetworkType(int networkTypeIndex, final long tolerance)
            throws Exception {
        boolean hasFeature = mPm.hasSystemFeature(
                mNetworkInterfacesToTest[networkTypeIndex].getSystemFeature());
        if (!hasFeature) {
            return false;
        }
        NetworkCallback callback = new NetworkCallback(tolerance, new URL(CHECK_CONNECTIVITY_URL));
        mCm.requestNetwork(new NetworkRequest.Builder()
                .addTransportType(mNetworkInterfacesToTest[networkTypeIndex].getTransportType())
                .addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET)
                .build(), callback);
        synchronized(this) {
            try {
                wait((int)(TIMEOUT_MILLIS * 1.2));
            } catch (InterruptedException e) {
            }
        }
        if (callback.success) {
            mNetworkInterfacesToTest[networkTypeIndex].setMetered(callback.metered);
            mNetworkInterfacesToTest[networkTypeIndex].setIsDefault(callback.isDefault);
            return true;
        }

        // This will always fail at this point as we know 'hasFeature' is true.
        assertFalse (mNetworkInterfacesToTest[networkTypeIndex].getSystemFeature() +
                " is a reported system feature, " +
                "however no corresponding connected network interface was found or the attempt " +
                "to connect has timed out (timeout = " + TIMEOUT_MILLIS + "ms)." +
                mNetworkInterfacesToTest[networkTypeIndex].getErrorMessage(), hasFeature);
        return false;
    }

    private String getSubscriberId(int networkIndex) {
        int networkType = mNetworkInterfacesToTest[networkIndex].getNetworkType();
        if (ConnectivityManager.TYPE_MOBILE == networkType) {
            TelephonyManager tm = (TelephonyManager) getInstrumentation().getContext()
                    .getSystemService(Context.TELEPHONY_SERVICE);
            return tm.getSubscriberId();
        }
        return "";
    }

    @AppModeFull
    public void testDeviceSummary() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            if (!shouldTestThisNetworkType(i, MINUTE/2)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats.Bucket bucket = null;
            try {
                bucket = mNsm.querySummaryForDevice(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
            } catch (RemoteException | SecurityException e) {
                fail("testDeviceSummary fails with exception: " + e.toString());
            }
            assertNotNull(bucket);
            assertTimestamps(bucket);
            assertEquals(bucket.getState(), STATE_ALL);
            assertEquals(bucket.getUid(), UID_ALL);
            assertEquals(bucket.getMetered(), METERED_ALL);
            assertEquals(bucket.getDefaultNetworkStatus(), DEFAULT_NETWORK_ALL);
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                bucket = mNsm.querySummaryForDevice(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                fail("negative testDeviceSummary fails: no exception thrown.");
            } catch (RemoteException e) {
                fail("testDeviceSummary fails with exception: " + e.toString());
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testUserSummary() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            if (!shouldTestThisNetworkType(i, MINUTE/2)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats.Bucket bucket = null;
            try {
                bucket = mNsm.querySummaryForUser(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
            } catch (RemoteException | SecurityException e) {
                fail("testUserSummary fails with exception: " + e.toString());
            }
            assertNotNull(bucket);
            assertTimestamps(bucket);
            assertEquals(bucket.getState(), STATE_ALL);
            assertEquals(bucket.getUid(), UID_ALL);
            assertEquals(bucket.getMetered(), METERED_ALL);
            assertEquals(bucket.getDefaultNetworkStatus(), DEFAULT_NETWORK_ALL);
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                bucket = mNsm.querySummaryForUser(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                fail("negative testUserSummary fails: no exception thrown.");
            } catch (RemoteException e) {
                fail("testUserSummary fails with exception: " + e.toString());
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testAppSummary() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            if (!shouldTestThisNetworkType(i, MINUTE/2)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats result = null;
            try {
                result = mNsm.querySummary(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                assertNotNull(result);
                NetworkStats.Bucket bucket = new NetworkStats.Bucket();
                long totalTxPackets = 0;
                long totalRxPackets = 0;
                long totalTxBytes = 0;
                long totalRxBytes = 0;
                boolean hasCorrectMetering = false;
                boolean hasCorrectDefaultStatus = false;
                int expectedMetering = mNetworkInterfacesToTest[i].getMetered() ?
                        METERED_YES : METERED_NO;
                int expectedDefaultStatus = mNetworkInterfacesToTest[i].getIsDefault() ?
                        DEFAULT_NETWORK_YES : DEFAULT_NETWORK_NO;
                while (result.hasNextBucket()) {
                    assertTrue(result.getNextBucket(bucket));
                    assertTimestamps(bucket);
                    hasCorrectMetering |= bucket.getMetered() == expectedMetering;
                    if (bucket.getUid() == Process.myUid()) {
                        totalTxPackets += bucket.getTxPackets();
                        totalRxPackets += bucket.getRxPackets();
                        totalTxBytes += bucket.getTxBytes();
                        totalRxBytes += bucket.getRxBytes();
                        hasCorrectDefaultStatus |=
                                bucket.getDefaultNetworkStatus() == expectedDefaultStatus;
                    }
                }
                assertFalse(result.getNextBucket(bucket));
                assertTrue("Incorrect metering for NetworkType: " +
                        mNetworkInterfacesToTest[i].getNetworkType(), hasCorrectMetering);
                assertTrue("Incorrect isDefault for NetworkType: " +
                        mNetworkInterfacesToTest[i].getNetworkType(), hasCorrectDefaultStatus);
                assertTrue("No Rx bytes usage for uid " + Process.myUid(), totalRxBytes > 0);
                assertTrue("No Rx packets usage for uid " + Process.myUid(), totalRxPackets > 0);
                assertTrue("No Tx bytes usage for uid " + Process.myUid(), totalTxBytes > 0);
                assertTrue("No Tx packets usage for uid " + Process.myUid(), totalTxPackets > 0);
            } finally {
                if (result != null) {
                    result.close();
                }
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                result = mNsm.querySummary(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                fail("negative testAppSummary fails: no exception thrown.");
            } catch (RemoteException e) {
                fail("testAppSummary fails with exception: " + e.toString());
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testAppDetails() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            // Relatively large tolerance to accommodate for history bucket size.
            if (!shouldTestThisNetworkType(i, MINUTE * 120)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats result = null;
            try {
                result = mNsm.queryDetails(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                long totalBytesWithSubscriberId = getTotalAndAssertNotEmpty(result);

                // Test without filtering by subscriberId
                result = mNsm.queryDetails(
                        mNetworkInterfacesToTest[i].getNetworkType(), null,
                        mStartTime, mEndTime);

                assertTrue("More bytes with subscriberId filter than without.",
                        getTotalAndAssertNotEmpty(result) >= totalBytesWithSubscriberId);
            } catch (RemoteException | SecurityException e) {
                fail("testAppDetails fails with exception: " + e.toString());
            } finally {
                if (result != null) {
                    result.close();
                }
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                result = mNsm.queryDetails(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime);
                fail("negative testAppDetails fails: no exception thrown.");
            } catch (RemoteException e) {
                fail("testAppDetails fails with exception: " + e.toString());
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testUidDetails() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            // Relatively large tolerance to accommodate for history bucket size.
            if (!shouldTestThisNetworkType(i, MINUTE * 120)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats result = null;
            try {
                result = mNsm.queryDetailsForUid(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime, Process.myUid());
                assertNotNull(result);
                NetworkStats.Bucket bucket = new NetworkStats.Bucket();
                long totalTxPackets = 0;
                long totalRxPackets = 0;
                long totalTxBytes = 0;
                long totalRxBytes = 0;
                while (result.hasNextBucket()) {
                    assertTrue(result.getNextBucket(bucket));
                    assertTimestamps(bucket);
                    assertEquals(bucket.getState(), STATE_ALL);
                    assertEquals(bucket.getMetered(), METERED_ALL);
                    assertEquals(bucket.getDefaultNetworkStatus(), DEFAULT_NETWORK_ALL);
                    assertEquals(bucket.getUid(), Process.myUid());
                    totalTxPackets += bucket.getTxPackets();
                    totalRxPackets += bucket.getRxPackets();
                    totalTxBytes += bucket.getTxBytes();
                    totalRxBytes += bucket.getRxBytes();
                }
                assertFalse(result.getNextBucket(bucket));
                assertTrue("No Rx bytes usage for uid " + Process.myUid(), totalRxBytes > 0);
                assertTrue("No Rx packets usage for uid " + Process.myUid(), totalRxPackets > 0);
                assertTrue("No Tx bytes usage for uid " + Process.myUid(), totalTxBytes > 0);
                assertTrue("No Tx packets usage for uid " + Process.myUid(), totalTxPackets > 0);
            } finally {
                if (result != null) {
                    result.close();
                }
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                result = mNsm.queryDetailsForUid(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime, Process.myUid());
                fail("negative testUidDetails fails: no exception thrown.");
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testTagDetails() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            // Relatively large tolerance to accommodate for history bucket size.
            if (!shouldTestThisNetworkType(i, MINUTE * 120)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats result = null;
            try {
                result = mNsm.queryDetailsForUidTag(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime, Process.myUid(), NETWORK_TAG);
                assertNotNull(result);
                NetworkStats.Bucket bucket = new NetworkStats.Bucket();
                long totalTxPackets = 0;
                long totalRxPackets = 0;
                long totalTxBytes = 0;
                long totalRxBytes = 0;
                while (result.hasNextBucket()) {
                    assertTrue(result.getNextBucket(bucket));
                    assertTimestamps(bucket);
                    assertEquals(bucket.getState(), STATE_ALL);
                    assertEquals(bucket.getMetered(), METERED_ALL);
                    assertEquals(bucket.getDefaultNetworkStatus(), DEFAULT_NETWORK_ALL);
                    assertEquals(bucket.getUid(), Process.myUid());
                    if (bucket.getTag() == NETWORK_TAG) {
                        totalTxPackets += bucket.getTxPackets();
                        totalRxPackets += bucket.getRxPackets();
                        totalTxBytes += bucket.getTxBytes();
                        totalRxBytes += bucket.getRxBytes();
                    }
                }
                assertTrue("No Rx bytes tagged with 0x" + Integer.toHexString(NETWORK_TAG)
                        + " for uid " + Process.myUid(), totalRxBytes > 0);
                assertTrue("No Rx packets tagged with 0x" + Integer.toHexString(NETWORK_TAG)
                        + " for uid " + Process.myUid(), totalRxPackets > 0);
                assertTrue("No Tx bytes tagged with 0x" + Integer.toHexString(NETWORK_TAG)
                        + " for uid " + Process.myUid(), totalTxBytes > 0);
                assertTrue("No Tx packets tagged with 0x" + Integer.toHexString(NETWORK_TAG)
                        + " for uid " + Process.myUid(), totalTxPackets > 0);
            } finally {
                if (result != null) {
                    result.close();
                }
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                result = mNsm.queryDetailsForUidTag(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime, Process.myUid(), NETWORK_TAG);
                fail("negative testUidDetails fails: no exception thrown.");
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    class QueryResult {
        public final int tag;
        public final int state;
        public final long total;

        public QueryResult(int tag, int state, NetworkStats stats) {
            this.tag = tag;
            this.state = state;
            total = getTotalAndAssertNotEmpty(stats, tag, state);
        }

        public String toString() {
            return String.format("QueryResult(tag=%s state=%s total=%d)",
                    tagToString(tag), stateToString(state), total);
        }
    }

    private NetworkStats getNetworkStatsForTagState(int i, int tag, int state) {
        return mNsm.queryDetailsForUidTagState(
                mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                mStartTime, mEndTime, Process.myUid(), tag, state);
    }

    private void assertWithinPercentage(String msg, long expected, long actual, int percentage) {
        long lowerBound = expected * (100 - percentage) / 100;
        long upperBound = expected * (100 + percentage) / 100;
        msg = String.format("%s: %d not within %d%% of %d", msg, actual, percentage, expected);
        assertTrue(msg, lowerBound <= actual);
        assertTrue(msg, upperBound >= actual);
    }

    private void assertAlmostNoUnexpectedTraffic(NetworkStats result, int expectedTag,
            int expectedState, long maxUnexpected) {
        long total = 0;
        NetworkStats.Bucket bucket = new NetworkStats.Bucket();
        while (result.hasNextBucket()) {
            assertTrue(result.getNextBucket(bucket));
            total += bucket.getRxBytes() + bucket.getTxBytes();
        }
        if (total <= maxUnexpected) return;

        fail(String.format("More than %d bytes of traffic when querying for "
                + "tag %s state %s. Last bucket: uid=%d tag=%s state=%s bytes=%d/%d",
                maxUnexpected, tagToString(expectedTag), stateToString(expectedState),
                bucket.getUid(), tagToString(bucket.getTag()), stateToString(bucket.getState()),
                bucket.getRxBytes(), bucket.getTxBytes()));
    }

    @AppModeFull
    public void testUidTagStateDetails() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            // Relatively large tolerance to accommodate for history bucket size.
            if (!shouldTestThisNetworkType(i, MINUTE * 120)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");
            NetworkStats result = null;
            try {
                int currentState = isInForeground() ? STATE_FOREGROUND : STATE_DEFAULT;
                int otherState = (currentState == STATE_DEFAULT) ? STATE_FOREGROUND : STATE_DEFAULT;

                int[] tagsWithTraffic = {NETWORK_TAG, TAG_NONE};
                int[] statesWithTraffic = {currentState, STATE_ALL};
                ArrayList<QueryResult> resultsWithTraffic = new ArrayList<>();

                int[] statesWithNoTraffic = {otherState};
                int[] tagsWithNoTraffic = {NETWORK_TAG + 1};
                ArrayList<QueryResult> resultsWithNoTraffic = new ArrayList<>();

                // Expect to see traffic when querying for any combination of a tag in
                // tagsWithTraffic and a state in statesWithTraffic.
                for (int tag : tagsWithTraffic) {
                    for (int state : statesWithTraffic) {
                        result = getNetworkStatsForTagState(i, tag, state);
                        resultsWithTraffic.add(new QueryResult(tag, state, result));
                        result.close();
                        result = null;
                    }
                }

                // Expect that the results are within a few percentage points of each other.
                // This is ensures that FIN retransmits after the transfer is complete don't cause
                // the test to be flaky. The test URL currently returns just over 100k so this
                // should not be too noisy. It also ensures that the traffic sent by the test
                // harness, which is untagged, won't cause a failure.
                long firstTotal = resultsWithTraffic.get(0).total;
                for (QueryResult queryResult : resultsWithTraffic) {
                    assertWithinPercentage(queryResult + "", firstTotal, queryResult.total, 10);
                }

                // Expect to see no traffic when querying for any tag in tagsWithNoTraffic or any
                // state in statesWithNoTraffic.
                for (int tag : tagsWithNoTraffic) {
                    for (int state : statesWithTraffic) {
                        result = getNetworkStatsForTagState(i, tag, state);
                        assertAlmostNoUnexpectedTraffic(result, tag, state, firstTotal / 100);
                        result.close();
                        result = null;
                    }
                }
                for (int tag : tagsWithTraffic) {
                    for (int state : statesWithNoTraffic) {
                        result = getNetworkStatsForTagState(i, tag, state);
                        assertAlmostNoUnexpectedTraffic(result, tag, state, firstTotal / 100);
                        result.close();
                        result = null;
                    }
                }
            } finally {
                if (result != null) {
                    result.close();
                }
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "deny");
            try {
                result = mNsm.queryDetailsForUidTag(
                        mNetworkInterfacesToTest[i].getNetworkType(), getSubscriberId(i),
                        mStartTime, mEndTime, Process.myUid(), NETWORK_TAG);
                fail("negative testUidDetails fails: no exception thrown.");
            } catch (SecurityException e) {
                // expected outcome
            }
        }
    }

    @AppModeFull
    public void testCallback() throws Exception {
        for (int i = 0; i < mNetworkInterfacesToTest.length; ++i) {
            // Relatively large tolerance to accommodate for history bucket size.
            if (!shouldTestThisNetworkType(i, MINUTE/2)) {
                continue;
            }
            setAppOpsMode(AppOpsManager.OPSTR_GET_USAGE_STATS, "allow");

            TestUsageCallback usageCallback = new TestUsageCallback();
            HandlerThread thread = new HandlerThread("callback-thread");
            thread.start();
            Handler handler = new Handler(thread.getLooper());
            mNsm.registerUsageCallback(mNetworkInterfacesToTest[i].getNetworkType(),
                    getSubscriberId(i), THRESHOLD_BYTES, usageCallback, handler);

            // TODO: Force traffic and check whether the callback is invoked.
            // Right now the test only covers whether the callback can be registered, but not
            // whether it is invoked upon data usage since we don't have a scalable way of
            // storing files of >2MB in CTS.

            mNsm.unregisterUsageCallback(usageCallback);
        }
    }

    private String tagToString(Integer tag) {
        if (tag == null) return "null";
        switch (tag) {
            case TAG_NONE:
                return "TAG_NONE";
            default:
                return "0x" + Integer.toHexString(tag);
        }
    }

    private String stateToString(Integer state) {
        if (state == null) return "null";
        switch (state) {
            case STATE_ALL:
                return "STATE_ALL";
            case STATE_DEFAULT:
                return "STATE_DEFAULT";
            case STATE_FOREGROUND:
                return "STATE_FOREGROUND";
        }
        throw new IllegalArgumentException("Unknown state " + state);
    }

    private long getTotalAndAssertNotEmpty(NetworkStats result, Integer expectedTag,
            Integer expectedState) {
        assertTrue(result != null);
        NetworkStats.Bucket bucket = new NetworkStats.Bucket();
        long totalTxPackets = 0;
        long totalRxPackets = 0;
        long totalTxBytes = 0;
        long totalRxBytes = 0;
        while (result.hasNextBucket()) {
            assertTrue(result.getNextBucket(bucket));
            assertTimestamps(bucket);
            if (expectedTag != null) assertEquals(bucket.getTag(), (int) expectedTag);
            if (expectedState != null) assertEquals(bucket.getState(), (int) expectedState);
            assertEquals(bucket.getMetered(), METERED_ALL);
            assertEquals(bucket.getDefaultNetworkStatus(), DEFAULT_NETWORK_ALL);
            if (bucket.getUid() == Process.myUid()) {
                totalTxPackets += bucket.getTxPackets();
                totalRxPackets += bucket.getRxPackets();
                totalTxBytes += bucket.getTxBytes();
                totalRxBytes += bucket.getRxBytes();
            }
        }
        assertFalse(result.getNextBucket(bucket));
        String msg = String.format("uid %d tag %s state %s",
                Process.myUid(), tagToString(expectedTag), stateToString(expectedState));
        assertTrue("No Rx bytes usage for " + msg, totalRxBytes > 0);
        assertTrue("No Rx packets usage for " + msg, totalRxPackets > 0);
        assertTrue("No Tx bytes usage for " + msg, totalTxBytes > 0);
        assertTrue("No Tx packets usage for " + msg, totalTxPackets > 0);

        return totalRxBytes + totalTxBytes;
    }

    private long getTotalAndAssertNotEmpty(NetworkStats result) {
        return getTotalAndAssertNotEmpty(result, null, STATE_ALL);
    }

    private void assertTimestamps(final NetworkStats.Bucket bucket) {
        assertTrue("Start timestamp " + bucket.getStartTimeStamp() + " is less than " +
                mStartTime, bucket.getStartTimeStamp() >= mStartTime);
        assertTrue("End timestamp " + bucket.getEndTimeStamp() + " is greater than " +
                mEndTime, bucket.getEndTimeStamp() <= mEndTime);
    }

    private static class TestUsageCallback extends NetworkStatsManager.UsageCallback {
        @Override
        public void onThresholdReached(int networkType, String subscriberId) {
            Log.v(LOG_TAG, "Called onThresholdReached for networkType=" + networkType
                    + " subscriberId=" + subscriberId);
        }
    }
}
