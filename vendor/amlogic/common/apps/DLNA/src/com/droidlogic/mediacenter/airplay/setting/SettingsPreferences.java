/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
*
*Licensed under the Apache License, Version 2.0 (the "License");
*you may not use this file except in compliance with the License.
*You may obtain a copy of the License at
*
*    http://www.apache.org/licenses/LICENSE-2.0
*
*Unless required by applicable law or agreed to in writing, software
*distributed under the License is distributed on an "AS IS" BASIS,
*WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
*See the License for the specific language governing permissions and
*limitations under the License.
******************************************************************/
package com.droidlogic.mediacenter.airplay.setting;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.SwitchPreference;
import android.util.Log;

import com.droidlogic.mediacenter.R;

public class SettingsPreferences extends SettingsPreferenceFragment implements
        OnSharedPreferenceChangeListener, OnPreferenceChangeListener {
        private static final String TAG               = "SettingsPreferences";
        public static final String  KEY_START_SERVICE = "start_airplay";
        public static final String  KEY_BOOT_CFG      = "boot_airplay";
        private SwitchPreference    mStartServicePref;
        private SwitchPreference    mBootCfgPref;
        // private AirplayProxy        mAirplayProxy;

        @Override
        public void onCreate ( Bundle icicle ) {
            super.onCreate ( icicle );
            addPreferencesFromResource ( R.xml.settings_airplay );
            mStartServicePref = ( SwitchPreference ) findPreference ( KEY_START_SERVICE );
            mStartServicePref.setLayoutResource(R.layout.preference);
            mBootCfgPref = ( SwitchPreference ) findPreference ( KEY_BOOT_CFG );
            //mAirplayProxy = AirplayProxy.getInstance(getActivity());
            mBootCfgPref.setLayoutResource(R.layout.preference);
        }

        @Override
        public void onResume() {
            super.onResume();
        }

        @Override
        public void onPause() {
            super.onPause();
        }

        @Override
        public void onStart() {
            super.onStart();
            getPreferenceScreen().getSharedPreferences()
            .registerOnSharedPreferenceChangeListener ( this );
            setPreferenceListeners ( this );
        }

        @Override
        public void onStop() {
            getPreferenceScreen().getSharedPreferences()
            .unregisterOnSharedPreferenceChangeListener ( this );
            setPreferenceListeners ( null );
            super.onStop();
        }

        @Override
        public boolean onPreferenceChange ( Preference preference, Object newValue ) {
            // TODO Auto-generated method stub
            return true;
        }

        @Override
        public void onSharedPreferenceChanged ( SharedPreferences sharedPreferences,
                                                String key ) {
        }

        private void setPreferenceListeners ( OnPreferenceChangeListener listener ) {
            mStartServicePref.setOnPreferenceChangeListener ( listener );
            mBootCfgPref.setOnPreferenceChangeListener ( listener );
        }
}
