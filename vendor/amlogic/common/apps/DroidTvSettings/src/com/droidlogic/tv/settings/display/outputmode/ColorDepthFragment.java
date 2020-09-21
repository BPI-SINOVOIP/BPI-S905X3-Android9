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

package com.droidlogic.tv.settings.display.outputmode;

import android.app.Activity;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.support.annotation.Keep;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.CheckBoxPreference;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.text.TextUtils;
import com.droidlogic.tv.settings.dialog.old.Action;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.R;

import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.provider.Settings;
import android.util.Log;
import android.view.View;
import android.widget.TextView;
import android.widget.Toast;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;

@Keep
public class ColorDepthFragment extends LeanbackPreferenceFragment {
    private static final String LOG_TAG = "ColorDepthFragment";
    private OutputUiManager mOutputUiManager;
    private static String saveValue = null;
    private static String curValue = null;
    private static String curMode = null;
    private static final int MSG_FRESH_UI = 0;
    private IntentFilter mIntentFilter;
    public boolean hpdFlag = false;
    private static final String DEFAULT_COLOR_DEPTH_VALUE = "8bit";
    private String ENABLE_COLOR_DEPTH_VALUE = "12bit";
    private static final String ACTION_ON = "on";
    private static final String ACTION_OFF = "off";

    private ArrayList<String> curValueList = new ArrayList();
    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            hpdFlag = intent.getBooleanExtra ("state", false);
            mHandler.sendEmptyMessageDelayed(MSG_FRESH_UI, hpdFlag ^ isHdmiMode() ? 2000 : 1000);
        }
    };
    public static ColorDepthFragment newInstance() {
        return new ColorDepthFragment();
    }
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mOutputUiManager = new OutputUiManager(getActivity());
        updatePreferenceFragment();
    }
    private boolean needfresh() {
        ArrayList<String> list = mOutputUiManager.getColorValueList();
        //Log.d(LOG_TAG, "curValueList: " + curValueList.toString() + "\n list: " + list.toString());
        if (curValueList.size() > 0 && curValueList.size() == list.size()) {
            for (String title : curValueList) {
                if (!list.contains(title))
                    return true;
            }
        }else {
            return true;
        }
        return false;
    }
    private void updatePreferenceFragment() {
        mOutputUiManager.updateUiMode();
        if (!needfresh()) return;
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.device_outputmode_color_depth);
        //R.string.device_outputmode_color_depth
        setPreferenceScreen(screen);
        if (!isHdmiMode()) {
            curValueList.clear();
            return;
        }
        final List<Action> InfoList = getMainActions();
        for (final Action Info : InfoList) {
            final String InfoTag = Info.getKey();
            final RadioPreference radioPreference = new RadioPreference(themedContext);
            radioPreference.setKey(InfoTag);
            radioPreference.setPersistent(false);
            radioPreference.setTitle(Info.getTitle());
            radioPreference.setLayoutResource(R.layout.preference_reversed_widget);
            if (Info.isChecked()) {
                radioPreference.setChecked(true);
            }
            screen.addPreference(radioPreference);
        }
    }
    private boolean isModeSupportColor(final String curMode, final String curValue){
        boolean  ret = false;
        ret = mOutputUiManager.isModeSupportColor(curMode, curValue);
        return ret;
    }

    private ArrayList<Action> getMainActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        curValueList.clear();
        ArrayList<String> mList = mOutputUiManager.getColorValueList();
        for (String color:mList) {
            curValueList.add(color);
        }
        ArrayList<String> colorValueList = mOutputUiManager.getColorValueList();
        String value = null;
        String filterValue = null;
        String  curColorDepthValue = mOutputUiManager.getCurrentColorAttribute().toString().trim();
        Log.i(LOG_TAG,"curColorDepthValue: "+curColorDepthValue);
        if (curColorDepthValue.equals("default"))
            curColorDepthValue = DEFAULT_COLOR_DEPTH_VALUE;
        for (int i = 0; i < curValueList.size(); i++) {
            value = colorValueList.get(i).trim();
            curMode = mOutputUiManager.getCurrentMode().trim();
            if (!isModeSupportColor(curMode, value)) {
                continue;
            }
            filterValue += value;
        }
        if (filterValue != null) {
            if (filterValue.contains(mOutputUiManager.getCurrentColorSpaceAttr().trim() + "," + "12bit")) {
                ENABLE_COLOR_DEPTH_VALUE = "12bit";
                actions.add(new Action.Builder().key(ACTION_ON)
                    .title("        " + getString(R.string.on))
                    .checked(curColorDepthValue.contains(ENABLE_COLOR_DEPTH_VALUE) ? true : false)
                    .description("")
                    .build());
            } else if (filterValue.contains(mOutputUiManager.getCurrentColorSpaceAttr().trim() + "," + "10bit")) {
                ENABLE_COLOR_DEPTH_VALUE = "10bit";
                actions.add(new Action.Builder().key(ACTION_ON)
                    .title("        " + getString(R.string.on))
                    .checked(curColorDepthValue.contains(ENABLE_COLOR_DEPTH_VALUE) ? true : false)
                    .description("")
                    .build());
            }
        }
        actions.add(new Action.Builder().key(ACTION_OFF)
                .title("        " + getString(R.string.off))
                .checked(curColorDepthValue.contains(DEFAULT_COLOR_DEPTH_VALUE) ? true : false)
                .description("")
                .build());
        return actions;
    }
    @Override
    public void onResume() {
        super.onResume();
        mIntentFilter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        getActivity().registerReceiver(mIntentReceiver, mIntentFilter);
    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mIntentReceiver);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            if (radioPreference.isChecked()) {
                if (onClickHandle(radioPreference.getKey()) == true) {
                    radioPreference.setChecked(true);
                }
            } else {
                radioPreference.setChecked(true);
                Log.i(LOG_TAG,"not checked");
            }
        }
      return super.onPreferenceTreeClick(preference);
    }
    public boolean onClickHandle(String key) {
        curValue = key.contains(ACTION_ON) ? ENABLE_COLOR_DEPTH_VALUE : DEFAULT_COLOR_DEPTH_VALUE;
        saveValue= mOutputUiManager.getCurrentColorDepthAttr().toString().trim();
        if (saveValue.equals("default"))
            saveValue = DEFAULT_COLOR_DEPTH_VALUE;
        curMode = mOutputUiManager.getCurrentMode().trim();
        Log.i(LOG_TAG,"Set Color Depth CurValue: "+curValue + "PreValue: "+saveValue);

        if (!curValue.equals(saveValue)) {
            curValue = mOutputUiManager.getCurrentColorSpaceAttr().trim() + "," + curValue;
            if (isModeSupportColor(curMode,curValue)) {
               mOutputUiManager.changeColorAttribte(curValue);
               return true;
           }
        }
        return false;
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    updatePreferenceFragment();
                    break;
            }
        }
    };
    private boolean isHdmiMode() {
        return mOutputUiManager.isHdmiMode();
    }
}
