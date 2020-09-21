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
package com.droidlogic.mediacenter.dlna;

import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.Preference.OnPreferenceClickListener;
import android.preference.SwitchPreference;
import org.cybergarage.util.Debug;
import com.droidlogic.mediacenter.airplay.setting.SettingsPreferenceFragment;
import com.droidlogic.mediacenter.R;

/**
 * @ClassName DmpStartFragment
 * @Description TODO
 * @Date 2013-9-11
 * @Email
 * @Author
 * @Version V1.0
 */
public class DmpStartFragment extends SettingsPreferenceFragment implements
        OnSharedPreferenceChangeListener, OnPreferenceChangeListener {
        private static final String TAG               = "SettingsPreferences";
        public static final String  KEY_START_SERVICE = "start_dmp";
        public static final String  KEY_BOOT_CFG      = "boot_dmp";
        private SwitchPreference    mStartServicePref;
        private SwitchPreference    mBootCfgPref;
        /* (non-Javadoc)
         * @see android.preference.Preference.OnPreferenceChangeListener#onPreferenceChange(android.preference.Preference, java.lang.Object)
         */
        @Override
        public boolean onPreferenceChange ( Preference preference, Object newValue ) {
            return true;
        }

        /* (non-Javadoc)
         * @see android.content.SharedPreferences.OnSharedPreferenceChangeListener#onSharedPreferenceChanged(android.content.SharedPreferences, java.lang.String)
         */
        @Override
        public void onSharedPreferenceChanged ( SharedPreferences sharedPreferences,
                                                String key ) {
            Debug.d ( "startfragment", "onSharedPreferenceChanged:" + key );
            Intent intent = new Intent ( getActivity(), MediaCenterService.class );
            if ( key.equals ( KEY_START_SERVICE ) ) {
                if ( mStartServicePref.isChecked() ) {
                    getActivity().startService ( intent );
                } else {
                    if ( !mBootCfgPref.isChecked() ) {
                        getActivity().stopService ( intent );
                    }
                }
            } else if ( key.equals ( KEY_BOOT_CFG ) ) {
                if ( mBootCfgPref.isChecked() ) {
                    getActivity().startService ( intent );
                } else {
                    if ( !mStartServicePref.isChecked() ) {
                        getActivity().stopService ( intent );
                    }
                }
            }
        }

        @Override
        public void onResume() {
            super.onResume();
        }

        private void setPreferenceListeners ( OnPreferenceChangeListener listener ) {
            mStartServicePref.setOnPreferenceChangeListener ( listener );
            mBootCfgPref.setOnPreferenceChangeListener ( listener );
        }
        @Override
        public void onCreate ( Bundle icicle ) {
            super.onCreate ( icicle );
            addPreferencesFromResource ( R.xml.settings_dlna );
            mStartServicePref = ( SwitchPreference ) findPreference ( KEY_START_SERVICE );
            mStartServicePref.setLayoutResource(R.layout.preference);
            mBootCfgPref = ( SwitchPreference ) findPreference ( KEY_BOOT_CFG );
            mBootCfgPref.setLayoutResource(R.layout.preference);
        }
        @Override
        public void onStart() {
            super.onStart();
            getPreferenceScreen().getSharedPreferences().registerOnSharedPreferenceChangeListener ( this );
            setPreferenceListeners ( this );
        }

        @Override
        public void onStop() {
            getPreferenceScreen().getSharedPreferences().unregisterOnSharedPreferenceChangeListener ( this );
            setPreferenceListeners ( null );
            super.onStop();
        }

}
