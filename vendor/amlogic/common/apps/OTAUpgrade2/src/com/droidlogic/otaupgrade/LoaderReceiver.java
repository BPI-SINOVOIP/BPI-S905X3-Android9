/******************************************************************
*
*Copyright (C) 2016 Amlogic, Inc.
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
package com.droidlogic.otaupgrade;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

import android.net.ConnectivityManager;
import android.net.NetworkInfo;

import android.net.wifi.WifiManager;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;

import com.amlogic.update.Backup;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.util.ArrayList;

public class LoaderReceiver extends BroadcastReceiver {
        private static final String TAG = PrefUtils.TAG;
        public static final String UPDATE_GET_NEW_VERSION = "com.android.update.UPDATE_GET_NEW_VERSION";
        public static final String CHECKING_TASK_COMPLETED = "com.android.update.CHECKING_TASK_COMPLETED";
        public static final String RESTOREDATA = "com.android.amlogic.restoredata";
        public static final String BACKUPDATA = "com.android.amlogic.backupdata";
        public static String BACKUP_FILE = "/data/data/com.droidlogic.otaupgrade/BACKUP";
        public static String BACKUP_OLDFILE = "/storage/external_storage/sdcard1/BACKUP";
        private PrefUtils mPref;
        private Context mContext;

        private void getBackUpFileName() {

            ArrayList<File> devs = mPref.getStorageList(true);
            for ( int i = 0; (devs != null) && i < devs.size(); i++) {
                File dev = devs.get(i);
                if ( dev != null && dev.isDirectory() && dev.canWrite() ) {
                    BACKUP_OLDFILE = dev.getAbsolutePath();
                    BACKUP_OLDFILE += "/BACKUP";
                    break;
                }
            }
        }

        @Override
        public void onReceive ( Context context, Intent intent ) {

            mContext = context;
            mPref = new PrefUtils ( mContext );
            getBackUpFileName();
            if ( intent.getAction().equals ( Intent.ACTION_BOOT_COMPLETED ) ||
                intent.getAction().equals ( RESTOREDATA ) ) {
                try{
                    Class abcheckService = Class.forName("com.droidlogic.otaupgrade.ABCheckUpService");
                    Intent abcheck = new Intent(mContext, abcheckService);
                    mContext.startService (abcheck);
                } catch(ClassNotFoundException ex) {
                    //ex.printStackTrace();
                }
                if (intent.getAction().equals ( Intent.ACTION_BOOT_COMPLETED )) {
                    mPref.clearData();
                }
                mPref.setBoolean ( "Boot_Checked", true );
                afterReboot();
            } else if ( intent.getAction().equals ( BACKUPDATA ) ) {
                if ( PrefUtils.DEBUG ) {
                    Log.d ( TAG, "backup" );
                }
                backup();
            }
            //Log.d(TAG,"getAction:"+intent.getAction());
            if ( ( ConnectivityManager.CONNECTIVITY_ACTION ).equals ( intent.getAction() ) ) {
                Bundle bundle = intent.getExtras();
                NetworkInfo netInfo = ( NetworkInfo )bundle.getParcelable( WifiManager.EXTRA_NETWORK_INFO);
                if ( PrefUtils.DEBUG ) {
                    Log.d ( TAG,
                            "BootCompleteFlag" +
                            ( mPref.getBooleanVal ( "Boot_Checked", false ) ) + "" +
                            ( netInfo != null ) + "" +
                            mPref.getBooleanVal ( PrefUtils.PREF_AUTO_CHECK, false ) +
                            ( netInfo.getDetailedState() == NetworkInfo.DetailedState.CONNECTED ) );
                }
                if ( mPref.getBooleanVal ( "Boot_Checked", false ) &&
                        ( netInfo != null ) &&
                        ( netInfo.getDetailedState() == NetworkInfo.DetailedState.CONNECTED ) ) {
                    if ( mPref.getBooleanVal ( PrefUtils.PREF_AUTO_CHECK, false ) ||( "true" ).equals ( PrefUtils.getProperties (
                                                        "ro.product.update.autocheck", "false" ) ) ) {
                        mPref.setBoolean ( PrefUtils.PREF_AUTO_CHECK, true );
                        mPref.setBoolean ( "Boot_Checked", false );
                        mContext.startService ( new Intent (
                                                    UpdateService.ACTION_AUTOCHECK ) );
                    }
                }
            }
        }

        private void afterReboot() {
            final String[] args = { BACKUP_FILE,"restore", "-apk", "-system","-widget","-compress", "-noshared" };
            new Thread() {
                public void run() {
                    File outFile = new File(BACKUP_OLDFILE);
                    File backupFile = new File ( BACKUP_FILE );
                    if ( outFile.exists() && !backupFile.exists() ) {
                        try {
                            PrefUtils.copyFile ( BACKUP_OLDFILE, BACKUP_FILE );
                        } catch ( Exception ex ) {
                            ex.printStackTrace();
                        }
                        new File ( BACKUP_OLDFILE ).delete();
                    }
                    //boolean ismounted = Environment.MEDIA_MOUNTED.equals(Environment.getExternalStorage2State());
                    File flagFile = new File ( new File ( BACKUP_FILE ).getParentFile(),
                                               PrefUtils.FlagFile );
                    //if (ismounted) {
                    //File bkfile = new File ( BACKUP_FILE );
                    if ( flagFile.exists() ) {
                        try {
                            String files = null;
                            BufferedReader input = new BufferedReader ( new FileReader (
                                        flagFile ) );
                            while ( ( files = input.readLine() ) != null ) {
                                File temp = new File ( files );
                                if ( temp.exists() ) {
                                    temp.delete();
                                }
                            }
                            flagFile.delete();
                        } catch ( IOException ex ) {
                        }
                    }
                    if ( backupFile.exists() && !mPref.getBooleanVal ( PrefUtils.PREF_START_RESTORE, false ) ) {
                        mPref.setBoolean ( PrefUtils.PREF_START_RESTORE, true );
                        try {
                            FileInputStream fis = new FileInputStream ( backupFile );
                            if ( fis.available() <= 0 ) {
                                backupFile.delete();
                            } else {
                                Backup mBackup = new Backup ( mContext );
                                mBackup.main ( args );
                            }
                        } catch ( Exception ex ) {
                        }
                    } else if ( backupFile.exists() ) {
                        mPref.setBoolean ( PrefUtils.PREF_START_RESTORE, false );
                        backupFile.delete();
                    }
                }
            } .start();
        }

        private void backup() {
            final String[] args = { BACKUP_FILE, "backup", "-apk", "-system","-widget","-compress", "-noshared"};
            Backup mBackup = new Backup ( mContext );
            mBackup.main ( args );
        }
}
