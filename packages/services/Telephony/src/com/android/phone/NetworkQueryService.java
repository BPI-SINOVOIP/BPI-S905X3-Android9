/*
 * Copyright (C) 2008 The Android Open Source Project
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

package com.android.phone;

import android.app.Service;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncResult;
import android.os.Binder;
import android.os.Handler;
import android.os.IBinder;
import android.os.Message;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.telephony.AccessNetworkConstants;
import android.telephony.CellIdentityGsm;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.NetworkScan;
import android.telephony.NetworkScanRequest;
import android.telephony.RadioAccessSpecifier;
import android.telephony.TelephonyManager;
import android.telephony.TelephonyScanManager;
import android.util.Log;

import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

import java.util.ArrayList;
import java.util.List;

/**
 * Service code used to assist in querying the network for service
 * availability.
 */
public class NetworkQueryService extends Service {
    // debug data
    private static final String LOG_TAG = "NetworkQuery";
    private static final boolean DBG = true;

    // static events
    private static final int EVENT_NETWORK_SCAN_VIA_PHONE_COMPLETED = 100;
    private static final int EVENT_NETWORK_SCAN_RESULTS = 200;
    private static final int EVENT_NETWORK_SCAN_ERROR = 300;
    private static final int EVENT_NETWORK_SCAN_COMPLETED = 400;

    // static states indicating the query status of the service
    private static final int QUERY_READY = -1;
    private static final int QUERY_IS_RUNNING = -2;

    // error statuses that will be retured in the callback.
    public static final int QUERY_OK = 0;
    public static final int QUERY_EXCEPTION = 1;

    static final String ACTION_LOCAL_BINDER = "com.android.phone.intent.action.LOCAL_BINDER";

    /** state of the query service */
    private int mState;

    private NetworkScan mNetworkScan;

    // NetworkScanRequest parameters
    private static final int SCAN_TYPE = NetworkScanRequest.SCAN_TYPE_ONE_SHOT;
    private static final boolean INCREMENTAL_RESULTS = true;
    // The parameters below are in seconds
    private static final int SEARCH_PERIODICITY_SEC = 5;
    private static final int MAX_SEARCH_TIME_SEC = 300;
    private static final int INCREMENTAL_RESULTS_PERIODICITY_SEC = 3;

    /**
     * Class for clients to access.  Because we know this service always
     * runs in the same process as its clients, we don't need to deal with
     * IPC.
     */
    public class LocalBinder extends Binder {
        INetworkQueryService getService() {
            return mBinder;
        }
    }
    private final IBinder mLocalBinder = new LocalBinder();

    /**
     * Local handler to receive the network query compete callback
     * from the RIL.
     */
    Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                // if the scan is complete, broadcast the results.
                // to all registerd callbacks.
                case EVENT_NETWORK_SCAN_VIA_PHONE_COMPLETED:
                    if (DBG) log("scan via Phone completed, broadcasting results");
                    broadcastQueryResults(msg);
                    break;

                case EVENT_NETWORK_SCAN_RESULTS:
                    if (DBG) log("get scan results, broadcasting results");
                    broadcastQueryResults(msg);
                    break;

                case EVENT_NETWORK_SCAN_ERROR:
                    if (DBG) log("get scan error, broadcasting error code");
                    broadcastQueryResults(msg);
                    break;

                case EVENT_NETWORK_SCAN_COMPLETED:
                    if (DBG) log("network scan or stop network query completed");
                    broadcastQueryResults(msg);
                    break;
            }
        }
    };

    /**
     * List of callback objects, also used to synchronize access to 
     * itself and to changes in state.
     */
    final RemoteCallbackList<INetworkQueryServiceCallback> mCallbacks =
            new RemoteCallbackList<INetworkQueryServiceCallback>();

    /**
     * This implementation of NetworkScanCallbackImpl is used to receive callback notifications from
     * the Telephony Manager.
     */
    public class NetworkScanCallbackImpl extends TelephonyScanManager.NetworkScanCallback {

        /** Returns the scan results to the user, this callback will be called at least one time. */
        public void onResults(List<CellInfo> results) {
            if (DBG) log("got network scan results: " + results.size());
            Message msg = mHandler.obtainMessage(EVENT_NETWORK_SCAN_RESULTS, results);
            msg.sendToTarget();
        }

        /**
         * Informs the user that the scan has stopped.
         *
         * This callback will be called when the scan is finished or cancelled by the user.
         * The related NetworkScanRequest will be deleted after this callback.
         */
        public void onComplete() {
            if (DBG) log("network scan completed");
            Message msg = mHandler.obtainMessage(EVENT_NETWORK_SCAN_COMPLETED);
            msg.sendToTarget();
        }

        /**
         * Informs the user that there is some error about the scan.
         *
         * This callback will be called whenever there is any error about the scan, and the scan
         * will be terminated. onComplete() will NOT be called.
         */
        public void onError(int error) {
            if (DBG) log("network scan got error: " + error);
            Message msg = mHandler.obtainMessage(EVENT_NETWORK_SCAN_ERROR, error, 0 /* arg2 */);
            msg.sendToTarget();
        }
    }

    /**
     * Implementation of the INetworkQueryService interface.
     */
    private final INetworkQueryService.Stub mBinder = new INetworkQueryService.Stub() {

        /**
         * Starts a query with a INetworkQueryServiceCallback object if
         * one has not been started yet.  Ignore the new query request
         * if the query has been started already.  Either way, place the
         * callback object in the queue to be notified upon request
         * completion.
         */
        public void startNetworkQuery(
                INetworkQueryServiceCallback cb, int phoneId, boolean isIncrementalResult) {
            if (cb != null) {
                // register the callback to the list of callbacks.
                synchronized (mCallbacks) {
                    mCallbacks.register(cb);
                    if (DBG) log("registering callback " + cb.getClass().toString());

                    switch (mState) {
                        case QUERY_READY:

                            if (isIncrementalResult) {
                                if (DBG) log("start network scan via TelephonManager");
                                TelephonyManager tm = (TelephonyManager) getSystemService(
                                        Context.TELEPHONY_SERVICE);
                                // The Radio Access Specifiers below are meant to scan
                                // all the bands for all the supported technologies.
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
                                RadioAccessSpecifier[] radioAccessSpecifier = {gsm, lte, wcdma};
                                NetworkScanRequest networkScanRequest = new NetworkScanRequest(
                                        SCAN_TYPE,
                                        radioAccessSpecifier,
                                        SEARCH_PERIODICITY_SEC,
                                        MAX_SEARCH_TIME_SEC,
                                        INCREMENTAL_RESULTS,
                                        INCREMENTAL_RESULTS_PERIODICITY_SEC,
                                        null /* List of PLMN ids (MCC-MNC) */);

                                // Construct a NetworkScanCallback
                                NetworkQueryService.NetworkScanCallbackImpl
                                        networkScanCallback =
                                        new NetworkQueryService.NetworkScanCallbackImpl();

                                // Request network scan
                                mNetworkScan = tm.requestNetworkScan(networkScanRequest,
                                        networkScanCallback);
                                mState = QUERY_IS_RUNNING;
                            } else {
                                Phone phone = PhoneFactory.getPhone(phoneId);
                                if (phone != null) {
                                    phone.getAvailableNetworks(
                                            mHandler.obtainMessage(
                                                    EVENT_NETWORK_SCAN_VIA_PHONE_COMPLETED));
                                    mState = QUERY_IS_RUNNING;
                                    if (DBG) log("start network scan via Phone");
                                } else {
                                    if (DBG) {
                                        log("phone is null");
                                    }
                                }
                            }

                            break;
                        // do nothing if we're currently busy.
                        case QUERY_IS_RUNNING:
                            if (DBG) log("query already in progress");
                            break;
                        default:
                    }
                }
            }
        }

        /**
         * Stops a query with a INetworkQueryServiceCallback object as
         * a token.
         */
        public void stopNetworkQuery() {
            if (DBG) log("stop network query");
            // Tells the RIL to terminate the query request.
            if (mNetworkScan != null) {
                try {
                    mNetworkScan.stop();
                    mState = QUERY_READY;
                } catch (RemoteException e) {
                    if (DBG) log("stop mNetworkScan failed");
                } catch (IllegalArgumentException e) {
                    // Do nothing, scan has already completed.
                }
            }
        }

        /**
         * Unregisters the callback without impacting an underlying query.
         */
        public void unregisterCallback(INetworkQueryServiceCallback cb) {
            if (cb != null) {
                synchronized (mCallbacks) {
                    if (DBG) log("unregistering callback " + cb.getClass().toString());
                    mCallbacks.unregister(cb);
                }
            }
        }
    };

    @Override
    public void onCreate() {
        mState = QUERY_READY;
    }

    /**
     * Required for service implementation.
     */
    @Override
    public void onStart(Intent intent, int startId) {
    }

    /**
     * Handle the bind request.
     */
    @Override
    public IBinder onBind(Intent intent) {
        if (DBG) log("binding service implementation");
        if (ACTION_LOCAL_BINDER.equals(intent.getAction())) {
            return mLocalBinder;
        }

        return mBinder;
    }

    /**
     * Broadcast the results from the query to all registered callback
     * objects. 
     */
    private void broadcastQueryResults(Message msg) {
        // reset the state.
        synchronized (mCallbacks) {
            mState = QUERY_READY;

            // Make the calls to all the registered callbacks.
            for (int i = (mCallbacks.beginBroadcast() - 1); i >= 0; i--) {
                INetworkQueryServiceCallback cb = mCallbacks.getBroadcastItem(i);
                if (DBG) log("broadcasting results to " + cb.getClass().toString());
                try {
                    switch (msg.what) {
                        case EVENT_NETWORK_SCAN_VIA_PHONE_COMPLETED:
                            AsyncResult ar = (AsyncResult) msg.obj;
                            if (ar != null) {
                                cb.onResults(getCellInfoList((List<OperatorInfo>) ar.result));
                            } else {
                                if (DBG) log("AsyncResult is null.");
                            }
                            // Send the onComplete() callback to indicate the one-time network
                            // scan has completed.
                            cb.onComplete();
                            break;

                        case EVENT_NETWORK_SCAN_RESULTS:
                            cb.onResults((List<CellInfo>) msg.obj);
                            break;

                        case EVENT_NETWORK_SCAN_COMPLETED:
                            cb.onComplete();
                            break;

                        case EVENT_NETWORK_SCAN_ERROR:
                            cb.onError(msg.arg1);
                            break;
                    }
                } catch (RemoteException e) {
                }
            }

            // finish up.
            mCallbacks.finishBroadcast();
        }
    }

    /**
     * Wraps up a list of OperatorInfo object to a list of CellInfo object. GsmCellInfo is used here
     * only because operatorInfo does not contain technology type while CellInfo is an abstract
     * object that requires to specify technology type. It doesn't matter which CellInfo type to
     * use here, since we only want to wrap the operator info and PLMN to a CellInfo object.
     */
    private List<CellInfo> getCellInfoList(List<OperatorInfo> operatorInfoList) {
        List<CellInfo> cellInfoList = new ArrayList<>();
        for (OperatorInfo oi: operatorInfoList) {
            String operatorNumeric = oi.getOperatorNumeric();
            String mcc = null;
            String mnc = null;
            log("operatorNumeric: " + operatorNumeric);
            if (operatorNumeric != null && operatorNumeric.matches("^[0-9]{5,6}$")) {
                mcc = operatorNumeric.substring(0, 3);
                mnc = operatorNumeric.substring(3);
            }
            CellIdentityGsm cig = new CellIdentityGsm(
                    Integer.MAX_VALUE /* lac */,
                    Integer.MAX_VALUE /* cid */,
                    Integer.MAX_VALUE /* arfcn */,
                    Integer.MAX_VALUE /* bsic */,
                    mcc,
                    mnc,
                    oi.getOperatorAlphaLong(),
                    oi.getOperatorAlphaShort());

            CellInfoGsm ci = new CellInfoGsm();
            ci.setCellIdentity(cig);
            cellInfoList.add(ci);
        }
        return cellInfoList;
    }

    private static void log(String msg) {
        Log.d(LOG_TAG, msg);
    }
}