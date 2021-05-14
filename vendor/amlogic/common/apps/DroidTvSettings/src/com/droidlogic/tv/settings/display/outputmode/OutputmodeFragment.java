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
import com.droidlogic.tv.settings.R;


import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Map;
import com.droidlogic.tv.settings.dialog.old.Action;
import com.droidlogic.tv.settings.RadioPreference;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.preference.PreferenceManager;
import android.provider.Settings;
import android.util.Log;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.LayoutInflater;
import android.view.Window;
import android.view.WindowManager;
import android.widget.TextView;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Timer;
import java.util.TimerTask;
@Keep
public class OutputmodeFragment extends LeanbackPreferenceFragment implements OnClickListener {
    private static final String LOG_TAG = "OutputmodeFragment";
    private OutputUiManager mOutputUiManager;
    private static String preMode;
    private static String curMode;
    RadioPreference prePreference;
    RadioPreference curPreference;
    private View view_dialog;
    private TextView tx_title;
    private TextView tx_content;
    private Timer timer;
    private TimerTask task;
    private AlertDialog mAlertDialog = null;
    private int countdown = 15;
    private static String mode = null;
    private static final int MSG_FRESH_UI = 0;
    private static final int MSG_COUNT_DOWN = 1;
    private static final int MSG_PLUG_FRESH_UI = 2;
    private IntentFilter mIntentFilter;
    public boolean hpdFlag = false;
    public ArrayList<String> outputmodeTitleList = new ArrayList();
    private BroadcastReceiver mIntentReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            hpdFlag = intent.getBooleanExtra ("state", false);
            mHandler.sendEmptyMessageDelayed(MSG_PLUG_FRESH_UI, hpdFlag ^ isHdmiMode() ? 2000 : 1000);
        }
    };
    public static OutputmodeFragment newInstance() {
        return new OutputmodeFragment();
    }
    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        mOutputUiManager = new OutputUiManager(getActivity());
        mIntentFilter = new IntentFilter("android.intent.action.HDMI_PLUGGED");
        mIntentFilter.addAction(Intent.ACTION_TIME_TICK);
        updatePreferenceFragment();
    }
    private ArrayList<Action> getMainActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        ArrayList<String> outputmodeValueList = mOutputUiManager.getOutputmodeValueList();
        outputmodeTitleList.clear();
        ArrayList<String> mList = mOutputUiManager.getOutputmodeTitleList();
        for (String title : mList) {
            outputmodeTitleList.add(title);
        }
        int currentModeIndex = mOutputUiManager.getCurrentModeIndex();
        for (int i = 0; i < outputmodeTitleList.size(); i++) {
            if (i == currentModeIndex) {
                actions.add(new Action.Builder().key(outputmodeValueList.get(i))
                      .title("        " + outputmodeTitleList.get(i))
                      .checked(true).build());
             }else {
                    actions.add(new Action.Builder().key(outputmodeValueList.get(i))
                    .title("        " + outputmodeTitleList.get(i))
                    .description("").build());
             }
        }
        return actions;
    }

    @Override
    public void onResume() {
        super.onResume();
        getActivity().registerReceiver(mIntentReceiver, mIntentFilter);
        mHandler.sendEmptyMessage(MSG_PLUG_FRESH_UI);
    }

    @Override
    public void onPause() {
        super.onPause();
        getActivity().unregisterReceiver(mIntentReceiver);
        if (mAlertDialog != null) {
            Log.d(LOG_TAG, "onPause dismiss AlertDialog");
            mAlertDialog.dismiss();
        }
        if (task != null)
            task.cancel();
        mHandler.removeMessages(MSG_COUNT_DOWN);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference instanceof RadioPreference) {
            final RadioPreference radioPreference = (RadioPreference) preference;
            radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
            if (radioPreference.isChecked()) {
                preMode = mOutputUiManager.getCurrentMode().trim();
                curMode = radioPreference.getKey();
                curPreference = radioPreference;
                mOutputUiManager.change2NewMode(curMode);
                showDialog();
                curPreference.setChecked(true);
            } else {
                radioPreference.setChecked(true);
            }
        }
        return super.onPreferenceTreeClick(preference);
    }
    private void showDialog () {
        if (mAlertDialog == null) {
            LayoutInflater inflater = (LayoutInflater)getActivity().getSystemService(Context.LAYOUT_INFLATER_SERVICE);
            view_dialog = inflater.inflate(R.layout.dialog_outputmode, null);

            AlertDialog.Builder builder = new AlertDialog.Builder(getActivity());
            mAlertDialog = builder.create();
            mAlertDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);

            tx_title = (TextView)view_dialog.findViewById(R.id.dialog_title);
            tx_content = (TextView)view_dialog.findViewById(R.id.dialog_content);

            TextView button_cancel = (TextView)view_dialog.findViewById(R.id.dialog_cancel);
            button_cancel.setOnClickListener(this);

            TextView button_ok = (TextView)view_dialog.findViewById(R.id.dialog_ok);
            button_ok.setOnClickListener(this);
        }
        mAlertDialog.show();
        mAlertDialog.getWindow().setContentView(view_dialog);
        mAlertDialog.setCancelable(false);

        if (mOutputUiManager.getOutputmodeTitleList().size() <= 0) {
            tx_content.setText("Get outputmode empty!");
        } else if (mOutputUiManager.getCurrentModeIndex() < mOutputUiManager.getOutputmodeTitleList().size()) {
            tx_content.setText(getResources().getString(R.string.device_outputmode_change)
                + " " +mOutputUiManager.getOutputmodeTitleList().get(mOutputUiManager.getCurrentModeIndex()));
        }
        countdown = 15;
        if (timer == null)
            timer = new Timer();
        if (task != null)
            task.cancel();
        task = new DialogTimerTask();
        timer.schedule(task, 0, 1000);
    }
    private void recoverOutputMode() {
       mOutputUiManager.change2NewMode(preMode);
       // need revert Preference display.
       curPreference = prePreference;
       mHandler.sendEmptyMessage(MSG_FRESH_UI);
    }

    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.dialog_cancel:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                }
                recoverOutputMode();
                break;
            case R.id.dialog_ok:
                if (mAlertDialog != null) {
                    mAlertDialog.dismiss();
                    prePreference = curPreference;
                }
                break;
        }
        task.cancel();
    }
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage(Message msg) {
            switch (msg.what) {
                case MSG_FRESH_UI:
                    curPreference.clearOtherRadioPreferences(getPreferenceScreen());
                    curPreference.setChecked(true);
                    break;
                case MSG_COUNT_DOWN:
                    tx_title.setText(Integer.toString(countdown) + " " + getResources().getString(R.string.device_outputmode_countdown));
                    if (countdown == 0) {
                        if (mAlertDialog != null) {
                            mAlertDialog.dismiss();
                        }
                        recoverOutputMode();
                        task.cancel();
                    }
                    countdown--;
                    break;
                case MSG_PLUG_FRESH_UI:
                    updatePreferenceFragment();
                    break;
            }
        }
    };

    private class DialogTimerTask extends TimerTask {
        @Override
        public void run() {
            if (mHandler != null) {
                mHandler.sendEmptyMessage(MSG_COUNT_DOWN);
            }
        }
    };
    private boolean needfresh() {
        ArrayList<String> list = mOutputUiManager.getOutputmodeTitleList();
        //Log.d(LOG_TAG, "outputmodeTitleList: " + outputmodeTitleList.toString() + "\n list: " + list.toString());
        if (outputmodeTitleList.size() > 0 && outputmodeTitleList.size() == list.size()) {
            for (String title:outputmodeTitleList) {
                if (!list.contains(title))
                    return true;
            }
        }else {
            return true;
        }
        return false;
    }

    /**
     * Display Outputmode list based on RadioPreference style.
     */
    private void updatePreferenceFragment() {
        mOutputUiManager.updateUiMode();
        if (!needfresh()) return;
        final Context themedContext = getPreferenceManager().getContext();
        final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(
                themedContext);
        screen.setTitle(R.string.device_displaymode);
        setPreferenceScreen(screen);

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
                curMode = InfoTag;
                prePreference = curPreference = radioPreference;
            }
            screen.addPreference(radioPreference);
        }
    }
    private boolean isHdmiMode() {
        return mOutputUiManager.isHdmiMode();
    }
}
