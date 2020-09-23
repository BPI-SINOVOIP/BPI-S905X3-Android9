/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC HdmiCecActiveness
 */

package com.android.server.hdmi;

import android.content.Context;
import android.content.ContentResolver;
import android.provider.Settings.Global;

import android.util.Slog;

import org.json.JSONObject;
import org.json.JSONException;


/**
 * Notify the activeness of a playback
 * HDMI-CEC Activeness​integration is required in NRDP 5.2. Following doc provides the guideline
 * for Android TV Partner to integrate HDMI-CEC Activeness for Netflix.
 * Ninja uses "nrdp_video_platform_capabilities" settings to signal video output related events and
 * capabilities. The "nrdp_video_platform_capabilities" setting can be updated by invoking
 * Settings.Global.putString(getContentResolver(), "nrdp_video_platform_capabilities", jsonCaps)
 * jsonCaps is a JSON string, for example: {"activeCecState":"active", "xxx":"yyy"}
 * Ninja APK uses “activeCecState” key in “nrdp_video_platform_capabilities” json value for
 * HDMI-CEC Activeness integration. Android/Fire TV partners must report the correct
 * “activeCecState” value in “nrdp_video_platform_capabilities” json if it’s HDMI source devices
 * and device’s Ninja Validation Version >= ninja_7.
 * Following table describes the supported JSON Keys for “nrdp_video_platform_capabilities”:
 *
 * Accepted values:
 * ● "active": cecState dpi set to CEC_ACTIVE, dpi supportCecActiveVideo set to true
 * ● "inactive": cecState dpi set to CEC_INACTIVE, dpi supportCecActiveVideo set to true
 * ● "unknown": cecState set to CEC_NOT_APPLICABLE, dpi supportCecActiveVideo set to true
 * ● no activeCecState value in "nrdp_video_platform_capabilities" json string: cecState set to
 * CEC_NOT_APPLICABLE, dpi supportCecActiveVideo set to false
 * ● other value: has the same effect as no activeCecState value
 *
 * notes:
 * 1) If the device doesn't support HDMI-CEC integration or it’s non HDMI source source devices(e.g.
 * smart TV), activeCecState should not be set.
 * 2) If the device supports HDMI-CEC integration and it’s HDMI source devices(e.g. set-top-boxes and streaming
 * sticks), activeCecState should be set to "active", "inactive" (or “unknown).
 * 3) HDMI-CEC integration is mandatory for ​HDMI source devices​with Ninja Validation Version >= ninja_7
 */
public class HdmiCecActiveness {
    private static final String TAG = "HdmiCecActiveness";

    private static final String HDMI_ACTIVENESS_KEY = "activeCecState";

    private static final String CEC_ACTIVE = "active";
    private static final String CEC_INACTIVE = "inactive";
    private static final String CEC_NOT_APPLICABLE = "unknown";
    private static final String CEC_OTHER = CEC_NOT_APPLICABLE;

    private static final String SETTINGS_CEC_ACTIVENESS = "nrdp_video_platform_capabilities";

    private static String CEC_ACTIVENESS_ACTIVE_JSON = "";
    private static String CEC_ACTIVENESS_INACTIVE_JSON = "";
    private static String CEC_ACTIVENESS_UNKOWN_JSON = "";

    public static boolean mActive;

    static {
        try {
            JSONObject activeness = new JSONObject();
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_ACTIVE);
            CEC_ACTIVENESS_ACTIVE_JSON = activeness.toString();

            activeness.remove(HDMI_ACTIVENESS_KEY);
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_INACTIVE);
            CEC_ACTIVENESS_INACTIVE_JSON = activeness.toString();

            activeness.remove(HDMI_ACTIVENESS_KEY);
            activeness.put(HDMI_ACTIVENESS_KEY, CEC_NOT_APPLICABLE);
            CEC_ACTIVENESS_UNKOWN_JSON = activeness.toString();

        } catch(JSONException e) {
            Slog.e(TAG, "init HdmiCecActiveness json fail " + e);
        }
    }

    public static void notifyState(Context context, boolean active) {
        Slog.d(TAG, "notifyState " + active);
        mActive = active;
        ContentResolver cr = context.getContentResolver();
        if (active) {
            Global.putString(cr, SETTINGS_CEC_ACTIVENESS, CEC_ACTIVENESS_ACTIVE_JSON);
        } else {
            Global.putString(cr, SETTINGS_CEC_ACTIVENESS, CEC_ACTIVENESS_INACTIVE_JSON);
        }
    }

    public static void enableState(Context context, boolean enable) {
        Slog.d(TAG, "enableState " + enable);
        // mActive state should be reset.
        mActive = false;
        ContentResolver cr = context.getContentResolver();
        if (enable) {
            Global.putString(cr, SETTINGS_CEC_ACTIVENESS, CEC_ACTIVENESS_UNKOWN_JSON);
        } else {
            Global.putString(cr, SETTINGS_CEC_ACTIVENESS, "");
        }
    }
}
