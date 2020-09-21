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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.amlogic.upnp.AmlogicCP;
import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.R;

import android.app.ActivityManager;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Binder;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;

/**
 * @ClassName DmpService
 * @Description TODO
 * @Date 2013-8-30
 * @Email
 * @Author
 * @Version V1.0
 */
public class DmpService extends Service {
        public static final String ITEM_NAME = "item_name";
        public static final String ITEM_ICON = "item_sel";
        public static final String ITEM_TYPE = "item_type";
        private boolean mStartSearch = false;
        private static final int START_DMP = 201;
        private static final int SEARCH_DMP = 202;
        private static final int STOP_DMP = 203;
        private static final int DMP_PIC = 204;
        private static final int ACTION_DEVICE = 205;
        private static final int ACTION_BROWSER = 206;
        private static final String TAG = "DLNA";
        private AmlogicCP mControlPoint;
        private HandlerThread mDmpThread;
        private DmpBinder mDmpBinder;
        private ArrayList<String> mDevList = null;
        private DmpHandler mHandler;
        private List<String> mDevices;
        private HashMap<String, String> mIconsCache;
        public static final String NETWORK_ERROR = "com.android.mediacenter.networkerr";
        List<Map<String, Object>> mBrowserList = new ArrayList<Map<String, Object>>();
        /* (non-Javadoc)
         * @see android.app.Service#onBind(android.content.Intent)
         */
        @Override
        public IBinder onBind ( Intent arg0 ) {
            mIconsCache = new HashMap<String, String>();
            IntentFilter filter = new IntentFilter();
            initDMP();
            filter.addAction ( NETWORK_ERROR );
            registerReceiver ( mDMPServiceListener, filter );
            return mDmpBinder;
        }
        private BroadcastReceiver mDMPServiceListener = new BroadcastReceiver() {
            @Override
            public void onReceive ( Context cxt, Intent intent ) {
                if ( intent.getAction() == NETWORK_ERROR ) {
                    startError();
                }
            }
        };
        private void startError() {
            ActivityManager am = ( ActivityManager ) getSystemService ( ACTIVITY_SERVICE );
            ComponentName cn = am.getRunningTasks ( 1 ).get ( 0 ).topActivity;
            String packageName = cn.getPackageName();
            if ( this.getClass().getPackage().getName().contains(packageName) ) {
                Intent mIntent = new Intent();
                mIntent.addFlags ( Intent.FLAG_ACTIVITY_NEW_TASK );
                mIntent.setClass ( DmpService.this, DMRError.class );
                startActivity ( mIntent );
            }
        }
        public class DmpBinder extends Binder {

                public DmpService getServiceInstance() {
                    return DmpService.this;
                }


        }

        public List<String> getDevList() {
            return mControlPoint.getMediaServerNameList();
        }

        public String getDeviceIcon ( String path ) {
            //Debug.d ( TAG, "getDeviceIcon()" );
            return mControlPoint.getDeviceIcon ( path );
        }

        public void restartDmp() {
            mHandler.sendEmptyMessage ( START_DMP );
        }
        public void forceStop() {
            mHandler.sendEmptyMessage ( STOP_DMP );
        }
        public void startSearch() {
            mHandler.sendEmptyMessage ( SEARCH_DMP );
            mDevices = null;
        }
        public List<Map<String, Object>> getBrowseResult ( String didl_str, List<Map<String, Object>> list, int itemTypeDir, int itemImgUnsel ) {
            return mControlPoint.getBrowseResult ( didl_str, list, itemTypeDir, itemImgUnsel );
        }
        public String actionBrowse ( String mediaServerName, String item_id,
        String flag ) {
            return mControlPoint.actionBrowse ( mediaServerName, item_id, flag );
        }


        private void initDMP() {
            if ( mControlPoint == null ) {
                mControlPoint = new AmlogicCP ( this );
                mControlPoint.setNMPRMode ( true );
                mDmpThread = new HandlerThread ( "dmpDemoThread" );
                mDmpThread.start();
            }
            mHandler = new DmpHandler ( mDmpThread.getLooper() );
            //mHandler.sendEmptyMessage ( START_DMP );
        }
        @Override
        public void onDestroy() {
            super.onDestroy();
            mHandler.sendEmptyMessage ( STOP_DMP );
            FileUtil.delCacheFile();
        }
        class DmpHandler extends Handler {
                public DmpHandler() {
                }
                public DmpHandler ( Looper looper ) {
                    super ( looper );
                }
                public void handleMessage ( Message msg ) {
                    switch ( msg.what ) {
                        case START_DMP:
                            mControlPoint.start();
                            break;
                        case SEARCH_DMP:
                            mControlPoint.search();
                            break;
                        case STOP_DMP:
                            mControlPoint.stop();
                            break;
                        case DMP_PIC:
                            String ret = mControlPoint.getDeviceIcon ( ( String ) msg.obj );
                            mIconsCache.put ( ( String ) msg.obj, ret );
                            break;
                        case ACTION_DEVICE:
                            mDevices = mControlPoint.getMediaServerNameList();
                            Debug.d ( TAG, "remote getmDevices notify " + ( mDevices == null ) );
                            break;
                        case ACTION_BROWSER:
                            Bundle b = msg.getData();
                            String serviceName = b.getString ( "service_name" );
                            String itemId = b.getString ( "item_id" );
                            String retVal = mControlPoint.actionBrowse ( serviceName, itemId, b.getString ( "flag" ) );
                            mControlPoint.getBrowseResult ( retVal, mBrowserList,  R.drawable.item_type_dir, R.drawable.item_img_unsel );
                            break;
                    }
                }
        }

        @Override
        public void onCreate() {
            super.onCreate();
            //initDMP();
            mDmpBinder = new DmpBinder();
            mDevList = new ArrayList<String>();
        }
        @Override
        public boolean onUnbind ( Intent intent ) {
            unregisterReceiver ( mDMPServiceListener );
            return super.onUnbind ( intent );
        }
}
