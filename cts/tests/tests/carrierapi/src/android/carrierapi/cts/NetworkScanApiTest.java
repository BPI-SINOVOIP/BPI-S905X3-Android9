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
package android.carrierapi.cts;

import android.content.Context;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Parcel;
import android.support.test.InstrumentationRegistry;
import android.support.test.runner.AndroidJUnit4;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.NetworkScan;
import android.telephony.NetworkScanRequest;
import android.telephony.RadioAccessSpecifier;
import android.telephony.AccessNetworkConstants;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyScanManager;
import android.util.Log;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotSame;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

/**
 * Build, install and run the tests by running the commands below:
 *  make cts -j64
 *  cts-tradefed run cts -m CtsCarrierApiTestCases --test android.carrierapi.cts.NetworkScanApiTest
 */
@RunWith(AndroidJUnit4.class)
public class NetworkScanApiTest {
    private TelephonyManager mTelephonyManager;
    private PackageManager mPackageManager;
    private static final String TAG = "NetworkScanApiTest";
    private int mNetworkScanStatus;
    private static final int EVENT_NETWORK_SCAN_START = 100;
    private static final int EVENT_NETWORK_SCAN_RESULTS = 200;
    private static final int EVENT_NETWORK_SCAN_ERROR = 300;
    private static final int EVENT_NETWORK_SCAN_COMPLETED = 400;
    private List<CellInfo> mScanResults = null;
    private NetworkScanHandlerThread mTestHandlerThread;
    private Handler mHandler;
    private NetworkScan mNetworkScan;
    private NetworkScanRequest mNetworkScanRequest;
    private NetworkScanCallbackImpl mNetworkScanCallback;
    private static final int MAX_INIT_WAIT_MS = 60000; // 60 seconds
    private Object mLock = new Object();
    private boolean mReady;
    private int mErrorCode;
    /* All the following constants are used to construct NetworkScanRequest*/
    private static final int SCAN_TYPE = NetworkScanRequest.SCAN_TYPE_ONE_SHOT;
    private static final boolean INCREMENTAL_RESULTS = true;
    private static final int SEARCH_PERIODICITY_SEC = 5;
    private static final int MAX_SEARCH_TIME_SEC = 300;
    private static final int INCREMENTAL_RESULTS_PERIODICITY_SEC = 3;
    private static final ArrayList<String> MCC_MNC = new ArrayList<>();
    private static final RadioAccessSpecifier[] RADIO_ACCESS_SPECIFIERS = {
            new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.GERAN,
                    null /* bands */,
                    null /* channels */),
            new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.EUTRAN,
                    null /* bands */,
                    null /* channels */),
            new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.UTRAN,
                    null /* bands */,
                    null /* channels */)
    };

    @Before
    public void setUp() throws Exception {
        mTelephonyManager = (TelephonyManager)
                InstrumentationRegistry.getContext().getSystemService(Context.TELEPHONY_SERVICE);
        mPackageManager = InstrumentationRegistry.getContext().getPackageManager();
        mTestHandlerThread = new NetworkScanHandlerThread(TAG);
        mTestHandlerThread.start();
    }

    @After
    public void tearDown() throws Exception {
        mTestHandlerThread.quit();
    }

    private void waitUntilReady() {
        synchronized (mLock) {
            try {
                mLock.wait(MAX_INIT_WAIT_MS);
            } catch (InterruptedException ie) {
            }

            if (!mReady) {
                fail("NetworkScanApiTest failed to initialize");
            }
        }
    }

    private void setReady(boolean ready) {
        synchronized (mLock) {
            mReady = ready;
            mLock.notifyAll();
        }
    }

    private class NetworkScanHandlerThread extends HandlerThread {

        public NetworkScanHandlerThread(String name) {
            super(name);
        }

        @Override
        public void onLooperPrepared() {
            /* create a custom handler for the Handler Thread */
            mHandler = new Handler(mTestHandlerThread.getLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case EVENT_NETWORK_SCAN_START:
                            Log.d(TAG, "request network scan");
                            mNetworkScan = mTelephonyManager.requestNetworkScan(
                                    mNetworkScanRequest, mNetworkScanCallback);
                            break;
                        default:
                            Log.d(TAG, "Unknown Event " + msg.what);
                    }
                }
            };
        }
    }

    private class NetworkScanCallbackImpl extends TelephonyScanManager.NetworkScanCallback {
        @Override
        public void onResults(List<CellInfo> results) {
            Log.d(TAG, "onResults: " + results.toString());
            mNetworkScanStatus = EVENT_NETWORK_SCAN_RESULTS;
            mScanResults = results;
        }

        @Override
        public void onComplete() {
            Log.d(TAG, "onComplete");
            mNetworkScanStatus = EVENT_NETWORK_SCAN_COMPLETED;
            setReady(true);
        }

        @Override
        public void onError(int error) {
            Log.d(TAG, "onError: " + String.valueOf(error));
            mNetworkScanStatus = EVENT_NETWORK_SCAN_ERROR;
            mErrorCode = error;
            Log.d(TAG, "Stop the network scan");
            mNetworkScan.stopScan();
            setReady(true);
        }
    }

    private RadioAccessSpecifier getRadioAccessSpecifier(CellInfo cellInfo) {
        RadioAccessSpecifier ras;
        if (cellInfo instanceof CellInfoLte) {
            int ranLte = AccessNetworkConstants.AccessNetworkType.EUTRAN;
            int[] lteChannels = {((CellInfoLte) cellInfo).getCellIdentity().getEarfcn()};
            ras = new RadioAccessSpecifier(ranLte, null /* bands */, lteChannels);
        } else if (cellInfo instanceof CellInfoWcdma) {
            int ranLte = AccessNetworkConstants.AccessNetworkType.UTRAN;
            int[] wcdmaChannels = {((CellInfoWcdma) cellInfo).getCellIdentity().getUarfcn()};
            ras = new RadioAccessSpecifier(ranLte, null /* bands */, wcdmaChannels);
        } else if (cellInfo instanceof CellInfoGsm) {
            int ranGsm = AccessNetworkConstants.AccessNetworkType.GERAN;
            int[] gsmChannels = {((CellInfoGsm) cellInfo).getCellIdentity().getArfcn()};
            ras = new RadioAccessSpecifier(ranGsm, null /* bands */, gsmChannels);
        } else {
            ras = null;
        }
        return ras;
    }

    /**
     * Tests that the device properly requests a network scan.
     */
    @Test
    public void testRequestNetworkScan() throws InterruptedException {
        if (!mPackageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            // Checks whether the cellular stack should be running on this device.
            Log.e(TAG, "No cellular support, the test will be skipped.");
            return;
        }
        if (!mTelephonyManager.hasCarrierPrivileges()) {
            fail("This test requires a SIM card with carrier privilege rule on it.");
        }

        // Make sure that there should be at least one entry.
        List<CellInfo> allCellInfo = mTelephonyManager.getAllCellInfo();
        if (allCellInfo == null) {
            fail("TelephonyManager.getAllCellInfo() returned NULL!");
        }
        if (allCellInfo.size() == 0) {
            fail("TelephonyManager.getAllCellInfo() returned zero-length list!");
        }

        // Construct a NetworkScanRequest
        List<RadioAccessSpecifier> radioAccessSpecifier = new ArrayList<>();
        for (int i = 0; i < allCellInfo.size(); i++) {
            RadioAccessSpecifier ras = getRadioAccessSpecifier(allCellInfo.get(i));
            if (ras != null) {
                radioAccessSpecifier.add(ras);
            }
        }
        if (radioAccessSpecifier.size() == 0) {
            RadioAccessSpecifier gsm = new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.GERAN,
                    null /* bands */,
                    null /* channels */);
            RadioAccessSpecifier lte = new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.EUTRAN,
                    null /* bands */,
                    null /* channels */);
            RadioAccessSpecifier wcdma = new RadioAccessSpecifier(
                    AccessNetworkConstants.AccessNetworkType.UTRAN,
                    null /* bands */,
                    null /* channels */);
            radioAccessSpecifier.add(gsm);
            radioAccessSpecifier.add(lte);
            radioAccessSpecifier.add(wcdma);
        }
        RadioAccessSpecifier[] radioAccessSpecifierArray =
                new RadioAccessSpecifier[radioAccessSpecifier.size()];
        mNetworkScanRequest = new NetworkScanRequest(
                NetworkScanRequest.SCAN_TYPE_ONE_SHOT /* scan type */,
                radioAccessSpecifier.toArray(radioAccessSpecifierArray),
                5 /* search periodicity */,
                60 /* max search time */,
                true /*enable incremental results*/,
                5 /* incremental results periodicity */,
                null /* List of PLMN ids (MCC-MNC) */);

        mNetworkScanCallback = new NetworkScanCallbackImpl();
        Message startNetworkScan = mHandler.obtainMessage(EVENT_NETWORK_SCAN_START);
        setReady(false);
        startNetworkScan.sendToTarget();
        waitUntilReady();

        Log.d(TAG, "mNetworkScanStatus: " + mNetworkScanStatus);
        assertTrue("The final scan status is not ScanCompleted or ScanError with an error "
                        + "code ERROR_MODEM_UNAVAILABLE or ERROR_UNSUPPORTED",
                isScanStatusValid());
    }

    private boolean isScanStatusValid() {
        // TODO(b/72162885): test the size of ScanResults is not zero after the blocking bug fixed.
        if ((mNetworkScanStatus == EVENT_NETWORK_SCAN_COMPLETED) && (mScanResults != null)) {
            // Scan complete.
            return true;
        }
        if ((mNetworkScanStatus == EVENT_NETWORK_SCAN_ERROR)
                && ((mErrorCode == NetworkScan.ERROR_MODEM_UNAVAILABLE)
                || (mErrorCode == NetworkScan.ERROR_UNSUPPORTED))) {
            // Scan error but the error type is allowed.
            return true;
        }
        return false;
    }

    private ArrayList<String> getPlmns() {
        ArrayList<String> mccMncs = new ArrayList<>();
        mccMncs.add("310260");
        mccMncs.add("310120");
        return mccMncs;
    }

    /**
     * To test its constructor and getters.
     */
    @Test
    public void testNetworkScanRequest_ConstructorAndGetters(){
        NetworkScanRequest networkScanRequest = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        assertEquals("getScanType() returns wrong value",
                SCAN_TYPE, networkScanRequest.getScanType());
        assertEquals("getSpecifiers() returns wrong value",
                RADIO_ACCESS_SPECIFIERS, networkScanRequest.getSpecifiers());
        assertEquals("getSearchPeriodicity() returns wrong value",
                SEARCH_PERIODICITY_SEC, networkScanRequest.getSearchPeriodicity());
        assertEquals("getMaxSearchTime() returns wrong value",
                MAX_SEARCH_TIME_SEC, networkScanRequest.getMaxSearchTime());
        assertEquals("getIncrementalResults() returns wrong value",
                INCREMENTAL_RESULTS, networkScanRequest.getIncrementalResults());
        assertEquals("getIncrementalResultsPeriodicity() returns wrong value",
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                networkScanRequest.getIncrementalResultsPeriodicity());
        assertEquals("getPlmns() returns wrong value", getPlmns(), networkScanRequest.getPlmns());
        assertEquals("describeContents() returns wrong value",
                0, networkScanRequest.describeContents());
    }

    /**
     * To test its hashCode method.
     */
    @Test
    public void testNetworkScanRequestParcel_Hashcode() {
        NetworkScanRequest networkScanRequest1 = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        NetworkScanRequest networkScanRequest2 = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        NetworkScanRequest networkScanRequest3 = new NetworkScanRequest(
                SCAN_TYPE,
                null,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                false,
                0,
                getPlmns());

        assertEquals("hashCode() returns different hash code for same objects",
                networkScanRequest1.hashCode(), networkScanRequest2.hashCode());
        assertNotSame("hashCode() returns same hash code for different objects",
                networkScanRequest1.hashCode(), networkScanRequest3.hashCode());
    }

    /**
     * To test its comparision method.
     */
    @Test
    public void testNetworkScanRequestParcel_Equals() {
        NetworkScanRequest networkScanRequest1 = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        NetworkScanRequest networkScanRequest2 = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        assertTrue(networkScanRequest1.equals(networkScanRequest2));

        networkScanRequest2 = new NetworkScanRequest(
                SCAN_TYPE,
                RADIO_ACCESS_SPECIFIERS,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                null /* List of PLMN ids (MCC-MNC) */);
        assertFalse(networkScanRequest1.equals(networkScanRequest2));
    }

    /**
     * To test its writeToParcel and createFromParcel methods.
     */
    @Test
    public void testNetworkScanRequestParcel_Parcel() {
        NetworkScanRequest networkScanRequest = new NetworkScanRequest(
                SCAN_TYPE,
                null /* Radio Access Specifier */,
                SEARCH_PERIODICITY_SEC,
                MAX_SEARCH_TIME_SEC,
                INCREMENTAL_RESULTS,
                INCREMENTAL_RESULTS_PERIODICITY_SEC,
                getPlmns());

        Parcel p = Parcel.obtain();
        networkScanRequest.writeToParcel(p, 0);
        p.setDataPosition(0);
        NetworkScanRequest newnsr = NetworkScanRequest.CREATOR.createFromParcel(p);
        assertTrue(networkScanRequest.equals(newnsr));

    }
}
