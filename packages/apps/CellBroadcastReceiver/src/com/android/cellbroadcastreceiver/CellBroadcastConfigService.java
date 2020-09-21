/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.cellbroadcastreceiver;

import static com.android.cellbroadcastreceiver.CellBroadcastReceiver.VDBG;

import android.app.IntentService;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.preference.PreferenceManager;
import android.telephony.SmsManager;
import android.telephony.SubscriptionManager;
import android.util.Log;

import com.android.cellbroadcastreceiver.CellBroadcastChannelManager.CellBroadcastChannelRange;
import com.android.internal.annotations.VisibleForTesting;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * This service manages enabling and disabling ranges of message identifiers
 * that the radio should listen for. It operates independently of the other
 * services and runs at boot time and after exiting airplane mode.
 *
 * Note that the entire range of emergency channels is enabled. Test messages
 * and lower priority broadcasts are filtered out in CellBroadcastAlertService
 * if the user has not enabled them in settings.
 *
 * TODO: add notification to re-enable channels after a radio reset.
 */
public class CellBroadcastConfigService extends IntentService {
    private static final String TAG = "CellBroadcastConfigService";

    static final String ACTION_ENABLE_CHANNELS = "ACTION_ENABLE_CHANNELS";

    public CellBroadcastConfigService() {
        super(TAG);          // use class name for worker thread name
    }

    @Override
    protected void onHandleIntent(Intent intent) {
        if (ACTION_ENABLE_CHANNELS.equals(intent.getAction())) {
            try {

                SubscriptionManager subManager = SubscriptionManager.from(getApplicationContext());
                int subId = SubscriptionManager.getDefaultSmsSubscriptionId();
                if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID) {
                    subId = SubscriptionManager.getDefaultSubscriptionId();
                    if (subId == SubscriptionManager.INVALID_SUBSCRIPTION_ID &&
                            subManager != null) {
                        int [] subIds = subManager.getActiveSubscriptionIdList();
                        if (subIds.length != 0) {
                            subId = subIds[0];
                        }
                    }
                }

                if (subManager != null) {
                    // Retrieve all the active sub ids. We only want to enable
                    // cell broadcast on the sub we are interested in and we'll disable
                    // it on other subs so the users will not receive duplicate messages from
                    // multiple carriers (e.g. for multi-sim users).
                    int [] subIds = subManager.getActiveSubscriptionIdList();
                    if (subIds.length != 0)
                    {
                        for (int id : subIds) {
                            SmsManager manager = SmsManager.getSmsManagerForSubscriptionId(id);
                            if (manager != null) {
                                if (id == subId) {
                                    // Enable cell broadcast messages on this sub.
                                    log("Enable CellBroadcast on sub " + id);
                                    setCellBroadcastOnSub(manager, true);
                                }
                                else {
                                    // Disable all cell broadcast message on this sub.
                                    // This is only for multi-sim scenario. For single SIM device
                                    // we should not reach here.
                                    log("Disable CellBroadcast on sub " + id);
                                    setCellBroadcastOnSub(manager, false);
                                }
                            }
                        }
                    }
                    else {
                        // For no sim scenario.
                        SmsManager manager = SmsManager.getDefault();
                        if (manager != null) {
                            setCellBroadcastOnSub(manager, true);
                        }
                    }
                }
            } catch (Exception ex) {
                Log.e(TAG, "exception enabling cell broadcast channels", ex);
            }
        }
    }

    /**
     * Enable/disable cell broadcast messages id on one subscription
     * This includes all ETWS and CMAS alerts.
     * @param manager SMS manager
     * @param enableForSub True if want to enable messages on this sub (e.g default SMS). False
     *                     will disable all messages
     */
    @VisibleForTesting
    public void setCellBroadcastOnSub(SmsManager manager, boolean enableForSub) {

        SharedPreferences prefs = PreferenceManager.getDefaultSharedPreferences(this);

        // boolean for each user preference checkbox, true for checked, false for unchecked
        // Note: If enableAlertsMasterToggle is false, it disables ALL emergency broadcasts
        // except for CMAS presidential. i.e. to receive CMAS severe alerts, both
        // enableAlertsMasterToggle AND enableCmasSevereAlerts must be true.
        boolean enableAlertsMasterToggle = enableForSub && prefs.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_ALERTS_MASTER_TOGGLE, true);

        boolean enableEtwsAlerts = enableAlertsMasterToggle;

        // CMAS Presidential must be always on (See 3GPP TS 22.268 Section 6.2) regardless
        // user's preference
        boolean enablePresidential = enableForSub;

        boolean enableCmasExtremeAlerts = enableAlertsMasterToggle && prefs.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_EXTREME_THREAT_ALERTS, true);

        boolean enableCmasSevereAlerts = enableAlertsMasterToggle && prefs.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_SEVERE_THREAT_ALERTS, true);

        boolean enableCmasAmberAlerts = enableAlertsMasterToggle && prefs.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_CMAS_AMBER_ALERTS, true);

        boolean enableTestAlerts = enableAlertsMasterToggle
                && prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_TEST_ALERTS, false);

        boolean enableAreaUpdateInfoAlerts = Resources.getSystem().getBoolean(
                com.android.internal.R.bool.config_showAreaUpdateInfoSettings)
                && prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_AREA_UPDATE_INFO_ALERTS,
                false);

        // Non-CMAS channels
        boolean enablePublicSafetyMessagesChannelAlerts = enableAlertsMasterToggle
                && prefs.getBoolean(CellBroadcastSettings.KEY_ENABLE_PUBLIC_SAFETY_MESSAGES,
                true);

        boolean enableEmergencyAlerts = enableAlertsMasterToggle && prefs.getBoolean(
                CellBroadcastSettings.KEY_ENABLE_EMERGENCY_ALERTS, true);

        if (VDBG) {
            log("enableAlertsMasterToggle = " + enableAlertsMasterToggle);
            log("enableEtwsAlerts = " + enableEtwsAlerts);
            log("enablePresidential = " + enablePresidential);
            log("enableCmasExtremeAlerts = " + enableCmasExtremeAlerts);
            log("enableCmasSevereAlerts = " + enableCmasExtremeAlerts);
            log("enableCmasAmberAlerts = " + enableCmasAmberAlerts);
            log("enableTestAlerts = " + enableTestAlerts);
            log("enableAreaUpdateInfoAlerts = " + enableAreaUpdateInfoAlerts);
            log("enablePublicSafetyMessagesChannelAlerts = "
                    + enablePublicSafetyMessagesChannelAlerts);
            log("enableEmergencyAlerts = " + enableEmergencyAlerts);
        }

        /** Enable CMAS series messages. */

        // Enable/Disable Presidential messages.
        setCellBroadcastRange(manager, enablePresidential,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.cmas_presidential_alerts_channels_range_strings));

        // Enable/Disable CMAS extreme messages.
        setCellBroadcastRange(manager, enableCmasExtremeAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.cmas_alert_extreme_channels_range_strings));

        // Enable/Disable CMAS severe messages.
        setCellBroadcastRange(manager, enableCmasSevereAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.cmas_alerts_severe_range_strings));

        // Enable/Disable CMAS amber alert messages.
        setCellBroadcastRange(manager, enableCmasAmberAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.cmas_amber_alerts_channels_range_strings));

        // Enable/Disable test messages.
        setCellBroadcastRange(manager, enableTestAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.required_monthly_test_range_strings));
        setCellBroadcastRange(manager, enableTestAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                        R.array.exercise_alert_range_strings));
        setCellBroadcastRange(manager, enableTestAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                        R.array.operator_defined_alert_range_strings));

        // Enable/Disable GSM ETWS messages.
        setCellBroadcastRange(manager, enableEtwsAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.etws_alerts_range_strings));

        // Enable/Disable GSM ETWS test messages.
        setCellBroadcastRange(manager, enableTestAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.etws_test_alerts_range_strings));

        // Enable/Disable GSM public safety messages.
        setCellBroadcastRange(manager, enablePublicSafetyMessagesChannelAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                        R.array.public_safety_messages_channels_range_strings));

        /** Enable non-CMAS series messages. */

        setCellBroadcastRange(manager, enableEmergencyAlerts,
                CellBroadcastChannelManager.getInstance().getCellBroadcastChannelRanges(this,
                R.array.emergency_alerts_channels_range_strings));

        // Enable/Disable additional channels based on carrier specific requirement.
        ArrayList<CellBroadcastChannelRange> ranges = CellBroadcastChannelManager
                .getInstance().getCellBroadcastChannelRanges(this,
                R.array.additional_cbs_channels_strings);
        if (ranges != null) {
            for (CellBroadcastChannelRange range: ranges) {
                boolean enableAlerts;
                switch (range.mAlertType) {
                    case AREA:
                        enableAlerts = enableAreaUpdateInfoAlerts;
                        break;
                    case TEST:
                        enableAlerts = enableTestAlerts;
                        break;
                    default:
                        enableAlerts = enableAlertsMasterToggle;
                }
                setCellBroadcastRange(manager, enableAlerts, new ArrayList<>(Arrays.asList(range)));
            }
        }
    }
    /**
     * Enable/disable cell broadcast with messages id range
     * @param manager SMS manager
     * @param enable True for enabling cell broadcast with id range, otherwise for disabling.
     * @param ranges Cell broadcast id ranges
     */
    private void setCellBroadcastRange(
            SmsManager manager, boolean enable, List<CellBroadcastChannelRange> ranges) {
        if (ranges != null) {
            for (CellBroadcastChannelRange range: ranges) {
                if (enable) {
                    manager.enableCellBroadcastRange(range.mStartId, range.mEndId, range.mRat);
                } else {
                    manager.disableCellBroadcastRange(range.mStartId, range.mEndId, range.mRat);
                }
            }
        }
    }

    private static void log(String msg) {
        Log.d(TAG, msg);
    }
}
