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

package com.android.internal.telephony.dataconnection;

import static android.Manifest.permission.READ_PHONE_STATE;

import android.annotation.NonNull;
import android.app.AlarmManager;
import android.app.PendingIntent;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.LinkProperties;
import android.net.NetworkCapabilities;
import android.net.NetworkConfig;
import android.net.NetworkRequest;
import android.net.NetworkUtils;
import android.net.ProxyInfo;
import android.net.TrafficStats;
import android.net.Uri;
import android.os.AsyncResult;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.PersistableBundle;
import android.os.RegistrantList;
import android.os.ServiceManager;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.provider.Settings.SettingNotFoundException;
import android.provider.Telephony;
import android.telephony.AccessNetworkConstants.TransportType;
import android.telephony.CarrierConfigManager;
import android.telephony.CellLocation;
import android.telephony.PcoData;
import android.telephony.Rlog;
import android.telephony.ServiceState;
import android.telephony.SubscriptionManager;
import android.telephony.SubscriptionManager.OnSubscriptionsChangedListener;
import android.telephony.TelephonyManager;
import android.telephony.cdma.CdmaCellLocation;
import android.telephony.data.DataProfile;
import android.telephony.gsm.GsmCellLocation;
import android.text.TextUtils;
import android.util.EventLog;
import android.util.LocalLog;
import android.util.Pair;
import android.util.SparseArray;
import android.view.WindowManager;

import com.android.internal.R;
import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.CarrierActionAgent;
import com.android.internal.telephony.DctConstants;
import com.android.internal.telephony.EventLogTags;
import com.android.internal.telephony.GsmCdmaPhone;
import com.android.internal.telephony.ITelephony;
import com.android.internal.telephony.Phone;
import com.android.internal.telephony.PhoneConstants;
import com.android.internal.telephony.PhoneFactory;
import com.android.internal.telephony.RILConstants;
import com.android.internal.telephony.SettingsObserver;
import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.dataconnection.DataConnectionReasons.DataAllowedReasonType;
import com.android.internal.telephony.dataconnection.DataConnectionReasons.DataDisallowedReasonType;
import com.android.internal.telephony.metrics.TelephonyMetrics;
import com.android.internal.telephony.uicc.IccRecords;
import com.android.internal.telephony.uicc.UiccController;
import com.android.internal.util.ArrayUtils;
import com.android.internal.util.AsyncChannel;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.Map.Entry;
import java.util.PriorityQueue;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.atomic.AtomicReference;
/**
 * {@hide}
 */
public class DcTracker extends Handler {
    private static final String LOG_TAG = "DCT";
    private static final boolean DBG = true;
    private static final boolean VDBG = false; // STOPSHIP if true
    private static final boolean VDBG_STALL = false; // STOPSHIP if true
    private static final boolean RADIO_TESTS = false;

    public AtomicBoolean isCleanupRequired = new AtomicBoolean(false);

    private final AlarmManager mAlarmManager;

    /* Currently requested APN type (TODO: This should probably be a parameter not a member) */
    private String mRequestedApnType = PhoneConstants.APN_TYPE_DEFAULT;

    // All data enabling/disabling related settings
    private final DataEnabledSettings mDataEnabledSettings;


    /**
     * After detecting a potential connection problem, this is the max number
     * of subsequent polls before attempting recovery.
     */
    // 1 sec. default polling interval when screen is on.
    private static final int POLL_NETSTAT_MILLIS = 1000;
    // 10 min. default polling interval when screen is off.
    private static final int POLL_NETSTAT_SCREEN_OFF_MILLIS = 1000*60*10;
    // Default sent packets without ack which triggers initial recovery steps
    private static final int NUMBER_SENT_PACKETS_OF_HANG = 10;

    // Default for the data stall alarm while non-aggressive stall detection
    private static final int DATA_STALL_ALARM_NON_AGGRESSIVE_DELAY_IN_MS_DEFAULT = 1000 * 60 * 6;
    // Default for the data stall alarm for aggressive stall detection
    private static final int DATA_STALL_ALARM_AGGRESSIVE_DELAY_IN_MS_DEFAULT = 1000 * 60;
    // Tag for tracking stale alarms
    private static final String DATA_STALL_ALARM_TAG_EXTRA = "data.stall.alram.tag";

    private static final boolean DATA_STALL_SUSPECTED = true;
    private static final boolean DATA_STALL_NOT_SUSPECTED = false;

    private static final String INTENT_RECONNECT_ALARM =
            "com.android.internal.telephony.data-reconnect";
    private static final String INTENT_RECONNECT_ALARM_EXTRA_TYPE = "reconnect_alarm_extra_type";
    private static final String INTENT_RECONNECT_ALARM_EXTRA_REASON =
            "reconnect_alarm_extra_reason";

    private static final String INTENT_DATA_STALL_ALARM =
            "com.android.internal.telephony.data-stall";

    private DcTesterFailBringUpAll mDcTesterFailBringUpAll;
    private DcController mDcc;

    /** kept in sync with mApnContexts
     * Higher numbers are higher priority and sorted so highest priority is first */
    private final PriorityQueue<ApnContext>mPrioritySortedApnContexts =
            new PriorityQueue<ApnContext>(5,
            new Comparator<ApnContext>() {
                public int compare(ApnContext c1, ApnContext c2) {
                    return c2.priority - c1.priority;
                }
            } );

    /** allApns holds all apns */
    private ArrayList<ApnSetting> mAllApnSettings = null;

    /** preferred apn */
    private ApnSetting mPreferredApn = null;

    /** Is packet service restricted by network */
    private boolean mIsPsRestricted = false;

    /** emergency apn Setting*/
    private ApnSetting mEmergencyApn = null;

    /* Once disposed dont handle any messages */
    private boolean mIsDisposed = false;

    private ContentResolver mResolver;

    /* Set to true with CMD_ENABLE_MOBILE_PROVISIONING */
    private boolean mIsProvisioning = false;

    /* The Url passed as object parameter in CMD_ENABLE_MOBILE_PROVISIONING */
    private String mProvisioningUrl = null;

    /* Intent for the provisioning apn alarm */
    private static final String INTENT_PROVISIONING_APN_ALARM =
            "com.android.internal.telephony.provisioning_apn_alarm";

    /* Tag for tracking stale alarms */
    private static final String PROVISIONING_APN_ALARM_TAG_EXTRA = "provisioning.apn.alarm.tag";

    /* Debug property for overriding the PROVISIONING_APN_ALARM_DELAY_IN_MS */
    private static final String DEBUG_PROV_APN_ALARM = "persist.debug.prov_apn_alarm";

    /* Default for the provisioning apn alarm timeout */
    private static final int PROVISIONING_APN_ALARM_DELAY_IN_MS_DEFAULT = 1000 * 60 * 15;

    /* The provision apn alarm intent used to disable the provisioning apn */
    private PendingIntent mProvisioningApnAlarmIntent = null;

    /* Used to track stale provisioning apn alarms */
    private int mProvisioningApnAlarmTag = (int) SystemClock.elapsedRealtime();

    private AsyncChannel mReplyAc = new AsyncChannel();

    private final LocalLog mDataRoamingLeakageLog = new LocalLog(50);

    private final BroadcastReceiver mIntentReceiver = new BroadcastReceiver () {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();

            if (action.equals(Intent.ACTION_SCREEN_ON)) {
                // TODO: Evaluate hooking this up with DeviceStateMonitor
                if (DBG) log("screen on");
                mIsScreenOn = true;
                stopNetStatPoll();
                startNetStatPoll();
                restartDataStallAlarm();
            } else if (action.equals(Intent.ACTION_SCREEN_OFF)) {
                if (DBG) log("screen off");
                mIsScreenOn = false;
                stopNetStatPoll();
                startNetStatPoll();
                restartDataStallAlarm();
            } else if (action.startsWith(INTENT_RECONNECT_ALARM)) {
                if (DBG) log("Reconnect alarm. Previous state was " + mState);
                onActionIntentReconnectAlarm(intent);
            } else if (action.equals(INTENT_DATA_STALL_ALARM)) {
                if (DBG) log("Data stall alarm");
                onActionIntentDataStallAlarm(intent);
            } else if (action.equals(INTENT_PROVISIONING_APN_ALARM)) {
                if (DBG) log("Provisioning apn alarm");
                onActionIntentProvisioningApnAlarm(intent);
            } else if (action.equals(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED)) {
                if (mIccRecords.get() != null && mIccRecords.get().getRecordsLoaded()) {
                    setDefaultDataRoamingEnabled();
                }
            } else {
                if (DBG) log("onReceive: Unknown action=" + action);
            }
        }
    };

    private final Runnable mPollNetStat = new Runnable() {
        @Override
        public void run() {
            updateDataActivity();

            if (mIsScreenOn) {
                mNetStatPollPeriod = Settings.Global.getInt(mResolver,
                        Settings.Global.PDP_WATCHDOG_POLL_INTERVAL_MS, POLL_NETSTAT_MILLIS);
            } else {
                mNetStatPollPeriod = Settings.Global.getInt(mResolver,
                        Settings.Global.PDP_WATCHDOG_LONG_POLL_INTERVAL_MS,
                        POLL_NETSTAT_SCREEN_OFF_MILLIS);
            }

            if (mNetStatPollEnabled) {
                mDataConnectionTracker.postDelayed(this, mNetStatPollPeriod);
            }
        }
    };

    private SubscriptionManager mSubscriptionManager;
    private final OnSubscriptionsChangedListener mOnSubscriptionsChangedListener =
            new OnSubscriptionsChangedListener() {
                public final AtomicInteger mPreviousSubId =
                        new AtomicInteger(SubscriptionManager.INVALID_SUBSCRIPTION_ID);

                /**
                 * Callback invoked when there is any change to any SubscriptionInfo. Typically
                 * this method invokes {@link SubscriptionManager#getActiveSubscriptionInfoList}
                 */
                @Override
                public void onSubscriptionsChanged() {
                    if (DBG) log("SubscriptionListener.onSubscriptionInfoChanged");
                    // Set the network type, in case the radio does not restore it.
                    int subId = mPhone.getSubId();
                    if (SubscriptionManager.isValidSubscriptionId(subId)) {
                        registerSettingsObserver();
                    }
                    if (mPreviousSubId.getAndSet(subId) != subId &&
                            SubscriptionManager.isValidSubscriptionId(subId)) {
                        onRecordsLoadedOrSubIdChanged();
                    }
                }
            };

    private final SettingsObserver mSettingsObserver;

    private void registerSettingsObserver() {
        mSettingsObserver.unobserve();
        String simSuffix = "";
        if (TelephonyManager.getDefault().getSimCount() > 1) {
            simSuffix = Integer.toString(mPhone.getSubId());
        }

        mSettingsObserver.observe(
                Settings.Global.getUriFor(Settings.Global.DATA_ROAMING + simSuffix),
                DctConstants.EVENT_ROAMING_SETTING_CHANGE);
        mSettingsObserver.observe(
                Settings.Global.getUriFor(Settings.Global.DEVICE_PROVISIONED),
                DctConstants.EVENT_DEVICE_PROVISIONED_CHANGE);
        mSettingsObserver.observe(
                Settings.Global.getUriFor(Settings.Global.DEVICE_PROVISIONING_MOBILE_DATA_ENABLED),
                DctConstants.EVENT_DEVICE_PROVISIONED_CHANGE);
    }

    /**
     * Maintain the sum of transmit and receive packets.
     *
     * The packet counts are initialized and reset to -1 and
     * remain -1 until they can be updated.
     */
    public static class TxRxSum {
        public long txPkts;
        public long rxPkts;

        public TxRxSum() {
            reset();
        }

        public TxRxSum(long txPkts, long rxPkts) {
            this.txPkts = txPkts;
            this.rxPkts = rxPkts;
        }

        public TxRxSum(TxRxSum sum) {
            txPkts = sum.txPkts;
            rxPkts = sum.rxPkts;
        }

        public void reset() {
            txPkts = -1;
            rxPkts = -1;
        }

        @Override
        public String toString() {
            return "{txSum=" + txPkts + " rxSum=" + rxPkts + "}";
        }

        public void updateTxRxSum() {
            this.txPkts = TrafficStats.getMobileTcpTxPackets();
            this.rxPkts = TrafficStats.getMobileTcpRxPackets();
        }
    }

    private void onActionIntentReconnectAlarm(Intent intent) {
        Message msg = obtainMessage(DctConstants.EVENT_DATA_RECONNECT);
        msg.setData(intent.getExtras());
        sendMessage(msg);
    }

    private void onDataReconnect(Bundle bundle) {
        String reason = bundle.getString(INTENT_RECONNECT_ALARM_EXTRA_REASON);
        String apnType = bundle.getString(INTENT_RECONNECT_ALARM_EXTRA_TYPE);

        int phoneSubId = mPhone.getSubId();
        int currSubId = bundle.getInt(PhoneConstants.SUBSCRIPTION_KEY,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        log("onDataReconnect: currSubId = " + currSubId + " phoneSubId=" + phoneSubId);

        // Stop reconnect if not current subId is not correct.
        // FIXME STOPSHIP - phoneSubId is coming up as -1 way after boot and failing this?
        if (!SubscriptionManager.isValidSubscriptionId(currSubId) || (currSubId != phoneSubId)) {
            log("receive ReconnectAlarm but subId incorrect, ignore");
            return;
        }

        ApnContext apnContext = mApnContexts.get(apnType);

        if (DBG) {
            log("onDataReconnect: mState=" + mState + " reason=" + reason + " apnType=" + apnType
                    + " apnContext=" + apnContext + " mDataConnectionAsyncChannels="
                    + mDataConnectionAcHashMap);
        }

        if ((apnContext != null) && (apnContext.isEnabled())) {
            apnContext.setReason(reason);
            DctConstants.State apnContextState = apnContext.getState();
            if (DBG) {
                log("onDataReconnect: apnContext state=" + apnContextState);
            }
            if ((apnContextState == DctConstants.State.FAILED)
                    || (apnContextState == DctConstants.State.IDLE)) {
                if (DBG) {
                    log("onDataReconnect: state is FAILED|IDLE, disassociate");
                }
                DcAsyncChannel dcac = apnContext.getDcAc();
                if (dcac != null) {
                    if (DBG) {
                        log("onDataReconnect: tearDown apnContext=" + apnContext);
                    }
                    dcac.tearDown(apnContext, "", null);
                }
                apnContext.setDataConnectionAc(null);
                apnContext.setState(DctConstants.State.IDLE);
            } else {
                if (DBG) log("onDataReconnect: keep associated");
            }
            // TODO: IF already associated should we send the EVENT_TRY_SETUP_DATA???
            sendMessage(obtainMessage(DctConstants.EVENT_TRY_SETUP_DATA, apnContext));

            apnContext.setReconnectIntent(null);
        }
    }

    private void onActionIntentDataStallAlarm(Intent intent) {
        if (VDBG_STALL) log("onActionIntentDataStallAlarm: action=" + intent.getAction());
        Message msg = obtainMessage(DctConstants.EVENT_DATA_STALL_ALARM,
                intent.getAction());
        msg.arg1 = intent.getIntExtra(DATA_STALL_ALARM_TAG_EXTRA, 0);
        sendMessage(msg);
    }

    private final ConnectivityManager mCm;

    /**
     * List of messages that are waiting to be posted, when data call disconnect
     * is complete
     */
    private ArrayList<Message> mDisconnectAllCompleteMsgList = new ArrayList<Message>();

    private RegistrantList mAllDataDisconnectedRegistrants = new RegistrantList();

    // member variables
    private final Phone mPhone;
    private final UiccController mUiccController;
    private final AtomicReference<IccRecords> mIccRecords = new AtomicReference<IccRecords>();
    private DctConstants.Activity mActivity = DctConstants.Activity.NONE;
    private DctConstants.State mState = DctConstants.State.IDLE;
    private final Handler mDataConnectionTracker;

    private long mTxPkts;
    private long mRxPkts;
    private int mNetStatPollPeriod;
    private boolean mNetStatPollEnabled = false;

    private TxRxSum mDataStallTxRxSum = new TxRxSum(0, 0);
    // Used to track stale data stall alarms.
    private int mDataStallAlarmTag = (int) SystemClock.elapsedRealtime();
    // The current data stall alarm intent
    private PendingIntent mDataStallAlarmIntent = null;
    // Number of packets sent since the last received packet
    private long mSentSinceLastRecv;
    // Controls when a simple recovery attempt it to be tried
    private int mNoRecvPollCount = 0;
    // Reference counter for enabling fail fast
    private static int sEnableFailFastRefCounter = 0;
    // True if data stall detection is enabled
    private volatile boolean mDataStallDetectionEnabled = true;

    private volatile boolean mFailFast = false;

    // True when in voice call
    private boolean mInVoiceCall = false;

    /** Intent sent when the reconnect alarm fires. */
    private PendingIntent mReconnectIntent = null;

    // When false we will not auto attach and manually attaching is required.
    private boolean mAutoAttachOnCreationConfig = false;
    private AtomicBoolean mAutoAttachOnCreation = new AtomicBoolean(false);

    // State of screen
    // (TODO: Reconsider tying directly to screen, maybe this is
    //        really a lower power mode")
    private boolean mIsScreenOn = true;

    // Indicates if we found mvno-specific APNs in the full APN list.
    // used to determine if we can accept mno-specific APN for tethering.
    private boolean mMvnoMatched = false;

    /** Allows the generation of unique Id's for DataConnection objects */
    private AtomicInteger mUniqueIdGenerator = new AtomicInteger(0);

    /** The data connections. */
    private HashMap<Integer, DataConnection> mDataConnections =
            new HashMap<Integer, DataConnection>();

    /** The data connection async channels */
    private HashMap<Integer, DcAsyncChannel> mDataConnectionAcHashMap =
            new HashMap<Integer, DcAsyncChannel>();

    /** Convert an ApnType string to Id (TODO: Use "enumeration" instead of String for ApnType) */
    private HashMap<String, Integer> mApnToDataConnectionId = new HashMap<String, Integer>();

    /** Phone.APN_TYPE_* ===> ApnContext */
    private final ConcurrentHashMap<String, ApnContext> mApnContexts =
            new ConcurrentHashMap<String, ApnContext>();

    private final SparseArray<ApnContext> mApnContextsById = new SparseArray<ApnContext>();

    private int mDisconnectPendingCount = 0;

    /** Indicate if metered APNs are disabled.
     *  set to block all the metered APNs from continuously sending requests, which causes
     *  undesired network load */
    private boolean mMeteredApnDisabled = false;

    /**
     * int to remember whether has setDataProfiles and with roaming or not.
     * 0: default, has never set data profile
     * 1: has set data profile with home protocol
     * 2: has set data profile with roaming protocol
     * This is not needed once RIL command is updated to support both home and roaming protocol.
     */
    private int mSetDataProfileStatus = 0;

    /**
     * Handles changes to the APN db.
     */
    private class ApnChangeObserver extends ContentObserver {
        public ApnChangeObserver () {
            super(mDataConnectionTracker);
        }

        @Override
        public void onChange(boolean selfChange) {
            sendMessage(obtainMessage(DctConstants.EVENT_APN_CHANGED));
        }
    }

    //***** Instance Variables

    private boolean mReregisterOnReconnectFailure = false;


    //***** Constants

    // Used by puppetmaster/*/radio_stress.py
    private static final String PUPPET_MASTER_RADIO_STRESS_TEST = "gsm.defaultpdpcontext.active";

    private static final int POLL_PDP_MILLIS = 5 * 1000;

    private static final int PROVISIONING_SPINNER_TIMEOUT_MILLIS = 120 * 1000;

    static final Uri PREFERAPN_NO_UPDATE_URI_USING_SUBID =
                        Uri.parse("content://telephony/carriers/preferapn_no_update/subId/");
    static final String APN_ID = "apn_id";

    private boolean mCanSetPreferApn = false;

    private AtomicBoolean mAttached = new AtomicBoolean(false);

    /** Watches for changes to the APN db. */
    private ApnChangeObserver mApnObserver;

    private final String mProvisionActionName;
    private BroadcastReceiver mProvisionBroadcastReceiver;
    private ProgressDialog mProvisioningSpinner;

    private final DataServiceManager mDataServiceManager;

    private final int mTransportType;

    //***** Constructor
    public DcTracker(Phone phone, int transportType) {
        super();
        mPhone = phone;
        if (DBG) log("DCT.constructor");
        mTransportType = transportType;
        mDataServiceManager = new DataServiceManager(phone, transportType);

        mResolver = mPhone.getContext().getContentResolver();
        mUiccController = UiccController.getInstance();
        mUiccController.registerForIccChanged(this, DctConstants.EVENT_ICC_CHANGED, null);
        mAlarmManager =
                (AlarmManager) mPhone.getContext().getSystemService(Context.ALARM_SERVICE);
        mCm = (ConnectivityManager) mPhone.getContext().getSystemService(
                Context.CONNECTIVITY_SERVICE);


        IntentFilter filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        filter.addAction(INTENT_DATA_STALL_ALARM);
        filter.addAction(INTENT_PROVISIONING_APN_ALARM);
        filter.addAction(CarrierConfigManager.ACTION_CARRIER_CONFIG_CHANGED);

        mDataEnabledSettings = new DataEnabledSettings(phone);

        mPhone.getContext().registerReceiver(mIntentReceiver, filter, null, mPhone);

        SharedPreferences sp = PreferenceManager.getDefaultSharedPreferences(mPhone.getContext());
        mAutoAttachOnCreation.set(sp.getBoolean(Phone.DATA_DISABLED_ON_BOOT_KEY, false));

        mSubscriptionManager = SubscriptionManager.from(mPhone.getContext());
        mSubscriptionManager.addOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);

        HandlerThread dcHandlerThread = new HandlerThread("DcHandlerThread");
        dcHandlerThread.start();
        Handler dcHandler = new Handler(dcHandlerThread.getLooper());
        mDcc = DcController.makeDcc(mPhone, this, mDataServiceManager, dcHandler);
        mDcTesterFailBringUpAll = new DcTesterFailBringUpAll(mPhone, dcHandler);

        mDataConnectionTracker = this;
        registerForAllEvents();
        update();
        mApnObserver = new ApnChangeObserver();
        phone.getContext().getContentResolver().registerContentObserver(
                Telephony.Carriers.CONTENT_URI, true, mApnObserver);

        initApnContexts();

        for (ApnContext apnContext : mApnContexts.values()) {
            // Register the reconnect and restart actions.
            filter = new IntentFilter();
            filter.addAction(INTENT_RECONNECT_ALARM + '.' + apnContext.getApnType());
            mPhone.getContext().registerReceiver(mIntentReceiver, filter, null, mPhone);
        }

        // Add Emergency APN to APN setting list by default to support EPDN in sim absent cases
        initEmergencyApnSetting();
        addEmergencyApnSetting();

        mProvisionActionName = "com.android.internal.telephony.PROVISION" + phone.getPhoneId();

        mSettingsObserver = new SettingsObserver(mPhone.getContext(), this);
        registerSettingsObserver();
    }

    @VisibleForTesting
    public DcTracker() {
        mAlarmManager = null;
        mCm = null;
        mPhone = null;
        mUiccController = null;
        mDataConnectionTracker = null;
        mProvisionActionName = null;
        mSettingsObserver = new SettingsObserver(null, this);
        mDataEnabledSettings = null;
        mTransportType = 0;
        mDataServiceManager = null;
    }

    public void registerServiceStateTrackerEvents() {
        mPhone.getServiceStateTracker().registerForDataConnectionAttached(this,
                DctConstants.EVENT_DATA_CONNECTION_ATTACHED, null);
        mPhone.getServiceStateTracker().registerForDataConnectionDetached(this,
                DctConstants.EVENT_DATA_CONNECTION_DETACHED, null);
        mPhone.getServiceStateTracker().registerForDataRoamingOn(this,
                DctConstants.EVENT_ROAMING_ON, null);
        mPhone.getServiceStateTracker().registerForDataRoamingOff(this,
                DctConstants.EVENT_ROAMING_OFF, null, true);
        mPhone.getServiceStateTracker().registerForPsRestrictedEnabled(this,
                DctConstants.EVENT_PS_RESTRICT_ENABLED, null);
        mPhone.getServiceStateTracker().registerForPsRestrictedDisabled(this,
                DctConstants.EVENT_PS_RESTRICT_DISABLED, null);
        mPhone.getServiceStateTracker().registerForDataRegStateOrRatChanged(this,
                DctConstants.EVENT_DATA_RAT_CHANGED, null);
    }

    public void unregisterServiceStateTrackerEvents() {
        mPhone.getServiceStateTracker().unregisterForDataConnectionAttached(this);
        mPhone.getServiceStateTracker().unregisterForDataConnectionDetached(this);
        mPhone.getServiceStateTracker().unregisterForDataRoamingOn(this);
        mPhone.getServiceStateTracker().unregisterForDataRoamingOff(this);
        mPhone.getServiceStateTracker().unregisterForPsRestrictedEnabled(this);
        mPhone.getServiceStateTracker().unregisterForPsRestrictedDisabled(this);
        mPhone.getServiceStateTracker().unregisterForDataRegStateOrRatChanged(this);
    }

    private void registerForAllEvents() {
        if (mTransportType == TransportType.WWAN) {
            mPhone.mCi.registerForAvailable(this, DctConstants.EVENT_RADIO_AVAILABLE, null);
            mPhone.mCi.registerForOffOrNotAvailable(this,
                    DctConstants.EVENT_RADIO_OFF_OR_NOT_AVAILABLE, null);
            mPhone.mCi.registerForPcoData(this, DctConstants.EVENT_PCO_DATA_RECEIVED, null);
        }

        // Note, this is fragile - the Phone is now presenting a merged picture
        // of PS (volte) & CS and by diving into its internals you're just seeing
        // the CS data.  This works well for the purposes this is currently used for
        // but that may not always be the case.  Should probably be redesigned to
        // accurately reflect what we're really interested in (registerForCSVoiceCallEnded).
        mPhone.getCallTracker().registerForVoiceCallEnded(this,
                DctConstants.EVENT_VOICE_CALL_ENDED, null);
        mPhone.getCallTracker().registerForVoiceCallStarted(this,
                DctConstants.EVENT_VOICE_CALL_STARTED, null);
        registerServiceStateTrackerEvents();
        mPhone.mCi.registerForPcoData(this, DctConstants.EVENT_PCO_DATA_RECEIVED, null);
        mPhone.getCarrierActionAgent().registerForCarrierAction(
                CarrierActionAgent.CARRIER_ACTION_SET_METERED_APNS_ENABLED, this,
                DctConstants.EVENT_SET_CARRIER_DATA_ENABLED, null, false);
        mDataServiceManager.registerForServiceBindingChanged(this,
                DctConstants.EVENT_DATA_SERVICE_BINDING_CHANGED, null);
    }

    public void dispose() {
        if (DBG) log("DCT.dispose");

        if (mProvisionBroadcastReceiver != null) {
            mPhone.getContext().unregisterReceiver(mProvisionBroadcastReceiver);
            mProvisionBroadcastReceiver = null;
        }
        if (mProvisioningSpinner != null) {
            mProvisioningSpinner.dismiss();
            mProvisioningSpinner = null;
        }

        cleanUpAllConnections(true, null);

        for (DcAsyncChannel dcac : mDataConnectionAcHashMap.values()) {
            dcac.disconnect();
        }
        mDataConnectionAcHashMap.clear();
        mIsDisposed = true;
        mPhone.getContext().unregisterReceiver(mIntentReceiver);
        mUiccController.unregisterForIccChanged(this);
        mSettingsObserver.unobserve();

        mSubscriptionManager
                .removeOnSubscriptionsChangedListener(mOnSubscriptionsChangedListener);
        mDcc.dispose();
        mDcTesterFailBringUpAll.dispose();

        mPhone.getContext().getContentResolver().unregisterContentObserver(mApnObserver);
        mApnContexts.clear();
        mApnContextsById.clear();
        mPrioritySortedApnContexts.clear();
        unregisterForAllEvents();

        destroyDataConnections();
    }

    private void unregisterForAllEvents() {
         //Unregister for all events
        if (mTransportType == TransportType.WWAN) {
            mPhone.mCi.unregisterForAvailable(this);
            mPhone.mCi.unregisterForOffOrNotAvailable(this);
            mPhone.mCi.unregisterForPcoData(this);
        }

        IccRecords r = mIccRecords.get();
        if (r != null) {
            r.unregisterForRecordsLoaded(this);
            mIccRecords.set(null);
        }
        mPhone.getCallTracker().unregisterForVoiceCallEnded(this);
        mPhone.getCallTracker().unregisterForVoiceCallStarted(this);
        unregisterServiceStateTrackerEvents();
        mPhone.mCi.unregisterForPcoData(this);
        mPhone.getCarrierActionAgent().unregisterForCarrierAction(this,
                CarrierActionAgent.CARRIER_ACTION_SET_METERED_APNS_ENABLED);
        mDataServiceManager.unregisterForServiceBindingChanged(this);
    }

    /**
     * Modify {@link android.provider.Settings.Global#MOBILE_DATA} value.
     */
    public void setUserDataEnabled(boolean enable) {
        Message msg = obtainMessage(DctConstants.CMD_SET_USER_DATA_ENABLE);
        msg.arg1 = enable ? 1 : 0;
        if (DBG) log("setDataEnabled: sendMessage: enable=" + enable);
        sendMessage(msg);
    }

    private void onSetUserDataEnabled(boolean enabled) {
        if (mDataEnabledSettings.isUserDataEnabled() != enabled) {
            mDataEnabledSettings.setUserDataEnabled(enabled);
            if (!getDataRoamingEnabled() && mPhone.getServiceState().getDataRoaming()) {
                if (enabled) {
                    notifyOffApnsOfAvailability(Phone.REASON_ROAMING_ON);
                } else {
                    notifyOffApnsOfAvailability(Phone.REASON_DATA_DISABLED);
                }
            }

            mPhone.notifyUserMobileDataStateChanged(enabled);

            // TODO: We should register for DataEnabledSetting's data enabled/disabled event and
            // handle the rest from there.
            if (enabled) {
                reevaluateDataConnections();
                onTrySetupData(Phone.REASON_DATA_ENABLED);
            } else {
                onCleanUpAllConnections(Phone.REASON_DATA_SPECIFIC_DISABLED);
            }
        }
    }

    /**
     * Reevaluate existing data connections when conditions change.
     *
     * For example, handle reverting restricted networks back to unrestricted. If we're changing
     * user data to enabled and this makes data truly enabled (not disabled by other factors) we
     * need to tear down any metered apn type that was enabled anyway by a privileged request.
     * This allows us to reconnect to it in an unrestricted way.
     *
     * Or when we brought up a unmetered data connection while data is off, we only limit this
     * data connection for unmetered use only. When data is turned back on, we need to tear that
     * down so a full capable data connection can be re-established.
     */
    private void reevaluateDataConnections() {
        if (mDataEnabledSettings.isDataEnabled()) {
            for (ApnContext apnContext : mApnContexts.values()) {
                if (apnContext.isConnectedOrConnecting()) {
                    final DcAsyncChannel dcac = apnContext.getDcAc();
                    if (dcac != null) {
                        final NetworkCapabilities netCaps = dcac.getNetworkCapabilitiesSync();
                        if (netCaps != null && !netCaps.hasCapability(NetworkCapabilities
                                .NET_CAPABILITY_NOT_RESTRICTED) && !netCaps.hasCapability(
                                NetworkCapabilities.NET_CAPABILITY_NOT_METERED)) {
                            if (DBG) {
                                log("Tearing down restricted metered net:" + apnContext);
                            }
                            // Tearing down the restricted metered data call when
                            // conditions change. This will allow reestablishing a new unrestricted
                            // data connection.
                            apnContext.setReason(Phone.REASON_DATA_ENABLED);
                            cleanUpConnection(true, apnContext);
                        } else if (apnContext.getApnSetting().isMetered(mPhone)
                                && (netCaps != null && netCaps.hasCapability(
                                        NetworkCapabilities.NET_CAPABILITY_NOT_METERED))) {
                            if (DBG) {
                                log("Tearing down unmetered net:" + apnContext);
                            }
                            // The APN settings is metered, but the data was still marked as
                            // unmetered data, must be the unmetered data connection brought up when
                            // data is off. We need to tear that down when data is enabled again.
                            // This will allow reestablishing a new full capability data connection.
                            apnContext.setReason(Phone.REASON_DATA_ENABLED);
                            cleanUpConnection(true, apnContext);
                        }
                    }
                }
            }
        }
    }

    private void onDeviceProvisionedChange() {
        if (isDataEnabled()) {
            reevaluateDataConnections();
            onTrySetupData(Phone.REASON_DATA_ENABLED);
        } else {
            onCleanUpAllConnections(Phone.REASON_DATA_SPECIFIC_DISABLED);
        }
    }


    public long getSubId() {
        return mPhone.getSubId();
    }

    public DctConstants.Activity getActivity() {
        return mActivity;
    }

    private void setActivity(DctConstants.Activity activity) {
        log("setActivity = " + activity);
        mActivity = activity;
        mPhone.notifyDataActivity();
    }

    public void requestNetwork(NetworkRequest networkRequest, LocalLog log) {
        final int apnId = ApnContext.apnIdForNetworkRequest(networkRequest);
        final ApnContext apnContext = mApnContextsById.get(apnId);
        log.log("DcTracker.requestNetwork for " + networkRequest + " found " + apnContext);
        if (apnContext != null) apnContext.requestNetwork(networkRequest, log);
    }

    public void releaseNetwork(NetworkRequest networkRequest, LocalLog log) {
        final int apnId = ApnContext.apnIdForNetworkRequest(networkRequest);
        final ApnContext apnContext = mApnContextsById.get(apnId);
        log.log("DcTracker.releaseNetwork for " + networkRequest + " found " + apnContext);
        if (apnContext != null) apnContext.releaseNetwork(networkRequest, log);
    }

    public boolean isApnSupported(String name) {
        if (name == null) {
            loge("isApnSupported: name=null");
            return false;
        }
        ApnContext apnContext = mApnContexts.get(name);
        if (apnContext == null) {
            loge("Request for unsupported mobile name: " + name);
            return false;
        }
        return true;
    }

    public int getApnPriority(String name) {
        ApnContext apnContext = mApnContexts.get(name);
        if (apnContext == null) {
            loge("Request for unsupported mobile name: " + name);
        }
        return apnContext.priority;
    }

    // Turn telephony radio on or off.
    private void setRadio(boolean on) {
        final ITelephony phone = ITelephony.Stub.asInterface(ServiceManager.checkService("phone"));
        try {
            phone.setRadio(on);
        } catch (Exception e) {
            // Ignore.
        }
    }

    // Class to handle Intent dispatched with user selects the "Sign-in to network"
    // notification.
    private class ProvisionNotificationBroadcastReceiver extends BroadcastReceiver {
        private final String mNetworkOperator;
        // Mobile provisioning URL.  Valid while provisioning notification is up.
        // Set prior to notification being posted as URL contains ICCID which
        // disappears when radio is off (which is the case when notification is up).
        private final String mProvisionUrl;

        public ProvisionNotificationBroadcastReceiver(String provisionUrl, String networkOperator) {
            mNetworkOperator = networkOperator;
            mProvisionUrl = provisionUrl;
        }

        private void setEnableFailFastMobileData(int enabled) {
            sendMessage(obtainMessage(DctConstants.CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA, enabled, 0));
        }

        private void enableMobileProvisioning() {
            final Message msg = obtainMessage(DctConstants.CMD_ENABLE_MOBILE_PROVISIONING);
            msg.setData(Bundle.forPair(DctConstants.PROVISIONING_URL_KEY, mProvisionUrl));
            sendMessage(msg);
        }

        @Override
        public void onReceive(Context context, Intent intent) {
            // Turning back on the radio can take time on the order of a minute, so show user a
            // spinner so they know something is going on.
            log("onReceive : ProvisionNotificationBroadcastReceiver");
            mProvisioningSpinner = new ProgressDialog(context);
            mProvisioningSpinner.setTitle(mNetworkOperator);
            mProvisioningSpinner.setMessage(
                    // TODO: Don't borrow "Connecting..." i18n string; give Telephony a version.
                    context.getText(com.android.internal.R.string.media_route_status_connecting));
            mProvisioningSpinner.setIndeterminate(true);
            mProvisioningSpinner.setCancelable(true);
            // Allow non-Activity Service Context to create a View.
            mProvisioningSpinner.getWindow().setType(
                    WindowManager.LayoutParams.TYPE_KEYGUARD_DIALOG);
            mProvisioningSpinner.show();
            // After timeout, hide spinner so user can at least use their device.
            // TODO: Indicate to user that it is taking an unusually long time to connect?
            sendMessageDelayed(obtainMessage(DctConstants.CMD_CLEAR_PROVISIONING_SPINNER,
                    mProvisioningSpinner), PROVISIONING_SPINNER_TIMEOUT_MILLIS);
            // This code is almost identical to the old
            // ConnectivityService.handleMobileProvisioningAction code.
            setRadio(true);
            setEnableFailFastMobileData(DctConstants.ENABLED);
            enableMobileProvisioning();
        }
    }

    @Override
    protected void finalize() {
        if(DBG && mPhone != null) log("finalize");
    }

    private ApnContext addApnContext(String type, NetworkConfig networkConfig) {
        ApnContext apnContext = new ApnContext(mPhone, type, LOG_TAG, networkConfig, this);
        mApnContexts.put(type, apnContext);
        mApnContextsById.put(ApnContext.apnIdForApnName(type), apnContext);
        mPrioritySortedApnContexts.add(apnContext);
        return apnContext;
    }

    private void initApnContexts() {
        log("initApnContexts: E");
        // Load device network attributes from resources
        String[] networkConfigStrings = mPhone.getContext().getResources().getStringArray(
                com.android.internal.R.array.networkAttributes);
        for (String networkConfigString : networkConfigStrings) {
            NetworkConfig networkConfig = new NetworkConfig(networkConfigString);
            ApnContext apnContext = null;

            switch (networkConfig.type) {
            case ConnectivityManager.TYPE_MOBILE:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_DEFAULT, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_MMS:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_MMS, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_SUPL:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_SUPL, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_DUN:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_DUN, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_HIPRI:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_HIPRI, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_FOTA:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_FOTA, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_IMS:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_IMS, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_CBS:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_CBS, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_IA:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_IA, networkConfig);
                break;
            case ConnectivityManager.TYPE_MOBILE_EMERGENCY:
                apnContext = addApnContext(PhoneConstants.APN_TYPE_EMERGENCY, networkConfig);
                break;
            default:
                log("initApnContexts: skipping unknown type=" + networkConfig.type);
                continue;
            }
            log("initApnContexts: apnContext=" + apnContext);
        }

        if (VDBG) log("initApnContexts: X mApnContexts=" + mApnContexts);
    }

    public LinkProperties getLinkProperties(String apnType) {
        ApnContext apnContext = mApnContexts.get(apnType);
        if (apnContext != null) {
            DcAsyncChannel dcac = apnContext.getDcAc();
            if (dcac != null) {
                if (DBG) log("return link properites for " + apnType);
                return dcac.getLinkPropertiesSync();
            }
        }
        if (DBG) log("return new LinkProperties");
        return new LinkProperties();
    }

    public NetworkCapabilities getNetworkCapabilities(String apnType) {
        ApnContext apnContext = mApnContexts.get(apnType);
        if (apnContext!=null) {
            DcAsyncChannel dataConnectionAc = apnContext.getDcAc();
            if (dataConnectionAc != null) {
                if (DBG) {
                    log("get active pdp is not null, return NetworkCapabilities for " + apnType);
                }
                return dataConnectionAc.getNetworkCapabilitiesSync();
            }
        }
        if (DBG) log("return new NetworkCapabilities");
        return new NetworkCapabilities();
    }

    // Return all active apn types
    public String[] getActiveApnTypes() {
        if (DBG) log("get all active apn types");
        ArrayList<String> result = new ArrayList<String>();

        for (ApnContext apnContext : mApnContexts.values()) {
            if (mAttached.get() && apnContext.isReady()) {
                result.add(apnContext.getApnType());
            }
        }

        return result.toArray(new String[0]);
    }

    // Return active apn of specific apn type
    public String getActiveApnString(String apnType) {
        if (VDBG) log( "get active apn string for type:" + apnType);
        ApnContext apnContext = mApnContexts.get(apnType);
        if (apnContext != null) {
            ApnSetting apnSetting = apnContext.getApnSetting();
            if (apnSetting != null) {
                return apnSetting.apn;
            }
        }
        return null;
    }

    /**
     * Returns {@link DctConstants.State} based on the state of the {@link DataConnection} that
     * contains a {@link ApnSetting} that supported the given apn type {@code anpType}.
     *
     * <p>
     * Assumes there is less than one {@link ApnSetting} can support the given apn type.
     */
    public DctConstants.State getState(String apnType) {
        for (DataConnection dc : mDataConnections.values()) {
            ApnSetting apnSetting = dc.getApnSetting();
            if (apnSetting != null && apnSetting.canHandleType(apnType)) {
                if (dc.isActive()) {
                    return DctConstants.State.CONNECTED;
                } else if (dc.isActivating()) {
                    return DctConstants.State.CONNECTING;
                } else if (dc.isInactive()) {
                    return DctConstants.State.IDLE;
                } else if (dc.isDisconnecting()) {
                    return DctConstants.State.DISCONNECTING;
                }
            }
        }

        return DctConstants.State.IDLE;
    }

    // Return if apn type is a provisioning apn.
    private boolean isProvisioningApn(String apnType) {
        ApnContext apnContext = mApnContexts.get(apnType);
        if (apnContext != null) {
            return apnContext.isProvisioningApn();
        }
        return false;
    }

    // Return state of overall
    public DctConstants.State getOverallState() {
        boolean isConnecting = false;
        boolean isFailed = true; // All enabled Apns should be FAILED.
        boolean isAnyEnabled = false;

        for (ApnContext apnContext : mApnContexts.values()) {
            if (apnContext.isEnabled()) {
                isAnyEnabled = true;
                switch (apnContext.getState()) {
                case CONNECTED:
                case DISCONNECTING:
                    if (VDBG) log("overall state is CONNECTED");
                    return DctConstants.State.CONNECTED;
                case RETRYING:
                case CONNECTING:
                    isConnecting = true;
                    isFailed = false;
                    break;
                case IDLE:
                case SCANNING:
                    isFailed = false;
                    break;
                default:
                    isAnyEnabled = true;
                    break;
                }
            }
        }

        if (!isAnyEnabled) { // Nothing enabled. return IDLE.
            if (VDBG) log( "overall state is IDLE");
            return DctConstants.State.IDLE;
        }

        if (isConnecting) {
            if (VDBG) log( "overall state is CONNECTING");
            return DctConstants.State.CONNECTING;
        } else if (!isFailed) {
            if (VDBG) log( "overall state is IDLE");
            return DctConstants.State.IDLE;
        } else {
            if (VDBG) log( "overall state is FAILED");
            return DctConstants.State.FAILED;
        }
    }

    /**
     * Whether data is enabled. This does not only check isUserDataEnabled(), but also
     * others like CarrierDataEnabled and internalDataEnabled.
     */
    @VisibleForTesting
    public boolean isDataEnabled() {
        return mDataEnabledSettings.isDataEnabled();
    }

    //****** Called from ServiceStateTracker
    /**
     * Invoked when ServiceStateTracker observes a transition from GPRS
     * attach to detach.
     */
    private void onDataConnectionDetached() {
        /*
         * We presently believe it is unnecessary to tear down the PDP context
         * when GPRS detaches, but we should stop the network polling.
         */
        if (DBG) log ("onDataConnectionDetached: stop polling and notify detached");
        stopNetStatPoll();
        stopDataStallAlarm();
        notifyDataConnection(Phone.REASON_DATA_DETACHED);
        mAttached.set(false);
    }

    private void onDataConnectionAttached() {
        if (DBG) log("onDataConnectionAttached");
        mAttached.set(true);
        if (getOverallState() == DctConstants.State.CONNECTED) {
            if (DBG) log("onDataConnectionAttached: start polling notify attached");
            startNetStatPoll();
            startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
            notifyDataConnection(Phone.REASON_DATA_ATTACHED);
        } else {
            // update APN availability so that APN can be enabled.
            notifyOffApnsOfAvailability(Phone.REASON_DATA_ATTACHED);
        }
        if (mAutoAttachOnCreationConfig) {
            mAutoAttachOnCreation.set(true);
        }
        setupDataOnConnectableApns(Phone.REASON_DATA_ATTACHED);
    }

    /**
     * Check if it is allowed to make a data connection (without checking APN context specific
     * conditions).
     *
     * @param dataConnectionReasons Data connection allowed or disallowed reasons as the output
     *                              param. It's okay to pass null here and no reasons will be
     *                              provided.
     * @return True if data connection is allowed, otherwise false.
     */
    public boolean isDataAllowed(DataConnectionReasons dataConnectionReasons) {
        return isDataAllowed(null, dataConnectionReasons);
    }

    /**
     * Check if it is allowed to make a data connection for a given APN type.
     *
     * @param apnContext APN context. If passing null, then will only check general but not APN
     *                   specific conditions (e.g. APN state, metered/unmetered APN).
     * @param dataConnectionReasons Data connection allowed or disallowed reasons as the output
     *                              param. It's okay to pass null here and no reasons will be
     *                              provided.
     * @return True if data connection is allowed, otherwise false.
     */
    boolean isDataAllowed(ApnContext apnContext, DataConnectionReasons dataConnectionReasons) {
        // Step 1: Get all environment conditions.
        // Step 2: Special handling for emergency APN.
        // Step 3. Build disallowed reasons.
        // Step 4: Determine if data should be allowed in some special conditions.

        DataConnectionReasons reasons = new DataConnectionReasons();

        // Step 1: Get all environment conditions.
        final boolean internalDataEnabled = mDataEnabledSettings.isInternalDataEnabled();
        boolean attachedState = mAttached.get();
        boolean desiredPowerState = mPhone.getServiceStateTracker().getDesiredPowerState();
        boolean radioStateFromCarrier = mPhone.getServiceStateTracker().getPowerStateFromCarrier();
        // TODO: Remove this hack added by ag/641832.
        int radioTech = mPhone.getServiceState().getRilDataRadioTechnology();
        if (radioTech == ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN) {
            desiredPowerState = true;
            radioStateFromCarrier = true;
        }

        boolean recordsLoaded = mIccRecords.get() != null && mIccRecords.get().getRecordsLoaded();

        boolean defaultDataSelected = SubscriptionManager.isValidSubscriptionId(
                SubscriptionManager.getDefaultDataSubscriptionId());

        boolean isMeteredApnType = apnContext == null
                || ApnSetting.isMeteredApnType(apnContext.getApnType(), mPhone);

        PhoneConstants.State phoneState = PhoneConstants.State.IDLE;
        // Note this is explicitly not using mPhone.getState.  See b/19090488.
        // mPhone.getState reports the merge of CS and PS (volte) voice call state
        // but we only care about CS calls here for data/voice concurrency issues.
        // Calling getCallTracker currently gives you just the CS side where the
        // ImsCallTracker is held internally where applicable.
        // This should be redesigned to ask explicitly what we want:
        // voiceCallStateAllowDataCall, or dataCallAllowed or something similar.
        if (mPhone.getCallTracker() != null) {
            phoneState = mPhone.getCallTracker().getState();
        }

        // Step 2: Special handling for emergency APN.
        if (apnContext != null
                && apnContext.getApnType().equals(PhoneConstants.APN_TYPE_EMERGENCY)
                && apnContext.isConnectable()) {
            // If this is an emergency APN, as long as the APN is connectable, we
            // should allow it.
            if (dataConnectionReasons != null) {
                dataConnectionReasons.add(DataAllowedReasonType.EMERGENCY_APN);
            }
            // Bail out without further checks.
            return true;
        }

        // Step 3. Build disallowed reasons.
        if (apnContext != null && !apnContext.isConnectable()) {
            reasons.add(DataDisallowedReasonType.APN_NOT_CONNECTABLE);
        }

        // If RAT is IWLAN then don't allow default/IA PDP at all.
        // Rest of APN types can be evaluated for remaining conditions.
        if ((apnContext != null && (apnContext.getApnType().equals(PhoneConstants.APN_TYPE_DEFAULT)
                || apnContext.getApnType().equals(PhoneConstants.APN_TYPE_IA)))
                && (radioTech == ServiceState.RIL_RADIO_TECHNOLOGY_IWLAN)) {
            reasons.add(DataDisallowedReasonType.ON_IWLAN);
        }

        if (isEmergency()) {
            reasons.add(DataDisallowedReasonType.IN_ECBM);
        }

        if (!(attachedState || mAutoAttachOnCreation.get())) {
            reasons.add(DataDisallowedReasonType.NOT_ATTACHED);
        }
        if (!recordsLoaded) {
            reasons.add(DataDisallowedReasonType.RECORD_NOT_LOADED);
        }
        if (phoneState != PhoneConstants.State.IDLE
                && !mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed()) {
            reasons.add(DataDisallowedReasonType.INVALID_PHONE_STATE);
            reasons.add(DataDisallowedReasonType.CONCURRENT_VOICE_DATA_NOT_ALLOWED);
        }
        if (!internalDataEnabled) {
            reasons.add(DataDisallowedReasonType.INTERNAL_DATA_DISABLED);
        }
        if (!defaultDataSelected) {
            reasons.add(DataDisallowedReasonType.DEFAULT_DATA_UNSELECTED);
        }
        if (mPhone.getServiceState().getDataRoaming() && !getDataRoamingEnabled()) {
            reasons.add(DataDisallowedReasonType.ROAMING_DISABLED);
        }
        if (mIsPsRestricted) {
            reasons.add(DataDisallowedReasonType.PS_RESTRICTED);
        }
        if (!desiredPowerState) {
            reasons.add(DataDisallowedReasonType.UNDESIRED_POWER_STATE);
        }
        if (!radioStateFromCarrier) {
            reasons.add(DataDisallowedReasonType.RADIO_DISABLED_BY_CARRIER);
        }
        if (!mDataEnabledSettings.isDataEnabled()) {
            reasons.add(DataDisallowedReasonType.DATA_DISABLED);
        }

        // If there are hard disallowed reasons, we should not allow data connection no matter what.
        if (reasons.containsHardDisallowedReasons()) {
            if (dataConnectionReasons != null) {
                dataConnectionReasons.copyFrom(reasons);
            }
            return false;
        }

        // Step 4: Determine if data should be allowed in some special conditions.

        // At this point, if data is not allowed, it must be because of the soft reasons. We
        // should start to check some special conditions that data will be allowed.

        // If the request APN type is unmetered and there are soft disallowed reasons (e.g. data
        // disabled, data roaming disabled) existing, we should allow the data because the user
        // won't be charged anyway.
        if (!isMeteredApnType && !reasons.allowed()) {
            reasons.add(DataAllowedReasonType.UNMETERED_APN);
        }

        // If the request is restricted and there are only disallowed reasons due to data
        // disabled, we should allow the data.
        if (apnContext != null
                && !apnContext.hasNoRestrictedRequests(true)
                && reasons.contains(DataDisallowedReasonType.DATA_DISABLED)) {
            reasons.add(DataAllowedReasonType.RESTRICTED_REQUEST);
        }

        // If at this point, we still haven't built any disallowed reasons, we should allow data.
        if (reasons.allowed()) {
            reasons.add(DataAllowedReasonType.NORMAL);
        }

        if (dataConnectionReasons != null) {
            dataConnectionReasons.copyFrom(reasons);
        }

        return reasons.allowed();
    }

    // arg for setupDataOnConnectableApns
    private enum RetryFailures {
        // retry failed networks always (the old default)
        ALWAYS,
        // retry only when a substantial change has occurred.  Either:
        // 1) we were restricted by voice/data concurrency and aren't anymore
        // 2) our apn list has change
        ONLY_ON_CHANGE
    };

    private void setupDataOnConnectableApns(String reason) {
        setupDataOnConnectableApns(reason, RetryFailures.ALWAYS);
    }

    private void setupDataOnConnectableApns(String reason, RetryFailures retryFailures) {
        if (VDBG) log("setupDataOnConnectableApns: " + reason);

        if (DBG && !VDBG) {
            StringBuilder sb = new StringBuilder(120);
            for (ApnContext apnContext : mPrioritySortedApnContexts) {
                sb.append(apnContext.getApnType());
                sb.append(":[state=");
                sb.append(apnContext.getState());
                sb.append(",enabled=");
                sb.append(apnContext.isEnabled());
                sb.append("] ");
            }
            log("setupDataOnConnectableApns: " + reason + " " + sb);
        }

        for (ApnContext apnContext : mPrioritySortedApnContexts) {
            if (VDBG) log("setupDataOnConnectableApns: apnContext " + apnContext);

            if (apnContext.getState() == DctConstants.State.FAILED
                    || apnContext.getState() == DctConstants.State.SCANNING) {
                if (retryFailures == RetryFailures.ALWAYS) {
                    apnContext.releaseDataConnection(reason);
                } else if (apnContext.isConcurrentVoiceAndDataAllowed() == false &&
                        mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed()) {
                    // RetryFailures.ONLY_ON_CHANGE - check if voice concurrency has changed
                    apnContext.releaseDataConnection(reason);
                }
            }
            if (apnContext.isConnectable()) {
                log("isConnectable() call trySetupData");
                apnContext.setReason(reason);
                trySetupData(apnContext);
            }
        }
    }

    boolean isEmergency() {
        final boolean result = mPhone.isInEcm() || mPhone.isInEmergencyCall();
        log("isEmergency: result=" + result);
        return result;
    }

    private boolean trySetupData(ApnContext apnContext) {

        if (mPhone.getSimulatedRadioControl() != null) {
            // Assume data is connected on the simulator
            // FIXME  this can be improved
            apnContext.setState(DctConstants.State.CONNECTED);
            mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());

            log("trySetupData: X We're on the simulator; assuming connected retValue=true");
            return true;
        }

        DataConnectionReasons dataConnectionReasons = new DataConnectionReasons();
        boolean isDataAllowed = isDataAllowed(apnContext, dataConnectionReasons);
        String logStr = "trySetupData for APN type " + apnContext.getApnType() + ", reason: "
                + apnContext.getReason() + ". " + dataConnectionReasons.toString();
        if (DBG) log(logStr);
        apnContext.requestLog(logStr);
        if (isDataAllowed) {
            if (apnContext.getState() == DctConstants.State.FAILED) {
                String str = "trySetupData: make a FAILED ApnContext IDLE so its reusable";
                if (DBG) log(str);
                apnContext.requestLog(str);
                apnContext.setState(DctConstants.State.IDLE);
            }
            int radioTech = mPhone.getServiceState().getRilDataRadioTechnology();
            apnContext.setConcurrentVoiceAndDataAllowed(mPhone.getServiceStateTracker()
                    .isConcurrentVoiceAndDataAllowed());
            if (apnContext.getState() == DctConstants.State.IDLE) {
                ArrayList<ApnSetting> waitingApns =
                        buildWaitingApns(apnContext.getApnType(), radioTech);
                if (waitingApns.isEmpty()) {
                    notifyNoData(DcFailCause.MISSING_UNKNOWN_APN, apnContext);
                    notifyOffApnsOfAvailability(apnContext.getReason());
                    String str = "trySetupData: X No APN found retValue=false";
                    if (DBG) log(str);
                    apnContext.requestLog(str);
                    return false;
                } else {
                    apnContext.setWaitingApns(waitingApns);
                    if (DBG) {
                        log ("trySetupData: Create from mAllApnSettings : "
                                    + apnListToString(mAllApnSettings));
                    }
                }
            }

            boolean retValue = setupData(apnContext, radioTech, dataConnectionReasons.contains(
                    DataAllowedReasonType.UNMETERED_APN));
            notifyOffApnsOfAvailability(apnContext.getReason());

            if (DBG) log("trySetupData: X retValue=" + retValue);
            return retValue;
        } else {
            if (!apnContext.getApnType().equals(PhoneConstants.APN_TYPE_DEFAULT)
                    && apnContext.isConnectable()) {
                mPhone.notifyDataConnectionFailed(apnContext.getReason(), apnContext.getApnType());
            }
            notifyOffApnsOfAvailability(apnContext.getReason());

            StringBuilder str = new StringBuilder();

            str.append("trySetupData failed. apnContext = [type=" + apnContext.getApnType()
                    + ", mState=" + apnContext.getState() + ", apnEnabled="
                    + apnContext.isEnabled() + ", mDependencyMet="
                    + apnContext.getDependencyMet() + "] ");

            if (!mDataEnabledSettings.isDataEnabled()) {
                str.append("isDataEnabled() = false. " + mDataEnabledSettings);
            }

            // If this is a data retry, we should set the APN state to FAILED so it won't stay
            // in SCANNING forever.
            if (apnContext.getState() == DctConstants.State.SCANNING) {
                apnContext.setState(DctConstants.State.FAILED);
                str.append(" Stop retrying.");
            }

            if (DBG) log(str.toString());
            apnContext.requestLog(str.toString());
            return false;
        }
    }

    // Disabled apn's still need avail/unavail notifications - send them out
    private void notifyOffApnsOfAvailability(String reason) {
        for (ApnContext apnContext : mApnContexts.values()) {
            if (!mAttached.get() || !apnContext.isReady()) {
                if (VDBG) log("notifyOffApnOfAvailability type:" + apnContext.getApnType());
                mPhone.notifyDataConnection(reason != null ? reason : apnContext.getReason(),
                                            apnContext.getApnType(),
                                            PhoneConstants.DataState.DISCONNECTED);
            } else {
                if (VDBG) {
                    log("notifyOffApnsOfAvailability skipped apn due to attached && isReady " +
                            apnContext.toString());
                }
            }
        }
    }

    /**
     * If tearDown is true, this only tears down a CONNECTED session. Presently,
     * there is no mechanism for abandoning an CONNECTING session,
     * but would likely involve cancelling pending async requests or
     * setting a flag or new state to ignore them when they came in
     * @param tearDown true if the underlying DataConnection should be
     * disconnected.
     * @param reason reason for the clean up.
     * @return boolean - true if we did cleanup any connections, false if they
     *                   were already all disconnected.
     */
    private boolean cleanUpAllConnections(boolean tearDown, String reason) {
        if (DBG) log("cleanUpAllConnections: tearDown=" + tearDown + " reason=" + reason);
        boolean didDisconnect = false;
        boolean disableMeteredOnly = false;

        // reasons that only metered apn will be torn down
        if (!TextUtils.isEmpty(reason)) {
            disableMeteredOnly = reason.equals(Phone.REASON_DATA_SPECIFIC_DISABLED) ||
                    reason.equals(Phone.REASON_ROAMING_ON) ||
                    reason.equals(Phone.REASON_CARRIER_ACTION_DISABLE_METERED_APN) ||
                    reason.equals(Phone.REASON_PDP_RESET);
        }

        for (ApnContext apnContext : mApnContexts.values()) {
            if (disableMeteredOnly) {
                // Use ApnSetting to decide metered or non-metered.
                // Tear down all metered data connections.
                ApnSetting apnSetting = apnContext.getApnSetting();
                if (apnSetting != null && apnSetting.isMetered(mPhone)) {
                    if (apnContext.isDisconnected() == false) didDisconnect = true;
                    if (DBG) log("clean up metered ApnContext Type: " + apnContext.getApnType());
                    apnContext.setReason(reason);
                    cleanUpConnection(tearDown, apnContext);
                }
            } else {
                // Exclude the IMS APN from single DataConenction case.
                if (reason.equals(Phone.REASON_SINGLE_PDN_ARBITRATION)
                        && apnContext.getApnType().equals(PhoneConstants.APN_TYPE_IMS)) {
                    continue;
                }
                // TODO - only do cleanup if not disconnected
                if (apnContext.isDisconnected() == false) didDisconnect = true;
                apnContext.setReason(reason);
                cleanUpConnection(tearDown, apnContext);
            }
        }

        stopNetStatPoll();
        stopDataStallAlarm();

        // TODO: Do we need mRequestedApnType?
        mRequestedApnType = PhoneConstants.APN_TYPE_DEFAULT;

        log("cleanUpConnection: mDisconnectPendingCount = " + mDisconnectPendingCount);
        if (tearDown && mDisconnectPendingCount == 0) {
            notifyDataDisconnectComplete();
            notifyAllDataDisconnected();
        }

        return didDisconnect;
    }

    /**
     * Cleanup all connections.
     *
     * TODO: Cleanup only a specified connection passed as a parameter.
     *       Also, make sure when you clean up a conn, if it is last apply
     *       logic as though it is cleanupAllConnections
     *
     * @param cause for the clean up.
     */
    private void onCleanUpAllConnections(String cause) {
        cleanUpAllConnections(true, cause);
    }

    void sendCleanUpConnection(boolean tearDown, ApnContext apnContext) {
        if (DBG) log("sendCleanUpConnection: tearDown=" + tearDown + " apnContext=" + apnContext);
        Message msg = obtainMessage(DctConstants.EVENT_CLEAN_UP_CONNECTION);
        msg.arg1 = tearDown ? 1 : 0;
        msg.arg2 = 0;
        msg.obj = apnContext;
        sendMessage(msg);
    }

    private void cleanUpConnection(boolean tearDown, ApnContext apnContext) {
        if (apnContext == null) {
            if (DBG) log("cleanUpConnection: apn context is null");
            return;
        }

        DcAsyncChannel dcac = apnContext.getDcAc();
        String str = "cleanUpConnection: tearDown=" + tearDown + " reason=" +
                apnContext.getReason();
        if (VDBG) log(str + " apnContext=" + apnContext);
        apnContext.requestLog(str);
        if (tearDown) {
            if (apnContext.isDisconnected()) {
                // The request is tearDown and but ApnContext is not connected.
                // If apnContext is not enabled anymore, break the linkage to the DCAC/DC.
                apnContext.setState(DctConstants.State.IDLE);
                if (!apnContext.isReady()) {
                    if (dcac != null) {
                        str = "cleanUpConnection: teardown, disconnected, !ready";
                        if (DBG) log(str + " apnContext=" + apnContext);
                        apnContext.requestLog(str);
                        dcac.tearDown(apnContext, "", null);
                    }
                    apnContext.setDataConnectionAc(null);
                }
            } else {
                // Connection is still there. Try to clean up.
                if (dcac != null) {
                    if (apnContext.getState() != DctConstants.State.DISCONNECTING) {
                        boolean disconnectAll = false;
                        if (PhoneConstants.APN_TYPE_DUN.equals(apnContext.getApnType())) {
                            // CAF_MSIM is this below condition required.
                            // if (PhoneConstants.APN_TYPE_DUN.equals(PhoneConstants.APN_TYPE_DEFAULT)) {
                            if (teardownForDun()) {
                                if (DBG) {
                                    log("cleanUpConnection: disconnectAll DUN connection");
                                }
                                // we need to tear it down - we brought it up just for dun and
                                // other people are camped on it and now dun is done.  We need
                                // to stop using it and let the normal apn list get used to find
                                // connections for the remaining desired connections
                                disconnectAll = true;
                            }
                        }
                        final int generation = apnContext.getConnectionGeneration();
                        str = "cleanUpConnection: tearing down" + (disconnectAll ? " all" : "") +
                                " using gen#" + generation;
                        if (DBG) log(str + "apnContext=" + apnContext);
                        apnContext.requestLog(str);
                        Pair<ApnContext, Integer> pair =
                                new Pair<ApnContext, Integer>(apnContext, generation);
                        Message msg = obtainMessage(DctConstants.EVENT_DISCONNECT_DONE, pair);
                        if (disconnectAll) {
                            apnContext.getDcAc().tearDownAll(apnContext.getReason(), msg);
                        } else {
                            apnContext.getDcAc()
                                .tearDown(apnContext, apnContext.getReason(), msg);
                        }
                        apnContext.setState(DctConstants.State.DISCONNECTING);
                        mDisconnectPendingCount++;
                    }
                } else {
                    // apn is connected but no reference to dcac.
                    // Should not be happen, but reset the state in case.
                    apnContext.setState(DctConstants.State.IDLE);
                    apnContext.requestLog("cleanUpConnection: connected, bug no DCAC");
                    mPhone.notifyDataConnection(apnContext.getReason(),
                                                apnContext.getApnType());
                }
            }
        } else {
            // force clean up the data connection.
            if (dcac != null) dcac.reqReset();
            apnContext.setState(DctConstants.State.IDLE);
            mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());
            apnContext.setDataConnectionAc(null);
        }

        // Make sure reconnection alarm is cleaned up if there is no ApnContext
        // associated to the connection.
        if (dcac != null) {
            cancelReconnectAlarm(apnContext);
        }
        str = "cleanUpConnection: X tearDown=" + tearDown + " reason=" + apnContext.getReason();
        if (DBG) log(str + " apnContext=" + apnContext + " dcac=" + apnContext.getDcAc());
        apnContext.requestLog(str);
    }

    /**
     * Fetch the DUN apns
     * @return a list of DUN ApnSetting objects
     */
    @VisibleForTesting
    public @NonNull ArrayList<ApnSetting> fetchDunApns() {
        if (SystemProperties.getBoolean("net.tethering.noprovisioning", false)) {
            log("fetchDunApns: net.tethering.noprovisioning=true ret: empty list");
            return new ArrayList<ApnSetting>(0);
        }
        int bearer = mPhone.getServiceState().getRilDataRadioTechnology();
        IccRecords r = mIccRecords.get();
        String operator = (r != null) ? r.getOperatorNumeric() : "";
        ArrayList<ApnSetting> dunCandidates = new ArrayList<ApnSetting>();
        ArrayList<ApnSetting> retDunSettings = new ArrayList<ApnSetting>();

        // Places to look for tether APN in order: TETHER_DUN_APN setting (to be deprecated soon),
        // APN database, and config_tether_apndata resource (to be deprecated soon).
        String apnData = Settings.Global.getString(mResolver, Settings.Global.TETHER_DUN_APN);
        if (!TextUtils.isEmpty(apnData)) {
            dunCandidates.addAll(ApnSetting.arrayFromString(apnData));
            if (VDBG) log("fetchDunApns: dunCandidates from Setting: " + dunCandidates);
        }

        // todo: remove this and config_tether_apndata after APNs are moved from overlay to apns xml
        // If TETHER_DUN_APN isn't set or APN database doesn't have dun APN,
        // try the resource as last resort.
        if (dunCandidates.isEmpty()) {
            String[] apnArrayData = mPhone.getContext().getResources()
                .getStringArray(R.array.config_tether_apndata);
            if (!ArrayUtils.isEmpty(apnArrayData)) {
                for (String apnString : apnArrayData) {
                    ApnSetting apn = ApnSetting.fromString(apnString);
                    // apn may be null if apnString isn't valid or has error parsing
                    if (apn != null) dunCandidates.add(apn);
                }
                if (VDBG) log("fetchDunApns: dunCandidates from resource: " + dunCandidates);
            }
        }

        if (dunCandidates.isEmpty()) {
            if (!ArrayUtils.isEmpty(mAllApnSettings)) {
                for (ApnSetting apn : mAllApnSettings) {
                    if (apn.canHandleType(PhoneConstants.APN_TYPE_DUN)) {
                        dunCandidates.add(apn);
                    }
                }
                if (VDBG) log("fetchDunApns: dunCandidates from database: " + dunCandidates);
            }
        }

        for (ApnSetting dunSetting : dunCandidates) {
            if (!ServiceState.bitmaskHasTech(dunSetting.networkTypeBitmask,
                    ServiceState.rilRadioTechnologyToNetworkType(bearer))) {
                continue;
            }
            if (dunSetting.numeric.equals(operator)) {
                if (dunSetting.hasMvnoParams()) {
                    if (r != null && ApnSetting.mvnoMatches(r, dunSetting.mvnoType,
                            dunSetting.mvnoMatchData)) {
                        retDunSettings.add(dunSetting);
                    }
                } else if (mMvnoMatched == false) {
                    retDunSettings.add(dunSetting);
                }
            }
        }

        if (VDBG) log("fetchDunApns: dunSettings=" + retDunSettings);
        return retDunSettings;
    }

    private int getPreferredApnSetId() {
        Cursor c = mPhone.getContext().getContentResolver()
                .query(Uri.withAppendedPath(Telephony.Carriers.CONTENT_URI,
                    "preferapnset/subId/" + mPhone.getSubId()),
                        new String[] {Telephony.Carriers.APN_SET_ID}, null, null, null);
        if (c.getCount() < 1) {
            loge("getPreferredApnSetId: no APNs found");
            return Telephony.Carriers.NO_SET_SET;
        } else {
            c.moveToFirst();
            return c.getInt(0 /* index of Telephony.Carriers.APN_SET_ID */);
        }
    }

    public boolean hasMatchedTetherApnSetting() {
        ArrayList<ApnSetting> matches = fetchDunApns();
        log("hasMatchedTetherApnSetting: APNs=" + matches);
        return matches.size() > 0;
    }

    /**
     * Determine if DUN connection is special and we need to teardown on start/stop
     */
    private boolean teardownForDun() {
        // CDMA always needs to do this the profile id is correct
        final int rilRat = mPhone.getServiceState().getRilDataRadioTechnology();
        if (ServiceState.isCdma(rilRat)) return true;

        ArrayList<ApnSetting> apns = fetchDunApns();
        return apns.size() > 0;
    }

    /**
     * Cancels the alarm associated with apnContext.
     *
     * @param apnContext on which the alarm should be stopped.
     */
    private void cancelReconnectAlarm(ApnContext apnContext) {
        if (apnContext == null) return;

        PendingIntent intent = apnContext.getReconnectIntent();

        if (intent != null) {
                AlarmManager am =
                    (AlarmManager) mPhone.getContext().getSystemService(Context.ALARM_SERVICE);
                am.cancel(intent);
                apnContext.setReconnectIntent(null);
        }
    }

    /**
     * @param types comma delimited list of APN types
     * @return array of APN types
     */
    private String[] parseTypes(String types) {
        String[] result;
        // If unset, set to DEFAULT.
        if (types == null || types.equals("")) {
            result = new String[1];
            result[0] = PhoneConstants.APN_TYPE_ALL;
        } else {
            result = types.split(",");
        }
        return result;
    }

    boolean isPermanentFailure(DcFailCause dcFailCause) {
        return (dcFailCause.isPermanentFailure(mPhone.getContext(), mPhone.getSubId()) &&
                (mAttached.get() == false || dcFailCause != DcFailCause.SIGNAL_LOST));
    }

    private ApnSetting makeApnSetting(Cursor cursor) {
        String[] types = parseTypes(
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.TYPE)));
        int networkTypeBitmask = cursor.getInt(
                cursor.getColumnIndexOrThrow(Telephony.Carriers.NETWORK_TYPE_BITMASK));

        ApnSetting apn = new ApnSetting(
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers._ID)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.NUMERIC)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.NAME)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN)),
                NetworkUtils.trimV4AddrZeros(
                        cursor.getString(
                        cursor.getColumnIndexOrThrow(Telephony.Carriers.PROXY))),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PORT)),
                NetworkUtils.trimV4AddrZeros(
                        cursor.getString(
                        cursor.getColumnIndexOrThrow(Telephony.Carriers.MMSC))),
                NetworkUtils.trimV4AddrZeros(
                        cursor.getString(
                        cursor.getColumnIndexOrThrow(Telephony.Carriers.MMSPROXY))),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.MMSPORT)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.USER)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PASSWORD)),
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.AUTH_TYPE)),
                types,
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.PROTOCOL)),
                cursor.getString(cursor.getColumnIndexOrThrow(
                        Telephony.Carriers.ROAMING_PROTOCOL)),
                cursor.getInt(cursor.getColumnIndexOrThrow(
                        Telephony.Carriers.CARRIER_ENABLED)) == 1,
                networkTypeBitmask,
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.PROFILE_ID)),
                cursor.getInt(cursor.getColumnIndexOrThrow(
                        Telephony.Carriers.MODEM_COGNITIVE)) == 1,
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.MAX_CONNS)),
                cursor.getInt(cursor.getColumnIndexOrThrow(
                        Telephony.Carriers.WAIT_TIME)),
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.MAX_CONNS_TIME)),
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.MTU)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.MVNO_TYPE)),
                cursor.getString(cursor.getColumnIndexOrThrow(Telephony.Carriers.MVNO_MATCH_DATA)),
                cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers.APN_SET_ID)));
        return apn;
    }

    private ArrayList<ApnSetting> createApnList(Cursor cursor) {
        ArrayList<ApnSetting> mnoApns = new ArrayList<ApnSetting>();
        ArrayList<ApnSetting> mvnoApns = new ArrayList<ApnSetting>();
        IccRecords r = mIccRecords.get();

        if (cursor.moveToFirst()) {
            do {
                ApnSetting apn = makeApnSetting(cursor);
                if (apn == null) {
                    continue;
                }

                if (apn.hasMvnoParams()) {
                    if (r != null && ApnSetting.mvnoMatches(r, apn.mvnoType, apn.mvnoMatchData)) {
                        mvnoApns.add(apn);
                    }
                } else {
                    mnoApns.add(apn);
                }
            } while (cursor.moveToNext());
        }

        ArrayList<ApnSetting> result;
        if (mvnoApns.isEmpty()) {
            result = mnoApns;
            mMvnoMatched = false;
        } else {
            result = mvnoApns;
            mMvnoMatched = true;
        }
        if (DBG) log("createApnList: X result=" + result);
        return result;
    }

    private boolean dataConnectionNotInUse(DcAsyncChannel dcac) {
        if (DBG) log("dataConnectionNotInUse: check if dcac is inuse dcac=" + dcac);
        for (ApnContext apnContext : mApnContexts.values()) {
            if (apnContext.getDcAc() == dcac) {
                if (DBG) log("dataConnectionNotInUse: in use by apnContext=" + apnContext);
                return false;
            }
        }
        // TODO: Fix retry handling so free DataConnections have empty apnlists.
        // Probably move retry handling into DataConnections and reduce complexity
        // of DCT.
        if (DBG) log("dataConnectionNotInUse: tearDownAll");
        dcac.tearDownAll("No connection", null);
        if (DBG) log("dataConnectionNotInUse: not in use return true");
        return true;
    }

    private DcAsyncChannel findFreeDataConnection() {
        for (DcAsyncChannel dcac : mDataConnectionAcHashMap.values()) {
            if (dcac.isInactiveSync() && dataConnectionNotInUse(dcac)) {
                if (DBG) {
                    log("findFreeDataConnection: found free DataConnection=" +
                        " dcac=" + dcac);
                }
                return dcac;
            }
        }
        log("findFreeDataConnection: NO free DataConnection");
        return null;
    }

    /**
     * Setup a data connection based on given APN type.
     *
     * @param apnContext APN context
     * @param radioTech RAT of the data connection
     * @param unmeteredUseOnly True if this data connection should be only used for unmetered
     *                         purposes only.
     * @return True if successful, otherwise false.
     */
    private boolean setupData(ApnContext apnContext, int radioTech, boolean unmeteredUseOnly) {
        if (DBG) log("setupData: apnContext=" + apnContext);
        apnContext.requestLog("setupData");
        ApnSetting apnSetting;
        DcAsyncChannel dcac = null;

        apnSetting = apnContext.getNextApnSetting();

        if (apnSetting == null) {
            if (DBG) log("setupData: return for no apn found!");
            return false;
        }

        int profileId = apnSetting.profileId;
        if (profileId == 0) {
            profileId = getApnProfileID(apnContext.getApnType());
        }

        // On CDMA, if we're explicitly asking for DUN, we need have
        // a dun-profiled connection so we can't share an existing one
        // On GSM/LTE we can share existing apn connections provided they support
        // this type.
        if (!apnContext.getApnType().equals(PhoneConstants.APN_TYPE_DUN)
                || ServiceState.isGsm(mPhone.getServiceState().getRilDataRadioTechnology())) {
            dcac = checkForCompatibleConnectedApnContext(apnContext);
            if (dcac != null) {
                // Get the dcacApnSetting for the connection we want to share.
                ApnSetting dcacApnSetting = dcac.getApnSettingSync();
                if (dcacApnSetting != null) {
                    // Setting is good, so use it.
                    apnSetting = dcacApnSetting;
                }
            }
        }
        if (dcac == null) {
            if (isOnlySingleDcAllowed(radioTech)) {
                if (isHigherPriorityApnContextActive(apnContext)) {
                    if (DBG) {
                        log("setupData: Higher priority ApnContext active.  Ignoring call");
                    }
                    return false;
                }

                if (!apnContext.getApnType().equals(PhoneConstants.APN_TYPE_IMS)) {
                    // Only lower priority calls left.  Disconnect them all in this single PDP case
                    // so that we can bring up the requested higher priority call (once we receive
                    // response for deactivate request for the calls we are about to disconnect
                    if (cleanUpAllConnections(true, Phone.REASON_SINGLE_PDN_ARBITRATION)) {
                        // If any call actually requested to be disconnected, means we can't
                        // bring up this connection yet as we need to wait for those data calls
                        // to be disconnected.
                        if (DBG) log("setupData: Some calls are disconnecting first."
                                + " Wait and retry");
                        return false;
                    }
                }

                // No other calls are active, so proceed
                if (DBG) log("setupData: Single pdp. Continue setting up data call.");
            }

            dcac = findFreeDataConnection();

            if (dcac == null) {
                dcac = createDataConnection();
            }

            if (dcac == null) {
                if (DBG) log("setupData: No free DataConnection and couldn't create one, WEIRD");
                return false;
            }
        }
        final int generation = apnContext.incAndGetConnectionGeneration();
        if (DBG) {
            log("setupData: dcac=" + dcac + " apnSetting=" + apnSetting + " gen#=" + generation);
        }

        apnContext.setDataConnectionAc(dcac);
        apnContext.setApnSetting(apnSetting);
        apnContext.setState(DctConstants.State.CONNECTING);
        mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());

        Message msg = obtainMessage();
        msg.what = DctConstants.EVENT_DATA_SETUP_COMPLETE;
        msg.obj = new Pair<ApnContext, Integer>(apnContext, generation);
        dcac.bringUp(apnContext, profileId, radioTech, unmeteredUseOnly, msg, generation);

        if (DBG) log("setupData: initing!");
        return true;
    }

    private void setInitialAttachApn() {
        ApnSetting iaApnSetting = null;
        ApnSetting defaultApnSetting = null;
        ApnSetting firstApnSetting = null;

        log("setInitialApn: E mPreferredApn=" + mPreferredApn);

        if (mPreferredApn != null && mPreferredApn.canHandleType(PhoneConstants.APN_TYPE_IA)) {
              iaApnSetting = mPreferredApn;
        } else if (mAllApnSettings != null && !mAllApnSettings.isEmpty()) {
            firstApnSetting = mAllApnSettings.get(0);
            log("setInitialApn: firstApnSetting=" + firstApnSetting);

            // Search for Initial APN setting and the first apn that can handle default
            for (ApnSetting apn : mAllApnSettings) {
                if (apn.canHandleType(PhoneConstants.APN_TYPE_IA)) {
                    // The Initial Attach APN is highest priority so use it if there is one
                    log("setInitialApn: iaApnSetting=" + apn);
                    iaApnSetting = apn;
                    break;
                } else if ((defaultApnSetting == null)
                        && (apn.canHandleType(PhoneConstants.APN_TYPE_DEFAULT))) {
                    // Use the first default apn if no better choice
                    log("setInitialApn: defaultApnSetting=" + apn);
                    defaultApnSetting = apn;
                }
            }
        }

        // The priority of apn candidates from highest to lowest is:
        //   1) APN_TYPE_IA (Initial Attach)
        //   2) mPreferredApn, i.e. the current preferred apn
        //   3) The first apn that than handle APN_TYPE_DEFAULT
        //   4) The first APN we can find.

        ApnSetting initialAttachApnSetting = null;
        if (iaApnSetting != null) {
            if (DBG) log("setInitialAttachApn: using iaApnSetting");
            initialAttachApnSetting = iaApnSetting;
        } else if (mPreferredApn != null) {
            if (DBG) log("setInitialAttachApn: using mPreferredApn");
            initialAttachApnSetting = mPreferredApn;
        } else if (defaultApnSetting != null) {
            if (DBG) log("setInitialAttachApn: using defaultApnSetting");
            initialAttachApnSetting = defaultApnSetting;
        } else if (firstApnSetting != null) {
            if (DBG) log("setInitialAttachApn: using firstApnSetting");
            initialAttachApnSetting = firstApnSetting;
        }

        if (initialAttachApnSetting == null) {
            if (DBG) log("setInitialAttachApn: X There in no available apn");
        } else {
            if (DBG) log("setInitialAttachApn: X selected Apn=" + initialAttachApnSetting);

            mDataServiceManager.setInitialAttachApn(createDataProfile(initialAttachApnSetting),
                    mPhone.getServiceState().getDataRoamingFromRegistration(), null);
        }
    }

    /**
     * Handles changes to the APN database.
     */
    private void onApnChanged() {
        DctConstants.State overallState = getOverallState();
        boolean isDisconnected = (overallState == DctConstants.State.IDLE ||
                overallState == DctConstants.State.FAILED);

        if (mPhone instanceof GsmCdmaPhone) {
            // The "current" may no longer be valid.  MMS depends on this to send properly. TBD
            ((GsmCdmaPhone)mPhone).updateCurrentCarrierInProvider();
        }

        // TODO: It'd be nice to only do this if the changed entrie(s)
        // match the current operator.
        if (DBG) log("onApnChanged: createAllApnList and cleanUpAllConnections");
        createAllApnList();
        setInitialAttachApn();
        cleanUpConnectionsOnUpdatedApns(!isDisconnected, Phone.REASON_APN_CHANGED);

        // FIXME: See bug 17426028 maybe no conditional is needed.
        if (mPhone.getSubId() == SubscriptionManager.getDefaultDataSubscriptionId()) {
            setupDataOnConnectableApns(Phone.REASON_APN_CHANGED);
        }
    }

    /**
     * @param cid Connection id provided from RIL.
     * @return DataConnectionAc associated with specified cid.
     */
    private DcAsyncChannel findDataConnectionAcByCid(int cid) {
        for (DcAsyncChannel dcac : mDataConnectionAcHashMap.values()) {
            if (dcac.getCidSync() == cid) {
                return dcac;
            }
        }
        return null;
    }

    /**
     * "Active" here means ApnContext isEnabled() and not in FAILED state
     * @param apnContext to compare with
     * @return true if higher priority active apn found
     */
    private boolean isHigherPriorityApnContextActive(ApnContext apnContext) {
        if (apnContext.getApnType().equals(PhoneConstants.APN_TYPE_IMS)) {
            return false;
        }

        for (ApnContext otherContext : mPrioritySortedApnContexts) {
            if (otherContext.getApnType().equals(PhoneConstants.APN_TYPE_IMS)) {
                continue;
            }
            if (apnContext.getApnType().equalsIgnoreCase(otherContext.getApnType())) return false;
            if (otherContext.isEnabled() && otherContext.getState() != DctConstants.State.FAILED) {
                return true;
            }
        }
        return false;
    }

    /**
     * Reports if we support multiple connections or not.
     * This is a combination of factors, based on carrier and RAT.
     * @param rilRadioTech the RIL Radio Tech currently in use
     * @return true if only single DataConnection is allowed
     */
    private boolean isOnlySingleDcAllowed(int rilRadioTech) {
        // Default single dc rats with no knowledge of carrier
        int[] singleDcRats = null;
        // get the carrier specific value, if it exists, from CarrierConfigManager.
        // generally configManager and bundle should not be null, but if they are it should be okay
        // to leave singleDcRats null as well
        CarrierConfigManager configManager = (CarrierConfigManager)
                mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        if (configManager != null) {
            PersistableBundle bundle = configManager.getConfig();
            if (bundle != null) {
                singleDcRats = bundle.getIntArray(
                        CarrierConfigManager.KEY_ONLY_SINGLE_DC_ALLOWED_INT_ARRAY);
            }
        }
        boolean onlySingleDcAllowed = false;
        if (Build.IS_DEBUGGABLE &&
                SystemProperties.getBoolean("persist.telephony.test.singleDc", false)) {
            onlySingleDcAllowed = true;
        }
        if (singleDcRats != null) {
            for (int i=0; i < singleDcRats.length && onlySingleDcAllowed == false; i++) {
                if (rilRadioTech == singleDcRats[i]) onlySingleDcAllowed = true;
            }
        }

        if (DBG) log("isOnlySingleDcAllowed(" + rilRadioTech + "): " + onlySingleDcAllowed);
        return onlySingleDcAllowed;
    }

    void sendRestartRadio() {
        if (DBG)log("sendRestartRadio:");
        Message msg = obtainMessage(DctConstants.EVENT_RESTART_RADIO);
        sendMessage(msg);
    }

    private void restartRadio() {
        if (DBG) log("restartRadio: ************TURN OFF RADIO**************");
        cleanUpAllConnections(true, Phone.REASON_RADIO_TURNED_OFF);
        mPhone.getServiceStateTracker().powerOffRadioSafely(this);
        /* Note: no need to call setRadioPower(true).  Assuming the desired
         * radio power state is still ON (as tracked by ServiceStateTracker),
         * ServiceStateTracker will call setRadioPower when it receives the
         * RADIO_STATE_CHANGED notification for the power off.  And if the
         * desired power state has changed in the interim, we don't want to
         * override it with an unconditional power on.
         */

        int reset = Integer.parseInt(SystemProperties.get("net.ppp.reset-by-timeout", "0"));
        SystemProperties.set("net.ppp.reset-by-timeout", String.valueOf(reset + 1));
    }

    /**
     * Return true if data connection need to be setup after disconnected due to
     * reason.
     *
     * @param apnContext APN context
     * @return true if try setup data connection is need for this reason
     */
    private boolean retryAfterDisconnected(ApnContext apnContext) {
        boolean retry = true;
        String reason = apnContext.getReason();

        if ( Phone.REASON_RADIO_TURNED_OFF.equals(reason) ||
                (isOnlySingleDcAllowed(mPhone.getServiceState().getRilDataRadioTechnology())
                 && isHigherPriorityApnContextActive(apnContext))) {
            retry = false;
        }
        return retry;
    }

    private void startAlarmForReconnect(long delay, ApnContext apnContext) {
        String apnType = apnContext.getApnType();

        Intent intent = new Intent(INTENT_RECONNECT_ALARM + "." + apnType);
        intent.putExtra(INTENT_RECONNECT_ALARM_EXTRA_REASON, apnContext.getReason());
        intent.putExtra(INTENT_RECONNECT_ALARM_EXTRA_TYPE, apnType);
        intent.addFlags(Intent.FLAG_RECEIVER_FOREGROUND);

        // Get current sub id.
        int subId = mPhone.getSubId();
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);

        if (DBG) {
            log("startAlarmForReconnect: delay=" + delay + " action=" + intent.getAction()
                    + " apn=" + apnContext);
        }

        PendingIntent alarmIntent = PendingIntent.getBroadcast(mPhone.getContext(), 0,
                                        intent, PendingIntent.FLAG_UPDATE_CURRENT);
        apnContext.setReconnectIntent(alarmIntent);

        // Use the exact timer instead of the inexact one to provide better user experience.
        // In some extreme cases, we saw the retry was delayed for few minutes.
        // Note that if the stated trigger time is in the past, the alarm will be triggered
        // immediately.
        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delay, alarmIntent);
    }

    private void notifyNoData(DcFailCause lastFailCauseCode,
                              ApnContext apnContext) {
        if (DBG) log( "notifyNoData: type=" + apnContext.getApnType());
        if (isPermanentFailure(lastFailCauseCode)
            && (!apnContext.getApnType().equals(PhoneConstants.APN_TYPE_DEFAULT))) {
            mPhone.notifyDataConnectionFailed(apnContext.getReason(), apnContext.getApnType());
        }
    }

    public boolean getAutoAttachOnCreation() {
        return mAutoAttachOnCreation.get();
    }

    private void onRecordsLoadedOrSubIdChanged() {
        if (DBG) log("onRecordsLoadedOrSubIdChanged: createAllApnList");
        mAutoAttachOnCreationConfig = mPhone.getContext().getResources()
                .getBoolean(com.android.internal.R.bool.config_auto_attach_data_on_creation);

        createAllApnList();
        setInitialAttachApn();
        if (mPhone.mCi.getRadioState().isOn()) {
            if (DBG) log("onRecordsLoadedOrSubIdChanged: notifying data availability");
            notifyOffApnsOfAvailability(Phone.REASON_SIM_LOADED);
        }
        setupDataOnConnectableApns(Phone.REASON_SIM_LOADED);
    }

    /**
     * Action set from carrier signalling broadcast receivers to enable/disable metered apns.
     */
    private void onSetCarrierDataEnabled(AsyncResult ar) {
        if (ar.exception != null) {
            Rlog.e(LOG_TAG, "CarrierDataEnable exception: " + ar.exception);
            return;
        }
        boolean enabled = (boolean) ar.result;
        if (enabled != mDataEnabledSettings.isCarrierDataEnabled()) {
            if (DBG) {
                log("carrier Action: set metered apns enabled: " + enabled);
            }

            // Disable/enable all metered apns
            mDataEnabledSettings.setCarrierDataEnabled(enabled);

            if (!enabled) {
                // Send otasp_sim_unprovisioned so that SuW is able to proceed and notify users
                mPhone.notifyOtaspChanged(TelephonyManager.OTASP_SIM_UNPROVISIONED);
                // Tear down all metered apns
                cleanUpAllConnections(true, Phone.REASON_CARRIER_ACTION_DISABLE_METERED_APN);
            } else {
                // Re-evaluate Otasp state
                int otaspState = mPhone.getServiceStateTracker().getOtasp();
                mPhone.notifyOtaspChanged(otaspState);

                reevaluateDataConnections();
                setupDataOnConnectableApns(Phone.REASON_DATA_ENABLED);
            }
        }
    }

    private void onSimNotReady() {
        if (DBG) log("onSimNotReady");

        cleanUpAllConnections(true, Phone.REASON_SIM_NOT_READY);
        mAllApnSettings = null;
        mAutoAttachOnCreationConfig = false;
        // Clear auto attach as modem is expected to do a new attach once SIM is ready
        mAutoAttachOnCreation.set(false);
    }

    private void onSetDependencyMet(String apnType, boolean met) {
        // don't allow users to tweak hipri to work around default dependency not met
        if (PhoneConstants.APN_TYPE_HIPRI.equals(apnType)) return;

        ApnContext apnContext = mApnContexts.get(apnType);
        if (apnContext == null) {
            loge("onSetDependencyMet: ApnContext not found in onSetDependencyMet(" +
                    apnType + ", " + met + ")");
            return;
        }
        applyNewState(apnContext, apnContext.isEnabled(), met);
        if (PhoneConstants.APN_TYPE_DEFAULT.equals(apnType)) {
            // tie actions on default to similar actions on HIPRI regarding dependencyMet
            apnContext = mApnContexts.get(PhoneConstants.APN_TYPE_HIPRI);
            if (apnContext != null) applyNewState(apnContext, apnContext.isEnabled(), met);
        }
    }

    public void setPolicyDataEnabled(boolean enabled) {
        if (DBG) log("setPolicyDataEnabled: " + enabled);
        Message msg = obtainMessage(DctConstants.CMD_SET_POLICY_DATA_ENABLE);
        msg.arg1 = (enabled ? DctConstants.ENABLED : DctConstants.DISABLED);
        sendMessage(msg);
    }

    private void onSetPolicyDataEnabled(boolean enabled) {
        final boolean prevEnabled = isDataEnabled();
        if (mDataEnabledSettings.isPolicyDataEnabled() != enabled) {
            mDataEnabledSettings.setPolicyDataEnabled(enabled);
            // TODO: We should register for DataEnabledSetting's data enabled/disabled event and
            // handle the rest from there.
            if (prevEnabled != isDataEnabled()) {
                if (!prevEnabled) {
                    reevaluateDataConnections();
                    onTrySetupData(Phone.REASON_DATA_ENABLED);
                } else {
                    onCleanUpAllConnections(Phone.REASON_DATA_SPECIFIC_DISABLED);
                }
            }
        }
    }

    private void applyNewState(ApnContext apnContext, boolean enabled, boolean met) {
        boolean cleanup = false;
        boolean trySetup = false;
        String str ="applyNewState(" + apnContext.getApnType() + ", " + enabled +
                "(" + apnContext.isEnabled() + "), " + met + "(" +
                apnContext.getDependencyMet() +"))";
        if (DBG) log(str);
        apnContext.requestLog(str);

        if (apnContext.isReady()) {
            cleanup = true;
            if (enabled && met) {
                DctConstants.State state = apnContext.getState();
                switch(state) {
                    case CONNECTING:
                    case CONNECTED:
                    case DISCONNECTING:
                        // We're "READY" and active so just return
                        if (DBG) log("applyNewState: 'ready' so return");
                        apnContext.requestLog("applyNewState state=" + state + ", so return");
                        return;
                    case IDLE:
                        // fall through: this is unexpected but if it happens cleanup and try setup
                    case FAILED:
                    case SCANNING:
                    case RETRYING: {
                        // We're "READY" but not active so disconnect (cleanup = true) and
                        // connect (trySetup = true) to be sure we retry the connection.
                        trySetup = true;
                        apnContext.setReason(Phone.REASON_DATA_ENABLED);
                        break;
                    }
                }
            } else if (met) {
                apnContext.setReason(Phone.REASON_DATA_DISABLED);
                // If ConnectivityService has disabled this network, stop trying to bring
                // it up, but do not tear it down - ConnectivityService will do that
                // directly by talking with the DataConnection.
                //
                // This doesn't apply to DUN, however.  Those connections have special
                // requirements from carriers and we need stop using them when the dun
                // request goes away.  This applies to both CDMA and GSM because they both
                // can declare the DUN APN sharable by default traffic, thus still satisfying
                // those requests and not torn down organically.
                if ((apnContext.getApnType() == PhoneConstants.APN_TYPE_DUN && teardownForDun())
                        || apnContext.getState() != DctConstants.State.CONNECTED) {
                    str = "Clean up the connection. Apn type = " + apnContext.getApnType()
                            + ", state = " + apnContext.getState();
                    if (DBG) log(str);
                    apnContext.requestLog(str);
                    cleanup = true;
                } else {
                    cleanup = false;
                }
            } else {
                apnContext.setReason(Phone.REASON_DATA_DEPENDENCY_UNMET);
            }
        } else {
            if (enabled && met) {
                if (apnContext.isEnabled()) {
                    apnContext.setReason(Phone.REASON_DATA_DEPENDENCY_MET);
                } else {
                    apnContext.setReason(Phone.REASON_DATA_ENABLED);
                }
                if (apnContext.getState() == DctConstants.State.FAILED) {
                    apnContext.setState(DctConstants.State.IDLE);
                }
                trySetup = true;
            }
        }
        apnContext.setEnabled(enabled);
        apnContext.setDependencyMet(met);
        if (cleanup) cleanUpConnection(true, apnContext);
        if (trySetup) {
            apnContext.resetErrorCodeRetries();
            trySetupData(apnContext);
        }
    }

    private DcAsyncChannel checkForCompatibleConnectedApnContext(ApnContext apnContext) {
        String apnType = apnContext.getApnType();
        ArrayList<ApnSetting> dunSettings = null;

        if (PhoneConstants.APN_TYPE_DUN.equals(apnType)) {
            dunSettings = sortApnListByPreferred(fetchDunApns());
        }
        if (DBG) {
            log("checkForCompatibleConnectedApnContext: apnContext=" + apnContext );
        }

        DcAsyncChannel potentialDcac = null;
        ApnContext potentialApnCtx = null;
        for (ApnContext curApnCtx : mApnContexts.values()) {
            DcAsyncChannel curDcac = curApnCtx.getDcAc();
            if (curDcac != null) {
                ApnSetting apnSetting = curApnCtx.getApnSetting();
                log("apnSetting: " + apnSetting);
                if (dunSettings != null && dunSettings.size() > 0) {
                    for (ApnSetting dunSetting : dunSettings) {
                        if (dunSetting.equals(apnSetting)) {
                            switch (curApnCtx.getState()) {
                                case CONNECTED:
                                    if (DBG) {
                                        log("checkForCompatibleConnectedApnContext:"
                                                + " found dun conn=" + curDcac
                                                + " curApnCtx=" + curApnCtx);
                                    }
                                    return curDcac;
                                case RETRYING:
                                case CONNECTING:
                                    potentialDcac = curDcac;
                                    potentialApnCtx = curApnCtx;
                                    break;
                                default:
                                    // Not connected, potential unchanged
                                    break;
                            }
                        }
                    }
                } else if (apnSetting != null && apnSetting.canHandleType(apnType)) {
                    switch (curApnCtx.getState()) {
                        case CONNECTED:
                            if (DBG) {
                                log("checkForCompatibleConnectedApnContext:"
                                        + " found canHandle conn=" + curDcac
                                        + " curApnCtx=" + curApnCtx);
                            }
                            return curDcac;
                        case RETRYING:
                        case CONNECTING:
                            potentialDcac = curDcac;
                            potentialApnCtx = curApnCtx;
                            break;
                        default:
                            // Not connected, potential unchanged
                            break;
                    }
                }
            } else {
                if (VDBG) {
                    log("checkForCompatibleConnectedApnContext: not conn curApnCtx=" + curApnCtx);
                }
            }
        }
        if (potentialDcac != null) {
            if (DBG) {
                log("checkForCompatibleConnectedApnContext: found potential conn=" + potentialDcac
                        + " curApnCtx=" + potentialApnCtx);
            }
            return potentialDcac;
        }

        if (DBG) log("checkForCompatibleConnectedApnContext: NO conn apnContext=" + apnContext);
        return null;
    }

    public void setEnabled(int id, boolean enable) {
        Message msg = obtainMessage(DctConstants.EVENT_ENABLE_NEW_APN);
        msg.arg1 = id;
        msg.arg2 = (enable ? DctConstants.ENABLED : DctConstants.DISABLED);
        sendMessage(msg);
    }

    private void onEnableApn(int apnId, int enabled) {
        ApnContext apnContext = mApnContextsById.get(apnId);
        if (apnContext == null) {
            loge("onEnableApn(" + apnId + ", " + enabled + "): NO ApnContext");
            return;
        }
        // TODO change our retry manager to use the appropriate numbers for the new APN
        if (DBG) log("onEnableApn: apnContext=" + apnContext + " call applyNewState");
        applyNewState(apnContext, enabled == DctConstants.ENABLED, apnContext.getDependencyMet());

        if ((enabled == DctConstants.DISABLED) &&
            isOnlySingleDcAllowed(mPhone.getServiceState().getRilDataRadioTechnology()) &&
            !isHigherPriorityApnContextActive(apnContext)) {

            if(DBG) log("onEnableApn: isOnlySingleDcAllowed true & higher priority APN disabled");
            // If the highest priority APN is disabled and only single
            // data call is allowed, try to setup data call on other connectable APN.
            setupDataOnConnectableApns(Phone.REASON_SINGLE_PDN_ARBITRATION);
        }
    }

    // TODO: We shouldnt need this.
    private boolean onTrySetupData(String reason) {
        if (DBG) log("onTrySetupData: reason=" + reason);
        setupDataOnConnectableApns(reason);
        return true;
    }

    private boolean onTrySetupData(ApnContext apnContext) {
        if (DBG) log("onTrySetupData: apnContext=" + apnContext);
        return trySetupData(apnContext);
    }

    /**
     * Whether data is enabled by user. Unlike isDataEnabled, this only
     * checks user setting stored in {@link android.provider.Settings.Global#MOBILE_DATA}
     * if not provisioning, or isProvisioningDataEnabled if provisioning.
     */
    public boolean isUserDataEnabled() {
        if (mDataEnabledSettings.isProvisioning()) {
            return mDataEnabledSettings.isProvisioningDataEnabled();
        } else {
            return mDataEnabledSettings.isUserDataEnabled();
        }
    }

    /**
     * Modify {@link android.provider.Settings.Global#DATA_ROAMING} value for user modification only
     */
    public void setDataRoamingEnabledByUser(boolean enabled) {
        final int phoneSubId = mPhone.getSubId();
        if (getDataRoamingEnabled() != enabled) {
            int roaming = enabled ? 1 : 0;

            // For single SIM phones, this is a per phone property.
            if (TelephonyManager.getDefault().getSimCount() == 1) {
                Settings.Global.putInt(mResolver, Settings.Global.DATA_ROAMING, roaming);
                setDataRoamingFromUserAction(true);
            } else {
                Settings.Global.putInt(mResolver, Settings.Global.DATA_ROAMING +
                         phoneSubId, roaming);
            }

            mSubscriptionManager.setDataRoaming(roaming, phoneSubId);
            // will trigger handleDataOnRoamingChange() through observer
            if (DBG) {
                log("setDataRoamingEnabledByUser: set phoneSubId=" + phoneSubId
                        + " isRoaming=" + enabled);
            }
        } else {
            if (DBG) {
                log("setDataRoamingEnabledByUser: unchanged phoneSubId=" + phoneSubId
                        + " isRoaming=" + enabled);
             }
        }
    }

    /**
     * Return current {@link android.provider.Settings.Global#DATA_ROAMING} value.
     */
    public boolean getDataRoamingEnabled() {
        boolean isDataRoamingEnabled;
        final int phoneSubId = mPhone.getSubId();

        // For single SIM phones, this is a per phone property.
        if (TelephonyManager.getDefault().getSimCount() == 1) {
            isDataRoamingEnabled = Settings.Global.getInt(mResolver,
                    Settings.Global.DATA_ROAMING,
                    getDefaultDataRoamingEnabled() ? 1 : 0) != 0;
        } else {
            isDataRoamingEnabled = Settings.Global.getInt(mResolver,
                    Settings.Global.DATA_ROAMING + phoneSubId,
                    getDefaultDataRoamingEnabled() ? 1 : 0) != 0;
        }

        if (VDBG) {
            log("getDataRoamingEnabled: phoneSubId=" + phoneSubId
                    + " isDataRoamingEnabled=" + isDataRoamingEnabled);
        }
        return isDataRoamingEnabled;
    }

    /**
     * get default values for {@link Settings.Global#DATA_ROAMING}
     * return {@code true} if either
     * {@link CarrierConfigManager#KEY_CARRIER_DEFAULT_DATA_ROAMING_ENABLED_BOOL} or
     * system property ro.com.android.dataroaming is set to true. otherwise return {@code false}
     */
    private boolean getDefaultDataRoamingEnabled() {
        final CarrierConfigManager configMgr = (CarrierConfigManager)
                mPhone.getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        boolean isDataRoamingEnabled = "true".equalsIgnoreCase(SystemProperties.get(
                "ro.com.android.dataroaming", "false"));
        isDataRoamingEnabled |= configMgr.getConfigForSubId(mPhone.getSubId()).getBoolean(
                CarrierConfigManager.KEY_CARRIER_DEFAULT_DATA_ROAMING_ENABLED_BOOL);
        return isDataRoamingEnabled;
    }

    /**
     * Set default value for {@link android.provider.Settings.Global#DATA_ROAMING}
     * if the setting is not from user actions. default value is based on carrier config and system
     * properties.
     */
    private void setDefaultDataRoamingEnabled() {
        // For single SIM phones, this is a per phone property.
        String setting = Settings.Global.DATA_ROAMING;
        boolean useCarrierSpecificDefault = false;
        if (TelephonyManager.getDefault().getSimCount() != 1) {
            setting = setting + mPhone.getSubId();
            try {
                Settings.Global.getInt(mResolver, setting);
            } catch (SettingNotFoundException ex) {
                // For msim, update to carrier default if uninitialized.
                useCarrierSpecificDefault = true;
            }
        } else if (!isDataRoamingFromUserAction()) {
            // for single sim device, update to carrier default if user action is not set
            useCarrierSpecificDefault = true;
        }
        if (useCarrierSpecificDefault) {
            boolean defaultVal = getDefaultDataRoamingEnabled();
            log("setDefaultDataRoamingEnabled: " + setting + "default value: " + defaultVal);
            Settings.Global.putInt(mResolver, setting, defaultVal ? 1 : 0);
            mSubscriptionManager.setDataRoaming(defaultVal ? 1 : 0, mPhone.getSubId());
        }
    }

    private boolean isDataRoamingFromUserAction() {
        final SharedPreferences sp = PreferenceManager
                .getDefaultSharedPreferences(mPhone.getContext());
        // since we don't want to unset user preference from system update, pass true as the default
        // value if shared pref does not exist and set shared pref to false explicitly from factory
        // reset.
        if (!sp.contains(Phone.DATA_ROAMING_IS_USER_SETTING_KEY)
                && Settings.Global.getInt(mResolver, Settings.Global.DEVICE_PROVISIONED, 0) == 0) {
            sp.edit().putBoolean(Phone.DATA_ROAMING_IS_USER_SETTING_KEY, false).commit();
        }
        return sp.getBoolean(Phone.DATA_ROAMING_IS_USER_SETTING_KEY, true);
    }

    private void setDataRoamingFromUserAction(boolean isUserAction) {
        final SharedPreferences.Editor sp = PreferenceManager
                .getDefaultSharedPreferences(mPhone.getContext()).edit();
        sp.putBoolean(Phone.DATA_ROAMING_IS_USER_SETTING_KEY, isUserAction).commit();
    }

    // When the data roaming status changes from roaming to non-roaming.
    private void onDataRoamingOff() {
        if (DBG) log("onDataRoamingOff");

        if (!getDataRoamingEnabled()) {
            // TODO: Remove this once all old vendor RILs are gone. We don't need to set initial apn
            // attach and send the data profile again as the modem should have both roaming and
            // non-roaming protocol in place. Modem should choose the right protocol based on the
            // roaming condition.
            setInitialAttachApn();
            setDataProfilesAsNeeded();

            // If the user did not enable data roaming, now when we transit from roaming to
            // non-roaming, we should try to reestablish the data connection.

            notifyOffApnsOfAvailability(Phone.REASON_ROAMING_OFF);
            setupDataOnConnectableApns(Phone.REASON_ROAMING_OFF);
        } else {
            notifyDataConnection(Phone.REASON_ROAMING_OFF);
        }
    }

    // This method is called
    // 1. When the data roaming status changes from non-roaming to roaming.
    // 2. When allowed data roaming settings is changed by the user.
    private void onDataRoamingOnOrSettingsChanged(int messageType) {
        if (DBG) log("onDataRoamingOnOrSettingsChanged");
        // Used to differentiate data roaming turned on vs settings changed.
        boolean settingChanged = (messageType == DctConstants.EVENT_ROAMING_SETTING_CHANGE);

        // Check if the device is actually data roaming
        if (!mPhone.getServiceState().getDataRoaming()) {
            if (DBG) log("device is not roaming. ignored the request.");
            return;
        }

        checkDataRoamingStatus(settingChanged);

        if (getDataRoamingEnabled()) {
            if (DBG) log("onDataRoamingOnOrSettingsChanged: setup data on roaming");

            setupDataOnConnectableApns(Phone.REASON_ROAMING_ON);
            notifyDataConnection(Phone.REASON_ROAMING_ON);
        } else {
            // If the user does not turn on data roaming, when we transit from non-roaming to
            // roaming, we need to tear down the data connection otherwise the user might be
            // charged for data roaming usage.
            if (DBG) log("onDataRoamingOnOrSettingsChanged: Tear down data connection on roaming.");
            cleanUpAllConnections(true, Phone.REASON_ROAMING_ON);
            notifyOffApnsOfAvailability(Phone.REASON_ROAMING_ON);
        }
    }

    // We want to track possible roaming data leakage. Which is, if roaming setting
    // is disabled, yet we still setup a roaming data connection or have a connected ApnContext
    // switched to roaming. When this happens, we log it in a local log.
    private void checkDataRoamingStatus(boolean settingChanged) {
        if (!settingChanged && !getDataRoamingEnabled()
                && mPhone.getServiceState().getDataRoaming()) {
            for (ApnContext apnContext : mApnContexts.values()) {
                if (apnContext.getState() == DctConstants.State.CONNECTED) {
                    mDataRoamingLeakageLog.log("PossibleRoamingLeakage "
                            + " connection params: " + (apnContext.getDcAc() != null
                            ? apnContext.getDcAc().mLastConnectionParams : ""));
                }
            }
        }
    }

    private void onRadioAvailable() {
        if (DBG) log("onRadioAvailable");
        if (mPhone.getSimulatedRadioControl() != null) {
            // Assume data is connected on the simulator
            // FIXME  this can be improved
            // setState(DctConstants.State.CONNECTED);
            notifyDataConnection(null);

            log("onRadioAvailable: We're on the simulator; assuming data is connected");
        }

        IccRecords r = mIccRecords.get();
        if (r != null && r.getRecordsLoaded()) {
            notifyOffApnsOfAvailability(null);
        }

        if (getOverallState() != DctConstants.State.IDLE) {
            cleanUpConnection(true, null);
        }
    }

    private void onRadioOffOrNotAvailable() {
        // Make sure our reconnect delay starts at the initial value
        // next time the radio comes on

        mReregisterOnReconnectFailure = false;

        // Clear auto attach as modem is expected to do a new attach
        mAutoAttachOnCreation.set(false);

        if (mPhone.getSimulatedRadioControl() != null) {
            // Assume data is connected on the simulator
            // FIXME  this can be improved
            log("We're on the simulator; assuming radio off is meaningless");
        } else {
            if (DBG) log("onRadioOffOrNotAvailable: is off and clean up all connections");
            cleanUpAllConnections(false, Phone.REASON_RADIO_TURNED_OFF);
        }
        notifyOffApnsOfAvailability(null);
    }

    private void completeConnection(ApnContext apnContext) {

        if (DBG) log("completeConnection: successful, notify the world apnContext=" + apnContext);

        if (mIsProvisioning && !TextUtils.isEmpty(mProvisioningUrl)) {
            if (DBG) {
                log("completeConnection: MOBILE_PROVISIONING_ACTION url="
                        + mProvisioningUrl);
            }
            Intent newIntent = Intent.makeMainSelectorActivity(Intent.ACTION_MAIN,
                    Intent.CATEGORY_APP_BROWSER);
            newIntent.setData(Uri.parse(mProvisioningUrl));
            newIntent.setFlags(Intent.FLAG_ACTIVITY_BROUGHT_TO_FRONT |
                    Intent.FLAG_ACTIVITY_NEW_TASK);
            try {
                mPhone.getContext().startActivity(newIntent);
            } catch (ActivityNotFoundException e) {
                loge("completeConnection: startActivityAsUser failed" + e);
            }
        }
        mIsProvisioning = false;
        mProvisioningUrl = null;
        if (mProvisioningSpinner != null) {
            sendMessage(obtainMessage(DctConstants.CMD_CLEAR_PROVISIONING_SPINNER,
                    mProvisioningSpinner));
        }

        mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());
        startNetStatPoll();
        startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
    }

    /**
     * A SETUP (aka bringUp) has completed, possibly with an error. If
     * there is an error this method will call {@link #onDataSetupCompleteError}.
     */
    private void onDataSetupComplete(AsyncResult ar) {

        DcFailCause cause = DcFailCause.UNKNOWN;
        boolean handleError = false;
        ApnContext apnContext = getValidApnContext(ar, "onDataSetupComplete");

        if (apnContext == null) return;

        if (ar.exception == null) {
            DcAsyncChannel dcac = apnContext.getDcAc();

            if (RADIO_TESTS) {
                // Note: To change radio.test.onDSC.null.dcac from command line you need to
                // adb root and adb remount and from the command line you can only change the
                // value to 1 once. To change it a second time you can reboot or execute
                // adb shell stop and then adb shell start. The command line to set the value is:
                // adb shell sqlite3 /data/data/com.android.providers.settings/databases/settings.db "insert into system (name,value) values ('radio.test.onDSC.null.dcac', '1');"
                ContentResolver cr = mPhone.getContext().getContentResolver();
                String radioTestProperty = "radio.test.onDSC.null.dcac";
                if (Settings.System.getInt(cr, radioTestProperty, 0) == 1) {
                    log("onDataSetupComplete: " + radioTestProperty +
                            " is true, set dcac to null and reset property to false");
                    dcac = null;
                    Settings.System.putInt(cr, radioTestProperty, 0);
                    log("onDataSetupComplete: " + radioTestProperty + "=" +
                            Settings.System.getInt(mPhone.getContext().getContentResolver(),
                                    radioTestProperty, -1));
                }
            }
            if (dcac == null) {
                log("onDataSetupComplete: no connection to DC, handle as error");
                cause = DcFailCause.CONNECTION_TO_DATACONNECTIONAC_BROKEN;
                handleError = true;
            } else {
                ApnSetting apn = apnContext.getApnSetting();
                if (DBG) {
                    log("onDataSetupComplete: success apn=" + (apn == null ? "unknown" : apn.apn));
                }
                if (apn != null && apn.proxy != null && apn.proxy.length() != 0) {
                    try {
                        String port = apn.port;
                        if (TextUtils.isEmpty(port)) port = "8080";
                        ProxyInfo proxy = new ProxyInfo(apn.proxy,
                                Integer.parseInt(port), null);
                        dcac.setLinkPropertiesHttpProxySync(proxy);
                    } catch (NumberFormatException e) {
                        loge("onDataSetupComplete: NumberFormatException making ProxyProperties (" +
                                apn.port + "): " + e);
                    }
                }

                // everything is setup
                if (TextUtils.equals(apnContext.getApnType(), PhoneConstants.APN_TYPE_DEFAULT)) {
                    try {
                        SystemProperties.set(PUPPET_MASTER_RADIO_STRESS_TEST, "true");
                    } catch (RuntimeException ex) {
                        log("Failed to set PUPPET_MASTER_RADIO_STRESS_TEST to true");
                    }
                    if (mCanSetPreferApn && mPreferredApn == null) {
                        if (DBG) log("onDataSetupComplete: PREFERRED APN is null");
                        mPreferredApn = apn;
                        if (mPreferredApn != null) {
                            setPreferredApn(mPreferredApn.id);
                        }
                    }
                } else {
                    try {
                        SystemProperties.set(PUPPET_MASTER_RADIO_STRESS_TEST, "false");
                    } catch (RuntimeException ex) {
                        log("Failed to set PUPPET_MASTER_RADIO_STRESS_TEST to false");
                    }
                }

                // A connection is setup
                apnContext.setState(DctConstants.State.CONNECTED);

                checkDataRoamingStatus(false);

                boolean isProvApn = apnContext.isProvisioningApn();
                final ConnectivityManager cm = ConnectivityManager.from(mPhone.getContext());
                if (mProvisionBroadcastReceiver != null) {
                    mPhone.getContext().unregisterReceiver(mProvisionBroadcastReceiver);
                    mProvisionBroadcastReceiver = null;
                }
                if ((!isProvApn) || mIsProvisioning) {
                    // Hide any provisioning notification.
                    cm.setProvisioningNotificationVisible(false, ConnectivityManager.TYPE_MOBILE,
                            mProvisionActionName);
                    // Complete the connection normally notifying the world we're connected.
                    // We do this if this isn't a special provisioning apn or if we've been
                    // told its time to provision.
                    completeConnection(apnContext);
                } else {
                    // This is a provisioning APN that we're reporting as connected. Later
                    // when the user desires to upgrade this to a "default" connection,
                    // mIsProvisioning == true, we'll go through the code path above.
                    // mIsProvisioning becomes true when CMD_ENABLE_MOBILE_PROVISIONING
                    // is sent to the DCT.
                    if (DBG) {
                        log("onDataSetupComplete: successful, BUT send connected to prov apn as"
                                + " mIsProvisioning:" + mIsProvisioning + " == false"
                                + " && (isProvisioningApn:" + isProvApn + " == true");
                    }

                    // While radio is up, grab provisioning URL.  The URL contains ICCID which
                    // disappears when radio is off.
                    mProvisionBroadcastReceiver = new ProvisionNotificationBroadcastReceiver(
                            cm.getMobileProvisioningUrl(),
                            TelephonyManager.getDefault().getNetworkOperatorName());
                    mPhone.getContext().registerReceiver(mProvisionBroadcastReceiver,
                            new IntentFilter(mProvisionActionName));
                    // Put up user notification that sign-in is required.
                    cm.setProvisioningNotificationVisible(true, ConnectivityManager.TYPE_MOBILE,
                            mProvisionActionName);
                    // Turn off radio to save battery and avoid wasting carrier resources.
                    // The network isn't usable and network validation will just fail anyhow.
                    setRadio(false);
                }
                if (DBG) {
                    log("onDataSetupComplete: SETUP complete type=" + apnContext.getApnType()
                        + ", reason:" + apnContext.getReason());
                }
                if (Build.IS_DEBUGGABLE) {
                    // adb shell setprop persist.radio.test.pco [pco_val]
                    String radioTestProperty = "persist.radio.test.pco";
                    int pcoVal = SystemProperties.getInt(radioTestProperty, -1);
                    if (pcoVal != -1) {
                        log("PCO testing: read pco value from persist.radio.test.pco " + pcoVal);
                        final byte[] value = new byte[1];
                        value[0] = (byte) pcoVal;
                        final Intent intent =
                                new Intent(TelephonyIntents.ACTION_CARRIER_SIGNAL_PCO_VALUE);
                        intent.putExtra(TelephonyIntents.EXTRA_APN_TYPE_KEY, "default");
                        intent.putExtra(TelephonyIntents.EXTRA_APN_PROTO_KEY, "IPV4V6");
                        intent.putExtra(TelephonyIntents.EXTRA_PCO_ID_KEY, 0xFF00);
                        intent.putExtra(TelephonyIntents.EXTRA_PCO_VALUE_KEY, value);
                        mPhone.getCarrierSignalAgent().notifyCarrierSignalReceivers(intent);
                    }
                }
            }
        } else {
            cause = (DcFailCause) (ar.result);
            if (DBG) {
                ApnSetting apn = apnContext.getApnSetting();
                log(String.format("onDataSetupComplete: error apn=%s cause=%s",
                        (apn == null ? "unknown" : apn.apn), cause));
            }
            if (cause.isEventLoggable()) {
                // Log this failure to the Event Logs.
                int cid = getCellLocationId();
                EventLog.writeEvent(EventLogTags.PDP_SETUP_FAIL,
                        cause.ordinal(), cid, TelephonyManager.getDefault().getNetworkType());
            }
            ApnSetting apn = apnContext.getApnSetting();
            mPhone.notifyPreciseDataConnectionFailed(apnContext.getReason(),
                    apnContext.getApnType(), apn != null ? apn.apn : "unknown", cause.toString());

            // Compose broadcast intent send to the specific carrier signaling receivers
            Intent intent = new Intent(TelephonyIntents
                    .ACTION_CARRIER_SIGNAL_REQUEST_NETWORK_FAILED);
            intent.putExtra(TelephonyIntents.EXTRA_ERROR_CODE_KEY, cause.getErrorCode());
            intent.putExtra(TelephonyIntents.EXTRA_APN_TYPE_KEY, apnContext.getApnType());
            mPhone.getCarrierSignalAgent().notifyCarrierSignalReceivers(intent);

            if (cause.isRestartRadioFail(mPhone.getContext(), mPhone.getSubId()) ||
                    apnContext.restartOnError(cause.getErrorCode())) {
                if (DBG) log("Modem restarted.");
                sendRestartRadio();
            }

            // If the data call failure cause is a permanent failure, we mark the APN as permanent
            // failed.
            if (isPermanentFailure(cause)) {
                log("cause = " + cause + ", mark apn as permanent failed. apn = " + apn);
                apnContext.markApnPermanentFailed(apn);
            }

            handleError = true;
        }

        if (handleError) {
            onDataSetupCompleteError(ar);
        }

        /* If flag is set to false after SETUP_DATA_CALL is invoked, we need
         * to clean data connections.
         */
        if (!mDataEnabledSettings.isInternalDataEnabled()) {
            cleanUpAllConnections(Phone.REASON_DATA_DISABLED);
        }

    }

    /**
     * check for obsolete messages.  Return ApnContext if valid, null if not
     */
    private ApnContext getValidApnContext(AsyncResult ar, String logString) {
        if (ar != null && ar.userObj instanceof Pair) {
            Pair<ApnContext, Integer>pair = (Pair<ApnContext, Integer>)ar.userObj;
            ApnContext apnContext = pair.first;
            if (apnContext != null) {
                final int generation = apnContext.getConnectionGeneration();
                if (DBG) {
                    log("getValidApnContext (" + logString + ") on " + apnContext + " got " +
                            generation + " vs " + pair.second);
                }
                if (generation == pair.second) {
                    return apnContext;
                } else {
                    log("ignoring obsolete " + logString);
                    return null;
                }
            }
        }
        throw new RuntimeException(logString + ": No apnContext");
    }

    /**
     * Error has occurred during the SETUP {aka bringUP} request and the DCT
     * should either try the next waiting APN or start over from the
     * beginning if the list is empty. Between each SETUP request there will
     * be a delay defined by {@link #getApnDelay()}.
     */
    private void onDataSetupCompleteError(AsyncResult ar) {

        ApnContext apnContext = getValidApnContext(ar, "onDataSetupCompleteError");

        if (apnContext == null) return;

        long delay = apnContext.getDelayForNextApn(mFailFast);

        // Check if we need to retry or not.
        if (delay >= 0) {
            if (DBG) log("onDataSetupCompleteError: Try next APN. delay = " + delay);
            apnContext.setState(DctConstants.State.SCANNING);
            // Wait a bit before trying the next APN, so that
            // we're not tying up the RIL command channel
            startAlarmForReconnect(delay, apnContext);
        } else {
            // If we are not going to retry any APN, set this APN context to failed state.
            // This would be the final state of a data connection.
            apnContext.setState(DctConstants.State.FAILED);
            mPhone.notifyDataConnection(Phone.REASON_APN_FAILED, apnContext.getApnType());
            apnContext.setDataConnectionAc(null);
            log("onDataSetupCompleteError: Stop retrying APNs.");
        }
    }

    /**
     * Called when EVENT_REDIRECTION_DETECTED is received.
     */
    private void onDataConnectionRedirected(String redirectUrl) {
        if (!TextUtils.isEmpty(redirectUrl)) {
            Intent intent = new Intent(TelephonyIntents.ACTION_CARRIER_SIGNAL_REDIRECTED);
            intent.putExtra(TelephonyIntents.EXTRA_REDIRECTION_URL_KEY, redirectUrl);
            mPhone.getCarrierSignalAgent().notifyCarrierSignalReceivers(intent);
            log("Notify carrier signal receivers with redirectUrl: " + redirectUrl);
        }
    }

    /**
     * Called when EVENT_DISCONNECT_DONE is received.
     */
    private void onDisconnectDone(AsyncResult ar) {
        ApnContext apnContext = getValidApnContext(ar, "onDisconnectDone");
        if (apnContext == null) return;

        if(DBG) log("onDisconnectDone: EVENT_DISCONNECT_DONE apnContext=" + apnContext);
        apnContext.setState(DctConstants.State.IDLE);

        mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());

        // if all data connection are gone, check whether Airplane mode request was
        // pending.
        if (isDisconnected()) {
            if (mPhone.getServiceStateTracker().processPendingRadioPowerOffAfterDataOff()) {
                if (DBG) log("onDisconnectDone: radio will be turned off, no retries");
                // Radio will be turned off. No need to retry data setup
                apnContext.setApnSetting(null);
                apnContext.setDataConnectionAc(null);

                // Need to notify disconnect as well, in the case of switching Airplane mode.
                // Otherwise, it would cause 30s delayed to turn on Airplane mode.
                if (mDisconnectPendingCount > 0) {
                    mDisconnectPendingCount--;
                }

                if (mDisconnectPendingCount == 0) {
                    notifyDataDisconnectComplete();
                    notifyAllDataDisconnected();
                }
                return;
            }
        }
        // If APN is still enabled, try to bring it back up automatically
        if (mAttached.get() && apnContext.isReady() && retryAfterDisconnected(apnContext)) {
            try {
                SystemProperties.set(PUPPET_MASTER_RADIO_STRESS_TEST, "false");
            } catch (RuntimeException ex) {
                log("Failed to set PUPPET_MASTER_RADIO_STRESS_TEST to false");
            }
            // Wait a bit before trying the next APN, so that
            // we're not tying up the RIL command channel.
            // This also helps in any external dependency to turn off the context.
            if (DBG) log("onDisconnectDone: attached, ready and retry after disconnect");
            long delay = apnContext.getRetryAfterDisconnectDelay();
            if (delay > 0) {
                // Data connection is in IDLE state, so when we reconnect later, we'll rebuild
                // the waiting APN list, which will also reset/reconfigure the retry manager.
                startAlarmForReconnect(delay, apnContext);
            }
        } else {
            boolean restartRadioAfterProvisioning = mPhone.getContext().getResources().getBoolean(
                    com.android.internal.R.bool.config_restartRadioAfterProvisioning);

            if (apnContext.isProvisioningApn() && restartRadioAfterProvisioning) {
                log("onDisconnectDone: restartRadio after provisioning");
                restartRadio();
            }
            apnContext.setApnSetting(null);
            apnContext.setDataConnectionAc(null);
            if (isOnlySingleDcAllowed(mPhone.getServiceState().getRilDataRadioTechnology())) {
                if(DBG) log("onDisconnectDone: isOnlySigneDcAllowed true so setup single apn");
                setupDataOnConnectableApns(Phone.REASON_SINGLE_PDN_ARBITRATION);
            } else {
                if(DBG) log("onDisconnectDone: not retrying");
            }
        }

        if (mDisconnectPendingCount > 0)
            mDisconnectPendingCount--;

        if (mDisconnectPendingCount == 0) {
            apnContext.setConcurrentVoiceAndDataAllowed(
                    mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed());
            notifyDataDisconnectComplete();
            notifyAllDataDisconnected();
        }

    }

    /**
     * Called when EVENT_DISCONNECT_DC_RETRYING is received.
     */
    private void onDisconnectDcRetrying(AsyncResult ar) {
        // We could just do this in DC!!!
        ApnContext apnContext = getValidApnContext(ar, "onDisconnectDcRetrying");
        if (apnContext == null) return;

        apnContext.setState(DctConstants.State.RETRYING);
        if(DBG) log("onDisconnectDcRetrying: apnContext=" + apnContext);

        mPhone.notifyDataConnection(apnContext.getReason(), apnContext.getApnType());
    }

    private void onVoiceCallStarted() {
        if (DBG) log("onVoiceCallStarted");
        mInVoiceCall = true;
        if (isConnected() && ! mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed()) {
            if (DBG) log("onVoiceCallStarted stop polling");
            stopNetStatPoll();
            stopDataStallAlarm();
            notifyDataConnection(Phone.REASON_VOICE_CALL_STARTED);
        }
    }

    private void onVoiceCallEnded() {
        if (DBG) log("onVoiceCallEnded");
        mInVoiceCall = false;
        if (isConnected()) {
            if (!mPhone.getServiceStateTracker().isConcurrentVoiceAndDataAllowed()) {
                startNetStatPoll();
                startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
                notifyDataConnection(Phone.REASON_VOICE_CALL_ENDED);
            } else {
                // clean slate after call end.
                resetPollStats();
            }
        }
        // reset reconnect timer
        setupDataOnConnectableApns(Phone.REASON_VOICE_CALL_ENDED);
    }

    private void onCleanUpConnection(boolean tearDown, int apnId, String reason) {
        if (DBG) log("onCleanUpConnection");
        ApnContext apnContext = mApnContextsById.get(apnId);
        if (apnContext != null) {
            apnContext.setReason(reason);
            cleanUpConnection(tearDown, apnContext);
        }
    }

    private boolean isConnected() {
        for (ApnContext apnContext : mApnContexts.values()) {
            if (apnContext.getState() == DctConstants.State.CONNECTED) {
                // At least one context is connected, return true
                return true;
            }
        }
        // There are not any contexts connected, return false
        return false;
    }

    public boolean isDisconnected() {
        for (ApnContext apnContext : mApnContexts.values()) {
            if (!apnContext.isDisconnected()) {
                // At least one context was not disconnected return false
                return false;
            }
        }
        // All contexts were disconnected so return true
        return true;
    }

    private void notifyDataConnection(String reason) {
        if (DBG) log("notifyDataConnection: reason=" + reason);
        for (ApnContext apnContext : mApnContexts.values()) {
            if (mAttached.get() && apnContext.isReady()) {
                if (DBG) log("notifyDataConnection: type:" + apnContext.getApnType());
                mPhone.notifyDataConnection(reason != null ? reason : apnContext.getReason(),
                        apnContext.getApnType());
            }
        }
        notifyOffApnsOfAvailability(reason);
    }

    private void setDataProfilesAsNeeded() {
        if (DBG) log("setDataProfilesAsNeeded");
        if (mAllApnSettings != null && !mAllApnSettings.isEmpty()) {
            ArrayList<DataProfile> dps = new ArrayList<DataProfile>();
            for (ApnSetting apn : mAllApnSettings) {
                if (apn.modemCognitive) {
                    DataProfile dp = createDataProfile(apn);
                    if (!dps.contains(dp)) {
                        dps.add(dp);
                    }
                }
            }
            if (dps.size() > 0) {
                mDataServiceManager.setDataProfile(dps,
                        mPhone.getServiceState().getDataRoamingFromRegistration(), null);
            }
        }
    }

    /**
     * Based on the sim operator numeric, create a list for all possible
     * Data Connections and setup the preferredApn.
     */
    private void createAllApnList() {
        mMvnoMatched = false;
        mAllApnSettings = new ArrayList<>();
        IccRecords r = mIccRecords.get();
        String operator = (r != null) ? r.getOperatorNumeric() : "";
        if (operator != null) {
            String selection = Telephony.Carriers.NUMERIC + " = '" + operator + "'";
            // query only enabled apn.
            // carrier_enabled : 1 means enabled apn, 0 disabled apn.
            // selection += " and carrier_enabled = 1";
            if (DBG) log("createAllApnList: selection=" + selection);

            // ORDER BY Telephony.Carriers._ID ("_id")
            Cursor cursor = mPhone.getContext().getContentResolver().query(
                    Uri.withAppendedPath(Telephony.Carriers.CONTENT_URI, "filtered"),
                    null, selection, null, Telephony.Carriers._ID);

            if (cursor != null) {
                if (cursor.getCount() > 0) {
                    mAllApnSettings = createApnList(cursor);
                }
                cursor.close();
            }
        }

        addEmergencyApnSetting();

        dedupeApnSettings();

        if (mAllApnSettings.isEmpty()) {
            if (DBG) log("createAllApnList: No APN found for carrier: " + operator);
            mPreferredApn = null;
            // TODO: What is the right behavior?
            //notifyNoData(DataConnection.FailCause.MISSING_UNKNOWN_APN);
        } else {
            mPreferredApn = getPreferredApn();
            if (mPreferredApn != null && !mPreferredApn.numeric.equals(operator)) {
                mPreferredApn = null;
                setPreferredApn(-1);
            }
            if (DBG) log("createAllApnList: mPreferredApn=" + mPreferredApn);
        }
        if (DBG) log("createAllApnList: X mAllApnSettings=" + mAllApnSettings);

        setDataProfilesAsNeeded();
    }

    private void dedupeApnSettings() {
        ArrayList<ApnSetting> resultApns = new ArrayList<ApnSetting>();

        // coalesce APNs if they are similar enough to prevent
        // us from bringing up two data calls with the same interface
        int i = 0;
        while (i < mAllApnSettings.size() - 1) {
            ApnSetting first = mAllApnSettings.get(i);
            ApnSetting second = null;
            int j = i + 1;
            while (j < mAllApnSettings.size()) {
                second = mAllApnSettings.get(j);
                if (first.similar(second)) {
                    ApnSetting newApn = mergeApns(first, second);
                    mAllApnSettings.set(i, newApn);
                    first = newApn;
                    mAllApnSettings.remove(j);
                } else {
                    j++;
                }
            }
            i++;
        }
    }

    private ApnSetting mergeApns(ApnSetting dest, ApnSetting src) {
        int id = dest.id;
        ArrayList<String> resultTypes = new ArrayList<String>();
        resultTypes.addAll(Arrays.asList(dest.types));
        for (String srcType : src.types) {
            if (resultTypes.contains(srcType) == false) resultTypes.add(srcType);
            if (srcType.equals(PhoneConstants.APN_TYPE_DEFAULT)) id = src.id;
        }
        String mmsc = (TextUtils.isEmpty(dest.mmsc) ? src.mmsc : dest.mmsc);
        String mmsProxy = (TextUtils.isEmpty(dest.mmsProxy) ? src.mmsProxy : dest.mmsProxy);
        String mmsPort = (TextUtils.isEmpty(dest.mmsPort) ? src.mmsPort : dest.mmsPort);
        String proxy = (TextUtils.isEmpty(dest.proxy) ? src.proxy : dest.proxy);
        String port = (TextUtils.isEmpty(dest.port) ? src.port : dest.port);
        String protocol = src.protocol.equals("IPV4V6") ? src.protocol : dest.protocol;
        String roamingProtocol = src.roamingProtocol.equals("IPV4V6") ? src.roamingProtocol :
                dest.roamingProtocol;
        int networkTypeBitmask = (dest.networkTypeBitmask == 0 || src.networkTypeBitmask == 0)
                ? 0 : (dest.networkTypeBitmask | src.networkTypeBitmask);
        if (networkTypeBitmask == 0) {
            int bearerBitmask = (dest.bearerBitmask == 0 || src.bearerBitmask == 0)
                    ? 0 : (dest.bearerBitmask | src.bearerBitmask);
            networkTypeBitmask = ServiceState.convertBearerBitmaskToNetworkTypeBitmask(
                    bearerBitmask);
        }

        return new ApnSetting(id, dest.numeric, dest.carrier, dest.apn,
                proxy, port, mmsc, mmsProxy, mmsPort, dest.user, dest.password,
                dest.authType, resultTypes.toArray(new String[0]), protocol,
                roamingProtocol, dest.carrierEnabled, networkTypeBitmask, dest.profileId,
                (dest.modemCognitive || src.modemCognitive), dest.maxConns, dest.waitTime,
                dest.maxConnsTime, dest.mtu, dest.mvnoType, dest.mvnoMatchData, dest.apnSetId);
    }

    /** Return the DC AsyncChannel for the new data connection */
    private DcAsyncChannel createDataConnection() {
        if (DBG) log("createDataConnection E");

        int id = mUniqueIdGenerator.getAndIncrement();
        DataConnection conn = DataConnection.makeDataConnection(mPhone, id, this,
                mDataServiceManager, mDcTesterFailBringUpAll, mDcc);
        mDataConnections.put(id, conn);
        DcAsyncChannel dcac = new DcAsyncChannel(conn, LOG_TAG);
        int status = dcac.fullyConnectSync(mPhone.getContext(), this, conn.getHandler());
        if (status == AsyncChannel.STATUS_SUCCESSFUL) {
            mDataConnectionAcHashMap.put(dcac.getDataConnectionIdSync(), dcac);
        } else {
            loge("createDataConnection: Could not connect to dcac=" + dcac + " status=" + status);
        }

        if (DBG) log("createDataConnection() X id=" + id + " dc=" + conn);
        return dcac;
    }

    private void destroyDataConnections() {
        if(mDataConnections != null) {
            if (DBG) log("destroyDataConnections: clear mDataConnectionList");
            mDataConnections.clear();
        } else {
            if (DBG) log("destroyDataConnections: mDataConnecitonList is empty, ignore");
        }
    }

    /**
     * Build a list of APNs to be used to create PDP's.
     *
     * @param requestedApnType
     * @return waitingApns list to be used to create PDP
     *          error when waitingApns.isEmpty()
     */
    private ArrayList<ApnSetting> buildWaitingApns(String requestedApnType, int radioTech) {
        if (DBG) log("buildWaitingApns: E requestedApnType=" + requestedApnType);
        ArrayList<ApnSetting> apnList = new ArrayList<ApnSetting>();

        if (requestedApnType.equals(PhoneConstants.APN_TYPE_DUN)) {
            ArrayList<ApnSetting> dunApns = fetchDunApns();
            if (dunApns.size() > 0) {
                for (ApnSetting dun : dunApns) {
                    apnList.add(dun);
                    if (DBG) log("buildWaitingApns: X added APN_TYPE_DUN apnList=" + apnList);
                }
                return sortApnListByPreferred(apnList);
            }
        }

        IccRecords r = mIccRecords.get();
        String operator = (r != null) ? r.getOperatorNumeric() : "";

        // This is a workaround for a bug (7305641) where we don't failover to other
        // suitable APNs if our preferred APN fails.  On prepaid ATT sims we need to
        // failover to a provisioning APN, but once we've used their default data
        // connection we are locked to it for life.  This change allows ATT devices
        // to say they don't want to use preferred at all.
        boolean usePreferred = true;
        try {
            usePreferred = ! mPhone.getContext().getResources().getBoolean(com.android.
                    internal.R.bool.config_dontPreferApn);
        } catch (Resources.NotFoundException e) {
            if (DBG) log("buildWaitingApns: usePreferred NotFoundException set to true");
            usePreferred = true;
        }
        if (usePreferred) {
            mPreferredApn = getPreferredApn();
        }
        if (DBG) {
            log("buildWaitingApns: usePreferred=" + usePreferred
                    + " canSetPreferApn=" + mCanSetPreferApn
                    + " mPreferredApn=" + mPreferredApn
                    + " operator=" + operator + " radioTech=" + radioTech
                    + " IccRecords r=" + r);
        }

        if (usePreferred && mCanSetPreferApn && mPreferredApn != null &&
                mPreferredApn.canHandleType(requestedApnType)) {
            if (DBG) {
                log("buildWaitingApns: Preferred APN:" + operator + ":"
                        + mPreferredApn.numeric + ":" + mPreferredApn);
            }
            if (mPreferredApn.numeric.equals(operator)) {
                if (ServiceState.bitmaskHasTech(mPreferredApn.networkTypeBitmask,
                        ServiceState.rilRadioTechnologyToNetworkType(radioTech))) {
                    apnList.add(mPreferredApn);
                    apnList = sortApnListByPreferred(apnList);
                    if (DBG) log("buildWaitingApns: X added preferred apnList=" + apnList);
                    return apnList;
                } else {
                    if (DBG) log("buildWaitingApns: no preferred APN");
                    setPreferredApn(-1);
                    mPreferredApn = null;
                }
            } else {
                if (DBG) log("buildWaitingApns: no preferred APN");
                setPreferredApn(-1);
                mPreferredApn = null;
            }
        }
        if (mAllApnSettings != null) {
            if (DBG) log("buildWaitingApns: mAllApnSettings=" + mAllApnSettings);
            for (ApnSetting apn : mAllApnSettings) {
                if (apn.canHandleType(requestedApnType)) {
                    if (ServiceState.bitmaskHasTech(apn.networkTypeBitmask,
                            ServiceState.rilRadioTechnologyToNetworkType(radioTech))) {
                        if (DBG) log("buildWaitingApns: adding apn=" + apn);
                        apnList.add(apn);
                    } else {
                        if (DBG) {
                            log("buildWaitingApns: bearerBitmask:" + apn.bearerBitmask
                                    + " or " + "networkTypeBitmask:" + apn.networkTypeBitmask
                                    + "do not include radioTech:" + radioTech);
                        }
                    }
                } else if (DBG) {
                    log("buildWaitingApns: couldn't handle requested ApnType="
                            + requestedApnType);
                }
            }
        } else {
            loge("mAllApnSettings is null!");
        }

        apnList = sortApnListByPreferred(apnList);
        if (DBG) log("buildWaitingApns: " + apnList.size() + " APNs in the list: " + apnList);
        return apnList;
    }

    /**
     * Sort a list of ApnSetting objects, with the preferred APNs at the front of the list
     *
     * e.g. if the preferred APN set = 2 and we have
     *   1. APN with apn_set_id = 0 = Carriers.NO_SET_SET (no set is set)
     *   2. APN with apn_set_id = 1 (not preferred set)
     *   3. APN with apn_set_id = 2 (preferred set)
     * Then the return order should be (3, 1, 2) or (3, 2, 1)
     *
     * e.g. if the preferred APN set = Carriers.NO_SET_SET (no preferred set) then the
     * return order can be anything
     */
    @VisibleForTesting
    public ArrayList<ApnSetting> sortApnListByPreferred(ArrayList<ApnSetting> list) {
        if (list == null || list.size() <= 1) return list;
        int preferredApnSetId = getPreferredApnSetId();
        if (preferredApnSetId != Telephony.Carriers.NO_SET_SET) {
            list.sort(new Comparator<ApnSetting>() {
                @Override
                public int compare(ApnSetting apn1, ApnSetting apn2) {
                    if (apn1.apnSetId == preferredApnSetId) return -1;
                    if (apn2.apnSetId == preferredApnSetId) return 1;
                    return 0;
                }
            });
        }
        return list;
    }

    private String apnListToString (ArrayList<ApnSetting> apns) {
        StringBuilder result = new StringBuilder();
        for (int i = 0, size = apns.size(); i < size; i++) {
            result.append('[')
                  .append(apns.get(i).toString())
                  .append(']');
        }
        return result.toString();
    }

    private void setPreferredApn(int pos) {
        if (!mCanSetPreferApn) {
            log("setPreferredApn: X !canSEtPreferApn");
            return;
        }

        String subId = Long.toString(mPhone.getSubId());
        Uri uri = Uri.withAppendedPath(PREFERAPN_NO_UPDATE_URI_USING_SUBID, subId);
        log("setPreferredApn: delete");
        ContentResolver resolver = mPhone.getContext().getContentResolver();
        resolver.delete(uri, null, null);

        if (pos >= 0) {
            log("setPreferredApn: insert");
            ContentValues values = new ContentValues();
            values.put(APN_ID, pos);
            resolver.insert(uri, values);
        }
    }

    private ApnSetting getPreferredApn() {
        if (mAllApnSettings == null || mAllApnSettings.isEmpty()) {
            log("getPreferredApn: mAllApnSettings is " + ((mAllApnSettings == null)?"null":"empty"));
            return null;
        }

        String subId = Long.toString(mPhone.getSubId());
        Uri uri = Uri.withAppendedPath(PREFERAPN_NO_UPDATE_URI_USING_SUBID, subId);
        Cursor cursor = mPhone.getContext().getContentResolver().query(
                uri, new String[] { "_id", "name", "apn" },
                null, null, Telephony.Carriers.DEFAULT_SORT_ORDER);

        if (cursor != null) {
            mCanSetPreferApn = true;
        } else {
            mCanSetPreferApn = false;
        }
        log("getPreferredApn: mRequestedApnType=" + mRequestedApnType + " cursor=" + cursor
                + " cursor.count=" + ((cursor != null) ? cursor.getCount() : 0));

        if (mCanSetPreferApn && cursor.getCount() > 0) {
            int pos;
            cursor.moveToFirst();
            pos = cursor.getInt(cursor.getColumnIndexOrThrow(Telephony.Carriers._ID));
            for(ApnSetting p : mAllApnSettings) {
                log("getPreferredApn: apnSetting=" + p);
                if (p.id == pos && p.canHandleType(mRequestedApnType)) {
                    log("getPreferredApn: X found apnSetting" + p);
                    cursor.close();
                    return p;
                }
            }
        }

        if (cursor != null) {
            cursor.close();
        }

        log("getPreferredApn: X not found");
        return null;
    }

    @Override
    public void handleMessage (Message msg) {
        if (VDBG) log("handleMessage msg=" + msg);

        switch (msg.what) {
            case DctConstants.EVENT_RECORDS_LOADED:
                // If onRecordsLoadedOrSubIdChanged() is not called here, it should be called on
                // onSubscriptionsChanged() when a valid subId is available.
                int subId = mPhone.getSubId();
                if (SubscriptionManager.isValidSubscriptionId(subId)) {
                    onRecordsLoadedOrSubIdChanged();
                } else {
                    log("Ignoring EVENT_RECORDS_LOADED as subId is not valid: " + subId);
                }
                break;

            case DctConstants.EVENT_DATA_CONNECTION_DETACHED:
                onDataConnectionDetached();
                break;

            case DctConstants.EVENT_DATA_CONNECTION_ATTACHED:
                onDataConnectionAttached();
                break;

            case DctConstants.EVENT_DO_RECOVERY:
                doRecovery();
                break;

            case DctConstants.EVENT_APN_CHANGED:
                onApnChanged();
                break;

            case DctConstants.EVENT_PS_RESTRICT_ENABLED:
                /**
                 * We don't need to explicitly to tear down the PDP context
                 * when PS restricted is enabled. The base band will deactive
                 * PDP context and notify us with PDP_CONTEXT_CHANGED.
                 * But we should stop the network polling and prevent reset PDP.
                 */
                if (DBG) log("EVENT_PS_RESTRICT_ENABLED " + mIsPsRestricted);
                stopNetStatPoll();
                stopDataStallAlarm();
                mIsPsRestricted = true;
                break;

            case DctConstants.EVENT_PS_RESTRICT_DISABLED:
                /**
                 * When PS restrict is removed, we need setup PDP connection if
                 * PDP connection is down.
                 */
                if (DBG) log("EVENT_PS_RESTRICT_DISABLED " + mIsPsRestricted);
                mIsPsRestricted  = false;
                if (isConnected()) {
                    startNetStatPoll();
                    startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
                } else {
                    // TODO: Should all PDN states be checked to fail?
                    if (mState == DctConstants.State.FAILED) {
                        cleanUpAllConnections(false, Phone.REASON_PS_RESTRICT_ENABLED);
                        mReregisterOnReconnectFailure = false;
                    }
                    ApnContext apnContext = mApnContextsById.get(DctConstants.APN_DEFAULT_ID);
                    if (apnContext != null) {
                        apnContext.setReason(Phone.REASON_PS_RESTRICT_ENABLED);
                        trySetupData(apnContext);
                    } else {
                        loge("**** Default ApnContext not found ****");
                        if (Build.IS_DEBUGGABLE) {
                            throw new RuntimeException("Default ApnContext not found");
                        }
                    }
                }
                break;

            case DctConstants.EVENT_TRY_SETUP_DATA:
                if (msg.obj instanceof ApnContext) {
                    onTrySetupData((ApnContext)msg.obj);
                } else if (msg.obj instanceof String) {
                    onTrySetupData((String)msg.obj);
                } else {
                    loge("EVENT_TRY_SETUP request w/o apnContext or String");
                }
                break;

            case DctConstants.EVENT_CLEAN_UP_CONNECTION:
                boolean tearDown = (msg.arg1 == 0) ? false : true;
                if (DBG) log("EVENT_CLEAN_UP_CONNECTION tearDown=" + tearDown);
                if (msg.obj instanceof ApnContext) {
                    cleanUpConnection(tearDown, (ApnContext)msg.obj);
                } else {
                    onCleanUpConnection(tearDown, msg.arg2, (String) msg.obj);
                }
                break;
            case DctConstants.EVENT_SET_INTERNAL_DATA_ENABLE: {
                final boolean enabled = (msg.arg1 == DctConstants.ENABLED) ? true : false;
                onSetInternalDataEnabled(enabled, (Message) msg.obj);
                break;
            }
            case DctConstants.EVENT_CLEAN_UP_ALL_CONNECTIONS:
                if ((msg.obj != null) && (msg.obj instanceof String == false)) {
                    msg.obj = null;
                }
                onCleanUpAllConnections((String) msg.obj);
                break;

            case DctConstants.EVENT_DATA_RAT_CHANGED:
                if (mPhone.getServiceState().getRilDataRadioTechnology()
                        == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
                    // unknown rat is an exception for data rat change. It's only received when out
                    // of service and is not applicable for apn bearer bitmask. We should bypass the
                    // check of waiting apn list and keep the data connection on, and no need to
                    // setup a new one.
                    break;
                }
                cleanUpConnectionsOnUpdatedApns(false, Phone.REASON_NW_TYPE_CHANGED);
                //May new Network allow setupData, so try it here
                setupDataOnConnectableApns(Phone.REASON_NW_TYPE_CHANGED,
                        RetryFailures.ONLY_ON_CHANGE);
                break;

            case DctConstants.CMD_CLEAR_PROVISIONING_SPINNER:
                // Check message sender intended to clear the current spinner.
                if (mProvisioningSpinner == msg.obj) {
                    mProvisioningSpinner.dismiss();
                    mProvisioningSpinner = null;
                }
                break;
            case AsyncChannel.CMD_CHANNEL_DISCONNECTED: {
                log("DISCONNECTED_CONNECTED: msg=" + msg);
                DcAsyncChannel dcac = (DcAsyncChannel) msg.obj;
                mDataConnectionAcHashMap.remove(dcac.getDataConnectionIdSync());
                dcac.disconnected();
                break;
            }
            case DctConstants.EVENT_ENABLE_NEW_APN:
                onEnableApn(msg.arg1, msg.arg2);
                break;

            case DctConstants.EVENT_DATA_STALL_ALARM:
                onDataStallAlarm(msg.arg1);
                break;

            case DctConstants.EVENT_ROAMING_OFF:
                onDataRoamingOff();
                break;

            case DctConstants.EVENT_ROAMING_ON:
            case DctConstants.EVENT_ROAMING_SETTING_CHANGE:
                onDataRoamingOnOrSettingsChanged(msg.what);
                break;

            case DctConstants.EVENT_DEVICE_PROVISIONED_CHANGE:
                onDeviceProvisionedChange();
                break;

            case DctConstants.EVENT_REDIRECTION_DETECTED:
                String url = (String) msg.obj;
                log("dataConnectionTracker.handleMessage: EVENT_REDIRECTION_DETECTED=" + url);
                onDataConnectionRedirected(url);

            case DctConstants.EVENT_RADIO_AVAILABLE:
                onRadioAvailable();
                break;

            case DctConstants.EVENT_RADIO_OFF_OR_NOT_AVAILABLE:
                onRadioOffOrNotAvailable();
                break;

            case DctConstants.EVENT_DATA_SETUP_COMPLETE:
                onDataSetupComplete((AsyncResult) msg.obj);
                break;

            case DctConstants.EVENT_DATA_SETUP_COMPLETE_ERROR:
                onDataSetupCompleteError((AsyncResult) msg.obj);
                break;

            case DctConstants.EVENT_DISCONNECT_DONE:
                log("DataConnectionTracker.handleMessage: EVENT_DISCONNECT_DONE msg=" + msg);
                onDisconnectDone((AsyncResult) msg.obj);
                break;

            case DctConstants.EVENT_DISCONNECT_DC_RETRYING:
                log("DataConnectionTracker.handleMessage: EVENT_DISCONNECT_DC_RETRYING msg=" + msg);
                onDisconnectDcRetrying((AsyncResult) msg.obj);
                break;

            case DctConstants.EVENT_VOICE_CALL_STARTED:
                onVoiceCallStarted();
                break;

            case DctConstants.EVENT_VOICE_CALL_ENDED:
                onVoiceCallEnded();
                break;
            case DctConstants.CMD_SET_USER_DATA_ENABLE: {
                final boolean enabled = (msg.arg1 == DctConstants.ENABLED) ? true : false;
                if (DBG) log("CMD_SET_USER_DATA_ENABLE enabled=" + enabled);
                onSetUserDataEnabled(enabled);
                break;
            }
            // TODO - remove
            case DctConstants.CMD_SET_DEPENDENCY_MET: {
                boolean met = (msg.arg1 == DctConstants.ENABLED) ? true : false;
                if (DBG) log("CMD_SET_DEPENDENCY_MET met=" + met);
                Bundle bundle = msg.getData();
                if (bundle != null) {
                    String apnType = (String)bundle.get(DctConstants.APN_TYPE_KEY);
                    if (apnType != null) {
                        onSetDependencyMet(apnType, met);
                    }
                }
                break;
            }
            case DctConstants.CMD_SET_POLICY_DATA_ENABLE: {
                final boolean enabled = (msg.arg1 == DctConstants.ENABLED) ? true : false;
                onSetPolicyDataEnabled(enabled);
                break;
            }
            case DctConstants.CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: {
                sEnableFailFastRefCounter += (msg.arg1 == DctConstants.ENABLED) ? 1 : -1;
                if (DBG) {
                    log("CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: "
                            + " sEnableFailFastRefCounter=" + sEnableFailFastRefCounter);
                }
                if (sEnableFailFastRefCounter < 0) {
                    final String s = "CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: "
                            + "sEnableFailFastRefCounter:" + sEnableFailFastRefCounter + " < 0";
                    loge(s);
                    sEnableFailFastRefCounter = 0;
                }
                final boolean enabled = sEnableFailFastRefCounter > 0;
                if (DBG) {
                    log("CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: enabled=" + enabled
                            + " sEnableFailFastRefCounter=" + sEnableFailFastRefCounter);
                }
                if (mFailFast != enabled) {
                    mFailFast = enabled;

                    mDataStallDetectionEnabled = !enabled;
                    if (mDataStallDetectionEnabled
                            && (getOverallState() == DctConstants.State.CONNECTED)
                            && (!mInVoiceCall ||
                                    mPhone.getServiceStateTracker()
                                        .isConcurrentVoiceAndDataAllowed())) {
                        if (DBG) log("CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: start data stall");
                        stopDataStallAlarm();
                        startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
                    } else {
                        if (DBG) log("CMD_SET_ENABLE_FAIL_FAST_MOBILE_DATA: stop data stall");
                        stopDataStallAlarm();
                    }
                }

                break;
            }
            case DctConstants.CMD_ENABLE_MOBILE_PROVISIONING: {
                Bundle bundle = msg.getData();
                if (bundle != null) {
                    try {
                        mProvisioningUrl = (String)bundle.get(DctConstants.PROVISIONING_URL_KEY);
                    } catch(ClassCastException e) {
                        loge("CMD_ENABLE_MOBILE_PROVISIONING: provisioning url not a string" + e);
                        mProvisioningUrl = null;
                    }
                }
                if (TextUtils.isEmpty(mProvisioningUrl)) {
                    loge("CMD_ENABLE_MOBILE_PROVISIONING: provisioning url is empty, ignoring");
                    mIsProvisioning = false;
                    mProvisioningUrl = null;
                } else {
                    loge("CMD_ENABLE_MOBILE_PROVISIONING: provisioningUrl=" + mProvisioningUrl);
                    mIsProvisioning = true;
                    startProvisioningApnAlarm();
                }
                break;
            }
            case DctConstants.EVENT_PROVISIONING_APN_ALARM: {
                if (DBG) log("EVENT_PROVISIONING_APN_ALARM");
                ApnContext apnCtx = mApnContextsById.get(DctConstants.APN_DEFAULT_ID);
                if (apnCtx.isProvisioningApn() && apnCtx.isConnectedOrConnecting()) {
                    if (mProvisioningApnAlarmTag == msg.arg1) {
                        if (DBG) log("EVENT_PROVISIONING_APN_ALARM: Disconnecting");
                        mIsProvisioning = false;
                        mProvisioningUrl = null;
                        stopProvisioningApnAlarm();
                        sendCleanUpConnection(true, apnCtx);
                    } else {
                        if (DBG) {
                            log("EVENT_PROVISIONING_APN_ALARM: ignore stale tag,"
                                    + " mProvisioningApnAlarmTag:" + mProvisioningApnAlarmTag
                                    + " != arg1:" + msg.arg1);
                        }
                    }
                } else {
                    if (DBG) log("EVENT_PROVISIONING_APN_ALARM: Not connected ignore");
                }
                break;
            }
            case DctConstants.CMD_IS_PROVISIONING_APN: {
                if (DBG) log("CMD_IS_PROVISIONING_APN");
                boolean isProvApn;
                try {
                    String apnType = null;
                    Bundle bundle = msg.getData();
                    if (bundle != null) {
                        apnType = (String)bundle.get(DctConstants.APN_TYPE_KEY);
                    }
                    if (TextUtils.isEmpty(apnType)) {
                        loge("CMD_IS_PROVISIONING_APN: apnType is empty");
                        isProvApn = false;
                    } else {
                        isProvApn = isProvisioningApn(apnType);
                    }
                } catch (ClassCastException e) {
                    loge("CMD_IS_PROVISIONING_APN: NO provisioning url ignoring");
                    isProvApn = false;
                }
                if (DBG) log("CMD_IS_PROVISIONING_APN: ret=" + isProvApn);
                mReplyAc.replyToMessage(msg, DctConstants.CMD_IS_PROVISIONING_APN,
                        isProvApn ? DctConstants.ENABLED : DctConstants.DISABLED);
                break;
            }
            case DctConstants.EVENT_ICC_CHANGED: {
                onUpdateIcc();
                break;
            }
            case DctConstants.EVENT_RESTART_RADIO: {
                restartRadio();
                break;
            }
            case DctConstants.CMD_NET_STAT_POLL: {
                if (msg.arg1 == DctConstants.ENABLED) {
                    handleStartNetStatPoll((DctConstants.Activity)msg.obj);
                } else if (msg.arg1 == DctConstants.DISABLED) {
                    handleStopNetStatPoll((DctConstants.Activity)msg.obj);
                }
                break;
            }
            case DctConstants.EVENT_PCO_DATA_RECEIVED: {
                handlePcoData((AsyncResult)msg.obj);
                break;
            }
            case DctConstants.EVENT_SET_CARRIER_DATA_ENABLED:
                onSetCarrierDataEnabled((AsyncResult) msg.obj);
                break;
            case DctConstants.EVENT_DATA_RECONNECT:
                onDataReconnect(msg.getData());
                break;
            case DctConstants.EVENT_DATA_SERVICE_BINDING_CHANGED:
                onDataServiceBindingChanged((Boolean) ((AsyncResult) msg.obj).result);
            default:
                Rlog.e("DcTracker", "Unhandled event=" + msg);
                break;

        }
    }

    private int getApnProfileID(String apnType) {
        if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_IMS)) {
            return RILConstants.DATA_PROFILE_IMS;
        } else if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_FOTA)) {
            return RILConstants.DATA_PROFILE_FOTA;
        } else if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_CBS)) {
            return RILConstants.DATA_PROFILE_CBS;
        } else if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_IA)) {
            return RILConstants.DATA_PROFILE_DEFAULT; // DEFAULT for now
        } else if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_DUN)) {
            return RILConstants.DATA_PROFILE_TETHERED;
        } else {
            return RILConstants.DATA_PROFILE_DEFAULT;
        }
    }

    private int getCellLocationId() {
        int cid = -1;
        CellLocation loc = mPhone.getCellLocation();

        if (loc != null) {
            if (loc instanceof GsmCellLocation) {
                cid = ((GsmCellLocation)loc).getCid();
            } else if (loc instanceof CdmaCellLocation) {
                cid = ((CdmaCellLocation)loc).getBaseStationId();
            }
        }
        return cid;
    }

    private IccRecords getUiccRecords(int appFamily) {
        return mUiccController.getIccRecords(mPhone.getPhoneId(), appFamily);
    }


    private void onUpdateIcc() {
        if (mUiccController == null ) {
            return;
        }

        IccRecords newIccRecords = getUiccRecords(UiccController.APP_FAM_3GPP);

        IccRecords r = mIccRecords.get();
        if (r != newIccRecords) {
            if (r != null) {
                log("Removing stale icc objects.");
                r.unregisterForRecordsLoaded(this);
                mIccRecords.set(null);
            }
            if (newIccRecords != null) {
                if (SubscriptionManager.isValidSubscriptionId(mPhone.getSubId())) {
                    log("New records found.");
                    mIccRecords.set(newIccRecords);
                    newIccRecords.registerForRecordsLoaded(
                            this, DctConstants.EVENT_RECORDS_LOADED, null);
                }
            } else {
                onSimNotReady();
            }
        }
    }

    public void update() {
        log("update sub = " + mPhone.getSubId());
        log("update(): Active DDS, register for all events now!");
        onUpdateIcc();

        mAutoAttachOnCreation.set(false);

        ((GsmCdmaPhone)mPhone).updateCurrentCarrierInProvider();
    }

    public void cleanUpAllConnections(String cause) {
        cleanUpAllConnections(cause, null);
    }

    public void updateRecords() {
        onUpdateIcc();
    }

    public void cleanUpAllConnections(String cause, Message disconnectAllCompleteMsg) {
        log("cleanUpAllConnections");
        if (disconnectAllCompleteMsg != null) {
            mDisconnectAllCompleteMsgList.add(disconnectAllCompleteMsg);
        }

        Message msg = obtainMessage(DctConstants.EVENT_CLEAN_UP_ALL_CONNECTIONS);
        msg.obj = cause;
        sendMessage(msg);
    }

    private void notifyDataDisconnectComplete() {
        log("notifyDataDisconnectComplete");
        for (Message m: mDisconnectAllCompleteMsgList) {
            m.sendToTarget();
        }
        mDisconnectAllCompleteMsgList.clear();
    }


    private void notifyAllDataDisconnected() {
        sEnableFailFastRefCounter = 0;
        mFailFast = false;
        mAllDataDisconnectedRegistrants.notifyRegistrants();
    }

    public void registerForAllDataDisconnected(Handler h, int what, Object obj) {
        mAllDataDisconnectedRegistrants.addUnique(h, what, obj);

        if (isDisconnected()) {
            log("notify All Data Disconnected");
            notifyAllDataDisconnected();
        }
    }

    public void unregisterForAllDataDisconnected(Handler h) {
        mAllDataDisconnectedRegistrants.remove(h);
    }

    public void registerForDataEnabledChanged(Handler h, int what, Object obj) {
        mDataEnabledSettings.registerForDataEnabledChanged(h, what, obj);
    }

    public void unregisterForDataEnabledChanged(Handler h) {
        mDataEnabledSettings.unregisterForDataEnabledChanged(h);
    }

    private void onSetInternalDataEnabled(boolean enabled, Message onCompleteMsg) {
        if (DBG) log("onSetInternalDataEnabled: enabled=" + enabled);
        boolean sendOnComplete = true;

        mDataEnabledSettings.setInternalDataEnabled(enabled);
        if (enabled) {
            log("onSetInternalDataEnabled: changed to enabled, try to setup data call");
            onTrySetupData(Phone.REASON_DATA_ENABLED);
        } else {
            sendOnComplete = false;
            log("onSetInternalDataEnabled: changed to disabled, cleanUpAllConnections");
            cleanUpAllConnections(Phone.REASON_DATA_DISABLED, onCompleteMsg);
        }

        if (sendOnComplete) {
            if (onCompleteMsg != null) {
                onCompleteMsg.sendToTarget();
            }
        }
    }

    public boolean setInternalDataEnabled(boolean enable) {
        return setInternalDataEnabled(enable, null);
    }

    public boolean setInternalDataEnabled(boolean enable, Message onCompleteMsg) {
        if (DBG) log("setInternalDataEnabled(" + enable + ")");

        Message msg = obtainMessage(DctConstants.EVENT_SET_INTERNAL_DATA_ENABLE, onCompleteMsg);
        msg.arg1 = (enable ? DctConstants.ENABLED : DctConstants.DISABLED);
        sendMessage(msg);
        return true;
    }

    private void log(String s) {
        Rlog.d(LOG_TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }

    private void loge(String s) {
        Rlog.e(LOG_TAG, "[" + mPhone.getPhoneId() + "]" + s);
    }

    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        pw.println("DcTracker:");
        pw.println(" RADIO_TESTS=" + RADIO_TESTS);
        pw.println(" mDataEnabledSettings=" + mDataEnabledSettings);
        pw.println(" isDataAllowed=" + isDataAllowed(null));
        pw.flush();
        pw.println(" mRequestedApnType=" + mRequestedApnType);
        pw.println(" mPhone=" + mPhone.getPhoneName());
        pw.println(" mActivity=" + mActivity);
        pw.println(" mState=" + mState);
        pw.println(" mTxPkts=" + mTxPkts);
        pw.println(" mRxPkts=" + mRxPkts);
        pw.println(" mNetStatPollPeriod=" + mNetStatPollPeriod);
        pw.println(" mNetStatPollEnabled=" + mNetStatPollEnabled);
        pw.println(" mDataStallTxRxSum=" + mDataStallTxRxSum);
        pw.println(" mDataStallAlarmTag=" + mDataStallAlarmTag);
        pw.println(" mDataStallDetectionEnabled=" + mDataStallDetectionEnabled);
        pw.println(" mSentSinceLastRecv=" + mSentSinceLastRecv);
        pw.println(" mNoRecvPollCount=" + mNoRecvPollCount);
        pw.println(" mResolver=" + mResolver);
        pw.println(" mReconnectIntent=" + mReconnectIntent);
        pw.println(" mAutoAttachOnCreation=" + mAutoAttachOnCreation.get());
        pw.println(" mIsScreenOn=" + mIsScreenOn);
        pw.println(" mUniqueIdGenerator=" + mUniqueIdGenerator);
        pw.println(" mDataRoamingLeakageLog= ");
        mDataRoamingLeakageLog.dump(fd, pw, args);
        pw.flush();
        pw.println(" ***************************************");
        DcController dcc = mDcc;
        if (dcc != null) {
            dcc.dump(fd, pw, args);
        } else {
            pw.println(" mDcc=null");
        }
        pw.println(" ***************************************");
        HashMap<Integer, DataConnection> dcs = mDataConnections;
        if (dcs != null) {
            Set<Entry<Integer, DataConnection> > mDcSet = mDataConnections.entrySet();
            pw.println(" mDataConnections: count=" + mDcSet.size());
            for (Entry<Integer, DataConnection> entry : mDcSet) {
                pw.printf(" *** mDataConnection[%d] \n", entry.getKey());
                entry.getValue().dump(fd, pw, args);
            }
        } else {
            pw.println("mDataConnections=null");
        }
        pw.println(" ***************************************");
        pw.flush();
        HashMap<String, Integer> apnToDcId = mApnToDataConnectionId;
        if (apnToDcId != null) {
            Set<Entry<String, Integer>> apnToDcIdSet = apnToDcId.entrySet();
            pw.println(" mApnToDataConnectonId size=" + apnToDcIdSet.size());
            for (Entry<String, Integer> entry : apnToDcIdSet) {
                pw.printf(" mApnToDataConnectonId[%s]=%d\n", entry.getKey(), entry.getValue());
            }
        } else {
            pw.println("mApnToDataConnectionId=null");
        }
        pw.println(" ***************************************");
        pw.flush();
        ConcurrentHashMap<String, ApnContext> apnCtxs = mApnContexts;
        if (apnCtxs != null) {
            Set<Entry<String, ApnContext>> apnCtxsSet = apnCtxs.entrySet();
            pw.println(" mApnContexts size=" + apnCtxsSet.size());
            for (Entry<String, ApnContext> entry : apnCtxsSet) {
                entry.getValue().dump(fd, pw, args);
            }
            pw.println(" ***************************************");
        } else {
            pw.println(" mApnContexts=null");
        }
        pw.flush();
        ArrayList<ApnSetting> apnSettings = mAllApnSettings;
        if (apnSettings != null) {
            pw.println(" mAllApnSettings size=" + apnSettings.size());
            for (int i=0; i < apnSettings.size(); i++) {
                pw.printf(" mAllApnSettings[%d]: %s\n", i, apnSettings.get(i));
            }
            pw.flush();
        } else {
            pw.println(" mAllApnSettings=null");
        }
        pw.println(" mPreferredApn=" + mPreferredApn);
        pw.println(" mIsPsRestricted=" + mIsPsRestricted);
        pw.println(" mIsDisposed=" + mIsDisposed);
        pw.println(" mIntentReceiver=" + mIntentReceiver);
        pw.println(" mReregisterOnReconnectFailure=" + mReregisterOnReconnectFailure);
        pw.println(" canSetPreferApn=" + mCanSetPreferApn);
        pw.println(" mApnObserver=" + mApnObserver);
        pw.println(" getOverallState=" + getOverallState());
        pw.println(" mDataConnectionAsyncChannels=%s\n" + mDataConnectionAcHashMap);
        pw.println(" mAttached=" + mAttached.get());
        mDataEnabledSettings.dump(fd, pw, args);
        pw.flush();
    }

    public String[] getPcscfAddress(String apnType) {
        log("getPcscfAddress()");
        ApnContext apnContext = null;

        if(apnType == null){
            log("apnType is null, return null");
            return null;
        }

        if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_EMERGENCY)) {
            apnContext = mApnContextsById.get(DctConstants.APN_EMERGENCY_ID);
        } else if (TextUtils.equals(apnType, PhoneConstants.APN_TYPE_IMS)) {
            apnContext = mApnContextsById.get(DctConstants.APN_IMS_ID);
        } else {
            log("apnType is invalid, return null");
            return null;
        }

        if (apnContext == null) {
            log("apnContext is null, return null");
            return null;
        }

        DcAsyncChannel dcac = apnContext.getDcAc();
        String[] result = null;

        if (dcac != null) {
            result = dcac.getPcscfAddr();

            if (result != null) {
                for (int i = 0; i < result.length; i++) {
                    log("Pcscf[" + i + "]: " + result[i]);
                }
            }
            return result;
        }
        return null;
    }

    /**
     * Read APN configuration from Telephony.db for Emergency APN
     * All opertors recognize the connection request for EPDN based on APN type
     * PLMN name,APN name are not mandatory parameters
     */
    private void initEmergencyApnSetting() {
        // Operator Numeric is not available when sim records are not loaded.
        // Query Telephony.db with APN type as EPDN request does not
        // require APN name, plmn and all operators support same APN config.
        // DB will contain only one entry for Emergency APN
        String selection = "type=\"emergency\"";
        Cursor cursor = mPhone.getContext().getContentResolver().query(
                Uri.withAppendedPath(Telephony.Carriers.CONTENT_URI, "filtered"),
                null, selection, null, null);

        if (cursor != null) {
            if (cursor.getCount() > 0) {
                if (cursor.moveToFirst()) {
                    mEmergencyApn = makeApnSetting(cursor);
                }
            }
            cursor.close();
        }
    }

    /**
     * Add the Emergency APN settings to APN settings list
     */
    private void addEmergencyApnSetting() {
        if(mEmergencyApn != null) {
            if(mAllApnSettings == null) {
                mAllApnSettings = new ArrayList<ApnSetting>();
            } else {
                boolean hasEmergencyApn = false;
                for (ApnSetting apn : mAllApnSettings) {
                    if (ArrayUtils.contains(apn.types, PhoneConstants.APN_TYPE_EMERGENCY)) {
                        hasEmergencyApn = true;
                        break;
                    }
                }

                if(hasEmergencyApn == false) {
                    mAllApnSettings.add(mEmergencyApn);
                } else {
                    log("addEmergencyApnSetting - E-APN setting is already present");
                }
            }
        }
    }

    private boolean containsAllApns(ArrayList<ApnSetting> oldApnList,
                                    ArrayList<ApnSetting> newApnList) {
        for (ApnSetting newApnSetting : newApnList) {
            boolean canHandle = false;
            for (ApnSetting oldApnSetting : oldApnList) {
                // Make sure at least one of the APN from old list can cover the new APN
                if (oldApnSetting.equals(newApnSetting,
                        mPhone.getServiceState().getDataRoamingFromRegistration())) {
                    canHandle = true;
                    break;
                }
            }
            if (!canHandle) return false;
        }
        return true;
    }

    private void cleanUpConnectionsOnUpdatedApns(boolean tearDown, String reason) {
        if (DBG) log("cleanUpConnectionsOnUpdatedApns: tearDown=" + tearDown);
        if (mAllApnSettings != null && mAllApnSettings.isEmpty()) {
            cleanUpAllConnections(tearDown, Phone.REASON_APN_CHANGED);
        } else {
            int radioTech = mPhone.getServiceState().getRilDataRadioTechnology();
            if (radioTech == ServiceState.RIL_RADIO_TECHNOLOGY_UNKNOWN) {
                // unknown rat is an exception for data rat change. Its only received when out of
                // service and is not applicable for apn bearer bitmask. We should bypass the check
                // of waiting apn list and keep the data connection on.
                return;
            }
            for (ApnContext apnContext : mApnContexts.values()) {
                ArrayList<ApnSetting> currentWaitingApns = apnContext.getWaitingApns();
                ArrayList<ApnSetting> waitingApns = buildWaitingApns(
                        apnContext.getApnType(),
                        mPhone.getServiceState().getRilDataRadioTechnology());
                if (VDBG) log("new waitingApns:" + waitingApns);
                if ((currentWaitingApns != null)
                        && ((waitingApns.size() != currentWaitingApns.size())
                        // Check if the existing waiting APN list can cover the newly built APN
                        // list. If yes, then we don't need to tear down the existing data call.
                        // TODO: We probably need to rebuild APN list when roaming status changes.
                        || !containsAllApns(currentWaitingApns, waitingApns))) {
                    if (VDBG) log("new waiting apn is different for " + apnContext);
                    apnContext.setWaitingApns(waitingApns);
                    if (!apnContext.isDisconnected()) {
                        if (VDBG) log("cleanUpConnectionsOnUpdatedApns for " + apnContext);
                        apnContext.setReason(reason);
                        cleanUpConnection(true, apnContext);
                    }
                }
            }
        }

        if (!isConnected()) {
            stopNetStatPoll();
            stopDataStallAlarm();
        }

        mRequestedApnType = PhoneConstants.APN_TYPE_DEFAULT;

        if (DBG) log("mDisconnectPendingCount = " + mDisconnectPendingCount);
        if (tearDown && mDisconnectPendingCount == 0) {
            notifyDataDisconnectComplete();
            notifyAllDataDisconnected();
        }
    }

    /**
     * Polling stuff
     */
    private void resetPollStats() {
        mTxPkts = -1;
        mRxPkts = -1;
        mNetStatPollPeriod = POLL_NETSTAT_MILLIS;
    }

    private void startNetStatPoll() {
        if (getOverallState() == DctConstants.State.CONNECTED
                && mNetStatPollEnabled == false) {
            if (DBG) {
                log("startNetStatPoll");
            }
            resetPollStats();
            mNetStatPollEnabled = true;
            mPollNetStat.run();
        }
        if (mPhone != null) {
            mPhone.notifyDataActivity();
        }
    }

    private void stopNetStatPoll() {
        mNetStatPollEnabled = false;
        removeCallbacks(mPollNetStat);
        if (DBG) {
            log("stopNetStatPoll");
        }

        // To sync data activity icon in the case of switching data connection to send MMS.
        if (mPhone != null) {
            mPhone.notifyDataActivity();
        }
    }

    public void sendStartNetStatPoll(DctConstants.Activity activity) {
        Message msg = obtainMessage(DctConstants.CMD_NET_STAT_POLL);
        msg.arg1 = DctConstants.ENABLED;
        msg.obj = activity;
        sendMessage(msg);
    }

    private void handleStartNetStatPoll(DctConstants.Activity activity) {
        startNetStatPoll();
        startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
        setActivity(activity);
    }

    public void sendStopNetStatPoll(DctConstants.Activity activity) {
        Message msg = obtainMessage(DctConstants.CMD_NET_STAT_POLL);
        msg.arg1 = DctConstants.DISABLED;
        msg.obj = activity;
        sendMessage(msg);
    }

    private void handleStopNetStatPoll(DctConstants.Activity activity) {
        stopNetStatPoll();
        stopDataStallAlarm();
        setActivity(activity);
    }

    private void updateDataActivity() {
        long sent, received;

        DctConstants.Activity newActivity;

        TxRxSum preTxRxSum = new TxRxSum(mTxPkts, mRxPkts);
        TxRxSum curTxRxSum = new TxRxSum();
        curTxRxSum.updateTxRxSum();
        mTxPkts = curTxRxSum.txPkts;
        mRxPkts = curTxRxSum.rxPkts;

        if (VDBG) {
            log("updateDataActivity: curTxRxSum=" + curTxRxSum + " preTxRxSum=" + preTxRxSum);
        }

        if (mNetStatPollEnabled && (preTxRxSum.txPkts > 0 || preTxRxSum.rxPkts > 0)) {
            sent = mTxPkts - preTxRxSum.txPkts;
            received = mRxPkts - preTxRxSum.rxPkts;

            if (VDBG)
                log("updateDataActivity: sent=" + sent + " received=" + received);
            if (sent > 0 && received > 0) {
                newActivity = DctConstants.Activity.DATAINANDOUT;
            } else if (sent > 0 && received == 0) {
                newActivity = DctConstants.Activity.DATAOUT;
            } else if (sent == 0 && received > 0) {
                newActivity = DctConstants.Activity.DATAIN;
            } else {
                newActivity = (mActivity == DctConstants.Activity.DORMANT) ?
                        mActivity : DctConstants.Activity.NONE;
            }

            if (mActivity != newActivity && mIsScreenOn) {
                if (VDBG)
                    log("updateDataActivity: newActivity=" + newActivity);
                mActivity = newActivity;
                mPhone.notifyDataActivity();
            }
        }
    }

    private void handlePcoData(AsyncResult ar) {
        if (ar.exception != null) {
            Rlog.e(LOG_TAG, "PCO_DATA exception: " + ar.exception);
            return;
        }
        PcoData pcoData = (PcoData)(ar.result);
        ArrayList<DataConnection> dcList = new ArrayList<>();
        DataConnection temp = mDcc.getActiveDcByCid(pcoData.cid);
        if (temp != null) {
            dcList.add(temp);
        }
        if (dcList.size() == 0) {
            Rlog.e(LOG_TAG, "PCO_DATA for unknown cid: " + pcoData.cid + ", inferring");
            for (DataConnection dc : mDataConnections.values()) {
                final int cid = dc.getCid();
                if (cid == pcoData.cid) {
                    if (VDBG) Rlog.d(LOG_TAG, "  found " + dc);
                    dcList.clear();
                    dcList.add(dc);
                    break;
                }
                // check if this dc is still connecting
                if (cid == -1) {
                    for (ApnContext apnContext : dc.mApnContexts.keySet()) {
                        if (apnContext.getState() == DctConstants.State.CONNECTING) {
                            if (VDBG) Rlog.d(LOG_TAG, "  found potential " + dc);
                            dcList.add(dc);
                            break;
                        }
                    }
                }
            }
        }
        if (dcList.size() == 0) {
            Rlog.e(LOG_TAG, "PCO_DATA - couldn't infer cid");
            return;
        }
        for (DataConnection dc : dcList) {
            if (dc.mApnContexts.size() == 0) {
                break;
            }
            // send one out for each apn type in play
            for (ApnContext apnContext : dc.mApnContexts.keySet()) {
                String apnType = apnContext.getApnType();

                final Intent intent = new Intent(TelephonyIntents.ACTION_CARRIER_SIGNAL_PCO_VALUE);
                intent.putExtra(TelephonyIntents.EXTRA_APN_TYPE_KEY, apnType);
                intent.putExtra(TelephonyIntents.EXTRA_APN_PROTO_KEY, pcoData.bearerProto);
                intent.putExtra(TelephonyIntents.EXTRA_PCO_ID_KEY, pcoData.pcoId);
                intent.putExtra(TelephonyIntents.EXTRA_PCO_VALUE_KEY, pcoData.contents);
                mPhone.getCarrierSignalAgent().notifyCarrierSignalReceivers(intent);
            }
        }
    }

    /**
     * Data-Stall
     */
    // Recovery action taken in case of data stall
    private static class RecoveryAction {
        public static final int GET_DATA_CALL_LIST      = 0;
        public static final int CLEANUP                 = 1;
        public static final int REREGISTER              = 2;
        public static final int RADIO_RESTART           = 3;

        private static boolean isAggressiveRecovery(int value) {
            return ((value == RecoveryAction.CLEANUP) ||
                    (value == RecoveryAction.REREGISTER) ||
                    (value == RecoveryAction.RADIO_RESTART));
        }
    }

    private int getRecoveryAction() {
        int action = Settings.System.getInt(mResolver,
                "radio.data.stall.recovery.action", RecoveryAction.GET_DATA_CALL_LIST);
        if (VDBG_STALL) log("getRecoveryAction: " + action);
        return action;
    }

    private void putRecoveryAction(int action) {
        Settings.System.putInt(mResolver, "radio.data.stall.recovery.action", action);
        if (VDBG_STALL) log("putRecoveryAction: " + action);
    }

    private void broadcastDataStallDetected(int recoveryAction) {
        Intent intent = new Intent(TelephonyManager.ACTION_DATA_STALL_DETECTED);
        SubscriptionManager.putPhoneIdAndSubIdExtra(intent, mPhone.getPhoneId());
        intent.putExtra(TelephonyManager.EXTRA_RECOVERY_ACTION, recoveryAction);
        mPhone.getContext().sendBroadcast(intent, READ_PHONE_STATE);
    }

    private void doRecovery() {
        if (getOverallState() == DctConstants.State.CONNECTED) {
            // Go through a series of recovery steps, each action transitions to the next action
            final int recoveryAction = getRecoveryAction();
            TelephonyMetrics.getInstance().writeDataStallEvent(mPhone.getPhoneId(), recoveryAction);
            broadcastDataStallDetected(recoveryAction);

            switch (recoveryAction) {
                case RecoveryAction.GET_DATA_CALL_LIST:
                    EventLog.writeEvent(EventLogTags.DATA_STALL_RECOVERY_GET_DATA_CALL_LIST,
                            mSentSinceLastRecv);
                    if (DBG) log("doRecovery() get data call list");
                    mDataServiceManager.getDataCallList(obtainMessage());
                    putRecoveryAction(RecoveryAction.CLEANUP);
                    break;
                case RecoveryAction.CLEANUP:
                    EventLog.writeEvent(EventLogTags.DATA_STALL_RECOVERY_CLEANUP,
                            mSentSinceLastRecv);
                    if (DBG) log("doRecovery() cleanup all connections");
                    cleanUpAllConnections(Phone.REASON_PDP_RESET);
                    putRecoveryAction(RecoveryAction.REREGISTER);
                    break;
                case RecoveryAction.REREGISTER:
                    EventLog.writeEvent(EventLogTags.DATA_STALL_RECOVERY_REREGISTER,
                            mSentSinceLastRecv);
                    if (DBG) log("doRecovery() re-register");
                    mPhone.getServiceStateTracker().reRegisterNetwork(null);
                    putRecoveryAction(RecoveryAction.RADIO_RESTART);
                    break;
                case RecoveryAction.RADIO_RESTART:
                    EventLog.writeEvent(EventLogTags.DATA_STALL_RECOVERY_RADIO_RESTART,
                            mSentSinceLastRecv);
                    if (DBG) log("restarting radio");
                    restartRadio();
                    putRecoveryAction(RecoveryAction.GET_DATA_CALL_LIST);
                    break;
                default:
                    throw new RuntimeException("doRecovery: Invalid recoveryAction="
                            + recoveryAction);
            }
            mSentSinceLastRecv = 0;
        }
    }

    private void updateDataStallInfo() {
        long sent, received;

        TxRxSum preTxRxSum = new TxRxSum(mDataStallTxRxSum);
        mDataStallTxRxSum.updateTxRxSum();

        if (VDBG_STALL) {
            log("updateDataStallInfo: mDataStallTxRxSum=" + mDataStallTxRxSum +
                    " preTxRxSum=" + preTxRxSum);
        }

        sent = mDataStallTxRxSum.txPkts - preTxRxSum.txPkts;
        received = mDataStallTxRxSum.rxPkts - preTxRxSum.rxPkts;

        if (RADIO_TESTS) {
            if (SystemProperties.getBoolean("radio.test.data.stall", false)) {
                log("updateDataStallInfo: radio.test.data.stall true received = 0;");
                received = 0;
            }
        }
        if ( sent > 0 && received > 0 ) {
            if (VDBG_STALL) log("updateDataStallInfo: IN/OUT");
            mSentSinceLastRecv = 0;
            putRecoveryAction(RecoveryAction.GET_DATA_CALL_LIST);
        } else if (sent > 0 && received == 0) {
            if (isPhoneStateIdle()) {
                mSentSinceLastRecv += sent;
            } else {
                mSentSinceLastRecv = 0;
            }
            if (DBG) {
                log("updateDataStallInfo: OUT sent=" + sent +
                        " mSentSinceLastRecv=" + mSentSinceLastRecv);
            }
        } else if (sent == 0 && received > 0) {
            if (VDBG_STALL) log("updateDataStallInfo: IN");
            mSentSinceLastRecv = 0;
            putRecoveryAction(RecoveryAction.GET_DATA_CALL_LIST);
        } else {
            if (VDBG_STALL) log("updateDataStallInfo: NONE");
        }
    }

    private boolean isPhoneStateIdle() {
        for (int i = 0; i < TelephonyManager.getDefault().getPhoneCount(); i++) {
            Phone phone = PhoneFactory.getPhone(i);
            if (phone != null && phone.getState() != PhoneConstants.State.IDLE) {
                log("isPhoneStateIdle false: Voice call active on phone " + i);
                return false;
            }
        }
        return true;
    }

    private void onDataStallAlarm(int tag) {
        if (mDataStallAlarmTag != tag) {
            if (DBG) {
                log("onDataStallAlarm: ignore, tag=" + tag + " expecting " + mDataStallAlarmTag);
            }
            return;
        }
        updateDataStallInfo();

        int hangWatchdogTrigger = Settings.Global.getInt(mResolver,
                Settings.Global.PDP_WATCHDOG_TRIGGER_PACKET_COUNT,
                NUMBER_SENT_PACKETS_OF_HANG);

        boolean suspectedStall = DATA_STALL_NOT_SUSPECTED;
        if (mSentSinceLastRecv >= hangWatchdogTrigger) {
            if (DBG) {
                log("onDataStallAlarm: tag=" + tag + " do recovery action=" + getRecoveryAction());
            }
            suspectedStall = DATA_STALL_SUSPECTED;
            sendMessage(obtainMessage(DctConstants.EVENT_DO_RECOVERY));
        } else {
            if (VDBG_STALL) {
                log("onDataStallAlarm: tag=" + tag + " Sent " + String.valueOf(mSentSinceLastRecv) +
                    " pkts since last received, < watchdogTrigger=" + hangWatchdogTrigger);
            }
        }
        startDataStallAlarm(suspectedStall);
    }

    private void startDataStallAlarm(boolean suspectedStall) {
        int nextAction = getRecoveryAction();
        int delayInMs;

        if (mDataStallDetectionEnabled && getOverallState() == DctConstants.State.CONNECTED) {
            // If screen is on or data stall is currently suspected, set the alarm
            // with an aggressive timeout.
            if (mIsScreenOn || suspectedStall || RecoveryAction.isAggressiveRecovery(nextAction)) {
                delayInMs = Settings.Global.getInt(mResolver,
                        Settings.Global.DATA_STALL_ALARM_AGGRESSIVE_DELAY_IN_MS,
                        DATA_STALL_ALARM_AGGRESSIVE_DELAY_IN_MS_DEFAULT);
            } else {
                delayInMs = Settings.Global.getInt(mResolver,
                        Settings.Global.DATA_STALL_ALARM_NON_AGGRESSIVE_DELAY_IN_MS,
                        DATA_STALL_ALARM_NON_AGGRESSIVE_DELAY_IN_MS_DEFAULT);
            }

            mDataStallAlarmTag += 1;
            if (VDBG_STALL) {
                log("startDataStallAlarm: tag=" + mDataStallAlarmTag +
                        " delay=" + (delayInMs / 1000) + "s");
            }
            Intent intent = new Intent(INTENT_DATA_STALL_ALARM);
            intent.putExtra(DATA_STALL_ALARM_TAG_EXTRA, mDataStallAlarmTag);
            mDataStallAlarmIntent = PendingIntent.getBroadcast(mPhone.getContext(), 0, intent,
                    PendingIntent.FLAG_UPDATE_CURRENT);
            mAlarmManager.set(AlarmManager.ELAPSED_REALTIME,
                    SystemClock.elapsedRealtime() + delayInMs, mDataStallAlarmIntent);
        } else {
            if (VDBG_STALL) {
                log("startDataStallAlarm: NOT started, no connection tag=" + mDataStallAlarmTag);
            }
        }
    }

    private void stopDataStallAlarm() {
        if (VDBG_STALL) {
            log("stopDataStallAlarm: current tag=" + mDataStallAlarmTag +
                    " mDataStallAlarmIntent=" + mDataStallAlarmIntent);
        }
        mDataStallAlarmTag += 1;
        if (mDataStallAlarmIntent != null) {
            mAlarmManager.cancel(mDataStallAlarmIntent);
            mDataStallAlarmIntent = null;
        }
    }

    private void restartDataStallAlarm() {
        if (isConnected() == false) return;
        // To be called on screen status change.
        // Do not cancel the alarm if it is set with aggressive timeout.
        int nextAction = getRecoveryAction();

        if (RecoveryAction.isAggressiveRecovery(nextAction)) {
            if (DBG) log("restartDataStallAlarm: action is pending. not resetting the alarm.");
            return;
        }
        if (VDBG_STALL) log("restartDataStallAlarm: stop then start.");
        stopDataStallAlarm();
        startDataStallAlarm(DATA_STALL_NOT_SUSPECTED);
    }

    /**
     * Provisioning APN
     */
    private void onActionIntentProvisioningApnAlarm(Intent intent) {
        if (DBG) log("onActionIntentProvisioningApnAlarm: action=" + intent.getAction());
        Message msg = obtainMessage(DctConstants.EVENT_PROVISIONING_APN_ALARM,
                intent.getAction());
        msg.arg1 = intent.getIntExtra(PROVISIONING_APN_ALARM_TAG_EXTRA, 0);
        sendMessage(msg);
    }

    private void startProvisioningApnAlarm() {
        int delayInMs = Settings.Global.getInt(mResolver,
                                Settings.Global.PROVISIONING_APN_ALARM_DELAY_IN_MS,
                                PROVISIONING_APN_ALARM_DELAY_IN_MS_DEFAULT);
        if (Build.IS_DEBUGGABLE) {
            // Allow debug code to use a system property to provide another value
            String delayInMsStrg = Integer.toString(delayInMs);
            delayInMsStrg = System.getProperty(DEBUG_PROV_APN_ALARM, delayInMsStrg);
            try {
                delayInMs = Integer.parseInt(delayInMsStrg);
            } catch (NumberFormatException e) {
                loge("startProvisioningApnAlarm: e=" + e);
            }
        }
        mProvisioningApnAlarmTag += 1;
        if (DBG) {
            log("startProvisioningApnAlarm: tag=" + mProvisioningApnAlarmTag +
                    " delay=" + (delayInMs / 1000) + "s");
        }
        Intent intent = new Intent(INTENT_PROVISIONING_APN_ALARM);
        intent.putExtra(PROVISIONING_APN_ALARM_TAG_EXTRA, mProvisioningApnAlarmTag);
        mProvisioningApnAlarmIntent = PendingIntent.getBroadcast(mPhone.getContext(), 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);
        mAlarmManager.set(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + delayInMs, mProvisioningApnAlarmIntent);
    }

    private void stopProvisioningApnAlarm() {
        if (DBG) {
            log("stopProvisioningApnAlarm: current tag=" + mProvisioningApnAlarmTag +
                    " mProvsioningApnAlarmIntent=" + mProvisioningApnAlarmIntent);
        }
        mProvisioningApnAlarmTag += 1;
        if (mProvisioningApnAlarmIntent != null) {
            mAlarmManager.cancel(mProvisioningApnAlarmIntent);
            mProvisioningApnAlarmIntent = null;
        }
    }

    private static DataProfile createDataProfile(ApnSetting apn) {
        return createDataProfile(apn, apn.profileId);
    }

    @VisibleForTesting
    public static DataProfile createDataProfile(ApnSetting apn, int profileId) {
        int profileType;

        int bearerBitmap = 0;
        bearerBitmap = ServiceState.convertNetworkTypeBitmaskToBearerBitmask(
                apn.networkTypeBitmask);

        if (bearerBitmap == 0) {
            profileType = DataProfile.TYPE_COMMON;
        } else if (ServiceState.bearerBitmapHasCdma(bearerBitmap)) {
            profileType = DataProfile.TYPE_3GPP2;
        } else {
            profileType = DataProfile.TYPE_3GPP;
        }

        return new DataProfile(profileId, apn.apn, apn.protocol,
                apn.authType, apn.user, apn.password, profileType,
                apn.maxConnsTime, apn.maxConns, apn.waitTime, apn.carrierEnabled, apn.typesBitmap,
                apn.roamingProtocol, bearerBitmap, apn.mtu, apn.mvnoType, apn.mvnoMatchData,
                apn.modemCognitive);
    }

    private void onDataServiceBindingChanged(boolean bound) {
        if (bound) {
            mDcc.start();
        } else {
            mDcc.dispose();
        }
    }
}
