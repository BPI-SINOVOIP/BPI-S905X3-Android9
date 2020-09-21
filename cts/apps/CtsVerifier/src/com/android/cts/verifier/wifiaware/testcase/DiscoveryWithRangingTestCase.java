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
 * limitations under the License.
 */

package com.android.cts.verifier.wifiaware.testcase;

import android.content.Context;
import android.net.wifi.rtt.RangingRequest;
import android.net.wifi.rtt.RangingResult;
import android.net.wifi.rtt.WifiRttManager;
import android.util.Log;
import android.util.Pair;

import com.android.cts.verifier.R;
import com.android.cts.verifier.wifiaware.CallbackUtils;

import java.util.Arrays;
import java.util.List;

/**
 * Test case for Discovery + Ranging:
 *
 * Subscribe test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Subscribe
 *    wait for results (subscribe session)
 * 3. Wait for discovery with ranging
 * 4. Send message
 *    Wait for success
 * 5. Wait for message ("done with RTT")
 * 6. Directed Range to Peer with PeerHandle
 * 7. Directed Range to Peer with MAC address
 * 8. Send message ("done with RTT")
 *    Wait for success
 * LAST. Destroy session
 *
 * Publish test sequence:
 * 1. Attach
 *    wait for results (session)
 * 2. Publish
 *    wait for results (publish session)
 * 3. Wait for rx message
 * 4. Directed Range to Peer with PeerHandler
 * 5. Directed Range to Peer with MAC address
 * 6. Send message ("done with RTT")
 *    Wait for success
 * 7. Wait for Message ("done with RTT")
 * LAST. Destroy session
 *
 * Validate that measured range is "reasonable" (i.e. 0 <= X <= SPECIFIED_LARGE_RANGE).
 *
 * Note that the process tries to prevent RTT concurrency, each peer will execute RTT in
 * sequence.
 */
public class DiscoveryWithRangingTestCase extends DiscoveryBaseTestCase {
    private static final String TAG = "DiscWithRangingTestCase";
    private static final boolean DBG = true;

    private static final int NUM_RTT_ITERATIONS = 10;
    private static final int MAX_RTT_RANGING_SUCCESS = 5; // high: but open environment
    private static final int MIN_RSSI = -100;

    private static final byte[] MSG_RTT = "Done with RTT".getBytes();

    private WifiRttManager mWifiRttManager;
    private boolean mIsPublish;

    public DiscoveryWithRangingTestCase(Context context, boolean isPublish) {
        super(context, true, true);

        mWifiRttManager = (WifiRttManager) mContext.getSystemService(
                Context.WIFI_RTT_RANGING_SERVICE);
        mIsPublish = isPublish;
    }

    @Override
    protected boolean executeTest() throws InterruptedException {
        boolean status = executeTestLocal();

        // LAST. destroy session
        if (mWifiAwareDiscoverySession != null) {
            mWifiAwareDiscoverySession.close();
            mWifiAwareDiscoverySession = null;
        }

        return status;
    }

    /**
     * Actual test execution (another level of abstraction to guarantee clean-up code gets
     * executed by parent method).
     */
    private boolean executeTestLocal() throws InterruptedException {
        if (DBG) Log.d(TAG, "executeTest: mIsPublish=" + mIsPublish);

        if (mIsPublish) {
            if (!executePublish()) {
                return false;
            }
        } else {
            if (!executeSubscribe()) {
                return false;
            }
        }

        if (!mIsPublish) {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_waiting_for_peer_rtt));

            // pausing to let Publisher perform its RTT operations (however long)
            CallbackUtils.DiscoveryCb.CallbackData callbackData =
                    mDiscoveryCb.waitForCallbacksNoTimeout(
                            CallbackUtils.DiscoveryCb.ON_MESSAGE_RECEIVED);
            if (!Arrays.equals(MSG_RTT, callbackData.serviceSpecificInfo)) {
                setFailureReason(mContext.getString(R.string.aware_status_receive_failure));
                Log.e(TAG, "executeTest: receive RTT message content mismatch: rx=" + (
                    (callbackData.serviceSpecificInfo == null) ? "<null>"
                        : ("'" + new String(callbackData.serviceSpecificInfo) + "'")));
                return false;
            }

            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_received_peer_rtt_done));

            // wait a few seconds to allow the previous RTT session to teardown, semi-arbitrary
            Thread.sleep(5000);

            if (DBG) Log.d(TAG, "executeTest: subscriber received message - can proceed with RTT");
        }

        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_starting_rtt));

        // Directed range to peer with PeerHandler
        int numFailuresToPeerHandle = 0;
        RangingRequest rangeToPeerHandle = new RangingRequest.Builder().addWifiAwarePeer(
                mPeerHandle).build();
        for (int i = 0; i < NUM_RTT_ITERATIONS; ++i) {
            if (!executeRanging(rangeToPeerHandle)) {
                numFailuresToPeerHandle++;
            }
        }
        Log.d(TAG, "executeTest: Direct RTT to PeerHandle " + numFailuresToPeerHandle + " of "
                + NUM_RTT_ITERATIONS + " FAIL");
        if (numFailuresToPeerHandle > MAX_RTT_RANGING_SUCCESS) {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_ranging_peer_failure,
                            numFailuresToPeerHandle, NUM_RTT_ITERATIONS));
        } else {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_ranging_peer_success,
                            NUM_RTT_ITERATIONS - numFailuresToPeerHandle, NUM_RTT_ITERATIONS));
        }

        // Directed range to peer with MAC address
        int numFailuresToPeerMac = 0;
        RangingRequest rangeToMAC = new RangingRequest.Builder().addWifiAwarePeer(
                mPeerMacAddress).build();
        for (int i = 0; i < NUM_RTT_ITERATIONS; ++i) {
            if (!executeRanging(rangeToMAC)) {
                numFailuresToPeerMac++;
            }
        }
        Log.e(TAG, "executeTest: Direct RTT to MAC " + numFailuresToPeerMac + " of "
                + NUM_RTT_ITERATIONS + " FAIL");
        if (numFailuresToPeerMac > MAX_RTT_RANGING_SUCCESS) {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_ranging_mac_failure,
                            numFailuresToPeerMac, NUM_RTT_ITERATIONS));
        } else {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_ranging_mac_success,
                            NUM_RTT_ITERATIONS - numFailuresToPeerMac, NUM_RTT_ITERATIONS));
        }

        // let peer know we're done with our RTT
        mWifiAwareDiscoverySession.sendMessage(mPeerHandle, MESSAGE_ID + 2, MSG_RTT);
        CallbackUtils.DiscoveryCb.CallbackData callbackData = mDiscoveryCb.waitForCallbacks(
                CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_SUCCEEDED
                        | CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED);
        switch (callbackData.callback) {
            case CallbackUtils.DiscoveryCb.TIMEOUT:
                setFailureReason(mContext.getString(R.string.aware_status_send_timeout));
                Log.e(TAG, "executeTest: send message TIMEOUT");
                return false;
            case CallbackUtils.DiscoveryCb.ON_MESSAGE_SEND_FAILED:
                setFailureReason(mContext.getString(R.string.aware_status_send_failed));
                Log.e(TAG, "executeTest: send message ON_MESSAGE_SEND_FAILED");
                return false;
        }
        if (DBG) {
            Log.d(TAG, "executeTest: sent message to peer - I'm done with RTT");
        }

        if (mIsPublish) {
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_waiting_for_peer_rtt));

            // then pausing to let Subscriber finish it's RTT (can't terminate our Aware session
            // while we want Aware operations to proceed!).
            callbackData = mDiscoveryCb.waitForCallbacksNoTimeout(
                    CallbackUtils.DiscoveryCb.ON_MESSAGE_RECEIVED);
            if (!Arrays.equals(MSG_RTT, callbackData.serviceSpecificInfo)) {
                setFailureReason(mContext.getString(R.string.aware_status_receive_failure));
                Log.e(TAG, "executeTest: receive RTT message content mismatch: rx=" + (
                    (callbackData.serviceSpecificInfo == null) ? "<null>"
                        : ("'" + new String(callbackData.serviceSpecificInfo) + "'")));
                return false;
            }
            mListener.onTestMsgReceived(
                    mContext.getString(R.string.aware_status_received_peer_rtt_done));
            if (DBG) Log.d(TAG, "executeTest: publisher received message - can proceed (finish)");
        }

        if (numFailuresToPeerHandle > MAX_RTT_RANGING_SUCCESS
                || numFailuresToPeerMac > MAX_RTT_RANGING_SUCCESS) {
            setFailureReason(mContext.getString(R.string.aware_status_lifecycle_failed,
                    numFailuresToPeerHandle, NUM_RTT_ITERATIONS));
            return false;
        }

        mListener.onTestMsgReceived(mContext.getString(R.string.aware_status_lifecycle_ok));
        return true;
    }

    private boolean executeRanging(RangingRequest request) throws InterruptedException {
        CallbackUtils.RangingCb rangingCb = new CallbackUtils.RangingCb();
        mWifiRttManager.startRanging(request, command -> mHandler.post(command), rangingCb);
        Pair<Integer, List<RangingResult>> results = rangingCb.waitForRangingResults();
        switch (results.first) {
            case CallbackUtils.RangingCb.TIMEOUT:
                Log.e(TAG, "executeRanging: ranging to peer TIMEOUT");
                return false;
            case CallbackUtils.RangingCb.ON_FAILURE:
                Log.e(TAG, "executeRanging: ranging peer ON_FAILURE");
                return false;
        }
        if (results.second == null || results.second.size() != 1) {
            Log.e(TAG, "executeRanging: ranging peer invalid results - null, empty, or wrong length");
            return false;
        }
        RangingResult result = results.second.get(0);
        if (result.getStatus() != RangingResult.STATUS_SUCCESS) {
            Log.e(TAG, "executeRanging: ranging peer failed - individual result failure code = "
                    + result.getStatus());
            return false;
        }
        int distanceMm = result.getDistanceMm();
        int rssi = result.getRssi();

        if (distanceMm > LARGE_ENOUGH_DISTANCE || rssi < MIN_RSSI) {
            Log.e(TAG, "executeRanging: ranging peer failed - invalid results: result=" + result);
            return false;
        }

        Log.d(TAG, "executeRanging: result=" + result);
        return true;
    }
}
