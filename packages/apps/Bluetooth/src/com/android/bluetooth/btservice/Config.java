/*
 * Copyright (C) 2012 The Android Open Source Project
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

package com.android.bluetooth.btservice;

import android.bluetooth.BluetoothProfile;
import android.content.ContentResolver;
import android.content.Context;
import android.content.res.Resources;
import android.provider.Settings;
import android.util.Log;

import com.android.bluetooth.R;
import com.android.bluetooth.a2dp.A2dpService;
import com.android.bluetooth.a2dpsink.A2dpSinkService;
import com.android.bluetooth.avrcp.AvrcpTargetService;
import com.android.bluetooth.avrcpcontroller.AvrcpControllerService;
import com.android.bluetooth.gatt.GattService;
import com.android.bluetooth.hdp.HealthService;
import com.android.bluetooth.hearingaid.HearingAidService;
import com.android.bluetooth.hfp.HeadsetService;
import com.android.bluetooth.hfpclient.HeadsetClientService;
import com.android.bluetooth.hid.HidDeviceService;
import com.android.bluetooth.hid.HidHostService;
import com.android.bluetooth.map.BluetoothMapService;
import com.android.bluetooth.mapclient.MapClientService;
import com.android.bluetooth.opp.BluetoothOppService;
import com.android.bluetooth.pan.PanService;
import com.android.bluetooth.pbap.BluetoothPbapService;
import com.android.bluetooth.pbapclient.PbapClientService;
import com.android.bluetooth.sap.SapService;

import java.util.ArrayList;

public class Config {
    private static final String TAG = "AdapterServiceConfig";

    private static class ProfileConfig {
        Class mClass;
        int mSupported;
        long mMask;

        ProfileConfig(Class theClass, int supportedFlag, long mask) {
            mClass = theClass;
            mSupported = supportedFlag;
            mMask = mask;
        }
    }

    /**
     * List of profile services with the profile-supported resource flag and bit mask.
     */
    private static final ProfileConfig[] PROFILE_SERVICES_AND_FLAGS = {
            new ProfileConfig(HeadsetService.class, R.bool.profile_supported_hs_hfp,
                    (1 << BluetoothProfile.HEADSET)),
            new ProfileConfig(A2dpService.class, R.bool.profile_supported_a2dp,
                    (1 << BluetoothProfile.A2DP)),
            new ProfileConfig(A2dpSinkService.class, R.bool.profile_supported_a2dp_sink,
                    (1 << BluetoothProfile.A2DP_SINK)),
            new ProfileConfig(HidHostService.class, R.bool.profile_supported_hid_host,
                    (1 << BluetoothProfile.HID_HOST)),
            new ProfileConfig(HealthService.class, R.bool.profile_supported_hdp,
                    (1 << BluetoothProfile.HEALTH)),
            new ProfileConfig(PanService.class, R.bool.profile_supported_pan,
                    (1 << BluetoothProfile.PAN)),
            new ProfileConfig(GattService.class, R.bool.profile_supported_gatt,
                    (1 << BluetoothProfile.GATT)),
            new ProfileConfig(BluetoothMapService.class, R.bool.profile_supported_map,
                    (1 << BluetoothProfile.MAP)),
            new ProfileConfig(HeadsetClientService.class, R.bool.profile_supported_hfpclient,
                    (1 << BluetoothProfile.HEADSET_CLIENT)),
            new ProfileConfig(AvrcpTargetService.class, R.bool.profile_supported_avrcp_target,
                    (1 << BluetoothProfile.AVRCP)),
            new ProfileConfig(AvrcpControllerService.class,
                    R.bool.profile_supported_avrcp_controller,
                    (1 << BluetoothProfile.AVRCP_CONTROLLER)),
            new ProfileConfig(SapService.class, R.bool.profile_supported_sap,
                    (1 << BluetoothProfile.SAP)),
            new ProfileConfig(PbapClientService.class, R.bool.profile_supported_pbapclient,
                    (1 << BluetoothProfile.PBAP_CLIENT)),
            new ProfileConfig(MapClientService.class, R.bool.profile_supported_mapmce,
                    (1 << BluetoothProfile.MAP_CLIENT)),
            new ProfileConfig(HidDeviceService.class, R.bool.profile_supported_hid_device,
                    (1 << BluetoothProfile.HID_DEVICE)),
            new ProfileConfig(BluetoothOppService.class, R.bool.profile_supported_opp,
                    (1 << BluetoothProfile.OPP)),
            new ProfileConfig(BluetoothPbapService.class, R.bool.profile_supported_pbap,
                    (1 << BluetoothProfile.PBAP)),
            new ProfileConfig(HearingAidService.class, R.bool.profile_supported_hearing_aid,
                    (1 << BluetoothProfile.HEARING_AID))
    };

    private static Class[] sSupportedProfiles = new Class[0];

    static void init(Context ctx) {
        if (ctx == null) {
            return;
        }
        Resources resources = ctx.getResources();
        if (resources == null) {
            return;
        }

        ArrayList<Class> profiles = new ArrayList<>(PROFILE_SERVICES_AND_FLAGS.length);
        for (ProfileConfig config : PROFILE_SERVICES_AND_FLAGS) {
            boolean supported = resources.getBoolean(config.mSupported);
            if (supported && !isProfileDisabled(ctx, config.mMask)) {
                Log.v(TAG, "Adding " + config.mClass.getSimpleName());
                profiles.add(config.mClass);
            }
            sSupportedProfiles = profiles.toArray(new Class[profiles.size()]);
        }
    }

    static Class[] getSupportedProfiles() {
        return sSupportedProfiles;
    }

    private static long getProfileMask(Class profile) {
        for (ProfileConfig config : PROFILE_SERVICES_AND_FLAGS) {
            if (config.mClass == profile) {
                return config.mMask;
            }
        }
        Log.w(TAG, "Could not find profile bit mask for " + profile.getSimpleName());
        return 0;
    }

    static long getSupportedProfilesBitMask() {
        long mask = 0;
        for (final Class profileClass : getSupportedProfiles()) {
            mask |= getProfileMask(profileClass);
        }
        return mask;
    }

    private static boolean isProfileDisabled(Context context, long profileMask) {
        final ContentResolver resolver = context.getContentResolver();
        final long disabledProfilesBitMask =
                Settings.Global.getLong(resolver, Settings.Global.BLUETOOTH_DISABLED_PROFILES, 0);

        return (disabledProfilesBitMask & profileMask) != 0;
    }
}
