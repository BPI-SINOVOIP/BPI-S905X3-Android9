/*
 * Copyright (C) 2016 The Android Open Source Project
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

package android.carrierapi.cts;

import android.content.BroadcastReceiver;
import android.content.ContentProviderClient;
import android.content.ContentValues;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.database.Cursor;
import android.net.ConnectivityManager;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.PersistableBundle;
import android.provider.VoicemailContract;
import android.telephony.CarrierConfigManager;
import android.telephony.PhoneStateListener;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.test.AndroidTestCase;
import android.util.Log;

import com.android.internal.telephony.TelephonyIntents;
import com.android.internal.telephony.uicc.IccUtils;

import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicReference;

public class CarrierApiTest extends AndroidTestCase {
    private static final String TAG = "CarrierApiTest";
    private TelephonyManager mTelephonyManager;
    private CarrierConfigManager mCarrierConfigManager;
    private PackageManager mPackageManager;
    private SubscriptionManager mSubscriptionManager;
    private ContentProviderClient mVoicemailProvider;
    private ContentProviderClient mStatusProvider;
    private Uri mVoicemailContentUri;
    private Uri mStatusContentUri;
    private boolean hasCellular;
    private String selfPackageName;
    private String selfCertHash;
    private HandlerThread mListenerThread;

    private static final String FiDevCert = "24EB92CBB156B280FA4E1429A6ECEEB6E5C1BFE4";

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mTelephonyManager = (TelephonyManager)
                getContext().getSystemService(Context.TELEPHONY_SERVICE);
        mCarrierConfigManager = (CarrierConfigManager)
                getContext().getSystemService(Context.CARRIER_CONFIG_SERVICE);
        mPackageManager = getContext().getPackageManager();
        mSubscriptionManager = (SubscriptionManager)
                getContext().getSystemService(Context.TELEPHONY_SUBSCRIPTION_SERVICE);
        selfPackageName = getContext().getPackageName();
        selfCertHash = getCertHash(selfPackageName);
        mVoicemailContentUri = VoicemailContract.Voicemails.buildSourceUri(selfPackageName);
        mVoicemailProvider = getContext().getContentResolver()
                .acquireContentProviderClient(mVoicemailContentUri);
        mStatusContentUri = VoicemailContract.Status.buildSourceUri(selfPackageName);
        mStatusProvider = getContext().getContentResolver()
                .acquireContentProviderClient(mStatusContentUri);
        mListenerThread = new HandlerThread("CarrierApiTest");
        mListenerThread.start();
        hasCellular = hasCellular();
        if (!hasCellular) {
            Log.e(TAG, "No cellular support, all tests will be skipped.");
        }
    }

    @Override
    public void tearDown() throws Exception {
        mListenerThread.quit();
        try {
            mStatusProvider.delete(mStatusContentUri, null, null);
            mVoicemailProvider.delete(mVoicemailContentUri, null, null);
        } catch (Exception e) {
            Log.w(TAG, "Failed to clean up voicemail tables in tearDown", e);
        }
        super.tearDown();
    }

    /**
     * Checks whether the cellular stack should be running on this device.
     */
    private boolean hasCellular() {
        ConnectivityManager mgr =
                (ConnectivityManager) getContext().getSystemService(Context.CONNECTIVITY_SERVICE);
        return mgr.isNetworkSupported(ConnectivityManager.TYPE_MOBILE) &&
               mTelephonyManager.isVoiceCapable();
    }

    private boolean isSimCardPresent() {
        return mTelephonyManager.getPhoneType() != TelephonyManager.PHONE_TYPE_NONE &&
                mTelephonyManager.getSimState() != TelephonyManager.SIM_STATE_ABSENT;
    }

    private String getCertHash(String pkgName) {
        try {
            PackageInfo pInfo = mPackageManager.getPackageInfo(pkgName,
                    PackageManager.GET_SIGNATURES | PackageManager.GET_DISABLED_UNTIL_USED_COMPONENTS);
            MessageDigest md = MessageDigest.getInstance("SHA-1");
            return IccUtils.bytesToHexString(md.digest(pInfo.signatures[0].toByteArray()));
        } catch (PackageManager.NameNotFoundException ex) {
            Log.e(TAG, pkgName + " not found", ex);
        } catch (NoSuchAlgorithmException ex) {
            Log.e(TAG, "Algorithm SHA1 is not found.");
        }
        return "";
    }

    private void failMessage() {
        if (FiDevCert.equalsIgnoreCase(selfCertHash)) {
            fail("This test requires a Project Fi SIM card.");
        } else {
            fail("This test requires a SIM card with carrier privilege rule on it.\n" +
                 "Cert hash: " + selfCertHash + "\n" +
                 "Visit https://source.android.com/devices/tech/config/uicc.html");
        }
    }

    public void testSimCardPresent() {
        if (!hasCellular) return;
        assertTrue("This test requires SIM card.", isSimCardPresent());
    }

    public void testHasCarrierPrivileges() {
        if (!hasCellular) return;
        if (!mTelephonyManager.hasCarrierPrivileges()) {
            failMessage();
        }
    }

    public void testGetIccAuthentication() {
        // EAP-SIM rand is 16 bytes.
        String base64Challenge = "ECcTqwuo6OfY8ddFRboD9WM=";
        String base64Challenge2 = "EMNxjsFrPCpm+KcgCmQGnwQ=";
        if (!hasCellular) return;
        try {
            assertNull("getIccAuthentication should return null for empty data.",
                    mTelephonyManager.getIccAuthentication(TelephonyManager.APPTYPE_USIM,
                    TelephonyManager.AUTHTYPE_EAP_AKA, ""));
            String response = mTelephonyManager.getIccAuthentication(TelephonyManager.APPTYPE_USIM,
                    TelephonyManager.AUTHTYPE_EAP_SIM, base64Challenge);
            assertTrue("Response to EAP-SIM Challenge must not be Null.", response != null);
            // response is base64 encoded. After decoding, the value should be:
            // 1 length byte + SRES(4 bytes) + 1 length byte + Kc(8 bytes)
            byte[] result = android.util.Base64.decode(response, android.util.Base64.DEFAULT);
            assertTrue("Result length must be 14 bytes.", 14 == result.length);
            String response2 = mTelephonyManager.getIccAuthentication(TelephonyManager.APPTYPE_USIM,
                    TelephonyManager.AUTHTYPE_EAP_SIM, base64Challenge2);
            assertTrue("Two responses must be different.", !response.equals(response2));
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testSendDialerSpecialCode() {
        if (!hasCellular) return;
        try {
            IntentReceiver intentReceiver = new IntentReceiver();
            final IntentFilter intentFilter = new IntentFilter();
            intentFilter.addAction(TelephonyIntents.SECRET_CODE_ACTION);
            intentFilter.addDataScheme("android_secret_code");
            getContext().registerReceiver(intentReceiver, intentFilter);

            mTelephonyManager.sendDialerSpecialCode("4636");
            assertTrue("Did not receive expected Intent: " + TelephonyIntents.SECRET_CODE_ACTION,
                    intentReceiver.waitForReceive());
        } catch (SecurityException e) {
            failMessage();
        } catch (InterruptedException e) {
            Log.d(TAG, "Broadcast receiver wait was interrupted.");
        }
    }

    public void testSubscriptionInfoListing() {
        if (!hasCellular) return;
        try {
            assertTrue("getActiveSubscriptionInfoCount() should be non-zero",
                    mSubscriptionManager.getActiveSubscriptionInfoCount() > 0);
            List<SubscriptionInfo> subInfoList =
                    mSubscriptionManager.getActiveSubscriptionInfoList();
            assertNotNull("getActiveSubscriptionInfoList() returned null", subInfoList);
            assertFalse("getActiveSubscriptionInfoList() returned an empty list",
                    subInfoList.isEmpty());
            for (SubscriptionInfo info : subInfoList) {
                TelephonyManager tm =
                        mTelephonyManager.createForSubscriptionId(info.getSubscriptionId());
                assertTrue("getActiveSubscriptionInfoList() returned an inaccessible subscription",
                        tm.hasCarrierPrivileges(info.getSubscriptionId()));

                // Check other APIs to make sure they are accessible and return consistent info.
                SubscriptionInfo infoForSlot =
                        mSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(
                                info.getSimSlotIndex());
                assertNotNull("getActiveSubscriptionInfoForSimSlotIndex() returned null",
                        infoForSlot);
                assertEquals(
                        "getActiveSubscriptionInfoForSimSlotIndex() returned inconsistent info",
                        info.getSubscriptionId(), infoForSlot.getSubscriptionId());

                SubscriptionInfo infoForSubId =
                        mSubscriptionManager.getActiveSubscriptionInfo(info.getSubscriptionId());
                assertNotNull("getActiveSubscriptionInfo() returned null", infoForSubId);
                assertEquals("getActiveSubscriptionInfo() returned inconsistent info",
                        info.getSubscriptionId(), infoForSubId.getSubscriptionId());
            }
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testCarrierConfigIsAccessible() {
        if (!hasCellular) return;
        try {
            PersistableBundle bundle = mCarrierConfigManager.getConfig();
            assertNotNull("CarrierConfigManager#getConfig() returned null", bundle);
            assertFalse("CarrierConfigManager#getConfig() returned empty bundle", bundle.isEmpty());

            int subId = SubscriptionManager.getDefaultSubscriptionId();
            bundle = mCarrierConfigManager.getConfigForSubId(subId);
            assertNotNull("CarrierConfigManager#getConfigForSubId() returned null", bundle);
            assertFalse("CarrierConfigManager#getConfigForSubId() returned empty bundle",
                    bundle.isEmpty());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testTelephonyApisAreAccessible() {
        if (!hasCellular) return;
        // The following methods may return any value depending on the state of the device. Simply
        // call them to make sure they do not throw any exceptions.
        try {
            mTelephonyManager.getDeviceSoftwareVersion();
            mTelephonyManager.getDeviceId();
            mTelephonyManager.getDeviceId(mTelephonyManager.getSlotIndex());
            mTelephonyManager.getImei();
            mTelephonyManager.getImei(mTelephonyManager.getSlotIndex());
            mTelephonyManager.getMeid();
            mTelephonyManager.getMeid(mTelephonyManager.getSlotIndex());
            mTelephonyManager.getNai();
            mTelephonyManager.getDataNetworkType();
            mTelephonyManager.getVoiceNetworkType();
            mTelephonyManager.getSimSerialNumber();
            mTelephonyManager.getSubscriberId();
            mTelephonyManager.getGroupIdLevel1();
            mTelephonyManager.getLine1Number();
            mTelephonyManager.getVoiceMailNumber();
            mTelephonyManager.getVisualVoicemailPackageName();
            mTelephonyManager.getVoiceMailAlphaTag();
            mTelephonyManager.getForbiddenPlmns();
            mTelephonyManager.getServiceState();
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testVoicemailTableIsAccessible() throws Exception {
        if (!hasCellular) return;
        ContentValues value = new ContentValues();
        value.put(VoicemailContract.Voicemails.NUMBER, "0123456789");
        value.put(VoicemailContract.Voicemails.SOURCE_PACKAGE, selfPackageName);
        try {
            Uri uri = mVoicemailProvider.insert(mVoicemailContentUri, value);
            assertNotNull(uri);
            Cursor cursor = mVoicemailProvider.query(uri,
                    new String[] {
                            VoicemailContract.Voicemails.NUMBER,
                            VoicemailContract.Voicemails.SOURCE_PACKAGE
                    }, null, null, null);
            assertNotNull(cursor);
            assertTrue(cursor.moveToFirst());
            assertEquals("0123456789", cursor.getString(0));
            assertEquals(selfPackageName, cursor.getString(1));
            assertFalse(cursor.moveToNext());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testVoicemailStatusTableIsAccessible() throws Exception {
        if (!hasCellular) return;
        ContentValues value = new ContentValues();
        value.put(VoicemailContract.Status.CONFIGURATION_STATE,
                VoicemailContract.Status.CONFIGURATION_STATE_OK);
        value.put(VoicemailContract.Status.SOURCE_PACKAGE, selfPackageName);
        try {
            Uri uri = mStatusProvider.insert(mStatusContentUri, value);
            assertNotNull(uri);
            Cursor cursor = mVoicemailProvider.query(uri,
                    new String[] {
                            VoicemailContract.Status.CONFIGURATION_STATE,
                            VoicemailContract.Status.SOURCE_PACKAGE
                    }, null, null, null);
            assertNotNull(cursor);
            assertTrue(cursor.moveToFirst());
            assertEquals(VoicemailContract.Status.CONFIGURATION_STATE_OK, cursor.getInt(0));
            assertEquals(selfPackageName, cursor.getString(1));
            assertFalse(cursor.moveToNext());
        } catch (SecurityException e) {
            failMessage();
        }
    }

    public void testPhoneStateListener() throws Exception {
        if (!hasCellular) return;
        final AtomicReference<SecurityException> error = new AtomicReference<>();
        final CountDownLatch latch = new CountDownLatch(1);
        new Handler(mListenerThread.getLooper()).post(() -> {
            PhoneStateListener listener = new PhoneStateListener() {};
            try {
                mTelephonyManager.listen(
                        listener, PhoneStateListener.LISTEN_MESSAGE_WAITING_INDICATOR);
                mTelephonyManager.listen(
                        listener, PhoneStateListener.LISTEN_CALL_FORWARDING_INDICATOR);
            } catch (SecurityException e) {
                error.set(e);
            } finally {
                mTelephonyManager.listen(listener, PhoneStateListener.LISTEN_NONE);
                latch.countDown();
            }
        });
        assertTrue("Test timed out", latch.await(30L, TimeUnit.SECONDS));
        if (error.get() != null) {
            failMessage();
        }
    }

    public void testSubscriptionInfoChangeListener() throws Exception {
        if (!hasCellular) return;
        final AtomicReference<SecurityException> error = new AtomicReference<>();
        final CountDownLatch latch = new CountDownLatch(1);
        new Handler(mListenerThread.getLooper()).post(() -> {
            SubscriptionManager.OnSubscriptionsChangedListener listener =
                    new SubscriptionManager.OnSubscriptionsChangedListener();
            try {
                mSubscriptionManager.addOnSubscriptionsChangedListener(listener);
            } catch (SecurityException e) {
                error.set(e);
            } finally {
                mSubscriptionManager.removeOnSubscriptionsChangedListener(listener);
                latch.countDown();
            }
        });
        assertTrue("Test timed out", latch.await(30L, TimeUnit.SECONDS));
        if (error.get() != null) {
            failMessage();
        }

    }

    private static class IntentReceiver extends BroadcastReceiver {
        private final CountDownLatch mReceiveLatch = new CountDownLatch(1);

        @Override
        public void onReceive(Context context, Intent intent) {
            mReceiveLatch.countDown();
        }

        public boolean waitForReceive() throws InterruptedException {
            return mReceiveLatch.await(30, TimeUnit.SECONDS);
        }
    }
}
