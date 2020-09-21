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

package com.android.cts.verifier.net;

import static android.net.NetworkCapabilities.NET_CAPABILITY_INTERNET;
import static android.net.NetworkCapabilities.TRANSPORT_CELLULAR;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;

import static com.android.cts.verifier.net.MultiNetworkConnectivityTestActivity.ValidatorState
        .COMPLETED;
import static com.android.cts.verifier.net.MultiNetworkConnectivityTestActivity.ValidatorState
        .NOT_STARTED;
import static com.android.cts.verifier.net.MultiNetworkConnectivityTestActivity.ValidatorState
        .STARTED;
import static com.android.cts.verifier.net.MultiNetworkConnectivityTestActivity.ValidatorState
        .WAITING_FOR_USER_INPUT;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.ConnectivityManager.NetworkCallback;
import android.net.DhcpInfo;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.net.NetworkRequest;
import android.net.wifi.SupplicantState;
import android.net.wifi.WifiConfiguration;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.telephony.TelephonyManager;
import android.text.Editable;
import android.text.TextUtils;
import android.text.TextWatcher;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import java.util.List;

/**
 * A CTS verifier to ensure that when an app calls WifiManager#enableNetwork,
 * - When the wifi network does not have internet connectivity, the device should
 * not disable other forms or connectivity, for example cellular.
 * - When the wifi network that the phone connects to loses connectivity, then
 * other forms of connectivity are restored, for example cellular when the phone
 * detects that the Wifi network doesn't have internet.
 */
public class MultiNetworkConnectivityTestActivity extends PassFailButtons.Activity {
    public static final String TAG = "MultinetworkTest";
    public static final int ENABLE_DISABLE_WIFI_DELAY_MS = 3000;
    public static final int WIFI_NETWORK_CONNECT_TIMEOUT_MS = 45000;
    public static final int WIFI_NETWORK_CONNECT_TO_BE_ACTIVE_MS = 25000;
    public static final int CELLULAR_NETWORK_CONNECT_TIMEOUT_MS = 45000;
    public static final int CELLULAR_NETWORK_RESTORE_TIMEOUT_MS = 15000;
    public static final int CELLULAR_NETWORK_RESTORE_AFTER_WIFI_INTERNET_LOST_TIMEOUT_MS = 60000;

    /**
     * Called by the validator test when it has different states.
     */
    private interface MultinetworkTestCallback {
        /** Notify test has started */
        void testStarted();

        /** Show / display progress using the message in progressMessage */
        void testProgress(int progressMessageResourceId);

        /** Test completed for the validator */
        void testCompleted(MultiNetworkValidator validator);
    }

    enum ValidatorState {
        NOT_STARTED,
        STARTED,
        WAITING_FOR_USER_INPUT,
        COMPLETED,
    }

    private final Handler mMainHandler = new Handler(Looper.getMainLooper());
    // Used only for posting bugs / debugging.
    private final BroadcastReceiver mMultiNetConnectivityReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            Log.d(TAG, "Action " + intent.getAction());
            if (intent.getAction().equals(WifiManager.NETWORK_STATE_CHANGED_ACTION)) {
                NetworkInfo networkInfo = intent.getParcelableExtra(WifiManager.EXTRA_NETWORK_INFO);
                Log.d(TAG, "New network state " + networkInfo.getState());
            } else if (intent.getAction().equals(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION)) {
                SupplicantState state = intent.getParcelableExtra(WifiManager.EXTRA_NEW_STATE);
                Log.d(TAG, "New supplicant state. " + state.name());
                Log.d(TAG, "Is connected to expected wifi AP. " +
                        isConnectedToExpectedWifiNetwork());
            }
        }
    };
    private final MultinetworkTestCallback mMultinetworkTestCallback =
            new MultinetworkTestCallback() {

                @Override
                public void testStarted() {
                    mTestInfoView.setText(R.string.multinetwork_connectivity_test_running);
                }

                @Override
                public void testProgress(int progressMessageResourceId) {
                    mTestInfoView.setText(progressMessageResourceId);
                }

                @Override
                public void testCompleted(MultiNetworkValidator validator) {
                    if (validator == mMultiNetworkValidators[mMultiNetworkValidators.length
                            - 1]) {
                        // Done all tests.
                        boolean passed = true;
                        for (MultiNetworkValidator multiNetworkValidator :
                                mMultiNetworkValidators) {
                            passed = passed && multiNetworkValidator.mTestResult;
                        }
                        setTestResultAndFinish(passed);
                    } else if (!validator.mTestResult) {
                        setTestResultAndFinish(false);
                    } else {
                        for (int i = 0; i < mMultiNetworkValidators.length; i++) {
                            if (mMultiNetworkValidators[i] == validator) {
                                mCurrentValidator = mMultiNetworkValidators[i + 1];
                                mTestNameView.setText(mCurrentValidator.mTestDescription);
                                mCurrentValidator.startTest();
                                break;
                            }
                        }
                    }
                }
            };
    private final MultiNetworkValidator[] mMultiNetworkValidators = {
            new ConnectToWifiWithNoInternetValidator(
                    R.string.multinetwork_connectivity_test_1_desc),
            new ConnectToWifiWithIntermittentInternetValidator(
                    R.string.multinetwork_connectivity_test_2_desc)
    };
    private final Runnable mTimeToCompletionRunnable = new Runnable() {
        @Override
        public void run() {
            mSecondsToCompletion--;
            if (mSecondsToCompletion > 0) {
                mStartButton.setText("" + mSecondsToCompletion);
                mMainHandler.postDelayed(this, 1000);
            }
        }
    };

    // User interface elements.
    private Button mStartButton;
    private TextView mTestNameView;
    private TextView mTestInfoView;
    private EditText mAccessPointSsidEditText;
    private EditText mPskEditText;

    // Current state memebers.
    private MultiNetworkValidator mCurrentValidator;
    private int mSecondsToCompletion;
    private String mAccessPointSsid = "";
    private String mPskValue = "";
    private ConnectivityManager mConnectivityManager;
    private WifiManager mWifiManager;

    private int mRecordedWifiConfiguration = -1;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mConnectivityManager = (ConnectivityManager) getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        recordCurrentWifiState();
        setupUserInterface();
        setupBroadcastReceivers();
    }

    @Override
    protected void onResume() {
        super.onResume();
        setupCurrentTestStateOnResume();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        destroyBroadcastReceivers();
        restoreOriginalWifiState();
    }

    private void recordCurrentWifiState() {
        if (!mWifiManager.isWifiEnabled()) {
            return;
        }
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        if (wifiInfo != null && SupplicantState.COMPLETED.equals(wifiInfo.getSupplicantState())) {
            mRecordedWifiConfiguration = wifiInfo.getNetworkId();
        }
    }

    private void restoreOriginalWifiState() {
        if (mRecordedWifiConfiguration >= 0) {
            mWifiManager.enableNetwork(mRecordedWifiConfiguration, true);
        }
    }

    private void setupUserInterface() {
        setContentView(R.layout.multinetwork_connectivity);
        setInfoResources(
                R.string.multinetwork_connectivity_test,
                R.string.multinetwork_connectivity_test_instructions,
                -1);
        mStartButton = findViewById(R.id.start_multinet_btn);
        mTestNameView = findViewById(R.id.current_test);
        mTestInfoView = findViewById(R.id.test_progress_info);
        mAccessPointSsidEditText = findViewById(R.id.test_ap_ssid);
        mPskEditText = findViewById(R.id.test_ap_psk);
        mAccessPointSsidEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void afterTextChanged(Editable editable) {
                mAccessPointSsid = editable.toString();
                Log.i(TAG, "Connecting to " + mAccessPointSsid);
                mStartButton.setEnabled(isReadyToStart());
            }
        });
        mPskEditText.addTextChangedListener(new TextWatcher() {
            @Override
            public void beforeTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void onTextChanged(CharSequence charSequence, int i, int i1, int i2) {}

            @Override
            public void afterTextChanged(Editable editable) {
                mPskValue = editable.toString();
                mStartButton.setEnabled(isReadyToStart());
            }
        });
        mStartButton.setOnClickListener(view -> processStartClicked());
    }

    private void setupBroadcastReceivers() {
        IntentFilter intentFilter = new IntentFilter(ConnectivityManager.CONNECTIVITY_ACTION);
        intentFilter.addAction(WifiManager.NETWORK_STATE_CHANGED_ACTION);
        intentFilter.addAction(WifiManager.SUPPLICANT_CONNECTION_CHANGE_ACTION);
        intentFilter.addAction(WifiManager.SUPPLICANT_STATE_CHANGED_ACTION);
        registerReceiver(mMultiNetConnectivityReceiver, intentFilter);
    }

    private void destroyBroadcastReceivers() {
        unregisterReceiver(mMultiNetConnectivityReceiver);
    }

    private boolean isReadyToStart() {
        return !(TextUtils.isEmpty(mAccessPointSsid) || TextUtils.isEmpty(mPskValue));
    }

    private static boolean isNetworkCellularAndHasInternet(ConnectivityManager connectivityManager,
            Network network) {
        NetworkCapabilities capabilities = connectivityManager.getNetworkCapabilities(network);
        return capabilities.hasTransport(TRANSPORT_CELLULAR)
                && capabilities.hasCapability(NET_CAPABILITY_INTERNET);
    }

    private boolean isMobileDataEnabled(TelephonyManager telephonyManager) {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            return telephonyManager.isDataEnabled();
        }
        Network[] allNetworks = mConnectivityManager.getAllNetworks();
        for (Network network : allNetworks) {
            if (isNetworkCellularAndHasInternet(mConnectivityManager, network)) {
                return true;
            }
        }
        return false;
    }

    private boolean checkPreRequisites() {
        TelephonyManager telephonyManager =
                (TelephonyManager) getSystemService(Context.TELEPHONY_SERVICE);
        if (telephonyManager == null) {
            Log.e(TAG, "Device does not have telephony manager");
            mTestInfoView.setText(R.string.multinetwork_connectivity_test_all_prereq_1);
            return false;
        } else if (!isMobileDataEnabled(telephonyManager)) {
            Log.e(TAG, "Device mobile data is not available");
            mTestInfoView.setText(R.string.multinetwork_connectivity_test_all_prereq_2);
            return false;
        }
        return true;
    }

    /**
     * If tester went back and came in again, make sure that test resumes from the previous state.
     */
    private void setupCurrentTestStateOnResume() {
        mCurrentValidator = null;
        mStartButton.setEnabled(false);

        if (!checkPreRequisites()) {
            return;
        }

        for (int i = 0; i < mMultiNetworkValidators.length; i++) {
            if (mMultiNetworkValidators[i].mValidatorState != COMPLETED) {
                mCurrentValidator = mMultiNetworkValidators[i];
                break;
            }
        }
        if (mCurrentValidator != null) {
            mTestNameView.setText(mCurrentValidator.mTestDescription);

            switch (mCurrentValidator.mValidatorState) {
                case NOT_STARTED:
                    mStartButton.setText(R.string.multinetwork_connectivity_test_start);
                    mStartButton.setEnabled(isReadyToStart());
                    break;
                case STARTED:
                    mTestInfoView.setText(getResources().getString(
                            mCurrentValidator.mTestProgressMessage));
                    break;
                case WAITING_FOR_USER_INPUT:
                    mStartButton.setText(R.string.multinetwork_connectivity_test_continue);
                    mStartButton.setEnabled(true);
                    mTestInfoView.setText(getResources().getString(
                            mCurrentValidator.mTestProgressMessage));
                case COMPLETED:
                    break;
            }
            mTestNameView.setText(mCurrentValidator.mTestDescription);
        } else {
            // All tests completed, so need to re run. It's not likely to get here as
            // the default action when all test completes is to mark success and finish.
            mStartButton.setText(R.string.multinetwork_connectivity_test_rerun);
            mStartButton.setEnabled(true);
            rerunMultinetworkTests();
            mCurrentValidator = mMultiNetworkValidators[0];
        }
    }

    private void rerunMultinetworkTests() {
        for (MultiNetworkValidator validator : mMultiNetworkValidators) {
            validator.reset();
        }
    }

    private void requestUserConfirmation() {
        mMainHandler.post(() -> {
            mStartButton.setText(R.string.multinetwork_connectivity_test_continue);
            mStartButton.setEnabled(true);
        });
    }

    private void processStartClicked() {
        if (mCurrentValidator == null) {
            rerunMultinetworkTests();
            setupCurrentTestStateOnResume();
        }
        mStartButton.setEnabled(false);
        if (mCurrentValidator.mValidatorState == NOT_STARTED) {
            mCurrentValidator.startTest();
        } else if (mCurrentValidator.mValidatorState == WAITING_FOR_USER_INPUT) {
            mStartButton.setEnabled(false);
            mCurrentValidator.continueWithTest();
        }
    }

    private WifiConfiguration buildWifiConfiguration() {
        WifiConfiguration wifiConfiguration = new WifiConfiguration();
        wifiConfiguration.SSID = "\"" + mAccessPointSsid + "\"";
        wifiConfiguration.preSharedKey = "\"" + mPskValue + "\"";
        wifiConfiguration.status = WifiConfiguration.Status.ENABLED;
        wifiConfiguration.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.TKIP);
        wifiConfiguration.allowedGroupCiphers.set(WifiConfiguration.GroupCipher.CCMP);
        wifiConfiguration.allowedKeyManagement.set(WifiConfiguration.KeyMgmt.WPA_PSK);
        wifiConfiguration.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.TKIP);
        wifiConfiguration.allowedPairwiseCiphers.set(WifiConfiguration.PairwiseCipher.CCMP);
        wifiConfiguration.allowedProtocols.set(WifiConfiguration.Protocol.RSN);
        return wifiConfiguration;
    }

    private int addOrUpdateNetwork() {
        List<WifiConfiguration> availableConfigurations = mWifiManager.getConfiguredNetworks();
        for (WifiConfiguration configuration : availableConfigurations) {
            if (mAccessPointSsid.equals(configuration.SSID)) {
                return configuration.networkId;
            }
        }
        int newNetwork = mWifiManager.addNetwork(buildWifiConfiguration());
        return newNetwork;
    }

    private boolean isConnectedToExpectedWifiNetwork() {
        WifiInfo wifiInfo = mWifiManager.getConnectionInfo();
        DhcpInfo dhcpInfo = mWifiManager.getDhcpInfo();
        Log.i(TAG, "Checking connected to expected " + mAccessPointSsid);
        if (wifiInfo != null
                && wifiInfo.getSupplicantState().equals(SupplicantState.COMPLETED)
                && dhcpInfo != null) {
            String failsafeSsid = String.format("\"%s\"", mAccessPointSsid);
            Log.i(TAG, "Connected to " + wifiInfo.getSSID() + " expected " + mAccessPointSsid);
            return mAccessPointSsid.equals(wifiInfo.getSSID())
                    || failsafeSsid.equals(wifiInfo.getSSID());
        }
        return false;
    }

    private void startTimerCountdownDisplay(int timeoutInSeconds) {
        mMainHandler.post(() -> mSecondsToCompletion = timeoutInSeconds);
        mMainHandler.post(mTimeToCompletionRunnable);
    }

    private void stopTimerCountdownDisplay() {
        mMainHandler.removeCallbacks(mTimeToCompletionRunnable);
        mStartButton.setText("--");
    }

    /**
     * Manage the connectivity state for each MultinetworkValidation.
     */
    private class TestConnectivityState {
        private final MultiNetworkValidator mMultiNetworkValidator;

        final NetworkCallback mWifiNetworkCallback = new NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                Log.i(TAG, "Wifi network available " + network.netId);
                stopTimerDisplayIfRequested();
                mMultiNetworkValidator.onWifiNetworkConnected(network);
            }

            @Override
            public void onUnavailable() {
                Log.e(TAG, "Failed to connect to wifi");
                stopTimerDisplayIfRequested();
                mMultiNetworkValidator.onWifiNetworkUnavailable();
            }
        };
        final NetworkCallback mCellularNetworkCallback = new NetworkCallback() {
            @Override
            public void onAvailable(Network network) {
                Log.i(TAG, "Cellular network available " + network.netId);
                stopTimerDisplayIfRequested();
                mMultiNetworkValidator.onCellularNetworkConnected(network);
            }

            @Override
            public void onUnavailable() {
                Log.e(TAG, "Cellular network unavailable ");
                stopTimerDisplayIfRequested();
                mMultiNetworkValidator.onCellularNetworkUnavailable();
            }
        };
        boolean mCellularNetworkRequested;
        boolean mWifiNetworkRequested;
        boolean mTimerStartRequested;

        TestConnectivityState(MultiNetworkValidator validator) {
            mMultiNetworkValidator = validator;
        }

        void reset() {
            mMainHandler.post(() -> stopTimerDisplayIfRequested());
            if (mWifiNetworkRequested) {
                mConnectivityManager.unregisterNetworkCallback(mWifiNetworkCallback);
                mWifiNetworkRequested = false;
            }
            if (mCellularNetworkRequested) {
                mConnectivityManager.unregisterNetworkCallback(mCellularNetworkCallback);
                mCellularNetworkRequested = false;
            }
        }

        private void connectToWifiNetwork(boolean requireInternet) {
            // If device is not connected to the expected WifiNetwork, connect to the wifi Network.
            // Timeout with failure if it can't connect.
            if (!isConnectedToExpectedWifiNetwork()) {
                int network = addOrUpdateNetwork();
                WifiManager wifiManager = (WifiManager) getApplicationContext()
                        .getSystemService(Context.WIFI_SERVICE);
                wifiManager.enableNetwork(network, true);
            }
            startTimerDisplay(WIFI_NETWORK_CONNECT_TIMEOUT_MS / 1000);
            NetworkRequest.Builder networkRequestBuilder = new NetworkRequest.Builder()
                    .addTransportType(TRANSPORT_WIFI);
            if (requireInternet) {
                networkRequestBuilder.addCapability(NET_CAPABILITY_INTERNET);
            }
            NetworkRequest networkRequest = networkRequestBuilder.build();
            mWifiNetworkRequested = true;
            mConnectivityManager.requestNetwork(networkRequest, mWifiNetworkCallback,
                    mMainHandler, WIFI_NETWORK_CONNECT_TIMEOUT_MS);
        }

        private void connectToCellularNetwork() {
            NetworkRequest networkRequest = new NetworkRequest.Builder()
                    .addTransportType(TRANSPORT_CELLULAR)
                    .addCapability(NET_CAPABILITY_INTERNET)
                    .build();
            startTimerDisplay(CELLULAR_NETWORK_CONNECT_TIMEOUT_MS / 1000);
            mCellularNetworkRequested = true;
            mConnectivityManager.requestNetwork(networkRequest, mCellularNetworkCallback,
                    mMainHandler, CELLULAR_NETWORK_CONNECT_TIMEOUT_MS);
        }

        private void startTimerDisplay(int timeInSeconds) {
            startTimerCountdownDisplay(timeInSeconds);
            mTimerStartRequested = true;
        }

        /** Timer is a shared resource, change the state only if it's started in a request. */
        private void stopTimerDisplayIfRequested() {
            if (mTimerStartRequested) {
                mTimerStartRequested = false;
                stopTimerCountdownDisplay();
            }
        }
    }

    /**
     * Manage the lifecycle of each test to be run in the validator.
     *
     * Each test goes through this cycle
     * - Start
     * - Connect to cellular network
     * - Connect to wifi network
     * - Check expectation
     * - End test
     */
    private abstract class MultiNetworkValidator {
        final String mTestName;
        final MultinetworkTestCallback mTestCallback;
        final TestConnectivityState mConnectivityState;

        int mTestDescription;
        boolean mTestResult = false;
        ValidatorState mValidatorState;
        int mTestResultMessage = -1;
        int mTestProgressMessage;

        MultiNetworkValidator(MultinetworkTestCallback testCallback,
                String testName, int testDescription) {
            mTestCallback = testCallback;
            mTestName = testName;
            mTestDescription = testDescription;
            mConnectivityState = new TestConnectivityState(this);
            mValidatorState = NOT_STARTED;
        }

        /** Start test if not started. */
        void startTest() {
            if (mValidatorState == NOT_STARTED) {
                mTestCallback.testStarted();
                WifiManager wifiManager = (WifiManager) getApplicationContext()
                        .getSystemService(Context.WIFI_SERVICE);
                wifiManager.setWifiEnabled(false);
                mMainHandler.postDelayed(() -> {
                    wifiManager.setWifiEnabled(true);
                    mTestCallback.testProgress(
                            R.string.multinetwork_connectivity_test_connect_cellular);
                    mConnectivityState.connectToCellularNetwork();
                }, ENABLE_DISABLE_WIFI_DELAY_MS);
            }
        }

        /** Make sure that the state is restored for re-running the test. */
        void reset() {
            mValidatorState = NOT_STARTED;
            mTestResultMessage = -1;
            mTestProgressMessage = -1;
        }

        /** Called when user has requested to continue with the test */
        void continueWithTest() {
            mValidatorState = STARTED;
        }


        void connectToWifi() {
            mTestCallback.testProgress(R.string.multinetwork_connectivity_test_connect_wifi);
            mConnectivityState.connectToWifiNetwork(false);
        }

        void onCellularNetworkUnavailable() {
            endTest(false, R.string.multinetwork_status_mobile_connect_timed_out);
        }

        void endTest(boolean status, int messageResId) {
            Log.i(TAG, "Ending test with status " + status + " message " +
                MultiNetworkConnectivityTestActivity.this.getResources().getString(messageResId));
            mMainHandler.post(() -> {
                mTestResult = status;
                mTestResultMessage = messageResId;
                mValidatorState = COMPLETED;
                mTestCallback.testCompleted(MultiNetworkValidator.this);
                mConnectivityState.reset();
            });
        }

        /** Called when cellular network is connected. */
        void onCellularNetworkConnected(Network network) {
            onContinuePreWifiConnect();
        }

        /**
         * @param transport The active network has this transport type
         * @return
         */
        boolean isExpectedTransportForActiveNetwork(int transport) {
            Network activeNetwork = mConnectivityManager.getActiveNetwork();
            NetworkCapabilities activeNetworkCapabilities =
                    mConnectivityManager.getNetworkCapabilities(activeNetwork);
            Log.i(TAG, "Network capabilities for " + activeNetwork.netId + " "
                    + activeNetworkCapabilities.hasCapability(NET_CAPABILITY_INTERNET));
            return activeNetworkCapabilities.hasTransport(transport)
                    && activeNetworkCapabilities.hasCapability(NET_CAPABILITY_INTERNET);
        }

        /**
         * @param network to check if connected or not.
         * @return
         */
        boolean isNetworkConnected(Network network) {
            NetworkInfo networkInfo = mConnectivityManager.getNetworkInfo(network);
            boolean status = networkInfo != null && networkInfo.isConnectedOrConnecting();
            Log.i(TAG, "Network connection status " + network.netId + " " + status);
            return status;
        }

        /**
         * Called before connecting to wifi. Specially if the concrete validator wants to
         * prompt a message
         */
        abstract void onContinuePreWifiConnect();

        /** Called when a wifi network is connected and available */
        void onWifiNetworkConnected(Network network) {
            Log.i(TAG, "Wifi network connected " + network.netId);
        }

        void onWifiNetworkUnavailable() {
            endTest(false, R.string.multinetwork_status_wifi_connect_timed_out);
        }
    }

    /**
     * Test that device does not lose cellular connectivity when it's connected to an access
     * point with no connectivity.
     */
    private class ConnectToWifiWithNoInternetValidator extends MultiNetworkValidator {

        ConnectToWifiWithNoInternetValidator(int description) {
            super(mMultinetworkTestCallback, "no_internet_test", description);
        }


        @Override
        void continueWithTest() {
            super.continueWithTest();
            connectToWifi();
        }

        @Override
        void onContinuePreWifiConnect() {
            mTestProgressMessage = R.string.multinetwork_connectivity_test_1_prereq;
            mTestCallback.testProgress(mTestProgressMessage);
            mValidatorState = WAITING_FOR_USER_INPUT;
            requestUserConfirmation();
        }

        @Override
        void onWifiNetworkConnected(Network wifiNetwork) {
            super.onWifiNetworkConnected(wifiNetwork);
            if (isConnectedToExpectedWifiNetwork()) {
                startTimerCountdownDisplay(CELLULAR_NETWORK_RESTORE_TIMEOUT_MS / 1000);
                mTestCallback.testProgress(R.string.multinetwork_connectivity_test_progress_2);

                // Wait for CELLULAR_NETWORK_RESTORE_TIMEOUT_MS, before checking if there is still
                // the active network as the cell network.
                mMainHandler.postDelayed(() -> {
                    stopTimerCountdownDisplay();
                    mMainHandler.post(() -> {
                        if (isExpectedTransportForActiveNetwork(TRANSPORT_CELLULAR)
                                && isNetworkConnected(wifiNetwork)) {
                            Log.d(TAG, "PASS test as device has connectivity");
                            endTest(true, R.string.multinetwork_status_mobile_restore_success);
                        } else {
                            Log.d(TAG, "Fail test as device didn't have connectivity");
                            endTest(false, R.string.multinetwork_status_mobile_restore_failed);
                        }
                    });
                }, CELLULAR_NETWORK_RESTORE_TIMEOUT_MS);
            } else {
                endTest(false, R.string.multinetwork_status_wifi_connect_wrong_ap);
            }
        }
    }

    /**
     * Test that device restores lost cellular connectivity when it's connected to an access
     * point which loses internet connectivity.
     */
    private class ConnectToWifiWithIntermittentInternetValidator extends MultiNetworkValidator {
        boolean mWaitingForWifiConnect = false;
        boolean mWaitingForCelluarToConnectBack = false;
        Network mWifiNetwork;

        ConnectToWifiWithIntermittentInternetValidator(int description) {
            super(mMultinetworkTestCallback, "no_internet_test", description);
        }

        @Override
        void continueWithTest() {
            super.continueWithTest();
            if (mWaitingForWifiConnect) {
                connectToWifi();
            } else if (mWaitingForCelluarToConnectBack) {
                mWaitingForCelluarToConnectBack = false;
                waitForConnectivityRestore();
            }
        }

        @Override
        void onContinuePreWifiConnect() {
            mTestProgressMessage = R.string.multinetwork_connectivity_test_2_prereq_1;
            mTestCallback.testProgress(mTestProgressMessage);
            mValidatorState = WAITING_FOR_USER_INPUT;
            mWaitingForWifiConnect = true;
            requestUserConfirmation();
        }

        @Override
        void connectToWifi() {
            mTestCallback.testProgress(R.string.multinetwork_connectivity_test_connect_wifi);
            mConnectivityState.connectToWifiNetwork(true);
        }

        @Override
        void onWifiNetworkConnected(Network wifiNetwork) {
            super.onWifiNetworkConnected(wifiNetwork);
            if (isConnectedToExpectedWifiNetwork()) {
                // If the device is connected to the expected network, then update the wifi
                // network to the latest.
                mWifiNetwork = wifiNetwork;
                // Do further processing only when the test is requesting and waiting for a wifi
                // connection.
                if (mWaitingForWifiConnect) {
                    mWaitingForWifiConnect = false;
                    startTimerCountdownDisplay(WIFI_NETWORK_CONNECT_TO_BE_ACTIVE_MS / 1000);

                    // Wait for WIFI_NETWORK_CONNECT_TO_BE_ACTIVE_MS, before checking
                    // if device has the active network as wifi network..
                    mTestCallback.testProgress(R.string.multinetwork_connectivity_test_progress_2);
                    mMainHandler.postDelayed(() -> {
                        stopTimerCountdownDisplay();
                        // In this case both active and peer are same as Wifi has internet access.
                        if (isExpectedTransportForActiveNetwork(TRANSPORT_WIFI)
                                && isNetworkConnected(mWifiNetwork)) {
                            // Ask the user to turn off wifi on the router and check connectivity.
                            mTestProgressMessage =
                                    R.string.multinetwork_connectivity_test_2_prereq_2;
                            mValidatorState = WAITING_FOR_USER_INPUT;
                            mTestCallback.testProgress(mTestProgressMessage);
                            mWaitingForCelluarToConnectBack = true;
                            requestUserConfirmation();
                        } else {
                            Log.d(TAG, "Fail test as device didn't have connectivity");
                            endTest(false, R.string.multinetwork_status_wifi_connectivity_failed);
                        }
                    }, WIFI_NETWORK_CONNECT_TO_BE_ACTIVE_MS);
                }
            } else {
                endTest(false, R.string.multinetwork_status_wifi_connect_wrong_ap);
            }
        }

        @Override
        void reset() {
            super.reset();
            mWaitingForCelluarToConnectBack = false;
            mWaitingForWifiConnect = false;
            mWifiNetwork = null;
        }

        @Override
        void onWifiNetworkUnavailable() {
            if (mWaitingForWifiConnect) {
                super.onWifiNetworkUnavailable();
            }
        }

        void waitForConnectivityRestore() {
            mTestCallback.testProgress(R.string.multinetwork_connectivity_test_progress_1);
            mConnectivityManager.reportNetworkConnectivity(mWifiNetwork, false);
            startTimerCountdownDisplay(
                    CELLULAR_NETWORK_RESTORE_AFTER_WIFI_INTERNET_LOST_TIMEOUT_MS / 1000);
            // Wait for CELLULAR_NETWORK_RESTORE_AFTER_WIFI_INTERNET_LOST_TIMEOUT_MS,
            // before checking if device now has the active network as cell network.
            mMainHandler.postDelayed(() -> {
                stopTimerCountdownDisplay();
                // Check if device has fallen back to cellular network when it loses internet access
                // in the wifi network.
                if (isExpectedTransportForActiveNetwork(TRANSPORT_CELLULAR)
                        && isNetworkConnected(mWifiNetwork)) {
                    Log.d(TAG, "PASS test as device has connectivity");
                    endTest(true, R.string.multinetwork_status_mobile_restore_success);
                } else {
                    Log.d(TAG, "Fail test as device didn't have connectivity");
                    endTest(false, R.string.multinetwork_status_mobile_restore_failed);
                }
            }, CELLULAR_NETWORK_RESTORE_AFTER_WIFI_INTERNET_LOST_TIMEOUT_MS);
        }
    }
}
