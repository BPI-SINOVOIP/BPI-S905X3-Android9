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
import java.util.Iterator;
import java.util.List;
import java.util.Map;

import org.amlogic.upnp.AmlogicCP;
import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.R;
import com.droidlogic.mediacenter.dlna.DmpService.DmpBinder;

import android.app.Activity;
import android.app.FragmentTransaction;
import android.app.ListFragment;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.os.Message;
import android.os.StrictMode;
import android.view.Gravity;
import android.view.View;
import android.view.animation.Animation;
import android.view.animation.AnimationUtils;
import android.widget.ImageButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.Toast;

/**
 * @ClassName DmpFragment
 * @Description TODO
 * @Date 2013-8-26
 * @Email
 * @Author
 * @Version V1.0
 */
public class DmpFragment extends ListFragment {
        private static final String       TAG                = "DmpFragment";
        private static final int          UPDATE_VIEW        = 0;
        private static final int          DIPLAY_DEV         = 1;
        private static final int          SEARCH_DEV         = 2;
        private static final int          SHOW_LOADING = 5;
        private static final int          HIDE_LOADING = 6;
        private static final int          UPDATE_DATA = 3;
        private static final int          DEVICE_VIEW = 4;
        private static final int          SEARCH_TIMEOUT     = 30000;
        private List<Map<String, Object>> mDevList;
        private FreshListener             mFreshListener;
        private HandlerThread RemoteHandler;
        private UPNPAdapter adapter;
        private ImageButton mFreshBtn ;
        private DevReceiver mDevReceiver = new DevReceiver();
        private Handler mRemoteHandler;
        private LoadingDialog progressDialog;
        private int mSearchTime;
        private boolean mFront = false;
        /*------------------------------------------------------------------------------------*/
        @Override
        public void onActivityCreated ( Bundle savedInstanceState ) {
            super.onActivityCreated ( savedInstanceState );
        }

        @Override
        public void onAttach ( Activity activity ) {
            super.onAttach ( activity );
            mFreshListener = ( FreshListener ) activity;
            mFreshListener.startDMP();
        }

        @Override
        public void onDetach() {
            super.onDetach();
            mHandler.removeMessages ( HIDE_LOADING );
            mHandler.removeMessages ( SEARCH_DEV );
            mHandler.removeMessages ( DIPLAY_DEV );
            mHandler.removeMessages ( UPDATE_VIEW );
            mHandler.removeMessages ( SHOW_LOADING );
            mFreshListener.stopDMP();
            hideLoading();
        }

        @Override
        public void onListItemClick ( ListView parent, View v, int pos, long id ) {
            Map<String, Object> item = ( Map<String, Object> ) parent.getItemAtPosition ( pos );
            /* item.put("item_sel", R.drawable.item_img_sel);
             adapter.notifyDataSetChanged();*/
            if ( mFreshBtn != null ) {
                mFreshBtn.setVisibility ( View.GONE );
            }
            String filename = ( String ) item.get ( "item_name" );
            DeviceFileBrowser devFragment = DeviceFileBrowser.newInstance ( filename );
            FragmentTransaction transaction = getFragmentManager().beginTransaction();
            //transaction.setCustomAnimations(android.R.anim.fade_in, android.R.anim.fade_out, android.R.anim.fade_in, android.R.anim.fade_out);
            transaction.replace ( R.id.frag_detail, devFragment );
            transaction.addToBackStack ( null );
            transaction.commit();
        }

        class DevReceiver extends BroadcastReceiver {
                public void onReceive ( Context context, Intent intent ) {
                    String action = intent.getAction();
                    if ( action == null )
                    { return; }
                    if ( action.equals ( AmlogicCP.UPNP_DMS_ADDED_ACTION )
                            || action.equals ( AmlogicCP.UPNP_DMS_REMOVED_ACTION ) ) {
                        String devName = intent
                                         .getStringExtra ( AmlogicCP.EXTRA_DEV_NAME );
                        Debug.d ( TAG, "DevReceiver: " + action + ", " + devName );
                        getDevData();
                        //mRemoteHandler.sendEmptyMessageDelayed ( UPDATE_VIEW, 100 );
                    }
                }
        }

        private void showLoading() {
            if ( progressDialog == null ) {
                progressDialog = new LoadingDialog ( getActivity(),
                                                     LoadingDialog.TYPE_LOADING, this.getResources().getString (
                                                             R.string.str_loading ) );
                progressDialog.show();
            }
        }

        private void hideLoading() {
            if ( progressDialog != null ) {
                progressDialog.stopAnim();
                progressDialog.dismiss();
                progressDialog = null;
            }
        }

        public void startSearchView() {
            mDevList.clear();
            adapter.notifyDataSetChanged();
            mFreshListener.startSearch();
            mHandler.sendEmptyMessage ( SHOW_LOADING );
            //mHandler.sendEmptyMessageDelayed(HIDE_LOADING,SEARCH_TIMEOUT);
            Message msg = new Message();
            msg.what = DIPLAY_DEV;
            msg.arg1 = mSearchTime * 2000;
            msg.arg2 = SEARCH_TIMEOUT;
            mHandler.sendMessageDelayed(msg, msg.arg1);
            //mRemoteHandler.sendMessageDelayed ( msg, msg.arg1 );
        }

        private void displayDev ( int delay, int timeout ) {
            //Debug.d ( TAG, "display Device:" + delay + " timeout:" + timeout );
            if ( timeout <= 0 ) {
                mHandler.removeMessages ( DIPLAY_DEV );
                displayViewImmediate ( true );
                return ;
            } else {
                getDevData();
                mSearchTime++;
                Message msg = new Message();
                msg.what = DIPLAY_DEV;
                msg.arg1 = mSearchTime * 2000;
                msg.arg2 = timeout - delay;
                mHandler.sendMessageDelayed(msg, msg.arg1);
                //mRemoteHandler.sendMessageDelayed ( msg, msg.arg1 );
            }
            displayViewImmediate ( false );
        }

        private void displayViewImmediate ( boolean isforce ) {
            getDevData();
            if ( mFront && isforce && ( mDevList == null || mDevList.size() == 0 ) ) {
                mSearchTime = 0;
                hideLoading();
                if ( getActivity() != null ) {
                    Toast toast = Toast.makeText ( getActivity(), getActivity()
                                                   .getResources().getString ( R.string.disply_err ),
                                                   Toast.LENGTH_LONG );
                    toast.setGravity ( Gravity.CENTER, 0, 0 );
                    toast.show();
                }
                return;
            }
        }

        private void ChangeRCIcon ( String obj ) {
            if ( mDevList == null )
            { return; }
            HashMap<String, Object> map;
            boolean changed = false;
            for ( int i = 0; i < mDevList.size(); i++ ) {
                map = ( HashMap<String, Object> ) mDevList.get ( i );
                if ( ( ( String ) map.get ( "item_name" ) ).equals ( obj ) ) {
                    String icon_type = mFreshListener.getDevIcon ( obj );
                    if ( icon_type != null ) {
                        map.put ( "item_type", icon_type );
                        changed = true;
                    }
                }
            }
            if ( changed ) {
                mHandler.sendEmptyMessage ( UPDATE_DATA );
            }
        }
        private Handler mHandler = new Handler() {
            public void handleMessage ( Message msg ) {
                switch ( msg.what ) {
                    case UPDATE_DATA:
                        adapter.notifyDataSetChanged();
                        break;
                    case SEARCH_DEV:
                        if ( mFreshBtn != null ) {
                            Animation animation = AnimationUtils.loadAnimation ( getActivity(), R.anim.refresh_btn );
                            if ( animation != null ) {
                                mFreshBtn.startAnimation ( animation );
                            }
                        }
                        mRemoteHandler.removeMessages ( DIPLAY_DEV );
                        mHandler.removeMessages ( SEARCH_DEV );
                        startSearchView();
                        break;
                    case SHOW_LOADING:
                        showLoading();
                        break;
                    case HIDE_LOADING:
                        hideLoading();
                        break;
                    case DIPLAY_DEV:
                        displayDev(msg.arg1, msg.arg2);
                        break;
                    default:
                        break;
                }
            }
        };
        private List<Map<String, Object>> getDevData() {
            ArrayList<String> deviceList = ( ArrayList<String> ) mFreshListener.getDevList();
            //List<Map<String, Object>> list = new ArrayList<Map<String,Object>>();
            HashMap<String, Object> map;
            if ( mDevList != null ) {
                mDevList.clear();
                adapter.notifyDataSetChanged();
            }
            if ( deviceList != null && deviceList.size() > 0 ) {
                hideLoading();
                try {
                    for ( int i = 0; i < deviceList.size(); i++ ) {
                        map = new HashMap<String, Object>();
                        String path = deviceList.get ( i );
                        map.put ( "item_name", path );
                        map.put ( "item_sel", R.drawable.item_img_unsel );
                        String icon_type = mFreshListener.getDevIcon ( ( String ) map.get ( "item_name" ) );
                        if ( icon_type != null && !icon_type.isEmpty() ) {
                            map.put ( "item_type", icon_type );
                        } else {
                            map.put ( "item_type", R.drawable.cloud );
                        }
                        /*Message msg = mRemoteHandler.obtainMessage();
                        msg.what=DEVICE_VIEW;
                        msg.obj=path;
                        mRemoteHandler.sendMessage(msg);*/
                        mDevList.add ( map );
                        adapter.notifyDataSetChanged();
                    }
                } catch ( Exception e ) {
                    e.printStackTrace();
                    return null;
                }
            } else {
                return null;
            }
            return mDevList;
        }
        public interface FreshListener {
            public void startSearch();
            public List<String> getDevList();
            public String getDevIcon ( String path );
            public void startDMP();
            public void stopDMP();
        }

        @Override
        public void onPause() {
            mFront = false;
            mHandler.removeMessages ( UPDATE_DATA );
            mHandler.removeMessages ( SHOW_LOADING );
            mHandler.removeMessages ( HIDE_LOADING );
            getActivity().unregisterReceiver ( mDevReceiver );
            mFreshBtn.setVisibility ( View.GONE );
            super.onPause();
        }

        @Override
        public void onResume() {
            super.onResume();
            mFront = true;
            mSearchTime = 0;
            mFreshBtn = ( ImageButton ) getActivity().findViewById ( R.id.fresh );
            mFreshBtn.setVisibility ( View.VISIBLE );
            mFreshBtn.setOnClickListener ( new View.OnClickListener() {
                @Override
                public void onClick ( View v ) {
                    mHandler.sendEmptyMessage ( SEARCH_DEV );
                }
            } );
            IntentFilter filter = new IntentFilter();
            filter.addAction ( AmlogicCP.UPNP_DMS_ADDED_ACTION );
            filter.addAction ( AmlogicCP.UPNP_DMS_REMOVED_ACTION );
            getActivity().registerReceiver ( mDevReceiver, filter );
            getDevData();
        }

        @Override
        public void onCreate ( Bundle savedInstanceState ) {
            super.onCreate ( savedInstanceState );
            RemoteHandler = new HandlerThread ( "AsyncWorkHandler" );
            RemoteHandler.start();
            mDevList = new ArrayList<Map<String, Object>>();
            adapter = new UPNPAdapter ( getActivity(), mDevList,
            R.layout.device_item, new String[] {
                "item_type", "item_name", "item_sel"
            }, new int[] {
                R.id.item_type, R.id.item_name, R.id.item_sel
            } );
            this.setListAdapter ( adapter );
            mRemoteHandler = new Handler ( RemoteHandler.getLooper() ) {
                public void handleMessage ( Message msg ) {
                    switch ( msg.what ) {
                        case DEVICE_VIEW:
                            ChangeRCIcon ( ( String ) msg.obj );
                            break;
                        case DIPLAY_DEV:
                            displayDev ( msg.arg1, msg.arg2 );
                            break;
                        case UPDATE_VIEW:
                            displayViewImmediate ( false );
                            break;
                    }
                }
            };
            mHandler.sendEmptyMessage ( SHOW_LOADING );
            mHandler.sendEmptyMessageDelayed ( SEARCH_DEV, 1000 );
        }

}
