/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC DisplayPositionFragment
 */



package com.droidlogic.tv.settings.display.position;

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.support.v7.preference.PreferenceCategory;
import android.util.ArrayMap;
import android.util.Log;
import android.text.TextUtils;

import com.droidlogic.app.DisplayPositionManager;
import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class DisplayPositionFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "DisplayPositionFragment";

    private static final String SCREEN_POSITION_SCALE = "screen_position_scale";
    private static final String ZOOM_IN = "zoom_in";
    private static final String ZOOM_OUT = "zoom_out";

    private DisplayPositionManager mDisplayPositionManager;

    private Preference screenPref;
    private PreferenceCategory mPref;
    private Preference zoominPref;
    private Preference zoomoutPref;

    public static DisplayPositionFragment newInstance() {
        return new DisplayPositionFragment();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.display_position, null);
        mDisplayPositionManager = new DisplayPositionManager((Context)getActivity());

        mPref       = (PreferenceCategory) findPreference(SCREEN_POSITION_SCALE);
        zoominPref  = (Preference) findPreference(ZOOM_IN);
        zoomoutPref = (Preference) findPreference(ZOOM_OUT);

        updateMainScreen();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        switch (preference.getKey()) {
            case ZOOM_IN:
                mDisplayPositionManager.zoomIn();
                break;
            case ZOOM_OUT:
                mDisplayPositionManager.zoomOut();
                break;
        }
        updateMainScreen();
        return true;
    }


    private void updateMainScreen() {
        int percent = mDisplayPositionManager.getCurrentRateValue();
        mPref.setTitle("current scaling is " + percent +"%");
    }
}
