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

package com.android.cts.managedprofile;

import static com.android.cts.managedprofile.BaseManagedProfileTest.ADMIN_RECEIVER_COMPONENT;

import android.app.admin.DevicePolicyManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.os.UserManager;
import android.provider.MediaStore;
import android.test.InstrumentationTestCase;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

/**
 * Verify that certain cross profile intent filters are disallowed when the device admin sets
 * DISALLOW_SHARE_INTO_MANAGED_PROFILE restriction.
 * <p>
 * The way we check if a particular cross profile intent filter is disallowed is by trying to
 * resolve an example intent that matches the intent filter. The cross profile filter functions
 * correctly if and only if the resolution result contains a system intent forwarder activity
 * (com.android.internal.app.IntentForwarderActivity), which is the framework's mechanism to
 * trampoline intents across profiles. Instead of hardcoding the system's intent forwarder activity,
 * we retrieve it programmatically by resolving known cross profile intents specifically set up for
 * this purpose: {@link #KNOWN_ACTION_TO_PROFILE} and {@link #KNOWN_ACTION_TO_PERSONAL}
 */
public class DisallowSharingIntoProfileTest extends InstrumentationTestCase {

    // These are the data sharing intents which can be forwarded to the managed profile.
    private final List<Intent> sharingIntentsToProfile = Arrays.asList(
            new Intent(Intent.ACTION_SEND).setType("*/*"),
            new Intent(Intent.ACTION_SEND_MULTIPLE).setType("*/*"));

    // These are the data sharing intents which can be forwarded to the primary profile.
    private final List<Intent> sharingIntentsToPersonal = new ArrayList<>(Arrays.asList(
            new Intent(Intent.ACTION_GET_CONTENT).setType("*/*").addCategory(
                    Intent.CATEGORY_OPENABLE),
            new Intent(Intent.ACTION_OPEN_DOCUMENT).setType("*/*").addCategory(
                    Intent.CATEGORY_OPENABLE),
            new Intent(Intent.ACTION_PICK).setType("*/*").addCategory(
                    Intent.CATEGORY_DEFAULT),
            new Intent(Intent.ACTION_PICK).addCategory(Intent.CATEGORY_DEFAULT)));

    // These are the data sharing intents which can be forwarded to the primary profile,
    // if the device supports FEATURE_CAMERA
    private final List<Intent> sharingIntentsToPersonalIfCameraExists = Arrays.asList(
            new Intent(MediaStore.ACTION_IMAGE_CAPTURE),
            new Intent(MediaStore.ACTION_VIDEO_CAPTURE),
            new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA),
            new Intent(MediaStore.INTENT_ACTION_VIDEO_CAMERA),
            new Intent(MediaStore.ACTION_IMAGE_CAPTURE_SECURE),
            new Intent(MediaStore.INTENT_ACTION_STILL_IMAGE_CAMERA_SECURE));

    private static final String KNOWN_ACTION_TO_PROFILE = ManagedProfileActivity.ACTION;
    private static final String KNOWN_ACTION_TO_PERSONAL = PrimaryUserActivity.ACTION;

    protected Context mContext;
    protected DevicePolicyManager mDevicePolicyManager;

    @Override
    public void setUp() throws Exception {
        super.setUp();

        mContext = getInstrumentation().getContext();
        mDevicePolicyManager = mContext.getSystemService(DevicePolicyManager.class);
        assertNotNull(mDevicePolicyManager);
    }

    public void testSetUp() throws Exception {
        // toggle the restriction to reset system's built-in cross profile intent filters,
        // simulating the default state of a work profile created by ManagedProvisioning
        testDisableSharingIntoProfile();
        testEnableSharingIntoProfile();

        PackageManager pm = mContext.getPackageManager();
        if (pm.hasSystemFeature(PackageManager.FEATURE_CAMERA)) {
            sharingIntentsToPersonal.addAll(sharingIntentsToPersonalIfCameraExists);
        }

        mDevicePolicyManager.clearCrossProfileIntentFilters(ADMIN_RECEIVER_COMPONENT);
        // Set up cross profile intent filters so we can resolve these to find out framework's
        // intent forwarder activity as ground truth
        mDevicePolicyManager.addCrossProfileIntentFilter(ADMIN_RECEIVER_COMPONENT,
                new IntentFilter(KNOWN_ACTION_TO_PERSONAL),
                DevicePolicyManager.FLAG_PARENT_CAN_ACCESS_MANAGED);
        mDevicePolicyManager.addCrossProfileIntentFilter(ADMIN_RECEIVER_COMPONENT,
                new IntentFilter(KNOWN_ACTION_TO_PROFILE),
                DevicePolicyManager.FLAG_MANAGED_CAN_ACCESS_PARENT);
    }

    /**
     * Test sharing initiated from the personal side are mainly driven from the host side, see
     * ManagedProfileTest.testDisallowSharingIntoProfileFromPersonal() See javadoc of
     * {@link #DisallowSharingIntoProfileTest} class for the mechanism behind this test.
     */
    public void testSharingFromPersonalFails() {
        ResolveInfo toProfileForwarderInfo = getIntentForwarder(
                new Intent(KNOWN_ACTION_TO_PROFILE));
        assertCrossProfileIntentsResolvability(sharingIntentsToProfile, toProfileForwarderInfo,
                /* expectForwardable */ false);
    }

    public void testSharingFromPersonalSucceeds() {
        ResolveInfo toProfileForwarderInfo = getIntentForwarder(
                new Intent(KNOWN_ACTION_TO_PROFILE));
        assertCrossProfileIntentsResolvability(sharingIntentsToProfile, toProfileForwarderInfo,
                /* expectForwardable */ true);
    }

    /**
     * Test sharing initiated from the profile side i.e. user tries to pick up personal data within
     * a work app. See javadoc of {@link #DisallowSharingIntoProfileTest} class for the mechanism
     * behind this test.
     */
    public void testSharingFromProfile() throws Exception {
        testSetUp();
        ResolveInfo toPersonalForwarderInfo = getIntentForwarder(
                new Intent(KNOWN_ACTION_TO_PERSONAL));

        testDisableSharingIntoProfile();
        assertCrossProfileIntentsResolvability(sharingIntentsToPersonal, toPersonalForwarderInfo,
                /* expectForwardable */ false);
        testEnableSharingIntoProfile();
        assertCrossProfileIntentsResolvability(sharingIntentsToPersonal, toPersonalForwarderInfo,
                /* expectForwardable */ true);
    }

    public void testEnableSharingIntoProfile() throws Exception {
        setSharingEnabled(true);
    }

    public void testDisableSharingIntoProfile() throws Exception {
        setSharingEnabled(false);
    }

    private void setSharingEnabled(boolean enabled) throws InterruptedException {
        final CountDownLatch latch = new CountDownLatch(1);
        BroadcastReceiver receiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                latch.countDown();
            }
        };
        IntentFilter filter = new IntentFilter();
        filter.addAction(DevicePolicyManager.ACTION_DATA_SHARING_RESTRICTION_APPLIED);
        mContext.registerReceiver(receiver, filter);
        try {
            if (enabled) {
                mDevicePolicyManager.clearUserRestriction(ADMIN_RECEIVER_COMPONENT,
                        UserManager.DISALLOW_SHARE_INTO_MANAGED_PROFILE);
            } else {
                mDevicePolicyManager.addUserRestriction(ADMIN_RECEIVER_COMPONENT,
                        UserManager.DISALLOW_SHARE_INTO_MANAGED_PROFILE);
            }
            // Wait for the restriction to apply
            assertTrue("Restriction not applied after 5 seconds", latch.await(5, TimeUnit.SECONDS));
        } finally {
            mContext.unregisterReceiver(receiver);
        }
    }

    private void assertCrossProfileIntentsResolvability(List<Intent> intents,
            ResolveInfo expectedForwarder, boolean expectForwardable) {
        for (Intent intent : intents) {
            List<ResolveInfo> resolveInfoList = mContext.getPackageManager().queryIntentActivities(
                    intent,
                    PackageManager.MATCH_DEFAULT_ONLY);
            if (expectForwardable) {
                assertTrue("Expect " + intent + " to be forwardable, but resolve list"
                        + " does not contain expected intent forwarder " + expectedForwarder,
                        containsResolveInfo(resolveInfoList, expectedForwarder));
            } else {
                assertFalse("Expect " + intent + " not to be forwardable, but resolve list "
                        + "contains intent forwarder " + expectedForwarder,
                        containsResolveInfo(resolveInfoList, expectedForwarder));
            }
        }
    }

    private ResolveInfo getIntentForwarder(Intent intent) {
        List<ResolveInfo> result = mContext.getPackageManager().queryIntentActivities(intent,
                PackageManager.MATCH_DEFAULT_ONLY);
        assertEquals("Expect only one resolve result for " + intent, 1, result.size());
        return result.get(0);
    }

    private boolean containsResolveInfo(List<ResolveInfo> list, ResolveInfo info) {
        for (ResolveInfo entry : list) {
            if (entry.activityInfo.packageName.equals(info.activityInfo.packageName)
                    && entry.activityInfo.name.equals(info.activityInfo.name)) {
                return true;
            }
        }
        return false;
    }
}
