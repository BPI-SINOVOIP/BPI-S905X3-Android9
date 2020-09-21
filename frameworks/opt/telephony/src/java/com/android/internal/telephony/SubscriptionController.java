/*
* Copyright (C) 2014 The Android Open Source Project
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

import android.Manifest;
import android.annotation.Nullable;
import android.app.AppOpsManager;
import android.content.ContentResolver;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.database.Cursor;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.net.Uri;
import android.os.Binder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.os.UserHandle;
import android.provider.Settings;
import android.telephony.RadioAccessFamily;
import android.telephony.Rlog;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.UiccAccessRule;
import android.telephony.euicc.EuiccManager;
import android.text.TextUtils;
import android.text.format.Time;
import android.util.Log;

import com.android.internal.annotations.VisibleForTesting;
import com.android.internal.telephony.IccCardConstants.State;
import com.android.internal.telephony.uicc.IccUtils;
import com.android.internal.telephony.uicc.UiccCard;
import com.android.internal.telephony.uicc.UiccController;

import java.io.FileDescriptor;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.ConcurrentHashMap;
import java.util.stream.Collectors;

/**
 * SubscriptionController to provide an inter-process communication to
 * access Sms in Icc.
 *
 * Any setters which take subId, slotIndex or phoneId as a parameter will throw an exception if the
 * parameter equals the corresponding INVALID_XXX_ID or DEFAULT_XXX_ID.
 *
 * All getters will lookup the corresponding default if the parameter is DEFAULT_XXX_ID. Ie calling
 * getPhoneId(DEFAULT_SUB_ID) will return the same as getPhoneId(getDefaultSubId()).
 *
 * Finally, any getters which perform the mapping between subscriptions, slots and phones will
 * return the corresponding INVALID_XXX_ID if the parameter is INVALID_XXX_ID. All other getters
 * will fail and return the appropriate error value. Ie calling
 * getSlotIndex(INVALID_SUBSCRIPTION_ID) will return INVALID_SIM_SLOT_INDEX and calling
 * getSubInfoForSubscriber(INVALID_SUBSCRIPTION_ID) will return null.
 *
 */
public class SubscriptionController extends ISub.Stub {
    static final String LOG_TAG = "SubscriptionController";
    static final boolean DBG = true;
    static final boolean VDBG = false;
    static final boolean DBG_CACHE = false;
    static final int MAX_LOCAL_LOG_LINES = 500; // TODO: Reduce to 100 when 17678050 is fixed
    private static final int DEPRECATED_SETTING = -1;
    private ScLocalLog mLocalLog = new ScLocalLog(MAX_LOCAL_LOG_LINES);

    /* The Cache of Active SubInfoRecord(s) list of currently in use SubInfoRecord(s) */
    private final List<SubscriptionInfo> mCacheActiveSubInfoList = new ArrayList<>();

    /**
     * Copied from android.util.LocalLog with flush() adding flush and line number
     * TODO: Update LocalLog
     */
    static class ScLocalLog {

        private LinkedList<String> mLog;
        private int mMaxLines;
        private Time mNow;

        public ScLocalLog(int maxLines) {
            mLog = new LinkedList<String>();
            mMaxLines = maxLines;
            mNow = new Time();
        }

        public synchronized void log(String msg) {
            if (mMaxLines > 0) {
                int pid = android.os.Process.myPid();
                int tid = android.os.Process.myTid();
                mNow.setToNow();
                mLog.add(mNow.format("%m-%d %H:%M:%S") + " pid=" + pid + " tid=" + tid + " " + msg);
                while (mLog.size() > mMaxLines) mLog.remove();
            }
        }

        public synchronized void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
            final int LOOPS_PER_FLUSH = 10; // Flush every N loops.
            Iterator<String> itr = mLog.listIterator(0);
            int i = 0;
            while (itr.hasNext()) {
                pw.println(Integer.toString(i++) + ": " + itr.next());
                // Flush periodically so we don't drop lines
                if ((i % LOOPS_PER_FLUSH) == 0) pw.flush();
            }
        }
    }

    private static final Comparator<SubscriptionInfo> SUBSCRIPTION_INFO_COMPARATOR =
            (arg0, arg1) -> {
                // Primary sort key on SimSlotIndex
                int flag = arg0.getSimSlotIndex() - arg1.getSimSlotIndex();
                if (flag == 0) {
                    // Secondary sort on SubscriptionId
                    return arg0.getSubscriptionId() - arg1.getSubscriptionId();
                }
                return flag;
            };

    protected final Object mLock = new Object();

    /** The singleton instance. */
    private static SubscriptionController sInstance = null;
    protected static Phone[] sPhones;
    protected Context mContext;
    protected TelephonyManager mTelephonyManager;
    protected CallManager mCM;

    private AppOpsManager mAppOps;

    // FIXME: Does not allow for multiple subs in a slot and change to SparseArray
    private static Map<Integer, Integer> sSlotIndexToSubId =
            new ConcurrentHashMap<Integer, Integer>();
    private static int mDefaultFallbackSubId = SubscriptionManager.INVALID_SUBSCRIPTION_ID;
    private static int mDefaultPhoneId = SubscriptionManager.DEFAULT_PHONE_INDEX;

    private int[] colorArr;
    private long mLastISubServiceRegTime;

    public static SubscriptionController init(Phone phone) {
        synchronized (SubscriptionController.class) {
            if (sInstance == null) {
                sInstance = new SubscriptionController(phone);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  sInstance = " + sInstance);
            }
            return sInstance;
        }
    }

    public static SubscriptionController init(Context c, CommandsInterface[] ci) {
        synchronized (SubscriptionController.class) {
            if (sInstance == null) {
                sInstance = new SubscriptionController(c);
            } else {
                Log.wtf(LOG_TAG, "init() called multiple times!  sInstance = " + sInstance);
            }
            return sInstance;
        }
    }

    public static SubscriptionController getInstance() {
        if (sInstance == null)
        {
           Log.wtf(LOG_TAG, "getInstance null");
        }

        return sInstance;
    }

    protected SubscriptionController(Context c) {
        init(c);
        migrateImsSettings();
    }

    protected void init(Context c) {
        mContext = c;
        mCM = CallManager.getInstance();
        mTelephonyManager = TelephonyManager.from(mContext);

        mAppOps = (AppOpsManager)mContext.getSystemService(Context.APP_OPS_SERVICE);

        if(ServiceManager.getService("isub") == null) {
            ServiceManager.addService("isub", this);
            mLastISubServiceRegTime = System.currentTimeMillis();
        }

        if (DBG) logdl("[SubscriptionController] init by Context");
    }

    private boolean isSubInfoReady() {
        return sSlotIndexToSubId.size() > 0;
    }

    private SubscriptionController(Phone phone) {
        mContext = phone.getContext();
        mCM = CallManager.getInstance();
        mAppOps = mContext.getSystemService(AppOpsManager.class);

        if(ServiceManager.getService("isub") == null) {
                ServiceManager.addService("isub", this);
        }

        migrateImsSettings();

        if (DBG) logdl("[SubscriptionController] init by Phone");
    }

    private void enforceModifyPhoneState(String message) {
        mContext.enforceCallingOrSelfPermission(
                android.Manifest.permission.MODIFY_PHONE_STATE, message);
    }

    /**
     * Broadcast when SubscriptionInfo has changed
     * FIXME: Hopefully removed if the API council accepts SubscriptionInfoListener
     */
     private void broadcastSimInfoContentChanged() {
        Intent intent = new Intent(TelephonyIntents.ACTION_SUBINFO_CONTENT_CHANGE);
        mContext.sendBroadcast(intent);
        intent = new Intent(TelephonyIntents.ACTION_SUBINFO_RECORD_UPDATED);
        mContext.sendBroadcast(intent);
     }

     public void notifySubscriptionInfoChanged() {
         ITelephonyRegistry tr = ITelephonyRegistry.Stub.asInterface(ServiceManager.getService(
                 "telephony.registry"));
         try {
             if (DBG) logd("notifySubscriptionInfoChanged:");
             tr.notifySubscriptionInfoChanged();
         } catch (RemoteException ex) {
             // Should never happen because its always available.
         }

         // FIXME: Remove if listener technique accepted.
         broadcastSimInfoContentChanged();
     }

    /**
     * New SubInfoRecord instance and fill in detail info
     * @param cursor
     * @return the query result of desired SubInfoRecord
     */
    private SubscriptionInfo getSubInfoRecord(Cursor cursor) {
        int id = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID));
        String iccId = cursor.getString(cursor.getColumnIndexOrThrow(
                SubscriptionManager.ICC_ID));
        int simSlotIndex = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.SIM_SLOT_INDEX));
        String displayName = cursor.getString(cursor.getColumnIndexOrThrow(
                SubscriptionManager.DISPLAY_NAME));
        String carrierName = cursor.getString(cursor.getColumnIndexOrThrow(
                SubscriptionManager.CARRIER_NAME));
        int nameSource = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.NAME_SOURCE));
        int iconTint = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.COLOR));
        String number = cursor.getString(cursor.getColumnIndexOrThrow(
                SubscriptionManager.NUMBER));
        int dataRoaming = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.DATA_ROAMING));
        // Get the blank bitmap for this SubInfoRecord
        Bitmap iconBitmap = BitmapFactory.decodeResource(mContext.getResources(),
                com.android.internal.R.drawable.ic_sim_card_multi_24px_clr);
        int mcc = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.MCC));
        int mnc = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.MNC));
        String cardId = cursor.getString(cursor.getColumnIndexOrThrow(
                SubscriptionManager.CARD_ID));
        // FIXME: consider stick this into database too
        String countryIso = getSubscriptionCountryIso(id);
        boolean isEmbedded = cursor.getInt(cursor.getColumnIndexOrThrow(
                SubscriptionManager.IS_EMBEDDED)) == 1;
        UiccAccessRule[] accessRules;
        if (isEmbedded) {
            accessRules = UiccAccessRule.decodeRules(cursor.getBlob(
                    cursor.getColumnIndexOrThrow(SubscriptionManager.ACCESS_RULES)));
        } else {
            accessRules = null;
        }

        if (VDBG) {
            String iccIdToPrint = SubscriptionInfo.givePrintableIccid(iccId);
            String cardIdToPrint = SubscriptionInfo.givePrintableIccid(cardId);
            logd("[getSubInfoRecord] id:" + id + " iccid:" + iccIdToPrint + " simSlotIndex:"
                    + simSlotIndex + " displayName:" + displayName + " nameSource:" + nameSource
                    + " iconTint:" + iconTint + " dataRoaming:" + dataRoaming
                    + " mcc:" + mcc + " mnc:" + mnc + " countIso:" + countryIso + " isEmbedded:"
                    + isEmbedded + " accessRules:" + Arrays.toString(accessRules)
                    + " cardId:" + cardIdToPrint);
        }

        // If line1number has been set to a different number, use it instead.
        String line1Number = mTelephonyManager.getLine1Number(id);
        if (!TextUtils.isEmpty(line1Number) && !line1Number.equals(number)) {
            number = line1Number;
        }
        return new SubscriptionInfo(id, iccId, simSlotIndex, displayName, carrierName,
                nameSource, iconTint, number, dataRoaming, iconBitmap, mcc, mnc, countryIso,
                isEmbedded, accessRules, cardId);
    }

    /**
     * Get ISO country code for the subscription's provider
     *
     * @param subId The subscription ID
     * @return The ISO country code for the subscription's provider
     */
    private String getSubscriptionCountryIso(int subId) {
        final int phoneId = getPhoneId(subId);
        if (phoneId < 0) {
            return "";
        }
        return mTelephonyManager.getSimCountryIsoForPhone(phoneId);
    }

    /**
     * Query SubInfoRecord(s) from subinfo database
     * @param selection A filter declaring which rows to return
     * @param queryKey query key content
     * @return Array list of queried result from database
     */
     private List<SubscriptionInfo> getSubInfo(String selection, Object queryKey) {
        if (VDBG) logd("selection:" + selection + " " + queryKey);
        String[] selectionArgs = null;
        if (queryKey != null) {
            selectionArgs = new String[] {queryKey.toString()};
        }
        ArrayList<SubscriptionInfo> subList = null;
        Cursor cursor = mContext.getContentResolver().query(SubscriptionManager.CONTENT_URI,
                null, selection, selectionArgs, null);
        try {
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    SubscriptionInfo subInfo = getSubInfoRecord(cursor);
                    if (subInfo != null)
                    {
                        if (subList == null)
                        {
                            subList = new ArrayList<SubscriptionInfo>();
                        }
                        subList.add(subInfo);
                    }
                }
            } else {
                if (DBG) logd("Query fail");
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }

        return subList;
    }

    /**
     * Find unused color to be set for new SubInfoRecord
     * @param callingPackage The package making the IPC.
     * @return RGB integer value of color
     */
    private int getUnusedColor(String callingPackage) {
        List<SubscriptionInfo> availableSubInfos = getActiveSubscriptionInfoList(callingPackage);
        colorArr = mContext.getResources().getIntArray(com.android.internal.R.array.sim_colors);
        int colorIdx = 0;

        if (availableSubInfos != null) {
            for (int i = 0; i < colorArr.length; i++) {
                int j;
                for (j = 0; j < availableSubInfos.size(); j++) {
                    if (colorArr[i] == availableSubInfos.get(j).getIconTint()) {
                        break;
                    }
                }
                if (j == availableSubInfos.size()) {
                    return colorArr[i];
                }
            }
            colorIdx = availableSubInfos.size() % colorArr.length;
        }
        return colorArr[colorIdx];
    }

    /**
     * Get the active SubscriptionInfo with the subId key
     * @param subId The unique SubscriptionInfo key in database
     * @param callingPackage The package making the IPC.
     * @return SubscriptionInfo, maybe null if its not active
     */
    @Override
    public SubscriptionInfo getActiveSubscriptionInfo(int subId, String callingPackage) {
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, subId, callingPackage, "getActiveSubscriptionInfo")) {
            return null;
        }

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            List<SubscriptionInfo> subList = getActiveSubscriptionInfoList(
                    mContext.getOpPackageName());
            if (subList != null) {
                for (SubscriptionInfo si : subList) {
                    if (si.getSubscriptionId() == subId) {
                        if (DBG) {
                            logd("[getActiveSubscriptionInfo]+ subId=" + subId + " subInfo=" + si);
                        }

                        return si;
                    }
                }
            }
            if (DBG) {
                logd("[getActiveSubInfoForSubscriber]- subId=" + subId
                        + " subList=" + subList + " subInfo=null");
            }
        } finally {
            Binder.restoreCallingIdentity(identity);
        }

        return null;
    }

    /**
     * Get the active SubscriptionInfo associated with the iccId
     * @param iccId the IccId of SIM card
     * @param callingPackage The package making the IPC.
     * @return SubscriptionInfo, maybe null if its not active
     */
    @Override
    public SubscriptionInfo getActiveSubscriptionInfoForIccId(String iccId, String callingPackage) {
        // Query the subscriptions unconditionally, and then check whether the caller has access to
        // the given subscription.
        final SubscriptionInfo si = getActiveSubscriptionInfoForIccIdInternal(iccId);

        final int subId = si != null
                ? si.getSubscriptionId() : SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, subId, callingPackage, "getActiveSubscriptionInfoForIccId")) {
            return null;
        }

        return si;
    }

    /**
     * Get the active SubscriptionInfo associated with the given iccId. The caller *must* perform
     * permission checks when using this method.
     */
    private SubscriptionInfo getActiveSubscriptionInfoForIccIdInternal(String iccId) {
        if (iccId == null) {
            return null;
        }

        final long identity = Binder.clearCallingIdentity();
        try {
            List<SubscriptionInfo> subList = getActiveSubscriptionInfoList(
                    mContext.getOpPackageName());
            if (subList != null) {
                for (SubscriptionInfo si : subList) {
                    if (iccId.equals(si.getIccId())) {
                        if (DBG)
                            logd("[getActiveSubInfoUsingIccId]+ iccId=" + iccId + " subInfo=" + si);
                        return si;
                    }
                }
            }
            if (DBG) {
                logd("[getActiveSubInfoUsingIccId]+ iccId=" + iccId
                        + " subList=" + subList + " subInfo=null");
            }
        } finally {
            Binder.restoreCallingIdentity(identity);
        }

        return null;
    }

    /**
     * Get the active SubscriptionInfo associated with the slotIndex
     * @param slotIndex the slot which the subscription is inserted
     * @param callingPackage The package making the IPC.
     * @return SubscriptionInfo, maybe null if its not active
     */
    @Override
    public SubscriptionInfo getActiveSubscriptionInfoForSimSlotIndex(int slotIndex,
            String callingPackage) {
        Phone phone = PhoneFactory.getPhone(slotIndex);
        if (phone == null) {
            if (DBG) {
                loge("[getActiveSubscriptionInfoForSimSlotIndex] no phone, slotIndex=" + slotIndex);
            }
            return null;
        }
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, phone.getSubId(), callingPackage,
                "getActiveSubscriptionInfoForSimSlotIndex")) {
            return null;
        }

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            List<SubscriptionInfo> subList = getActiveSubscriptionInfoList(
                    mContext.getOpPackageName());
            if (subList != null) {
                for (SubscriptionInfo si : subList) {
                    if (si.getSimSlotIndex() == slotIndex) {
                        if (DBG) {
                            logd("[getActiveSubscriptionInfoForSimSlotIndex]+ slotIndex="
                                    + slotIndex + " subId=" + si);
                        }
                        return si;
                    }
                }
                if (DBG) {
                    logd("[getActiveSubscriptionInfoForSimSlotIndex]+ slotIndex=" + slotIndex
                            + " subId=null");
                }
            } else {
                if (DBG) {
                    logd("[getActiveSubscriptionInfoForSimSlotIndex]+ subList=null");
                }
            }
        } finally {
            Binder.restoreCallingIdentity(identity);
        }

        return null;
    }

    /**
     * @param callingPackage The package making the IPC.
     * @return List of all SubscriptionInfo records in database,
     * include those that were inserted before, maybe empty but not null.
     * @hide
     */
    @Override
    public List<SubscriptionInfo> getAllSubInfoList(String callingPackage) {
        if (DBG) logd("[getAllSubInfoList]+");

        // This API isn't public, so no need to provide a valid subscription ID - we're not worried
        // about carrier-privileged callers not having access.
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, SubscriptionManager.INVALID_SUBSCRIPTION_ID, callingPackage,
                "getAllSubInfoList")) {
            return null;
        }

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            List<SubscriptionInfo> subList = null;
            subList = getSubInfo(null, null);
            if (subList != null) {
                if (DBG) logd("[getAllSubInfoList]- " + subList.size() + " infos return");
            } else {
                if (DBG) logd("[getAllSubInfoList]- no info return");
            }
            return subList;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Get the SubInfoRecord(s) of the currently inserted SIM(s)
     * @param callingPackage The package making the IPC.
     * @return Array list of currently inserted SubInfoRecord(s)
     */
    @Override
    public List<SubscriptionInfo> getActiveSubscriptionInfoList(String callingPackage) {
        if (!isSubInfoReady()) {
            if (DBG) logdl("[getActiveSubInfoList] Sub Controller not ready");
            return null;
        }

        boolean canReadAllPhoneState;
        try {
            canReadAllPhoneState = TelephonyPermissions.checkReadPhoneState(mContext,
                    SubscriptionManager.INVALID_SUBSCRIPTION_ID, Binder.getCallingPid(),
                    Binder.getCallingUid(), callingPackage, "getActiveSubscriptionInfoList");
        } catch (SecurityException e) {
            canReadAllPhoneState = false;
        }

        synchronized (mCacheActiveSubInfoList) {
            // If the caller can read all phone state, just return the full list.
            if (canReadAllPhoneState) {
                return new ArrayList<>(mCacheActiveSubInfoList);
            }

            // Filter the list to only include subscriptions which the caller can manage.
            return mCacheActiveSubInfoList.stream()
                    .filter(subscriptionInfo -> {
                        try {
                            return TelephonyPermissions.checkCallingOrSelfReadPhoneState(mContext,
                                    subscriptionInfo.getSubscriptionId(), callingPackage,
                                    "getActiveSubscriptionInfoList");
                        } catch (SecurityException e) {
                            return false;
                        }
                    })
                    .collect(Collectors.toList());
        }
    }

    /**
     * Refresh the cache of SubInfoRecord(s) of the currently inserted SIM(s)
     */
    @VisibleForTesting  // For mockito to mock this method
    public void refreshCachedActiveSubscriptionInfoList() {
        if (!isSubInfoReady()) {
            if (DBG_CACHE) {
                logdl("[refreshCachedActiveSubscriptionInfoList] "
                        + "Sub Controller not ready ");
            }
            return;
        }

        synchronized (mCacheActiveSubInfoList) {
            mCacheActiveSubInfoList.clear();
            List<SubscriptionInfo> activeSubscriptionInfoList = getSubInfo(
                    SubscriptionManager.SIM_SLOT_INDEX + ">=0", null);
            if (activeSubscriptionInfoList != null) {
                mCacheActiveSubInfoList.addAll(activeSubscriptionInfoList);
            }
            if (DBG_CACHE) {
                if (!mCacheActiveSubInfoList.isEmpty()) {
                    for (SubscriptionInfo si : mCacheActiveSubInfoList) {
                        logd("[refreshCachedActiveSubscriptionInfoList] Setting Cached info="
                                + si);
                    }
                } else {
                    logdl("[refreshCachedActiveSubscriptionInfoList]- no info return");
                }
            }
        }
    }

    /**
     * Get the SUB count of active SUB(s)
     * @param callingPackage The package making the IPC.
     * @return active SIM count
     */
    @Override
    public int getActiveSubInfoCount(String callingPackage) {
        // Let getActiveSubscriptionInfoList perform permission checks / filtering.
        List<SubscriptionInfo> records = getActiveSubscriptionInfoList(callingPackage);
        if (records == null) {
            if (VDBG) logd("[getActiveSubInfoCount] records null");
            return 0;
        }
        if (VDBG) logd("[getActiveSubInfoCount]- count: " + records.size());
        return records.size();
    }

    /**
     * Get the SUB count of all SUB(s) in SubscriptoinInfo database
     * @param callingPackage The package making the IPC.
     * @return all SIM count in database, include what was inserted before
     */
    @Override
    public int getAllSubInfoCount(String callingPackage) {
        if (DBG) logd("[getAllSubInfoCount]+");

        // This API isn't public, so no need to provide a valid subscription ID - we're not worried
        // about carrier-privileged callers not having access.
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, SubscriptionManager.INVALID_SUBSCRIPTION_ID, callingPackage,
                "getAllSubInfoCount")) {
            return 0;
        }

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            Cursor cursor = mContext.getContentResolver().query(SubscriptionManager.CONTENT_URI,
                    null, null, null, null);
            try {
                if (cursor != null) {
                    int count = cursor.getCount();
                    if (DBG) logd("[getAllSubInfoCount]- " + count + " SUB(s) in DB");
                    return count;
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }
            if (DBG) logd("[getAllSubInfoCount]- no SUB in DB");

            return 0;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * @return the maximum number of subscriptions this device will support at any one time.
     */
    @Override
    public int getActiveSubInfoCountMax() {
        // FIXME: This valid now but change to use TelephonyDevController in the future
        return mTelephonyManager.getSimCount();
    }

    @Override
    public List<SubscriptionInfo> getAvailableSubscriptionInfoList(String callingPackage) {
        // This API isn't public, so no need to provide a valid subscription ID - we're not worried
        // about carrier-privileged callers not having access.
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, SubscriptionManager.INVALID_SUBSCRIPTION_ID, callingPackage,
                "getAvailableSubscriptionInfoList")) {
            throw new SecurityException("Need READ_PHONE_STATE to call "
                    + " getAvailableSubscriptionInfoList");
        }

        // Now that all security checks pass, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            EuiccManager euiccManager =
                    (EuiccManager) mContext.getSystemService(Context.EUICC_SERVICE);
            if (!euiccManager.isEnabled()) {
                if (DBG) logdl("[getAvailableSubInfoList] Embedded subscriptions are disabled");
                return null;
            }

            List<SubscriptionInfo> subList = getSubInfo(
                    SubscriptionManager.SIM_SLOT_INDEX + ">=0 OR "
                            + SubscriptionManager.IS_EMBEDDED + "=1", null);

            if (subList != null) {
                subList.sort(SUBSCRIPTION_INFO_COMPARATOR);

                if (VDBG) logdl("[getAvailableSubInfoList]- " + subList.size() + " infos return");
            } else {
                if (DBG) logdl("[getAvailableSubInfoList]- no info return");
            }

            return subList;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    @Override
    public List<SubscriptionInfo> getAccessibleSubscriptionInfoList(String callingPackage) {
        EuiccManager euiccManager = (EuiccManager) mContext.getSystemService(Context.EUICC_SERVICE);
        if (!euiccManager.isEnabled()) {
            if (DBG) {
                logdl("[getAccessibleSubInfoList] Embedded subscriptions are disabled");
            }
            return null;
        }

        // Verify that the given package belongs to the calling UID.
        mAppOps.checkPackage(Binder.getCallingUid(), callingPackage);

        // Perform the operation as ourselves. If the caller cannot read phone state, they may still
        // have carrier privileges per the subscription metadata, so we always need to make the
        // query and then filter the results.
        final long identity = Binder.clearCallingIdentity();
        List<SubscriptionInfo> subList;
        try {
            subList = getSubInfo(SubscriptionManager.IS_EMBEDDED + "=1", null);
        } finally {
            Binder.restoreCallingIdentity(identity);
        }

        if (subList == null) {
            if (DBG) logdl("[getAccessibleSubInfoList] No info returned");
            return null;
        }

        // Filter the list to only include subscriptions which the (restored) caller can manage.
        List<SubscriptionInfo> filteredList = subList.stream()
                .filter(subscriptionInfo ->
                        subscriptionInfo.canManageSubscription(mContext, callingPackage))
                .sorted(SUBSCRIPTION_INFO_COMPARATOR)
                .collect(Collectors.toList());
        if (VDBG) {
            logdl("[getAccessibleSubInfoList] " + filteredList.size() + " infos returned");
        }
        return filteredList;
    }

    /**
     * Return the list of subscriptions in the database which are either:
     * <ul>
     * <li>Embedded (but see note about {@code includeNonRemovableSubscriptions}, or
     * <li>In the given list of current embedded ICCIDs (which may not yet be in the database, or
     *     which may not currently be marked as embedded).
     * </ul>
     *
     * <p>NOTE: This is not accessible to external processes, so it does not need a permission
     * check. It is only intended for use by {@link SubscriptionInfoUpdater}.
     *
     * @param embeddedIccids all ICCIDs of available embedded subscriptions. This is used to surface
     *     entries for profiles which had been previously deleted.
     * @param isEuiccRemovable whether the current ICCID is removable. Non-removable subscriptions
     *     will only be returned if the current ICCID is not removable; otherwise, they are left
     *     alone (not returned here unless in the embeddedIccids list) under the assumption that
     *     they will still be accessible when the eUICC containing them is activated.
     */
    @VisibleForTesting(visibility = VisibleForTesting.Visibility.PACKAGE)
    public List<SubscriptionInfo> getSubscriptionInfoListForEmbeddedSubscriptionUpdate(
            String[] embeddedIccids, boolean isEuiccRemovable) {
        StringBuilder whereClause = new StringBuilder();
        whereClause.append("(").append(SubscriptionManager.IS_EMBEDDED).append("=1");
        if (isEuiccRemovable) {
            // Current eUICC is removable, so don't return non-removable subscriptions (which would
            // be deleted), as these are expected to still be present on a different, non-removable
            // eUICC.
            whereClause.append(" AND ").append(SubscriptionManager.IS_REMOVABLE).append("=1");
        }
        // Else, return both removable and non-removable subscriptions. This is expected to delete
        // all removable subscriptions, which is desired as they may not be accessible.

        whereClause.append(") OR ").append(SubscriptionManager.ICC_ID).append(" IN (");
        // ICCIDs are validated to contain only numbers when passed in, and come from a trusted
        // app, so no need to escape.
        for (int i = 0; i < embeddedIccids.length; i++) {
            if (i > 0) {
                whereClause.append(",");
            }
            whereClause.append("\"").append(embeddedIccids[i]).append("\"");
        }
        whereClause.append(")");

        List<SubscriptionInfo> list = getSubInfo(whereClause.toString(), null);
        if (list == null) {
            return Collections.emptyList();
        }
        return list;
    }

    @Override
    public void requestEmbeddedSubscriptionInfoListRefresh() {
        mContext.enforceCallingOrSelfPermission(Manifest.permission.WRITE_EMBEDDED_SUBSCRIPTIONS,
                "requestEmbeddedSubscriptionInfoListRefresh");
        long token = Binder.clearCallingIdentity();
        try {
            PhoneFactory.requestEmbeddedSubscriptionInfoListRefresh(null /* callback */);
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    /**
     * Asynchronously refresh the embedded subscription info list.
     *
     * @param callback Optional callback to execute after the refresh completes. Must terminate
     *     quickly as it will be called from SubscriptionInfoUpdater's handler thread.
     */
    // No permission check needed as this is not exposed via AIDL.
    public void requestEmbeddedSubscriptionInfoListRefresh(@Nullable Runnable callback) {
        PhoneFactory.requestEmbeddedSubscriptionInfoListRefresh(callback);
    }

    /**
     * Add a new SubInfoRecord to subinfo database if needed
     * @param iccId the IccId of the SIM card
     * @param slotIndex the slot which the SIM is inserted
     * @return 0 if success, < 0 on error.
     */
    @Override
    public int addSubInfoRecord(String iccId, int slotIndex) {
        if (DBG) logdl("[addSubInfoRecord]+ iccId:" + SubscriptionInfo.givePrintableIccid(iccId) +
                " slotIndex:" + slotIndex);

        enforceModifyPhoneState("addSubInfoRecord");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            if (iccId == null) {
                if (DBG) logdl("[addSubInfoRecord]- null iccId");
                return -1;
            }

            ContentResolver resolver = mContext.getContentResolver();
            Cursor cursor = resolver.query(SubscriptionManager.CONTENT_URI,
                    new String[]{SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID,
                            SubscriptionManager.SIM_SLOT_INDEX, SubscriptionManager.NAME_SOURCE,
                            SubscriptionManager.ICC_ID, SubscriptionManager.CARD_ID},
                    SubscriptionManager.ICC_ID + "=?" + " OR " + SubscriptionManager.ICC_ID + "=?",
                            new String[]{iccId, IccUtils.getDecimalSubstring(iccId)}, null);

            boolean setDisplayName = false;
            try {
                if (cursor == null || !cursor.moveToFirst()) {
                    setDisplayName = true;
                    Uri uri = insertEmptySubInfoRecord(iccId, slotIndex);
                    if (DBG) logdl("[addSubInfoRecord] New record created: " + uri);
                } else {
                    int subId = cursor.getInt(0);
                    int oldSimInfoId = cursor.getInt(1);
                    int nameSource = cursor.getInt(2);
                    String oldIccId = cursor.getString(3);
                    String oldCardId = cursor.getString(4);
                    ContentValues value = new ContentValues();

                    if (slotIndex != oldSimInfoId) {
                        value.put(SubscriptionManager.SIM_SLOT_INDEX, slotIndex);
                    }

                    if (nameSource != SubscriptionManager.NAME_SOURCE_USER_INPUT) {
                        setDisplayName = true;
                    }

                    if (oldIccId != null && oldIccId.length() < iccId.length()
                            && (oldIccId.equals(IccUtils.getDecimalSubstring(iccId)))) {
                        value.put(SubscriptionManager.ICC_ID, iccId);
                    }

                    UiccCard card = UiccController.getInstance().getUiccCardForPhone(slotIndex);
                    if (card != null) {
                        String cardId = card.getCardId();
                        if (cardId != null && cardId != oldCardId) {
                            value.put(SubscriptionManager.CARD_ID, cardId);
                        }
                    }

                    if (value.size() > 0) {
                        resolver.update(SubscriptionManager.CONTENT_URI, value,
                                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID +
                                        "=" + Long.toString(subId), null);

                        // Refresh the Cache of Active Subscription Info List
                        refreshCachedActiveSubscriptionInfoList();
                    }

                    if (DBG) logdl("[addSubInfoRecord] Record already exists");
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }

            cursor = resolver.query(SubscriptionManager.CONTENT_URI, null,
                    SubscriptionManager.SIM_SLOT_INDEX + "=?",
                    new String[] {String.valueOf(slotIndex)}, null);
            try {
                if (cursor != null && cursor.moveToFirst()) {
                    do {
                        int subId = cursor.getInt(cursor.getColumnIndexOrThrow(
                                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID));
                        // If sSlotIndexToSubId already has the same subId for a slotIndex/phoneId,
                        // do not add it.
                        Integer currentSubId = sSlotIndexToSubId.get(slotIndex);
                        if (currentSubId == null
                                || currentSubId != subId
                                || !SubscriptionManager.isValidSubscriptionId(currentSubId)) {
                            // TODO While two subs active, if user deactivats first
                            // one, need to update the default subId with second one.

                            // FIXME: Currently we assume phoneId == slotIndex which in the future
                            // may not be true, for instance with multiple subs per slot.
                            // But is true at the moment.
                            sSlotIndexToSubId.put(slotIndex, subId);
                            int subIdCountMax = getActiveSubInfoCountMax();
                            int defaultSubId = getDefaultSubId();
                            if (DBG) {
                                logdl("[addSubInfoRecord]"
                                        + " sSlotIndexToSubId.size=" + sSlotIndexToSubId.size()
                                        + " slotIndex=" + slotIndex + " subId=" + subId
                                        + " defaultSubId=" + defaultSubId + " simCount=" + subIdCountMax);
                            }

                            // Set the default sub if not set or if single sim device
                            if (!SubscriptionManager.isValidSubscriptionId(defaultSubId)
                                    || subIdCountMax == 1) {
                                setDefaultFallbackSubId(subId);
                            }
                            // If single sim device, set this subscription as the default for everything
                            if (subIdCountMax == 1) {
                                if (DBG) {
                                    logdl("[addSubInfoRecord] one sim set defaults to subId=" + subId);
                                }
                                setDefaultDataSubId(subId);
                                setDefaultSmsSubId(subId);
                                setDefaultVoiceSubId(subId);
                            }
                        } else {
                            if (DBG) {
                                logdl("[addSubInfoRecord] currentSubId != null"
                                        + " && currentSubId is valid, IGNORE");
                            }
                        }
                        if (DBG) logdl("[addSubInfoRecord] hashmap(" + slotIndex + "," + subId + ")");
                    } while (cursor.moveToNext());
                }
            } finally {
                if (cursor != null) {
                    cursor.close();
                }
            }

            // Set Display name after sub id is set above so as to get valid simCarrierName
            int subId = getSubIdUsingPhoneId(slotIndex);
            if (!SubscriptionManager.isValidSubscriptionId(subId)) {
                if (DBG) {
                    logdl("[addSubInfoRecord]- getSubId failed invalid subId = " + subId);
                }
                return -1;
            }
            if (setDisplayName) {
                String simCarrierName = mTelephonyManager.getSimOperatorName(subId);
                String nameToSet;

                if (!TextUtils.isEmpty(simCarrierName)) {
                    nameToSet = simCarrierName;
                } else {
                    nameToSet = "CARD " + Integer.toString(slotIndex + 1);
                }

                ContentValues value = new ContentValues();
                value.put(SubscriptionManager.DISPLAY_NAME, nameToSet);
                resolver.update(SubscriptionManager.CONTENT_URI, value,
                        SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID +
                                "=" + Long.toString(subId), null);

                // Refresh the Cache of Active Subscription Info List
                refreshCachedActiveSubscriptionInfoList();

                if (DBG) logdl("[addSubInfoRecord] sim name = " + nameToSet);
            }

            // Once the records are loaded, notify DcTracker
            sPhones[slotIndex].updateDataConnectionTracker();

            if (DBG) logdl("[addSubInfoRecord]- info size=" + sSlotIndexToSubId.size());

        } finally {
            Binder.restoreCallingIdentity(identity);
        }
        return 0;
    }

    /**
     * Insert an empty SubInfo record into the database.
     *
     * <p>NOTE: This is not accessible to external processes, so it does not need a permission
     * check. It is only intended for use by {@link SubscriptionInfoUpdater}.
     *
     * <p>Precondition: No record exists with this iccId.
     */
    @VisibleForTesting(visibility = VisibleForTesting.Visibility.PACKAGE)
    public Uri insertEmptySubInfoRecord(String iccId, int slotIndex) {
        ContentResolver resolver = mContext.getContentResolver();
        ContentValues value = new ContentValues();
        value.put(SubscriptionManager.ICC_ID, iccId);
        int color = getUnusedColor(mContext.getOpPackageName());
        // default SIM color differs between slots
        value.put(SubscriptionManager.COLOR, color);
        value.put(SubscriptionManager.SIM_SLOT_INDEX, slotIndex);
        value.put(SubscriptionManager.CARRIER_NAME, "");
        UiccCard card = UiccController.getInstance().getUiccCardForPhone(slotIndex);
        if (card != null) {
            String cardId = card.getCardId();
            if (cardId != null) {
                value.put(SubscriptionManager.CARD_ID, cardId);
            } else {
                value.put(SubscriptionManager.CARD_ID, iccId);
            }
        } else {
            value.put(SubscriptionManager.CARD_ID, iccId);
        }

        Uri uri = resolver.insert(SubscriptionManager.CONTENT_URI, value);

        // Refresh the Cache of Active Subscription Info List
        refreshCachedActiveSubscriptionInfoList();

        return uri;
    }

    /**
     * Generate and set carrier text based on input parameters
     * @param showPlmn flag to indicate if plmn should be included in carrier text
     * @param plmn plmn to be included in carrier text
     * @param showSpn flag to indicate if spn should be included in carrier text
     * @param spn spn to be included in carrier text
     * @return true if carrier text is set, false otherwise
     */
    public boolean setPlmnSpn(int slotIndex, boolean showPlmn, String plmn, boolean showSpn,
                              String spn) {
        synchronized (mLock) {
            int subId = getSubIdUsingPhoneId(slotIndex);
            if (mContext.getPackageManager().resolveContentProvider(
                    SubscriptionManager.CONTENT_URI.getAuthority(), 0) == null ||
                    !SubscriptionManager.isValidSubscriptionId(subId)) {
                // No place to store this info. Notify registrants of the change anyway as they
                // might retrieve the SPN/PLMN text from the SST sticky broadcast.
                // TODO: This can be removed once SubscriptionController is not running on devices
                // that don't need it, such as TVs.
                if (DBG) logd("[setPlmnSpn] No valid subscription to store info");
                notifySubscriptionInfoChanged();
                return false;
            }
            String carrierText = "";
            if (showPlmn) {
                carrierText = plmn;
                if (showSpn) {
                    // Need to show both plmn and spn if both are not same.
                    if(!Objects.equals(spn, plmn)) {
                        String separator = mContext.getString(
                                com.android.internal.R.string.kg_text_message_separator).toString();
                        carrierText = new StringBuilder().append(carrierText).append(separator)
                                .append(spn).toString();
                    }
                }
            } else if (showSpn) {
                carrierText = spn;
            }
            setCarrierText(carrierText, subId);
            return true;
        }
    }

    /**
     * Set carrier text by simInfo index
     * @param text new carrier text
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    private int setCarrierText(String text, int subId) {
        if (DBG) logd("[setCarrierText]+ text:" + text + " subId:" + subId);

        enforceModifyPhoneState("setCarrierText");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            ContentValues value = new ContentValues(1);
            value.put(SubscriptionManager.CARRIER_NAME, text);

            int result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI,
                    value, SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=" +
                    Long.toString(subId), null);

            // Refresh the Cache of Active Subscription Info List
            refreshCachedActiveSubscriptionInfoList();

            notifySubscriptionInfoChanged();

            return result;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Set SIM color tint by simInfo index
     * @param tint the tint color of the SIM
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    @Override
    public int setIconTint(int tint, int subId) {
        if (DBG) logd("[setIconTint]+ tint:" + tint + " subId:" + subId);

        enforceModifyPhoneState("setIconTint");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            validateSubId(subId);
            ContentValues value = new ContentValues(1);
            value.put(SubscriptionManager.COLOR, tint);
            if (DBG) logd("[setIconTint]- tint:" + tint + " set");

            int result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI,
                    value, SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=" +
                            Long.toString(subId), null);

            // Refresh the Cache of Active Subscription Info List
            refreshCachedActiveSubscriptionInfoList();

            notifySubscriptionInfoChanged();

            return result;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Set display name by simInfo index
     * @param displayName the display name of SIM card
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    @Override
    public int setDisplayName(String displayName, int subId) {
        return setDisplayNameUsingSrc(displayName, subId, -1);
    }

    /**
     * Set display name by simInfo index with name source
     * @param displayName the display name of SIM card
     * @param subId the unique SubInfoRecord index in database
     * @param nameSource 0: NAME_SOURCE_DEFAULT_SOURCE, 1: NAME_SOURCE_SIM_SOURCE,
     *                   2: NAME_SOURCE_USER_INPUT, -1 NAME_SOURCE_UNDEFINED
     * @return the number of records updated
     */
    @Override
    public int setDisplayNameUsingSrc(String displayName, int subId, long nameSource) {
        if (DBG) {
            logd("[setDisplayName]+  displayName:" + displayName + " subId:" + subId
                + " nameSource:" + nameSource);
        }

        enforceModifyPhoneState("setDisplayNameUsingSrc");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            validateSubId(subId);
            String nameToSet;
            if (displayName == null) {
                nameToSet = mContext.getString(SubscriptionManager.DEFAULT_NAME_RES);
            } else {
                nameToSet = displayName;
            }
            ContentValues value = new ContentValues(1);
            value.put(SubscriptionManager.DISPLAY_NAME, nameToSet);
            if (nameSource >= SubscriptionManager.NAME_SOURCE_DEFAULT_SOURCE) {
                if (DBG) logd("Set nameSource=" + nameSource);
                value.put(SubscriptionManager.NAME_SOURCE, nameSource);
            }
            if (DBG) logd("[setDisplayName]- mDisplayName:" + nameToSet + " set");
            // TODO(b/33075886): If this is an embedded subscription, we must also save the new name
            // to the eSIM itself. Currently it will be blown away the next time the subscription
            // list is updated.

            int result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI,
                    value, SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=" +
                    Long.toString(subId), null);

            // Refresh the Cache of Active Subscription Info List
            refreshCachedActiveSubscriptionInfoList();

            notifySubscriptionInfoChanged();

            return result;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Set phone number by subId
     * @param number the phone number of the SIM
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    @Override
    public int setDisplayNumber(String number, int subId) {
        if (DBG) logd("[setDisplayNumber]+ subId:" + subId);

        enforceModifyPhoneState("setDisplayNumber");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            validateSubId(subId);
            int result;
            int phoneId = getPhoneId(subId);

            if (number == null || phoneId < 0 ||
                    phoneId >= mTelephonyManager.getPhoneCount()) {
                if (DBG) logd("[setDispalyNumber]- fail");
                return -1;
            }
            ContentValues value = new ContentValues(1);
            value.put(SubscriptionManager.NUMBER, number);

            // This function had a call to update number on the SIM (Phone.setLine1Number()) but
            // that was removed as there doesn't seem to be a reason for that. If it is added
            // back, watch out for deadlocks.

            result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI, value,
                    SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID
                            + "=" + Long.toString(subId), null);

            // Refresh the Cache of Active Subscription Info List
            refreshCachedActiveSubscriptionInfoList();

            if (DBG) logd("[setDisplayNumber]- update result :" + result);
            notifySubscriptionInfoChanged();

            return result;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Set data roaming by simInfo index
     * @param roaming 0:Don't allow data when roaming, 1:Allow data when roaming
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    @Override
    public int setDataRoaming(int roaming, int subId) {
        if (DBG) logd("[setDataRoaming]+ roaming:" + roaming + " subId:" + subId);

        enforceModifyPhoneState("setDataRoaming");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            validateSubId(subId);
            if (roaming < 0) {
                if (DBG) logd("[setDataRoaming]- fail");
                return -1;
            }
            ContentValues value = new ContentValues(1);
            value.put(SubscriptionManager.DATA_ROAMING, roaming);
            if (DBG) logd("[setDataRoaming]- roaming:" + roaming + " set");

            int result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI,
                    value, SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=" +
                    Long.toString(subId), null);

            // Refresh the Cache of Active Subscription Info List
            refreshCachedActiveSubscriptionInfoList();

            notifySubscriptionInfoChanged();

            return result;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    /**
     * Set MCC/MNC by subscription ID
     * @param mccMnc MCC/MNC associated with the subscription
     * @param subId the unique SubInfoRecord index in database
     * @return the number of records updated
     */
    public int setMccMnc(String mccMnc, int subId) {
        int mcc = 0;
        int mnc = 0;
        try {
            mcc = Integer.parseInt(mccMnc.substring(0,3));
            mnc = Integer.parseInt(mccMnc.substring(3));
        } catch (NumberFormatException e) {
            loge("[setMccMnc] - couldn't parse mcc/mnc: " + mccMnc);
        }
        if (DBG) logd("[setMccMnc]+ mcc/mnc:" + mcc + "/" + mnc + " subId:" + subId);
        ContentValues value = new ContentValues(2);
        value.put(SubscriptionManager.MCC, mcc);
        value.put(SubscriptionManager.MNC, mnc);

        int result = mContext.getContentResolver().update(SubscriptionManager.CONTENT_URI, value,
                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=" + Long.toString(subId), null);

        // Refresh the Cache of Active Subscription Info List
        refreshCachedActiveSubscriptionInfoList();

        notifySubscriptionInfoChanged();

        return result;
    }

    @Override
    public int getSlotIndex(int subId) {
        if (VDBG) printStackTrace("[getSlotIndex] subId=" + subId);

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            subId = getDefaultSubId();
        }
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            if (DBG) logd("[getSlotIndex]- subId invalid");
            return SubscriptionManager.INVALID_SIM_SLOT_INDEX;
        }

        int size = sSlotIndexToSubId.size();

        if (size == 0)
        {
            if (DBG) logd("[getSlotIndex]- size == 0, return SIM_NOT_INSERTED instead");
            return SubscriptionManager.SIM_NOT_INSERTED;
        }

        for (Entry<Integer, Integer> entry: sSlotIndexToSubId.entrySet()) {
            int sim = entry.getKey();
            int sub = entry.getValue();

            if (subId == sub)
            {
                if (VDBG) logv("[getSlotIndex]- return = " + sim);
                return sim;
            }
        }

        if (DBG) logd("[getSlotIndex]- return fail");
        return SubscriptionManager.INVALID_SIM_SLOT_INDEX;
    }

    /**
     * Return the subId for specified slot Id.
     * @deprecated
     */
    @Override
    @Deprecated
    public int[] getSubId(int slotIndex) {
        if (VDBG) printStackTrace("[getSubId]+ slotIndex=" + slotIndex);

        // Map default slotIndex to the current default subId.
        // TODO: Not used anywhere sp consider deleting as it's somewhat nebulous
        // as a slot maybe used for multiple different type of "connections"
        // such as: voice, data and sms. But we're doing the best we can and using
        // getDefaultSubId which makes a best guess.
        if (slotIndex == SubscriptionManager.DEFAULT_SIM_SLOT_INDEX) {
            slotIndex = getSlotIndex(getDefaultSubId());
            if (VDBG) logd("[getSubId] map default slotIndex=" + slotIndex);
        }

        // Check that we have a valid slotIndex
        if (!SubscriptionManager.isValidSlotIndex(slotIndex)) {
            if (DBG) logd("[getSubId]- invalid slotIndex=" + slotIndex);
            return null;
        }

        // Check if we've got any SubscriptionInfo records using slotIndexToSubId as a surrogate.
        int size = sSlotIndexToSubId.size();
        if (size == 0) {
            if (VDBG) {
                logd("[getSubId]- sSlotIndexToSubId.size == 0, return DummySubIds slotIndex="
                        + slotIndex);
            }
            return getDummySubIds(slotIndex);
        }

        // Create an array of subIds that are in this slot?
        ArrayList<Integer> subIds = new ArrayList<Integer>();
        for (Entry<Integer, Integer> entry: sSlotIndexToSubId.entrySet()) {
            int slot = entry.getKey();
            int sub = entry.getValue();
            if (slotIndex == slot) {
                subIds.add(sub);
            }
        }

        // Convert ArrayList to array
        int numSubIds = subIds.size();
        if (numSubIds > 0) {
            int[] subIdArr = new int[numSubIds];
            for (int i = 0; i < numSubIds; i++) {
                subIdArr[i] = subIds.get(i);
            }
            if (VDBG) logd("[getSubId]- subIdArr=" + subIdArr);
            return subIdArr;
        } else {
            if (DBG) logd("[getSubId]- numSubIds == 0, return DummySubIds slotIndex=" + slotIndex);
            return getDummySubIds(slotIndex);
        }
    }

    @Override
    public int getPhoneId(int subId) {
        if (VDBG) printStackTrace("[getPhoneId] subId=" + subId);
        int phoneId;

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            subId = getDefaultSubId();
            if (DBG) logdl("[getPhoneId] asked for default subId=" + subId);
        }

        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            if (VDBG) {
                logdl("[getPhoneId]- invalid subId return="
                        + SubscriptionManager.INVALID_PHONE_INDEX);
            }
            return SubscriptionManager.INVALID_PHONE_INDEX;
        }

        int size = sSlotIndexToSubId.size();
        if (size == 0) {
            phoneId = mDefaultPhoneId;
            if (DBG) logdl("[getPhoneId]- no sims, returning default phoneId=" + phoneId);
            return phoneId;
        }

        // FIXME: Assumes phoneId == slotIndex
        for (Entry<Integer, Integer> entry: sSlotIndexToSubId.entrySet()) {
            int sim = entry.getKey();
            int sub = entry.getValue();

            if (subId == sub) {
                if (VDBG) logdl("[getPhoneId]- found subId=" + subId + " phoneId=" + sim);
                return sim;
            }
        }

        phoneId = mDefaultPhoneId;
        if (DBG) {
            logdl("[getPhoneId]- subId=" + subId + " not found return default phoneId=" + phoneId);
        }
        return phoneId;

    }

    private int[] getDummySubIds(int slotIndex) {
        // FIXME: Remove notion of Dummy SUBSCRIPTION_ID.
        // I tested this returning null as no one appears to care,
        // but no connection came up on sprout with two sims.
        // We need to figure out why and hopefully remove DummySubsIds!!!
        int numSubs = getActiveSubInfoCountMax();
        if (numSubs > 0) {
            int[] dummyValues = new int[numSubs];
            for (int i = 0; i < numSubs; i++) {
                dummyValues[i] = SubscriptionManager.DUMMY_SUBSCRIPTION_ID_BASE - slotIndex;
            }
            if (VDBG) {
                logd("getDummySubIds: slotIndex=" + slotIndex
                    + " return " + numSubs + " DummySubIds with each subId=" + dummyValues[0]);
            }
            return dummyValues;
        } else {
            return null;
        }
    }

    /**
     * @return the number of records cleared
     */
    @Override
    public int clearSubInfo() {
        enforceModifyPhoneState("clearSubInfo");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            int size = sSlotIndexToSubId.size();

            if (size == 0) {
                if (DBG) logdl("[clearSubInfo]- no simInfo size=" + size);
                return 0;
            }

            sSlotIndexToSubId.clear();
            if (DBG) logdl("[clearSubInfo]- clear size=" + size);
            return size;
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    private void logvl(String msg) {
        logv(msg);
        mLocalLog.log(msg);
    }

    private void logv(String msg) {
        Rlog.v(LOG_TAG, msg);
    }

    private void logdl(String msg) {
        logd(msg);
        mLocalLog.log(msg);
    }

    private static void slogd(String msg) {
        Rlog.d(LOG_TAG, msg);
    }

    private void logd(String msg) {
        Rlog.d(LOG_TAG, msg);
    }

    private void logel(String msg) {
        loge(msg);
        mLocalLog.log(msg);
    }

    private void loge(String msg) {
        Rlog.e(LOG_TAG, msg);
    }

    @Override
    public int getDefaultSubId() {
        int subId;
        boolean isVoiceCapable = mContext.getResources().getBoolean(
                com.android.internal.R.bool.config_voice_capable);
        if (isVoiceCapable) {
            subId = getDefaultVoiceSubId();
            if (VDBG) logdl("[getDefaultSubId] isVoiceCapable subId=" + subId);
        } else {
            subId = getDefaultDataSubId();
            if (VDBG) logdl("[getDefaultSubId] NOT VoiceCapable subId=" + subId);
        }
        if (!isActiveSubId(subId)) {
            subId = mDefaultFallbackSubId;
            if (VDBG) logdl("[getDefaultSubId] NOT active use fall back subId=" + subId);
        }
        if (VDBG) logv("[getDefaultSubId]- value = " + subId);
        return subId;
    }

    @Override
    public void setDefaultSmsSubId(int subId) {
        enforceModifyPhoneState("setDefaultSmsSubId");

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultSmsSubId called with DEFAULT_SUB_ID");
        }
        if (DBG) logdl("[setDefaultSmsSubId] subId=" + subId);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_SMS_SUBSCRIPTION, subId);
        broadcastDefaultSmsSubIdChanged(subId);
    }

    private void broadcastDefaultSmsSubIdChanged(int subId) {
        // Broadcast an Intent for default sms sub change
        if (DBG) logdl("[broadcastDefaultSmsSubIdChanged] subId=" + subId);
        Intent intent = new Intent(SubscriptionManager.ACTION_DEFAULT_SMS_SUBSCRIPTION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING
                | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);
        intent.putExtra(SubscriptionManager.EXTRA_SUBSCRIPTION_INDEX, subId);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    @Override
    public int getDefaultSmsSubId() {
        int subId = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_SMS_SUBSCRIPTION,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        if (VDBG) logd("[getDefaultSmsSubId] subId=" + subId);
        return subId;
    }

    @Override
    public void setDefaultVoiceSubId(int subId) {
        enforceModifyPhoneState("setDefaultVoiceSubId");

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultVoiceSubId called with DEFAULT_SUB_ID");
        }
        if (DBG) logdl("[setDefaultVoiceSubId] subId=" + subId);
        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_VOICE_CALL_SUBSCRIPTION, subId);
        broadcastDefaultVoiceSubIdChanged(subId);
    }

    /**
     * Broadcast intent of ACTION_DEFAULT_VOICE_SUBSCRIPTION_CHANGED.
     * @hide
     */
    @VisibleForTesting(visibility = VisibleForTesting.Visibility.PRIVATE)
    public void broadcastDefaultVoiceSubIdChanged(int subId) {
        // Broadcast an Intent for default voice sub change
        if (DBG) logdl("[broadcastDefaultVoiceSubIdChanged] subId=" + subId);
        Intent intent = new Intent(TelephonyIntents.ACTION_DEFAULT_VOICE_SUBSCRIPTION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING
                | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    @Override
    public int getDefaultVoiceSubId() {
        int subId = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_VOICE_CALL_SUBSCRIPTION,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        if (VDBG) slogd("[getDefaultVoiceSubId] subId=" + subId);
        return subId;
    }

    @Override
    public int getDefaultDataSubId() {
        int subId = Settings.Global.getInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION,
                SubscriptionManager.INVALID_SUBSCRIPTION_ID);
        if (VDBG) logd("[getDefaultDataSubId] subId= " + subId);
        return subId;
    }

    @Override
    public void setDefaultDataSubId(int subId) {
        enforceModifyPhoneState("setDefaultDataSubId");

        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultDataSubId called with DEFAULT_SUB_ID");
        }

        ProxyController proxyController = ProxyController.getInstance();
        int len = sPhones.length;
        logdl("[setDefaultDataSubId] num phones=" + len + ", subId=" + subId);

        if (SubscriptionManager.isValidSubscriptionId(subId)) {
            // Only re-map modems if the new default data sub is valid
            RadioAccessFamily[] rafs = new RadioAccessFamily[len];
            boolean atLeastOneMatch = false;
            for (int phoneId = 0; phoneId < len; phoneId++) {
                Phone phone = sPhones[phoneId];
                int raf;
                int id = phone.getSubId();
                if (id == subId) {
                    // TODO Handle the general case of N modems and M subscriptions.
                    raf = proxyController.getMaxRafSupported();
                    atLeastOneMatch = true;
                } else {
                    // TODO Handle the general case of N modems and M subscriptions.
                    raf = proxyController.getMinRafSupported();
                }
                logdl("[setDefaultDataSubId] phoneId=" + phoneId + " subId=" + id + " RAF=" + raf);
                rafs[phoneId] = new RadioAccessFamily(phoneId, raf);
            }
            if (atLeastOneMatch) {
                proxyController.setRadioCapability(rafs);
            } else {
                if (DBG) logdl("[setDefaultDataSubId] no valid subId's found - not updating.");
            }
        }

        // FIXME is this still needed?
        updateAllDataConnectionTrackers();

        Settings.Global.putInt(mContext.getContentResolver(),
                Settings.Global.MULTI_SIM_DATA_CALL_SUBSCRIPTION, subId);
        broadcastDefaultDataSubIdChanged(subId);
    }

    private void updateAllDataConnectionTrackers() {
        // Tell Phone Proxies to update data connection tracker
        int len = sPhones.length;
        if (DBG) logdl("[updateAllDataConnectionTrackers] sPhones.length=" + len);
        for (int phoneId = 0; phoneId < len; phoneId++) {
            if (DBG) logdl("[updateAllDataConnectionTrackers] phoneId=" + phoneId);
            sPhones[phoneId].updateDataConnectionTracker();
        }
    }

    private void broadcastDefaultDataSubIdChanged(int subId) {
        // Broadcast an Intent for default data sub change
        if (DBG) logdl("[broadcastDefaultDataSubIdChanged] subId=" + subId);
        Intent intent = new Intent(TelephonyIntents.ACTION_DEFAULT_DATA_SUBSCRIPTION_CHANGED);
        intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING
                | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
        intent.putExtra(PhoneConstants.SUBSCRIPTION_KEY, subId);
        mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
    }

    /* Sets the default subscription. If only one sub is active that
     * sub is set as default subId. If two or more  sub's are active
     * the first sub is set as default subscription
     */
    private void setDefaultFallbackSubId(int subId) {
        if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("setDefaultSubId called with DEFAULT_SUB_ID");
        }
        if (DBG) logdl("[setDefaultFallbackSubId] subId=" + subId);
        if (SubscriptionManager.isValidSubscriptionId(subId)) {
            int phoneId = getPhoneId(subId);
            if (phoneId >= 0 && (phoneId < mTelephonyManager.getPhoneCount()
                    || mTelephonyManager.getSimCount() == 1)) {
                if (DBG) logdl("[setDefaultFallbackSubId] set mDefaultFallbackSubId=" + subId);
                mDefaultFallbackSubId = subId;
                // Update MCC MNC device configuration information
                String defaultMccMnc = mTelephonyManager.getSimOperatorNumericForPhone(phoneId);
                MccTable.updateMccMncConfiguration(mContext, defaultMccMnc, false);

                // Broadcast an Intent for default sub change
                Intent intent = new Intent(TelephonyIntents.ACTION_DEFAULT_SUBSCRIPTION_CHANGED);
                intent.addFlags(Intent.FLAG_RECEIVER_REPLACE_PENDING
                        | Intent.FLAG_RECEIVER_INCLUDE_BACKGROUND);
                SubscriptionManager.putPhoneIdAndSubIdExtra(intent, phoneId, subId);
                if (DBG) {
                    logdl("[setDefaultFallbackSubId] broadcast default subId changed phoneId=" +
                            phoneId + " subId=" + subId);
                }
                mContext.sendStickyBroadcastAsUser(intent, UserHandle.ALL);
            } else {
                if (DBG) {
                    logdl("[setDefaultFallbackSubId] not set invalid phoneId=" + phoneId
                            + " subId=" + subId);
                }
            }
        }
    }

    @Override
    public void clearDefaultsForInactiveSubIds() {
        enforceModifyPhoneState("clearDefaultsForInactiveSubIds");

        // Now that all security checks passes, perform the operation as ourselves.
        final long identity = Binder.clearCallingIdentity();
        try {
            final List<SubscriptionInfo> records = getActiveSubscriptionInfoList(
                    mContext.getOpPackageName());
            if (DBG) logdl("[clearDefaultsForInactiveSubIds] records: " + records);
            if (shouldDefaultBeCleared(records, getDefaultDataSubId())) {
                if (DBG) logd("[clearDefaultsForInactiveSubIds] clearing default data sub id");
                setDefaultDataSubId(SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            }
            if (shouldDefaultBeCleared(records, getDefaultSmsSubId())) {
                if (DBG) logdl("[clearDefaultsForInactiveSubIds] clearing default sms sub id");
                setDefaultSmsSubId(SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            }
            if (shouldDefaultBeCleared(records, getDefaultVoiceSubId())) {
                if (DBG) logdl("[clearDefaultsForInactiveSubIds] clearing default voice sub id");
                setDefaultVoiceSubId(SubscriptionManager.INVALID_SUBSCRIPTION_ID);
            }
        } finally {
            Binder.restoreCallingIdentity(identity);
        }
    }

    private boolean shouldDefaultBeCleared(List<SubscriptionInfo> records, int subId) {
        if (DBG) logdl("[shouldDefaultBeCleared: subId] " + subId);
        if (records == null) {
            if (DBG) logdl("[shouldDefaultBeCleared] return true no records subId=" + subId);
            return true;
        }
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            // If the subId parameter is not valid its already cleared so return false.
            if (DBG) logdl("[shouldDefaultBeCleared] return false only one subId, subId=" + subId);
            return false;
        }
        for (SubscriptionInfo record : records) {
            int id = record.getSubscriptionId();
            if (DBG) logdl("[shouldDefaultBeCleared] Record.id: " + id);
            if (id == subId) {
                logdl("[shouldDefaultBeCleared] return false subId is active, subId=" + subId);
                return false;
            }
        }
        if (DBG) logdl("[shouldDefaultBeCleared] return true not active subId=" + subId);
        return true;
    }

    // FIXME: We need we should not be assuming phoneId == slotIndex as it will not be true
    // when there are multiple subscriptions per sim and probably for other reasons.
    public int getSubIdUsingPhoneId(int phoneId) {
        int[] subIds = getSubId(phoneId);
        if (subIds == null || subIds.length == 0) {
            return SubscriptionManager.INVALID_SUBSCRIPTION_ID;
        }
        return subIds[0];
    }

    /** Must be public for access from instrumentation tests. */
    @VisibleForTesting
    public List<SubscriptionInfo> getSubInfoUsingSlotIndexPrivileged(int slotIndex,
            boolean needCheck) {
        if (DBG) logd("[getSubInfoUsingSlotIndexPrivileged]+ slotIndex:" + slotIndex);
        if (slotIndex == SubscriptionManager.DEFAULT_SIM_SLOT_INDEX) {
            slotIndex = getSlotIndex(getDefaultSubId());
        }
        if (!SubscriptionManager.isValidSlotIndex(slotIndex)) {
            if (DBG) logd("[getSubInfoUsingSlotIndexPrivileged]- invalid slotIndex");
            return null;
        }

        if (needCheck && !isSubInfoReady()) {
            if (DBG) logd("[getSubInfoUsingSlotIndexPrivileged]- not ready");
            return null;
        }

        Cursor cursor = mContext.getContentResolver().query(SubscriptionManager.CONTENT_URI,
                null, SubscriptionManager.SIM_SLOT_INDEX + "=?",
                new String[]{String.valueOf(slotIndex)}, null);
        ArrayList<SubscriptionInfo> subList = null;
        try {
            if (cursor != null) {
                while (cursor.moveToNext()) {
                    SubscriptionInfo subInfo = getSubInfoRecord(cursor);
                    if (subInfo != null) {
                        if (subList == null) {
                            subList = new ArrayList<SubscriptionInfo>();
                        }
                        subList.add(subInfo);
                    }
                }
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DBG) logd("[getSubInfoUsingSlotIndex]- null info return");

        return subList;
    }

    private void validateSubId(int subId) {
        if (DBG) logd("validateSubId subId: " + subId);
        if (!SubscriptionManager.isValidSubscriptionId(subId)) {
            throw new RuntimeException("Invalid sub id passed as parameter");
        } else if (subId == SubscriptionManager.DEFAULT_SUBSCRIPTION_ID) {
            throw new RuntimeException("Default sub id passed as parameter");
        }
    }

    public void updatePhonesAvailability(Phone[] phones) {
        sPhones = phones;
    }

    /**
     * @return the list of subId's that are active, is never null but the length maybe 0.
     */
    @Override
    public int[] getActiveSubIdList() {
        Set<Entry<Integer, Integer>> simInfoSet = new HashSet<>(sSlotIndexToSubId.entrySet());

        int[] subIdArr = new int[simInfoSet.size()];
        int i = 0;
        for (Entry<Integer, Integer> entry: simInfoSet) {
            int sub = entry.getValue();
            subIdArr[i] = sub;
            i++;
        }

        if (VDBG) {
            logdl("[getActiveSubIdList] simInfoSet=" + simInfoSet + " subIdArr.length="
                    + subIdArr.length);
        }
        return subIdArr;
    }

    @Override
    public boolean isActiveSubId(int subId) {
        boolean retVal = SubscriptionManager.isValidSubscriptionId(subId)
                && sSlotIndexToSubId.containsValue(subId);

        if (VDBG) logdl("[isActiveSubId]- " + retVal);
        return retVal;
    }

    /**
     * Get the SIM state for the slot index
     * @return SIM state as the ordinal of {@See IccCardConstants.State}
     */
    @Override
    public int getSimStateForSlotIndex(int slotIndex) {
        State simState;
        String err;
        if (slotIndex < 0) {
            simState = IccCardConstants.State.UNKNOWN;
            err = "invalid slotIndex";
        } else {
            Phone phone = PhoneFactory.getPhone(slotIndex);
            if (phone == null) {
                simState = IccCardConstants.State.UNKNOWN;
                err = "phone == null";
            } else {
                IccCard icc = phone.getIccCard();
                if (icc == null) {
                    simState = IccCardConstants.State.UNKNOWN;
                    err = "icc == null";
                } else {
                    simState = icc.getState();
                    err = "";
                }
            }
        }
        if (VDBG) {
            logd("getSimStateForSlotIndex: " + err + " simState=" + simState
                    + " ordinal=" + simState.ordinal() + " slotIndex=" + slotIndex);
        }
        return simState.ordinal();
    }

    /**
     * Store properties associated with SubscriptionInfo in database
     * @param subId Subscription Id of Subscription
     * @param propKey Column name in database associated with SubscriptionInfo
     * @param propValue Value to store in DB for particular subId & column name
     * @hide
     */
    @Override
    public void setSubscriptionProperty(int subId, String propKey, String propValue) {
        enforceModifyPhoneState("setSubscriptionProperty");
        final long token = Binder.clearCallingIdentity();
        ContentResolver resolver = mContext.getContentResolver();

        setSubscriptionPropertyIntoContentResolver(subId, propKey, propValue, resolver);

        // Refresh the Cache of Active Subscription Info List
        refreshCachedActiveSubscriptionInfoList();

        Binder.restoreCallingIdentity(token);
    }

    private static void setSubscriptionPropertyIntoContentResolver(
            int subId, String propKey, String propValue, ContentResolver resolver) {
        ContentValues value = new ContentValues();
        switch (propKey) {
            case SubscriptionManager.CB_EXTREME_THREAT_ALERT:
            case SubscriptionManager.CB_SEVERE_THREAT_ALERT:
            case SubscriptionManager.CB_AMBER_ALERT:
            case SubscriptionManager.CB_EMERGENCY_ALERT:
            case SubscriptionManager.CB_ALERT_SOUND_DURATION:
            case SubscriptionManager.CB_ALERT_REMINDER_INTERVAL:
            case SubscriptionManager.CB_ALERT_VIBRATE:
            case SubscriptionManager.CB_ALERT_SPEECH:
            case SubscriptionManager.CB_ETWS_TEST_ALERT:
            case SubscriptionManager.CB_CHANNEL_50_ALERT:
            case SubscriptionManager.CB_CMAS_TEST_ALERT:
            case SubscriptionManager.CB_OPT_OUT_DIALOG:
            case SubscriptionManager.ENHANCED_4G_MODE_ENABLED:
            case SubscriptionManager.VT_IMS_ENABLED:
            case SubscriptionManager.WFC_IMS_ENABLED:
            case SubscriptionManager.WFC_IMS_MODE:
            case SubscriptionManager.WFC_IMS_ROAMING_MODE:
            case SubscriptionManager.WFC_IMS_ROAMING_ENABLED:
                value.put(propKey, Integer.parseInt(propValue));
                break;
            default:
                if (DBG) slogd("Invalid column name");
                break;
        }

        resolver.update(SubscriptionManager.CONTENT_URI, value,
                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID +
                        "=" + Integer.toString(subId), null);
    }

    /**
     * Store properties associated with SubscriptionInfo in database
     * @param subId Subscription Id of Subscription
     * @param propKey Column name in SubscriptionInfo database
     * @return Value associated with subId and propKey column in database
     * @hide
     */
    @Override
    public String getSubscriptionProperty(int subId, String propKey, String callingPackage) {
        if (!TelephonyPermissions.checkCallingOrSelfReadPhoneState(
                mContext, subId, callingPackage, "getSubscriptionProperty")) {
            return null;
        }
        String resultValue = null;
        ContentResolver resolver = mContext.getContentResolver();
        Cursor cursor = resolver.query(SubscriptionManager.CONTENT_URI,
                new String[]{propKey},
                SubscriptionManager.UNIQUE_KEY_SUBSCRIPTION_ID + "=?",
                new String[]{subId + ""}, null);

        try {
            if (cursor != null) {
                if (cursor.moveToFirst()) {
                    switch (propKey) {
                        case SubscriptionManager.CB_EXTREME_THREAT_ALERT:
                        case SubscriptionManager.CB_SEVERE_THREAT_ALERT:
                        case SubscriptionManager.CB_AMBER_ALERT:
                        case SubscriptionManager.CB_EMERGENCY_ALERT:
                        case SubscriptionManager.CB_ALERT_SOUND_DURATION:
                        case SubscriptionManager.CB_ALERT_REMINDER_INTERVAL:
                        case SubscriptionManager.CB_ALERT_VIBRATE:
                        case SubscriptionManager.CB_ALERT_SPEECH:
                        case SubscriptionManager.CB_ETWS_TEST_ALERT:
                        case SubscriptionManager.CB_CHANNEL_50_ALERT:
                        case SubscriptionManager.CB_CMAS_TEST_ALERT:
                        case SubscriptionManager.CB_OPT_OUT_DIALOG:
                        case SubscriptionManager.ENHANCED_4G_MODE_ENABLED:
                        case SubscriptionManager.VT_IMS_ENABLED:
                        case SubscriptionManager.WFC_IMS_ENABLED:
                        case SubscriptionManager.WFC_IMS_MODE:
                        case SubscriptionManager.WFC_IMS_ROAMING_MODE:
                        case SubscriptionManager.WFC_IMS_ROAMING_ENABLED:
                            resultValue = cursor.getInt(0) + "";
                            break;
                        default:
                            if(DBG) logd("Invalid column name");
                            break;
                    }
                } else {
                    if(DBG) logd("Valid row not present in db");
                }
            } else {
                if(DBG) logd("Query failed");
            }
        } finally {
            if (cursor != null) {
                cursor.close();
            }
        }
        if (DBG) logd("getSubscriptionProperty Query value = " + resultValue);
        return resultValue;
    }

    private static void printStackTrace(String msg) {
        RuntimeException re = new RuntimeException();
        slogd("StackTrace - " + msg);
        StackTraceElement[] st = re.getStackTrace();
        boolean first = true;
        for (StackTraceElement ste : st) {
            if (first) {
                first = false;
            } else {
                slogd(ste.toString());
            }
        }
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        mContext.enforceCallingOrSelfPermission(android.Manifest.permission.DUMP,
                "Requires DUMP");
        final long token = Binder.clearCallingIdentity();
        try {
            pw.println("SubscriptionController:");
            pw.println(" mLastISubServiceRegTime=" + mLastISubServiceRegTime);
            pw.println(" defaultSubId=" + getDefaultSubId());
            pw.println(" defaultDataSubId=" + getDefaultDataSubId());
            pw.println(" defaultVoiceSubId=" + getDefaultVoiceSubId());
            pw.println(" defaultSmsSubId=" + getDefaultSmsSubId());

            pw.println(" defaultDataPhoneId=" + SubscriptionManager
                    .from(mContext).getDefaultDataPhoneId());
            pw.println(" defaultVoicePhoneId=" + SubscriptionManager.getDefaultVoicePhoneId());
            pw.println(" defaultSmsPhoneId=" + SubscriptionManager
                    .from(mContext).getDefaultSmsPhoneId());
            pw.flush();

            for (Entry<Integer, Integer> entry : sSlotIndexToSubId.entrySet()) {
                pw.println(" sSlotIndexToSubId[" + entry.getKey() + "]: subId=" + entry.getValue());
            }
            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");

            List<SubscriptionInfo> sirl = getActiveSubscriptionInfoList(
                    mContext.getOpPackageName());
            if (sirl != null) {
                pw.println(" ActiveSubInfoList:");
                for (SubscriptionInfo entry : sirl) {
                    pw.println("  " + entry.toString());
                }
            } else {
                pw.println(" ActiveSubInfoList: is null");
            }
            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");

            sirl = getAllSubInfoList(mContext.getOpPackageName());
            if (sirl != null) {
                pw.println(" AllSubInfoList:");
                for (SubscriptionInfo entry : sirl) {
                    pw.println("  " + entry.toString());
                }
            } else {
                pw.println(" AllSubInfoList: is null");
            }
            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");

            mLocalLog.dump(fd, pw, args);
            pw.flush();
            pw.println("++++++++++++++++++++++++++++++++");
            pw.flush();
        } finally {
            Binder.restoreCallingIdentity(token);
        }
    }

    /**
     * Migrating Ims settings from global setting to subscription DB, if not already done.
     */
    @VisibleForTesting(visibility = VisibleForTesting.Visibility.PRIVATE)
    public void migrateImsSettings() {
        migrateImsSettingHelper(
                Settings.Global.ENHANCED_4G_MODE_ENABLED,
                SubscriptionManager.ENHANCED_4G_MODE_ENABLED);
        migrateImsSettingHelper(
                Settings.Global.VT_IMS_ENABLED,
                SubscriptionManager.VT_IMS_ENABLED);
        migrateImsSettingHelper(
                Settings.Global.WFC_IMS_ENABLED,
                SubscriptionManager.WFC_IMS_ENABLED);
        migrateImsSettingHelper(
                Settings.Global.WFC_IMS_MODE,
                SubscriptionManager.WFC_IMS_MODE);
        migrateImsSettingHelper(
                Settings.Global.WFC_IMS_ROAMING_MODE,
                SubscriptionManager.WFC_IMS_ROAMING_MODE);
        migrateImsSettingHelper(
                Settings.Global.WFC_IMS_ROAMING_ENABLED,
                SubscriptionManager.WFC_IMS_ROAMING_ENABLED);
    }

    private void migrateImsSettingHelper(String settingGlobal, String subscriptionProperty) {
        ContentResolver resolver = mContext.getContentResolver();
        int defaultSubId = getDefaultVoiceSubId();
        try {
            int prevSetting = Settings.Global.getInt(resolver, settingGlobal);

            if (prevSetting != DEPRECATED_SETTING) {
                // Write previous setting into Subscription DB.
                setSubscriptionPropertyIntoContentResolver(defaultSubId, subscriptionProperty,
                        Integer.toString(prevSetting), resolver);
                // Write global setting value with DEPRECATED_SETTING making sure
                // migration only happen once.
                Settings.Global.putInt(resolver, settingGlobal, DEPRECATED_SETTING);
            }
        } catch (Settings.SettingNotFoundException e) {
        }
    }
}
