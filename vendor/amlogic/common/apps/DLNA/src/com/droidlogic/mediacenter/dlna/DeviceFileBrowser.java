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
import java.util.Collection;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.amlogic.upnp.AmlogicCP;
import org.amlogic.upnp.DesUtils;
import org.cybergarage.util.Debug;

import com.droidlogic.mediacenter.MediaCenterActivity;
import com.droidlogic.mediacenter.R;
import com.droidlogic.mediacenter.MediaCenterActivity.Callbacks;

import android.app.Activity;
import android.app.Fragment;
import android.app.FragmentManager;
import android.app.FragmentTransaction;
import android.app.ListFragment;
import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.view.KeyEvent;
import android.view.View;
import android.widget.ListView;
import android.widget.Toast;

/**
 * @ClassName DeviceFileBrowser
 * @Description TODO
 * @Date 2013-9-2
 * @Email
 * @Author
 * @Version V1.0
 */
public class DeviceFileBrowser extends ListFragment implements Callbacks {
        private String                          mediaServerName;
        private static final boolean DEBUG = false;
        private static final String             LABLE                = "media_name";
        private static final String             TAG                  = "DeviceBrowser";
        public static final String              CURENT_POS = "play_curpos";
        public static final String              AMLOGIC_UPNP_NAME    = "amlogic_upnp_name";
        public static final String              IMAGE_URI_ACTION     = "com.amlogic.IMAGE_URI_ACTION";
        public static final String              MEDIA_SERVER_ROOT_ID = "0";
        public static final String              TYPE_VIDEO           = "video/*";
        public static final String              TYPE_AUDIO           = "audio/*";
        public static final String              TYPE_IMAGE           = "image/*";
        public static final String              TYPE_APK             = "application/vnd.android.package-archive";
        public static final String              TYPE_APP             = "application/*";
        public static final String              DEV_TYPE             = "DEV_TYPE";
        public static final String              TYPE_DMP             = "DMP";
        private String                          curItemId            = MEDIA_SERVER_ROOT_ID;
        private boolean                         if_thread_run        = true;
        public static List<Map<String, Object>> playList             = new ArrayList<Map<String, Object>>();
        public int                       play_index           = 0;                                         ;
        private List<Map<String, Object>>       filelist             = null;
        private Tree<Map<String, Object>>       mediaServerTree      = new Tree<Map<String, Object>> ( new HashMap<String, Object>() );
        private Tree<Map<String, Object>>       curTree              = null;
        private List<Map<String, Object>>       pathList             = new ArrayList<Map<String, Object>>();
        private int                             pathLevel            = -1;
        public static final int                 REQUEST_CODE         = 100;
        public static final int                 RESULT_OK            = 101;
        private NetDeviceBrowserThread          mDeviceBrowserTask   = null;
        private static final int                LOADINGSHOW          = 1;
        private static final int                DISPLAYDATA          = 2;
        private static final int                LOADERR              = 3;
        private static final int                LOADINGHIDE          = 4;
        private HandlerThread RemoteHandler;
        private static DeviceFileBrowser deviceInstance ;
        private Handler mHandler;
        /**************************************************************************************************************************/
        static DeviceFileBrowser newInstance ( String label ) {
            //if(deviceInstance != null && deviceInstance.mediaServerName == label){
            //    return deviceInstance;
            //}else{
            deviceInstance = new DeviceFileBrowser();
            Bundle b = new Bundle();
            b.putString ( LABLE, label );
            deviceInstance.setArguments ( b );
            //}
            return deviceInstance;
        }

        @Override
        public void onListItemClick ( ListView parent, View v, int pos, long id ) {
            Map<String, Object> item = ( Map<String, Object> ) parent
                                       .getItemAtPosition ( pos );
            if ( item.get ( "item_type" ).equals ( R.drawable.item_type_back ) ) {
                up2top();
                return;
            }
            curItemId = ( String ) item.get ( "item_id" );
            String item_type_desc = ( String ) item.get ( "item_type_desc" );
            if ( item_type_desc == null )
            { return; }
            if ( item_type_desc.equals ( "container" ) ) {
                curTree = mediaServerTree.getTree ( item );
                String name = ( String ) item.get ( "item_name" );
                addPathList ( name, item );
                displayView();
            } else if ( item_type_desc.equals ( "item" ) ) {
                String uri = ( String ) item.get ( "item_uri" );
                String pre_uri = ( String ) item.get ( "preview_uri" );
                String name = ( String ) item.get ( "item_name" );
                String type = ( String ) item.get ( "item_media_type" );
                Debug.d ( TAG, "URI: " + uri );
                Debug.d ( TAG, "preview URI: " + pre_uri );
                play_index = pos - 1;
                openNetFile ( uri, type, name ,play_index);
            }
        }

        @Override
        public void onViewCreated ( View view, Bundle savedInstanceState ) {
            super.onViewCreated ( view, savedInstanceState );
        }

        private void addPathList ( String name, Map<String, Object> item ) {
            Map<String, Object> map = new HashMap<String, Object>();
            map.put ( "item_name", name );
            map.put ( "item_map", item );
            pathLevel++;
            map.put ( "item_level", pathLevel );
            pathList.add ( pathLevel, map );
        }

        private void removePathList ( int level ) {
            while ( ( pathLevel > level ) && ( pathLevel > 0 ) ) {
                pathList.remove ( pathLevel );
                pathLevel--;
            }
        }

        private void displayView() {
            this.setListAdapter ( getFileAdapter ( 0 ) );
        }

        @Override
        public void onCreate ( Bundle savedInstanceState ) {
            super.onCreate ( savedInstanceState );
            Bundle args = getArguments();
            if ( args != null ) {
                mediaServerName = args.getString ( LABLE );
            }
            RemoteHandler = new HandlerThread ( "AsyncThreadHandler" );
            RemoteHandler.start();
            mHandler = new Handler ( RemoteHandler.getLooper() ) {
                @Override
                public void handleMessage ( Message msg ) {
                    super.handleMessage ( msg );
                }
            };
            mDeviceBrowserTask = new NetDeviceBrowserThread ( "searchFile" );
            curTree = mediaServerTree;
            addPathList ( mediaServerName, mediaServerTree.getHead() );
            displayView();
        }

        private UPNPAdapter getFileAdapter ( int type ) {
            if ( type == 0 ) {
                return new UPNPAdapter ( getActivity(), getListData ( false ),
                R.layout.device_item, new String[] {
                    "item_type", "item_name", "item_sel"
                }, new int[] {
                    R.id.item_type, R.id.item_name, R.id.item_sel,
                } );
            } else {
                if ( filelist == null ) {
                    filelist = new ArrayList<Map<String, Object>>();
                    handlerUI.sendEmptyMessageDelayed ( LOADERR, 500 );
                }
                return new UPNPAdapter ( getActivity(), filelist,
                        R.layout.dmp_item, new String[] {"item_type", "item_name", "item_sel"
                }, new int[] {
                    R.id.item_type, R.id.item_name, R.id.item_sel,
                } );
            }
        }

        private List<Map<String, Object>> getListData ( boolean force ) {
            filelist = null;/*
            if (mDeviceBrowserTask != null) {
                mDeviceBrowserTask = null;
            }*/
            // if(mHandler.hasCallbacks(mDeviceBrowserTask)){
            mHandler.removeCallbacks ( mDeviceBrowserTask );
            // }
            mDeviceBrowserTask = new NetDeviceBrowserThread ( "searchFile" );
            mHandler.post ( mDeviceBrowserTask );
            return new ArrayList<Map<String, Object>>(); // getNetFileData(path,
            // item_id, type);
        }

        private List<Map<String, Object>> getNetFileData ( String item_id ) {
            List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
            Debug.d ( TAG, "getNetFileData: item_id[" + item_id + "]" );
            Collection<Map<String, Object>> children = null;
            children = curTree.getSuccessors ( curTree.getHead() );
            if ( children != null && children.size() > 0 ) {
                Debug.d ( TAG, "GET list from Cache" );
                Object[] obj = children.toArray();
                for ( int i = 0; i < obj.length; i++ ) {
                    Map<String, Object> tmp = ( Map<String, Object> ) obj[i];
                    list.add ( tmp );
                }
            } else if ( mediaServerName != null ) {
                Debug.d ( TAG, "GET list by UPnP" );
                String didl_str;
                Map<String, Object> map = new HashMap<String, Object>();
                map.put ( "item_name", getText ( R.string.str_back ) );
                map.put ( "file_path", ".." );
                map.put ( "item_sel", R.drawable.item_img_unsel );
                map.put ( "item_type", R.drawable.item_type_back );
                list.add ( map );
                didl_str = ( ( MediaCenterActivity ) getActivity() ).actionBrowse (
                               mediaServerName, item_id, "BrowseDirectChildren" );
                List<Map<String, Object>> list_returned;
                if ( getActivity() == null )
                { return list; }
                list_returned = ( ( MediaCenterActivity ) getActivity() )
                                .getBrowseResult ( didl_str, list, R.drawable.item_type_dir,
                                                   R.drawable.item_img_unsel );
                if ( list_returned != null ) {
                    for ( int i = 0; i < list_returned.size(); i++ ) {
                        Map<String, Object> item = list_returned.get ( i );
                        String item_type_desc = ( String ) item.get ( "item_type_desc" );
                        if ( item_type_desc != null && item_type_desc.equals ( "item" ) ) {
                            String upnp_class = ( String ) item.get ( "item_upnp_class" );
                            String type = TYPE_APP;
                            if ( upnp_class != null ) {
                                if ( DEBUG ) { Debug.d ( TAG, "upnp_class: " + upnp_class ); }
                                if ( upnp_class.startsWith ( AmlogicCP.UPNP_CLASS_IMAGE ) ) {
                                    type = TYPE_IMAGE;
                                } else if ( upnp_class.startsWith ( AmlogicCP.UPNP_CLASS_AUDIO ) ) {
                                    type = TYPE_AUDIO;
                                } else if ( upnp_class.startsWith ( AmlogicCP.UPNP_CLASS_VIDEO ) ) {
                                    type = TYPE_VIDEO;
                                } else {
                                    type = TYPE_APP;
                                }
                            }
                            if ( TYPE_APP.equals ( type ) ) {
                                type = DesUtils.CheckMediaType ( ( String ) item
                                                                 .get ( "item_uri" ) );
                            }
                            item.put ( "item_media_type", type );

                            String pre_uri = (String) item.get("preview_uri");
                            if ( type == TYPE_IMAGE ) {
                                item.put("item_type", (null!=pre_uri &&(!pre_uri.isEmpty()))?pre_uri:getFileTypeImg(type));
                            } else {
                                item.put("item_type", getFileTypeImg(type));
                            }
                        }
                        mediaServerTree.addLeaf ( curTree.getHead(), item );
                        if ( DEBUG ) { Debug.d ( TAG, "item[" + i + "] got: " + item ); }
                    }
                } else {
                    Debug.d ( TAG, "actionBrowse fail" );
                    handlerUI.sendEmptyMessage ( LOADERR );
                    return list;
                }
            }
            return list;
        }

        private int getFileTypeImg ( String name ) {
            // int resid = type.equals(NetBrowserOp.LIST) ?
            // R.drawable.item_type_file : R.drawable.item_preview_file;
            int resid = R.drawable.item_type_dir;
            if ( name == null ) {
                return resid;
            }
            if ( name.equals ( TYPE_VIDEO ) ) {
                resid = R.drawable.video;
            } else if ( name.equals ( TYPE_AUDIO ) ) {
                resid = R.drawable.music;
            } else if ( name.equals ( TYPE_IMAGE ) ) {
                resid = R.drawable.photo;
            }
            return resid;
        }

        class NetDeviceBrowserThread implements Runnable {
            private String mThreadName;
            public NetDeviceBrowserThread ( String threadName ) {
                mThreadName = threadName;
            }

            public void run() {
                if ( if_thread_run == true ) {
                    handlerUI.removeMessages ( DISPLAYDATA );
                    handlerUI.sendEmptyMessageDelayed ( LOADINGSHOW, 500 );
                    if ( curItemId == null )
                    { curItemId = MEDIA_SERVER_ROOT_ID; }
                    try{
                        filelist = getNetFileData ( curItemId );
                        Map<String, Object> map = new HashMap<String, Object>();
                        map.put ( "item_name", getText ( R.string.str_back ) );
                        map.put ( "file_path", ".." );
                        map.put ( "item_sel", R.drawable.item_img_unsel );
                        map.put ( "item_type", R.drawable.item_type_back );
                        if ( !filelist.contains ( map ) ) {
                            Debug.d ( TAG, "filelist not contains back" );
                            if ( filelist.get ( 0 ) != null ) {
                                filelist.add ( filelist.get ( 0 ) );
                                filelist.set ( 0, map );
                            }
                        }
                        if ( handlerUI != null ) {
                            handlerUI.removeMessages ( LOADINGSHOW );
                            handlerUI.sendEmptyMessage ( DISPLAYDATA );
                            handlerUI.sendEmptyMessage ( LOADINGHIDE );
                        }
                    }catch(Exception ex){
                    }
                    if ( handlerUI != null ) {
                        handlerUI.removeMessages(LOADINGSHOW);
                    }
                }
            }
        }

    private LoadingDialog progressDialog = null;

    private void showLoading() {
        if (progressDialog == null) {
            progressDialog = new LoadingDialog(getActivity(),
                    LoadingDialog.TYPE_LOADING, this.getResources().getString(
                            R.string.str_loading));
            progressDialog.show();
        }
    }

    private void hideLoading() {
        if (progressDialog != null) {
            progressDialog.stopAnim();
            progressDialog.dismiss();
            progressDialog = null;
        }
    }

    private void displayData() {
        UPNPAdapter adapter = getFileAdapter(1);
        if (adapter.getCount() <= 1) {
            Toast.makeText(getActivity(),
                    getResources().getString(R.string.no_files),
                    Toast.LENGTH_SHORT);
        }
        setListAdapter(adapter);
    }

    private Handler handlerUI = new Handler() {
                                  @Override
                                  public void handleMessage(Message msg) {
                                      switch (msg.what) {
                                          case LOADINGSHOW:
                                              showLoading();
                                              break;
                                          case DISPLAYDATA:
                                              displayData();
                                              break;
                                          case LOADERR:
                                              hideLoading();
                                              Toast.makeText(
                                                      getActivity(),
                                                      getResources()
                                                              .getString(
                                                                      R.string.browser_fail),
                                                      Toast.LENGTH_SHORT);
                                              break;
                                          case LOADINGHIDE:
                                              hideLoading();
                                          default:
                                              break;
                                      }
                                  }
                              };
    @Override
    public void onDestroyView() {
        if_thread_run = false;
        if (handlerUI != null) {
            handlerUI.removeMessages(LOADINGSHOW);
        }
        super.onDestroyView();
    }
    private void up2top() {
        if (curTree == null || curItemId == null
                || curItemId == MEDIA_SERVER_ROOT_ID) {
            if_thread_run = false;
            Debug.d(TAG, "back to device list");
            FragmentManager fm = getFragmentManager();
            if (fm.getBackStackEntryCount() > 0) {
                fm.popBackStack();
            }
            return;
        }
        curTree = curTree.getParent();
        if (curTree == null) {
            curItemId = MEDIA_SERVER_ROOT_ID;
        } else {
            Map<String, Object> tmp = curTree.getHead();
            curItemId = (String) tmp.get("item_id");
        }
        removePathList(pathLevel - 1);
        displayView();
    }

    private void getPlayList(String type) {
        playList.clear();
        for (int i = 0; i < filelist.size(); i++) {
            Map<String, Object> item = filelist.get(i);
            String mediaType = (String) item.get("item_media_type");
            if (type.equals(mediaType)) {
                playList.add(item);
            }
        }
    }

    protected void openNetFile(String uri, String type, String upnp_name, int playid) {
        if (type == null) {
            type = DesUtils.CheckMediaType(uri);
        }
        if (type.equals(TYPE_IMAGE)) {
            getPlayList(TYPE_IMAGE);
            Intent intent = new Intent(IMAGE_URI_ACTION);
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_URI, uri);
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_TYPE, TYPE_IMAGE);
            intent.setClass(getActivity(), ImageFromUrl.class);
            intent.putExtra(DEV_TYPE, TYPE_DMP);
            intent.putExtra(CURENT_POS, play_index);
            //startActivity(intent);
            startActivityForResult(intent,play_index);
        }
        else if (type.equals(TYPE_VIDEO)) {
            getPlayList(TYPE_VIDEO);
            Intent intent = new Intent();
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_URI, uri);
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_TYPE, TYPE_VIDEO);
            intent.putExtra(DEV_TYPE, TYPE_DMP);
            intent.putExtra(CURENT_POS, play_index);
            intent.setClass(getActivity(), VideoPlayer.class);
            //startActivity(intent);
            startActivityForResult(intent, play_index);
        } else if (type.equals(TYPE_AUDIO)) {
            getPlayList(TYPE_AUDIO);
            Intent intent = new Intent();
            intent.setClass(getActivity(), MusicPlayer.class);
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_URI, uri);
            intent.putExtra(AmlogicCP.EXTRA_MEDIA_TYPE, TYPE_AUDIO);
            intent.putExtra(AmlogicCP.EXTRA_FILE_NAME, upnp_name);
            intent.putExtra(DEV_TYPE, TYPE_DMP);
            intent.putExtra(CURENT_POS, play_index);
            //startActivity(intent);
            startActivityForResult(intent,play_index);
        } else {
            Intent intent = new Intent();
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.setAction(android.content.Intent.ACTION_VIEW);
            intent.setDataAndType(Uri.parse(uri), type);
            intent.putExtra(AMLOGIC_UPNP_NAME, upnp_name);
            startActivity(intent);
        }
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (resultCode > 0) {
            android.util.Log.d(TAG,"onActivityResult:"+resultCode);
            setSelection(resultCode+1);
        }
    }

    @Override
    public void onResume() {
        super.onResume();
        android.util.Log.d(TAG,"onResume:");
    }

    /* (non-Javadoc)
     * @see com.droidlogic.mediacenter.MediaCenterActivity.Callbacks#onBackPressedCallback()
     */
    @Override
    public void onBackPressedCallback() {
        up2top();
    }

}
