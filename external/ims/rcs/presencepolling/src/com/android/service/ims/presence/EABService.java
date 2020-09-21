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

import java.util.ArrayList;
import java.util.Calendar;

import android.app.Service;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.database.ContentObserver;
import android.database.Cursor;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.provider.ContactsContract;
import android.provider.ContactsContract.CommonDataKinds.Phone;
import android.telephony.PhoneNumberUtils;
import android.text.TextUtils;

import android.telephony.SubscriptionManager;
import com.android.ims.internal.EABContract;

import com.android.ims.ImsConfig;
import com.android.ims.ImsManager;
import com.android.ims.ImsException;

import com.android.ims.RcsManager;
import com.android.ims.RcsPresence;
import com.android.ims.RcsException;
import com.android.ims.internal.Logger;

public class EABService extends Service {

    private Logger logger = Logger.getLogger(this.getClass().getName());

    private Context mContext;
    private Looper mServiceLooper = null;
    private ServiceHandler mServiceHandler = null;
    // Used to avoid any content observer processing during EABService
    // initialisation as anyways it will check for Contact db changes as part of boot-up.
    private boolean isEABServiceInitializing = true;

    private static final int BOOT_COMPLETED = 0;
    private static final int CONTACT_TABLE_MODIFIED = 1;
    private static final int CONTACT_PROFILE_TABLE_MODIFIED = 2;
    private static final int EAB_DATABASE_RESET = 3;

    private static final int SYNC_COMPLETE_DELAY_TIMER = 3 * 1000; // 3 seconds.
    private static final String TAG = "EABService";

    // Framework interface files.
    private RcsManager mRcsManager = null;
    private RcsPresence mRcsPresence = null;

    public EABService() {
        super();
        logger.info("EAB Service constructed");
    }

    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

    /**
     * When "clear data" is done for contact storage in system settings, EAB
     * Provider must be cleared.
     */
    private BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            logger.debug("onReceive intent: " + action);

            switch(action) {
                case ContactsContract.Intents.CONTACTS_DATABASE_CREATED: {
                    logger.debug("Contacts database created.");
                    // Delete all entries from EAB Provider as it has to be re-synced with
                    // Contact db.
                    mContext.getContentResolver().delete(
                            EABContract.EABColumns.CONTENT_URI, null, null);
                    // Initialise EABProvider.
                    logger.debug("Resetting timestamp values in shared pref.");
                    SharedPrefUtil.resetEABSharedPref(mContext);
                    // init the EAB db after re-setting.
                    ensureInitDone();
                    break;
                }
                case Intent.ACTION_TIME_CHANGED:
                    // fall through
                case Intent.ACTION_TIMEZONE_CHANGED: {
                    Calendar cal = Calendar.getInstance();
                    long currentTimestamp = cal.getTimeInMillis();
                    long lastChangedTimestamp = SharedPrefUtil.getLastContactChangedTimestamp(
                            mContext, currentTimestamp);
                    logger.debug("lastChangedTimestamp=" + lastChangedTimestamp +
                            " currentTimestamp=" + currentTimestamp);
                    // Changed time backwards.
                    if(lastChangedTimestamp > currentTimestamp) {
                        logger.debug("Resetting timestamp values in shared pref.");
                        SharedPrefUtil.resetEABSharedPref(mContext);
                        // Set Init done to true as only the contact sync timestamps are cleared and
                        // the EABProvider table data is not cleared.
                        SharedPrefUtil.setInitDone(mContext, true);
                        CapabilityPolling capabilityPolling = CapabilityPolling.getInstance(null);
                        if (capabilityPolling != null) {
                            capabilityPolling.enqueueDiscovery(
                                    CapabilityPolling.ACTION_POLLING_NORMAL);
                        }
                    }
                    break;
                }
                case Contacts.ACTION_EAB_DATABASE_RESET: {
                    logger.info("EAB Database Reset, Recreating...");
                    sendEABResetMessage();
                    break;
                }
            }
        }
    };

    private  ContentObserver mContactChangedListener = null;
    private class ContactChangedListener extends ContentObserver {
        public ContactChangedListener() {
            super(null);
        }

        @Override
        public boolean deliverSelfNotifications() {
            return false;
        }

        @Override
        public void onChange(boolean selfChange) {
            logger.debug("onChange for ContactChangedListener");
            sendDelayedContactChangeMsg();
        }
    }

    private  ContentObserver mContactProfileListener = null;
    private class ContactProfileListener extends ContentObserver {
        public ContactProfileListener() {
            super(null);
        }

        @Override
        public boolean deliverSelfNotifications() {
            return false;
        }

        @Override
        public void onChange(boolean selfChange) {
            logger.debug("onChange for ContactProfileListener");
            sendDelayedContactProfileMsg();
        }
    }

    @Override
    public void onCreate() {
        logger.debug("Enter : onCreate");
        mContext = getApplicationContext();
        HandlerThread thread = new HandlerThread("EABServiceHandler");
        thread.start();

        mServiceLooper = thread.getLooper();
        if (mServiceLooper != null) {
            mServiceHandler = new ServiceHandler(mServiceLooper);
        } else {
            logger.debug("mServiceHandler could not be initialized since looper is null");
        }

        IntentFilter filter = new IntentFilter();
        filter.addAction(ContactsContract.Intents.CONTACTS_DATABASE_CREATED);
        filter.addAction(Intent.ACTION_TIME_CHANGED);
        filter.addAction(Intent.ACTION_TIMEZONE_CHANGED);
        filter.addAction(Contacts.ACTION_EAB_DATABASE_RESET);
        registerReceiver(mReceiver, filter);

        initializeRcsInterfacer();

        startResetContentObserverAlarm();
        super.onCreate();
    }

    @Override
    public void onDestroy() {
        cancelResetContentObserverAlarm();
        unregisterContentObservers();
        if (null != mReceiver) {
            unregisterReceiver(mReceiver);
        }
        if (null != mServiceHandler) {
            mServiceHandler = null;
        }
    }

    private void initializeRcsInterfacer() {
        // Get instance of mRcsManagr.
        if (null == mRcsManager) {
            mRcsManager = RcsManager.getInstance(this, 0);
        }
        try{
            if (null == mRcsPresence) {
                mRcsPresence = mRcsManager.getRcsPresenceInterface();
                logger.debug("mRcsManager : " + mRcsManager + " mRcsPresence : " + mRcsPresence);
            }
         }catch (RcsException e){
             logger.error("getRcsPresenceInterface() exception : ", e);
            mRcsPresence = null;
         } catch (Exception e) {
             logger.error("getRcsPresenceInterface() exception : ", e);
            mRcsPresence = null;
            mRcsManager = null;
         }
     }

    private void registerContentObservers() {
        logger.debug("Registering for Contact and Profile Change Listener.");
        mContactChangedListener = new ContactChangedListener();
        getContentResolver().registerContentObserver(
                ContactsContract.Contacts.CONTENT_URI, true,
                mContactChangedListener);

        mContactProfileListener = new ContactProfileListener();
        getContentResolver().registerContentObserver(
                ContactsContract.Profile.CONTENT_URI, true,
                mContactProfileListener);
    }

    private void unregisterContentObservers() {
        logger.debug("Un-registering for Contact and Profile Change Listener.");
        if (null != mContactChangedListener) {
            getContentResolver().unregisterContentObserver(
                    mContactChangedListener);
            mContactChangedListener = null;
        }
        if (null != mContactProfileListener) {
            getContentResolver().unregisterContentObserver(
                    mContactProfileListener);
            mContactProfileListener = null;
        }
    }

    private void resetContentObservers() {
        unregisterContentObservers();
        registerContentObservers();
    }

    private AlarmManager.OnAlarmListener mResetContentObserverListener = () -> {
        logger.debug("mResetContentObserverListener Callback Received");

        resetContentObservers();
        startResetContentObserverAlarm();
    };

    private void startResetContentObserverAlarm() {
        logger.debug("startResetContentObserverAlarm: content Observers reset every 12 hours");
        long startInterval = System.currentTimeMillis() + AlarmManager.INTERVAL_HALF_DAY;

        // Start the resetContentObservers Alarm on the ServiceHandler
        ((AlarmManager) getSystemService(Context.ALARM_SERVICE)).set(AlarmManager.RTC_WAKEUP,
                startInterval, TAG, mResetContentObserverListener, mServiceHandler);
    }

    private void cancelResetContentObserverAlarm() {
        ((AlarmManager) getSystemService(Context.ALARM_SERVICE)).cancel(
                mResetContentObserverListener);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        // As the return type is START_STICKY, check for null intent is not
        // needed as if this service's process is killed while it is started,
        // system will try to re-create the service with a null intent object if
        // there are not any pending start commands
        if (intent != null) {
            logger.debug("Enter : onStartCommand for intent : " + intent.getAction());
        }
        registerContentObservers();
        Message msg = mServiceHandler.obtainMessage(BOOT_COMPLETED);
        mServiceHandler.sendMessage(msg);
        // This service should be a always-on service.
        return START_STICKY;
    }

    private final class ServiceHandler extends Handler {
        public ServiceHandler(Looper looper) {
            super(looper);
        }

        @Override
        public void handleMessage(Message msg) {
            logger.debug("Enter: handleMessage");

            switch (msg.what) {
            case BOOT_COMPLETED:
                logger.debug("case BOOT_COMPLETED");
                ensureInitDone();
                isEABServiceInitializing = false;
                break;
            case CONTACT_TABLE_MODIFIED:
                logger.debug("case CONTACT_TABLE_MODIFIED");
                validateAndSyncFromContactsDb();
                break;
            case CONTACT_PROFILE_TABLE_MODIFIED:
                logger.debug("case CONTACT_PROFILE_TABLE_MODIFIED");
                validateAndSyncFromProfileDb();
                break;
            case EAB_DATABASE_RESET:
                // Initialise EABProvider.
                logger.debug("Resetting timestamp values in shared pref.");
                SharedPrefUtil.resetEABSharedPref(mContext);
                // init the EAB db after re-setting.
                ensureInitDone();
                break;
            default:
                logger.debug("default usecase hit! Do nothing");
                break;
            }
            logger.debug("Exit: handleMessage");
        }
    }

    // synchronized is used to prevent sync happening in parallel due to
    // multiple content change notifys from contacts observer.
    private synchronized void validateAndSyncFromContactsDb() {
        logger.debug("Enter : validateAndSyncFromContactsDb()");
        checkForContactNumberChanges();
        checkForDeletedContact();
        logger.debug("Exit : validateAndSyncFromContactsDb()");
    }

    // synchronized is used to prevent sync happening in parallel due to
    // multiple content change notify from contacts observer.
    private synchronized void validateAndSyncFromProfileDb() {
        logger.debug("Enter : validateAndSyncFromProfileDb()");
        checkForProfileNumberChanges();
        checkForDeletedProfileContacts();
        logger.debug("Exit : validateAndSyncFromProfileDb()");
    }

    private void ensureInitDone() {
        logger.debug("Enter : ensureInitDone()");
        if(SharedPrefUtil.isInitDone(mContext)) {
            logger.debug("EAB initialized already!!! Just Sync with Contacts db.");
            validateAndSyncFromContactsDb();
            validateAndSyncFromProfileDb();
            return;
        } else {
            logger.debug("Initializing EAB Provider.");
            // This API will sync the numbers from Contacts db to EAB db based on
            // contact last updated timestamp.
            EABDbUtil.validateAndSyncFromContactsDb(mContext);
            // This API's will sync the profile numbers from Contacts db to EAB db based on
            // contact last updated timestamp.
            validateAndSyncFromProfileDb();
            SharedPrefUtil.setInitDone(mContext, true);
        }
    }

    private void sendEABResetMessage() {
        logger.debug("Enter: sendEABResetMsg()");
        if (null != mServiceHandler) {
            if (mServiceHandler.hasMessages(EAB_DATABASE_RESET)) {
                mServiceHandler.removeMessages(EAB_DATABASE_RESET);
                logger.debug("Removed previous EAB_DATABASE_RESET msg.");
            }

            logger.debug("Sending new EAB_DATABASE_RESET msg.");
            Message msg = mServiceHandler.obtainMessage(EAB_DATABASE_RESET);
            mServiceHandler.sendMessage(msg);
        }
    }

    private void sendDelayedContactChangeMsg() {
        logger.debug("Enter: sendDelayedContactChangeMsg()");
        if (null != mServiceHandler && !isEABServiceInitializing) {
            // Remove any previous message for CONTACT_TABLE_MODIFIED.
            if (mServiceHandler.hasMessages(CONTACT_TABLE_MODIFIED)) {
                mServiceHandler.removeMessages(CONTACT_TABLE_MODIFIED);
                logger.debug("Removed previous CONTACT_TABLE_MODIFIED msg.");
            }

            logger.debug("Sending new CONTACT_TABLE_MODIFIED msg.");
            // Send a new delayed message for CONTACT_TABLE_MODIFIED.
            Message msg = mServiceHandler.obtainMessage(CONTACT_TABLE_MODIFIED);
            mServiceHandler.sendMessageDelayed(msg, SYNC_COMPLETE_DELAY_TIMER);
        }
    }

    private void sendDelayedContactProfileMsg() {
        logger.debug("Enter: sendDelayedContactProfileMsg()");
        if (null != mServiceHandler && !isEABServiceInitializing) {
            // Remove any previous message for CONTACT_PROFILE_TABLE_MODIFIED.
            if (mServiceHandler.hasMessages(CONTACT_PROFILE_TABLE_MODIFIED)) {
                mServiceHandler.removeMessages(CONTACT_PROFILE_TABLE_MODIFIED);
                logger.debug("Removed previous CONTACT_PROFILE_TABLE_MODIFIED msg.");
            }

            logger.debug("Sending new CONTACT_PROFILE_TABLE_MODIFIED msg.");
            // Send a new delayed message for CONTACT_PROFILE_TABLE_MODIFIED.
            Message msg = mServiceHandler.obtainMessage(CONTACT_PROFILE_TABLE_MODIFIED);
            mServiceHandler.sendMessageDelayed(msg, SYNC_COMPLETE_DELAY_TIMER);
        }
    }

    private void checkForContactNumberChanges() {
        logger.debug("Enter: checkForContactNumberChanges()");
        String[] projection = new String[] {
                ContactsContract.Data._ID,
                ContactsContract.Data.CONTACT_ID,
                ContactsContract.Data.RAW_CONTACT_ID,
                ContactsContract.Data.MIMETYPE,
                ContactsContract.Data.DATA1,
                ContactsContract.Data.DISPLAY_NAME,
                ContactsContract.Data.CONTACT_LAST_UPDATED_TIMESTAMP };

        long contactLastChange = SharedPrefUtil.getLastContactChangedTimestamp(mContext, 0);
        logger.debug("contactLastChange : " + contactLastChange);

        String selection = ContactsContract.Data.MIMETYPE + " = '" + Phone.CONTENT_ITEM_TYPE +
                "' AND " + ContactsContract.Data.CONTACT_LAST_UPDATED_TIMESTAMP  + " > '" +
                contactLastChange + "'";
        String sortOrder = ContactsContract.Data.CONTACT_LAST_UPDATED_TIMESTAMP + " desc";
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(ContactsContract.Data.CONTENT_URI,
                    projection, selection, null, sortOrder);

            if (null != cursor) {
                int count = cursor.getCount();
                logger.debug("cursor count : " + count);
                if (count > 0) {
                    ArrayList<Long> uniqueRawContactIds = new ArrayList<Long>();
                    while (cursor.moveToNext()) {
                        Long dataId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                                ContactsContract.Data._ID)));
                        Long contactId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                                ContactsContract.Data.CONTACT_ID)));
                        Long rawContactId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                                ContactsContract.Data.RAW_CONTACT_ID)));
                        String phoneNumber = cursor.getString(cursor.getColumnIndex(
                                ContactsContract.Data.DATA1));
                        String displayName = cursor.getString(cursor.getColumnIndex(
                               ContactsContract.Data.DISPLAY_NAME));
                        logger.debug("dataId : " + dataId + " rawContactId :"  + rawContactId +
                               " contactId : " + contactId
                               + " phoneNumber :" + phoneNumber + " displayName :" + displayName);
                        verifyInsertOrUpdateAction(dataId, contactId, rawContactId, phoneNumber,
                              displayName);
                        if (uniqueRawContactIds.isEmpty()) {
                            uniqueRawContactIds.add(rawContactId);
                        } else if (!uniqueRawContactIds.contains(rawContactId)) {
                            uniqueRawContactIds.add(rawContactId);
                        } else {
                            // Do nothing.
                            logger.debug("uniqueRawContactIds already contains rawContactId : " +
                                    rawContactId);
                        }
                    }
                    checkForPhoneNumberDelete(uniqueRawContactIds);
                    // Save the largest timestamp returned. Only need the first one due to
                    // the sort order.
                    cursor.moveToFirst();
                    long timestamp = cursor.getLong(cursor
                            .getColumnIndex(ContactsContract.Data.CONTACT_LAST_UPDATED_TIMESTAMP));
                    if (timestamp > 0) {
                        SharedPrefUtil.saveLastContactChangedTimestamp(mContext, timestamp);
                    }
                }
            } else {
                logger.error("cursor is null!");
            }
        } catch (Exception e) {
            logger.error("checkForContactNumberChanges() exception:", e);
        } finally {
            if (null != cursor) {
                cursor.close();
            }
        }
        logger.debug("Exit: checkForContactNumberChanges()");
    }

    private void verifyInsertOrUpdateAction(Long dataId, Long contactId,
            Long rawContactId, String phoneNumber, String displayName) {
        logger.debug("Enter: verifyInsertOrUpdateAction() phoneNumber : " + phoneNumber);
        if (null == phoneNumber){
            logger.error("Error: return as phoneNumber is null");
            return;
        }
        // Check if the contact is already available in EAB Provider.
        String[] eabProjection = new String[] {
                EABContract.EABColumns.CONTACT_NUMBER,
                EABContract.EABColumns.CONTACT_NAME };
        String eabWhereClause = EABContract.EABColumns.DATA_ID + " ='" + dataId.toString()
                + "' AND " + EABContract.EABColumns.RAW_CONTACT_ID + " ='"
                + rawContactId.toString() + "' AND " + EABContract.EABColumns.CONTACT_ID
                + " ='" + contactId.toString() + "'";
        logger.debug("eabWhereClause : " + eabWhereClause);

        Cursor eabCursor = getContentResolver().query(EABContract.EABColumns.CONTENT_URI,
                eabProjection, eabWhereClause, null, null);
        if (null != eabCursor) {
            int eabCursorCount = eabCursor.getCount();
            logger.debug("EAB cursor count : " + eabCursorCount);
            if (eabCursorCount > 0) {
                while (eabCursor.moveToNext()) {
                    // EABProvider has entry for dataId & rawContactId. Try to
                    // match the contact number.
                    String eabPhoneNumber = eabCursor.getString(eabCursor
                                    .getColumnIndex(EABContract.EABColumns.CONTACT_NUMBER));
                    String eabDisplayName = eabCursor.getString(eabCursor
                            .getColumnIndex(EABContract.EABColumns.CONTACT_NAME));
                    logger.debug("phoneNumber : " + phoneNumber
                            + " eabPhoneNumber :" + eabPhoneNumber);
                    // If contact number do not match, then update EAB database with the new
                    // number. If contact name do not match, then update EAB database with the
                    // new name.
                    if (null != eabPhoneNumber) {
                        if (!phoneNumber.equals(eabPhoneNumber)) {
                            // Update use-case.
                            handlePhoneNumberChanged(dataId, contactId, rawContactId,
                                    eabPhoneNumber, phoneNumber, displayName);
                        } else if (!TextUtils.equals(displayName, eabDisplayName)) {
                            // Update name use-case.
                            handlePhoneNameUpdate(dataId, contactId, rawContactId,
                                    phoneNumber, displayName);
                        } else {
                            // Do nothing.
                            logger.debug("The contact name and number is already available " +
                                    "in EAB Provider.");
                        }
                    }
                }
            } else {
                // insert use-case.
                handlePhoneNumberInsertion(dataId, contactId, rawContactId, phoneNumber,
                        displayName);
            }
        }
        if (null != eabCursor) {
            eabCursor.close();
        }
        logger.debug("Exit: verifyInsertOrUpdateAction()");
    }

    private void checkForPhoneNumberDelete(ArrayList<Long> uniqueRawContactIds) {
        logger.debug("Enter: checkForPhoneNumberDelete() ");
        if (null != uniqueRawContactIds && uniqueRawContactIds.size() > 0) {
            for (int i = 0; i < uniqueRawContactIds.size(); i++) {
                Long rawContactId = uniqueRawContactIds.get(i);
                int contactsDbCount = 0;
                int eabDbCursorCount = 0;

                // Find the total number of dataIds under the rawContactId in
                // Contacts Provider DB.
                String[] projection = new String[] { ContactsContract.Data._ID,
                        ContactsContract.Data.CONTACT_ID,
                        ContactsContract.Data.RAW_CONTACT_ID,
                        ContactsContract.Data.MIMETYPE,
                        ContactsContract.Data.DATA1,
                        ContactsContract.Data.DISPLAY_NAME };

                // Get LastContactChangedTimestamp for knowing which contact
                // number deleted from the contact id.
                long contactLastChange = SharedPrefUtil.getLastContactChangedTimestamp(
                        mContext, 0);

                String selection = ContactsContract.Data.MIMETYPE + " = '"
                        + Phone.CONTENT_ITEM_TYPE + "' AND " +
                        ContactsContract.Data.CONTACT_LAST_UPDATED_TIMESTAMP
                        + " > '" + contactLastChange + "' AND " +
                        ContactsContract.Data.RAW_CONTACT_ID + " = '"
                        + rawContactId + "'";

                String sortOrder = ContactsContract.Data.RAW_CONTACT_ID + " desc";

                Cursor contactDbCursor = getContentResolver().query(
                        ContactsContract.Data.CONTENT_URI, projection,
                        selection, null, sortOrder);

                if (null != contactDbCursor) {
                    contactsDbCount = contactDbCursor.getCount();
                    logger.debug("contactDbCursor count : " + contactsDbCount);
                }

                // Find the total number of dataIds under the rawContactId in
                // EAB Provider DB.
                String[] eabProjection = new String[] {
                        EABContract.EABColumns.CONTACT_ID,
                        EABContract.EABColumns.RAW_CONTACT_ID,
                        EABContract.EABColumns.DATA_ID,
                        EABContract.EABColumns.CONTACT_NUMBER,
                        EABContract.EABColumns.CONTACT_NAME };

                String eabWhereClause = EABContract.EABColumns.RAW_CONTACT_ID
                        + " ='" + rawContactId.toString() + "'";

                Cursor eabDbCursor = getContentResolver().query(
                        EABContract.EABColumns.CONTENT_URI, eabProjection,
                        eabWhereClause, null, null);
                if (null != eabDbCursor) {
                    eabDbCursorCount = eabDbCursor.getCount();
                    logger.debug("eabDbCursor count : " + eabDbCursorCount);
                }
                if (0 == contactsDbCount && 0 == eabDbCursorCount) {
                    // Error scenario. Continue for checking the next rawContactId.
                    logger.error("Both cursor counts are 0. move to next rawContactId");
                } else {
                    if (contactsDbCount == eabDbCursorCount) {
                        // Do nothing as both DB have the same number of contacts.
                        logger.debug("Both the databases have the same number of contacts." +
                                " Do nothing.");
                    } else if (contactsDbCount > eabDbCursorCount) {
                        logger.error("EAB DB has less contacts then Contacts DB. Do nothing!");
                    } else if (contactsDbCount < eabDbCursorCount) {
                        // find and number and delete it from EAB Provider.
                        logger.debug("Delete usecase hit. Find and delete contact from EAB DB.");
                        ArrayList <Long> eabDataIdList = new ArrayList <Long>();
                        while (eabDbCursor.moveToNext()) {
                            String eabPhoneNumber = eabDbCursor.getString(eabDbCursor
                                    .getColumnIndex(EABContract.EABColumns.CONTACT_NUMBER));
                            logger.debug("eabPhoneNumber :" + eabPhoneNumber);
                            Long eabDataId = Long.valueOf(eabDbCursor.getLong(eabDbCursor
                                    .getColumnIndex(EABContract.EABColumns.DATA_ID)));
                            logger.debug("eabDataId :" + eabDataId);
                            if (eabDataIdList.isEmpty()) {
                                eabDataIdList.add(eabDataId);
                            } else if (!eabDataIdList.contains(eabDataId) )  {
                                eabDataIdList.add(eabDataId);
                            } else {
                                // Something is wrong. There can not be duplicate numbers.
                                logger.error("Duplicate entry for PhoneNumber :" + eabPhoneNumber
                                        + " with DataId : " + eabDataId + " found in EABProvider.");
                            }
                        }
                        logger.debug("Before computation eabDataIdList size :" +
                                eabDataIdList.size());
                        while (contactDbCursor.moveToNext()) {
                            String contactPhoneNumber = contactDbCursor.getString(contactDbCursor
                                    .getColumnIndex(ContactsContract.Data.DATA1));
                            Long contactDataId = Long.valueOf(contactDbCursor.getLong(
                                    contactDbCursor
                                            .getColumnIndex(ContactsContract.Data._ID)));
                            logger.debug("contactPhoneNumber : " + contactPhoneNumber +
                                    " dataId : " + contactDataId);
                            if (eabDataIdList.contains(contactDataId) )  {
                                eabDataIdList.remove(contactDataId);
                                logger.debug("Number removed from eabDataIdList");
                            } else {
                                // Something is wrong. There can not be new number in Contacts DB.
                                logger.error("Number :" + contactPhoneNumber
                                        + " with DataId : " + contactDataId +
                                        " not found in EABProvider.");
                            }
                        }
                        logger.debug("After computation eabPhoneNumberList size :" +
                                eabDataIdList.size());
                        if (eabDataIdList.size() > 0) {
                            handlePhoneNumbersDeleted(rawContactId, eabDataIdList);
                        }
                    }
                }
                if (null != contactDbCursor) {
                    contactDbCursor.close();
                }
                if (null != eabDbCursor) {
                    eabDbCursor.close();
                }
            }
        } else {
            // Do nothing.
            logger.debug("uniqueRawContactIds is null or empty. Do nothing. ");
        }
        logger.debug("Exit: checkForPhoneNumberDelete() ");
    }

    private void checkForDeletedContact() {
        logger.debug("Enter: checkForDeletedContact()");
        String[] projection = new String[] {
                ContactsContract.DeletedContacts.CONTACT_ID,
                ContactsContract.DeletedContacts.CONTACT_DELETED_TIMESTAMP };

        long contactLastDeleted = SharedPrefUtil.getLastContactDeletedTimestamp(mContext, 0);
        logger.debug("contactLastDeleted : " + contactLastDeleted);

        String selection = ContactsContract.DeletedContacts.CONTACT_DELETED_TIMESTAMP
                + " > '" + contactLastDeleted + "'";

        String sortOrder = ContactsContract.DeletedContacts.CONTACT_DELETED_TIMESTAMP + " desc";

        Cursor cursor = getContentResolver().query(
                ContactsContract.DeletedContacts.CONTENT_URI, projection,
                selection, null, sortOrder);
        if (null != cursor) {
            int count = cursor.getCount();
            logger.debug("cursor count : " + count);
            if (count > 0) {
                while (cursor.moveToNext()) {
                    Long contactId = Long.valueOf(cursor.getLong(cursor
                                    .getColumnIndex(ContactsContract.DeletedContacts.CONTACT_ID)));
                    logger.debug("contactId : " + contactId);
                    handleContactDeleted(contactId);
                }
                // Save the largest returned timestamp. Only need the first
                // cursor element due to the sort order.
                cursor.moveToFirst();
                long timestamp = cursor.getLong(cursor
                        .getColumnIndex(
                        ContactsContract.DeletedContacts.CONTACT_DELETED_TIMESTAMP));
                if (timestamp > 0) {
                    SharedPrefUtil.saveLastContactDeletedTimestamp(mContext, timestamp);
                }
            }
        }
        if (null != cursor) {
            cursor.close();
        }
        logger.debug("Exit: checkForDeletedContact()");
    }

    private void checkForProfileNumberChanges() {
        logger.debug("Enter: checkForProfileNumberChanges()");
        String[] projection = new String[] {
                ContactsContract.Contacts.Entity.CONTACT_ID,
                ContactsContract.Contacts.Entity.RAW_CONTACT_ID,
                ContactsContract.Contacts.Entity.DATA_ID,
                ContactsContract.Contacts.Entity.MIMETYPE,
                ContactsContract.Contacts.Entity.DATA1,
                ContactsContract.Contacts.Entity.DISPLAY_NAME,
                ContactsContract.Contacts.Entity.CONTACT_LAST_UPDATED_TIMESTAMP};

        long profileContactLastChange = SharedPrefUtil.getLastProfileContactChangedTimestamp(
                mContext, 0);
        logger.debug("profileContactLastChange : " + profileContactLastChange);

        String selection = ContactsContract.Contacts.Entity.MIMETYPE + " ='" +
                Phone.CONTENT_ITEM_TYPE + "' AND "
                + ContactsContract.Contacts.Entity.CONTACT_LAST_UPDATED_TIMESTAMP  + " > '" +
                profileContactLastChange + "'";
        String sortOrder = ContactsContract.Contacts.Entity.CONTACT_LAST_UPDATED_TIMESTAMP +
                " desc";
        // Construct the Uri to access Profile's Entity view.
        Uri profileUri = ContactsContract.Profile.CONTENT_URI;
        Uri entiryUri = Uri.withAppendedPath(profileUri,
                ContactsContract.Contacts.Entity.CONTENT_DIRECTORY);

        Cursor cursor = getContentResolver().query(entiryUri, projection, selection, null,
                sortOrder);
        if (null != cursor) {
            int count = cursor.getCount();
            logger.debug("cursor count : " + count);
            if (count > 0) {
                ArrayList <String> profileNumberList = new ArrayList <String>();
                ArrayList <Long> profileDataIdList = new ArrayList <Long>();
                Long contactId = null;
                Long rawContactId = null;
                while (cursor.moveToNext()) {
                    contactId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                            ContactsContract.Contacts.Entity.CONTACT_ID)));
                    rawContactId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                            ContactsContract.Contacts.Entity.RAW_CONTACT_ID)));
                    Long dataId = Long.valueOf(cursor.getLong(cursor.getColumnIndex(
                            ContactsContract.Contacts.Entity.DATA_ID)));
                    String contactNumber = cursor.getString(cursor.getColumnIndex(
                            ContactsContract.Contacts.Entity.DATA1));
                    String profileName = cursor.getString(cursor.getColumnIndex(
                            ContactsContract.Contacts.Entity.DISPLAY_NAME));
                    logger.debug("Profile Name : " + profileName
                            + " Profile Number : " + contactNumber
                            + " profile dataId : " + dataId
                            + " profile rawContactId : " + rawContactId
                            + " profile contactId : " + contactId);
                    if (profileDataIdList.isEmpty()) {
                        profileDataIdList.add(dataId);
                        profileNumberList.clear();
                        profileNumberList.add(contactNumber);
                    } else if (!profileDataIdList.contains(dataId)) {
                        profileDataIdList.add(dataId);
                        profileNumberList.add(contactNumber);
                    } else {
                        // There are duplicate entries in Profile's Table
                        logger.error("Duplicate entry in Profile's Table for contact :" +
                                contactNumber + " dataId : " + dataId);
                    }
                    verifyInsertOrUpdateAction(dataId, contactId, rawContactId, contactNumber,
                            profileName);
                }
                checkForProfilePhoneNumberDelete(contactId, rawContactId, profileDataIdList);
                // Save the largest timestamp returned. Only need the first cursor element
                // due to sort order.
                cursor.moveToFirst();
                long timestamp = cursor.getLong(cursor.getColumnIndex(
                        ContactsContract.Contacts.Entity.CONTACT_LAST_UPDATED_TIMESTAMP));
                if (timestamp > 0) {
                    SharedPrefUtil.saveLastProfileContactChangedTimestamp(mContext, timestamp);
                }
            } else {
                logger.error("cursor is zero. Do nothing.");
            }
        } else {
            logger.error("ContactsContract.Profile.CONTENT_URI cursor is null!");
        }
        if (null != cursor) {
            cursor.close();
        }
        logger.debug("Exit: checkForProfileNumberChanges()");
    }

    private void checkForProfilePhoneNumberDelete(Long profileContactId,
            Long profileRawContactId, ArrayList<Long> profileDataIdList) {
        logger.debug("Enter: checkForProfilePhoneNumberDelete()");
        if (!ContactsContract.isProfileId(profileContactId)) {
            logger.error("Not a Profile Contact Id : " + profileContactId);
            return;
        }
        int eabDbCursorCount = 0;
        int profileContactsDbCount = profileDataIdList.size();
        logger.error("profileContactsDbCount size : " + profileContactsDbCount);
        // Find the total number of dataIds under the rawContactId in EAB Provider DB.
        String[] eabProjection = new String[] {
                EABContract.EABColumns.CONTACT_ID,
                EABContract.EABColumns.DATA_ID,
                EABContract.EABColumns.CONTACT_NUMBER};
        String eabWhereClause = EABContract.EABColumns.CONTACT_ID + " ='" +
                profileContactId.toString() + "'";

        Cursor eabDbCursor = getContentResolver().query( EABContract.EABColumns.CONTENT_URI,
                eabProjection,
                eabWhereClause, null, null);
        if (null != eabDbCursor) {
            eabDbCursorCount = eabDbCursor.getCount();
            logger.debug("eabDbCursor count : " + eabDbCursorCount);
        }
        if (0 == profileContactsDbCount && 0 == eabDbCursorCount) {
            // Error scenario. Continue for checking the next rawContactId.
            logger.error("Both cursor counts are 0. Do nothing");
        } else {
            if (profileContactsDbCount == eabDbCursorCount) {
                // Do nothing as both DB have the same number of contacts.
                logger.debug("Both the databases have the same number of contacts. Do nothing.");
            } else if (profileContactsDbCount > eabDbCursorCount) {
                logger.error("EAB DB has less contacts then Contacts DB. Do nothing!");
            } else if (profileContactsDbCount < eabDbCursorCount) {
                // find and number and delete it from EAB Provider.
                logger.debug("Delete usecase hit. Find and delete contact from EAB DB.");
                ArrayList <Long> eabDataIdList = new ArrayList <Long>();
                while (eabDbCursor.moveToNext()) {
                    Long eabDataId = Long.valueOf(eabDbCursor.getLong(eabDbCursor
                                    .getColumnIndex(EABContract.EABColumns.DATA_ID)));
                    logger.debug("eabDataId : " + eabDataId);
                    if (eabDataIdList.isEmpty()) {
                        eabDataIdList.add(eabDataId);
                    } else if (!eabDataIdList.contains(eabDataId) )  {
                        eabDataIdList.add(eabDataId);
                    } else {
                        // Something is wrong. There can not be duplicate numbers.
                        logger.error("Duplicate entry for eabDataId in EABProvider : " +
                                eabDataId);
                    }
                }
                logger.debug("Before computation eabDataIdList size : " + eabDataIdList.size());
                for (int i = 0; i < profileDataIdList.size(); i++) {
                    Long contactDataId = profileDataIdList.get(i);
                    logger.debug("Profile contactDataId : " + contactDataId);
                    if (eabDataIdList.contains(contactDataId) )  {
                        eabDataIdList.remove(contactDataId);
                        logger.debug("Number removed from eabDataIdList");
                    } else {
                        // Something is wrong. There can not be new number in Contacts DB.
                        logger.error("DataId : " + contactDataId +
                                " not found in EAB Provider DB.");
                    }
                }
                logger.debug("After computation eabDataIdList size : " + eabDataIdList.size());
                if (eabDataIdList.size() > 0) {
                    handlePhoneNumbersDeleted(profileRawContactId, eabDataIdList);
                }
            }
        }
        if (null != eabDbCursor) {
            eabDbCursor.close();
        }
        logger.debug("Exit: checkForProfilePhoneNumberDelete() ");
    }

    private void checkForDeletedProfileContacts() {
        logger.debug("Enter: checkForDeletedProfileContacts()");
        String[] projection = new String[] {
                ContactsContract.Contacts.Entity.DATA1,
                ContactsContract.Contacts.Entity.DISPLAY_NAME,
                ContactsContract.Contacts.Entity.CONTACT_LAST_UPDATED_TIMESTAMP};

        String selection = ContactsContract.Contacts.Entity.MIMETYPE + " ='" +
                Phone.CONTENT_ITEM_TYPE + "'";
        // Construct the Uri to access Profile's Entity view.
        Uri profileUri = ContactsContract.Profile.CONTENT_URI;
        Uri entiryUri = Uri.withAppendedPath(profileUri,
                ContactsContract.Contacts.Entity.CONTENT_DIRECTORY);

        // Due to issue in AOSP contact profile db, table
        // ContactsContract.Profile.CONTENT_URI can not be checked for
        // selection = ContactsContract.Profile.HAS_PHONE_NUMBER + " = '" + 1 + "'".
        // This is resulting in 0 cursor count even when there are valid
        // contact numbers under contacts profile db.
        Cursor cursor = getContentResolver().query(entiryUri, projection, selection, null, null);
        if (null != cursor) {
            int count = cursor.getCount();
            logger.debug("Profile contact cursor count : " + count);
            if (count == 0) {
                logger.debug("cursor count is Zero. There are no contacts in Contact Profile db.");
                handleContactProfileDeleted();
            } else {
                logger.debug("Profile is available. Do nothing");
            }
            cursor.close();
        }
        logger.debug("Exit: checkForDeletedProfileContacts()");
    }

    private void handlePhoneNumberInsertion(Long dataId, Long contactId,
            Long rawContactId, String phoneNumber, String contactName) {

        logger.debug("handlePhoneNumberInsertion() rawContactId : "
                + rawContactId + " dataId :" + dataId + " contactId :"
                + contactId + " phoneNumber :" + phoneNumber + " contactName :"
                + contactName);
        if (!EABDbUtil.validateEligibleContact(mContext, phoneNumber)) {
            logger.debug("Return as number is not elegible for VT.");
            return;
        }
        String sRawContactId = null;
        String sDataId = null;
        String sContactId = null;
        if (null != rawContactId) {
            sRawContactId = rawContactId.toString();
        }
        if (null != dataId) {
            sDataId = dataId.toString();
        }
        if (null != contactId) {
            sContactId = contactId.toString();
        }
        String formattedNumber = EABDbUtil.formatNumber(phoneNumber);
        ArrayList<PresenceContact> contactListToInsert = new ArrayList<PresenceContact>();
        contactListToInsert.add(new PresenceContact(contactName, phoneNumber, formattedNumber,
                sRawContactId, sContactId, sDataId));

        EABDbUtil.addContactsToEabDb(getApplicationContext(),
                contactListToInsert);
    }

    private void handlePhoneNumberChanged(Long dataId, Long contactId,
            Long rawContactId, String oldPhoneNumber, String newPhoneNumber,
            String contactName) {

        logger.debug("handlePhoneNumberChanged() rawContactId : " + rawContactId
                + " dataId :" + dataId + " oldPhoneNumber :" + oldPhoneNumber
                + " newPhoneNumber :" + newPhoneNumber + " contactName :"
                + contactName);

        if (null == oldPhoneNumber && null == newPhoneNumber) {
            logger.debug("Both old and new numbers are null.");
            return;
        }

        ArrayList<PresenceContact> contactListToInsert = new ArrayList<PresenceContact>();
        ArrayList<PresenceContact> contactListToDelete = new ArrayList<PresenceContact>();
        String sRawContactId = null;
        String sDataId = null;
        String sContactId = null;
        if (null != rawContactId) {
            sRawContactId = rawContactId.toString();
        }
        if (null != dataId) {
            sDataId = dataId.toString();
        }
        if (null != contactId) {
            sContactId = contactId.toString();
        }
        String newFormattedNumber = EABDbUtil.formatNumber(newPhoneNumber);
        logger.debug("newFormattedNumber : " + newFormattedNumber);
        logger.debug("Removing old number and inserting new number in EABProvider.");
        if (null != oldPhoneNumber) {
            contactListToDelete.add(new PresenceContact(contactName,
                    oldPhoneNumber, null /*formattedNumber*/, sRawContactId, sContactId, sDataId));
            // Delete old number from EAB Presence Table
            EABDbUtil.deleteNumbersFromEabDb(getApplicationContext(), contactListToDelete);
        }
        if (null != newPhoneNumber) {
            if (EABDbUtil.validateEligibleContact(mContext, newPhoneNumber)) {
                contactListToInsert.add(new PresenceContact(contactName,
                        newPhoneNumber, newFormattedNumber, sRawContactId, sContactId, sDataId));
                // Insert new number from EAB Presence Table
                EABDbUtil.addContactsToEabDb(getApplicationContext(), contactListToInsert);
            } else {
                logger.debug("Return as number is not elegible for VT.");
            }
        }
    }

    private void handlePhoneNumbersDeleted(Long rawContactId, ArrayList <Long> contactDataIdList) {
        ArrayList<PresenceContact> phoneNumberToDeleteList = new ArrayList<PresenceContact>();
        for (int v = 0; v < contactDataIdList.size(); v++) {
            Long staleDataId = contactDataIdList.get(v);
            if (null != staleDataId) {
                logger.debug("calling delete for staleNumber :" + staleDataId);
                String sRawContactId = null;
                if (null != rawContactId) {
                    sRawContactId = rawContactId.toString();
                }
                phoneNumberToDeleteList.add(new PresenceContact(null, null, null,
                        sRawContactId, null, staleDataId.toString()));
            }
        }
        // Delete number from EAB Provider table.
        EABDbUtil.deleteNumbersFromEabDb(getApplicationContext(), phoneNumberToDeleteList);
    }

    private void handlePhoneNameUpdate(Long dataId, Long contactId,
            Long rawContactId, String phoneNumber, String newDisplayName) {
        logger.debug("handlePhoneNameUpdate() rawContactId : " + rawContactId
                + " dataId :" + dataId + " newDisplayName :" + newDisplayName);
        String sRawContactId = null;
        String sDataId = null;
        String sContactId = null;
        if (null != rawContactId) {
            sRawContactId = rawContactId.toString();
        }
        if (null != dataId) {
            sDataId = dataId.toString();
        }
        if (null != contactId) {
            sContactId = contactId.toString();
        }
        ArrayList<PresenceContact> contactNameToUpdate = new ArrayList<PresenceContact>();
        contactNameToUpdate.add(new PresenceContact(newDisplayName,
                phoneNumber, null /*formattedNumber */, sRawContactId, sContactId, sDataId));

        EABDbUtil.updateNamesInEabDb(getApplicationContext(), contactNameToUpdate);
    }

    private void handleContactDeleted(Long contactId) {

        if (null == contactId) {
            logger.debug("handleContactDeleted : contactId is null");
        }
        ArrayList<PresenceContact> contactListToDelete = new ArrayList<PresenceContact>();
        contactListToDelete.add(new PresenceContact(null, null, null, null, contactId.toString(),
                null));

        //ContactDbUtil.deleteRawContact(getApplicationContext(), contactListToDelete);
        EABDbUtil.deleteContactsFromEabDb(mContext, contactListToDelete);
    }

    private void handleContactProfileDeleted() {
        Long contactProfileMinId = Long.valueOf(ContactsContract.Profile.MIN_ID);
        logger.debug("contactProfileMinId : " + contactProfileMinId);

        ArrayList<PresenceContact> contactListToDelete = new ArrayList<PresenceContact>();
        contactListToDelete.add(new PresenceContact(null, null, null, null,
                contactProfileMinId.toString(), null));

        EABDbUtil.deleteContactsFromEabDb(mContext, contactListToDelete);
    }

    private boolean isRcsProvisioned(){
        boolean isVoLTEProvisioned = false;
        boolean isLvcProvisioned = false;
        boolean isEabProvisioned = false;
        ImsManager imsManager = null;
        ImsConfig imsConfig = null;

        // Get instance of imsManagr.
        imsManager = ImsManager.getInstance(mContext,
                SubscriptionManager.getDefaultVoiceSubscriptionId());
        try {
            imsConfig = imsManager.getConfigInterface();
            logger.debug("imsConfig initialized.");
        } catch (Exception e) {
            logger.error("getConfigInterface() exception:", e);
            imsConfig = null;
        }

        if (null != imsConfig) {
            try {
                isVoLTEProvisioned = imsConfig.getProvisionedValue(
                        ImsConfig.ConfigConstants.VLT_SETTING_ENABLED)
                        == ImsConfig.FeatureValueConstants.ON;
                isLvcProvisioned = imsConfig.getProvisionedValue(
                        ImsConfig.ConfigConstants.LVC_SETTING_ENABLED)
                        == ImsConfig.FeatureValueConstants.ON;
                isEabProvisioned = imsConfig.getProvisionedValue(
                        ImsConfig.ConfigConstants.EAB_SETTING_ENABLED)
                        == ImsConfig.FeatureValueConstants.ON;
            } catch (ImsException e) {
                logger.error("ImsException in isRcsProvisioned() : ", e);
            }
        } else {
            logger.debug("isRcsProvisioned - imsConfig is null");
        }
        logger.debug("isVoLTEProvisioned : " + isVoLTEProvisioned + " isLvcProvisioned : " +
                isLvcProvisioned
                + " isEabProvisioned : " + isEabProvisioned);
        return (isVoLTEProvisioned && isLvcProvisioned && isEabProvisioned);
    }
}
