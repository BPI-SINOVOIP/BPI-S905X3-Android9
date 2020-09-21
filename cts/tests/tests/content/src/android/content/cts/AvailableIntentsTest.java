/*
 * Copyright (C) 2009 The Android Open Source Project
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

package android.content.cts;

import android.app.DownloadManager;
import android.app.SearchManager;
import android.content.ContentUris;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.storage.StorageManager;
import android.provider.AlarmClock;
import android.provider.MediaStore;
import android.provider.Settings;
import android.provider.Telephony;
import android.speech.RecognizerIntent;
import android.telecom.TelecomManager;
import android.test.AndroidTestCase;

import com.android.compatibility.common.util.FeatureUtil;

import java.util.List;

public class AvailableIntentsTest extends AndroidTestCase {
    private static final String NORMAL_URL = "http://www.google.com/";
    private static final String SECURE_URL = "https://www.google.com/";

    /**
     * Assert target intent can be handled by at least one Activity.
     * @param intent - the Intent will be handled.
     */
    private void assertCanBeHandled(final Intent intent) {
        PackageManager packageManager = mContext.getPackageManager();
        List<ResolveInfo> resolveInfoList = packageManager.queryIntentActivities(intent, 0);
        assertNotNull(resolveInfoList);
        // one or more activity can handle this intent.
        assertTrue(resolveInfoList.size() > 0);
    }

    /**
     * Assert target intent is not resolved by a filter with priority greater than 0.
     * @param intent - the Intent will be handled.
     */
    private void assertDefaultHandlerValidPriority(final Intent intent) {
        PackageManager packageManager = mContext.getPackageManager();
        List<ResolveInfo> resolveInfoList = packageManager.queryIntentActivities(intent, 0);
        assertNotNull(resolveInfoList);
        // one or more activity can handle this intent.
        assertTrue(resolveInfoList.size() > 0);
        // no activities override defaults with a high priority. Only system activities can override
        // the priority.
        for (ResolveInfo resolveInfo : resolveInfoList) {
            assertTrue(resolveInfo.priority <= 0);
        }
    }

    /**
     * Test ACTION_VIEW when url is http://web_address,
     * it will open a browser window to the URL specified.
     */
    public void testViewNormalUrl() {
        Uri uri = Uri.parse(NORMAL_URL);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        assertCanBeHandled(intent);
    }

    /**
     * Test ACTION_VIEW when url is https://web_address,
     * it will open a browser window to the URL specified.
     */
    public void testViewSecureUrl() {
        Uri uri = Uri.parse(SECURE_URL);
        Intent intent = new Intent(Intent.ACTION_VIEW, uri);
        assertCanBeHandled(intent);
    }

    /**
     * Test ACTION_WEB_SEARCH when url is http://web_address,
     * it will open a browser window to the URL specified.
     */
    public void testWebSearchNormalUrl() {
        Uri uri = Uri.parse(NORMAL_URL);
        Intent intent = new Intent(Intent.ACTION_WEB_SEARCH);
        intent.putExtra(SearchManager.QUERY, uri);
        assertCanBeHandled(intent);
    }

    /**
     * Test ACTION_WEB_SEARCH when url is https://web_address,
     * it will open a browser window to the URL specified.
     */
    public void testWebSearchSecureUrl() {
        Uri uri = Uri.parse(SECURE_URL);
        Intent intent = new Intent(Intent.ACTION_WEB_SEARCH);
        intent.putExtra(SearchManager.QUERY, uri);
        assertCanBeHandled(intent);
    }

    /**
     * Test ACTION_WEB_SEARCH when url is empty string,
     * google search will be applied for the plain text.
     */
    public void testWebSearchPlainText() {
        String searchString = "where am I?";
        Intent intent = new Intent(Intent.ACTION_WEB_SEARCH);
        intent.putExtra(SearchManager.QUERY, searchString);
        assertCanBeHandled(intent);
    }

    /**
     * Test ACTION_CALL when uri is a phone number, it will call the entered phone number.
     */
    public void testCallPhoneNumber() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Uri uri = Uri.parse("tel:2125551212");
            Intent intent = new Intent(Intent.ACTION_CALL, uri);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_DIAL when uri is a phone number, it will dial the entered phone number.
     */
    public void testDialPhoneNumber() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Uri uri = Uri.parse("tel:(212)5551212");
            Intent intent = new Intent(Intent.ACTION_DIAL, uri);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_DIAL when uri is a phone number, it will dial the entered phone number.
     */
    public void testDialVoicemail() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Uri uri = Uri.parse("voicemail:");
            Intent intent = new Intent(Intent.ACTION_DIAL, uri);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_CHANGE_PHONE_ACCOUNTS, it will display the phone account preferences.
     */
    public void testChangePhoneAccounts() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Intent intent = new Intent(TelecomManager.ACTION_CHANGE_PHONE_ACCOUNTS);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_SHOW_CALL_ACCESSIBILITY_SETTINGS, it will display the call accessibility preferences.
     */
    public void testShowCallAccessibilitySettings() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Intent intent = new Intent(TelecomManager.ACTION_SHOW_CALL_ACCESSIBILITY_SETTINGS);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_SHOW_CALL_SETTINGS, it will display the call preferences.
     */
    public void testShowCallSettings() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Intent intent = new Intent(TelecomManager.ACTION_SHOW_CALL_SETTINGS);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test ACTION_SHOW_RESPOND_VIA_SMS_SETTINGS, it will display the respond by SMS preferences.
     */
    public void testShowRespondViaSmsSettings() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Intent intent = new Intent(TelecomManager.ACTION_SHOW_RESPOND_VIA_SMS_SETTINGS);
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test start camera by intent
     */
    public void testCamera() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA)
                || packageManager.hasSystemFeature(PackageManager.FEATURE_CAMERA_FRONT)) {
            Intent intent = new Intent(MediaStore.ACTION_IMAGE_CAPTURE);
            assertCanBeHandled(intent);

            intent.setAction(MediaStore.ACTION_VIDEO_CAPTURE);
            assertCanBeHandled(intent);

            intent.setAction(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA);
            assertCanBeHandled(intent);

            intent.setAction(MediaStore.INTENT_ACTION_VIDEO_CAMERA);
            assertCanBeHandled(intent);
        }
    }

    public void testSettings() {
        assertCanBeHandled(new Intent(Settings.ACTION_SETTINGS));
    }

    /**
     * Test add event in calendar
     */
    public void testCalendarAddAppointment() {
        Intent addAppointmentIntent = new Intent(Intent.ACTION_EDIT);
        addAppointmentIntent.setType("vnd.android.cursor.item/event");
        assertCanBeHandled(addAppointmentIntent);
    }

    /**
     * Test view call logs
     */
    public void testContactsCallLogs() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            Intent intent = new Intent(Intent.ACTION_VIEW);
            intent.setType("vnd.android.cursor.dir/calls");
            assertCanBeHandled(intent);
        }
    }

    /**
     * Test view music playback
     */
    public void testMusicPlayback() {
        Intent intent = new Intent(Intent.ACTION_VIEW);
        intent.setDataAndType(ContentUris.withAppendedId(
                MediaStore.Audio.Media.INTERNAL_CONTENT_URI, 1), "audio/*");
        assertCanBeHandled(intent);
    }

    public void testAlarmClockSetAlarm() {
        Intent intent = new Intent(AlarmClock.ACTION_SET_ALARM);
        intent.putExtra(AlarmClock.EXTRA_MESSAGE, "Custom message");
        intent.putExtra(AlarmClock.EXTRA_HOUR, 12);
        intent.putExtra(AlarmClock.EXTRA_MINUTES, 0);
        assertCanBeHandled(intent);
    }

    public void testAlarmClockSetTimer() {
        Intent intent = new Intent(AlarmClock.ACTION_SET_TIMER);
        intent.putExtra(AlarmClock.EXTRA_LENGTH, 60000);
        assertCanBeHandled(intent);
    }

    public void testAlarmClockShowAlarms() {
        Intent intent = new Intent(AlarmClock.ACTION_SHOW_ALARMS);
        assertCanBeHandled(intent);
    }

    public void testAlarmClockShowTimers() {
        if (mContext.getPackageManager().hasSystemFeature(PackageManager.FEATURE_LEANBACK_ONLY)) {
            return;
        }
        Intent intent = new Intent(AlarmClock.ACTION_SHOW_TIMERS);
        assertCanBeHandled(intent);
    }

    public void testOpenDocumentAny() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        assertCanBeHandled(intent);
    }

    public void testOpenDocumentImage() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("image/*");
        assertCanBeHandled(intent);
    }

    public void testCreateDocument() {
        Intent intent = new Intent(Intent.ACTION_CREATE_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("text/plain");
        assertCanBeHandled(intent);
    }

    public void testGetContentAny() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        assertCanBeHandled(intent);
    }

    public void testGetContentImage() {
        Intent intent = new Intent(Intent.ACTION_GET_CONTENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("image/*");
        assertCanBeHandled(intent);
    }

    public void testRingtonePicker() {
        assertCanBeHandled(new Intent(RingtoneManager.ACTION_RINGTONE_PICKER));
    }

    public void testViewDownloads() {
        assertCanBeHandled(new Intent(DownloadManager.ACTION_VIEW_DOWNLOADS));
    }

    public void testManageStorage() {
        assertCanBeHandled(new Intent(StorageManager.ACTION_MANAGE_STORAGE));
    }

    public void testFingerprintEnrollStart() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_FINGERPRINT)) {
            assertCanBeHandled(new Intent(Settings.ACTION_FINGERPRINT_ENROLL));
        }
    }

    public void testPictureInPictureSettings() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_PICTURE_IN_PICTURE)) {
            assertCanBeHandled(new Intent(Settings.ACTION_PICTURE_IN_PICTURE_SETTINGS));
        }
    }

    public void testChangeDefaultSmsApplication() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_TELEPHONY)) {
            assertCanBeHandled(new Intent(Telephony.Sms.Intents.ACTION_CHANGE_DEFAULT));
        }
    }

    public void testLocationScanningSettings() {
        PackageManager packageManager = mContext.getPackageManager();
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_WATCH)) {
            // Skip the test for wearable device.
            return;
        }
        if (packageManager.hasSystemFeature(PackageManager.FEATURE_WIFI)
                || packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)) {
            assertCanBeHandled(new Intent("android.settings.LOCATION_SCANNING_SETTINGS"));
        }
    }
}
