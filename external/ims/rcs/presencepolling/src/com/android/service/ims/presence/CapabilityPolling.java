/*
 * Copyright (c) 2015, Motorola Mobility LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     - Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     - Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     - Neither the name of Motorola Mobility nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA MOBILITY LLC BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

package com.android.service.ims.presence;

import android.app.AlarmManager;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.os.Process;
import android.os.SystemClock;
import android.telephony.PhoneNumberUtils;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.text.format.Time;
import android.text.TextUtils;
import android.content.ComponentName;

import com.android.ims.ImsConfig;
import com.android.ims.ImsException;
import com.android.ims.ImsManager;
import com.android.internal.telephony.IccCardConstants;
import com.android.internal.telephony.TelephonyIntents;

import com.android.ims.RcsException;
import com.android.ims.RcsManager;
import com.android.ims.RcsManager.ResultCode;
import com.android.ims.RcsPresence;
import com.android.ims.RcsPresence.PublishState;
import com.android.ims.RcsPresenceInfo;
import com.android.ims.internal.ContactNumberUtils;
import com.android.ims.internal.Logger;

import java.util.ArrayList;
import java.util.List;

public class CapabilityPolling {
    private Logger logger = Logger.getLogger(this.getClass().getName());
    private final Context mContext;

    public static final String ACTION_PERIODICAL_DISCOVERY_ALARM =
            "com.android.service.ims.presence.periodical_capability_discovery";
    private PendingIntent mDiscoveryAlarmIntent = null;

    public static final int ACTION_POLLING_NORMAL = 0;
    public static final int ACTION_POLLING_NEW_CONTACTS = 1;

    private long mCapabilityPollInterval = 604800000L;
    private long mMinCapabilityPollInterval = 60480000L;
    private long mCapabilityCacheExpiration = 7776000000L;
    private long mNextPollingTimeStamp = 0L;
    private final Object mScheduleSyncObj = new Object();

    private boolean mInitialized = false;
    private AlarmManager mAlarmManager = null;
    private EABContactManager mEABContactManager = null;
    private boolean mStackAvailable = false;
    private int mPublished = -1;
    private int mProvisioned = -1;

    private HandlerThread mDiscoveryThread;
    private Handler mDiscoveryHandler;

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            logger.info("onReceive(), intent: " + intent +
                    ", context: " + context);

            String action = intent.getAction();
            if (RcsManager.ACTION_RCS_SERVICE_AVAILABLE.equals(action)) {
                enqueueServiceStatusChanged(true);
            } else if (RcsManager.ACTION_RCS_SERVICE_UNAVAILABLE.equals(action)) {
                enqueueServiceStatusChanged(false);
            } else if (RcsManager.ACTION_RCS_SERVICE_DIED.equals(action)) {
                logger.warn("No handler for this intent: " + action);
            } else if (RcsPresence.ACTION_PUBLISH_STATE_CHANGED.equals(action)) {
                int state = intent.getIntExtra(
                        RcsPresence.EXTRA_PUBLISH_STATE,
                        RcsPresence.PublishState.PUBLISH_STATE_NOT_PUBLISHED);
                enqueuePublishStateChanged(state);
            } else if (ImsConfig.ACTION_IMS_CONFIG_CHANGED.equals(action)) {
                int item = intent.getIntExtra(ImsConfig.EXTRA_CHANGED_ITEM, -1);
                if ((ImsConfig.ConfigConstants.CAPABILITIES_POLL_INTERVAL == item) ||
                    (ImsConfig.ConfigConstants.CAPABILITIES_CACHE_EXPIRATION == item)) {
                    enqueueSettingsChanged();
                } else if ((ImsConfig.ConfigConstants.VLT_SETTING_ENABLED == item) ||
                        (ImsConfig.ConfigConstants.LVC_SETTING_ENABLED == item) ||
                        (ImsConfig.ConfigConstants.EAB_SETTING_ENABLED == item)) {
                    enqueueProvisionStateChanged();
                }
            } else if(TelephonyIntents.ACTION_SIM_STATE_CHANGED.equalsIgnoreCase(action)) {
                String stateExtra = intent.getStringExtra(
                        IccCardConstants.INTENT_KEY_ICC_STATE);
                logger.print("SIM_STATE_CHANGED: " + stateExtra);
                if (IccCardConstants.INTENT_VALUE_ICC_LOADED.equalsIgnoreCase(stateExtra)) {
                    enqueueSimLoaded();
                }
            } else {
                logger.debug("No interest in this intent: " + action);
            }
        }
    };

    private static CapabilityPolling sInstance = null;
    public static synchronized CapabilityPolling getInstance(Context context) {
        if ((sInstance == null) && (context != null)) {
            sInstance = new CapabilityPolling(context);
        }

        return sInstance;
    }

    private CapabilityPolling(Context context) {
        mContext = context;

        ContactNumberUtils.getDefault().setContext(mContext);
        PresencePreferences.getInstance().setContext(mContext);

        mAlarmManager = (AlarmManager) mContext.getSystemService(Context.ALARM_SERVICE);
        mEABContactManager = new EABContactManager(mContext.getContentResolver(),
                mContext.getPackageName());

        setRcsTestMode(PresencePreferences.getInstance().getRcsTestMode());
        logger.debug("CapabilityPolling is created.");
    }

    public void setRcsTestMode(boolean test) {
        logger.setRcsTestMode(test);

        PresencePreferences pref = PresencePreferences.getInstance();
        if (pref != null) {
            if (pref.getRcsTestMode() != test) {
                pref.setRcsTestMode(test);
            }
        }
    }

    private void initialise() {
        if (mInitialized) {
            return;
        }

        PresenceSetting.init(mContext);
        long capabilityPollInterval = PresenceSetting.getCapabilityPollInterval();
        logger.print("getCapabilityPollInterval: " + capabilityPollInterval);
        if (capabilityPollInterval == -1) {
            capabilityPollInterval = mContext.getResources().getInteger(
                    R.integer.capability_poll_interval);
        }
        if (capabilityPollInterval < 10) {
            capabilityPollInterval = 10;
        }

        long capabilityCacheExpiration = PresenceSetting.getCapabilityCacheExpiration();
        logger.print("getCapabilityCacheExpiration: " + capabilityCacheExpiration);
        if (capabilityCacheExpiration == -1) {
            capabilityCacheExpiration = mContext.getResources().getInteger(
                    R.integer.capability_cache_expiration);
        }
        if (capabilityCacheExpiration < 10) {
            capabilityCacheExpiration = 10;
        }

        int publishTimer = PresenceSetting.getPublishTimer();
        logger.print("getPublishTimer: " + publishTimer);
        int publishTimerExtended = PresenceSetting.getPublishTimerExtended();
        logger.print("getPublishTimerExtended: " + publishTimerExtended);
        int maxEntriesInRequest = PresenceSetting.getMaxNumberOfEntriesInRequestContainedList();
        logger.print("getMaxNumberOfEntriesInRequestContainedList: " + maxEntriesInRequest);
        if ((capabilityPollInterval <= 30 * 60) ||     // default: 7 days
            (capabilityCacheExpiration <= 60 * 60) ||  // default: 90 days
            (maxEntriesInRequest <= 20) ||             // default: 100
            (publishTimer <= 10 * 60) ||               // default: 20 minutes
            (publishTimerExtended <= 20 * 60)) {       // default: 1 day
            setRcsTestMode(true);
        }

        if (capabilityCacheExpiration < capabilityPollInterval) {
            capabilityPollInterval = capabilityCacheExpiration;
        }

        mCapabilityPollInterval = capabilityPollInterval * 1000;
        mMinCapabilityPollInterval = mCapabilityPollInterval / 10;
        logger.info("mCapabilityPollInterval: " + mCapabilityPollInterval +
                ", mMinCapabilityPollInterval: " + mMinCapabilityPollInterval);

        mCapabilityCacheExpiration = capabilityCacheExpiration * 1000;
        logger.info("mCapabilityCacheExpiration: " + mCapabilityCacheExpiration);

        mInitialized = true;
    }

    public void start() {
        mDiscoveryThread = new HandlerThread("Presence-DiscoveryThread");
        mDiscoveryThread.start();
        mDiscoveryHandler = new Handler(mDiscoveryThread.getLooper(), mDiscoveryCallback);

        registerForBroadcasts();

        if (isPollingReady()) {
            schedulePolling(5 * 1000, ACTION_POLLING_NORMAL);
        }
    }

    public void stop() {
        cancelDiscoveryAlarm();
        clearPollingTasks();
        mContext.unregisterReceiver(mReceiver);
        mDiscoveryThread.quit();
    }

    private void registerForBroadcasts() {
        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(RcsManager.ACTION_RCS_SERVICE_AVAILABLE);
        intentFilter.addAction(RcsManager.ACTION_RCS_SERVICE_UNAVAILABLE);
        intentFilter.addAction(RcsManager.ACTION_RCS_SERVICE_DIED);
        intentFilter.addAction(RcsPresence.ACTION_PUBLISH_STATE_CHANGED);
        intentFilter.addAction(ImsConfig.ACTION_IMS_CONFIG_CHANGED);
        intentFilter.addAction(TelephonyIntents.ACTION_SIM_STATE_CHANGED);
        mContext.registerReceiver(mReceiver, intentFilter);
    }

    private boolean isPollingReady() {
        if (mPublished == -1) {
            RcsManager rcsManager = RcsManager.getInstance(mContext, 0);
            if (rcsManager != null) {
                mStackAvailable = rcsManager.isRcsServiceAvailable();
                logger.print("isPollingReady, mStackAvailable: " + mStackAvailable);
                try {
                    RcsPresence rcsPresence = rcsManager.getRcsPresenceInterface();
                    if (rcsPresence != null) {
                        int state = rcsPresence.getPublishState();
                        mPublished = (PublishState.PUBLISH_STATE_200_OK == state) ? 1 : 0;
                        logger.print("isPollingReady, mPublished: " + mPublished);
                    }
                } catch (RcsException ex) {
                    logger.warn("RcsPresence.getPublishState failed, exception: " + ex);
                    mPublished = -1;
                }
            }
        }

        if (mProvisioned == -1) {
            ImsManager imsManager = ImsManager.getInstance(mContext,
                    SubscriptionManager.getDefaultVoicePhoneId());
            if (imsManager != null) {
                try {
                    ImsConfig imsConfig = imsManager.getConfigInterface();
                    if (imsConfig != null) {
                        mProvisioned = imsConfig.getProvisionedValue(
                            ImsConfig.ConfigConstants.EAB_SETTING_ENABLED);
                        logger.print("isPollingReady, mProvisioned: " + mProvisioned);
                    }
                } catch (ImsException ex) {
                    logger.warn("ImsConfig.getEabProvisioned failed, exception: " + ex);
                    mProvisioned = -1;
                }
            }
        }
        logger.print("isPollingReady, mProvisioned: " + mProvisioned +
                ", mStackAvailable: " + mStackAvailable + ", mPublished: " + mPublished);
        return mStackAvailable && (mPublished == 1) && (mProvisioned == 1);
    }

    private void serviceStatusChanged(boolean enabled) {
        logger.print("Enter serviceStatusChanged: " + enabled);
        mStackAvailable = enabled;

        if (isPollingReady()) {
            schedulePolling(0, ACTION_POLLING_NORMAL);
        } else {
            cancelDiscoveryAlarm();
            clearPollingTasks();
        }
    }

    private void publishStateChanged(int state) {
        logger.print("Enter publishStateChanged: " + state);
        mPublished = (PublishState.PUBLISH_STATE_200_OK == state) ? 1 : 0;
        if (mPublished == 1) {
            PresencePreferences pref = PresencePreferences.getInstance();
            if (pref != null) {
                String mdn_old = pref.getLine1Number();
                String subscriberId_old = pref.getSubscriberId();
                if (TextUtils.isEmpty(mdn_old) && TextUtils.isEmpty(subscriberId_old)) {
                    String mdn = getLine1Number();
                    pref.setLine1Number(mdn);
                    String subscriberId = getSubscriberId();
                    pref.setSubscriberId(subscriberId);
                }
            }
        }

        if (isPollingReady()) {
            schedulePolling(0, ACTION_POLLING_NORMAL);
        } else {
            cancelDiscoveryAlarm();
            clearPollingTasks();
        }
    }

    private void provisionStateChanged() {
        ImsManager imsManager = ImsManager.getInstance(mContext,
                SubscriptionManager.getDefaultVoicePhoneId());
        if (imsManager == null) {
            return;
        }

        try {
            ImsConfig imsConfig = imsManager.getConfigInterface();
            if (imsConfig == null) {
                return;
            }
            boolean volteProvision = imsConfig.getProvisionedValue(
                    ImsConfig.ConfigConstants.VLT_SETTING_ENABLED)
                    == ImsConfig.FeatureValueConstants.ON;
            boolean vtProvision = imsConfig.getProvisionedValue(
                    ImsConfig.ConfigConstants.LVC_SETTING_ENABLED)
                    == ImsConfig.FeatureValueConstants.ON;
            boolean eabProvision = imsConfig.getProvisionedValue(
                    ImsConfig.ConfigConstants.EAB_SETTING_ENABLED)
                    == ImsConfig.FeatureValueConstants.ON;
            logger.print("Provision state changed, VolteProvision: "
                        + volteProvision + ", vtProvision: " + vtProvision
                        + ", eabProvision: " + eabProvision);

            if ((mProvisioned == 1) && !eabProvision) {
                logger.print("EAB Provision is disabled, clear all capabilities!");
                if (mEABContactManager != null) {
                    mEABContactManager.updateAllCapabilityToUnknown();
                }
            }
            mProvisioned = eabProvision ? 1 : 0;
        } catch (ImsException ex) {
            logger.warn("ImsConfig.getEabProvisioned failed, exception: " + ex);
            mProvisioned = -1;
            return;
        }

        if (isPollingReady()) {
            schedulePolling(0, ACTION_POLLING_NORMAL);
        } else {
            cancelDiscoveryAlarm();
            clearPollingTasks();
        }
    }

    private void settingsChanged() {
        logger.print("Enter settingsChanged.");
        cancelDiscoveryAlarm();
        clearPollingTasks();
        mInitialized = false;

        if (isPollingReady()) {
            schedulePolling(0, ACTION_POLLING_NORMAL);
        }
    }

    private void newContactAdded(String number) {
        if (TextUtils.isEmpty(number)) {
            return;
        }

        EABContactManager.Request request = new EABContactManager.Request(number)
                .setLastUpdatedTimeStamp(0);
        int result = mEABContactManager.update(request);
        if (result <= 0) {
            return;
        }

        if (isPollingReady()) {
            schedulePolling(5 * 1000, ACTION_POLLING_NEW_CONTACTS);
        }
    }

    private void verifyPollingResult(int counts) {
        if (isPollingReady()) {
            PresencePreferences pref = PresencePreferences.getInstance();
            if ((pref != null) && pref.getRcsTestMode()) {
                counts = 1;
            }
            long lm = (long)(1 << (counts - 1));
            schedulePolling(30 * 1000 * lm, ACTION_POLLING_NORMAL);
        }
    }

    public synchronized void schedulePolling(long msec, int type) {
        logger.print("schedulePolling msec=" + msec + " type=" + type);
        if (!isPollingReady()) {
            logger.debug("Cancel the polling since the network is not ready");
            return;
        }

        if (type == ACTION_POLLING_NEW_CONTACTS) {
            cancelDiscoveryAlarm();
        }

        if (mNextPollingTimeStamp != 0L) {
            long scheduled = mNextPollingTimeStamp - System.currentTimeMillis();
            if ((scheduled > 0) && (scheduled < msec)) {
                logger.print("There has been a discovery scheduled at "
                        + getTimeString(mNextPollingTimeStamp));
                return;
            }
        }

        long nextTime = System.currentTimeMillis() + msec;
        logger.print("A new discovery needs to be started in " +
                (msec / 1000) + " seconds at "
                + getTimeString(nextTime));

        cancelDiscoveryAlarm();

        if (msec <= 0) {
            enqueueDiscovery(type);
            return;
        }

        Intent intent = new Intent(ACTION_PERIODICAL_DISCOVERY_ALARM);
        intent.setClass(mContext, AlarmBroadcastReceiver.class);
        intent.putExtra("pollingType", type);

        mDiscoveryAlarmIntent = PendingIntent.getBroadcast(mContext, 0, intent,
                PendingIntent.FLAG_UPDATE_CURRENT);
        mAlarmManager.setExact(AlarmManager.ELAPSED_REALTIME_WAKEUP,
                SystemClock.elapsedRealtime() + msec, mDiscoveryAlarmIntent);

        mNextPollingTimeStamp = nextTime;
    }

    private synchronized void doCapabilityDiscovery(int type) {
        logger.print("doCapabilityDiscovery type=" + type);
        if (!isPollingReady()) {
            logger.debug("doCapabilityDiscovery isPollingReady=false");
            return;
        }

        long delay = 0;
        List<Contacts.Item> list = null;

        initialise();

        mNextPollingTimeStamp = 0L;
        Cursor cursor = null;
        EABContactManager.Query baseQuery = new EABContactManager.Query()
                .orderBy(EABContactManager.COLUMN_LAST_UPDATED_TIMESTAMP,
                         EABContactManager.Query.ORDER_ASCENDING);
        try {
            logger.debug("doCapabilityDiscovery.query:\n" + baseQuery);
            cursor = mEABContactManager.query(baseQuery);
            if (cursor == null) {
                logger.print("Cursor is null, there is no database found.");
                return;
            }
            int count = cursor.getCount();
            if (count == 0) {
                logger.print("Cursor.getCount() is 0, there is no items found in db.");
                return;
            }

            list = new ArrayList<Contacts.Item>();
            list.clear();

            long current = System.currentTimeMillis();
            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                long id = cursor.getLong(cursor.getColumnIndex(Contacts.Impl._ID));
                long last = cursor.getLong(cursor.getColumnIndex(
                        Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP));

                Contacts.Item item = new Contacts.Item(id);
                item.setLastUpdateTime(last);
                item.setNumber(cursor.getString(
                        cursor.getColumnIndex(Contacts.Impl.CONTACT_NUMBER)));
                item.setName(cursor.getString(
                        cursor.getColumnIndex(Contacts.Impl.CONTACT_NAME)));
                item.setVolteTimestamp(cursor.getLong(
                        cursor.getColumnIndex(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP)));
                item.setVideoTimestamp(cursor.getLong(
                        cursor.getColumnIndex(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP)));

                if ((current - last < 0) ||
                    (current - last >= mCapabilityPollInterval - mMinCapabilityPollInterval)) {
                    logger.print("This item will be updated:\n"
                            + item);
                    if (item.isValid()) {
                        list.add(item);
                    }
                } else {
                    logger.print("The first item which will be updated next time:\n" + item);
                    delay = randomCapabilityPollInterval() - (current - last);
                    if (delay > 0) {
                        break;
                    }
                }
            }
        } catch (Exception ex) {
            logger.warn("Exception in doCapabilityDiscovery: " + ex);
            if (delay <= 0) {
                delay = 5 * 60 * 1000;
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        if (delay <= 0) {
            delay = randomCapabilityPollInterval();
        }
        logger.print("Polling delay: " + delay);
        schedulePolling(delay, ACTION_POLLING_NORMAL);

        updateObsoleteItems();

        if ((list != null) && (list.size() > 0)) {
            PollingsQueue queue = PollingsQueue.getInstance(mContext);
            if (queue != null) {
                queue.setCapabilityPolling(this);
                queue.add(type, list);
            }
        }
    }

    private long randomCapabilityPollInterval() {
        double random = Math.random() * 0.2 + 0.9;
        logger.print("The random for this time polling is: " + random);
        random = random * mCapabilityPollInterval;
        return (long)random;
    }

    private void updateObsoleteItems() {
        long current = System.currentTimeMillis();
        long last = current - mCapabilityCacheExpiration;
        long last3year = current - 3 * 365 * 24 * 3600000L;
        StringBuilder sb = new StringBuilder();
        sb.append("((");
        sb.append(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP + "<='" + last + "'");
        sb.append(" OR ");
        sb.append(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP + "<='" + last + "'");
        sb.append(") AND ");
        sb.append(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP + ">='" + last3year + "'");
        sb.append(")");
        EABContactManager.Query baseQuery = new EABContactManager.Query()
                .setFilterByTime(sb.toString())
                .orderBy(EABContactManager.COLUMN_ID,
                         EABContactManager.Query.ORDER_ASCENDING);

        Cursor cursor = null;
        List<Contacts.Item> list = null;
        try {
            logger.debug("updateObsoleteItems.query:\n" + baseQuery);
            cursor = mEABContactManager.query(baseQuery);
            if (cursor == null) {
                logger.print("Cursor is null, there is no database found.");
                return;
            }
            int count = cursor.getCount();
            if (count == 0) {
                logger.print("Cursor.getCount() is 0, there is no obsolete items found.");
                return;
            }

            list = new ArrayList<Contacts.Item>();
            list.clear();

            for (cursor.moveToFirst(); !cursor.isAfterLast(); cursor.moveToNext()) {
                long id = cursor.getLong(cursor.getColumnIndex(Contacts.Impl._ID));

                Contacts.Item item = new Contacts.Item(id);
                item.setLastUpdateTime(cursor.getLong(
                        cursor.getColumnIndex(Contacts.Impl.CONTACT_LAST_UPDATED_TIMESTAMP)));
                item.setNumber(cursor.getString(
                        cursor.getColumnIndex(Contacts.Impl.CONTACT_NUMBER)));
                item.setName(cursor.getString(
                        cursor.getColumnIndex(Contacts.Impl.CONTACT_NAME)));
                item.setVolteTimestamp(cursor.getLong(
                        cursor.getColumnIndex(Contacts.Impl.VOLTE_CALL_CAPABILITY_TIMESTAMP)));
                item.setVideoTimestamp(cursor.getLong(
                        cursor.getColumnIndex(Contacts.Impl.VIDEO_CALL_CAPABILITY_TIMESTAMP)));
                logger.print("updateObsoleteItems, the obsolete item:\n" + item);

                if ((item.lastUpdateTime() > 0) && item.isValid()) {
                    list.add(item);
                }
            }
        } catch (Exception ex) {
            logger.warn("Exception in updateObsoleteItems: " + ex);
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        if ((list == null) || (list.size() <= 0)) {
            return;
        }

        for (Contacts.Item item : list) {
            EABContactManager.Request request = new EABContactManager.Request(item.id());
            if (item.volteTimestamp() <= last) {
                request.setVolteCallCapability(false);
                request.setVolteCallCapabilityTimeStamp(current);
            }
            if (item.videoTimestamp() <= last) {
                request.setVideoCallCapability(false);
                request.setVideoCallCapabilityTimeStamp(current);
            }
            int result = mEABContactManager.update(request);
            if (result <= 0) {
                logger.print("Failed to update this request: " + request);
            }
        }
    }

    private void cancelDiscoveryAlarm() {
        if (mDiscoveryAlarmIntent != null) {
            mAlarmManager.cancel(mDiscoveryAlarmIntent);
            mDiscoveryAlarmIntent = null;
            mNextPollingTimeStamp = 0L;
        }
    }

    private void clearPollingTasks() {
        PollingsQueue queue = PollingsQueue.getInstance(null);
        if (queue != null) {
            queue.clear();
        }
    }

    private String getTimeString(long time) {
        if (time <= 0) {
            time = System.currentTimeMillis();
        }

        Time tobj = new Time();
        tobj.set(time);
        return String.format("%s.%s", tobj.format("%m-%d %H:%M:%S"), time % 1000);
    }

    private static final int MSG_CHECK_DISCOVERY = 1;
    private static final int MSG_NEW_CONTACT_ADDED = 2;
    private static final int MSG_SERVICE_STATUS_CHANGED = 3;
    private static final int MSG_SETTINGS_CHANGED = 4;
    private static final int MSG_PUBLISH_STATE_CHANGED = 5;
    private static final int MSG_SIM_LOADED = 6;
    private static final int MSG_PROVISION_STATE_CHANGED = 7;
    private static final int MSG_VERIFY_POLLING_RESULT = 8;

    public void enqueueDiscovery(int type) {
        mDiscoveryHandler.removeMessages(MSG_CHECK_DISCOVERY);
        mDiscoveryHandler.obtainMessage(MSG_CHECK_DISCOVERY, type, -1).sendToTarget();
    }

    public void enqueueNewContact(String number) {
        mDiscoveryHandler.obtainMessage(MSG_NEW_CONTACT_ADDED, number).sendToTarget();
    }

    private void enqueueServiceStatusChanged(boolean enabled) {
        mDiscoveryHandler.removeMessages(MSG_SERVICE_STATUS_CHANGED);
        mDiscoveryHandler.obtainMessage(MSG_SERVICE_STATUS_CHANGED,
                enabled ? 1 : 0, -1).sendToTarget();
    }

    private void enqueuePublishStateChanged(int state) {
        mDiscoveryHandler.removeMessages(MSG_PUBLISH_STATE_CHANGED);
        mDiscoveryHandler.obtainMessage(MSG_PUBLISH_STATE_CHANGED,
                state, -1).sendToTarget();
    }

    public void enqueueSettingsChanged() {
        mDiscoveryHandler.removeMessages(MSG_SETTINGS_CHANGED);
        mDiscoveryHandler.obtainMessage(MSG_SETTINGS_CHANGED).sendToTarget();
    }

    private void enqueueSimLoaded() {
        mDiscoveryHandler.removeMessages(MSG_SIM_LOADED);
        mDiscoveryHandler.obtainMessage(MSG_SIM_LOADED).sendToTarget();
    }

    private void enqueueProvisionStateChanged() {
        mDiscoveryHandler.removeMessages(MSG_PROVISION_STATE_CHANGED);
        mDiscoveryHandler.obtainMessage(MSG_PROVISION_STATE_CHANGED).sendToTarget();
    }

    public void enqueueVerifyPollingResult(int counts) {
        mDiscoveryHandler.removeMessages(MSG_VERIFY_POLLING_RESULT);
        mDiscoveryHandler.obtainMessage(MSG_VERIFY_POLLING_RESULT, counts, -1).sendToTarget();
    }

    private Handler.Callback mDiscoveryCallback = new Handler.Callback() {
        @Override
        public boolean handleMessage(Message msg) {
            Process.setThreadPriority(Process.THREAD_PRIORITY_BACKGROUND);

            if (msg.what == MSG_CHECK_DISCOVERY) {
                doCapabilityDiscovery(msg.arg1);
            } else if (msg.what == MSG_NEW_CONTACT_ADDED) {
                newContactAdded((String)msg.obj);
            } else if (msg.what == MSG_SERVICE_STATUS_CHANGED) {
                serviceStatusChanged(msg.arg1 == 1);
            } else if (msg.what == MSG_PUBLISH_STATE_CHANGED) {
                publishStateChanged(msg.arg1);
            } else if (msg.what == MSG_SETTINGS_CHANGED) {
                settingsChanged();
            } else if(msg.what == MSG_SIM_LOADED) {
                onSimLoaded();
            } else if(msg.what == MSG_PROVISION_STATE_CHANGED) {
                provisionStateChanged();
            } else if(msg.what == MSG_VERIFY_POLLING_RESULT) {
                verifyPollingResult(msg.arg1);
            } else {
            }

            return true;
        }
    };

    private void onSimLoaded() {
        PresencePreferences pref = PresencePreferences.getInstance();
        if (pref == null) {
            return;
        }

        String mdn_old = pref.getLine1Number();
        if (TextUtils.isEmpty(mdn_old)) {
            return;
        }
        String subscriberId_old = pref.getSubscriberId();
        if (TextUtils.isEmpty(subscriberId_old)) {
            return;
        }

        String mdn = getLine1Number();
        String subscriberId = getSubscriberId();
        if (TextUtils.isEmpty(mdn) && TextUtils.isEmpty(subscriberId)) {
            return;
        }

        boolean mdnMatched = false;
        if (TextUtils.isEmpty(mdn) || PhoneNumberUtils.compare(mdn_old, mdn)) {
            mdnMatched = true;
        }
        boolean subscriberIdMatched = false;
        if (TextUtils.isEmpty(subscriberId) || subscriberId.equals(subscriberId_old)) {
            subscriberIdMatched = true;
        }
        if (mdnMatched && subscriberIdMatched) {
            return;
        }

        logger.print("Remove presence cache for Sim card changed!");
        pref.setLine1Number("");
        pref.setSubscriberId("");

        if (mEABContactManager != null) {
            mEABContactManager.updateAllCapabilityToUnknown();
        }
    }

    private static final int DEFAULT_SUBSCRIPTION = 1;
    private String getLine1Number() {
        if (mContext == null) {
            return null;
        }

        TelephonyManager telephony = (TelephonyManager)
                mContext.getSystemService(Context.TELEPHONY_SERVICE);
        if (telephony == null) {
            return null;
        }

        String mdn = null;
        if (TelephonyManager.getDefault().isMultiSimEnabled()) {
            int subId = SubscriptionManager.getDefaultDataSubscriptionId();
            if (!SubscriptionManager.isValidSubscriptionId(subId)) {
                subId = DEFAULT_SUBSCRIPTION;
            }
            mdn = telephony.getLine1Number(subId);
        } else {
            mdn = telephony.getLine1Number();
        }

        if ((mdn == null) || (mdn.length() == 0) ||  mdn.startsWith("00000")) {
            return null;
        }

        logger.print("getLine1Number: " + mdn);
        return mdn;
    }

    private String getSubscriberId() {
        if (mContext == null) {
            return null;
        }

        TelephonyManager telephony = (TelephonyManager)
                mContext.getSystemService(Context.TELEPHONY_SERVICE);
        if (telephony == null) {
            return null;
        }

        String subscriberId = null;
        if (TelephonyManager.getDefault().isMultiSimEnabled()) {
            int subId = SubscriptionManager.getDefaultDataSubscriptionId();
            if (!SubscriptionManager.isValidSubscriptionId(subId)) {
                subId = DEFAULT_SUBSCRIPTION;
            }
            subscriberId = telephony.getSubscriberId(subId);
        } else {
            subscriberId = telephony.getSubscriberId();
        }

        logger.print("getSubscriberId: " + subscriberId);
        return subscriberId;
    }
}
