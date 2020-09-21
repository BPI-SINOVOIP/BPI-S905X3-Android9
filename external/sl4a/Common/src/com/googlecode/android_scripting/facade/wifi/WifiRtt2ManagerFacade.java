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

package com.googlecode.android_scripting.facade.wifi;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.net.MacAddress;
import android.net.wifi.aware.PeerHandle;
import android.net.wifi.rtt.RangingRequest;
import android.net.wifi.rtt.RangingResult;
import android.net.wifi.rtt.RangingResultCallback;
import android.net.wifi.rtt.WifiRttManager;
import android.os.Bundle;
import android.os.Parcelable;
import android.os.RemoteException;
import android.os.WorkSource;

import com.android.internal.annotations.GuardedBy;

import libcore.util.HexEncoding;

import com.googlecode.android_scripting.facade.EventFacade;
import com.googlecode.android_scripting.facade.FacadeManager;
import com.googlecode.android_scripting.jsonrpc.RpcReceiver;
import com.googlecode.android_scripting.rpc.Rpc;
import com.googlecode.android_scripting.rpc.RpcOptional;
import com.googlecode.android_scripting.rpc.RpcParameter;

import org.json.JSONArray;
import org.json.JSONException;

import java.util.List;

/**
 * Facade for RTTv2 manager.
 */
public class WifiRtt2ManagerFacade extends RpcReceiver {
    private final Service mService;
    private final EventFacade mEventFacade;
    private final StateChangedReceiver mStateChangedReceiver;

    private final Object mLock = new Object(); // lock access to the following vars

    @GuardedBy("mLock")
    private WifiRttManager mMgr;

    @GuardedBy("mLock")
    private int mNextRangingResultCallbackId = 1;

    public WifiRtt2ManagerFacade(FacadeManager manager) {
        super(manager);
        mService = manager.getService();
        mEventFacade = manager.getReceiver(EventFacade.class);

        mMgr = (WifiRttManager) mService.getSystemService(Context.WIFI_RTT_RANGING_SERVICE);

        mStateChangedReceiver = new StateChangedReceiver();
        IntentFilter filter = new IntentFilter(WifiRttManager.ACTION_WIFI_RTT_STATE_CHANGED);
        mService.registerReceiver(mStateChangedReceiver, filter);
    }

    @Override
    public void shutdown() {
        // empty
    }

    @Rpc(description = "Does the device support the Wi-Fi RTT feature?")
    public Boolean doesDeviceSupportWifiRttFeature() {
        return mService.getPackageManager().hasSystemFeature(PackageManager.FEATURE_WIFI_RTT);
    }

    @Rpc(description = "Is Wi-Fi RTT Usage Enabled?")
    public Boolean wifiIsRttAvailable() throws RemoteException {
        synchronized (mLock) {
            return mMgr.isAvailable();
        }
    }

    @Rpc(description = "The maximum number of peers permitted in a single RTT request")
    public Integer wifiRttMaxPeersInRequest() {
      return RangingRequest.getMaxPeers();
    }

    /**
     * Start Wi-Fi RTT ranging to an Acccess Point using the given scan results. Returns the id
     * associated with the listener used for ranging. The ranging result event will be decorated
     * with the listener id.
     */
    @Rpc(description = "Start ranging to an Access Points.", returns = "Id of the listener "
            + "associated with the started ranging.")
    public Integer wifiRttStartRangingToAccessPoints(
            @RpcParameter(name = "scanResults") JSONArray scanResults,
            @RpcParameter(name = "uidsOverride", description = "Overrides for the UID")
                @RpcOptional JSONArray uidsOverride) throws JSONException {
        synchronized (mLock) {
            int id = mNextRangingResultCallbackId++;
            RangingResultCallback callback = new RangingResultCallbackFacade(id);
            mMgr.startRanging(getWorkSource(uidsOverride),
                    new RangingRequest.Builder().addAccessPoints(
                            WifiJsonParser.getScanResults(scanResults)).build(),
                    mService.getMainExecutor(), callback);
            return id;
        }
    }

    @Rpc(description = "Start ranging to an Aware peer.", returns = "Id of the listener "
            + "associated with the started ranging.")
    public Integer wifiRttStartRangingToAwarePeerMac(
            @RpcParameter(name = "peerMac") String peerMac,
            @RpcParameter(name = "uidsOverride", description = "Overrides for the UID")
            @RpcOptional JSONArray uidsOverride) throws JSONException {
        synchronized (mLock) {
            int id = mNextRangingResultCallbackId++;
            RangingResultCallback callback = new RangingResultCallbackFacade(id);
            byte[] peerMacBytes = HexEncoding.decode(peerMac); // since Aware returns string w/o ":"
            mMgr.startRanging(getWorkSource(uidsOverride),
                    new RangingRequest.Builder().addWifiAwarePeer(
                            MacAddress.fromBytes(peerMacBytes)).build(), mService.getMainExecutor(),
                    callback);
            return id;
        }
    }

    @Rpc(description = "Start ranging to an Aware peer.", returns = "Id of the listener "
            + "associated with the started ranging.")
    public Integer wifiRttStartRangingToAwarePeerId(
            @RpcParameter(name = "peer ID") Integer peerId,
            @RpcParameter(name = "uidsOverride", description = "Overrides for the UID")
            @RpcOptional JSONArray uidsOverride) throws JSONException {
        synchronized (mLock) {
            int id = mNextRangingResultCallbackId++;
            RangingResultCallback callback = new RangingResultCallbackFacade(id);
            mMgr.startRanging(getWorkSource(uidsOverride),
                    new RangingRequest.Builder().addWifiAwarePeer(
                            new PeerHandle(peerId)).build(), mService.getMainExecutor(), callback);
            return id;
        }
    }

    @Rpc(description = "Cancel ranging requests for the specified UIDs")
    public void wifiRttCancelRanging(@RpcParameter(name = "uids", description = "List of UIDs")
        @RpcOptional JSONArray uids) throws JSONException {
        synchronized (mLock) {
            mMgr.cancelRanging(getWorkSource(uids));
        }
    }

    private class RangingResultCallbackFacade extends RangingResultCallback {
        private int mCallbackId;

        RangingResultCallbackFacade(int callbackId) {
            mCallbackId = callbackId;
        }

        @Override
        public void onRangingFailure(int status) {
            Bundle msg = new Bundle();
            msg.putInt("status", status);
            mEventFacade.postEvent("WifiRttRangingFailure_" + mCallbackId, msg);
        }

        @Override
        public void onRangingResults(List<RangingResult> results) {
            Bundle msg = new Bundle();
            Parcelable[] resultBundles = new Parcelable[results.size()];
            for (int i = 0; i < results.size(); i++) {
                resultBundles[i] = packRttResult(results.get(i));
            }
            msg.putParcelableArray("Results", resultBundles);
            mEventFacade.postEvent("WifiRttRangingResults_" + mCallbackId, msg);
        }
    }

    // conversion utilities
    private static Bundle packRttResult(RangingResult result) {
        Bundle bundle = new Bundle();
        bundle.putInt("status", result.getStatus());
        if (result.getStatus() == RangingResult.STATUS_SUCCESS) { // only valid on SUCCESS
            bundle.putInt("distanceMm", result.getDistanceMm());
            bundle.putInt("distanceStdDevMm", result.getDistanceStdDevMm());
            bundle.putInt("rssi", result.getRssi());
            bundle.putInt("numAttemptedMeasurements", result.getNumAttemptedMeasurements());
            bundle.putInt("numSuccessfulMeasurements", result.getNumSuccessfulMeasurements());
            bundle.putByteArray("lci", result.getLci());
            bundle.putByteArray("lcr", result.getLcr());
            bundle.putLong("timestamp", result.getRangingTimestampMillis());
        }
        if (result.getPeerHandle() != null) {
            bundle.putInt("peerId", result.getPeerHandle().peerId);
        }
        if (result.getMacAddress() != null) {
            bundle.putByteArray("mac", result.getMacAddress().toByteArray());
            bundle.putString("macAsString", result.getMacAddress().toString());
        }
        return bundle;
    }

    private static WorkSource getWorkSource(JSONArray uids) throws JSONException {
        if (uids == null) {
            return null;
        }
        WorkSource ws = new WorkSource();
        for (int i = 0; i < uids.length(); ++i) {
            ws.add(uids.getInt(i));
        }
        return ws;
    }

    class StateChangedReceiver extends BroadcastReceiver {
        @Override
        public void onReceive(Context c, Intent intent) {
            boolean isAvailable = mMgr.isAvailable();
            mEventFacade.postEvent(isAvailable ? "WifiRttAvailable" : "WifiRttNotAvailable",
                    new Bundle());
        }
    }
}
