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

import android.content.Context;
import android.os.Bundle;
import android.os.Handler;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.os.SystemClock;
import android.os.SystemProperties;
import android.os.PowerManager;
import android.provider.Settings;
import android.support.v17.preference.LeanbackPreferenceFragment;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.util.ArrayMap;
import android.util.Log;

import com.droidlogic.tv.settings.R;
import com.droidlogic.tv.settings.RadioPreference;
import com.droidlogic.tv.settings.dialog.old.Action;

import java.util.List;
import java.util.Map;
import java.util.ArrayList;

public class PowerKeyActionDefinitionFragment extends LeanbackPreferenceFragment {
	private static final String TAG = "PowerKeyActionDefinitionFragment";
	private static final String POWERKEYACTION_RADIO_GROUP = "PowerKeyAction";
	private static final String POWER_KEY_DEFINITION = "power_key_definition";
	private static final String POWER_KEY_SUSPEND = "power_key_suspend";
	private static final String POWER_KEY_SHUTDOWN = "power_key_shutdown";
	private static final String POWER_KEY_RESTART = "power_key_restart";
       private static final int SUSPEND = 0;
       private static final int SHUTDOWN = 1;
       private static final int RESTART = 2;
	private Context mContext;
	private static final int POWERKEY_SET_DELAY_MS = 500;
	private final Handler mDelayHandler = new Handler();
	private String mNewKeyDefinition;
	private boolean mIsTv;
	private final Runnable mSetPowerKeyActionRunnable = new Runnable() {
		@Override
		public void run() {
			PowerManager pm = (PowerManager) mContext.getSystemService(Context.POWER_SERVICE);
			if (POWER_KEY_SUSPEND.equals(mNewKeyDefinition)) {
				setPowerKeyActionDefinition(SUSPEND);
			} else if (POWER_KEY_SHUTDOWN.equals(mNewKeyDefinition)) {
				setPowerKeyActionDefinition(SHUTDOWN);
			} else if (POWER_KEY_RESTART.equals(mNewKeyDefinition)) {
				setPowerKeyActionDefinition(RESTART);
			}
		}
	};

	public static PowerKeyActionDefinitionFragment newInstance() {
		return new PowerKeyActionDefinitionFragment();
	}

	@Override
	public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
		final Context themedContext = getPreferenceManager().getContext();
		mContext = themedContext;
		mIsTv = SettingsConstant.needDroidlogicTvFeature(mContext);
		final PreferenceScreen screen = getPreferenceManager().createPreferenceScreen(themedContext);
		screen.setTitle(R.string.system_powerkeyaction);
		String currentPowerKeyMode = null;
		Preference activePref = null;

		final List<Action> powerkeyInfoList = getActions();
		for (final Action powerkeyInfo : powerkeyInfoList) {
			final String powerkeyTag = powerkeyInfo.getKey();
			final RadioPreference radioPreference = new RadioPreference(themedContext);
			radioPreference.setKey(powerkeyTag);
			radioPreference.setPersistent(false);
			radioPreference.setTitle(powerkeyInfo.getTitle());
			radioPreference.setRadioGroup(POWERKEYACTION_RADIO_GROUP);
			radioPreference.setLayoutResource(R.layout.preference_reversed_widget);
			if (powerkeyInfo.isChecked()) {
				currentPowerKeyMode = powerkeyTag;
				radioPreference.setChecked(true);
				activePref = radioPreference;
			}
			screen.addPreference(radioPreference);
		}
		if (activePref != null && savedInstanceState == null) {
			scrollToPreference(activePref);
		}
		setPreferenceScreen(screen);
	}

	private ArrayList<Action> getActions() {
		ArrayList<Action> actions = new ArrayList<Action>();
		int checkedKey = whichPowerKeyDefinition();
		actions.add(new Action.Builder().key(POWER_KEY_SUSPEND).title(getString(R.string.power_action_suspend))
				.checked(checkedKey == SUSPEND).build());
		actions.add(new Action.Builder().key(POWER_KEY_SHUTDOWN).title(getString(R.string.power_action_shutdown))
				.checked(checkedKey == SHUTDOWN).build());
		if (!mIsTv) {
			actions.add(new Action.Builder().key(POWER_KEY_RESTART).title(getString(R.string.power_action_restart))
					.checked(checkedKey == RESTART).build());
		}
		return actions;
	}

	@Override
	public boolean onPreferenceTreeClick(Preference preference) {
		if (preference instanceof RadioPreference) {
			final RadioPreference radioPreference = (RadioPreference) preference;
			radioPreference.clearOtherRadioPreferences(getPreferenceScreen());
			mNewKeyDefinition = radioPreference.getKey().toString();
			mDelayHandler.removeCallbacks(mSetPowerKeyActionRunnable);
			mDelayHandler.postDelayed(mSetPowerKeyActionRunnable, POWERKEY_SET_DELAY_MS);
			radioPreference.setChecked(true);
		}
		return super.onPreferenceTreeClick(preference);
	}

	private int whichPowerKeyDefinition() {
		int default_value = SUSPEND;
		return Settings.System.getInt(mContext.getContentResolver(), POWER_KEY_DEFINITION, default_value);
	}

	private void setPowerKeyActionDefinition(int keyValue) {
		Settings.System.putInt(mContext.getContentResolver(), POWER_KEY_DEFINITION, keyValue);
	}
}
