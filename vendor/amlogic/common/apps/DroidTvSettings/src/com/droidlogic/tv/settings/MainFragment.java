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

package com.droidlogic.tv.settings;

import android.accounts.Account;
import android.accounts.AccountManager;
import android.accounts.AuthenticatorDescription;
import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import android.os.SystemProperties;
import android.os.UserHandle;
import android.os.RemoteException;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceGroup;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.content.ActivityNotFoundException;

import com.droidlogic.tv.settings.util.DroidUtils;
import com.droidlogic.tv.settings.SettingsConstant;
import com.droidlogic.tv.settings.tvoption.DroidSettingsModeFragment;
import com.droidlogic.tv.settings.tvoption.SoundParameterSettingManager;

import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;

import java.util.ArrayList;
import java.util.Set;
import java.util.List;

public class MainFragment extends LeanbackPreferenceFragment {
    private static final String TAG = "MainFragment";

    private static final String KEY_MAIN_MENU = "droidsettings";
    private static final String KEY_DISPLAY = "display";
    private static final String KEY_MBOX_SOUNDS = "mbox_sound";
    private static final String KEY_POWERKEY = "powerkey_action";
    private static final String KEY_POWERONMODE = "poweronmode_action";
    private static final String MORE_SETTINGS_APP_PACKAGE = "com.android.settings";
    private static final String KEY_UPGRADE_BLUTOOTH_REMOTE = "upgrade_bluetooth_remote";
    private static final String KEY_PLAYBACK_SETTINGS = "playback_settings";
    private static final String KEY_SOUNDS = "sound_effects";
    private static final String KEY_KEYSTONE = "keyStone";
    private static final String KEY_NETFLIX_ESN = "netflix_esn";
    private static final String KEY_MORE_SETTINGS = "more";
    private static final String KEY_PICTURE = "pictrue_mode";
    private static final String KEY_TV_OPTION = "tv_option";
    private static final String KEY_TV_CHANNEL = "channel";
    private static final String KEY_TV_SETTINGS = "tv_settings";
    private static final String KEY_HDMI_CEC_CONTROL = "hdmicec";
    private static final String DTVKIT_PACKAGE = "org.dtvkit.inputsource";
    private boolean mTvUiMode;

    private Preference mUpgradeBluetoothRemote;
    private Preference mSoundsPref;

    private String mEsnText;

    private BroadcastReceiver esnReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            mEsnText = intent.getStringExtra("ESNValue");
            findPreference(KEY_NETFLIX_ESN).setSummary(mEsnText);
        }
    };

    public static MainFragment newInstance() {
        return new MainFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.main_prefs, null);
        boolean is_from_live_tv = getActivity().getIntent().getIntExtra("from_live_tv", 0) == 1;
        String inputId = getActivity().getIntent().getStringExtra("current_tvinputinfo_id");
        mTvUiMode = DroidUtils.hasTvUiMode();
        //tvFlag, is true when TV and T962E as TV, false when Mbox and T962E as Mbox.
        boolean tvFlag = SettingsConstant.needDroidlogicTvFeature(getContext())
                && (SystemProperties.getBoolean("tv.soc.as.mbox", false) == false);

        final Preference mainPref = findPreference(KEY_MAIN_MENU);
        final Preference displayPref = findPreference(KEY_DISPLAY);
        final Preference hdmicecPref = findPreference(KEY_HDMI_CEC_CONTROL);
        final Preference playbackPref = findPreference(KEY_PLAYBACK_SETTINGS);
        mSoundsPref = findPreference(KEY_SOUNDS);
        final Preference mboxSoundsPref = findPreference(KEY_MBOX_SOUNDS);
        final Preference powerKeyPref = findPreference(KEY_POWERKEY);
        final Preference powerKeyOnModePref = findPreference(KEY_POWERONMODE);
        final Preference keyStone = findPreference(KEY_KEYSTONE);
        //BluetoothRemote/HDMI cec/Playback Settings display only in Mbox
        mUpgradeBluetoothRemote = findPreference(KEY_UPGRADE_BLUTOOTH_REMOTE);
        final Preference netflixesnPref = findPreference(KEY_NETFLIX_ESN);
        //hide it forcedly as new bluetooth remote upgrade application is not available now
        mUpgradeBluetoothRemote.setVisible(false/*is_from_live_tv ? false : (SettingsConstant.needDroidlogicBluetoothRemoteFeature(getContext()) && !tvFlag)*/);
        hdmicecPref.setVisible((getContext().getPackageManager().hasSystemFeature("android.hardware.hdmi.cec")
                && SettingsConstant.needDroidlogicHdmicecFeature(getContext())) && !is_from_live_tv);
        playbackPref.setVisible(false);
        if (netflixesnPref != null) {
            if (is_from_live_tv) {
                netflixesnPref.setVisible(false);
            } else if (getContext().getPackageManager().hasSystemFeature("droidlogic.software.netflix")) {
                netflixesnPref.setVisible(true);
                netflixesnPref.setSummary(mEsnText);
            } else {
                netflixesnPref.setVisible(false);
            }
        }

        final Preference moreSettingsPref = findPreference(KEY_MORE_SETTINGS);
        if (is_from_live_tv) {
            moreSettingsPref.setVisible(false);
         } else if (!isPackageInstalled(getActivity(), MORE_SETTINGS_APP_PACKAGE)) {
            getPreferenceScreen().removePreference(moreSettingsPref);
        }

        final Preference picturePref = findPreference(KEY_PICTURE);
        final Preference mTvOption = findPreference(KEY_TV_OPTION);
        final Preference channelPref = findPreference(KEY_TV_CHANNEL);
        final Preference settingsPref = findPreference(KEY_TV_SETTINGS);

        if (is_from_live_tv) {
            mainPref.setTitle(R.string.settings_menu);
            displayPref.setVisible(false);
            mboxSoundsPref.setVisible(false);
            powerKeyPref.setVisible(false);
            powerKeyOnModePref.setVisible(false);
            mTvOption.setVisible(false);
            moreSettingsPref.setVisible(false);
            keyStone.setVisible(false);
            boolean isPassthrough = isPassthroughInput(inputId);
            if (isPassthrough) {
                channelPref.setVisible(false);
            } else {
                channelPref.setVisible(true);
            }
            if (!SettingsConstant.needDroidlogicTvFeature(getContext())) {
                mSoundsPref.setVisible(false);//mbox doesn't surport sound effect
            }
            if (inputId != null && inputId.startsWith(DTVKIT_PACKAGE)) {
                DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_HIDE_STARTUP);
            } else {
                DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_SHOW_STARTUP);
            }
        } else {
            picturePref.setVisible(!SettingsConstant.needDroidlogicTvFeature(getContext()));
            mTvOption.setVisible(SettingsConstant.needDroidlogicTvFeature(getContext()));
            mSoundsPref.setVisible(false);
            channelPref.setVisible(false);
            settingsPref.setVisible(false);
            if (!mTvUiMode) {
                powerKeyOnModePref.setVisible(false);
            }
            DroidUtils.store(getActivity(), DroidUtils.KEY_HIDE_STARTUP, DroidUtils.VALUE_HIDE_STARTUP);
        }
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        super.onPreferenceTreeClick(preference);
        if (TextUtils.equals(preference.getKey(), KEY_TV_CHANNEL)) {
            startUiInLiveTv(KEY_TV_CHANNEL);
        } else if (TextUtils.equals(preference.getKey(), KEY_SOUNDS)) {
            startSoundEffectSettings(getActivity());
        } else if (TextUtils.equals(preference.getKey(), KEY_KEYSTONE)) {
            startKeyStoneCorrectionActivity(getActivity());
        }
        return false;
    }

    private void startUiInLiveTv(String value) {
        Intent intent = new Intent();
        intent.setAction("action.startlivetv.settingui");
        intent.putExtra(value, true);
        getActivity().sendBroadcast(intent);
        getActivity().finish();
    }

    public static void startSoundEffectSettings(Context context){
        try {
            Intent intent = new Intent();
            intent.setClassName("com.droidlogic.tv.soundeffectsettings", "com.droidlogic.tv.soundeffectsettings.SoundModeActivity");
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.d(TAG, "startSoundEffectSettings not found!");
            return;
        }
    }

    public static void startKeyStoneCorrectionActivity(Context context){
        try {
            Intent intent = new Intent();
            intent.setClassName("com.droidlogic.keystone", "com.droidlogic.keystone.keyStoneCorrectionActivity");
            context.startActivity(intent);
        } catch (ActivityNotFoundException e) {
            Log.d(TAG, "startKeyStoneCorrectionActivity not found!");
            return;
        }
    }

    @Override
    public void onStart() {
        super.onStart();
    }

    @Override
    public void onResume() {
        super.onResume();
        updateSounds();
        IntentFilter esnIntentFilter = new IntentFilter("com.netflix.ninja.intent.action.ESN_RESPONSE");
        getActivity().getApplicationContext().registerReceiver(esnReceiver, esnIntentFilter,
                "com.netflix.ninja.permission.ESN", null);
        Intent esnQueryIntent = new Intent("com.netflix.ninja.intent.action.ESN");
        esnQueryIntent.setPackage("com.netflix.ninja");
        esnQueryIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
        getActivity().getApplicationContext().sendBroadcast(esnQueryIntent);
    }

    @Override
    public void onPause() {
        super.onPause();
        if (esnReceiver != null) {
            getActivity().getApplicationContext().unregisterReceiver(esnReceiver);
        }
    }

    private void updateSounds() {
        if (mSoundsPref == null) {
            return;
        }

        mSoundsPref.setIcon(SoundParameterSettingManager.getSoundEffectsEnabled(getContext().getContentResolver())
                ? R.drawable.ic_volume_up : R.drawable.ic_volume_off);
    }

    private void hideIfIntentUnhandled(Preference preference) {
        if (preference == null) {
            return;
        }
        preference.setVisible(systemIntentIsHandled(preference.getIntent()) != null);
    }

    private static boolean isPackageInstalled(Context context, String packageName) {
        try {
            return context.getPackageManager().getPackageInfo(packageName, 0) != null;
        } catch (PackageManager.NameNotFoundException e) {
            return false;
        }
    }

    private ResolveInfo systemIntentIsHandled(Intent intent) {
        if (intent == null) {
            return null;
        }

        final PackageManager pm = getContext().getPackageManager();

        for (ResolveInfo info : pm.queryIntentActivities(intent, 0)) {
            if (info.activityInfo != null && info.activityInfo.enabled && (info.activityInfo.applicationInfo.flags
                    & ApplicationInfo.FLAG_SYSTEM) == ApplicationInfo.FLAG_SYSTEM) {
                return info;
            }
        }
        return null;
    }

    public boolean isPassthroughInput(String inputId) {
        boolean result = false;
        try {
            TvInputManager tvInputManager = (TvInputManager)getActivity().getSystemService(Context.TV_INPUT_SERVICE);
            List<TvInputInfo> inputList = tvInputManager.getTvInputList();
            for (TvInputInfo input : inputList) {
                if (input.isPassthroughInput() && TextUtils.equals(inputId, input.getId())) {
                    result = true;
                    break;
                }
            }
        } catch (Exception e) {
            Log.i(TAG, "isPassthroughInput Exception = " + e.getMessage());
        }
        return result;
    }
}
