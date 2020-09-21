/*
 * Copyright (C) 2006 The Android Open Source Project
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

import android.app.ProgressDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.AsyncResult;
import android.os.AsyncTask;
import android.os.Handler;
import android.os.Message;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;
import android.preference.ListPreference;
import android.preference.Preference;
import android.telephony.CellInfo;
import android.telephony.CellInfoCdma;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellSignalStrengthCdma;
import android.telephony.CellSignalStrengthGsm;
import android.telephony.CellSignalStrengthLte;
import android.telephony.CellSignalStrengthWcdma;
import android.telephony.NetworkScan;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.BidiFormatter;
import android.text.TextDirectionHeuristics;
import android.text.TextUtils;
import android.util.AttributeSet;
import android.util.Log;

import com.android.internal.logging.MetricsLogger;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.internal.telephony.OperatorInfo;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneFactory;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;


/**
 * "Networks" preference in "Mobile network" settings UI for the Phone app.
 * It's used to manually search and choose mobile network. Enabled only when
 * autoSelect preference is turned off.
 */
public class NetworkSelectListPreference extends ListPreference
        implements DialogInterface.OnCancelListener,
        Preference.OnPreferenceChangeListener{

    private static final String LOG_TAG = "networkSelect";
    private static final boolean DBG = true;

    private static final int EVENT_NETWORK_SELECTION_DONE = 1;
    private static final int EVENT_NETWORK_SCAN_RESULTS = 2;
    private static final int EVENT_NETWORK_SCAN_ERROR = 3;
    private static final int EVENT_NETWORK_SCAN_COMPLETED = 4;

    //dialog ids
    private static final int DIALOG_NETWORK_SELECTION = 100;
    private static final int DIALOG_NETWORK_LIST_LOAD = 200;

    private int mPhoneId = SubscriptionManager.INVALID_PHONE_INDEX;
    private List<CellInfo> mCellInfoList;
    private CellInfo mCellInfo;

    private int mSubId;
    private NetworkOperators mNetworkOperators;
    private boolean mNeedScanAgain;
    private List<String> mForbiddenPlmns;

    private ProgressDialog mProgressDialog;
    public NetworkSelectListPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public NetworkSelectListPreference(Context context, AttributeSet attrs, int defStyleAttr,
                                       int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    protected void onClick() {
        showProgressDialog(DIALOG_NETWORK_LIST_LOAD);
        TelephonyManager telephonyManager = (TelephonyManager)
                getContext().getSystemService(Context.TELEPHONY_SERVICE);
        new AsyncTask<Void, Void, List<String>>() {
            @Override
            protected List<String> doInBackground(Void... voids) {
                return Arrays.asList(telephonyManager.getForbiddenPlmns());
            }

            @Override
            protected void onPostExecute(List<String> result) {
                mForbiddenPlmns = result;
                loadNetworksList(true);
            }
        }.executeOnExecutor(AsyncTask.THREAD_POOL_EXECUTOR);
    }

    private final Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            AsyncResult ar;
            switch (msg.what) {
                case EVENT_NETWORK_SELECTION_DONE:
                    if (DBG) logd("hideProgressPanel");
                    try {
                        dismissProgressBar();
                    } catch (IllegalArgumentException e) {
                    }
                    setEnabled(true);

                    ar = (AsyncResult) msg.obj;
                    if (ar.exception != null) {
                        if (DBG) logd("manual network selection: failed!");
                        mNetworkOperators.displayNetworkSelectionFailed(ar.exception);
                    } else {
                        if (DBG) {
                            logd("manual network selection: succeeded! "
                                    + getNetworkTitle(mCellInfo));
                        }
                        mNetworkOperators.displayNetworkSelectionSucceeded(msg.arg1);
                    }
                    mNetworkOperators.getNetworkSelectionMode();
                    break;

                case EVENT_NETWORK_SCAN_RESULTS:
                    List<CellInfo> results = (List<CellInfo>) msg.obj;
                    results.removeIf(cellInfo -> cellInfo == null);
                    if (results.size() > 0) {
                        boolean isInvalidCellInfoList = true;
                        // Regard the list as invalid only if all the elements in the list are
                        // invalid.
                        for (CellInfo cellInfo : results) {
                            if (!isInvalidCellInfo(cellInfo)) {
                                isInvalidCellInfoList = false;
                                break;
                            }
                        }
                        if (isInvalidCellInfoList) {
                            mNeedScanAgain = true;
                            if (DBG) {
                                logd("Invalid cell info. Stop current network scan "
                                        + "and start a new one via old API");
                            }
                            // Stop current network scan flow. This behavior will result in a
                            // onComplete() callback, after which we will start a new network query
                            // via Phone.getAvailableNetworks(). This behavior might also result in
                            // a onError() callback if the modem did not stop network query
                            // successfully. In this case we will display network query failed
                            // instead of resending a new request.
                            try {
                                if (mNetworkQueryService != null) {
                                    mNetworkQueryService.stopNetworkQuery();
                                }
                            } catch (RemoteException e) {
                                loge("exception from stopNetworkQuery " + e);
                            }
                        } else {
                            // TODO(b/70530820): Display the scan results incrementally after
                            // finalizing the UI desing on Mobile Network Setting page. For now,
                            // just update the CellInfo list when received the onResult callback,
                            // and display the scan result when received the onComplete callback
                            // in the end.
                            mCellInfoList = new ArrayList<>(results);
                            if (DBG) logd("CALLBACK_SCAN_RESULTS" + mCellInfoList.toString());
                        }
                    }

                    break;

                case EVENT_NETWORK_SCAN_ERROR:
                    int error = msg.arg1;
                    if (DBG) logd("error while querying available networks " + error);
                    if (error == NetworkScan.ERROR_UNSUPPORTED) {
                        if (DBG) {
                            logd("Modem does not support: try to scan network again via Phone");
                        }
                        if (!mNeedScanAgain) {
                            // Avoid blinking while showing the dialog again.
                            showProgressDialog(DIALOG_NETWORK_LIST_LOAD);
                        }
                        loadNetworksList(false);
                    } else {
                        try {
                            if (mNetworkQueryService != null) {
                                mNetworkQueryService.unregisterCallback(mCallback);
                            }
                        } catch (RemoteException e) {
                            loge("onError: exception from unregisterCallback " + e);
                        }
                        displayNetworkQueryFailed(error);
                    }
                    break;

                case EVENT_NETWORK_SCAN_COMPLETED:
                    if (mNeedScanAgain) {
                        logd("CellInfo is invalid to display. Start a new scan via Phone. ");
                        loadNetworksList(false);
                        mNeedScanAgain = false;
                    } else {
                        try {
                            if (mNetworkQueryService != null) {
                                mNetworkQueryService.unregisterCallback(mCallback);
                            }
                        } catch (RemoteException e) {
                            loge("onComplete: exception from unregisterCallback " + e);
                        }
                        if (DBG) logd("scan complete, load the cellInfosList");
                        // Modify UI to indicate users that the scan has completed.
                        networksListLoaded();
                    }
                    break;
            }
            return;
        }
    };

    INetworkQueryService mNetworkQueryService = null;
    /**
     * This implementation of INetworkQueryServiceCallback is used to receive
     * callback notifications from the network query service.
     */
    private final INetworkQueryServiceCallback mCallback = new INetworkQueryServiceCallback.Stub() {

        /** Returns the scan results to the user, this callback will be called at lease one time. */
        public void onResults(List<CellInfo> results) {
            if (DBG) logd("get scan results: " + results.toString());
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
            if (DBG) logd("network scan completed.");
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
            if (DBG) logd("get onError callback with error code: " + error);
            Message msg = mHandler.obtainMessage(EVENT_NETWORK_SCAN_ERROR, error, 0 /* arg2 */);
            msg.sendToTarget();
        }
    };

    @Override
    //implemented for DialogInterface.OnCancelListener
    public void onCancel(DialogInterface dialog) {
        if (DBG) logd("user manually close the dialog");
        // request that the service stop the query with this callback object.
        try {
            if (mNetworkQueryService != null) {
                mNetworkQueryService.stopNetworkQuery();
                mNetworkQueryService.unregisterCallback(mCallback);
            }
            // If cancelled, we query NetworkSelectMode and update states of AutoSelect button.
            mNetworkOperators.getNetworkSelectionMode();
        } catch (RemoteException e) {
            loge("onCancel: exception from stopNetworkQuery " + e);
        }
    }

    @Override
    protected void onDialogClosed(boolean positiveResult) {
        super.onDialogClosed(positiveResult);
        // If dismissed, we query NetworkSelectMode and update states of AutoSelect button.
        if (!positiveResult) {
            mNetworkOperators.getNetworkSelectionMode();
        }
    }

    // This method is provided besides initialize() because bind to network query service
    // may be binded after initialize(). In that case this method needs to be called explicitly
    // to set mNetworkQueryService. Otherwise mNetworkQueryService will remain null.
    public void setNetworkQueryService(INetworkQueryService queryService) {
        mNetworkQueryService = queryService;
    }

    // This initialize method needs to be called for this preference to work properly.
    protected void initialize(int subId, INetworkQueryService queryService,
                              NetworkOperators networkOperators, ProgressDialog progressDialog) {
        mSubId = subId;
        mNetworkQueryService = queryService;
        mNetworkOperators = networkOperators;
        // This preference should share the same progressDialog with networkOperators category.
        mProgressDialog = progressDialog;
        mNeedScanAgain = false;

        if (SubscriptionManager.isValidSubscriptionId(mSubId)) {
            mPhoneId = SubscriptionManager.getPhoneId(mSubId);
        }

        TelephonyManager telephonyManager = (TelephonyManager)
                getContext().getSystemService(Context.TELEPHONY_SERVICE);

        setSummary(telephonyManager.getNetworkOperatorName());

        setOnPreferenceChangeListener(this);
    }

    @Override
    protected void onPrepareForRemoval() {
        destroy();
        super.onPrepareForRemoval();
    }

    private void destroy() {
        try {
            dismissProgressBar();
        } catch (IllegalArgumentException e) {
            loge("onDestroy: exception from dismissProgressBar " + e);
        }

        try {
            if (mNetworkQueryService != null) {
                // used to un-register callback
                mNetworkQueryService.unregisterCallback(mCallback);
            }
        } catch (RemoteException e) {
            loge("onDestroy: exception from unregisterCallback " + e);
        }
    }

    private void displayEmptyNetworkList() {
        String status = getContext().getResources().getString(R.string.empty_networks_list);

        final PhoneGlobals app = PhoneGlobals.getInstance();
        app.notificationMgr.postTransientNotification(
                NotificationMgr.NETWORK_SELECTION_NOTIFICATION, status);
    }

    private void displayNetworkSelectionInProgress() {
        showProgressDialog(DIALOG_NETWORK_SELECTION);
    }

    private void displayNetworkQueryFailed(int error) {
        String status = getContext().getResources().getString(R.string.network_query_error);

        try {
            dismissProgressBar();
        } catch (IllegalArgumentException e1) {
            // do nothing
        }

        final PhoneGlobals app = PhoneGlobals.getInstance();
        app.notificationMgr.postTransientNotification(
                NotificationMgr.NETWORK_SELECTION_NOTIFICATION, status);
    }

    private void loadNetworksList(boolean isIncrementalResult) {
        if (DBG) logd("load networks list...");
        try {
            if (mNetworkQueryService != null) {
                mNetworkQueryService.startNetworkQuery(mCallback, mPhoneId, isIncrementalResult);
            } else {
                displayNetworkQueryFailed(NetworkQueryService.QUERY_EXCEPTION);
            }
        } catch (RemoteException e) {
            loge("loadNetworksList: exception from startNetworkQuery " + e);
            displayNetworkQueryFailed(NetworkQueryService.QUERY_EXCEPTION);
        }
    }

    private void networksListLoaded() {
        if (DBG) logd("networks list loaded");

        // update the state of the preferences.
        if (DBG) logd("hideProgressPanel");

        // Always try to dismiss the dialog because activity may
        // be moved to background after dialog is shown.
        try {
            dismissProgressBar();
        } catch (IllegalArgumentException e) {
            // It's not a error in following scenario, we just ignore it.
            // "Load list" dialog will not show, if NetworkQueryService is
            // connected after this activity is moved to background.
            loge("Fail to dismiss network load list dialog " + e);
        }
        mNetworkOperators.getNetworkSelectionMode();
        if (mCellInfoList != null) {
            // create a preference for each item in the list.
            // just use the operator name instead of the mildly
            // confusing mcc/mnc.
            List<CharSequence> networkEntriesList = new ArrayList<>();
            List<CharSequence> networkEntryValuesList = new ArrayList<>();
            for (CellInfo cellInfo: mCellInfoList) {
                // Display each operator name only once.
                String networkTitle = getNetworkTitle(cellInfo);
                if (!networkEntriesList.contains(networkTitle)) {
                    if (CellInfoUtil.isForbidden(cellInfo, mForbiddenPlmns)) {
                        networkTitle += " "
                                + getContext().getResources().getString(R.string.forbidden_network);
                    }
                    networkEntriesList.add(networkTitle);
                    networkEntryValuesList.add(getOperatorNumeric(cellInfo));
                }
            }
            setEntries(networkEntriesList.toArray(new CharSequence[networkEntriesList.size()]));
            setEntryValues(networkEntryValuesList.toArray(
                    new CharSequence[networkEntryValuesList.size()]));

            super.onClick();
        } else {
            displayEmptyNetworkList();
        }
    }

    private void dismissProgressBar() {
        if (mProgressDialog != null && mProgressDialog.isShowing()) {
            mProgressDialog.dismiss();
        }
    }

    private void showProgressDialog(int id) {
        if (mProgressDialog == null) {
            mProgressDialog = new ProgressDialog(getContext());
        } else {
            // Dismiss progress bar if it's showing now.
            dismissProgressBar();
        }

        switch (id) {
            case DIALOG_NETWORK_SELECTION:
                final String networkSelectMsg = getContext().getResources()
                        .getString(R.string.register_on_network,
                                getNetworkTitle(mCellInfo));
                mProgressDialog.setMessage(networkSelectMsg);
                mProgressDialog.setCanceledOnTouchOutside(false);
                mProgressDialog.setCancelable(false);
                mProgressDialog.setIndeterminate(true);
                break;
            case DIALOG_NETWORK_LIST_LOAD:
                mProgressDialog.setMessage(
                        getContext().getResources().getString(R.string.load_networks_progress));
                mProgressDialog.setCanceledOnTouchOutside(false);
                mProgressDialog.setCancelable(true);
                mProgressDialog.setIndeterminate(false);
                mProgressDialog.setOnCancelListener(this);
                break;
            default:
        }
        mProgressDialog.show();
    }

    /**
     * Implemented to support onPreferenceChangeListener to look for preference
     * changes specifically on this button.
     *
     * @param preference is the preference to be changed, should be network select button.
     * @param newValue should be the value of the selection as index of operators.
     */
    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        int operatorIndex = findIndexOfValue((String) newValue);
        mCellInfo = mCellInfoList.get(operatorIndex);
        if (DBG) logd("selected network: " + mCellInfo.toString());

        MetricsLogger.action(getContext(),
                MetricsEvent.ACTION_MOBILE_NETWORK_MANUAL_SELECT_NETWORK);

        Message msg = mHandler.obtainMessage(EVENT_NETWORK_SELECTION_DONE);
        Phone phone = PhoneFactory.getPhone(mPhoneId);
        if (phone != null) {
            OperatorInfo operatorInfo = getOperatorInfoFromCellInfo(mCellInfo);
            if (DBG) logd("manually selected network: " + operatorInfo.toString());
            phone.selectNetworkManually(operatorInfo, true, msg);
            displayNetworkSelectionInProgress();
        } else {
            loge("Error selecting network. phone is null.");
        }
        return true;
    }

    /**
     * Returns the title of the network obtained in the manual search.
     *
     * @param cellInfo contains the information of the network.
     * @return Long Name if not null/empty, otherwise Short Name if not null/empty,
     * else MCCMNC string.
     */
    private String getNetworkTitle(CellInfo cellInfo) {
        OperatorInfo ni = getOperatorInfoFromCellInfo(cellInfo);

        if (!TextUtils.isEmpty(ni.getOperatorAlphaLong())) {
            return ni.getOperatorAlphaLong();
        } else if (!TextUtils.isEmpty(ni.getOperatorAlphaShort())) {
            return ni.getOperatorAlphaShort();
        } else {
            BidiFormatter bidiFormatter = BidiFormatter.getInstance();
            return bidiFormatter.unicodeWrap(ni.getOperatorNumeric(), TextDirectionHeuristics.LTR);
        }
    }

    /**
     * Returns the operator numeric (MCCMNC) obtained in the manual search.
     *
     * @param cellInfo contains the information of the network.
     * @return MCCMNC string.
     */
    private String getOperatorNumeric(CellInfo cellInfo) {
        return getOperatorInfoFromCellInfo(cellInfo).getOperatorNumeric();
    }

    /**
     * Wrap a cell info into an operator info.
     */
    private OperatorInfo getOperatorInfoFromCellInfo(CellInfo cellInfo) {
        OperatorInfo oi;
        if (cellInfo instanceof CellInfoLte) {
            CellInfoLte lte = (CellInfoLte) cellInfo;
            oi = new OperatorInfo(
                    (String) lte.getCellIdentity().getOperatorAlphaLong(),
                    (String) lte.getCellIdentity().getOperatorAlphaShort(),
                    lte.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoWcdma) {
            CellInfoWcdma wcdma = (CellInfoWcdma) cellInfo;
            oi = new OperatorInfo(
                    (String) wcdma.getCellIdentity().getOperatorAlphaLong(),
                    (String) wcdma.getCellIdentity().getOperatorAlphaShort(),
                    wcdma.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoGsm) {
            CellInfoGsm gsm = (CellInfoGsm) cellInfo;
            oi = new OperatorInfo(
                    (String) gsm.getCellIdentity().getOperatorAlphaLong(),
                    (String) gsm.getCellIdentity().getOperatorAlphaShort(),
                    gsm.getCellIdentity().getMobileNetworkOperator());
        } else if (cellInfo instanceof CellInfoCdma) {
            CellInfoCdma cdma = (CellInfoCdma) cellInfo;
            oi = new OperatorInfo(
                    (String) cdma.getCellIdentity().getOperatorAlphaLong(),
                    (String) cdma.getCellIdentity().getOperatorAlphaShort(),
                    "" /* operator numeric */);
        } else {
            oi = new OperatorInfo("", "", "");
        }
        return oi;
    }


    /**
     * Check if the CellInfo is valid to display. If a CellInfo has signal strength but does
     * not have operator info, it is invalid to display.
     */
    private boolean isInvalidCellInfo(CellInfo cellInfo) {
        if (DBG) logd("Check isInvalidCellInfo: " + cellInfo.toString());
        CharSequence al = null;
        CharSequence as = null;
        boolean hasSignalStrength = false;
        if (cellInfo instanceof CellInfoLte) {
            CellInfoLte lte = (CellInfoLte) cellInfo;
            al = lte.getCellIdentity().getOperatorAlphaLong();
            as = lte.getCellIdentity().getOperatorAlphaShort();
            hasSignalStrength = !lte.getCellSignalStrength().equals(new CellSignalStrengthLte());
        } else if (cellInfo instanceof CellInfoWcdma) {
            CellInfoWcdma wcdma = (CellInfoWcdma) cellInfo;
            al = wcdma.getCellIdentity().getOperatorAlphaLong();
            as = wcdma.getCellIdentity().getOperatorAlphaShort();
            hasSignalStrength = !wcdma.getCellSignalStrength().equals(
                    new CellSignalStrengthWcdma());
        } else if (cellInfo instanceof CellInfoGsm) {
            CellInfoGsm gsm = (CellInfoGsm) cellInfo;
            al = gsm.getCellIdentity().getOperatorAlphaLong();
            as = gsm.getCellIdentity().getOperatorAlphaShort();
            hasSignalStrength = !gsm.getCellSignalStrength().equals(new CellSignalStrengthGsm());
        } else if (cellInfo instanceof CellInfoCdma) {
            CellInfoCdma cdma = (CellInfoCdma) cellInfo;
            al = cdma.getCellIdentity().getOperatorAlphaLong();
            as = cdma.getCellIdentity().getOperatorAlphaShort();
            hasSignalStrength = !cdma.getCellSignalStrength().equals(new CellSignalStrengthCdma());
        } else {
            return true;
        }
        if (TextUtils.isEmpty(al) && TextUtils.isEmpty(as) && hasSignalStrength) {
            return true;
        }
        return false;
    }

    @Override
    protected Parcelable onSaveInstanceState() {
        final Parcelable superState = super.onSaveInstanceState();
        if (isPersistent()) {
            // No need to save instance state since it's persistent
            return superState;
        }

        final SavedState myState = new SavedState(superState);
        myState.mDialogListEntries = getEntries();
        myState.mDialogListEntryValues = getEntryValues();
        myState.mCellInfoList = mCellInfoList;
        return myState;
    }

    @Override
    protected void onRestoreInstanceState(Parcelable state) {
        if (state == null || !state.getClass().equals(SavedState.class)) {
            // Didn't save state for us in onSaveInstanceState
            super.onRestoreInstanceState(state);
            return;
        }

        SavedState myState = (SavedState) state;

        if (getEntries() == null && myState.mDialogListEntries != null) {
            setEntries(myState.mDialogListEntries);
        }
        if (getEntryValues() == null && myState.mDialogListEntryValues != null) {
            setEntryValues(myState.mDialogListEntryValues);
        }
        if (mCellInfoList == null && myState.mCellInfoList != null) {
            mCellInfoList = myState.mCellInfoList;
        }

        super.onRestoreInstanceState(myState.getSuperState());
    }

    /**
     *  We save entries, entryValues and operatorInfoList into bundle.
     *  At onCreate of fragment, dialog will be restored if it was open. In this case,
     *  we need to restore entries, entryValues and operatorInfoList. Without those information,
     *  onPreferenceChange will fail if user select network from the dialog.
     */
    private static class SavedState extends BaseSavedState {
        CharSequence[] mDialogListEntries;
        CharSequence[] mDialogListEntryValues;
        List<CellInfo> mCellInfoList;

        SavedState(Parcel source) {
            super(source);
            final ClassLoader boot = Object.class.getClassLoader();
            mDialogListEntries = source.readCharSequenceArray();
            mDialogListEntryValues = source.readCharSequenceArray();
            mCellInfoList = source.readParcelableList(mCellInfoList, boot);
        }

        @Override
        public void writeToParcel(Parcel dest, int flags) {
            super.writeToParcel(dest, flags);
            dest.writeCharSequenceArray(mDialogListEntries);
            dest.writeCharSequenceArray(mDialogListEntryValues);
            dest.writeParcelableList(mCellInfoList, flags);
        }

        SavedState(Parcelable superState) {
            super(superState);
        }

        public static final Parcelable.Creator<SavedState> CREATOR =
                new Parcelable.Creator<SavedState>() {
                    public SavedState createFromParcel(Parcel in) {
                        return new SavedState(in);
                    }

                    public SavedState[] newArray(int size) {
                        return new SavedState[size];
                    }
                };
    }

    private void logd(String msg) {
        Log.d(LOG_TAG, "[NetworksList] " + msg);
    }

    private void loge(String msg) {
        Log.e(LOG_TAG, "[NetworksList] " + msg);
    }
}