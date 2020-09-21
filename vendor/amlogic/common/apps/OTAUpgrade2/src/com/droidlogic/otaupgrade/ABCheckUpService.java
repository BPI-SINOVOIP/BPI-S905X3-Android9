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

package com.droidlogic.otaupgrade;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.MalformedURLException;
import java.net.URL;
import java.net.URLConnection;

import com.amlogic.update.util.UpgradeInfo;
import com.amlogic.update.util.PrefUtil;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.util.Log;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiManager;
import android.os.Bundle;
public class ABCheckUpService extends Service {
    private static String TAG = "ABCheckUpService";
    private HandlerThread mRCThread;
    private Handler mRCHandler;
    private static final String AB_UPDATE="ro.build.ab_update";
    private static final String URL_UPDATE="ro.product.otaupdateurl";
    private static final String BROADCAST_ACTION = "android.action.updateui";
    public static final String REASON = "reason";
    public static final String REASON_UPDATE = "update";
    public static final String REASON_COMPLETE = "complete";
    private boolean download = false;
    private PrefUtils mPref;
    private BroadcastReceiver mReceiver;
    private UpgradeInfo mUpdateInfo;
    @Override
    public void onCreate() {
        super.onCreate();
        mRCThread = new HandlerThread("update");
        mRCThread.start();
        mRCHandler = new Handler(mRCThread.getLooper());
        mPref = new PrefUtils ( this );
        mReceiver = new ConnectReceiver();
        IntentFilter filter = new IntentFilter();
        filter.addAction(ConnectivityManager.CONNECTIVITY_ACTION);
        this.registerReceiver(mReceiver, filter);
        mUpdateInfo = new UpgradeInfo(this);
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }
    public String createParams(){
        PrefUtil pref=new PrefUtil(ABCheckUpService.this);
        if (UpgradeInfo.firmware.equals("unknown")) {
            String url = ABCheckUpService.this.getResources().getString(R.string.otaupdateurl);
            String version = ABCheckUpService.this.getResources().getString(R.string.cur_version);
            mUpdateInfo.setCustomValue(version,url);
        }
        String params = "updating_apk_version="+UpgradeInfo.updating_apk_version;
        params+="&board="+UpgradeInfo.board;
        params+="&device="+UpgradeInfo.device;
        params+="&firmware="+UpgradeInfo.firmware;
        params+="&id="+pref.getID();
        params+="&android="+UpgradeInfo.android;
        params+="&abupdate="+UpgradeInfo.getString(AB_UPDATE);
        return params;
    }
    public String sendPost(String url,String params){
         PrintWriter out = null;
         BufferedReader in = null;
         String result="";
        try {
            URL readUrl = new URL(url);
            URLConnection conn=readUrl.openConnection();
            conn.setRequestProperty("accept", "*/*");
             conn.setDoOutput(true);
             conn.setDoInput(true);
             out = new PrintWriter(conn.getOutputStream());
             out.print(params);
             out.flush();
             in = new BufferedReader(new InputStreamReader(conn.getInputStream()));
             String line;
             while ( ( line=in.readLine() ) != null ) {
                 result+=line;
             }
        } catch (MalformedURLException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        } catch (IOException ex) {
            // TODO Auto-generated catch block
            ex.printStackTrace();
        }
        finally{
            try{
                if ( out != null ) {
                    out.close();
                }
                if ( in != null ) {
                    in.close();
                }
            }catch(IOException exp){
                exp.printStackTrace();
            }
            return result;
        }
    }
    private void disableOTA(boolean disable) {
        PackageManager pmg=ABCheckUpService.this.getPackageManager();
        ComponentName component=new ComponentName(ABCheckUpService.this,MainActivity.class);
        int res = pmg.getComponentEnabledSetting(component);
        if ( disable ) {
             pmg.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                        PackageManager.DONT_KILL_APP);
        }else{
            pmg.setComponentEnabledSetting(component, PackageManager.COMPONENT_ENABLED_STATE_DEFAULT,
                    PackageManager.DONT_KILL_APP);
        }
    }
    private Runnable update=new Runnable(){

        @Override
        public void run() {
            String ab_update = UpgradeInfo.getString(AB_UPDATE);
            if ( ab_update.equalsIgnoreCase("true") ) {
                download = true;
                String url=UpgradeInfo.getString(URL_UPDATE);
                if (url.equals("unknown")) {
                    url = ABCheckUpService.this.getResources().getString(R.string.otaupdateurl);
                }
                String params = createParams();
                Log.d(TAG, "url:"+url+"  params="+params);
                String httpResult = sendPost(url,params);
                Log.d(TAG, "value:"+httpResult);
                String[] urlparams=httpResult.split(";");
                if ( urlparams.length != 2 ) {
                    return;
                }
                String[] headers=urlparams[1].split("&");
                UpdateEngine engine = new UpdateEngine(ABCheckUpService.this);
                UpdateEngineCallback callback = new UpdateEngineCallback(ABCheckUpService.this);
                engine.bind(callback.asBinder());
                Log.d(TAG, "url:"+urlparams[0]+"  params="+headers.length);
                engine.cancel();
                engine.applyPayload(urlparams[0],0L,0L,headers);
            }
        }
    };

    @Override
    public void onDestroy() {
        Log.d(TAG,"onDestroy..............");
        unregisterReceiver(mReceiver);
        if ( mRCThread != null ) {
            mRCThread.quitSafely();
            mRCThread = null;
        }
    }

    class ConnectReceiver extends BroadcastReceiver {
        @Override
        public void onReceive ( Context context, Intent intent ) {
            if ( ( ConnectivityManager.CONNECTIVITY_ACTION ).equals ( intent.getAction() ) ) {
                Log.d(TAG, "CONNECTIVITY_CHANGED, Context = " + context.getClass().getName());
                Bundle bundle = intent.getExtras();
                NetworkInfo netInfo = ( NetworkInfo )bundle.getParcelable( WifiManager.EXTRA_NETWORK_INFO);
                if ( mPref.getBooleanVal ( "Boot_Checked", false ) &&
                        ( netInfo != null ) &&
                        ( netInfo.getDetailedState() == NetworkInfo.DetailedState.CONNECTED ) ) {
                    mPref.setBoolean ( "Boot_Checked", false );
                    mRCHandler.post(update);
                }
                context.sendBroadcast(new Intent(BROADCAST_ACTION));
            }
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        String ab_update = UpgradeInfo.getString(AB_UPDATE);
        if ( ab_update.equalsIgnoreCase("true") ) {
           disableOTA(true);
        } else {
           disableOTA(false);
        }
        return START_NOT_STICKY;
    }

}
