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


import com.droidlogic.mediacenter.airplay.setting.SettingsPreferences;
import com.droidlogic.mediacenter.airplay.util.Utils;
import com.droidlogic.mediacenter.dlna.DmpService;
import com.droidlogic.mediacenter.dlna.DmpStartFragment;
import com.droidlogic.mediacenter.dlna.MediaCenterService;
import com.droidlogic.mediacenter.dlna.PrefUtils;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.IBinder;
import android.os.Handler;
import android.os.Message;
import org.cybergarage.util.Debug;
import android.util.Log;

public class WeakRefService extends Service {
    private BroadcastReceiver mReceiver;
    private Context mContext;
    private PrefUtils mPrefUtils;
    public static final int WIFI_AP_STATE_ENABLED = 13;
    public static final int WIFI_AP_STATE_FAILED = 14;
    public static final String WIFI_AP_STATE_CHANGED_ACTION = "android.net.wifi.WIFI_AP_STATE_CHANGED";
    public static final String WIFI_STAT = "wifi_state";
    private static final String TAG = "WeakRefService";
    private static final int STOPSERVICE = 0;
    private static final int STARTSERVICE = 1;
    @Override
    public IBinder onBind(Intent intent) {
        throw new UnsupportedOperationException("Not yet implemented");
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mReceiver = new IntentBroadCastReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        filter.addAction(WIFI_AP_STATE_CHANGED_ACTION);
        this.registerReceiver(mReceiver, filter);
        mContext = this;
        mPrefUtils = new PrefUtils(this);
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        this.unregisterReceiver(mReceiver);
        super.onDestroy();
    }

    class IntentBroadCastReceiver extends BroadcastReceiver {

        @Override
        public void onReceive(Context context, Intent intent) {
            if ((ConnectivityManager.CONNECTIVITY_ACTION).equals(intent.getAction())) {
                ConnectivityManager cMgr = ( ConnectivityManager ) mContext.getSystemService ( Context.CONNECTIVITY_SERVICE );
                NetworkInfo netInfo = ( NetworkInfo ) intent.getExtras ().getParcelable(ConnectivityManager.EXTRA_NETWORK_INFO);
                NetworkInfo netInfoWIFI = cMgr.getNetworkInfo(ConnectivityManager.TYPE_WIFI);
                NetworkInfo netInfoETH = cMgr.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);

                if (netInfo != null) {
                    if (netInfo.getState() == NetworkInfo.State.CONNECTED) {
                        Log.d ( TAG ,"TypeName " + netInfo.getTypeName()+ " connected");
                        mHandler.sendEmptyMessage (STOPSERVICE);
                        mHandler.sendEmptyMessageDelayed ( STARTSERVICE, 3000 );
                    } else if (netInfo.getState() == NetworkInfo.State.DISCONNECTED) {
                        Log.d ( TAG ,"TypeName " + netInfo.getTypeName()+ " disconnected");
                        mHandler.sendEmptyMessage (STOPSERVICE);
                    }

                } else {
                    if (netInfoWIFI != null && netInfoETH != null)
                        Log.d ( TAG ," No ActiveNetwork !"+ netInfoWIFI.getState() +"|" +netInfoETH.getState());
                }
            }
            if ( ( WIFI_AP_STATE_CHANGED_ACTION ).equals ( intent.getAction() ) ) {

                int wifi_AP_State =  intent.getIntExtra ( "wifi_state", 14 );
                if ( WIFI_AP_STATE_ENABLED == wifi_AP_State ) {
                    mContext.stopService ( new Intent ( mContext, MediaCenterService.class ) );
                    mHandler.sendEmptyMessage (STOPSERVICE);
                    mHandler.sendEmptyMessageDelayed ( STARTSERVICE, 3000 );
                } else {
                    mHandler.sendEmptyMessage (STOPSERVICE);
                }
            }
        }
    }
    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage ( Message msg ) {
            switch ( msg.what ) {
                case STOPSERVICE:
                    Intent mNetIntent = new Intent(DmpService.NETWORK_ERROR);
                    mContext.sendBroadcast ( mNetIntent );
                    break;
                case STARTSERVICE:
                    if (mPrefUtils.getBooleanVal(DmpStartFragment.KEY_BOOT_CFG,false)) {
                        mContext.startService (new Intent(mContext, MediaCenterService.class));
                    }
                    break;
            }
        }
    };
}
