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

package com.android.internal.telephony;

import static android.provider.Telephony.ServiceStateTable.getContentValuesForServiceState;
import static android.provider.Telephony.ServiceStateTable.getUriForSubscriptionId;

import static com.android.internal.telephony.CarrierActionAgent.CARRIER_ACTION_SET_RADIO_ENABLED;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.hardware.radio.V1_0.CellInfoType;
import android.os.AsyncResult;
import android.os.BaseBundle;
import android.os.Build;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.Registrant;
import android.os.RegistrantList;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.WorkSource;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.telephony.AccessNetworkConstants;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.telephony.CarrierConfigManager;
import android.telephony.CellIdentity;
import android.telephony.CellIdentityCdma;
import android.telephony.CellIdentityGsm;
import android.telephony.CellIdentityLte;
import android.telephony.CellIdentityTdscdma;
import android.telephony.CellIdentityWcdma;
import android.telephony.CellInfo;
import android.telephony.CellInfoGsm;
import android.telephony.CellInfoLte;
import android.telephony.CellInfoWcdma;
import android.telephony.CellLocation;
import android.telephony.DataSpecificRegistrationStates;
import android.telephony.NetworkRegistrationState;
import android.telephony.PhysicalChannelConfig;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.SignalStrength;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.telephony.TelephonyManager;
import android.telephony.VoiceSpecificRegistrationStates;
import android.telephony.cdma.CdmaCellLocation;
import android.telephony.gsm.GsmCellLocation;
import android.text.TextUtils;
import android.util.EventLog;
import android.util.LocalLog;
import android.util.Pair;
import android.util.SparseArray;
import android.util.StatsLog;
import android.util.TimeUtils;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.cdma.CdmaSubscriptionSourceManager;
import com.android.internal.telephony.cdma.EriInfo;
import com.android.internal.telephony.dataconnection.DcTracker;
import com.android.internal.telephony.dataconnection.TransportManager;
import com.android.internal.telephony.metrics.TelephonyMetrics;
import com.android.internal.telephony.uicc.IccCardApplicationStatus.AppState;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.RuimRecords;
import com.android.internal.telephony.uicc.SIMRecords;
import com.android.internal.telephony.uicc.UiccCardApplication;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.telephony.util.NotificationChannelController;
import com.android.internal.telephony.util.TimeStampedValue;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.IndentingPrintWriter;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.TimeZone;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.regex.PatternSyntaxException;

/**
 * {@hide}
 */
public class ServiceStateTracker extends Handler {
    static final String LOG_TAG = "SST";
    static final boolean DBG = true;
    private static final boolean VDBG = false;  // STOPSHIP if true

    private static final String PROP_FORCE_ROAMING = "telephony.test.forceRoaming";

    private CommandsInterface mCi;
    private UiccController mUiccController = null;
    private UiccCardApplication mUiccApplcation = null;
    private IccRecords mIccRecords = null;

    private boolean mVoiceCapable;

    public ServiceState mSS;
    private ServiceState mNewSS;

    private static final long LAST_CELL_INFO_LIST_MAX_AGE_MS = 2000;
    private long mLastCellInfoListTime;
    private List<CellInfo> mLastCellInfoList = null;
    private List<PhysicalChannelConfig> mLastPhysicalChannelConfigList = null;

    private SignalStrength mSignalStrength;

    // TODO - this should not be public, right now used externally GsmConnetion.
    public RestrictedState mRestrictedState;

    /**
     * A unique identifier to track requests associated with a poll
     * and ignore stale responses.  The value is a count-down of
     * expected responses in this pollingContext.
     */
    @VisibleForTesting
    public int[] mPollingContext;
    private boolean mDesiredPowerState;

    /**
     * By default, strength polling is enabled.  However, if we're
     * getting unsolicited signal strength updates from the radio, set
     * value to true and don't bother polling any more.
     */
    private boolean mDontPollSignalStrength = false;

    private RegistrantList mVoiceRoamingOnRegistrants = new RegistrantList();
    private RegistrantList mVoiceRoamingOffRegistrants = new RegistrantList();
    private RegistrantList mDataRoamingOnRegistrants = new RegistrantList();
    private RegistrantList mDataRoamingOffRegistrants = new RegistrantList();
    protected RegistrantList mAttachedRegistrants = new RegistrantList();
    protected RegistrantList mDetachedRegistrants = new RegistrantList();
    private RegistrantList mDataRegStateOrRatChangedRegistrants = new RegistrantList();
    private RegistrantList mNetworkAttachedRegistrants = new RegistrantList();
    private RegistrantList mNetworkDetachedRegistrants = new RegistrantList();
    private RegistrantList mPsRestrictEnabledRegistrants = new RegistrantList();
    private RegistrantList mPsRestrictDisabledRegistrants = new RegistrantList();

    /* Radio power off pending flag and tag counter */
    private boolean mPendingRadioPowerOffAfterDataOff = false;
    private int mPendingRadioPowerOffAfterDataOffTag = 0;

    /** Signal strength poll rate. */
    private static final int POLL_PERIOD_MILLIS = 20 * 1000;

    /** Waiting period before recheck gprs and voice registration. */
    public static final int DEFAULT_GPRS_CHECK_PERIOD_MILLIS = 60 * 1000;

    /** GSM events */
    protected static final int EVENT_RADIO_STATE_CHANGED               = 1;
    protected static final int EVENT_NETWORK_STATE_CHANGED             = 2;
    protected static final int EVENT_GET_SIGNAL_STRENGTH               = 3;
    protected static final int EVENT_POLL_STATE_REGISTRATION           = 4;
    protected static final int EVENT_POLL_STATE_GPRS                   = 5;
    protected static final int EVENT_POLL_STATE_OPERATOR               = 6;
    protected static final int EVENT_POLL_SIGNAL_STRENGTH              = 10;
    protected static final int EVENT_NITZ_TIME                         = 11;
    protected static final int EVENT_SIGNAL_STRENGTH_UPDATE            = 12;
    protected static final int EVENT_POLL_STATE_NETWORK_SELECTION_MODE = 14;
    protected static final int EVENT_GET_LOC_DONE                      = 15;
    protected static final int EVENT_SIM_RECORDS_LOADED                = 16;
    protected static final int EVENT_SIM_READY                         = 17;
    protected static final int EVENT_LOCATION_UPDATES_ENABLED          = 18;
    protected static final int EVENT_GET_PREFERRED_NETWORK_TYPE        = 19;
    protected static final int EVENT_SET_PREFERRED_NETWORK_TYPE        = 20;
    protected static final int EVENT_RESET_PREFERRED_NETWORK_TYPE      = 21;
    protected static final int EVENT_CHECK_REPORT_GPRS                 = 22;
    protected static final int EVENT_RESTRICTED_STATE_CHANGED          = 23;

    /** CDMA events */
    protected static final int EVENT_RUIM_READY                        = 26;
    protected static final int EVENT_RUIM_RECORDS_LOADED               = 27;
    protected static final int EVENT_POLL_STATE_CDMA_SUBSCRIPTION      = 34;
    protected static final int EVENT_NV_READY                          = 35;
    protected static final int EVENT_ERI_FILE_LOADED                   = 36;
    protected static final int EVENT_OTA_PROVISION_STATUS_CHANGE       = 37;
    protected static final int EVENT_SET_RADIO_POWER_OFF               = 38;
    protected static final int EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED  = 39;
    protected static final int EVENT_CDMA_PRL_VERSION_CHANGED          = 40;

    protected static final int EVENT_RADIO_ON                          = 41;
    public    static final int EVENT_ICC_CHANGED                       = 42;
    protected static final int EVENT_GET_CELL_INFO_LIST                = 43;
    protected static final int EVENT_UNSOL_CELL_INFO_LIST              = 44;
    protected static final int EVENT_CHANGE_IMS_STATE                  = 45;
    protected static final int EVENT_IMS_STATE_CHANGED                 = 46;
    protected static final int EVENT_IMS_STATE_DONE                    = 47;
    protected static final int EVENT_IMS_CAPABILITY_CHANGED            = 48;
    protected static final int EVENT_ALL_DATA_DISCONNECTED             = 49;
    protected static final int EVENT_PHONE_TYPE_SWITCHED               = 50;
    protected static final int EVENT_RADIO_POWER_FROM_CARRIER          = 51;
    protected static final int EVENT_SIM_NOT_INSERTED                  = 52;
    protected static final int EVENT_IMS_SERVICE_STATE_CHANGED         = 53;
    protected static final int EVENT_RADIO_POWER_OFF_DONE              = 54;
    protected static final int EVENT_PHYSICAL_CHANNEL_CONFIG           = 55;

    private class CellInfoResult {
        List<CellInfo> list;
        Object lockObj = new Object();
    }

    /** Reason for registration denial. */
    protected static final String REGISTRATION_DENIED_GEN  = "General";
    protected static final String REGISTRATION_DENIED_AUTH = "Authentication Failure";

    private boolean mImsRegistrationOnOff = false;
    private boolean mAlarmSwitch = false;
    /** Radio is disabled by carrier. Radio power will not be override if this field is set */
    private boolean mRadioDisabledByCarrier = false;
    private PendingIntent mRadioOffIntent = null;
    private static final String ACTION_RADIO_OFF = "android.intent.action.ACTION_RADIO_OFF";
    private boolean mPowerOffDelayNeed = true;
    private boolean mDeviceShuttingDown = false;
    /** Keep track of SPN display rules, so we only broadcast intent if something changes. */
    private boolean mSpnUpdatePending = false;
    private String mCurSpn = null;
    private String mCurDataSpn = null;
    private String mCurPlmn = null;
    private boolean mCurShowPlmn = false;
    private boolean mCurShowSpn = false;
    @VisibleForTesting
    public int mSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    private int mPrevSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;

    private boolean mImsRegistered = false;

    private SubscriptionManager mSubscriptionManager;
    private SubscriptionController mSubscriptionController;
    private final SstSubscriptionsChangedListener mOnSubscriptionsChangedListener =
        new SstSubscriptionsChangedListener();


    private final RatRatcheter mRatRatcheter;

    private final HandlerThread mHandlerThread;
    private final LocaleTracker mLocaleTracker;

    private final LocalLog mRoamingLog = new LocalLog(10);
    private final LocalLog mAttachLog = new LocalLog(10);
    private final LocalLog mPhoneTypeLog = new LocalLog(10);
    private final LocalLog mRatLog = new LocalLog(20);
    private final LocalLog mRadioPowerLog = new LocalLog(20);

    private class SstSubscriptionsChangedListener extends OnSubscriptionsChangedListener {
        public final AtomicInteger mPreviousSubId =
                new AtomicInteger(SubscriptionManager.INVALID_SUBSCRIPTION_ID);

        /**
         * Callback invoked when there is any change to any SubscriptionInfo. Typically
         * this method would invoke {@link SubscriptionManager#getActiveSubscriptionInfoList}
         */
        @Override
        public void onSubscriptionsChanged() {
            if (DBG) log("SubscriptionListener.onSubscriptionInfoChanged");
            // Set the network type, in case the radio does not restore it.
            int subId = mPhone.getSubId();
            ServiceStateTracker.this.mPrevSubId = mPreviousSubId.get();
            if (mPreviousSubId.getAndSet(subId) != subId) {
                if (SubscriptionManager.isValidSubscriptionId(subId)) {
                    Context context = mPhone.getContext();

                    mPhone.notifyPhoneStateChanged();
                    mPhone.notifyCallForwardingIndicator();

                    boolean restoreSelection = !context.getResources().getBoolean(
                            com.android.internal.R.bool.skip_restoring_network_selection);
                    mPhone.sendSubscriptionSettings(restoreSelection);

                    mPhone.setSystemProperty(TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE,
                            ServiceState.rilRadioTechnologyToString(
                                    mSS.getRilDataRadioTechnology()));

                    if (mSpnUpdatePending) {
                        mSubscriptionController.setPlmnSpn(mPhone.getPhoneId(), mCurShowPlmn,
                                mCurPlmn, mCurShowSpn, mCurSpn);
                        mSpnUpdatePending = false;
                    }

                    // Remove old network selection sharedPreferences since SP key names are now
                    // changed to include subId. This will be done only once when upgrading from an
                    // older build that did not include subId in the names.
                    SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(
                            context);
                    String oldNetworkSelection = sp.getString(
                            Phone.NETWORK_SELECTION_KEY, "");
                    String oldNetworkSelectionName = sp.getString(
                            Phone.NETWORK_SELECTION_NAME_KEY, "");
                    String oldNetworkSelectionShort = sp.getString(
                            Phone.NETWORK_SELECTION_SHORT_KEY, "");
                    if (!TextUtils.isEmpty(oldNetworkSelection) ||
                            !TextUtils.isEmpty(oldNetworkSelectionName) ||
                            !TextUtils.isEmpty(oldNetworkSelectionShort)) {
                        SharedPreferences.Editor editor = sp.edit();
                        editor.putString(Phone.NETWORK_SELECTION_KEY + subId,
                                oldNetworkSelection);
                        editor.putString(Phone.NETWORK_SELECTION_NAME_KEY + subId,
                                oldNetworkSelectionName);
                        editor.putString(Phone.NETWORK_SELECTION_SHORT_KEY + subId,
                                oldNetworkSelectionShort);
                        editor.remove(Phone.NETWORK_SELECTION_KEY);
                        editor.remove(Phone.NETWORK_SELECTION_NAME_KEY);
                        editor.remove(Phone.NETWORK_SELECTION_SHORT_KEY);
                        editor.commit();
                    }

                    // Once sub id becomes valid, we need to update the service provider name
                    // displayed on the UI again. The old SPN update intents sent to
                    // MobileSignalController earlier were actually ignored due to invalid sub id.
                    updateSpnDisplay();
                }
                // update voicemail count and notify message waiting changed
                mPhone.updateVoiceMail();

                // cancel notifications if we see SIM_NOT_INSERTED (This happens on bootup before
                // the SIM is first detected and then subsequently on SIM removals)
                if (mSubscriptionController.getSlotIndex(subId)
                        == SubscriptionManager.SIM_NOT_INSERTED) {
                    // this is handled on the main thread to mitigate racing with setNotification().
                    sendMessage(obtainMessage(EVENT_SIM_NOT_INSERTED));
                }
            }
        }
    };

    //Common
    private final GsmCdmaPhone mPhone;

    public CellLocation mCellLoc;
    private CellLocation mNewCellLoc;
    private static final int MS_PER_HOUR = 60 * 60 * 1000;
    private final NitzStateMachine mNitzState;
    private final ContentResolver mCr;

    //GSM
    private int mPreferredNetworkType;
    private int mMaxDataCalls = 1;
    private int mNewMaxDataCalls = 1;
    private int mReasonDataDenied = -1;
    private int mNewReasonDataDenied = -1;

    /**
     * The code of the rejection cause that is sent by network when the CS
     * registration is rejected. It should be shown to the user as a notification.
     */
    private int mRejectCode;
    private int mNewRejectCode;

    /**
     * GSM roaming status solely based on TS 27.007 7.2 CREG. Only used by
     * handlePollStateResult to store CREG roaming result.
     */
    private boolean mGsmRoaming = false;
    /**
     * Data roaming status solely based on TS 27.007 10.1.19 CGREG. Only used by
     * handlePollStateResult to store CGREG roaming result.
     */
    private boolean mDataRoaming = false;
    /**
     * Mark when service state is in emergency call only mode
     */
    private boolean mEmergencyOnly = false;
    /** Started the recheck process after finding gprs should registered but not. */
    private boolean mStartedGprsRegCheck;
    /** Already sent the event-log for no gprs register. */
    private boolean mReportedGprsNoReg;

    private CarrierServiceStateTracker mCSST;
    /**
     * The Notification object given to the NotificationManager.
     */
    private Notification mNotification;
    /** Notification type. */
    public static final int PS_ENABLED = 1001;            // Access Control blocks data service
    public static final int PS_DISABLED = 1002;           // Access Control enables data service
    public static final int CS_ENABLED = 1003;            // Access Control blocks all voice/sms service
    public static final int CS_DISABLED = 1004;           // Access Control enables all voice/sms service
    public static final int CS_NORMAL_ENABLED = 1005;     // Access Control blocks normal voice/sms service
    public static final int CS_EMERGENCY_ENABLED = 1006;  // Access Control blocks emergency call service
    public static final int CS_REJECT_CAUSE_ENABLED = 2001;     // Notify MM rejection cause
    /** Notification id. */
    public static final int PS_NOTIFICATION = 888;  // Id to update and cancel PS restricted
    public static final int CS_NOTIFICATION = 999;  // Id to update and cancel CS restricted
    public static final int CS_REJECT_CAUSE_NOTIFICATION = 111; // Id to update and cancel MM
                                                                // rejection cause

    /** To identify whether EVENT_SIM_READY is received or not */
    private boolean mIsSimReady = false;

    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(
                    CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED)) {
                onCarrierConfigChanged();
                return;
            }

            if (!mPhone.isPhoneTypeGsm()) {
                loge("Ignoring intent " + intent + " received on CDMA phone");
                return;
            }

            if (intent.getAction().equals(Intent.ACTION_LOCALE_CHANGED)) {
                // update emergency string whenever locale changed
                updateSpnDisplay();
            } else if (intent.getAction().equals(ACTION_RADIO_OFF)) {
                mAlarmSwitch = false;
                DcTracker dcTracker = mPhone.mDcTracker;
                powerOffRadioSafely(dcTracker);
            }
        }
    };

    //CDMA
    // Min values used to by getOtasp()
    public static final String UNACTIVATED_MIN2_VALUE = "000000";
    public static final String UNACTIVATED_MIN_VALUE = "1111110111";
    // Current Otasp value
    private int mCurrentOtaspMode = TelephonyManager.OTASP_UNINITIALIZED;
    private int mRoamingIndicator;
    private boolean mIsInPrl;
    private int mDefaultRoamingIndicator;
    /**
     * Initially assume no data connection.
     */
    private int mRegistrationState = -1;
    private RegistrantList mCdmaForSubscriptionInfoReadyRegistrants = new RegistrantList();
    private String mMdn;
    private int mHomeSystemId[] = null;
    private int mHomeNetworkId[] = null;
    private String mMin;
    private String mPrlVersion;
    private boolean mIsMinInfoReady = false;
    private boolean mIsEriTextLoaded = false;
    private boolean mIsSubscriptionFromRuim = false;
    private CdmaSubscriptionSourceManager mCdmaSSM;
    public static final String INVALID_MCC = "000";
    public static final String DEFAULT_MNC = "00";
    private HbpcdUtils mHbpcdUtils = null;
    /* Used only for debugging purposes. */
    private String mRegistrationDeniedReason;
    private String mCurrentCarrier = null;

    private final TransportManager mTransportManager;
    private final SparseArray<NetworkRegistrationManager> mRegStateManagers = new SparseArray<>();

    /* list of LTE EARFCNs (E-UTRA Absolute Radio Frequency Channel Number,
     * Reference: 3GPP TS 36.104 5.4.3)
     * inclusive ranges for which the lte rsrp boost is applied */
    private ArrayList<Pair<Integer, Integer>> mEarfcnPairListForRsrpBoost = null;

    private int mLteRsrpBoost = 0; // offset which is reduced from the rsrp threshold
                                   // while calculating signal strength level.
    private final Object mLteRsrpBoostLock = new Object();
    private static final int INVALID_LTE_EARFCN = -1;

    public ServiceStateTracker(GsmCdmaPhone phone, CommandsInterface ci) {
        mNitzState = TelephonyComponentFactory.getInstance().makeNitzStateMachine(phone);
        mPhone = phone;
        mCi = ci;

        mRatRatcheter = new RatRatcheter(mPhone);
        mVoiceCapable = mPhone.getContext().getResources().getBoolean(
                com.android.internal.R.bool.config_voice_capable);
        mUiccController = UiccController.getInstance();

        mUiccController.registerForIccChanged(this, EVENT_ICC_CHANGED, null);
        mCi.setOnSignalStrengthUpdate(this, EVENT_SIGNAL_STRENGTH_UPDATE, null);
        mCi.registerForCellInfoList(this, EVENT_UNSOL_CELL_INFO_LIST, null);
        mCi.registerForPhysicalChannelConfiguration(this, EVENT_PHYSICAL_CHANNEL_CONFIG, null);

        mSubscriptionController = SubscriptionController.getInstance();
        mSubscriptionManager = SubscriptionManager.from(phone.getContext());
        mSubscriptionManager
                .addOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);
        mRestrictedState = new RestrictedState();

        mTransportManager = new TransportManager();

        for (int transportType : mTransportManager.getAvailableTransports()) {
            mRegStateManagers.append(transportType, new NetworkRegistrationManager(
                    transportType, phone));
            mRegStateManagers.get(transportType).registerForNetworkRegistrationStateChanged(
                    this, EVENT_NETWORK_STATE_CHANGED, null);
        }

        // Create a new handler thread dedicated for locale tracker because the blocking
        // getAllCellInfo call requires clients calling from a different thread.
        mHandlerThread = new HandlerThread(LocaleTracker.class.getSimpleName());
        mHandlerThread.start();
        mLocaleTracker = TelephonyComponentFactory.getInstance().makeLocaleTracker(
                mPhone, mHandlerThread.getLooper());

        mCi.registerForImsNetworkStateChanged(this, EVENT_IMS_STATE_CHANGED, null);
        mCi.registerForRadioStateChanged(this, EVENT_RADIO_STATE_CHANGED, null);
        mCi.setOnNITZTime(this, EVENT_NITZ_TIME, null);

        mCr = phone.getContext().getContentResolver();
        // system setting property AIRPLANE_MODE_ON is set in Settings.
        int airplaneMode = Settings.Global.getInt(mCr, Settings.Global.AIRPLANE_MODE_ON, 0);
        int enableCellularOnBoot = Settings.Global.getInt(mCr,
                Settings.Global.ENABLE_CELLULAR_ON_BOOT, 1);
        mDesiredPowerState = (enableCellularOnBoot > 0) && ! (airplaneMode > 0);
        mRadioPowerLog.log("init : airplane mode = " + airplaneMode + " enableCellularOnBoot = " +
                enableCellularOnBoot);


        setSignalStrengthDefaultValues();
        mPhone.getCarrierActionAgent().registerForCarrierAction(CARRIER_ACTION_SET_RADIO_ENABLED,
                this, EVENT_RADIO_POWER_FROM_CARRIER, null, false);

        // Monitor locale change
        Context context = mPhone.getContext();
        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_LOCALE_CHANGED);
        context.registerReceiver(mIntentReceiver, filter);
        filter = new IntentFilter();
        filter.addAction(ACTION_RADIO_OFF);
        context.registerReceiver(mIntentReceiver, filter);
        filter = new IntentFilter();
        filter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);
        context.registerReceiver(mIntentReceiver, filter);

        mPhone.notifyOtaspChanged(TelephonyManager.OTASP_UNINITIALIZED);

        mCi.setOnRestrictedStateChanged(this, EVENT_RESTRICTED_STATE_CHANGED, null);
        updatePhoneType();

        mCSST = new CarrierServiceStateTracker(phone, this);

        registerForNetworkAttached(mCSST,
                CarrierServiceStateTracker.CARRIER_EVENT_VOICE_REGISTRATION, null);
        registerForNetworkDetached(mCSST,
                CarrierServiceStateTracker.CARRIER_EVENT_VOICE_DEREGISTRATION, null);
        registerForDataConnectionAttached(mCSST,
                CarrierServiceStateTracker.CARRIER_EVENT_DATA_REGISTRATION, null);
        registerForDataConnectionDetached(mCSST,
                CarrierServiceStateTracker.CARRIER_EVENT_DATA_DEREGISTRATION, null);
    }

    @VisibleForTesting
    public void updatePhoneType() {
        // If we are previously voice roaming, we need to notify that roaming status changed before
        // we change back to non-roaming.
        if (mSS != null && mSS.getVoiceRoaming()) {
            mVoiceRoamingOffRegistrants.notifyRegistrants();
        }

        // If we are previously data roaming, we need to notify that roaming status changed before
        // we change back to non-roaming.
        if (mSS != null && mSS.getDataRoaming()) {
            mDataRoamingOffRegistrants.notifyRegistrants();
        }

        // If we are previously in service, we need to notify that we are out of service now.
        if (mSS != null && mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE) {
            mNetworkDetachedRegistrants.notifyRegistrants();
        }

        // If we are previously in service, we need to notify that we are out of service now.
        if (mSS != null && mSS.getDataRegState() == ServiceState.STATE_IN_SERVICE) {
            mDetachedRegistrants.notifyRegistrants();
        }

        mSS = new ServiceState();
        mNewSS = new ServiceState();
        mLastCellInfoListTime = 0;
        mLastCellInfoList = null;
        mSignalStrength = new SignalStrength();
        mStartedGprsRegCheck = false;
        mReportedGprsNoReg = false;
        mMdn = null;
        mMin = null;
        mPrlVersion = null;
        mIsMinInfoReady = false;
        mNitzState.handleNetworkUnavailable();

        //cancel any pending pollstate request on voice tech switching
        cancelPollState();

        if (mPhone.isPhoneTypeGsm()) {
            //clear CDMA registrations first
            if (mCdmaSSM != null) {
                mCdmaSSM.dispose(this);
            }

            mCi.unregisterForCdmaPrlChanged(this);
            mPhone.unregisterForEriFileLoaded(this);
            mCi.unregisterForCdmaOtaProvision(this);
            mPhone.unregisterForSimRecordsLoaded(this);

            mCellLoc = new GsmCellLocation();
            mNewCellLoc = new GsmCellLocation();
        } else {
            mPhone.registerForSimRecordsLoaded(this, EVENT_SIM_RECORDS_LOADED, null);
            mCellLoc = new CdmaCellLocation();
            mNewCellLoc = new CdmaCellLocation();
            mCdmaSSM = CdmaSubscriptionSourceManager.getInstance(mPhone.getContext(), mCi, this,
                    EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED, null);
            mIsSubscriptionFromRuim = (mCdmaSSM.getCdmaSubscriptionSource() ==
                    CdmaSubscriptionSourceManager.SUBSCRIPTION_FROM_RUIM);

            mCi.registerForCdmaPrlChanged(this, EVENT_CDMA_PRL_VERSION_CHANGED, null);
            mPhone.registerForEriFileLoaded(this, EVENT_ERI_FILE_LOADED, null);
            mCi.registerForCdmaOtaProvision(this, EVENT_OTA_PROVISION_STATUS_CHANGE, null);

            mHbpcdUtils = new HbpcdUtils(mPhone.getContext());
            // update OTASP state in case previously set by another service
            updateOtaspState();
        }

        // This should be done after the technology specific initializations above since it relies
        // on fields like mIsSubscriptionFromRuim (which is updated above)
        onUpdateIccAvailability();

        mPhone.setSystemProperty(TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE,
                ServiceState.rilRadioTechnologyToString(ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN));
        // Query signal strength from the modem after service tracker is created (i.e. boot up,
        // switching between GSM and CDMA phone), because the unsolicited signal strength
        // information might come late or even never come. This will get the accurate signal
        // strength information displayed on the UI.
        mCi.getSignalStrength(obtainMessage(EVENT_GET_SIGNAL_STRENGTH));
        sendMessage(obtainMessage(EVENT_PHONE_TYPE_SWITCHED));

        logPhoneTypeChange();

        // Tell everybody that the registration state and RAT have changed.
        notifyDataRegStateRilRadioTechnologyChanged();
    }

    @VisibleForTesting
    public void requestShutdown() {
        if (mDeviceShuttingDown == true) return;
        mDeviceShuttingDown = true;
        mDesiredPowerState = false;
        setPowerStateToDesired();
    }

    public void dispose() {
        mCi.unSetOnSignalStrengthUpdate(this);
        mUiccController.unregisterForIccChanged(this);
        mCi.unregisterForCellInfoList(this);
        mCi.unregisterForPhysicalChannelConfiguration(this);
        mSubscriptionManager
            .removeOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);
        mHandlerThread.quit();
        mCi.unregisterForImsNetworkStateChanged(this);
        mPhone.getCarrierActionAgent().unregisterForCarrierAction(this,
                CARRIER_ACTION_SET_RADIO_ENABLED);
        if (mCSST != null) {
            mCSST.dispose();
            mCSST = null;
        }
    }

    public boolean getDesiredPowerState() {
        return mDesiredPowerState;
    }
    public boolean getPowerStateFromCarrier() { return !mRadioDisabledByCarrier; }

    private SignalStrength mLastSignalStrength = null;
    protected boolean notifySignalStrength() {
        boolean notified = false;
        if (!mSignalStrength.equals(mLastSignalStrength)) {
            try {
                mPhone.notifySignalStrength();
                notified = true;
                mLastSignalStrength = mSignalStrength;
            } catch (NullPointerException ex) {
                loge("updateSignalStrength() Phone already destroyed: " + ex
                        + "SignalStrength not notified");
            }
        }
        return notified;
    }

    /**
     * Notify all mDataConnectionRatChangeRegistrants using an
     * AsyncResult in msg.obj where AsyncResult#result contains the
     * new RAT as an Integer Object.
     */
    protected void notifyDataRegStateRilRadioTechnologyChanged() {
        int rat = mSS.getRilDataRadioTechnology();
        int drs = mSS.getDataRegState();
        if (DBG) log("notifyDataRegStateRilRadioTechnologyChanged: drs=" + drs + " rat=" + rat);

        mPhone.setSystemProperty(TelephonyProperties.PROPERTY_DATA_NETWORK_TYPE,
                ServiceState.rilRadioTechnologyToString(rat));
        mDataRegStateOrRatChangedRegistrants.notifyResult(new Pair<Integer, Integer>(drs, rat));
    }

    /**
     * Some operators have been known to report registration failure
     * data only devices, to fix that use DataRegState.
     */
    protected void useDataRegStateForDataOnlyDevices() {
        if (mVoiceCapable == false) {
            if (DBG) {
                log("useDataRegStateForDataOnlyDevice: VoiceRegState=" + mNewSS.getVoiceRegState()
                    + " DataRegState=" + mNewSS.getDataRegState());
            }
            // TODO: Consider not lying and instead have callers know the difference.
            mNewSS.setVoiceRegState(mNewSS.getDataRegState());
        }
    }

    protected void updatePhoneObject() {
        if (mPhone.getContext().getResources().
                getBoolean(com.android.internal.R.bool.config_switch_phone_on_voice_reg_state_change)) {
            // If the phone is not registered on a network, no need to update.
            boolean isRegistered = mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE ||
                    mSS.getVoiceRegState() == ServiceState.STATE_EMERGENCY_ONLY;
            if (!isRegistered) {
                log("updatePhoneObject: Ignore update");
                return;
            }
            mPhone.updatePhoneObject(mSS.getRilVoiceRadioTechnology());
        }
    }

    /**
     * Registration point for combined roaming on of mobile voice
     * combined roaming is true when roaming is true and ONS differs SPN
     *
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForVoiceRoamingOn(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mVoiceRoamingOnRegistrants.add(r);

        if (mSS.getVoiceRoaming()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForVoiceRoamingOn(Handler h) {
        mVoiceRoamingOnRegistrants.remove(h);
    }

    /**
     * Registration point for roaming off of mobile voice
     * combined roaming is true when roaming is true and ONS differs SPN
     *
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForVoiceRoamingOff(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mVoiceRoamingOffRegistrants.add(r);

        if (!mSS.getVoiceRoaming()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForVoiceRoamingOff(Handler h) {
        mVoiceRoamingOffRegistrants.remove(h);
    }

    /**
     * Registration point for combined roaming on of mobile data
     * combined roaming is true when roaming is true and ONS differs SPN
     *
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForDataRoamingOn(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mDataRoamingOnRegistrants.add(r);

        if (mSS.getDataRoaming()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForDataRoamingOn(Handler h) {
        mDataRoamingOnRegistrants.remove(h);
    }

    /**
     * Registration point for roaming off of mobile data
     * combined roaming is true when roaming is true and ONS differs SPN
     *
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     * @param notifyNow notify upon registration if data roaming is off
     */
    public void registerForDataRoamingOff(Handler h, int what, Object obj, boolean notifyNow) {
        Registrant r = new Registrant(h, what, obj);
        mDataRoamingOffRegistrants.add(r);

        if (notifyNow && !mSS.getDataRoaming()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForDataRoamingOff(Handler h) {
        mDataRoamingOffRegistrants.remove(h);
    }

    /**
     * Re-register network by toggling preferred network type.
     * This is a work-around to deregister and register network since there is
     * no ril api to set COPS=2 (deregister) only.
     *
     * @param onComplete is dispatched when this is complete.  it will be
     * an AsyncResult, and onComplete.obj.exception will be non-null
     * on failure.
     */
    public void reRegisterNetwork(Message onComplete) {
        mCi.getPreferredNetworkType(
                obtainMessage(EVENT_GET_PREFERRED_NETWORK_TYPE, onComplete));
    }

    public void
    setRadioPower(boolean power) {
        mDesiredPowerState = power;

        setPowerStateToDesired();
    }

    /**
     * Radio power set from carrier action. if set to false means carrier desire to turn radio off
     * and radio wont be re-enabled unless carrier explicitly turn it back on.
     * @param enable indicate if radio power is enabled or disabled from carrier action.
     */
    public void setRadioPowerFromCarrier(boolean enable) {
        mRadioDisabledByCarrier = !enable;
        setPowerStateToDesired();
    }

    /**
     * These two flags manage the behavior of the cell lock -- the
     * lock should be held if either flag is true.  The intention is
     * to allow temporary acquisition of the lock to get a single
     * update.  Such a lock grab and release can thus be made to not
     * interfere with more permanent lock holds -- in other words, the
     * lock will only be released if both flags are false, and so
     * releases by temporary users will only affect the lock state if
     * there is no continuous user.
     */
    private boolean mWantContinuousLocationUpdates;
    private boolean mWantSingleLocationUpdate;

    public void enableSingleLocationUpdate() {
        if (mWantSingleLocationUpdate || mWantContinuousLocationUpdates) return;
        mWantSingleLocationUpdate = true;
        mCi.setLocationUpdates(true, obtainMessage(EVENT_LOCATION_UPDATES_ENABLED));
    }

    public void enableLocationUpdates() {
        if (mWantSingleLocationUpdate || mWantContinuousLocationUpdates) return;
        mWantContinuousLocationUpdates = true;
        mCi.setLocationUpdates(true, obtainMessage(EVENT_LOCATION_UPDATES_ENABLED));
    }

    protected void disableSingleLocationUpdate() {
        mWantSingleLocationUpdate = false;
        if (!mWantSingleLocationUpdate && !mWantContinuousLocationUpdates) {
            mCi.setLocationUpdates(false, null);
        }
    }

    public void disableLocationUpdates() {
        mWantContinuousLocationUpdates = false;
        if (!mWantSingleLocationUpdate && !mWantContinuousLocationUpdates) {
            mCi.setLocationUpdates(false, null);
        }
    }

    private void processCellLocationInfo(CellLocation cellLocation, CellIdentity cellIdentity) {
        if (mPhone.isPhoneTypeGsm()) {
            int psc = -1;
            int cid = -1;
            int lac = -1;
            if (cellIdentity != null) {
                switch (cellIdentity.getType()) {
                    case CellInfoType.GSM: {
                        cid = ((CellIdentityGsm) cellIdentity).getCid();
                        lac = ((CellIdentityGsm) cellIdentity).getLac();
                        break;
                    }
                    case CellInfoType.WCDMA: {
                        cid = ((CellIdentityWcdma) cellIdentity).getCid();
                        lac = ((CellIdentityWcdma) cellIdentity).getLac();
                        psc = ((CellIdentityWcdma) cellIdentity).getPsc();
                        break;
                    }
                    case CellInfoType.TD_SCDMA: {
                        cid = ((CellIdentityTdscdma) cellIdentity).getCid();
                        lac = ((CellIdentityTdscdma) cellIdentity).getLac();
                        break;
                    }
                    case CellInfoType.LTE: {
                        cid = ((CellIdentityLte) cellIdentity).getCi();
                        lac = ((CellIdentityLte) cellIdentity).getTac();
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            // LAC and CID are -1 if not avail
            ((GsmCellLocation) cellLocation).setLacAndCid(lac, cid);
            ((GsmCellLocation) cellLocation).setPsc(psc);
        } else {
            int baseStationId = -1;
            int baseStationLatitude = CdmaCellLocation.INVALID_LAT_LONG;
            int baseStationLongitude = CdmaCellLocation.INVALID_LAT_LONG;
            int systemId = 0;
            int networkId = 0;

            if (cellIdentity != null) {
                switch (cellIdentity.getType()) {
                    case CellInfoType.CDMA: {
                        baseStationId = ((CellIdentityCdma) cellIdentity).getBasestationId();
                        baseStationLatitude = ((CellIdentityCdma) cellIdentity).getLatitude();
                        baseStationLongitude = ((CellIdentityCdma) cellIdentity).getLongitude();
                        systemId = ((CellIdentityCdma) cellIdentity).getSystemId();
                        networkId = ((CellIdentityCdma) cellIdentity).getNetworkId();
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }

            // Some carriers only return lat-lngs of 0,0
            if (baseStationLatitude == 0 && baseStationLongitude == 0) {
                baseStationLatitude  = CdmaCellLocation.INVALID_LAT_LONG;
                baseStationLongitude = CdmaCellLocation.INVALID_LAT_LONG;
            }

            // Values are -1 if not available.
            ((CdmaCellLocation) cellLocation).setCellLocationData(baseStationId,
                    baseStationLatitude, baseStationLongitude, systemId, networkId);
        }
    }

    private int getLteEarfcn(CellIdentity cellIdentity) {
        int lteEarfcn = INVALID_LTE_EARFCN;
        if (cellIdentity != null) {
            switch (cellIdentity.getType()) {
                case CellInfoType.LTE: {
                    lteEarfcn = ((CellIdentityLte) cellIdentity).getEarfcn();
                    break;
                }
                default: {
                    break;
                }
            }
        }

        return lteEarfcn;
    }

    @Override
    public void handleMessage(Message msg) {
        AsyncResult ar;
        int[] ints;
        Message message;

        if (VDBG) log("received event " + msg.what);
        switch (msg.what) {
            case EVENT_SET_RADIO_POWER_OFF:
                synchronized(this) {
                    if (mPendingRadioPowerOffAfterDataOff &&
                            (msg.arg1 == mPendingRadioPowerOffAfterDataOffTag)) {
                        if (DBG) log("EVENT_SET_RADIO_OFF, turn radio off now.");
                        hangupAndPowerOff();
                        mPendingRadioPowerOffAfterDataOffTag += 1;
                        mPendingRadioPowerOffAfterDataOff = false;
                    } else {
                        log("EVENT_SET_RADIO_OFF is stale arg1=" + msg.arg1 +
                                "!= tag=" + mPendingRadioPowerOffAfterDataOffTag);
                    }
                }
                break;

            case EVENT_ICC_CHANGED:
                onUpdateIccAvailability();
                if (mUiccApplcation != null
                        && mUiccApplcation.getState() != AppState.APPSTATE_READY) {
                    mIsSimReady = false;
                    updateSpnDisplay();
                }
                break;

            case EVENT_GET_CELL_INFO_LIST: {
                ar = (AsyncResult) msg.obj;
                CellInfoResult result = (CellInfoResult) ar.userObj;
                synchronized(result.lockObj) {
                    if (ar.exception != null) {
                        log("EVENT_GET_CELL_INFO_LIST: error ret null, e=" + ar.exception);
                        result.list = null;
                    } else {
                        result.list = (List<CellInfo>) ar.result;

                        if (VDBG) {
                            log("EVENT_GET_CELL_INFO_LIST: size=" + result.list.size()
                                    + " list=" + result.list);
                        }
                    }
                    mLastCellInfoListTime = SystemClock.elapsedRealtime();
                    mLastCellInfoList = result.list;
                    result.lockObj.notify();
                }
                break;
            }

            case EVENT_UNSOL_CELL_INFO_LIST: {
                ar = (AsyncResult) msg.obj;
                if (ar.exception != null) {
                    log("EVENT_UNSOL_CELL_INFO_LIST: error ignoring, e=" + ar.exception);
                } else {
                    List<CellInfo> list = (List<CellInfo>) ar.result;
                    if (VDBG) {
                        log("EVENT_UNSOL_CELL_INFO_LIST: size=" + list.size() + " list=" + list);
                    }
                    mLastCellInfoListTime = SystemClock.elapsedRealtime();
                    mLastCellInfoList = list;
                    mPhone.notifyCellInfo(list);
                }
                break;
            }

            case  EVENT_IMS_STATE_CHANGED: // received unsol
                mCi.getImsRegistrationState(this.obtainMessage(EVENT_IMS_STATE_DONE));
                break;

            case EVENT_IMS_STATE_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    int[] responseArray = (int[])ar.result;
                    mImsRegistered = (responseArray[0] == 1) ? true : false;
                }
                break;

            case EVENT_RADIO_POWER_OFF_DONE:
                if (DBG) log("EVENT_RADIO_POWER_OFF_DONE");
                if (mDeviceShuttingDown && mCi.getRadioState().isAvailable()) {
                    // during shutdown the modem may not send radio state changed event
                    // as a result of radio power request
                    // Hence, issuing shut down regardless of radio power response
                    mCi.requestShutdown(null);
                }
                break;

            // GSM
            case EVENT_SIM_READY:
                // Reset the mPreviousSubId so we treat a SIM power bounce
                // as a first boot.  See b/19194287
                mOnSubscriptionsChangedListener.mPreviousSubId.set(
                        SubscriptionManager.INVALID_SUBSCRIPTION_ID);
                mPrevSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
                mIsSimReady = true;
                pollState();
                // Signal strength polling stops when radio is off
                queueNextSignalStrengthPoll();
                break;

            case EVENT_RADIO_STATE_CHANGED:
            case EVENT_PHONE_TYPE_SWITCHED:
                if(!mPhone.isPhoneTypeGsm() &&
                        mCi.getRadioState() == CommandsInterface.RadioState.RADIO_ON) {
                    handleCdmaSubscriptionSource(mCdmaSSM.getCdmaSubscriptionSource());

                    // Signal strength polling stops when radio is off.
                    queueNextSignalStrengthPoll();
                }
                // This will do nothing in the 'radio not available' case
                setPowerStateToDesired();
                // These events are modem triggered, so pollState() needs to be forced
                modemTriggeredPollState();
                break;

            case EVENT_NETWORK_STATE_CHANGED:
                modemTriggeredPollState();
                break;

            case EVENT_GET_SIGNAL_STRENGTH:
                // This callback is called when signal strength is polled
                // all by itself

                if (!(mCi.getRadioState().isOn())) {
                    // Polling will continue when radio turns back on
                    return;
                }
                ar = (AsyncResult) msg.obj;
                onSignalStrengthResult(ar);
                queueNextSignalStrengthPoll();

                break;

            case EVENT_GET_LOC_DONE:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    CellIdentity cellIdentity = ((NetworkRegistrationState) ar.result)
                            .getCellIdentity();
                    processCellLocationInfo(mCellLoc, cellIdentity);
                    mPhone.notifyLocationChanged();
                }

                // Release any temporary cell lock, which could have been
                // acquired to allow a single-shot location update.
                disableSingleLocationUpdate();
                break;

            case EVENT_POLL_STATE_REGISTRATION:
            case EVENT_POLL_STATE_GPRS:
            case EVENT_POLL_STATE_OPERATOR:
                ar = (AsyncResult) msg.obj;
                handlePollStateResult(msg.what, ar);
                break;

            case EVENT_POLL_STATE_NETWORK_SELECTION_MODE:
                if (DBG) log("EVENT_POLL_STATE_NETWORK_SELECTION_MODE");
                ar = (AsyncResult) msg.obj;
                if (mPhone.isPhoneTypeGsm()) {
                    handlePollStateResult(msg.what, ar);
                } else {
                    if (ar.exception == null && ar.result != null) {
                        ints = (int[])ar.result;
                        if (ints[0] == 1) {  // Manual selection.
                            mPhone.setNetworkSelectionModeAutomatic(null);
                        }
                    } else {
                        log("Unable to getNetworkSelectionMode");
                    }
                }
                break;

            case EVENT_POLL_SIGNAL_STRENGTH:
                // Just poll signal strength...not part of pollState()

                mCi.getSignalStrength(obtainMessage(EVENT_GET_SIGNAL_STRENGTH));
                break;

            case EVENT_NITZ_TIME:
                ar = (AsyncResult) msg.obj;

                String nitzString = (String)((Object[])ar.result)[0];
                long nitzReceiveTime = ((Long)((Object[])ar.result)[1]).longValue();

                setTimeFromNITZString(nitzString, nitzReceiveTime);
                break;

            case EVENT_SIGNAL_STRENGTH_UPDATE:
                // This is a notification from CommandsInterface.setOnSignalStrengthUpdate

                ar = (AsyncResult) msg.obj;

                // The radio is telling us about signal strength changes
                // we don't have to ask it
                mDontPollSignalStrength = true;

                onSignalStrengthResult(ar);
                break;

            case EVENT_SIM_RECORDS_LOADED:
                log("EVENT_SIM_RECORDS_LOADED: what=" + msg.what);
                updatePhoneObject();
                updateOtaspState();
                if (mPhone.isPhoneTypeGsm()) {
                    updateSpnDisplay();
                }
                break;

            case EVENT_LOCATION_UPDATES_ENABLED:
                ar = (AsyncResult) msg.obj;

                if (ar.exception == null) {
                    mRegStateManagers.get(AccessNetworkConstants.TransportType.WWAN)
                            .getNetworkRegistrationState(NetworkRegistrationState.DOMAIN_CS,
                            obtainMessage(EVENT_GET_LOC_DONE, null));
                }
                break;

            case EVENT_SET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;
                // Don't care the result, only use for dereg network (COPS=2)
                message = obtainMessage(EVENT_RESET_PREFERRED_NETWORK_TYPE, ar.userObj);
                mCi.setPreferredNetworkType(mPreferredNetworkType, message);
                break;

            case EVENT_RESET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;
                if (ar.userObj != null) {
                    AsyncResult.forMessage(((Message) ar.userObj)).exception
                            = ar.exception;
                    ((Message) ar.userObj).sendToTarget();
                }
                break;

            case EVENT_GET_PREFERRED_NETWORK_TYPE:
                ar = (AsyncResult) msg.obj;

                if (ar.exception == null) {
                    mPreferredNetworkType = ((int[])ar.result)[0];
                } else {
                    mPreferredNetworkType = RILConstants.NETWORK_MODE_GLOBAL;
                }

                message = obtainMessage(EVENT_SET_PREFERRED_NETWORK_TYPE, ar.userObj);
                int toggledNetworkType = RILConstants.NETWORK_MODE_GLOBAL;

                mCi.setPreferredNetworkType(toggledNetworkType, message);
                break;

            case EVENT_CHECK_REPORT_GPRS:
                if (mPhone.isPhoneTypeGsm() && mSS != null &&
                        !isGprsConsistent(mSS.getDataRegState(), mSS.getVoiceRegState())) {

                    // Can't register data service while voice service is ok
                    // i.e. CREG is ok while CGREG is not
                    // possible a network or baseband side error
                    GsmCellLocation loc = ((GsmCellLocation)mPhone.getCellLocation());
                    EventLog.writeEvent(EventLogTags.DATA_NETWORK_REGISTRATION_FAIL,
                            mSS.getOperatorNumeric(), loc != null ? loc.getCid() : -1);
                    mReportedGprsNoReg = true;
                }
                mStartedGprsRegCheck = false;
                break;

            case EVENT_RESTRICTED_STATE_CHANGED:
                if (mPhone.isPhoneTypeGsm()) {
                    // This is a notification from
                    // CommandsInterface.setOnRestrictedStateChanged

                    if (DBG) log("EVENT_RESTRICTED_STATE_CHANGED");

                    ar = (AsyncResult) msg.obj;

                    onRestrictedStateChanged(ar);
                }
                break;

            case EVENT_SIM_NOT_INSERTED:
                if (DBG) log("EVENT_SIM_NOT_INSERTED");
                cancelAllNotifications();
                mMdn = null;
                mMin = null;
                mIsMinInfoReady = false;
                break;

            case EVENT_ALL_DATA_DISCONNECTED:
                int dds = SubscriptionManager.getDefaultDataSubscriptionId();
                ProxyController.getInstance().unregisterForAllDataDisconnected(dds, this);
                synchronized(this) {
                    if (mPendingRadioPowerOffAfterDataOff) {
                        if (DBG) log("EVENT_ALL_DATA_DISCONNECTED, turn radio off now.");
                        hangupAndPowerOff();
                        mPendingRadioPowerOffAfterDataOff = false;
                    } else {
                        log("EVENT_ALL_DATA_DISCONNECTED is stale");
                    }
                }
                break;

            case EVENT_CHANGE_IMS_STATE:
                if (DBG) log("EVENT_CHANGE_IMS_STATE:");

                setPowerStateToDesired();
                break;

            case EVENT_IMS_CAPABILITY_CHANGED:
                if (DBG) log("EVENT_IMS_CAPABILITY_CHANGED");
                updateSpnDisplay();
                break;

            case EVENT_IMS_SERVICE_STATE_CHANGED:
                if (DBG) log("EVENT_IMS_SERVICE_STATE_CHANGED");
                // IMS state will only affect the merged service state if the service state of
                // GsmCdma phone is not STATE_IN_SERVICE.
                if (mSS.getState() != ServiceState.STATE_IN_SERVICE) {
                    mPhone.notifyServiceStateChanged(mPhone.getServiceState());
                }
                break;

            //CDMA
            case EVENT_CDMA_SUBSCRIPTION_SOURCE_CHANGED:
                handleCdmaSubscriptionSource(mCdmaSSM.getCdmaSubscriptionSource());
                break;

            case EVENT_RUIM_READY:
                if (mPhone.getLteOnCdmaMode() == PhoneConstants.LTE_ON_CDMA_TRUE) {
                    // Subscription will be read from SIM I/O
                    if (DBG) log("Receive EVENT_RUIM_READY");
                    pollState();
                } else {
                    if (DBG) log("Receive EVENT_RUIM_READY and Send Request getCDMASubscription.");
                    getSubscriptionInfoAndStartPollingThreads();
                }

                // Only support automatic selection mode in CDMA.
                mCi.getNetworkSelectionMode(obtainMessage(EVENT_POLL_STATE_NETWORK_SELECTION_MODE));

                break;

            case EVENT_NV_READY:
                updatePhoneObject();

                // Only support automatic selection mode in CDMA.
                mCi.getNetworkSelectionMode(obtainMessage(EVENT_POLL_STATE_NETWORK_SELECTION_MODE));

                // For Non-RUIM phones, the subscription information is stored in
                // Non Volatile. Here when Non-Volatile is ready, we can poll the CDMA
                // subscription info.
                getSubscriptionInfoAndStartPollingThreads();
                break;

            case EVENT_POLL_STATE_CDMA_SUBSCRIPTION: // Handle RIL_CDMA_SUBSCRIPTION
                if (!mPhone.isPhoneTypeGsm()) {
                    ar = (AsyncResult) msg.obj;

                    if (ar.exception == null) {
                        String cdmaSubscription[] = (String[]) ar.result;
                        if (cdmaSubscription != null && cdmaSubscription.length >= 5) {
                            mMdn = cdmaSubscription[0];
                            parseSidNid(cdmaSubscription[1], cdmaSubscription[2]);

                            mMin = cdmaSubscription[3];
                            mPrlVersion = cdmaSubscription[4];
                            if (DBG) log("GET_CDMA_SUBSCRIPTION: MDN=" + mMdn);

                            mIsMinInfoReady = true;

                            updateOtaspState();
                            // Notify apps subscription info is ready
                            notifyCdmaSubscriptionInfoReady();

                            if (!mIsSubscriptionFromRuim && mIccRecords != null) {
                                if (DBG) {
                                    log("GET_CDMA_SUBSCRIPTION set imsi in mIccRecords");
                                }
                                mIccRecords.setImsi(getImsi());
                            } else {
                                if (DBG) {
                                    log("GET_CDMA_SUBSCRIPTION either mIccRecords is null or NV " +
                                            "type device - not setting Imsi in mIccRecords");
                                }
                            }
                        } else {
                            if (DBG) {
                                log("GET_CDMA_SUBSCRIPTION: error parsing cdmaSubscription " +
                                        "params num=" + cdmaSubscription.length);
                            }
                        }
                    }
                }
                break;

            case EVENT_RUIM_RECORDS_LOADED:
                if (!mPhone.isPhoneTypeGsm()) {
                    log("EVENT_RUIM_RECORDS_LOADED: what=" + msg.what);
                    updatePhoneObject();
                    if (mPhone.isPhoneTypeCdma()) {
                        updateSpnDisplay();
                    } else {
                        RuimRecords ruim = (RuimRecords) mIccRecords;
                        if (ruim != null) {
                            if (ruim.isProvisioned()) {
                                mMdn = ruim.getMdn();
                                mMin = ruim.getMin();
                                parseSidNid(ruim.getSid(), ruim.getNid());
                                mPrlVersion = ruim.getPrlVersion();
                                mIsMinInfoReady = true;
                            }
                            updateOtaspState();
                            // Notify apps subscription info is ready
                            notifyCdmaSubscriptionInfoReady();
                        }
                        // SID/NID/PRL is loaded. Poll service state
                        // again to update to the roaming state with
                        // the latest variables.
                        pollState();
                    }
                }
                break;

            case EVENT_ERI_FILE_LOADED:
                // Repoll the state once the ERI file has been loaded.
                if (DBG) log("ERI file has been loaded, repolling.");
                pollState();
                break;

            case EVENT_OTA_PROVISION_STATUS_CHANGE:
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    ints = (int[]) ar.result;
                    int otaStatus = ints[0];
                    if (otaStatus == Phone.CDMA_OTA_PROVISION_STATUS_COMMITTED
                            || otaStatus == Phone.CDMA_OTA_PROVISION_STATUS_OTAPA_STOPPED) {
                        if (DBG) log("EVENT_OTA_PROVISION_STATUS_CHANGE: Complete, Reload MDN");
                        mCi.getCDMASubscription( obtainMessage(EVENT_POLL_STATE_CDMA_SUBSCRIPTION));
                    }
                }
                break;

            case EVENT_CDMA_PRL_VERSION_CHANGED:
                ar = (AsyncResult)msg.obj;
                if (ar.exception == null) {
                    ints = (int[]) ar.result;
                    mPrlVersion = Integer.toString(ints[0]);
                }
                break;

            case EVENT_RADIO_POWER_FROM_CARRIER:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    boolean enable = (boolean) ar.result;
                    if (DBG) log("EVENT_RADIO_POWER_FROM_CARRIER: " + enable);
                    setRadioPowerFromCarrier(enable);
                }
                break;

            case EVENT_PHYSICAL_CHANNEL_CONFIG:
                ar = (AsyncResult) msg.obj;
                if (ar.exception == null) {
                    List<PhysicalChannelConfig> list = (List<PhysicalChannelConfig>) ar.result;
                    if (VDBG) {
                        log("EVENT_PHYSICAL_CHANNEL_CONFIG: size=" + list.size() + " list="
                                + list);
                    }
                    mPhone.notifyPhysicalChannelConfiguration(list);
                    mLastPhysicalChannelConfigList = list;

                    // only notify if bandwidths changed
                    if (RatRatcheter.updateBandwidths(getBandwidthsFromConfigs(list), mSS)) {
                        mPhone.notifyServiceStateChanged(mSS);
                    }
                }
                break;

            default:
                log("Unhandled message with number: " + msg.what);
                break;
        }
    }

    private int[] getBandwidthsFromConfigs(List<PhysicalChannelConfig> list) {
        return list.stream()
                .map(PhysicalChannelConfig::getCellBandwidthDownlink)
                .mapToInt(Integer::intValue)
                .toArray();
    }

    protected boolean isSidsAllZeros() {
        if (mHomeSystemId != null) {
            for (int i=0; i < mHomeSystemId.length; i++) {
                if (mHomeSystemId[i] != 0) {
                    return false;
                }
            }
        }
        return true;
    }

    /**
     * Check whether a specified system ID that matches one of the home system IDs.
     */
    private boolean isHomeSid(int sid) {
        if (mHomeSystemId != null) {
            for (int i=0; i < mHomeSystemId.length; i++) {
                if (sid == mHomeSystemId[i]) {
                    return true;
                }
            }
        }
        return false;
    }

    public String getMdnNumber() {
        return mMdn;
    }

    public String getCdmaMin() {
        return mMin;
    }

    /** Returns null if NV is not yet ready */
    public String getPrlVersion() {
        return mPrlVersion;
    }

    /**
     * Returns IMSI as MCC + MNC + MIN
     */
    public String getImsi() {
        // TODO: When RUIM is enabled, IMSI will come from RUIM not build-time props.
        String operatorNumeric = ((TelephonyManager) mPhone.getContext().
                getSystemService(Context.TELEPHONY_SERVICE)).
                getSimOperatorNumericForPhone(mPhone.getPhoneId());

        if (!TextUtils.isEmpty(operatorNumeric) && getCdmaMin() != null) {
            return (operatorNumeric + getCdmaMin());
        } else {
            return null;
        }
    }

    /**
     * Check if subscription data has been assigned to mMin
     *
     * return true if MIN info is ready; false otherwise.
     */
    public boolean isMinInfoReady() {
        return mIsMinInfoReady;
    }

    /**
     * Returns OTASP_UNKNOWN, OTASP_UNINITIALIZED, OTASP_NEEDED or OTASP_NOT_NEEDED
     */
    public int getOtasp() {
        int provisioningState;
        // if sim is not loaded, return otasp uninitialized
        if(!mPhone.getIccRecordsLoaded()) {
            if(DBG) log("getOtasp: otasp uninitialized due to sim not loaded");
            return TelephonyManager.OTASP_UNINITIALIZED;
        }
        // if voice tech is Gsm, return otasp not needed
        if(mPhone.isPhoneTypeGsm()) {
            if(DBG) log("getOtasp: otasp not needed for GSM");
            return TelephonyManager.OTASP_NOT_NEEDED;
        }
        // for ruim, min is null means require otasp.
        if (mIsSubscriptionFromRuim && mMin == null) {
            return TelephonyManager.OTASP_NEEDED;
        }
        if (mMin == null || (mMin.length() < 6)) {
            if (DBG) log("getOtasp: bad mMin='" + mMin + "'");
            provisioningState = TelephonyManager.OTASP_UNKNOWN;
        } else {
            if ((mMin.equals(UNACTIVATED_MIN_VALUE)
                    || mMin.substring(0,6).equals(UNACTIVATED_MIN2_VALUE))
                    || SystemProperties.getBoolean("test_cdma_setup", false)) {
                provisioningState = TelephonyManager.OTASP_NEEDED;
            } else {
                provisioningState = TelephonyManager.OTASP_NOT_NEEDED;
            }
        }
        if (DBG) log("getOtasp: state=" + provisioningState);
        return provisioningState;
    }

    protected void parseSidNid (String sidStr, String nidStr) {
        if (sidStr != null) {
            String[] sid = sidStr.split(",");
            mHomeSystemId = new int[sid.length];
            for (int i = 0; i < sid.length; i++) {
                try {
                    mHomeSystemId[i] = Integer.parseInt(sid[i]);
                } catch (NumberFormatException ex) {
                    loge("error parsing system id: " + ex);
                }
            }
        }
        if (DBG) log("CDMA_SUBSCRIPTION: SID=" + sidStr);

        if (nidStr != null) {
            String[] nid = nidStr.split(",");
            mHomeNetworkId = new int[nid.length];
            for (int i = 0; i < nid.length; i++) {
                try {
                    mHomeNetworkId[i] = Integer.parseInt(nid[i]);
                } catch (NumberFormatException ex) {
                    loge("CDMA_SUBSCRIPTION: error parsing network id: " + ex);
                }
            }
        }
        if (DBG) log("CDMA_SUBSCRIPTION: NID=" + nidStr);
    }

    protected void updateOtaspState() {
        int otaspMode = getOtasp();
        int oldOtaspMode = mCurrentOtaspMode;
        mCurrentOtaspMode = otaspMode;

        if (oldOtaspMode != mCurrentOtaspMode) {
            if (DBG) {
                log("updateOtaspState: call notifyOtaspChanged old otaspMode=" +
                        oldOtaspMode + " new otaspMode=" + mCurrentOtaspMode);
            }
            mPhone.notifyOtaspChanged(mCurrentOtaspMode);
        }
    }

    protected Phone getPhone() {
        return mPhone;
    }

    protected void handlePollStateResult(int what, AsyncResult ar) {
        // Ignore stale requests from last poll
        if (ar.userObj != mPollingContext) return;

        if (ar.exception != null) {
            CommandException.Error err=null;

            if (ar.exception instanceof IllegalStateException) {
                log("handlePollStateResult exception " + ar.exception);
            }

            if (ar.exception instanceof CommandException) {
                err = ((CommandException)(ar.exception)).getCommandError();
            }

            if (err == CommandException.Error.RADIO_NOT_AVAILABLE) {
                // Radio has crashed or turned off
                cancelPollState();
                return;
            }

            if (err != CommandException.Error.OP_NOT_ALLOWED_BEFORE_REG_NW) {
                loge("RIL implementation has returned an error where it must succeed" +
                        ar.exception);
            }
        } else try {
            handlePollStateResultMessage(what, ar);
        } catch (RuntimeException ex) {
            loge("Exception while polling service state. Probably malformed RIL response." + ex);
        }

        mPollingContext[0]--;

        if (mPollingContext[0] == 0) {
            if (mPhone.isPhoneTypeGsm()) {
                updateRoamingState();
                mNewSS.setEmergencyOnly(mEmergencyOnly);
            } else {
                boolean namMatch = false;
                if (!isSidsAllZeros() && isHomeSid(mNewSS.getCdmaSystemId())) {
                    namMatch = true;
                }

                // Setting SS Roaming (general)
                if (mIsSubscriptionFromRuim) {
                    boolean isRoamingBetweenOperators = isRoamingBetweenOperators(
                            mNewSS.getVoiceRoaming(), mNewSS);
                    if (isRoamingBetweenOperators != mNewSS.getVoiceRoaming()) {
                        log("isRoamingBetweenOperators=" + isRoamingBetweenOperators
                                + ". Override CDMA voice roaming to " + isRoamingBetweenOperators);
                        mNewSS.setVoiceRoaming(isRoamingBetweenOperators);
                    }
                }
                /**
                 * For CDMA, voice and data should have the same roaming status.
                 * If voice is not in service, use TSB58 roaming indicator to set
                 * data roaming status. If TSB58 roaming indicator is not in the
                 * carrier-specified list of ERIs for home system then set roaming.
                 */
                final int dataRat = mNewSS.getRilDataRadioTechnology();
                if (ServiceState.isCdma(dataRat)) {
                    final boolean isVoiceInService =
                            (mNewSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE);
                    if (isVoiceInService) {
                        boolean isVoiceRoaming = mNewSS.getVoiceRoaming();
                        if (mNewSS.getDataRoaming() != isVoiceRoaming) {
                            log("Data roaming != Voice roaming. Override data roaming to "
                                    + isVoiceRoaming);
                            mNewSS.setDataRoaming(isVoiceRoaming);
                        }
                    } else {
                        /**
                         * As per VoiceRegStateResult from radio types.hal the TSB58
                         * Roaming Indicator shall be sent if device is registered
                         * on a CDMA or EVDO system.
                         */
                        boolean isRoamIndForHomeSystem = isRoamIndForHomeSystem(
                                Integer.toString(mRoamingIndicator));
                        if (mNewSS.getDataRoaming() == isRoamIndForHomeSystem) {
                            log("isRoamIndForHomeSystem=" + isRoamIndForHomeSystem
                                    + ", override data roaming to " + !isRoamIndForHomeSystem);
                            mNewSS.setDataRoaming(!isRoamIndForHomeSystem);
                        }
                    }
                }

                // Setting SS CdmaRoamingIndicator and CdmaDefaultRoamingIndicator
                mNewSS.setCdmaDefaultRoamingIndicator(mDefaultRoamingIndicator);
                mNewSS.setCdmaRoamingIndicator(mRoamingIndicator);
                boolean isPrlLoaded = true;
                if (TextUtils.isEmpty(mPrlVersion)) {
                    isPrlLoaded = false;
                }
                if (!isPrlLoaded || (mNewSS.getRilVoiceRadioTechnology()
                        == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN)) {
                    log("Turn off roaming indicator if !isPrlLoaded or voice RAT is unknown");
                    mNewSS.setCdmaRoamingIndicator(EriInfo.ROAMING_INDICATOR_OFF);
                } else if (!isSidsAllZeros()) {
                    if (!namMatch && !mIsInPrl) {
                        // Use default
                        mNewSS.setCdmaRoamingIndicator(mDefaultRoamingIndicator);
                    } else if (namMatch && !mIsInPrl) {
                        // TODO this will be removed when we handle roaming on LTE on CDMA+LTE phones
                        if (ServiceState.isLte(mNewSS.getRilVoiceRadioTechnology())) {
                            log("Turn off roaming indicator as voice is LTE");
                            mNewSS.setCdmaRoamingIndicator(EriInfo.ROAMING_INDICATOR_OFF);
                        } else {
                            mNewSS.setCdmaRoamingIndicator(EriInfo.ROAMING_INDICATOR_FLASH);
                        }
                    } else if (!namMatch && mIsInPrl) {
                        // Use the one from PRL/ERI
                        mNewSS.setCdmaRoamingIndicator(mRoamingIndicator);
                    } else {
                        // It means namMatch && mIsInPrl
                        if ((mRoamingIndicator <= 2)) {
                            mNewSS.setCdmaRoamingIndicator(EriInfo.ROAMING_INDICATOR_OFF);
                        } else {
                            // Use the one from PRL/ERI
                            mNewSS.setCdmaRoamingIndicator(mRoamingIndicator);
                        }
                    }
                }

                int roamingIndicator = mNewSS.getCdmaRoamingIndicator();
                mNewSS.setCdmaEriIconIndex(mPhone.mEriManager.getCdmaEriIconIndex(roamingIndicator,
                        mDefaultRoamingIndicator));
                mNewSS.setCdmaEriIconMode(mPhone.mEriManager.getCdmaEriIconMode(roamingIndicator,
                        mDefaultRoamingIndicator));

                // NOTE: Some operator may require overriding mCdmaRoaming
                // (set by the modem), depending on the mRoamingIndicator.

                if (DBG) {
                    log("Set CDMA Roaming Indicator to: " + mNewSS.getCdmaRoamingIndicator()
                            + ". voiceRoaming = " + mNewSS.getVoiceRoaming()
                            + ". dataRoaming = " + mNewSS.getDataRoaming()
                            + ", isPrlLoaded = " + isPrlLoaded
                            + ". namMatch = " + namMatch + " , mIsInPrl = " + mIsInPrl
                            + ", mRoamingIndicator = " + mRoamingIndicator
                            + ", mDefaultRoamingIndicator= " + mDefaultRoamingIndicator);
                }
            }
            pollStateDone();
        }

    }

    /**
     * Set roaming state when cdmaRoaming is true and ons is different from spn
     * @param cdmaRoaming TS 27.007 7.2 CREG registered roaming
     * @param s ServiceState hold current ons
     * @return true for roaming state set
     */
    private boolean isRoamingBetweenOperators(boolean cdmaRoaming, ServiceState s) {
        return cdmaRoaming && !isSameOperatorNameFromSimAndSS(s);
    }

    void handlePollStateResultMessage(int what, AsyncResult ar) {
        int ints[];
        switch (what) {
            case EVENT_POLL_STATE_REGISTRATION: {
                NetworkRegistrationState networkRegState = (NetworkRegistrationState) ar.result;
                VoiceSpecificRegistrationStates voiceSpecificStates =
                        networkRegState.getVoiceSpecificStates();

                int registrationState = networkRegState.getRegState();
                int cssIndicator = voiceSpecificStates.cssSupported ? 1 : 0;
                int newVoiceRat = ServiceState.networkTypeToRilRadioTechnology(
                        networkRegState.getAccessNetworkTechnology());

                mNewSS.setVoiceRegState(regCodeToServiceState(registrationState));
                mNewSS.setCssIndicator(cssIndicator);
                mNewSS.setRilVoiceRadioTechnology(newVoiceRat);
                mNewSS.addNetworkRegistrationState(networkRegState);
                setPhyCellInfoFromCellIdentity(mNewSS, networkRegState.getCellIdentity());

                //Denial reason if registrationState = 3
                int reasonForDenial = networkRegState.getReasonForDenial();
                if (mPhone.isPhoneTypeGsm()) {

                    mGsmRoaming = regCodeIsRoaming(registrationState);
                    mNewRejectCode = reasonForDenial;

                    boolean isVoiceCapable = mPhone.getContext().getResources()
                            .getBoolean(com.android.internal.R.bool.config_voice_capable);
                    mEmergencyOnly = networkRegState.isEmergencyEnabled();
                } else {
                    int roamingIndicator = voiceSpecificStates.roamingIndicator;

                    //Indicates if current system is in PR
                    int systemIsInPrl = voiceSpecificStates.systemIsInPrl;

                    //Is default roaming indicator from PRL
                    int defaultRoamingIndicator = voiceSpecificStates.defaultRoamingIndicator;

                    mRegistrationState = registrationState;
                    // When registration state is roaming and TSB58
                    // roaming indicator is not in the carrier-specified
                    // list of ERIs for home system, mCdmaRoaming is true.
                    boolean cdmaRoaming =
                            regCodeIsRoaming(registrationState)
                                    && !isRoamIndForHomeSystem(
                                            Integer.toString(roamingIndicator));
                    mNewSS.setVoiceRoaming(cdmaRoaming);
                    mRoamingIndicator = roamingIndicator;
                    mIsInPrl = (systemIsInPrl == 0) ? false : true;
                    mDefaultRoamingIndicator = defaultRoamingIndicator;

                    int systemId = 0;
                    int networkId = 0;
                    CellIdentity cellIdentity = networkRegState.getCellIdentity();
                    if (cellIdentity != null && cellIdentity.getType() == CellInfoType.CDMA) {
                        systemId = ((CellIdentityCdma) cellIdentity).getSystemId();
                        networkId = ((CellIdentityCdma) cellIdentity).getNetworkId();
                    }
                    mNewSS.setCdmaSystemAndNetworkId(systemId, networkId);

                    if (reasonForDenial == 0) {
                        mRegistrationDeniedReason = ServiceStateTracker.REGISTRATION_DENIED_GEN;
                    } else if (reasonForDenial == 1) {
                        mRegistrationDeniedReason = ServiceStateTracker.REGISTRATION_DENIED_AUTH;
                    } else {
                        mRegistrationDeniedReason = "";
                    }

                    if (mRegistrationState == 3) {
                        if (DBG) log("Registration denied, " + mRegistrationDeniedReason);
                    }
                }

                processCellLocationInfo(mNewCellLoc, networkRegState.getCellIdentity());

                if (DBG) {
                    log("handlPollVoiceRegResultMessage: regState=" + registrationState
                            + " radioTechnology=" + newVoiceRat);
                }
                break;
            }

            case EVENT_POLL_STATE_GPRS: {
                NetworkRegistrationState networkRegState = (NetworkRegistrationState) ar.result;
                DataSpecificRegistrationStates dataSpecificStates =
                        networkRegState.getDataSpecificStates();
                int registrationState = networkRegState.getRegState();
                int serviceState = regCodeToServiceState(registrationState);
                int newDataRat = ServiceState.networkTypeToRilRadioTechnology(
                        networkRegState.getAccessNetworkTechnology());
                mNewSS.setDataRegState(serviceState);
                mNewSS.setRilDataRadioTechnology(newDataRat);
                mNewSS.addNetworkRegistrationState(networkRegState);

                // When we receive OOS reset the PhyChanConfig list so that non-return-to-idle
                // implementers of PhyChanConfig unsol will not carry forward a CA report
                // (2 or more cells) to a new cell if they camp for emergency service only.
                if (serviceState == ServiceState.STATE_OUT_OF_SERVICE) {
                    mLastPhysicalChannelConfigList = null;
                }
                setPhyCellInfoFromCellIdentity(mNewSS, networkRegState.getCellIdentity());

                if (mPhone.isPhoneTypeGsm()) {

                    mNewReasonDataDenied = networkRegState.getReasonForDenial();
                    mNewMaxDataCalls = dataSpecificStates.maxDataCalls;
                    mDataRoaming = regCodeIsRoaming(registrationState);
                    // Save the data roaming state reported by modem registration before resource
                    // overlay or carrier config possibly overrides it.
                    mNewSS.setDataRoamingFromRegistration(mDataRoaming);

                    if (DBG) {
                        log("handlPollStateResultMessage: GsmSST dataServiceState=" + serviceState
                                + " regState=" + registrationState
                                + " dataRadioTechnology=" + newDataRat);
                    }
                } else if (mPhone.isPhoneTypeCdma()) {

                    boolean isDataRoaming = regCodeIsRoaming(registrationState);
                    mNewSS.setDataRoaming(isDataRoaming);
                    // Save the data roaming state reported by modem registration before resource
                    // overlay or carrier config possibly overrides it.
                    mNewSS.setDataRoamingFromRegistration(isDataRoaming);

                    if (DBG) {
                        log("handlPollStateResultMessage: cdma dataServiceState=" + serviceState
                                + " regState=" + registrationState
                                + " dataRadioTechnology=" + newDataRat);
                    }
                } else {

                    // If the unsolicited signal strength comes just before data RAT family changes
                    // (i.e. from UNKNOWN to LTE, CDMA to LTE, LTE to CDMA), the signal bar might
                    // display the wrong information until the next unsolicited signal strength
                    // information coming from the modem, which might take a long time to come or
                    // even not come at all.  In order to provide the best user experience, we
                    // query the latest signal information so it will show up on the UI on time.
                    int oldDataRAT = mSS.getRilDataRadioTechnology();
                    if (((oldDataRAT == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN)
                            && (newDataRat != ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN))
                            || (ServiceState.isCdma(oldDataRAT) && ServiceState.isLte(newDataRat))
                            || (ServiceState.isLte(oldDataRAT)
                            && ServiceState.isCdma(newDataRat))) {
                        mCi.getSignalStrength(obtainMessage(EVENT_GET_SIGNAL_STRENGTH));
                    }

                    // voice roaming state in done while handling EVENT_POLL_STATE_REGISTRATION_CDMA
                    boolean isDataRoaming = regCodeIsRoaming(registrationState);
                    mNewSS.setDataRoaming(isDataRoaming);
                    // Save the data roaming state reported by modem registration before resource
                    // overlay or carrier config possibly overrides it.
                    mNewSS.setDataRoamingFromRegistration(isDataRoaming);
                    if (DBG) {
                        log("handlPollStateResultMessage: CdmaLteSST dataServiceState="
                                + serviceState + " registrationState=" + registrationState
                                + " dataRadioTechnology=" + newDataRat);
                    }
                }

                updateServiceStateLteEarfcnBoost(mNewSS,
                        getLteEarfcn(networkRegState.getCellIdentity()));
                break;
            }

            case EVENT_POLL_STATE_OPERATOR: {
                if (mPhone.isPhoneTypeGsm()) {
                    String opNames[] = (String[]) ar.result;

                    if (opNames != null && opNames.length >= 3) {
                        // FIXME: Giving brandOverride higher precedence, is this desired?
                        String brandOverride = mUiccController.getUiccCard(getPhoneId()) != null
                                ? mUiccController.getUiccCard(getPhoneId())
                                        .getOperatorBrandOverride() : null;
                        if (brandOverride != null) {
                            log("EVENT_POLL_STATE_OPERATOR: use brandOverride=" + brandOverride);
                            mNewSS.setOperatorName(brandOverride, brandOverride, opNames[2]);
                        } else {
                            mNewSS.setOperatorName(opNames[0], opNames[1], opNames[2]);
                        }
                    }
                } else {
                    String opNames[] = (String[])ar.result;

                    if (opNames != null && opNames.length >= 3) {
                        // TODO: Do we care about overriding in this case.
                        // If the NUMERIC field isn't valid use PROPERTY_CDMA_HOME_OPERATOR_NUMERIC
                        if ((opNames[2] == null) || (opNames[2].length() < 5)
                                || ("00000".equals(opNames[2]))) {
                            opNames[2] = SystemProperties.get(
                                    GsmCdmaPhone.PROPERTY_CDMA_HOME_OPERATOR_NUMERIC, "00000");
                            if (DBG) {
                                log("RIL_REQUEST_OPERATOR.response[2], the numeric, " +
                                        " is bad. Using SystemProperties '" +
                                        GsmCdmaPhone.PROPERTY_CDMA_HOME_OPERATOR_NUMERIC +
                                        "'= " + opNames[2]);
                            }
                        }

                        if (!mIsSubscriptionFromRuim) {
                            // NV device (as opposed to CSIM)
                            mNewSS.setOperatorName(opNames[0], opNames[1], opNames[2]);
                        } else {
                            String brandOverride = mUiccController.getUiccCard(getPhoneId()) != null
                                    ? mUiccController.getUiccCard(getPhoneId())
                                    .getOperatorBrandOverride() : null;
                            if (brandOverride != null) {
                                mNewSS.setOperatorName(brandOverride, brandOverride, opNames[2]);
                            } else {
                                mNewSS.setOperatorName(opNames[0], opNames[1], opNames[2]);
                            }
                        }
                    } else {
                        if (DBG) log("EVENT_POLL_STATE_OPERATOR_CDMA: error parsing opNames");
                    }
                }
                break;
            }

            case EVENT_POLL_STATE_NETWORK_SELECTION_MODE: {
                ints = (int[])ar.result;
                mNewSS.setIsManualSelection(ints[0] == 1);
                if ((ints[0] == 1) && (mPhone.shouldForceAutoNetworkSelect())) {
                        /*
                         * modem is currently in manual selection but manual
                         * selection is not allowed in the current mode so
                         * switch to automatic registration
                         */
                    mPhone.setNetworkSelectionModeAutomatic (null);
                    log(" Forcing Automatic Network Selection, " +
                            "manual selection is not allowed");
                }
                break;
            }

            default:
                loge("handlePollStateResultMessage: Unexpected RIL response received: " + what);
        }
    }

    private static boolean isValidLteBandwidthKhz(int bandwidth) {
        // Valid bandwidths, see 3gpp 36.101 sec. 5.6
        switch (bandwidth) {
            case 1400:
            case 3000:
            case 5000:
            case 10000:
            case 15000:
            case 20000:
                return true;
            default:
                return false;
        }
    }

    private void setPhyCellInfoFromCellIdentity(ServiceState ss, CellIdentity cellIdentity) {
        if (cellIdentity == null) {
            if (DBG) {
                log("Could not set ServiceState channel number. CellIdentity null");
            }
            return;
        }

        ss.setChannelNumber(cellIdentity.getChannelNumber());
        if (VDBG) {
            log("Setting channel number: " + cellIdentity.getChannelNumber());
        }

        if (cellIdentity instanceof CellIdentityLte) {
            CellIdentityLte cl = (CellIdentityLte) cellIdentity;
            int[] bandwidths = null;
            // Prioritize the PhysicalChannelConfig list because we might already be in carrier
            // aggregation by the time poll state is performed.
            if (!ArrayUtils.isEmpty(mLastPhysicalChannelConfigList)) {
                bandwidths = getBandwidthsFromConfigs(mLastPhysicalChannelConfigList);
                for (int bw : bandwidths) {
                    if (!isValidLteBandwidthKhz(bw)) {
                        loge("Invalid LTE Bandwidth in RegistrationState, " + bw);
                        bandwidths = null;
                        break;
                    }
                }
            }
            // If we don't have a PhysicalChannelConfig[] list, then pull from CellIdentityLte.
            // This is normal if we're in idle mode and the PhysicalChannelConfig[] has already
            // been updated. This is also a fallback in case the PhysicalChannelConfig info
            // is invalid (ie, broken).
            // Also, for vendor implementations that do not report return-to-idle, we should
            // prioritize the bandwidth report in the CellIdentity, because the physical channel
            // config report may be stale in the case where a single carrier was used previously
            // and we transition to camped-for-emergency (since we never have a physical
            // channel active). In the normal case of single-carrier non-return-to-idle, the
            // values *must* be the same, so it doesn't matter which is chosen.
            if (bandwidths == null || bandwidths.length == 1) {
                final int cbw = cl.getBandwidth();
                if (isValidLteBandwidthKhz(cbw)) {
                    bandwidths = new int[] {cbw};
                } else if (cbw == Integer.MAX_VALUE) {
                    // Bandwidth is unreported; c'est la vie. This is not an error because
                    // pre-1.2 HAL implementations do not support bandwidth reporting.
                } else {
                    loge("Invalid LTE Bandwidth in RegistrationState, " + cbw);
                }
            }
            if (bandwidths != null) {
                ss.setCellBandwidths(bandwidths);
            }
        } else {
            if (VDBG) log("Skipping bandwidth update for Non-LTE cell.");
        }
    }

    /**
     * Determine whether a roaming indicator is in the carrier-specified list of ERIs for
     * home system
     *
     * @param roamInd roaming indicator in String
     * @return true if the roamInd is in the carrier-specified list of ERIs for home network
     */
    private boolean isRoamIndForHomeSystem(String roamInd) {
        // retrieve the carrier-specified list of ERIs for home system
        String[] homeRoamIndicators = Resources.getSystem()
                .getStringArray(com.android.internal.R.array.config_cdma_home_system);
        log("isRoamIndForHomeSystem: homeRoamIndicators=" + Arrays.toString(homeRoamIndicators));

        if (homeRoamIndicators != null) {
            // searches through the comma-separated list for a match,
            // return true if one is found.
            for (String homeRoamInd : homeRoamIndicators) {
                if (homeRoamInd.equals(roamInd)) {
                    return true;
                }
            }
            // no matches found against the list!
            log("isRoamIndForHomeSystem: No match found against list for roamInd=" + roamInd);
            return false;
        }

        // no system property found for the roaming indicators for home system
        log("isRoamIndForHomeSystem: No list found");
        return false;
    }

    /**
     * Query the carrier configuration to determine if there any network overrides
     * for roaming or not roaming for the current service state.
     */
    protected void updateRoamingState() {
        if (mPhone.isPhoneTypeGsm()) {
            /**
             * Since the roaming state of gsm service (from +CREG) and
             * data service (from +CGREG) could be different, the new SS
             * is set to roaming when either is true.
             *
             * There are exceptions for the above rule.
             * The new SS is not set as roaming while gsm service reports
             * roaming but indeed it is same operator.
             * And the operator is considered non roaming.
             *
             * The test for the operators is to handle special roaming
             * agreements and MVNO's.
             */
            boolean roaming = (mGsmRoaming || mDataRoaming);

            if (mGsmRoaming && !isOperatorConsideredRoaming(mNewSS)
                    && (isSameNamedOperators(mNewSS) || isOperatorConsideredNonRoaming(mNewSS))) {
                log("updateRoamingState: resource override set non roaming.isSameNamedOperators="
                        + isSameNamedOperators(mNewSS) + ",isOperatorConsideredNonRoaming="
                        + isOperatorConsideredNonRoaming(mNewSS));
                roaming = false;
            }

            CarrierConfigManager configLoader = (CarrierConfigManager)
                    mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);

            if (configLoader != null) {
                try {
                    PersistableBundle b = configLoader.getConfigForSubId(mPhone.getSubId());

                    if (alwaysOnHomeNetwork(b)) {
                        log("updateRoamingState: carrier config override always on home network");
                        roaming = false;
                    } else if (isNonRoamingInGsmNetwork(b, mNewSS.getOperatorNumeric())) {
                        log("updateRoamingState: carrier config override set non roaming:"
                                + mNewSS.getOperatorNumeric());
                        roaming = false;
                    } else if (isRoamingInGsmNetwork(b, mNewSS.getOperatorNumeric())) {
                        log("updateRoamingState: carrier config override set roaming:"
                                + mNewSS.getOperatorNumeric());
                        roaming = true;
                    }
                } catch (Exception e) {
                    loge("updateRoamingState: unable to access carrier config service");
                }
            } else {
                log("updateRoamingState: no carrier config service available");
            }

            mNewSS.setVoiceRoaming(roaming);
            mNewSS.setDataRoaming(roaming);
        } else {
            CarrierConfigManager configLoader = (CarrierConfigManager)
                    mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
            if (configLoader != null) {
                try {
                    PersistableBundle b = configLoader.getConfigForSubId(mPhone.getSubId());
                    String systemId = Integer.toString(mNewSS.getCdmaSystemId());

                    if (alwaysOnHomeNetwork(b)) {
                        log("updateRoamingState: carrier config override always on home network");
                        setRoamingOff();
                    } else if (isNonRoamingInGsmNetwork(b, mNewSS.getOperatorNumeric())
                            || isNonRoamingInCdmaNetwork(b, systemId)) {
                        log("updateRoamingState: carrier config override set non-roaming:"
                                + mNewSS.getOperatorNumeric() + ", " + systemId);
                        setRoamingOff();
                    } else if (isRoamingInGsmNetwork(b, mNewSS.getOperatorNumeric())
                            || isRoamingInCdmaNetwork(b, systemId)) {
                        log("updateRoamingState: carrier config override set roaming:"
                                + mNewSS.getOperatorNumeric() + ", " + systemId);
                        setRoamingOn();
                    }
                } catch (Exception e) {
                    loge("updateRoamingState: unable to access carrier config service");
                }
            } else {
                log("updateRoamingState: no carrier config service available");
            }

            if (Build.IS_DEBUGGABLE && SystemProperties.getBoolean(PROP_FORCE_ROAMING, false)) {
                mNewSS.setVoiceRoaming(true);
                mNewSS.setDataRoaming(true);
            }
        }
    }

    private void setRoamingOn() {
        mNewSS.setVoiceRoaming(true);
        mNewSS.setDataRoaming(true);
        mNewSS.setCdmaEriIconIndex(EriInfo.ROAMING_INDICATOR_ON);
        mNewSS.setCdmaEriIconMode(EriInfo.ROAMING_ICON_MODE_NORMAL);
    }

    private void setRoamingOff() {
        mNewSS.setVoiceRoaming(false);
        mNewSS.setDataRoaming(false);
        mNewSS.setCdmaEriIconIndex(EriInfo.ROAMING_INDICATOR_OFF);
    }

    protected void updateSpnDisplay() {
        updateOperatorNameFromEri();

        String wfcVoiceSpnFormat = null;
        String wfcDataSpnFormat = null;
        int combinedRegState = getCombinedRegState();
        if (mPhone.getImsPhone() != null && mPhone.getImsPhone().isWifiCallingEnabled()
                && (combinedRegState == ServiceState.STATE_IN_SERVICE)) {
            // In Wi-Fi Calling mode show SPN or PLMN + WiFi Calling
            //
            // 1) Show SPN + Wi-Fi Calling If SIM has SPN and SPN display condition
            //    is satisfied or SPN override is enabled for this carrier
            //
            // 2) Show PLMN + Wi-Fi Calling if there is no valid SPN in case 1

            String[] wfcSpnFormats = mPhone.getContext().getResources().getStringArray(
                    com.android.internal.R.array.wfcSpnFormats);
            int voiceIdx = 0;
            int dataIdx = 0;
            CarrierConfigManager configLoader = (CarrierConfigManager)
                    mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
            if (configLoader != null) {
                try {
                    PersistableBundle b = configLoader.getConfigForSubId(mPhone.getSubId());
                    if (b != null) {
                        voiceIdx = b.getInt(CarrierConfigManager.KEY_WFC_SPN_FORMAT_IDX_INT);
                        dataIdx = b.getInt(
                                CarrierConfigManager.KEY_WFC_DATA_SPN_FORMAT_IDX_INT);
                    }
                } catch (Exception e) {
                    loge("updateSpnDisplay: carrier config error: " + e);
                }
            }

            wfcVoiceSpnFormat = wfcSpnFormats[voiceIdx];
            wfcDataSpnFormat = wfcSpnFormats[dataIdx];
        }

        if (mPhone.isPhoneTypeGsm()) {
            // The values of plmn/showPlmn change in different scenarios.
            // 1) No service but emergency call allowed -> expected
            //    to show "Emergency call only"
            //    EXTRA_SHOW_PLMN = true
            //    EXTRA_PLMN = "Emergency call only"

            // 2) No service at all --> expected to show "No service"
            //    EXTRA_SHOW_PLMN = true
            //    EXTRA_PLMN = "No service"

            // 3) Normal operation in either home or roaming service
            //    EXTRA_SHOW_PLMN = depending on IccRecords rule
            //    EXTRA_PLMN = plmn

            // 4) No service due to power off, aka airplane mode
            //    EXTRA_SHOW_PLMN = false
            //    EXTRA_PLMN = null

            IccRecords iccRecords = mIccRecords;
            String plmn = null;
            boolean showPlmn = false;
            int rule = (iccRecords != null) ? iccRecords.getDisplayRule(mSS) : 0;
            boolean noService = false;
            if (combinedRegState == ServiceState.STATE_OUT_OF_SERVICE
                    || combinedRegState == ServiceState.STATE_EMERGENCY_ONLY) {
                showPlmn = true;

                // Force display no service
                final boolean forceDisplayNoService = mPhone.getContext().getResources().getBoolean(
                        com.android.internal.R.bool.config_display_no_service_when_sim_unready)
                                && !mIsSimReady;
                if (mEmergencyOnly && !forceDisplayNoService) {
                    // No service but emergency call allowed
                    plmn = Resources.getSystem().
                            getText(com.android.internal.R.string.emergency_calls_only).toString();
                } else {
                    // No service at all
                    plmn = Resources.getSystem().
                            getText(com.android.internal.R.string.lockscreen_carrier_default).toString();
                    noService = true;
                }
                if (DBG) log("updateSpnDisplay: radio is on but out " +
                        "of service, set plmn='" + plmn + "'");
            } else if (combinedRegState == ServiceState.STATE_IN_SERVICE) {
                // In either home or roaming service
                plmn = mSS.getOperatorAlpha();
                showPlmn = !TextUtils.isEmpty(plmn) &&
                        ((rule & SIMRecords.SPN_RULE_SHOW_PLMN)
                                == SIMRecords.SPN_RULE_SHOW_PLMN);
            } else {
                // Power off state, such as airplane mode, show plmn as "No service"
                showPlmn = true;
                plmn = Resources.getSystem().
                        getText(com.android.internal.R.string.lockscreen_carrier_default).toString();
                if (DBG) log("updateSpnDisplay: radio is off w/ showPlmn="
                        + showPlmn + " plmn=" + plmn);
            }

            // The value of spn/showSpn are same in different scenarios.
            //    EXTRA_SHOW_SPN = depending on IccRecords rule and radio/IMS state
            //    EXTRA_SPN = spn
            //    EXTRA_DATA_SPN = dataSpn
            String spn = (iccRecords != null) ? iccRecords.getServiceProviderName() : "";
            String dataSpn = spn;
            boolean showSpn = !noService && !TextUtils.isEmpty(spn)
                    && ((rule & SIMRecords.SPN_RULE_SHOW_SPN)
                    == SIMRecords.SPN_RULE_SHOW_SPN);

            if (!TextUtils.isEmpty(spn) && !TextUtils.isEmpty(wfcVoiceSpnFormat) &&
                    !TextUtils.isEmpty(wfcDataSpnFormat)) {
                // Show SPN + Wi-Fi Calling If SIM has SPN and SPN display condition
                // is satisfied or SPN override is enabled for this carrier.

                String originalSpn = spn.trim();
                spn = String.format(wfcVoiceSpnFormat, originalSpn);
                dataSpn = String.format(wfcDataSpnFormat, originalSpn);
                showSpn = true;
                showPlmn = false;
            } else if (!TextUtils.isEmpty(plmn) && !TextUtils.isEmpty(wfcVoiceSpnFormat)) {
                // Show PLMN + Wi-Fi Calling if there is no valid SPN in the above case

                String originalPlmn = plmn.trim();
                plmn = String.format(wfcVoiceSpnFormat, originalPlmn);
            } else if (mSS.getVoiceRegState() == ServiceState.STATE_POWER_OFF
                    || (showPlmn && TextUtils.equals(spn, plmn))) {
                // airplane mode or spn equals plmn, do not show spn
                spn = null;
                showSpn = false;
            }

            int subId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
            int[] subIds = SubscriptionManager.getSubId(mPhone.getPhoneId());
            if (subIds != null && subIds.length > 0) {
                subId = subIds[0];
            }

            // Update SPN_STRINGS_UPDATED_ACTION IFF any value changes
            if (mSubId != subId ||
                    showPlmn != mCurShowPlmn
                    || showSpn != mCurShowSpn
                    || !TextUtils.equals(spn, mCurSpn)
                    || !TextUtils.equals(dataSpn, mCurDataSpn)
                    || !TextUtils.equals(plmn, mCurPlmn)) {
                if (DBG) {
                    log(String.format("updateSpnDisplay: changed sending intent rule=" + rule +
                            " showPlmn='%b' plmn='%s' showSpn='%b' spn='%s' dataSpn='%s' " +
                            "subId='%d'", showPlmn, plmn, showSpn, spn, dataSpn, subId));
                }
                Intent intent = new Intent(TelephonyIntents.SPN_STRINGS_UPDATED_ACTION);
                intent.putExtra(TelephonyIntents.EXTRA_SHOW_SPN, showSpn);
                intent.putExtra(TelephonyIntents.EXTRA_SPN, spn);
                intent.putExtra(TelephonyIntents.EXTRA_DATA_SPN, dataSpn);
                intent.putExtra(TelephonyIntents.EXTRA_SHOW_PLMN, showPlmn);
                intent.putExtra(TelephonyIntents.EXTRA_PLMN, plmn);
                SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhone.getPhoneId());
                mPhone.getContext().sendStickyBroadcastAsUser(intent, UserHandle.ALL);

                if (!mSubscriptionController.setPlmnSpn(mPhone.getPhoneId(),
                        showPlmn, plmn, showSpn, spn)) {
                    mSpnUpdatePending = true;
                }
            }

            mSubId = subId;
            mCurShowSpn = showSpn;
            mCurShowPlmn = showPlmn;
            mCurSpn = spn;
            mCurDataSpn = dataSpn;
            mCurPlmn = plmn;
        } else {
            // mOperatorAlpha contains the ERI text
            String plmn = mSS.getOperatorAlpha();
            boolean showPlmn = false;

            showPlmn = plmn != null;

            int subId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
            int[] subIds = SubscriptionManager.getSubId(mPhone.getPhoneId());
            if (subIds != null && subIds.length > 0) {
                subId = subIds[0];
            }

            if (!TextUtils.isEmpty(plmn) && !TextUtils.isEmpty(wfcVoiceSpnFormat)) {
                // In Wi-Fi Calling mode show SPN+WiFi

                String originalPlmn = plmn.trim();
                plmn = String.format(wfcVoiceSpnFormat, originalPlmn);
            } else if (mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF) {
                // todo: temporary hack; should have a better fix. This is to avoid using operator
                // name from ServiceState (populated in resetServiceStateInIwlanMode()) until
                // wifi calling is actually enabled
                log("updateSpnDisplay: overwriting plmn from " + plmn + " to null as radio " +
                        "state is off");
                plmn = null;
            }

            if (combinedRegState == ServiceState.STATE_OUT_OF_SERVICE) {
                plmn = Resources.getSystem().getText(com.android.internal.R.string
                        .lockscreen_carrier_default).toString();
                if (DBG) {
                    log("updateSpnDisplay: radio is on but out of svc, set plmn='" + plmn + "'");
                }
            }

            if (mSubId != subId || !TextUtils.equals(plmn, mCurPlmn)) {
                // Allow A blank plmn, "" to set showPlmn to true. Previously, we
                // would set showPlmn to true only if plmn was not empty, i.e. was not
                // null and not blank. But this would cause us to incorrectly display
                // "No Service". Now showPlmn is set to true for any non null string.
                if (DBG) {
                    log(String.format("updateSpnDisplay: changed sending intent" +
                            " showPlmn='%b' plmn='%s' subId='%d'", showPlmn, plmn, subId));
                }
                Intent intent = new Intent(TelephonyIntents.SPN_STRINGS_UPDATED_ACTION);
                intent.putExtra(TelephonyIntents.EXTRA_SHOW_SPN, false);
                intent.putExtra(TelephonyIntents.EXTRA_SPN, "");
                intent.putExtra(TelephonyIntents.EXTRA_SHOW_PLMN, showPlmn);
                intent.putExtra(TelephonyIntents.EXTRA_PLMN, plmn);
                SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhone.getPhoneId());
                mPhone.getContext().sendStickyBroadcastAsUser(intent, UserHandle.ALL);

                if (!mSubscriptionController.setPlmnSpn(mPhone.getPhoneId(),
                        showPlmn, plmn, false, "")) {
                    mSpnUpdatePending = true;
                }
            }

            mSubId = subId;
            mCurShowSpn = false;
            mCurShowPlmn = showPlmn;
            mCurSpn = "";
            mCurPlmn = plmn;
        }
    }

    protected void setPowerStateToDesired() {
        if (DBG) {
            String tmpLog = "mDeviceShuttingDown=" + mDeviceShuttingDown +
                    ", mDesiredPowerState=" + mDesiredPowerState +
                    ", getRadioState=" + mCi.getRadioState() +
                    ", mPowerOffDelayNeed=" + mPowerOffDelayNeed +
                    ", mAlarmSwitch=" + mAlarmSwitch +
                    ", mRadioDisabledByCarrier=" + mRadioDisabledByCarrier;
            log(tmpLog);
            mRadioPowerLog.log(tmpLog);
        }

        if (mPhone.isPhoneTypeGsm() && mAlarmSwitch) {
            if(DBG) log("mAlarmSwitch == true");
            Context context = mPhone.getContext();
            AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
            am.cancel(mRadioOffIntent);
            mAlarmSwitch = false;
        }

        // If we want it on and it's off, turn it on
        if (mDesiredPowerState && !mRadioDisabledByCarrier
                && mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF) {
            mCi.setRadioPower(true, null);
        } else if ((!mDesiredPowerState || mRadioDisabledByCarrier) && mCi.getRadioState().isOn()) {
            // If it's on and available and we want it off gracefully
            if (mPhone.isPhoneTypeGsm() && mPowerOffDelayNeed) {
                if (mImsRegistrationOnOff && !mAlarmSwitch) {
                    if(DBG) log("mImsRegistrationOnOff == true");
                    Context context = mPhone.getContext();
                    AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);

                    Intent intent = new Intent(ACTION_RADIO_OFF);
                    mRadioOffIntent = PendingIntent.getBroadcast(context, 0, intent, 0);

                    mAlarmSwitch = true;
                    if (DBG) log("Alarm setting");
                    am.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                            SystemClock.elapsedRealtime() + 3000, mRadioOffIntent);
                } else {
                    DcTracker dcTracker = mPhone.mDcTracker;
                    powerOffRadioSafely(dcTracker);
                }
            } else {
                DcTracker dcTracker = mPhone.mDcTracker;
                powerOffRadioSafely(dcTracker);
            }
        } else if (mDeviceShuttingDown && mCi.getRadioState().isAvailable()) {
            mCi.requestShutdown(null);
        }
    }

    protected void onUpdateIccAvailability() {
        if (mUiccController == null ) {
            return;
        }

        UiccCardApplication newUiccApplication = getUiccCardApplication();

        if (mUiccApplcation != newUiccApplication) {
            if (mUiccApplcation != null) {
                log("Removing stale icc objects.");
                mUiccApplcation.unregisterForReady(this);
                if (mIccRecords != null) {
                    mIccRecords.unregisterForRecordsLoaded(this);
                }
                mIccRecords = null;
                mUiccApplcation = null;
            }
            if (newUiccApplication != null) {
                log("New card found");
                mUiccApplcation = newUiccApplication;
                mIccRecords = mUiccApplcation.getIccRecords();
                if (mPhone.isPhoneTypeGsm()) {
                    mUiccApplcation.registerForReady(this, EVENT_SIM_READY, null);
                    if (mIccRecords != null) {
                        mIccRecords.registerForRecordsLoaded(this, EVENT_SIM_RECORDS_LOADED, null);
                    }
                } else if (mIsSubscriptionFromRuim) {
                    mUiccApplcation.registerForReady(this, EVENT_RUIM_READY, null);
                    if (mIccRecords != null) {
                        mIccRecords.registerForRecordsLoaded(this, EVENT_RUIM_RECORDS_LOADED, null);
                    }
                }
            }
        }
    }

    private void logRoamingChange() {
        mRoamingLog.log(mSS.toString());
    }

    private void logAttachChange() {
        mAttachLog.log(mSS.toString());
    }

    private void logPhoneTypeChange() {
        mPhoneTypeLog.log(Integer.toString(mPhone.getPhoneType()));
    }

    private void logRatChange() {
        mRatLog.log(mSS.toString());
    }

    protected final void log(String s) {
        Rlog.d(LOG_TAG, "[" + mPhone.getPhoneId() + "] " + s);
    }

    protected final void loge(String s) {
        Rlog.e(LOG_TAG, "[" + mPhone.getPhoneId() + "] " + s);
    }

    /**
     * @return The current GPRS state. IN_SERVICE is the same as "attached"
     * and OUT_OF_SERVICE is the same as detached.
     */
    public int getCurrentDataConnectionState() {
        return mSS.getDataRegState();
    }

    /**
     * @return true if phone is camping on a technology (eg UMTS)
     * that could support voice and data simultaneously.
     */
    public boolean isConcurrentVoiceAndDataAllowed() {
        if (mSS.getCssIndicator() == 1) {
            // Checking the Concurrent Service Supported flag first for all phone types.
            return true;
        } else if (mPhone.isPhoneTypeGsm()) {
            return (mSS.getRilDataRadioTechnology() >= ServiceState.RIL_RADIO_TECHNOLOGY_UMTS);
        } else {
            return false;
        }
    }

    /** Called when the service state of ImsPhone is changed. */
    public void onImsServiceStateChanged() {
        sendMessage(obtainMessage(EVENT_IMS_SERVICE_STATE_CHANGED));
    }

    public void setImsRegistrationState(boolean registered) {
        log("ImsRegistrationState - registered : " + registered);

        if (mImsRegistrationOnOff && !registered) {
            if (mAlarmSwitch) {
                mImsRegistrationOnOff = registered;

                Context context = mPhone.getContext();
                AlarmManager am = (AlarmManager) context.getSystemService(Context.ALARM_SERVICE);
                am.cancel(mRadioOffIntent);
                mAlarmSwitch = false;

                sendMessage(obtainMessage(EVENT_CHANGE_IMS_STATE));
                return;
            }
        }
        mImsRegistrationOnOff = registered;
    }

    public void onImsCapabilityChanged() {
        sendMessage(obtainMessage(EVENT_IMS_CAPABILITY_CHANGED));
    }

    public boolean isRadioOn() {
        return mCi.getRadioState() == CommandsInterface.RadioState.RADIO_ON;
    }

    /**
     * A complete "service state" from our perspective is
     * composed of a handful of separate requests to the radio.
     *
     * We make all of these requests at once, but then abandon them
     * and start over again if the radio notifies us that some
     * event has changed
     */
    public void pollState() {
        pollState(false);
    }
    /**
     * We insist on polling even if the radio says its off.
     * Used when we get a network changed notification
     * but the radio is off - part of iwlan hack
     */
    private void modemTriggeredPollState() {
        pollState(true);
    }

    public void pollState(boolean modemTriggered) {
        mPollingContext = new int[1];
        mPollingContext[0] = 0;

        log("pollState: modemTriggered=" + modemTriggered);

        switch (mCi.getRadioState()) {
            case RADIO_UNAVAILABLE:
                mNewSS.setStateOutOfService();
                mNewCellLoc.setStateInvalid();
                setSignalStrengthDefaultValues();
                mNitzState.handleNetworkUnavailable();
                pollStateDone();
                break;

            case RADIO_OFF:
                mNewSS.setStateOff();
                mNewCellLoc.setStateInvalid();
                setSignalStrengthDefaultValues();
                mNitzState.handleNetworkUnavailable();
                // don't poll when device is shutting down or the poll was not modemTrigged
                // (they sent us new radio data) and current network is not IWLAN
                if (mDeviceShuttingDown ||
                        (!modemTriggered && ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                        != mSS.getRilDataRadioTechnology())) {
                    pollStateDone();
                    break;
                }

            default:
                // Issue all poll-related commands at once then count down the responses, which
                // are allowed to arrive out-of-order
                // TODO: Add WLAN support.
                mPollingContext[0]++;
                mCi.getOperator(obtainMessage(EVENT_POLL_STATE_OPERATOR, mPollingContext));

                mPollingContext[0]++;
                mRegStateManagers.get(AccessNetworkConstants.TransportType.WWAN)
                        .getNetworkRegistrationState(NetworkRegistrationState.DOMAIN_PS,
                        obtainMessage(EVENT_POLL_STATE_GPRS, mPollingContext));

                mPollingContext[0]++;
                mRegStateManagers.get(AccessNetworkConstants.TransportType.WWAN)
                        .getNetworkRegistrationState(NetworkRegistrationState.DOMAIN_CS,
                        obtainMessage(EVENT_POLL_STATE_REGISTRATION, mPollingContext));

                if (mPhone.isPhoneTypeGsm()) {
                    mPollingContext[0]++;
                    mCi.getNetworkSelectionMode(obtainMessage(
                            EVENT_POLL_STATE_NETWORK_SELECTION_MODE, mPollingContext));
                }
                break;
        }
    }

    private void pollStateDone() {
        if (!mPhone.isPhoneTypeGsm()) {
            updateRoamingState();
        }

        if (Build.IS_DEBUGGABLE && SystemProperties.getBoolean(PROP_FORCE_ROAMING, false)) {
            mNewSS.setVoiceRoaming(true);
            mNewSS.setDataRoaming(true);
        }
        useDataRegStateForDataOnlyDevices();
        resetServiceStateInIwlanMode();

        if (Build.IS_DEBUGGABLE && mPhone.mTelephonyTester != null) {
            mPhone.mTelephonyTester.overrideServiceState(mNewSS);
        }

        if (DBG) {
            log("Poll ServiceState done: "
                    + " oldSS=[" + mSS + "] newSS=[" + mNewSS + "]"
                    + " oldMaxDataCalls=" + mMaxDataCalls
                    + " mNewMaxDataCalls=" + mNewMaxDataCalls
                    + " oldReasonDataDenied=" + mReasonDataDenied
                    + " mNewReasonDataDenied=" + mNewReasonDataDenied);
        }

        boolean hasRegistered =
                mSS.getVoiceRegState() != ServiceState.STATE_IN_SERVICE
                        && mNewSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE;

        boolean hasDeregistered =
                mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE
                        && mNewSS.getVoiceRegState() != ServiceState.STATE_IN_SERVICE;

        boolean hasDataAttached =
                mSS.getDataRegState() != ServiceState.STATE_IN_SERVICE
                        && mNewSS.getDataRegState() == ServiceState.STATE_IN_SERVICE;

        boolean hasDataDetached =
                mSS.getDataRegState() == ServiceState.STATE_IN_SERVICE
                        && mNewSS.getDataRegState() != ServiceState.STATE_IN_SERVICE;

        boolean hasDataRegStateChanged =
                mSS.getDataRegState() != mNewSS.getDataRegState();

        boolean hasVoiceRegStateChanged =
                mSS.getVoiceRegState() != mNewSS.getVoiceRegState();

        boolean hasLocationChanged = !mNewCellLoc.equals(mCellLoc);

        // ratchet the new tech up through its rat family but don't drop back down
        // until cell change or device is OOS
        boolean isDataInService = mNewSS.getDataRegState() == ServiceState.STATE_IN_SERVICE;

        if (isDataInService) {
            mRatRatcheter.ratchet(mSS, mNewSS, hasLocationChanged);
        }

        boolean hasRilVoiceRadioTechnologyChanged =
                mSS.getRilVoiceRadioTechnology() != mNewSS.getRilVoiceRadioTechnology();

        boolean hasRilDataRadioTechnologyChanged =
                mSS.getRilDataRadioTechnology() != mNewSS.getRilDataRadioTechnology();

        boolean hasChanged = !mNewSS.equals(mSS);

        boolean hasVoiceRoamingOn = !mSS.getVoiceRoaming() && mNewSS.getVoiceRoaming();

        boolean hasVoiceRoamingOff = mSS.getVoiceRoaming() && !mNewSS.getVoiceRoaming();

        boolean hasDataRoamingOn = !mSS.getDataRoaming() && mNewSS.getDataRoaming();

        boolean hasDataRoamingOff = mSS.getDataRoaming() && !mNewSS.getDataRoaming();

        boolean hasRejectCauseChanged = mRejectCode != mNewRejectCode;

        boolean hasCssIndicatorChanged = (mSS.getCssIndicator() != mNewSS.getCssIndicator());

        boolean has4gHandoff = false;
        boolean hasMultiApnSupport = false;
        boolean hasLostMultiApnSupport = false;
        if (mPhone.isPhoneTypeCdmaLte()) {
            has4gHandoff = mNewSS.getDataRegState() == ServiceState.STATE_IN_SERVICE
                    && ((ServiceState.isLte(mSS.getRilDataRadioTechnology())
                    && (mNewSS.getRilDataRadioTechnology()
                    == ServiceState.RIL_RADIO_TECHNOLOGY_EHRPD))
                    ||
                    ((mSS.getRilDataRadioTechnology()
                            == ServiceState.RIL_RADIO_TECHNOLOGY_EHRPD)
                            && ServiceState.isLte(mNewSS.getRilDataRadioTechnology())));

            hasMultiApnSupport = ((ServiceState.isLte(mNewSS.getRilDataRadioTechnology())
                    || (mNewSS.getRilDataRadioTechnology()
                    == ServiceState.RIL_RADIO_TECHNOLOGY_EHRPD))
                    &&
                    (!ServiceState.isLte(mSS.getRilDataRadioTechnology())
                            && (mSS.getRilDataRadioTechnology()
                            != ServiceState.RIL_RADIO_TECHNOLOGY_EHRPD)));

            hasLostMultiApnSupport =
                    ((mNewSS.getRilDataRadioTechnology()
                            >= ServiceState.RIL_RADIO_TECHNOLOGY_IS95A)
                            && (mNewSS.getRilDataRadioTechnology()
                            <= ServiceState.RIL_RADIO_TECHNOLOGY_EVDO_A));
        }

        if (DBG) {
            log("pollStateDone:"
                    + " hasRegistered=" + hasRegistered
                    + " hasDeregistered=" + hasDeregistered
                    + " hasDataAttached=" + hasDataAttached
                    + " hasDataDetached=" + hasDataDetached
                    + " hasDataRegStateChanged=" + hasDataRegStateChanged
                    + " hasRilVoiceRadioTechnologyChanged= " + hasRilVoiceRadioTechnologyChanged
                    + " hasRilDataRadioTechnologyChanged=" + hasRilDataRadioTechnologyChanged
                    + " hasChanged=" + hasChanged
                    + " hasVoiceRoamingOn=" + hasVoiceRoamingOn
                    + " hasVoiceRoamingOff=" + hasVoiceRoamingOff
                    + " hasDataRoamingOn=" + hasDataRoamingOn
                    + " hasDataRoamingOff=" + hasDataRoamingOff
                    + " hasLocationChanged=" + hasLocationChanged
                    + " has4gHandoff = " + has4gHandoff
                    + " hasMultiApnSupport=" + hasMultiApnSupport
                    + " hasLostMultiApnSupport=" + hasLostMultiApnSupport
                    + " hasCssIndicatorChanged=" + hasCssIndicatorChanged);
        }

        // Add an event log when connection state changes
        if (hasVoiceRegStateChanged || hasDataRegStateChanged) {
            EventLog.writeEvent(mPhone.isPhoneTypeGsm() ? EventLogTags.GSM_SERVICE_STATE_CHANGE :
                            EventLogTags.CDMA_SERVICE_STATE_CHANGE,
                    mSS.getVoiceRegState(), mSS.getDataRegState(),
                    mNewSS.getVoiceRegState(), mNewSS.getDataRegState());
        }

        if (mPhone.isPhoneTypeGsm()) {
            // Add an event log when network type switched
            // TODO: we may add filtering to reduce the event logged,
            // i.e. check preferred network setting, only switch to 2G, etc
            if (hasRilVoiceRadioTechnologyChanged) {
                int cid = -1;
                GsmCellLocation loc = (GsmCellLocation) mNewCellLoc;
                if (loc != null) cid = loc.getCid();
                // NOTE: this code was previously located after mSS and mNewSS are swapped, so
                // existing logs were incorrectly using the new state for "network_from"
                // and STATE_OUT_OF_SERVICE for "network_to". To avoid confusion, use a new log tag
                // to record the correct states.
                EventLog.writeEvent(EventLogTags.GSM_RAT_SWITCHED_NEW, cid,
                        mSS.getRilVoiceRadioTechnology(),
                        mNewSS.getRilVoiceRadioTechnology());
                if (DBG) {
                    log("RAT switched "
                            + ServiceState.rilRadioTechnologyToString(
                            mSS.getRilVoiceRadioTechnology())
                            + " -> "
                            + ServiceState.rilRadioTechnologyToString(
                            mNewSS.getRilVoiceRadioTechnology()) + " at cell " + cid);
                }
            }

            if (hasCssIndicatorChanged) {
                mPhone.notifyDataConnection(Phone.REASON_CSS_INDICATOR_CHANGED);
            }

            mReasonDataDenied = mNewReasonDataDenied;
            mMaxDataCalls = mNewMaxDataCalls;
            mRejectCode = mNewRejectCode;
        }

        ServiceState oldMergedSS = mPhone.getServiceState();

        // swap mSS and mNewSS to put new state in mSS
        ServiceState tss = mSS;
        mSS = mNewSS;
        mNewSS = tss;
        // clean slate for next time
        mNewSS.setStateOutOfService();

        // swap mCellLoc and mNewCellLoc to put new state in mCellLoc
        CellLocation tcl = mCellLoc;
        mCellLoc = mNewCellLoc;
        mNewCellLoc = tcl;

        if (hasRilVoiceRadioTechnologyChanged) {
            updatePhoneObject();
        }

        TelephonyManager tm =
                (TelephonyManager) mPhone.getContext().getSystemService(Context.TELEPHONY_SERVICE);

        if (hasRilDataRadioTechnologyChanged) {
            tm.setDataNetworkTypeForPhone(mPhone.getPhoneId(), mSS.getRilDataRadioTechnology());
            StatsLog.write(StatsLog.MOBILE_RADIO_TECHNOLOGY_CHANGED,
                    ServiceState.rilRadioTechnologyToNetworkType(mSS.getRilDataRadioTechnology()),
                    mPhone.getPhoneId());

            if (ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                    == mSS.getRilDataRadioTechnology()) {
                log("pollStateDone: IWLAN enabled");
            }
        }

        if (hasRegistered) {
            mNetworkAttachedRegistrants.notifyRegistrants();
            mNitzState.handleNetworkAvailable();
        }

        if (hasDeregistered) {
            mNetworkDetachedRegistrants.notifyRegistrants();
            mNitzState.handleNetworkUnavailable();
        }

        if (hasRejectCauseChanged) {
            setNotification(CS_REJECT_CAUSE_ENABLED);
        }

        if (hasChanged) {
            updateSpnDisplay();

            tm.setNetworkOperatorNameForPhone(mPhone.getPhoneId(), mSS.getOperatorAlpha());

            String prevOperatorNumeric = tm.getNetworkOperatorForPhone(mPhone.getPhoneId());
            String prevCountryIsoCode = tm.getNetworkCountryIso(mPhone.getPhoneId());
            String operatorNumeric = mSS.getOperatorNumeric();

            if (!mPhone.isPhoneTypeGsm()) {
                // try to fix the invalid Operator Numeric
                if (isInvalidOperatorNumeric(operatorNumeric)) {
                    int sid = mSS.getCdmaSystemId();
                    operatorNumeric = fixUnknownMcc(operatorNumeric, sid);
                }
            }

            tm.setNetworkOperatorNumericForPhone(mPhone.getPhoneId(), operatorNumeric);

            if (isInvalidOperatorNumeric(operatorNumeric)) {
                if (DBG) log("operatorNumeric " + operatorNumeric + " is invalid");
                // Passing empty string is important for the first update. The initial value of
                // operator numeric in locale tracker is null. The async update will allow getting
                // cell info from the modem instead of using the cached one.
                mLocaleTracker.updateOperatorNumericAsync("");
                mNitzState.handleNetworkUnavailable();
            } else if (mSS.getRilDataRadioTechnology() != ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN) {
                // If the device is on IWLAN, modems manufacture a ServiceState with the MCC/MNC of
                // the SIM as if we were talking to towers. Telephony code then uses that with
                // mccTable to suggest a timezone. We shouldn't do that if the MCC/MNC is from IWLAN

                // Update IDD.
                if (!mPhone.isPhoneTypeGsm()) {
                    setOperatorIdd(operatorNumeric);
                }

                mLocaleTracker.updateOperatorNumericSync(operatorNumeric);
                String countryIsoCode = mLocaleTracker.getCurrentCountry();

                // Update Time Zone.
                boolean iccCardExists = iccCardExists();
                boolean networkIsoChanged =
                        networkCountryIsoChanged(countryIsoCode, prevCountryIsoCode);

                // Determine countryChanged: networkIso is only reliable if there's an ICC card.
                boolean countryChanged = iccCardExists && networkIsoChanged;
                if (DBG) {
                    long ctm = System.currentTimeMillis();
                    log("Before handleNetworkCountryCodeKnown:"
                            + " countryChanged=" + countryChanged
                            + " iccCardExist=" + iccCardExists
                            + " countryIsoChanged=" + networkIsoChanged
                            + " operatorNumeric=" + operatorNumeric
                            + " prevOperatorNumeric=" + prevOperatorNumeric
                            + " countryIsoCode=" + countryIsoCode
                            + " prevCountryIsoCode=" + prevCountryIsoCode
                            + " ltod=" + TimeUtils.logTimeOfDay(ctm));
                }
                mNitzState.handleNetworkCountryCodeSet(countryChanged);
            }

            tm.setNetworkRoamingForPhone(mPhone.getPhoneId(),
                    mPhone.isPhoneTypeGsm() ? mSS.getVoiceRoaming() :
                            (mSS.getVoiceRoaming() || mSS.getDataRoaming()));

            setRoamingType(mSS);
            log("Broadcasting ServiceState : " + mSS);
            // notify using PhoneStateListener and the legacy intent ACTION_SERVICE_STATE_CHANGED
            // notify service state changed only if the merged service state is changed.
            if (!oldMergedSS.equals(mPhone.getServiceState())) {
                mPhone.notifyServiceStateChanged(mPhone.getServiceState());
            }

            // insert into ServiceStateProvider. This will trigger apps to wake through JobScheduler
            mPhone.getContext().getContentResolver()
                    .insert(getUriForSubscriptionId(mPhone.getSubId()),
                            getContentValuesForServiceState(mSS));

            TelephonyMetrics.getInstance().writeServiceStateChanged(mPhone.getPhoneId(), mSS);
        }

        if (hasDataAttached || has4gHandoff || hasDataDetached || hasRegistered
                || hasDeregistered) {
            logAttachChange();
        }

        if (hasDataAttached || has4gHandoff) {
            mAttachedRegistrants.notifyRegistrants();
        }

        if (hasDataDetached) {
            mDetachedRegistrants.notifyRegistrants();
        }

        if (hasRilDataRadioTechnologyChanged || hasRilVoiceRadioTechnologyChanged) {
            logRatChange();
        }

        if (hasDataRegStateChanged || hasRilDataRadioTechnologyChanged) {
            notifyDataRegStateRilRadioTechnologyChanged();

            if (ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                    == mSS.getRilDataRadioTechnology()) {
                mPhone.notifyDataConnection(Phone.REASON_IWLAN_AVAILABLE);
            } else {
                mPhone.notifyDataConnection(null);
            }
        }

        if (hasVoiceRoamingOn || hasVoiceRoamingOff || hasDataRoamingOn || hasDataRoamingOff) {
            logRoamingChange();
        }

        if (hasVoiceRoamingOn) {
            mVoiceRoamingOnRegistrants.notifyRegistrants();
        }

        if (hasVoiceRoamingOff) {
            mVoiceRoamingOffRegistrants.notifyRegistrants();
        }

        if (hasDataRoamingOn) {
            mDataRoamingOnRegistrants.notifyRegistrants();
        }

        if (hasDataRoamingOff) {
            mDataRoamingOffRegistrants.notifyRegistrants();
        }

        if (hasLocationChanged) {
            mPhone.notifyLocationChanged();
        }

        if (mPhone.isPhoneTypeGsm()) {
            if (!isGprsConsistent(mSS.getDataRegState(), mSS.getVoiceRegState())) {
                if (!mStartedGprsRegCheck && !mReportedGprsNoReg) {
                    mStartedGprsRegCheck = true;

                    int check_period = Settings.Global.getInt(
                            mPhone.getContext().getContentResolver(),
                            Settings.Global.GPRS_REGISTER_CHECK_PERIOD_MS,
                            DEFAULT_GPRS_CHECK_PERIOD_MILLIS);
                    sendMessageDelayed(obtainMessage(EVENT_CHECK_REPORT_GPRS),
                            check_period);
                }
            } else {
                mReportedGprsNoReg = false;
            }
        }
    }

    private void updateOperatorNameFromEri() {
        if (mPhone.isPhoneTypeCdma()) {
            if ((mCi.getRadioState().isOn()) && (!mIsSubscriptionFromRuim)) {
                String eriText;
                // Now the Phone sees the new ServiceState so it can get the new ERI text
                if (mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE) {
                    eriText = mPhone.getCdmaEriText();
                } else {
                    // Note that ServiceState.STATE_OUT_OF_SERVICE is valid used for
                    // mRegistrationState 0,2,3 and 4
                    eriText = mPhone.getContext().getText(
                            com.android.internal.R.string.roamingTextSearching).toString();
                }
                mSS.setOperatorAlphaLong(eriText);
            }
        } else if (mPhone.isPhoneTypeCdmaLte()) {
            boolean hasBrandOverride = mUiccController.getUiccCard(getPhoneId()) != null &&
                    mUiccController.getUiccCard(getPhoneId()).getOperatorBrandOverride() != null;
            if (!hasBrandOverride && (mCi.getRadioState().isOn()) && (mPhone.isEriFileLoaded()) &&
                    (!ServiceState.isLte(mSS.getRilVoiceRadioTechnology()) ||
                            mPhone.getContext().getResources().getBoolean(com.android.internal.R.
                                    bool.config_LTE_eri_for_network_name))) {
                // Only when CDMA is in service, ERI will take effect
                String eriText = mSS.getOperatorAlpha();
                // Now the Phone sees the new ServiceState so it can get the new ERI text
                if (mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE) {
                    eriText = mPhone.getCdmaEriText();
                } else if (mSS.getVoiceRegState() == ServiceState.STATE_POWER_OFF) {
                    eriText = (mIccRecords != null) ? mIccRecords.getServiceProviderName() : null;
                    if (TextUtils.isEmpty(eriText)) {
                        // Sets operator alpha property by retrieving from
                        // build-time system property
                        eriText = SystemProperties.get("ro.cdma.home.operator.alpha");
                    }
                } else if (mSS.getDataRegState() != ServiceState.STATE_IN_SERVICE) {
                    // Note that ServiceState.STATE_OUT_OF_SERVICE is valid used
                    // for mRegistrationState 0,2,3 and 4
                    eriText = mPhone.getContext()
                            .getText(com.android.internal.R.string.roamingTextSearching).toString();
                }
                mSS.setOperatorAlphaLong(eriText);
            }

            if (mUiccApplcation != null && mUiccApplcation.getState() == AppState.APPSTATE_READY &&
                    mIccRecords != null && getCombinedRegState() == ServiceState.STATE_IN_SERVICE
                    && !ServiceState.isLte(mSS.getRilVoiceRadioTechnology())) {
                // SIM is found on the device. If ERI roaming is OFF, and SID/NID matches
                // one configured in SIM, use operator name from CSIM record. Note that ERI, SID,
                // and NID are CDMA only, not applicable to LTE.
                boolean showSpn =
                        ((RuimRecords) mIccRecords).getCsimSpnDisplayCondition();
                int iconIndex = mSS.getCdmaEriIconIndex();

                if (showSpn && (iconIndex == EriInfo.ROAMING_INDICATOR_OFF)
                        && isInHomeSidNid(mSS.getCdmaSystemId(), mSS.getCdmaNetworkId())
                        && mIccRecords != null) {
                    mSS.setOperatorAlphaLong(mIccRecords.getServiceProviderName());
                }
            }
        }
    }

    /**
     * Check whether the specified SID and NID pair appears in the HOME SID/NID list
     * read from NV or SIM.
     *
     * @return true if provided sid/nid pair belongs to operator's home network.
     */
    private boolean isInHomeSidNid(int sid, int nid) {
        // if SID/NID is not available, assume this is home network.
        if (isSidsAllZeros()) return true;

        // length of SID/NID shold be same
        if (mHomeSystemId.length != mHomeNetworkId.length) return true;

        if (sid == 0) return true;

        for (int i = 0; i < mHomeSystemId.length; i++) {
            // Use SID only if NID is a reserved value.
            // SID 0 and NID 0 and 65535 are reserved. (C.0005 2.6.5.2)
            if ((mHomeSystemId[i] == sid) &&
                    ((mHomeNetworkId[i] == 0) || (mHomeNetworkId[i] == 65535) ||
                            (nid == 0) || (nid == 65535) || (mHomeNetworkId[i] == nid))) {
                return true;
            }
        }
        // SID/NID are not in the list. So device is not in home network
        return false;
    }

    protected void setOperatorIdd(String operatorNumeric) {
        // Retrieve the current country information
        // with the MCC got from opeatorNumeric.
        String idd = mHbpcdUtils.getIddByMcc(
                Integer.parseInt(operatorNumeric.substring(0,3)));
        if (idd != null && !idd.isEmpty()) {
            mPhone.setGlobalSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_IDP_STRING,
                    idd);
        } else {
            // use default "+", since we don't know the current IDP
            mPhone.setGlobalSystemProperty(TelephonyProperties.PROPERTY_OPERATOR_IDP_STRING, "+");
        }
    }

    private boolean isInvalidOperatorNumeric(String operatorNumeric) {
        return operatorNumeric == null || operatorNumeric.length() < 5 ||
                operatorNumeric.startsWith(INVALID_MCC);
    }

    private String fixUnknownMcc(String operatorNumeric, int sid) {
        if (sid <= 0) {
            // no cdma information is available, do nothing
            return operatorNumeric;
        }

        // resolve the mcc from sid;
        // if mNitzState.getSavedTimeZoneId() is null, TimeZone would get the default timeZone,
        // and the mNitzState.fixTimeZone() couldn't help, because it depends on operator Numeric;
        // if the sid is conflict and timezone is unavailable, the mcc may be not right.
        boolean isNitzTimeZone;
        TimeZone tzone;
        if (mNitzState.getSavedTimeZoneId() != null) {
            tzone = TimeZone.getTimeZone(mNitzState.getSavedTimeZoneId());
            isNitzTimeZone = true;
        } else {
            NitzData lastNitzData = mNitzState.getCachedNitzData();
            if (lastNitzData == null) {
                tzone = null;
            } else {
                tzone = TimeZoneLookupHelper.guessZoneByNitzStatic(lastNitzData);
                if (ServiceStateTracker.DBG) {
                    log("fixUnknownMcc(): guessNitzTimeZone returned "
                            + (tzone == null ? tzone : tzone.getID()));
                }
            }
            isNitzTimeZone = false;
        }

        int utcOffsetHours = 0;
        if (tzone != null) {
            utcOffsetHours = tzone.getRawOffset() / MS_PER_HOUR;
        }

        NitzData nitzData = mNitzState.getCachedNitzData();
        boolean isDst = nitzData != null && nitzData.isDst();
        int mcc = mHbpcdUtils.getMcc(sid, utcOffsetHours, (isDst ? 1 : 0), isNitzTimeZone);
        if (mcc > 0) {
            operatorNumeric = Integer.toString(mcc) + DEFAULT_MNC;
        }
        return operatorNumeric;
    }

    /**
     * Check if GPRS got registered while voice is registered.
     *
     * @param dataRegState i.e. CGREG in GSM
     * @param voiceRegState i.e. CREG in GSM
     * @return false if device only register to voice but not gprs
     */
    private boolean isGprsConsistent(int dataRegState, int voiceRegState) {
        return !((voiceRegState == ServiceState.STATE_IN_SERVICE) &&
                (dataRegState != ServiceState.STATE_IN_SERVICE));
    }

    /** convert ServiceState registration code
     * to service state */
    private int regCodeToServiceState(int code) {
        switch (code) {
            case NetworkRegistrationState.REG_STATE_HOME:
            case NetworkRegistrationState.REG_STATE_ROAMING:
                return ServiceState.STATE_IN_SERVICE;
            default:
                return ServiceState.STATE_OUT_OF_SERVICE;
        }
    }

    /**
     * code is registration state 0-5 from TS 27.007 7.2
     * returns true if registered roam, false otherwise
     */
    private boolean regCodeIsRoaming (int code) {
        return NetworkRegistrationState.REG_STATE_ROAMING == code;
    }

    private boolean isSameOperatorNameFromSimAndSS(ServiceState s) {
        String spn = ((TelephonyManager) mPhone.getContext().
                getSystemService(Context.TELEPHONY_SERVICE)).
                getSimOperatorNameForPhone(getPhoneId());

        // NOTE: in case of RUIM we should completely ignore the ERI data file and
        // mOperatorAlphaLong is set from RIL_REQUEST_OPERATOR response 0 (alpha ONS)
        String onsl = s.getOperatorAlphaLong();
        String onss = s.getOperatorAlphaShort();

        boolean equalsOnsl = !TextUtils.isEmpty(spn) && spn.equalsIgnoreCase(onsl);
        boolean equalsOnss = !TextUtils.isEmpty(spn) && spn.equalsIgnoreCase(onss);

        return (equalsOnsl || equalsOnss);
    }

    /**
     * Set roaming state if operator mcc is the same as sim mcc
     * and ons is not different from spn
     *
     * @param s ServiceState hold current ons
     * @return true if same operator
     */
    private boolean isSameNamedOperators(ServiceState s) {
        return currentMccEqualsSimMcc(s) && isSameOperatorNameFromSimAndSS(s);
    }

    /**
     * Compare SIM MCC with Operator MCC
     *
     * @param s ServiceState hold current ons
     * @return true if both are same
     */
    private boolean currentMccEqualsSimMcc(ServiceState s) {
        String simNumeric = ((TelephonyManager) mPhone.getContext().
                getSystemService(Context.TELEPHONY_SERVICE)).
                getSimOperatorNumericForPhone(getPhoneId());
        String operatorNumeric = s.getOperatorNumeric();
        boolean equalsMcc = true;

        try {
            equalsMcc = simNumeric.substring(0, 3).
                    equals(operatorNumeric.substring(0, 3));
        } catch (Exception e){
        }
        return equalsMcc;
    }

    /**
     * Do not set roaming state in case of oprators considered non-roaming.
     *
     * Can use mcc or mcc+mnc as item of
     * {@link CarrierConfigManager#KEY_NON_ROAMING_OPERATOR_STRING_ARRAY}.
     * For example, 302 or 21407. If mcc or mcc+mnc match with operator,
     * don't set roaming state.
     *
     * @param s ServiceState hold current ons
     * @return false for roaming state set
     */
    private boolean isOperatorConsideredNonRoaming(ServiceState s) {
        String operatorNumeric = s.getOperatorNumeric();
        final CarrierConfigManager configManager = (CarrierConfigManager) mPhone.getContext()
                .getSystemService(Context.CARRIER_CONFIG_SERVICE);
        String[] numericArray = null;
        if (configManager != null) {
            PersistableBundle config = configManager.getConfigForSubId(mPhone.getSubId());
            if (config != null) {
                numericArray = config.getStringArray(
                        CarrierConfigManager.KEY_NON_ROAMING_OPERATOR_STRING_ARRAY);
            }
        }
        if (ArrayUtils.isEmpty(numericArray) || operatorNumeric == null) {
            return false;
        }

        for (String numeric : numericArray) {
            if (!TextUtils.isEmpty(numeric) && operatorNumeric.startsWith(numeric)) {
                return true;
            }
        }
        return false;
    }

    private boolean isOperatorConsideredRoaming(ServiceState s) {
        String operatorNumeric = s.getOperatorNumeric();
        final CarrierConfigManager configManager = (CarrierConfigManager) mPhone.getContext()
                .getSystemService(Context.CARRIER_CONFIG_SERVICE);
        String[] numericArray = null;
        if (configManager != null) {
            PersistableBundle config = configManager.getConfigForSubId(mPhone.getSubId());
            if (config != null) {
                numericArray = config.getStringArray(
                        CarrierConfigManager.KEY_ROAMING_OPERATOR_STRING_ARRAY);
            }
        }
        if (ArrayUtils.isEmpty(numericArray) || operatorNumeric == null) {
            return false;
        }

        for (String numeric : numericArray) {
            if (!TextUtils.isEmpty(numeric) && operatorNumeric.startsWith(numeric)) {
                return true;
            }
        }
        return false;
    }

    /**
     * Set restricted state based on the OnRestrictedStateChanged notification
     * If any voice or packet restricted state changes, trigger a UI
     * notification and notify registrants when sim is ready.
     *
     * @param ar an int value of RIL_RESTRICTED_STATE_*
     */
    private void onRestrictedStateChanged(AsyncResult ar) {
        RestrictedState newRs = new RestrictedState();

        if (DBG) log("onRestrictedStateChanged: E rs "+ mRestrictedState);

        if (ar.exception == null && ar.result != null) {
            int state = (int)ar.result;

            newRs.setCsEmergencyRestricted(
                    ((state & RILConstants.RIL_RESTRICTED_STATE_CS_EMERGENCY) != 0) ||
                            ((state & RILConstants.RIL_RESTRICTED_STATE_CS_ALL) != 0) );
            //ignore the normal call and data restricted state before SIM READY
            if (mUiccApplcation != null && mUiccApplcation.getState() == AppState.APPSTATE_READY) {
                newRs.setCsNormalRestricted(
                        ((state & RILConstants.RIL_RESTRICTED_STATE_CS_NORMAL) != 0) ||
                                ((state & RILConstants.RIL_RESTRICTED_STATE_CS_ALL) != 0) );
                newRs.setPsRestricted(
                        (state & RILConstants.RIL_RESTRICTED_STATE_PS_ALL)!= 0);
            }

            if (DBG) log("onRestrictedStateChanged: new rs "+ newRs);

            if (!mRestrictedState.isPsRestricted() && newRs.isPsRestricted()) {
                mPsRestrictEnabledRegistrants.notifyRegistrants();
                setNotification(PS_ENABLED);
            } else if (mRestrictedState.isPsRestricted() && !newRs.isPsRestricted()) {
                mPsRestrictDisabledRegistrants.notifyRegistrants();
                setNotification(PS_DISABLED);
            }

            /**
             * There are two kind of cs restriction, normal and emergency. So
             * there are 4 x 4 combinations in current and new restricted states
             * and we only need to notify when state is changed.
             */
            if (mRestrictedState.isCsRestricted()) {
                if (!newRs.isAnyCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (!newRs.isCsNormalRestricted()) {
                    // remove normal restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                } else if (!newRs.isCsEmergencyRestricted()) {
                    // remove emergency restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            } else if (mRestrictedState.isCsEmergencyRestricted() &&
                    !mRestrictedState.isCsNormalRestricted()) {
                if (!newRs.isAnyCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsNormalRestricted()) {
                    // remove emergency restriction and enable normal restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            } else if (!mRestrictedState.isCsEmergencyRestricted() &&
                    mRestrictedState.isCsNormalRestricted()) {
                if (!newRs.isAnyCsRestricted()) {
                    // remove all restriction
                    setNotification(CS_DISABLED);
                } else if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsEmergencyRestricted()) {
                    // remove normal restriction and enable emergency restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                }
            } else {
                if (newRs.isCsRestricted()) {
                    // enable all restriction
                    setNotification(CS_ENABLED);
                } else if (newRs.isCsEmergencyRestricted()) {
                    // enable emergency restriction
                    setNotification(CS_EMERGENCY_ENABLED);
                } else if (newRs.isCsNormalRestricted()) {
                    // enable normal restriction
                    setNotification(CS_NORMAL_ENABLED);
                }
            }

            mRestrictedState = newRs;
        }
        log("onRestrictedStateChanged: X rs "+ mRestrictedState);
    }

    /**
     * @param workSource calling WorkSource
     * @return the current cell location information. Prefer Gsm location
     * information if available otherwise return LTE location information
     */
    public CellLocation getCellLocation(WorkSource workSource) {
        if (((GsmCellLocation)mCellLoc).getLac() >= 0 &&
                ((GsmCellLocation)mCellLoc).getCid() >= 0) {
            if (VDBG) log("getCellLocation(): X good mCellLoc=" + mCellLoc);
            return mCellLoc;
        } else {
            List<CellInfo> result = getAllCellInfo(workSource);
            if (result != null) {
                // A hack to allow tunneling of LTE information via GsmCellLocation
                // so that older Network Location Providers can return some information
                // on LTE only networks, see bug 9228974.
                //
                // We'll search the return CellInfo array preferring GSM/WCDMA
                // data, but if there is none we'll tunnel the first LTE information
                // in the list.
                //
                // The tunnel'd LTE information is returned as follows:
                //   LAC = TAC field
                //   CID = CI field
                //   PSC = 0.
                GsmCellLocation cellLocOther = new GsmCellLocation();
                for (CellInfo ci : result) {
                    if (ci instanceof CellInfoGsm) {
                        CellInfoGsm cellInfoGsm = (CellInfoGsm)ci;
                        CellIdentityGsm cellIdentityGsm = cellInfoGsm.getCellIdentity();
                        cellLocOther.setLacAndCid(cellIdentityGsm.getLac(),
                                cellIdentityGsm.getCid());
                        cellLocOther.setPsc(cellIdentityGsm.getPsc());
                        if (VDBG) log("getCellLocation(): X ret GSM info=" + cellLocOther);
                        return cellLocOther;
                    } else if (ci instanceof CellInfoWcdma) {
                        CellInfoWcdma cellInfoWcdma = (CellInfoWcdma)ci;
                        CellIdentityWcdma cellIdentityWcdma = cellInfoWcdma.getCellIdentity();
                        cellLocOther.setLacAndCid(cellIdentityWcdma.getLac(),
                                cellIdentityWcdma.getCid());
                        cellLocOther.setPsc(cellIdentityWcdma.getPsc());
                        if (VDBG) log("getCellLocation(): X ret WCDMA info=" + cellLocOther);
                        return cellLocOther;
                    } else if ((ci instanceof CellInfoLte) &&
                            ((cellLocOther.getLac() < 0) || (cellLocOther.getCid() < 0))) {
                        // We'll return the first good LTE info we get if there is no better answer
                        CellInfoLte cellInfoLte = (CellInfoLte)ci;
                        CellIdentityLte cellIdentityLte = cellInfoLte.getCellIdentity();
                        if ((cellIdentityLte.getTac() != Integer.MAX_VALUE)
                                && (cellIdentityLte.getCi() != Integer.MAX_VALUE)) {
                            cellLocOther.setLacAndCid(cellIdentityLte.getTac(),
                                    cellIdentityLte.getCi());
                            cellLocOther.setPsc(0);
                            if (VDBG) {
                                log("getCellLocation(): possible LTE cellLocOther=" + cellLocOther);
                            }
                        }
                    }
                }
                if (VDBG) {
                    log("getCellLocation(): X ret best answer cellLocOther=" + cellLocOther);
                }
                return cellLocOther;
            } else {
                if (VDBG) {
                    log("getCellLocation(): X empty mCellLoc and CellInfo mCellLoc=" + mCellLoc);
                }
                return mCellLoc;
            }
        }
    }

    /**
     * nitzReceiveTime is time_t that the NITZ time was posted
     */
    private void setTimeFromNITZString(String nitzString, long nitzReceiveTime) {
        long start = SystemClock.elapsedRealtime();
        if (DBG) {
            Rlog.d(LOG_TAG, "NITZ: " + nitzString + "," + nitzReceiveTime
                    + " start=" + start + " delay=" + (start - nitzReceiveTime));
        }
        NitzData newNitzData = NitzData.parse(nitzString);
        if (newNitzData != null) {
            try {
                TimeStampedValue<NitzData> nitzSignal =
                        new TimeStampedValue<>(newNitzData, nitzReceiveTime);
                mNitzState.handleNitzReceived(nitzSignal);
            } finally {
                if (DBG) {
                    long end = SystemClock.elapsedRealtime();
                    Rlog.d(LOG_TAG, "NITZ: end=" + end + " dur=" + (end - start));
                }
            }
        }
    }

    /**
     * Cancels all notifications posted to NotificationManager for this subId. These notifications
     * for restricted state and rejection cause for cs registration are no longer valid after the
     * SIM has been removed.
     */
    private void cancelAllNotifications() {
        if (DBG) log("cancelAllNotifications: mPrevSubId=" + mPrevSubId);
        NotificationManager notificationManager = (NotificationManager)
                mPhone.getContext().getSystemService(Context.NOTIFICATION_SERVICE);
        if (SubscriptionManager.isValidSubscriptionId(mPrevSubId)) {
            notificationManager.cancel(Integer.toString(mPrevSubId), PS_NOTIFICATION);
            notificationManager.cancel(Integer.toString(mPrevSubId), CS_NOTIFICATION);
            notificationManager.cancel(Integer.toString(mPrevSubId), CS_REJECT_CAUSE_NOTIFICATION);
        }
    }

    /**
     * Post a notification to NotificationManager for restricted state and
     * rejection cause for cs registration
     *
     * @param notifyType is one state of PS/CS_*_ENABLE/DISABLE
     */
    @VisibleForTesting
    public void setNotification(int notifyType) {
        if (DBG) log("setNotification: create notification " + notifyType);

        if (!SubscriptionManager.isValidSubscriptionId(mSubId)) {
            // notifications are posted per-sub-id, so return if current sub-id is invalid
            loge("cannot setNotification on invalid subid mSubId=" + mSubId);
            return;
        }

        // Needed because sprout RIL sends these when they shouldn't?
        boolean isSetNotification = mPhone.getContext().getResources().getBoolean(
                com.android.internal.R.bool.config_user_notification_of_restrictied_mobile_access);
        if (!isSetNotification) {
            if (DBG) log("Ignore all the notifications");
            return;
        }

        Context context = mPhone.getContext();

        CarrierConfigManager configManager = (CarrierConfigManager)
                context.getSystemService(Context.CARRIER_CONFIG_SERVICE);
        if (configManager != null) {
            PersistableBundle bundle = configManager.getConfig();
            if (bundle != null) {
                boolean disableVoiceBarringNotification = bundle.getBoolean(
                        CarrierConfigManager.KEY_DISABLE_VOICE_BARRING_NOTIFICATION_BOOL, false);
                if(disableVoiceBarringNotification && (notifyType == CS_ENABLED
                        || notifyType == CS_NORMAL_ENABLED
                        || notifyType == CS_EMERGENCY_ENABLED)) {
                    if (DBG) log("Voice/emergency call barred notification disabled");
                    return;
                }
            }
        }

        CharSequence details = "";
        CharSequence title = "";
        int notificationId = CS_NOTIFICATION;
        int icon = com.android.internal.R.drawable.stat_sys_warning;

        final boolean multipleSubscriptions = (((TelephonyManager) mPhone.getContext()
                  .getSystemService(Context.TELEPHONY_SERVICE)).getPhoneCount() > 1);
        final int simNumber = mSubscriptionController.getSlotIndex(mSubId) + 1;

        switch (notifyType) {
            case PS_ENABLED:
                long dataSubId = SubscriptionManager.getDefaultDataSubscriptionId();
                if (dataSubId != mPhone.getSubId()) {
                    return;
                }
                notificationId = PS_NOTIFICATION;
                title = context.getText(com.android.internal.R.string.RestrictedOnDataTitle);
                details = multipleSubscriptions
                        ? context.getString(
                                com.android.internal.R.string.RestrictedStateContentMsimTemplate,
                                simNumber) :
                        context.getText(com.android.internal.R.string.RestrictedStateContent);
                break;
            case PS_DISABLED:
                notificationId = PS_NOTIFICATION;
                break;
            case CS_ENABLED:
                title = context.getText(com.android.internal.R.string.RestrictedOnAllVoiceTitle);
                details = multipleSubscriptions
                        ? context.getString(
                                com.android.internal.R.string.RestrictedStateContentMsimTemplate,
                                simNumber) :
                        context.getText(com.android.internal.R.string.RestrictedStateContent);
                break;
            case CS_NORMAL_ENABLED:
                title = context.getText(com.android.internal.R.string.RestrictedOnNormalTitle);
                details = multipleSubscriptions
                        ? context.getString(
                                com.android.internal.R.string.RestrictedStateContentMsimTemplate,
                                simNumber) :
                        context.getText(com.android.internal.R.string.RestrictedStateContent);
                break;
            case CS_EMERGENCY_ENABLED:
                title = context.getText(com.android.internal.R.string.RestrictedOnEmergencyTitle);
                details = multipleSubscriptions
                        ? context.getString(
                                com.android.internal.R.string.RestrictedStateContentMsimTemplate,
                                simNumber) :
                        context.getText(com.android.internal.R.string.RestrictedStateContent);
                break;
            case CS_DISABLED:
                // do nothing and cancel the notification later
                break;
            case CS_REJECT_CAUSE_ENABLED:
                notificationId = CS_REJECT_CAUSE_NOTIFICATION;
                int resId = selectResourceForRejectCode(mRejectCode, multipleSubscriptions);
                if (0 == resId) {
                    loge("setNotification: mRejectCode=" + mRejectCode + " is not handled.");
                    return;
                } else {
                    icon = com.android.internal.R.drawable.stat_notify_mmcc_indication_icn;
                    // if using the single SIM resource, mSubId will be ignored
                    title = context.getString(resId, mSubId);
                    details = null;
                }
                break;
        }

        if (DBG) {
            log("setNotification, create notification, notifyType: " + notifyType
                    + ", title: " + title + ", details: " + details + ", subId: " + mSubId);
        }

        mNotification = new Notification.Builder(context)
                .setWhen(System.currentTimeMillis())
                .setAutoCancel(true)
                .setSmallIcon(icon)
                .setTicker(title)
                .setColor(context.getResources().getColor(
                        com.android.internal.R.color.system_notification_accent_color))
                .setContentTitle(title)
                .setStyle(new Notification.BigTextStyle().bigText(details))
                .setContentText(details)
                .setChannel(NotificationChannelController.CHANNEL_ID_ALERT)
                .build();

        NotificationManager notificationManager = (NotificationManager)
                context.getSystemService(Context.NOTIFICATION_SERVICE);

        if (notifyType == PS_DISABLED || notifyType == CS_DISABLED) {
            // cancel previous post notification
            notificationManager.cancel(Integer.toString(mSubId), notificationId);
        } else {
            boolean show = false;
            if (mSS.isEmergencyOnly() && notifyType == CS_EMERGENCY_ENABLED) {
                // if reg state is emergency only, always show restricted emergency notification.
                show = true;
            } else if (notifyType == CS_REJECT_CAUSE_ENABLED) {
                // always show notification due to CS reject irrespective of service state.
                show = true;
            } else if (mSS.getState() == ServiceState.STATE_IN_SERVICE) {
                // for non in service states, we have system UI and signal bar to indicate limited
                // service. No need to show notification again. This also helps to mitigate the
                // issue if phone go to OOS and camp to other networks and received restricted ind.
                show = true;
            }
            // update restricted state notification for this subId
            if (show) {
                notificationManager.notify(Integer.toString(mSubId), notificationId, mNotification);
            }
        }
    }

    /**
     * Selects the resource ID, which depends on rejection cause that is sent by the network when CS
     * registration is rejected.
     *
     * @param rejCode should be compatible with TS 24.008.
     */
    private int selectResourceForRejectCode(int rejCode, boolean multipleSubscriptions) {
        int rejResourceId = 0;
        switch (rejCode) {
            case 1:// Authentication reject
                rejResourceId = multipleSubscriptions
                        ? com.android.internal.R.string.mmcc_authentication_reject_msim_template :
                        com.android.internal.R.string.mmcc_authentication_reject;
                break;
            case 2:// IMSI unknown in HLR
                rejResourceId = multipleSubscriptions
                        ? com.android.internal.R.string.mmcc_imsi_unknown_in_hlr_msim_template :
                        com.android.internal.R.string.mmcc_imsi_unknown_in_hlr;
                break;
            case 3:// Illegal MS
                rejResourceId = multipleSubscriptions
                        ? com.android.internal.R.string.mmcc_illegal_ms_msim_template :
                        com.android.internal.R.string.mmcc_illegal_ms;
                break;
            case 6:// Illegal ME
                rejResourceId = multipleSubscriptions
                        ? com.android.internal.R.string.mmcc_illegal_me_msim_template :
                        com.android.internal.R.string.mmcc_illegal_me;
                break;
            default:
                // The other codes are not defined or not required by operators till now.
                break;
        }
        return rejResourceId;
    }

    private UiccCardApplication getUiccCardApplication() {
        if (mPhone.isPhoneTypeGsm()) {
            return mUiccController.getUiccCardApplication(mPhone.getPhoneId(),
                    UiccController.APP_FAM_3GPP);
        } else {
            return mUiccController.getUiccCardApplication(mPhone.getPhoneId(),
                    UiccController.APP_FAM_3GPP2);
        }
    }

    private void queueNextSignalStrengthPoll() {
        if (mDontPollSignalStrength) {
            // The radio is telling us about signal strength changes
            // we don't have to ask it
            return;
        }

        Message msg;

        msg = obtainMessage();
        msg.what = EVENT_POLL_SIGNAL_STRENGTH;

        long nextTime;

        // TODO Don't poll signal strength if screen is off
        sendMessageDelayed(msg, POLL_PERIOD_MILLIS);
    }

    private void notifyCdmaSubscriptionInfoReady() {
        if (mCdmaForSubscriptionInfoReadyRegistrants != null) {
            if (DBG) log("CDMA_SUBSCRIPTION: call notifyRegistrants()");
            mCdmaForSubscriptionInfoReadyRegistrants.notifyRegistrants();
        }
    }

    /**
     * Registration point for transition into DataConnection attached.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForDataConnectionAttached(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mAttachedRegistrants.add(r);

        if (getCurrentDataConnectionState() == ServiceState.STATE_IN_SERVICE) {
            r.notifyRegistrant();
        }
    }
    public void unregisterForDataConnectionAttached(Handler h) {
        mAttachedRegistrants.remove(h);
    }

    /**
     * Registration point for transition into DataConnection detached.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForDataConnectionDetached(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mDetachedRegistrants.add(r);

        if (getCurrentDataConnectionState() != ServiceState.STATE_IN_SERVICE) {
            r.notifyRegistrant();
        }
    }
    public void unregisterForDataConnectionDetached(Handler h) {
        mDetachedRegistrants.remove(h);
    }

    /**
     * Registration for DataConnection RIL Data Radio Technology changing. The
     * new radio technology will be returned AsyncResult#result as an Integer Object.
     * The AsyncResult will be in the notification Message#obj.
     *
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForDataRegStateOrRatChanged(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mDataRegStateOrRatChangedRegistrants.add(r);
        notifyDataRegStateRilRadioTechnologyChanged();
    }
    public void unregisterForDataRegStateOrRatChanged(Handler h) {
        mDataRegStateOrRatChangedRegistrants.remove(h);
    }

    /**
     * Registration point for transition into network attached.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj in Message.obj
     */
    public void registerForNetworkAttached(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);

        mNetworkAttachedRegistrants.add(r);
        if (mSS.getVoiceRegState() == ServiceState.STATE_IN_SERVICE) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForNetworkAttached(Handler h) {
        mNetworkAttachedRegistrants.remove(h);
    }

    /**
     * Registration point for transition into network detached.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj in Message.obj
     */
    public void registerForNetworkDetached(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);

        mNetworkDetachedRegistrants.add(r);
        if (mSS.getVoiceRegState() != ServiceState.STATE_IN_SERVICE) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForNetworkDetached(Handler h) {
        mNetworkDetachedRegistrants.remove(h);
    }

    /**
     * Registration point for transition into packet service restricted zone.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForPsRestrictedEnabled(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mPsRestrictEnabledRegistrants.add(r);

        if (mRestrictedState.isPsRestricted()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForPsRestrictedEnabled(Handler h) {
        mPsRestrictEnabledRegistrants.remove(h);
    }

    /**
     * Registration point for transition out of packet service restricted zone.
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForPsRestrictedDisabled(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mPsRestrictDisabledRegistrants.add(r);

        if (mRestrictedState.isPsRestricted()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForPsRestrictedDisabled(Handler h) {
        mPsRestrictDisabledRegistrants.remove(h);
    }

    /**
     * Clean up existing voice and data connection then turn off radio power.
     *
     * Hang up the existing voice calls to decrease call drop rate.
     */
    public void powerOffRadioSafely(DcTracker dcTracker) {
        synchronized (this) {
            if (!mPendingRadioPowerOffAfterDataOff) {
                int dds = SubscriptionManager.getDefaultDataSubscriptionId();
                // To minimize race conditions we call cleanUpAllConnections on
                // both if else paths instead of before this isDisconnected test.
                if (dcTracker.isDisconnected()
                        && (dds == mPhone.getSubId()
                        || (dds != mPhone.getSubId()
                        && ProxyController.getInstance().isDataDisconnected(dds)))) {
                    // To minimize race conditions we do this after isDisconnected
                    dcTracker.cleanUpAllConnections(Phone.REASON_RADIO_TURNED_OFF);
                    if (DBG) log("Data disconnected, turn off radio right away.");
                    hangupAndPowerOff();
                } else {
                    // hang up all active voice calls first
                    if (mPhone.isPhoneTypeGsm() && mPhone.isInCall()) {
                        mPhone.mCT.mRingingCall.hangupIfAlive();
                        mPhone.mCT.mBackgroundCall.hangupIfAlive();
                        mPhone.mCT.mForegroundCall.hangupIfAlive();
                    }
                    dcTracker.cleanUpAllConnections(Phone.REASON_RADIO_TURNED_OFF);
                    if (dds != mPhone.getSubId()
                            && !ProxyController.getInstance().isDataDisconnected(dds)) {
                        if (DBG) log("Data is active on DDS.  Wait for all data disconnect");
                        // Data is not disconnected on DDS. Wait for the data disconnect complete
                        // before sending the RADIO_POWER off.
                        ProxyController.getInstance().registerForAllDataDisconnected(dds, this,
                                EVENT_ALL_DATA_DISCONNECTED, null);
                        mPendingRadioPowerOffAfterDataOff = true;
                    }
                    Message msg = Message.obtain(this);
                    msg.what = EVENT_SET_RADIO_POWER_OFF;
                    msg.arg1 = ++mPendingRadioPowerOffAfterDataOffTag;
                    if (sendMessageDelayed(msg, 30000)) {
                        if (DBG) log("Wait upto 30s for data to disconnect, then turn off radio.");
                        mPendingRadioPowerOffAfterDataOff = true;
                    } else {
                        log("Cannot send delayed Msg, turn off radio right away.");
                        hangupAndPowerOff();
                        mPendingRadioPowerOffAfterDataOff = false;
                    }
                }
            }
        }
    }

    /**
     * process the pending request to turn radio off after data is disconnected
     *
     * return true if there is pending request to process; false otherwise.
     */
    public boolean processPendingRadioPowerOffAfterDataOff() {
        synchronized(this) {
            if (mPendingRadioPowerOffAfterDataOff) {
                if (DBG) log("Process pending request to turn radio off.");
                mPendingRadioPowerOffAfterDataOffTag += 1;
                hangupAndPowerOff();
                mPendingRadioPowerOffAfterDataOff = false;
                return true;
            }
            return false;
        }
    }

    /**
     * Checks if the provided earfcn falls withing the range of earfcns.
     *
     * return true if earfcn falls within the provided range; false otherwise.
     */
    private boolean containsEarfcnInEarfcnRange(ArrayList<Pair<Integer, Integer>> earfcnPairList,
            int earfcn) {
        if (earfcnPairList != null) {
            for (Pair<Integer, Integer> earfcnPair : earfcnPairList) {
                if ((earfcn >= earfcnPair.first) && (earfcn <= earfcnPair.second)) {
                    return true;
                }
            }
        }

        return false;
    }

    /**
     * Convert the earfcnStringArray to list of pairs.
     *
     * Format of the earfcnsList is expected to be {"erafcn1_start-earfcn1_end",
     * "earfcn2_start-earfcn2_end" ... }
     */
    ArrayList<Pair<Integer, Integer>> convertEarfcnStringArrayToPairList(String[] earfcnsList) {
        ArrayList<Pair<Integer, Integer>> earfcnPairList = new ArrayList<Pair<Integer, Integer>>();

        if (earfcnsList != null) {
            int earfcnStart;
            int earfcnEnd;
            for (int i = 0; i < earfcnsList.length; i++) {
                try {
                    String[] earfcns = earfcnsList[i].split("-");
                    if (earfcns.length != 2) {
                        if (VDBG) {
                            log("Invalid earfcn range format");
                        }
                        return null;
                    }

                    earfcnStart = Integer.parseInt(earfcns[0]);
                    earfcnEnd = Integer.parseInt(earfcns[1]);

                    if (earfcnStart > earfcnEnd) {
                        if (VDBG) {
                            log("Invalid earfcn range format");
                        }
                        return null;
                    }

                    earfcnPairList.add(new Pair<Integer, Integer>(earfcnStart, earfcnEnd));
                } catch (PatternSyntaxException pse) {
                    if (VDBG) {
                        log("Invalid earfcn range format");
                    }
                    return null;
                } catch (NumberFormatException nfe) {
                    if (VDBG) {
                        log("Invalid earfcn number format");
                    }
                    return null;
                }
            }
        }

        return earfcnPairList;
    }

    private void onCarrierConfigChanged() {
        CarrierConfigManager configManager = (CarrierConfigManager)
                mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        PersistableBundle config = configManager.getConfigForSubId(mPhone.getSubId());

        if (config != null) {
            updateLteEarfcnLists(config);
            updateReportingCriteria(config);
        }
    }

    private void updateLteEarfcnLists(PersistableBundle config) {
        synchronized (mLteRsrpBoostLock) {
            mLteRsrpBoost = config.getInt(CarrierConfigManager.KEY_LTE_EARFCNS_RSRP_BOOST_INT, 0);
            String[] earfcnsStringArrayForRsrpBoost = config.getStringArray(
                    CarrierConfigManager.KEY_BOOSTED_LTE_EARFCNS_STRING_ARRAY);
            mEarfcnPairListForRsrpBoost = convertEarfcnStringArrayToPairList(
                    earfcnsStringArrayForRsrpBoost);
        }
    }

    private void updateReportingCriteria(PersistableBundle config) {
        mPhone.setSignalStrengthReportingCriteria(
                config.getIntArray(CarrierConfigManager.KEY_LTE_RSRP_THRESHOLDS_INT_ARRAY),
                AccessNetworkType.EUTRAN);
        mPhone.setSignalStrengthReportingCriteria(
                config.getIntArray(CarrierConfigManager.KEY_WCDMA_RSCP_THRESHOLDS_INT_ARRAY),
                AccessNetworkType.UTRAN);
    }

    private void updateServiceStateLteEarfcnBoost(ServiceState serviceState, int lteEarfcn) {
        synchronized (mLteRsrpBoostLock) {
            if ((lteEarfcn != INVALID_LTE_EARFCN)
                    && containsEarfcnInEarfcnRange(mEarfcnPairListForRsrpBoost, lteEarfcn)) {
                serviceState.setLteEarfcnRsrpBoost(mLteRsrpBoost);
            } else {
                serviceState.setLteEarfcnRsrpBoost(0);
            }
        }
    }

    /**
     * send signal-strength-changed notification if changed Called both for
     * solicited and unsolicited signal strength updates
     *
     * @return true if the signal strength changed and a notification was sent.
     */
    protected boolean onSignalStrengthResult(AsyncResult ar) {
        boolean isGsm = false;
        int dataRat = mSS.getRilDataRadioTechnology();
        int voiceRat = mSS.getRilVoiceRadioTechnology();

        // Override isGsm based on currently camped data and voice RATs
        // Set isGsm to true if the RAT belongs to GSM family and not IWLAN
        if ((dataRat != ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                && ServiceState.isGsm(dataRat))
                || (voiceRat != ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                && ServiceState.isGsm(voiceRat))) {
            isGsm = true;
        }

        // This signal is used for both voice and data radio signal so parse
        // all fields

        if ((ar.exception == null) && (ar.result != null)) {
            mSignalStrength = (SignalStrength) ar.result;
            mSignalStrength.validateInput();
            if (dataRat == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN
                    && voiceRat == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
                mSignalStrength.fixType();
            } else {
                mSignalStrength.setGsm(isGsm);
            }
            mSignalStrength.setLteRsrpBoost(mSS.getLteEarfcnRsrpBoost());

            PersistableBundle config = getCarrierConfig();
            mSignalStrength.setUseOnlyRsrpForLteLevel(config.getBoolean(
                    CarrierConfigManager.KEY_USE_ONLY_RSRP_FOR_LTE_SIGNAL_BAR_BOOL));
            mSignalStrength.setLteRsrpThresholds(config.getIntArray(
                    CarrierConfigManager.KEY_LTE_RSRP_THRESHOLDS_INT_ARRAY));
            mSignalStrength.setWcdmaDefaultSignalMeasurement(config.getString(
                    CarrierConfigManager.KEY_WCDMA_DEFAULT_SIGNAL_STRENGTH_MEASUREMENT_STRING));
            mSignalStrength.setWcdmaRscpThresholds(config.getIntArray(
                    CarrierConfigManager.KEY_WCDMA_RSCP_THRESHOLDS_INT_ARRAY));
        } else {
            log("onSignalStrengthResult() Exception from RIL : " + ar.exception);
            mSignalStrength = new SignalStrength(isGsm);
        }

        boolean ssChanged = notifySignalStrength();

        return ssChanged;
    }

    /**
     * Hang up all voice call and turn off radio. Implemented by derived class.
     */
    protected void hangupAndPowerOff() {
        // hang up all active voice calls
        if (!mPhone.isPhoneTypeGsm() || mPhone.isInCall()) {
            mPhone.mCT.mRingingCall.hangupIfAlive();
            mPhone.mCT.mBackgroundCall.hangupIfAlive();
            mPhone.mCT.mForegroundCall.hangupIfAlive();
        }

        mCi.setRadioPower(false, obtainMessage(EVENT_RADIO_POWER_OFF_DONE));

    }

    /** Cancel a pending (if any) pollState() operation */
    protected void cancelPollState() {
        // This will effectively cancel the rest of the poll requests.
        mPollingContext = new int[1];
    }

    /**
     * Return true if the network operator's country code changed.
     */
    private boolean networkCountryIsoChanged(String newCountryIsoCode, String prevCountryIsoCode) {
        // Return false if the new ISO code isn't valid as we don't know where we are.
        // Return true if the previous ISO code wasn't valid, or if it was and the new one differs.

        // If newCountryIsoCode is invalid then we'll return false
        if (TextUtils.isEmpty(newCountryIsoCode)) {
            if (DBG) {
                log("countryIsoChanged: no new country ISO code");
            }
            return false;
        }

        if (TextUtils.isEmpty(prevCountryIsoCode)) {
            if (DBG) {
                log("countryIsoChanged: no previous country ISO code");
            }
            return true;
        }
        return !newCountryIsoCode.equals(prevCountryIsoCode);
    }

    // Determine if the Icc card exists
    private boolean iccCardExists() {
        boolean iccCardExist = false;
        if (mUiccApplcation != null) {
            iccCardExist = mUiccApplcation.getState() != AppState.APPSTATE_UNKNOWN;
        }
        return iccCardExist;
    }

    public String getSystemProperty(String property, String defValue) {
        return TelephonyManager.getTelephonyProperty(mPhone.getPhoneId(), property, defValue);
    }

    /**
     * @return all available cell information or null if none.
     */
    public List<CellInfo> getAllCellInfo(WorkSource workSource) {
        CellInfoResult result = new CellInfoResult();
        if (VDBG) log("SST.getAllCellInfo(): E");
        int ver = mCi.getRilVersion();
        if (ver >= 8) {
            if (isCallerOnDifferentThread()) {
                if ((SystemClock.elapsedRealtime() - mLastCellInfoListTime)
                        > LAST_CELL_INFO_LIST_MAX_AGE_MS) {
                    Message msg = obtainMessage(EVENT_GET_CELL_INFO_LIST, result);
                    synchronized(result.lockObj) {
                        result.list = null;
                        mCi.getCellInfoList(msg, workSource);
                        try {
                            result.lockObj.wait(5000);
                        } catch (InterruptedException e) {
                            e.printStackTrace();
                        }
                    }
                } else {
                    if (DBG) log("SST.getAllCellInfo(): return last, back to back calls");
                    result.list = mLastCellInfoList;
                }
            } else {
                if (DBG) log("SST.getAllCellInfo(): return last, same thread can't block");
                result.list = mLastCellInfoList;
            }
        } else {
            if (DBG) log("SST.getAllCellInfo(): not implemented");
            result.list = null;
        }
        synchronized(result.lockObj) {
            if (result.list != null) {
                if (VDBG) log("SST.getAllCellInfo(): X size=" + result.list.size()
                        + " list=" + result.list);
                return result.list;
            } else {
                if (DBG) log("SST.getAllCellInfo(): X size=0 list=null");
                return null;
            }
        }
    }

    /**
     * @return signal strength
     */
    public SignalStrength getSignalStrength() {
        return mSignalStrength;
    }

    /**
     * Registration point for subscription info ready
     * @param h handler to notify
     * @param what what code of message when delivered
     * @param obj placed in Message.obj
     */
    public void registerForSubscriptionInfoReady(Handler h, int what, Object obj) {
        Registrant r = new Registrant(h, what, obj);
        mCdmaForSubscriptionInfoReadyRegistrants.add(r);

        if (isMinInfoReady()) {
            r.notifyRegistrant();
        }
    }

    public void unregisterForSubscriptionInfoReady(Handler h) {
        mCdmaForSubscriptionInfoReadyRegistrants.remove(h);
    }

    /**
     * Save current source of cdma subscription
     * @param source - 1 for NV, 0 for RUIM
     */
    private void saveCdmaSubscriptionSource(int source) {
        log("Storing cdma subscription source: " + source);
        Settings.Global.putInt(mPhone.getContext().getContentResolver(),
                Settings.Global.CDMA_SUBSCRIPTION_MODE,
                source);
        log("Read from settings: " + Settings.Global.getInt(mPhone.getContext().getContentResolver(),
                Settings.Global.CDMA_SUBSCRIPTION_MODE, -1));
    }

    private void getSubscriptionInfoAndStartPollingThreads() {
        mCi.getCDMASubscription(obtainMessage(EVENT_POLL_STATE_CDMA_SUBSCRIPTION));

        // Get Registration Information
        pollState();
    }

    private void handleCdmaSubscriptionSource(int newSubscriptionSource) {
        log("Subscription Source : " + newSubscriptionSource);
        mIsSubscriptionFromRuim =
                (newSubscriptionSource == CdmaSubscriptionSourceManager.SUBSCRIPTION_FROM_RUIM);
        log("isFromRuim: " + mIsSubscriptionFromRuim);
        saveCdmaSubscriptionSource(newSubscriptionSource);
        if (!mIsSubscriptionFromRuim) {
            // NV is ready when subscription source is NV
            sendMessage(obtainMessage(EVENT_NV_READY));
        }
    }

    private void dumpEarfcnPairList(PrintWriter pw) {
        pw.print(" mEarfcnPairListForRsrpBoost={");
        if (mEarfcnPairListForRsrpBoost != null) {
            int i = mEarfcnPairListForRsrpBoost.size();
            for (Pair<Integer, Integer> earfcnPair : mEarfcnPairListForRsrpBoost) {
                pw.print("(");
                pw.print(earfcnPair.first);
                pw.print(",");
                pw.print(earfcnPair.second);
                pw.print(")");
                if ((--i) != 0) {
                    pw.print(",");
                }
            }
        }
        pw.println("}");
    }

    private void dumpCellInfoList(PrintWriter pw) {
        pw.print(" mLastCellInfoList={");
        if(mLastCellInfoList != null) {
            boolean first = true;
            for(CellInfo info : mLastCellInfoList) {
               if(first == false) {
                   pw.print(",");
               }
               first = false;
               pw.print(info.toString());
            }
        }
        pw.println("}");
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("ServiceStateTracker:");
        pw.println(" mSubId=" + mSubId);
        pw.println(" mSS=" + mSS);
        pw.println(" mNewSS=" + mNewSS);
        pw.println(" mVoiceCapable=" + mVoiceCapable);
        pw.println(" mRestrictedState=" + mRestrictedState);
        pw.println(" mPollingContext=" + mPollingContext + " - " +
                (mPollingContext != null ? mPollingContext[0] : ""));
        pw.println(" mDesiredPowerState=" + mDesiredPowerState);
        pw.println(" mDontPollSignalStrength=" + mDontPollSignalStrength);
        pw.println(" mSignalStrength=" + mSignalStrength);
        pw.println(" mLastSignalStrength=" + mLastSignalStrength);
        pw.println(" mRestrictedState=" + mRestrictedState);
        pw.println(" mPendingRadioPowerOffAfterDataOff=" + mPendingRadioPowerOffAfterDataOff);
        pw.println(" mPendingRadioPowerOffAfterDataOffTag=" + mPendingRadioPowerOffAfterDataOffTag);
        pw.println(" mCellLoc=" + Rlog.pii(VDBG, mCellLoc));
        pw.println(" mNewCellLoc=" + Rlog.pii(VDBG, mNewCellLoc));
        pw.println(" mLastCellInfoListTime=" + mLastCellInfoListTime);
        dumpCellInfoList(pw);
        pw.flush();
        pw.println(" mPreferredNetworkType=" + mPreferredNetworkType);
        pw.println(" mMaxDataCalls=" + mMaxDataCalls);
        pw.println(" mNewMaxDataCalls=" + mNewMaxDataCalls);
        pw.println(" mReasonDataDenied=" + mReasonDataDenied);
        pw.println(" mNewReasonDataDenied=" + mNewReasonDataDenied);
        pw.println(" mGsmRoaming=" + mGsmRoaming);
        pw.println(" mDataRoaming=" + mDataRoaming);
        pw.println(" mEmergencyOnly=" + mEmergencyOnly);
        pw.flush();
        mNitzState.dumpState(pw);
        pw.flush();
        pw.println(" mStartedGprsRegCheck=" + mStartedGprsRegCheck);
        pw.println(" mReportedGprsNoReg=" + mReportedGprsNoReg);
        pw.println(" mNotification=" + mNotification);
        pw.println(" mCurSpn=" + mCurSpn);
        pw.println(" mCurDataSpn=" + mCurDataSpn);
        pw.println(" mCurShowSpn=" + mCurShowSpn);
        pw.println(" mCurPlmn=" + mCurPlmn);
        pw.println(" mCurShowPlmn=" + mCurShowPlmn);
        pw.flush();
        pw.println(" mCurrentOtaspMode=" + mCurrentOtaspMode);
        pw.println(" mRoamingIndicator=" + mRoamingIndicator);
        pw.println(" mIsInPrl=" + mIsInPrl);
        pw.println(" mDefaultRoamingIndicator=" + mDefaultRoamingIndicator);
        pw.println(" mRegistrationState=" + mRegistrationState);
        pw.println(" mMdn=" + mMdn);
        pw.println(" mHomeSystemId=" + mHomeSystemId);
        pw.println(" mHomeNetworkId=" + mHomeNetworkId);
        pw.println(" mMin=" + mMin);
        pw.println(" mPrlVersion=" + mPrlVersion);
        pw.println(" mIsMinInfoReady=" + mIsMinInfoReady);
        pw.println(" mIsEriTextLoaded=" + mIsEriTextLoaded);
        pw.println(" mIsSubscriptionFromRuim=" + mIsSubscriptionFromRuim);
        pw.println(" mCdmaSSM=" + mCdmaSSM);
        pw.println(" mRegistrationDeniedReason=" + mRegistrationDeniedReason);
        pw.println(" mCurrentCarrier=" + mCurrentCarrier);
        pw.flush();
        pw.println(" mImsRegistered=" + mImsRegistered);
        pw.println(" mImsRegistrationOnOff=" + mImsRegistrationOnOff);
        pw.println(" mAlarmSwitch=" + mAlarmSwitch);
        pw.println(" mRadioDisabledByCarrier" + mRadioDisabledByCarrier);
        pw.println(" mPowerOffDelayNeed=" + mPowerOffDelayNeed);
        pw.println(" mDeviceShuttingDown=" + mDeviceShuttingDown);
        pw.println(" mSpnUpdatePending=" + mSpnUpdatePending);
        pw.println(" mLteRsrpBoost=" + mLteRsrpBoost);
        dumpEarfcnPairList(pw);

        mLocaleTracker.dump(fd, pw, args);

        pw.println(" Roaming Log:");
        IndentingPrintWriter ipw = new IndentingPrintWriter(pw, "  ");
        ipw.increaseIndent();
        mRoamingLog.dump(fd, ipw, args);
        ipw.decreaseIndent();

        ipw.println(" Attach Log:");
        ipw.increaseIndent();
        mAttachLog.dump(fd, ipw, args);
        ipw.decreaseIndent();

        ipw.println(" Phone Change Log:");
        ipw.increaseIndent();
        mPhoneTypeLog.dump(fd, ipw, args);
        ipw.decreaseIndent();

        ipw.println(" Rat Change Log:");
        ipw.increaseIndent();
        mRatLog.dump(fd, ipw, args);
        ipw.decreaseIndent();

        ipw.println(" Radio power Log:");
        ipw.increaseIndent();
        mRadioPowerLog.dump(fd, ipw, args);

        mNitzState.dumpLogs(fd, ipw, args);
    }

    public boolean isImsRegistered() {
        return mImsRegistered;
    }
    /**
     * Verifies the current thread is the same as the thread originally
     * used in the initialization of this instance. Throws RuntimeException
     * if not.
     *
     * @exception RuntimeException if the current thread is not
     * the thread that originally obtained this Phone instance.
     */
    protected void checkCorrectThread() {
        if (Thread.currentThread() != getLooper().getThread()) {
            throw new RuntimeException(
                    "ServiceStateTracker must be used from within one thread");
        }
    }

    protected boolean isCallerOnDifferentThread() {
        boolean value = Thread.currentThread() != getLooper().getThread();
        if (VDBG) log("isCallerOnDifferentThread: " + value);
        return value;
    }

    protected void updateCarrierMccMncConfiguration(String newOp, String oldOp, Context context) {
        // if we have a change in operator, notify wifi (even to/from none)
        if (((newOp == null) && (TextUtils.isEmpty(oldOp) == false)) ||
                ((newOp != null) && (newOp.equals(oldOp) == false))) {
            log("update mccmnc=" + newOp + " fromServiceState=true");
            MccTable.updateMccMncConfiguration(context, newOp, true);
        }
    }

    /**
     * Check ISO country by MCC to see if phone is roaming in same registered country
     */
    protected boolean inSameCountry(String operatorNumeric) {
        if (TextUtils.isEmpty(operatorNumeric) || (operatorNumeric.length() < 5)) {
            // Not a valid network
            return false;
        }
        final String homeNumeric = getHomeOperatorNumeric();
        if (TextUtils.isEmpty(homeNumeric) || (homeNumeric.length() < 5)) {
            // Not a valid SIM MCC
            return false;
        }
        boolean inSameCountry = true;
        final String networkMCC = operatorNumeric.substring(0, 3);
        final String homeMCC = homeNumeric.substring(0, 3);
        final String networkCountry = MccTable.countryCodeForMcc(Integer.parseInt(networkMCC));
        final String homeCountry = MccTable.countryCodeForMcc(Integer.parseInt(homeMCC));
        if (networkCountry.isEmpty() || homeCountry.isEmpty()) {
            // Not a valid country
            return false;
        }
        inSameCountry = homeCountry.equals(networkCountry);
        if (inSameCountry) {
            return inSameCountry;
        }
        // special same country cases
        if ("us".equals(homeCountry) && "vi".equals(networkCountry)) {
            inSameCountry = true;
        } else if ("vi".equals(homeCountry) && "us".equals(networkCountry)) {
            inSameCountry = true;
        }
        return inSameCountry;
    }

    /**
     * Set both voice and data roaming type,
     * judging from the ISO country of SIM VS network.
     */
    protected void setRoamingType(ServiceState currentServiceState) {
        final boolean isVoiceInService =
                (currentServiceState.getVoiceRegState() == ServiceState.STATE_IN_SERVICE);
        if (isVoiceInService) {
            if (currentServiceState.getVoiceRoaming()) {
                if (mPhone.isPhoneTypeGsm()) {
                    // check roaming type by MCC
                    if (inSameCountry(currentServiceState.getVoiceOperatorNumeric())) {
                        currentServiceState.setVoiceRoamingType(
                                ServiceState.ROAMING_TYPE_DOMESTIC);
                    } else {
                        currentServiceState.setVoiceRoamingType(
                                ServiceState.ROAMING_TYPE_INTERNATIONAL);
                    }
                } else {
                    // some carrier defines international roaming by indicator
                    int[] intRoamingIndicators = mPhone.getContext().getResources().getIntArray(
                            com.android.internal.R.array.config_cdma_international_roaming_indicators);
                    if ((intRoamingIndicators != null) && (intRoamingIndicators.length > 0)) {
                        // It's domestic roaming at least now
                        currentServiceState.setVoiceRoamingType(ServiceState.ROAMING_TYPE_DOMESTIC);
                        int curRoamingIndicator = currentServiceState.getCdmaRoamingIndicator();
                        for (int i = 0; i < intRoamingIndicators.length; i++) {
                            if (curRoamingIndicator == intRoamingIndicators[i]) {
                                currentServiceState.setVoiceRoamingType(
                                        ServiceState.ROAMING_TYPE_INTERNATIONAL);
                                break;
                            }
                        }
                    } else {
                        // check roaming type by MCC
                        if (inSameCountry(currentServiceState.getVoiceOperatorNumeric())) {
                            currentServiceState.setVoiceRoamingType(
                                    ServiceState.ROAMING_TYPE_DOMESTIC);
                        } else {
                            currentServiceState.setVoiceRoamingType(
                                    ServiceState.ROAMING_TYPE_INTERNATIONAL);
                        }
                    }
                }
            } else {
                currentServiceState.setVoiceRoamingType(ServiceState.ROAMING_TYPE_NOT_ROAMING);
            }
        }
        final boolean isDataInService =
                (currentServiceState.getDataRegState() == ServiceState.STATE_IN_SERVICE);
        final int dataRegType = currentServiceState.getRilDataRadioTechnology();
        if (isDataInService) {
            if (!currentServiceState.getDataRoaming()) {
                currentServiceState.setDataRoamingType(ServiceState.ROAMING_TYPE_NOT_ROAMING);
            } else {
                if (mPhone.isPhoneTypeGsm()) {
                    if (ServiceState.isGsm(dataRegType)) {
                        if (isVoiceInService) {
                            // GSM data should have the same state as voice
                            currentServiceState.setDataRoamingType(currentServiceState
                                    .getVoiceRoamingType());
                        } else {
                            // we can not decide GSM data roaming type without voice
                            currentServiceState.setDataRoamingType(ServiceState.ROAMING_TYPE_UNKNOWN);
                        }
                    } else {
                        // we can not decide 3gpp2 roaming state here
                        currentServiceState.setDataRoamingType(ServiceState.ROAMING_TYPE_UNKNOWN);
                    }
                } else {
                    if (ServiceState.isCdma(dataRegType)) {
                        if (isVoiceInService) {
                            // CDMA data should have the same state as voice
                            currentServiceState.setDataRoamingType(currentServiceState
                                    .getVoiceRoamingType());
                        } else {
                            // we can not decide CDMA data roaming type without voice
                            // set it as same as last time
                            currentServiceState.setDataRoamingType(ServiceState.ROAMING_TYPE_UNKNOWN);
                        }
                    } else {
                        // take it as 3GPP roaming
                        if (inSameCountry(currentServiceState.getDataOperatorNumeric())) {
                            currentServiceState.setDataRoamingType(ServiceState.ROAMING_TYPE_DOMESTIC);
                        } else {
                            currentServiceState.setDataRoamingType(
                                    ServiceState.ROAMING_TYPE_INTERNATIONAL);
                        }
                    }
                }
            }
        }
    }

    private void setSignalStrengthDefaultValues() {
        mSignalStrength = new SignalStrength(true);
    }

    protected String getHomeOperatorNumeric() {
        String numeric = ((TelephonyManager) mPhone.getContext().
                getSystemService(Context.TELEPHONY_SERVICE)).
                getSimOperatorNumericForPhone(mPhone.getPhoneId());
        if (!mPhone.isPhoneTypeGsm() && TextUtils.isEmpty(numeric)) {
            numeric = SystemProperties.get(GsmCdmaPhone.PROPERTY_CDMA_HOME_OPERATOR_NUMERIC, "");
        }
        return numeric;
    }

    protected int getPhoneId() {
        return mPhone.getPhoneId();
    }

    /* Reset Service state when IWLAN is enabled as polling in airplane mode
     * causes state to go to OUT_OF_SERVICE state instead of STATE_OFF
     */
    protected void resetServiceStateInIwlanMode() {
        if (mCi.getRadioState() == CommandsInterface.RadioState.RADIO_OFF) {
            boolean resetIwlanRatVal = false;
            log("set service state as POWER_OFF");
            if (ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN
                        == mNewSS.getRilDataRadioTechnology()) {
                log("pollStateDone: mNewSS = " + mNewSS);
                log("pollStateDone: reset iwlan RAT value");
                resetIwlanRatVal = true;
            }
            // operator info should be kept in SS
            String operator = mNewSS.getOperatorAlphaLong();
            mNewSS.setStateOff();
            if (resetIwlanRatVal) {
                mNewSS.setRilDataRadioTechnology(ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN);
                mNewSS.setDataRegState(ServiceState.STATE_IN_SERVICE);
                mNewSS.setOperatorAlphaLong(operator);
                log("pollStateDone: mNewSS = " + mNewSS);
            }
        }
    }

    /**
     * Check if device is non-roaming and always on home network.
     *
     * @param b carrier config bundle obtained from CarrierConfigManager
     * @return true if network is always on home network, false otherwise
     * @see CarrierConfigManager
     */
    protected final boolean alwaysOnHomeNetwork(BaseBundle b) {
        return b.getBoolean(CarrierConfigManager.KEY_FORCE_HOME_NETWORK_BOOL);
    }

    /**
     * Check if the network identifier has membership in the set of
     * network identifiers stored in the carrier config bundle.
     *
     * @param b carrier config bundle obtained from CarrierConfigManager
     * @param network The network identifier to check network existence in bundle
     * @param key The key to index into the bundle presenting a string array of
     *            networks to check membership
     * @return true if network has membership in bundle networks, false otherwise
     * @see CarrierConfigManager
     */
    private boolean isInNetwork(BaseBundle b, String network, String key) {
        String[] networks = b.getStringArray(key);

        if (networks != null && Arrays.asList(networks).contains(network)) {
            return true;
        }
        return false;
    }

    protected final boolean isRoamingInGsmNetwork(BaseBundle b, String network) {
        return isInNetwork(b, network, CarrierConfigManager.KEY_GSM_ROAMING_NETWORKS_STRING_ARRAY);
    }

    protected final boolean isNonRoamingInGsmNetwork(BaseBundle b, String network) {
        return isInNetwork(b, network, CarrierConfigManager.KEY_GSM_NONROAMING_NETWORKS_STRING_ARRAY);
    }

    protected final boolean isRoamingInCdmaNetwork(BaseBundle b, String network) {
        return isInNetwork(b, network, CarrierConfigManager.KEY_CDMA_ROAMING_NETWORKS_STRING_ARRAY);
    }

    protected final boolean isNonRoamingInCdmaNetwork(BaseBundle b, String network) {
        return isInNetwork(b, network, CarrierConfigManager.KEY_CDMA_NONROAMING_NETWORKS_STRING_ARRAY);
    }

    /** Check if the device is shutting down. */
    public boolean isDeviceShuttingDown() {
        return mDeviceShuttingDown;
    }

    /**
     * Consider dataRegState if voiceRegState is OOS to determine SPN to be displayed
     */
    protected int getCombinedRegState() {
        int regState = mSS.getVoiceRegState();
        int dataRegState = mSS.getDataRegState();
        if ((regState == ServiceState.STATE_OUT_OF_SERVICE
                || regState == ServiceState.STATE_POWER_OFF)
                && (dataRegState == ServiceState.STATE_IN_SERVICE)) {
            log("getCombinedRegState: return STATE_IN_SERVICE as Data is in service");
            regState = dataRegState;
        }
        return regState;
    }

    /**
     * Gets the carrier configuration values for a particular subscription.
     *
     * @return A {@link PersistableBundle} containing the config for the given subId,
     *         or default values for an invalid subId.
     */
    private PersistableBundle getCarrierConfig() {
        CarrierConfigManager configManager = (CarrierConfigManager) mPhone.getContext()
                .getSystemService(Context.CARRIER_CONFIG_SERVICE);
        if (configManager != null) {
            // If an invalid subId is used, this bundle will contain default values.
            PersistableBundle config = configManager.getConfigForSubId(mPhone.getSubId());
            if (config != null) {
                return config;
            }
        }
        // Return static default defined in CarrierConfigManager.
        return CarrierConfigManager.getDefaultConfig();
    }

    public LocaleTracker getLocaleTracker() {
        return mLocaleTracker;
    }
}
