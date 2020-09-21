/*
 * Copyright (C) 2015 The Android Open Source Project
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
 * limitations under the License
 */

package com.droidlogic.tv.settings.pqsettings;

import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.os.SystemProperties;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Toast;
import android.media.tv.TvInputInfo;
import android.widget.FrameLayout;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.LayoutInflater;
import android.view.ViewGroup;

import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.MainFragment;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.SettingsConstant;

import com.droidlogic.app.OutputModeManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;

import java.util.ArrayList;
import java.util.List;

public class PictrueModeFragment extends LeanbackPreferenceFragment implements Preference.OnPreferenceChangeListener {

    private static final String TAG = "PictrueModeFragment";
    private static final String PQ_PICTRUE_MODE = "pq_pictrue_mode";
    private static final String PQ_BRIGHTNESS = "pq_brightness";
    private static final String PQ_CONTRAST = "pq_contrast";
    private static final String PQ_COLOR = "pq_color";
    private static final String PQ_SHARPNESS = "pq_sharpness";
    private static final String PQ_BACKLIGHT = "pq_backlight";
    private static final String PQ_COLOR_TEMPRATURE = "pq_color_temprature";
    private static final String PQ_ASPECT_RATIO = "pq_aspect_ratio";
    private static final String PQ_DNR = "pq_dnr";
    private static final String PQ_HDMI_COLOR = "pq_hdmi_color_range";
    private static final String PQ_CUSTOM = "pq_custom";

    private static final String CURRENT_DEVICE_ID = "current_device_id";
    private static final String TV_CURRENT_DEVICE_ID = "tv_current_device_id";
    private static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";

    private PQSettingsManager mPQSettingsManager;

    public static PictrueModeFragment newInstance() {
        return new PictrueModeFragment();
    }

    private boolean CanDebug() {
        return PQSettingsManager.CanDebug();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        final ListPreference picturemodePref = (ListPreference) findPreference(PQ_PICTRUE_MODE);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        if (mPQSettingsManager.isHdmiSource()) {
            picturemodePref.setEntries(setHdmiPicEntries());
            picturemodePref.setEntryValues(setHdmiPicEntryValues());
        }
        picturemodePref.setValue(mPQSettingsManager.getPictureModeStatus());

        int is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0);
        String currentInputInfoId = getActivity().getIntent().getStringExtra("current_tvinputinfo_id");
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        boolean hasMboxFeature = SettingsConstant.hasMboxFeature(getActivity());
        String curPictureMode = mPQSettingsManager.getPictureModeStatus();
        final Preference backlightPref = (Preference) findPreference(PQ_BACKLIGHT);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_backlight)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_backlight)) ||
                (isTv && isDtvKitInput(currentInputInfoId))) {
            backlightPref.setSummary(mPQSettingsManager.getBacklightStatus() + "%");
        } else {
            backlightPref.setVisible(false);
        }

        final ListPreference dnrPref = (ListPreference) findPreference(PQ_DNR);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_dnr)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_dnr))) {
            if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
                curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
                dnrPref.setVisible(false);
            } else {
                dnrPref.setVisible(true);
                dnrPref.setValueIndex(mPQSettingsManager.getDnrStatus());
            }
        } else {
            dnrPref.setVisible(false);
        }

        final Preference pictureCustomerPref = (Preference) findPreference(PQ_CUSTOM);
        if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
            curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
            pictureCustomerPref.setVisible(false);
        } else {
            pictureCustomerPref.setVisible(true);
        }
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        Log.d(TAG, "onCreateView");
        final View innerView = super.onCreateView(inflater, container, savedInstanceState);
        if (getActivity().getIntent().getIntExtra("from_live_tv", 0) == 1) {
            //MainFragment.changeToLiveTvStyle(innerView, getActivity());
        }
        return innerView;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        Log.d(TAG, "onCreatePreferences");
        setPreferencesFromResource(R.xml.pq_pictrue_mode, null);
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }

        int is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0);
        boolean isTv = SettingsConstant.needDroidlogicTvFeature(getActivity());
        boolean hasMboxFeature = SettingsConstant.hasMboxFeature(getActivity());
        String curPictureMode = mPQSettingsManager.getPictureModeStatus();
        final ListPreference picturemodePref = (ListPreference) findPreference(PQ_PICTRUE_MODE);
        Log.d(TAG, "curPictureMode: " + curPictureMode + "isTv: " + isTv + "isLiveTv: " + is_from_live_tv);
        if (mPQSettingsManager.isHdmiSource()) {
            picturemodePref.setEntries(setHdmiPicEntries());
            picturemodePref.setEntryValues(setHdmiPicEntryValues());
        }
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_pictrue_mode)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_pictrue_mode))) {
            picturemodePref.setValue(curPictureMode);
            picturemodePref.setOnPreferenceChangeListener(this);
        } else {
            picturemodePref.setVisible(false);
        }
        final ListPreference aspectratioPref = (ListPreference) findPreference(PQ_ASPECT_RATIO);
        if (is_from_live_tv == 1 || (isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_aspect_ratio)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_aspect_ratio))) {
            aspectratioPref.setValueIndex(mPQSettingsManager.getAspectRatioStatus());
            aspectratioPref.setOnPreferenceChangeListener(this);
        } else {
            aspectratioPref.setVisible(false);
        }
        final ListPreference colortemperaturePref = (ListPreference) findPreference(PQ_COLOR_TEMPRATURE);
        if (!hasMboxFeature) {
            if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_color_temprature)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_color_temprature))) {
                if (is_from_live_tv == 1) {
                    colortemperaturePref.setValueIndex(mPQSettingsManager.getColorTemperatureStatus());
                } else {
                    colortemperaturePref.setVisible(false);
                }
                colortemperaturePref.setOnPreferenceChangeListener(this);
            } else {
                colortemperaturePref.setVisible(false);
            }
        } else {
            colortemperaturePref.setVisible(false);
        }
        final ListPreference dnrPref = (ListPreference) findPreference(PQ_DNR);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_dnr)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_dnr))) {
            if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
                curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
                dnrPref.setVisible(false);
            } else {
                dnrPref.setValueIndex(mPQSettingsManager.getDnrStatus());
                dnrPref.setOnPreferenceChangeListener(this);
            }
        } else {
            dnrPref.setVisible(false);
        }
        final Preference backlightPref = (Preference) findPreference(PQ_BACKLIGHT);
        if ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_backlight)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_backlight))) {
            backlightPref.setSummary(mPQSettingsManager.getBacklightStatus() + "%");
        } else {
            backlightPref.setVisible(false);
        }
        final ListPreference hdmicolorPref = (ListPreference) findPreference(PQ_HDMI_COLOR);
        if (mPQSettingsManager.isHdmiSource() && ((isTv && getActivity().getResources().getBoolean(R.bool.tv_pq_need_hdmicolor)) ||
                (!isTv && getActivity().getResources().getBoolean(R.bool.box_pq_need_hdmicolor)))) {
            hdmicolorPref.setValueIndex(mPQSettingsManager.getHdmiColorRangeStatus());
            hdmicolorPref.setOnPreferenceChangeListener(this);
        } else {
            hdmicolorPref.setVisible(false);
        }
        final Preference pictureCustomerPref = (Preference) findPreference(PQ_CUSTOM);
        if (curPictureMode.equals(PQSettingsManager.STATUS_MONITOR) ||
            curPictureMode.equals(PQSettingsManager.STATUS_GAME)) {
            pictureCustomerPref.setVisible(false);
        } else {
            pictureCustomerPref.setVisible(true);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (CanDebug()) Log.d(TAG, "[onPreferenceTreeClick] preference.getKey() = " + preference.getKey());
        return super.onPreferenceTreeClick(preference);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        Log.d(TAG, "[onPreferenceChange] preference.getKey() = " + preference.getKey() + ", newValue = " + newValue);
        //final int selection = Integer.parseInt((String)newValue);
        if (TextUtils.equals(preference.getKey(), PQ_PICTRUE_MODE)) {
            mPQSettingsManager.setPictureMode((String)newValue);
        } else if (TextUtils.equals(preference.getKey(), PQ_COLOR_TEMPRATURE)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setColorTemperature(selection);
        } else if (TextUtils.equals(preference.getKey(), PQ_DNR)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setDnr(selection);
        } else if (TextUtils.equals(preference.getKey(), PQ_ASPECT_RATIO)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setAspectRatio(selection);
        } else if (TextUtils.equals(preference.getKey(), PQ_HDMI_COLOR)) {
            final int selection = Integer.parseInt((String)newValue);
            mPQSettingsManager.setHdmiColorRangeValue(selection);
        }
        return true;
    }

    private final int[] HDMI_PIC_RES = {R.string.pq_standard, R.string.pq_vivid, R.string.pq_soft, R.string.pq_sport, R.string.pq_movie, R.string.pq_monitor,R.string.pq_game,R.string.pq_user};
    private final String[] HDMI_PIC_MODE = {PQSettingsManager.STATUS_STANDARD, PQSettingsManager.STATUS_VIVID, PQSettingsManager.STATUS_SOFT,
        PQSettingsManager.STATUS_SPORT, PQSettingsManager.STATUS_MOVIE, PQSettingsManager.STATUS_MONITOR, PQSettingsManager.STATUS_GAME, PQSettingsManager.STATUS_USER};

    private String[] setHdmiPicEntries() {
        String[] temp = null;//new String[HDMI_PIC_RES.length];
        List<String> list = new ArrayList<String>();
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        if (mPQSettingsManager.isHdmiSource()) {
            for (int i = 0; i < HDMI_PIC_RES.length; i++) {
                list.add(getString(HDMI_PIC_RES[i]));
            }
        }
        temp = (String[])list.toArray(new String[list.size()]);

        return temp;
    }

    private String[] setHdmiPicEntryValues() {
        String[] temp = null;//new String[HDMI_PIC_MODE.length];
        List<String> list = new ArrayList<String>();
        if (mPQSettingsManager == null) {
            mPQSettingsManager = new PQSettingsManager(getActivity());
        }
        if (mPQSettingsManager.isHdmiSource()) {
            for (int i = 0; i < HDMI_PIC_MODE.length; i++) {
                list.add(HDMI_PIC_MODE[i]);
            }
        }
        temp = (String[])list.toArray(new String[list.size()]);

        return temp;
    }

    private static boolean isDtvKitInput(String inputId) {
        boolean result = false;
        if (inputId != null && inputId.startsWith(DTVKIT_PACKAGE)) {
            result = true;
        }
        Log.d(TAG, "isDtvKitInput result = " + result);
        return result;
    }

}
