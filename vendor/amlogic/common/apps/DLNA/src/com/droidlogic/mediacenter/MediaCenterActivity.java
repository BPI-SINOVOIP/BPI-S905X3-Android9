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

import java.util.List;
import java.util.Map;

import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.dlna.DMRError;
import com.droidlogic.mediacenter.dlna.DmpFragment;
import com.droidlogic.mediacenter.dlna.DmpService;
import com.droidlogic.mediacenter.dlna.DmpStartFragment;
import com.droidlogic.mediacenter.dlna.MediaCenterService;
import com.droidlogic.mediacenter.dlna.PrefUtils;
import com.droidlogic.mediacenter.dlna.DmpFragment.FreshListener;
import com.droidlogic.mediacenter.dlna.DmpService.DmpBinder;
import com.droidlogic.mediacenter.airplay.setting.SettingsPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.NetworkInfo.State;
import android.os.Bundle;
import android.os.IBinder;
import android.app.Activity;
import android.app.Fragment;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.ServiceConnection;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.animation.Animation;
import android.view.animation.AnimationSet;
import android.view.animation.AnimationUtils;
import android.widget.TextView;

public class MediaCenterActivity extends Activity  implements FreshListener {
        private static final String TAG = "DLNA";
        private static final String SYSTEM_DIALOG_REASON_KEY = "reason";
        private static final String SYSTEM_DIALOG_REASON_HOME_KEY = "homekey";
        private PrefUtils mPrefUtils;
        private Animation animation;
        //private DmpBinder mBinder;
        private DmpService mService;
        private boolean mStartDmp;
        private ServiceConnection mConn = null;
        private TextView mDeviceName = null;
        private Fragment mCallbacks;
        private Context mContent;
        @Override
        protected void onCreate ( Bundle savedInstanceState ) {
            super.onCreate ( savedInstanceState );
            setContentView ( R.layout.activity_main );
            mPrefUtils = new PrefUtils ( this );
            animation = ( AnimationSet ) AnimationUtils.loadAnimation ( this, R.anim.refresh_btn );
            mDeviceName = ( TextView ) findViewById ( R.id.device_name );
            mContent = this;
            if (mPrefUtils.getBooleanVal ( SettingsPreferences.KEY_BOOT_CFG, false )) {
                Log.e(TAG,"Start Service on create");
                checkNet();
            }
            LogStart();
        }

        private void startDmpService() {
            mConn = new ServiceConnection() {
                @Override
                public void onServiceConnected ( ComponentName name, IBinder service ) {
                    DmpBinder mBinder = ( DmpBinder ) service;
                    mService = mBinder.getServiceInstance();
                }
                @Override
                public void onServiceDisconnected ( ComponentName name ) {
                    mService.forceStop();
                }
            };
            getApplicationContext().bindService ( new Intent ( mContent, DmpService.class ), mConn, Context.BIND_AUTO_CREATE );
            mStartDmp = true;
        }



        /**
         * @Description TODO
         * @return
         */
        public PrefUtils getPref() {
            return mPrefUtils;
        }


        @Override
        protected void onDestroy() {
            stopMediaCenterService();
            stopDmpService();
            super.onDestroy();
        }

        private void stopDmpService() {
            if ( mStartDmp && mConn != null ) {
                mStartDmp = false;
                getApplicationContext().unbindService ( mConn );
            }
        }

        @Override
        protected void onResume() {
            super.onResume();
            checkNet();
            registerHomeKeyReceiver(mContent);
            /*mRefreshView.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    animation.startNow();
                }
            });*/
            showDeviceName();
        }
        @Override
        protected void onPause() {
            unregisterHomeKeyReceiver(mContent);
            super.onPause();
        }

        public void showDeviceName() {
            String serviceName = mPrefUtils.getString ( SettingsFragment.KEY_DEVICE_NAME, getString ( R.string.config_default_name ) );
            mDeviceName.setText ( serviceName );
        }

        public void startMediaCenterService() {
            boolean startApk = mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_START_SERVICE, false );
            boolean startReboot = mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_BOOT_CFG, false );
            if ( startApk || startReboot ) {
                Intent intent = new Intent ( mContent, MediaCenterService.class );
                startService ( intent );
            }
        }

        private void stopMediaCenterService() {
            boolean startApk = mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_START_SERVICE, false );
            boolean startReboot = mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_BOOT_CFG, false );
            if ( !startReboot ) {
                Intent intent = new Intent ( mContent, MediaCenterService.class );
                stopService ( intent );
            }
        }

        /* (non-Javadoc)
         * @see com.droidlogic.mediacenter.DmpFragment.FreshListener#startSearch()
         */
        @Override
        public void startSearch() {
            if ( mService != null )
            { mService.startSearch(); }
        }
        @Override
        public void startDMP(){
            if ( mService != null )
                mService.restartDmp();
        }

        @Override
        public void stopDMP(){
            if ( mService != null )
                mService.forceStop();
        }
        /* (non-Javadoc)
         * @see com.droidlogic.mediacenter.DmpFragment.FreshListener#getFullList()
         */
        @Override
        public List<String> getDevList() {
            if ( mService == null )
            { return null; }
            return mService.getDevList();
        }

        /* (non-Javadoc)
         * @see com.droidlogic.mediacenter.FreshListener#getDevIcon(java.lang.String)
         */
        @Override
        public String getDevIcon ( String path ) {
            if ( mService == null )
            { return null; }
            return mService.getDeviceIcon ( path );
        }

        public List<Map<String, Object>> getBrowseResult ( String didl_str, List<Map<String, Object>> list, int itemTypeDir, int itemImgUnsel ) {
            if ( mService == null )
            { return null; }
            return mService.getBrowseResult ( didl_str, list, itemTypeDir, itemImgUnsel );
        }

        public String actionBrowse ( String mediaServerName, String item_id,
        String flag ) {
            if ( mService == null )
            { return null; }
            return mService.actionBrowse ( mediaServerName, item_id, flag );
        }

        @Override
        public boolean onKeyDown ( int keyCode, KeyEvent event ) {
            if ( keyCode == KeyEvent.KEYCODE_BACK ) {
                mCallbacks = getFragmentManager().findFragmentById ( R.id.frag_detail );
                if ( mCallbacks instanceof Callbacks ) {
                    ( ( Callbacks ) mCallbacks ).onBackPressedCallback();
                } else {
                    stopMediaCenterService();
                    stopDmpService();
                    MediaCenterActivity.this.finish();
                }
                return true;
            }
            return super.onKeyDown ( keyCode, event );
        }

        private void registerHomeKeyReceiver(Context context) {
            Log.i(TAG, "registerHomeKeyReceiver");
            final IntentFilter homeFilter = new IntentFilter(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
            context.registerReceiver(mHomeKeyReceiver, homeFilter);
        }

        private void unregisterHomeKeyReceiver(Context context) {
            Log.i(TAG, "unregisterHomeKeyReceiver");
            if (null != mHomeKeyReceiver) {
                context.unregisterReceiver(mHomeKeyReceiver);
            }
        }
        private void checkNet() {
            ConnectivityManager mConnectivityManager = ( ConnectivityManager ) mContent.getSystemService ( Context.CONNECTIVITY_SERVICE );
            NetworkInfo wifiInfo = mConnectivityManager.getNetworkInfo ( ConnectivityManager.TYPE_WIFI );
            NetworkInfo ethInfo = mConnectivityManager.getNetworkInfo ( ConnectivityManager.TYPE_ETHERNET );
            NetworkInfo mobileInfo = mConnectivityManager.getNetworkInfo ( ConnectivityManager.TYPE_MOBILE );
            if ( ethInfo == null && wifiInfo == null && mobileInfo == null ) {
                Intent mIntent = new Intent();
                mIntent.addFlags ( Intent.FLAG_ACTIVITY_NEW_TASK );
                mIntent.setClass ( mContent, DMRError.class );
                startActivity ( mIntent );
                return;
            }
            if ( ( ethInfo != null && ethInfo.isConnectedOrConnecting() ) ||
                    ( wifiInfo != null && wifiInfo.isConnectedOrConnecting() ) ||
            ( mobileInfo != null && mobileInfo.isConnectedOrConnecting() ) ) {
                if ( mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_START_SERVICE, false ) || mPrefUtils.getBooleanVal ( DmpStartFragment.KEY_BOOT_CFG, false ) ) {
                    startMediaCenterService();
                }
                startDmpService();
            } else {
                Intent mIntent = new Intent();
                mIntent.addFlags ( Intent.FLAG_ACTIVITY_NEW_TASK );
                mIntent.setClass ( mContent, DMRError.class );
                startActivity ( mIntent );
            }
        }

        public interface Callbacks {
            public void onBackPressedCallback();
        }

        public void LogStart() {
            if ( PrefUtils.getProperties ( "rw.app.dlna.debug", "false" ).equals("true") ) {
                org.cybergarage.util.Debug.on(); //LOG OFF
            } else {
                org.cybergarage.util.Debug.off();  //LOG ON
            }
        }
        private BroadcastReceiver mHomeKeyReceiver =
            new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                String action = intent.getAction();
                Log.i(TAG, "onReceive: action: " + action);
                if (action.equals(Intent.ACTION_CLOSE_SYSTEM_DIALOGS)) {
                    String reason = intent.getStringExtra(SYSTEM_DIALOG_REASON_KEY);
                    Log.i(TAG, "reason: " + reason);
                    if (SYSTEM_DIALOG_REASON_HOME_KEY.equals(reason)) {
                        Log.i(TAG, "homekey stop service");
                        stopMediaCenterService();
                        stopDmpService();
                        MediaCenterActivity.this.finish();
                    }
                }

            }
        };
}
