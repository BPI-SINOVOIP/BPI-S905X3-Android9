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
package com.droidlogic.mediacenter;

import android.app.Activity;
import android.app.Dialog;
import android.content.DialogInterface;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.SharedPreferences.OnSharedPreferenceChangeListener;
import android.os.Bundle;
import android.preference.Preference;
import android.preference.Preference.OnPreferenceChangeListener;
import android.preference.PreferenceScreen;

import com.droidlogic.mediacenter.airplay.setting.NameSetDialog;
import com.droidlogic.mediacenter.airplay.setting.SettingsPreferenceFragment;
import com.droidlogic.mediacenter.airplay.util.Utils;
import com.droidlogic.mediacenter.dlna.MediaCenterService;
import com.droidlogic.mediacenter.dlna.PrefUtils;

/**
 * @ClassName SettingsFragment
 * @Description TODO
 * @Date 2013-9-11
 * @Email
 * @Author
 * @Version V1.0
 */
public class SettingsFragment extends SettingsPreferenceFragment implements
    DialogInterface.OnClickListener, OnSharedPreferenceChangeListener,
        OnPreferenceChangeListener {
        public static final String KEY_NAME = "name";
        public static final String KEY_VERSION = "version";
        public static final String KEY_DEVICE_NAME = PrefUtils.SERVICE_NAME;
        private Preference mVersionPref;
        private Preference mDeviceNamePref;
        private static final int DIALOG_NAME_SETTINGS = 1;
        private NameSetDialog mDialog;
        private String mDeviceName;
        //private AirplayProxy mAirplayProxy;
        /* (non-Javadoc)
         * @see android.preference.Preference.OnPreferenceChangeListener#onPreferenceChange(android.preference.Preference, java.lang.Object)
         */
        @Override
        public boolean onPreferenceChange ( Preference arg0, Object arg1 ) {
            return false;
        }

        /* (non-Javadoc)
         * @see android.content.SharedPreferences.OnSharedPreferenceChangeListener#onSharedPreferenceChanged(android.content.SharedPreferences, java.lang.String)
         */
        @Override
        public void onSharedPreferenceChanged ( SharedPreferences arg0, String arg1 ) {
        }

        @Override
        public void onCreate ( Bundle icicle ) {
            super.onCreate ( icicle );
            addPreferencesFromResource ( R.xml.about_preferences );
            mVersionPref = ( Preference ) findPreference ( KEY_VERSION );
            mDeviceNamePref = ( Preference ) findPreference ( KEY_NAME );
            mVersionPref.setLayoutResource(R.layout.preference);
            mDeviceNamePref.setLayoutResource(R.layout.preference);
            //mAirplayProxy = AirplayProxy.getInstance(getActivity());
        }
        @Override
        public void onResume() {
            super.onResume();
            SharedPreferences prefs = Utils.getSharedPreferences ( getActivity() );
            mDeviceName = prefs.getString ( KEY_DEVICE_NAME, getActivity().getString ( R.string.config_default_name ) );
            mDeviceNamePref.setSummary ( mDeviceName );
        }

        @Override
        public boolean onPreferenceTreeClick ( PreferenceScreen screen, Preference preference ) {
            if ( preference == mDeviceNamePref ) {
                showDialog ( DIALOG_NAME_SETTINGS );
            }
            return super.onPreferenceTreeClick ( screen, preference );
        }

        @Override
        public void onClick ( DialogInterface dialogInterface, int button ) {
            if ( button == DialogInterface.BUTTON_POSITIVE ) {
                onDeviceNameChanged ( mDialog.getName() );
            }
        }

        @Override
        public Dialog onCreateDialog ( int id ) {
            if ( id == DIALOG_NAME_SETTINGS ) {
                final Activity activity = getActivity();
                mDialog = new NameSetDialog ( activity, this, mDeviceName );
                return mDialog;
            }
            return null;
        }

        private void onDeviceNameChanged ( String name ) {
            if ( name != null && !mDeviceName.equals ( name ) ) {
                mDeviceName = name;
                SharedPreferences prefs = Utils.getSharedPreferences ( getActivity() );
                prefs.edit().putString ( KEY_DEVICE_NAME, mDeviceName )
                .commit();
                mDeviceNamePref
                .setSummary ( prefs.getString ( KEY_DEVICE_NAME,
                                                getActivity().getString ( R.string.config_default_name ) ) );
                send2DLNAServer ( name );
            }
        }

        /**
         * @Description TODO
         */
        private void send2DLNAServer ( String name ) {
            Intent intent = new Intent (
                MediaCenterService.SERVICE_NAME_CHANGE );
            intent.putExtra ( "service_name", name );
            getActivity().sendBroadcast ( intent );
            ( ( MediaCenterActivity ) getActivity() ).showDeviceName();
        }
}
