/******************************************************************
*
*Copyright (C) 2012  Amlogic, Inc.
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
package com.droidlogic.FileBrower;

import android.os.storage.*;
import java.io.File;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Iterator;
import java.text.SimpleDateFormat;
import java.util.Date;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.DialogInterface;
import android.content.DialogInterface.OnDismissListener;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.content.pm.PackageManager.NameNotFoundException;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.Display;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.View;
import android.view.View.OnClickListener;
import android.view.ViewGroup;
import android.view.KeyEvent;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemClickListener;
import android.widget.BaseAdapter;
import android.widget.Button;
import android.widget.GridView;
import android.widget.EditText;
import android.widget.ListView;
import android.widget.ProgressBar;
import android.widget.SimpleAdapter;
import android.widget.TextView;
import android.widget.Toast;
import android.widget.ToggleButton;
import android.os.PowerManager;
import android.content.res.Configuration;
import android.os.Environment;
import android.os.storage.StorageVolume;

import com.droidlogic.FileBrower.FileBrowerDatabase.FileMarkCursor;
import com.droidlogic.FileBrower.FileOp.FileOpReturn;
import com.droidlogic.FileBrower.FileOp.FileOpTodo;
import com.droidlogic.app.FileListManager;

import android.bluetooth.BluetoothAdapter;

    /** Called when the activity is first created. */
public class ThumbnailView1 extends Activity{
    public static final String TAG = "ThumbnailView";

    private List<Map<String, Object>> mList;
    private boolean mListLoaded = false;
    private static final int LOAD_DIALOG_ID = 4;
    private ProgressDialog load_dialog;
    private boolean mLoadCancel = false;

    private boolean mMediaScannerRunning;
    private PowerManager.WakeLock mWakeLock;
    private static final String SD_PATH_EQUAL = "/storage/sdcard1";

    public static final String KEY_NAME = "key_name";
    public static final String KEY_PATH = "key_path";
    public static final String KEY_TYPE = "key_type";
    public static final String KEY_DATE = "key_date";
    public static final String KEY_SIZE = "key_size";
    public static final String KEY_SELE = "key_sele";
    public static final String KEY_RDWR = "key_rdwr";

    public static String cur_path = FileListManager.STORAGE;
    protected static final int SORT_DIALOG_ID = 0;
    protected static final int EDIT_DIALOG_ID = 1;
    private static final int HELP_DIALOG_ID = 3;
    protected static  String cur_sort_type = null;
    private AlertDialog sort_dialog;
    private AlertDialog edit_dialog;
    private AlertDialog help_dialog;
    private ListView sort_lv;
    private ListView edit_lv;
    private ListView help_lv;
    public static Handler mProgressHandler;
    public static FileBrowerDatabase db;
    public static FileMarkCursor myCursor;
    private boolean local_mode;
    GridView ThumbnailView;
    int request_code = 1550;
    private ToggleButton btn_mode;
    private String lv_sort_flag = "by_name";
    private boolean isInFileBrowserView=false;
    private FileListManager mFileListManager;

    Comparator  mFileComparator = new Comparator<File>(){
        @Override
        public int compare(File o1, File o2) {
            if (o1.isDirectory() && o2.isFile())
                return -1;
            if (o1.isFile() && o2.isDirectory())
                return 1;
            return o1.getName().compareTo(o2.getName());
        }
    };

    private void updateThumbnials() {
        // sendBroadcast(new Intent(Intent.ACTION_MEDIA_MOUNTED, Uri.parse("file://"
        //         + Environment.getExternalStorageDirectory())));
        // Log.i("scan...", ": " + Uri.parse("file://"
        //         + Environment.getExternalStorageDirectory()));
    }

    private List<Map<String, Object>> getDeviceListData() {
        List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        list = mFileListManager.getDevices();
        int fileCnt = list.size();
        for (int i = 0; i < fileCnt; i++) {
            Map<String, Object> fMap = list.get(i);
            String sType = (String)fMap.get(KEY_TYPE);
            if (sType.equals("type_nand")) {
                fMap.put(KEY_TYPE, R.drawable.sdcard_default);
            } else if (sType.equals("type_udisk")) {
                fMap.put(KEY_TYPE, R.drawable.usb_default);
            } else {
                fMap.put(KEY_TYPE, R.drawable.sdcard_default);
            }
        }
        FileOp.updatePathShow(this, mFileListManager, FileListManager.STORAGE, true);
        if (!mMediaScannerRunning)
            ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), path);
        if (!list.isEmpty()) {
            Collections.sort(list, new Comparator<Map<String, Object>>() {
                public int compare(Map<String, Object> object1, Map<String, Object> object2) {
                    return ((Integer) object1.get(KEY_SIZE)).compareTo((Integer) object2.get(KEY_SIZE));
                }
            });
        }
        return list;
    }

    private String getThumbnail(String file_path) {
        return file_path;
    }

    private List<Map<String, Object>> getFileListData(String path) {
        List<Map<String, Object>> list = mFileListManager.getFiles(path);
        int fileCnt = list.size();
        String tmpPath = null;
        for (int i = 0; i < fileCnt; i++) {
            Map<String, Object> fMap = list.get(i);
            tmpPath = (String)fMap.get(KEY_PATH);
            if (tmpPath !=null) {
                File file = new File(tmpPath);
                String temp_name = FileOp.getShortName(file.getAbsolutePath());
                fMap.put(KEY_NAME, temp_name);
                if (FileOp.isFileSelected(tmpPath,"thumbnail1")) {
                    fMap.put(KEY_SELE, R.drawable.item_img_sel);
                } else {
                    fMap.put(KEY_SELE, R.drawable.item_img_unsel);
                }
                if (file.isDirectory()) {
                    fMap.put(KEY_TYPE, R.drawable.item_preview_dir);
                } else {
                    fMap.put(KEY_TYPE, FileOp.getThumbImage(file.getName()));
                    if (mFileListManager.isPhoto(file.getName())) {
                        String thumbnail_path = getThumbnail(tmpPath);
                        if (thumbnail_path != null) {
                            if (new File(thumbnail_path).exists()) {
                                fMap.put(KEY_TYPE, thumbnail_path);
                            } else {
                                fMap.put(KEY_TYPE, R.drawable.item_preview_photo);
                            }
                        }
                    }
                }
                long file_date = file.lastModified();
                fMap.put("file_date", file_date);	//use for sorting
                long file_size = file.length();
                fMap.put("file_size", file_size);       //use for sorting
            }
        }
        return list;
    }

    private List<Map<String, Object>> getFileListDataSorted(String path, String sort_type) {
        FileOp.updatePathShow(this, mFileListManager, path, true);
        if (!mMediaScannerRunning)
            ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), path);

        if (!mListLoaded) {
            mListLoaded = true;
            showDialog(LOAD_DIALOG_ID);

            final String ppath = path;
            final String ssort_type = sort_type;
            new Thread("getFileListDataSortedAsync") {
                @Override
                public void run() {
                    mList = getFileListDataSortedAsync(ppath, ssort_type);
                    mProgressHandler.sendMessage(Message.obtain(mProgressHandler, 10));
                }
            }.start();

            return new ArrayList<Map<String, Object>>();
        }
        else {
            return mList;
        }
    }

    private List<Map<String, Object>> getFileListDataSortedAsync(String path, String sort_type) {
        List<Map<String, Object>> list = mFileListManager.getFiles(path);
        int fileCnt = list.size();
        String tmpPath = null;
        for (int i = 0; i < fileCnt; i++) {
            Map<String, Object> fMap = list.get(i);
            tmpPath = (String)fMap.get(KEY_PATH);
            if (tmpPath !=null) {
                File file = new File(tmpPath);
                String temp_name = FileOp.getShortName(file.getAbsolutePath());
                fMap.put(KEY_NAME, temp_name);
                if (FileOp.isFileSelected(tmpPath,"thumbnail1")) {
                    fMap.put(KEY_SELE, R.drawable.item_img_sel);
                } else {
                    fMap.put(KEY_SELE, R.drawable.item_img_unsel);
                }
                if (file.isDirectory()) {
                    fMap.put(KEY_TYPE, R.drawable.item_preview_dir);
                } else {
                    fMap.put(KEY_TYPE, FileOp.getThumbImage(file.getName()));
                    if (mFileListManager.isPhoto(file.getName())) {
                        String thumbnail_path = getThumbnail(tmpPath);
                        if (thumbnail_path != null) {
                            if (new File(thumbnail_path).exists()) {
                                fMap.put("item_type", thumbnail_path);
                            } else {
                                fMap.put("item_type", R.drawable.item_preview_photo);
                            }
                        }
                    }
                }
                long file_date = file.lastModified();
                fMap.put(KEY_DATE, file_date);	//use for sorting
                long file_size = file.length();
                fMap.put(KEY_SIZE, file_size);       //use for sorting
            }
        }

        /* sorting */
        if (!list.isEmpty()) {
            if (sort_type.equals("by_name")) {
                Collections.sort(list, new Comparator<Map<String, Object>>() {
                    public int compare(Map<String, Object> object1,
                    Map<String, Object> object2) {
                        File file1 = new File((String) object1.get(KEY_PATH));
                        File file2 = new File((String) object2.get(KEY_PATH));
                        if (file1.isFile() && file2.isFile() || file1.isDirectory() && file2.isDirectory()) {
                            return ((String) object1.get(KEY_NAME)).toLowerCase()
                                .compareTo(((String) object2.get(KEY_NAME)).toLowerCase());
                        }
                        else {
                            return file1.isFile() ? 1 : -1;
                        }
                    }
                });
            }
            else if (sort_type.equals("by_date")) {
                Collections.sort(list, new Comparator<Map<String, Object>>() {
                    public int compare(Map<String, Object> object1,
                    Map<String, Object> object2) {
                        return ((Long) object1.get(KEY_DATE)).compareTo((Long) object2.get(KEY_DATE));
                    }
                });
            }
            else if (sort_type.equals("by_size")) {
                Collections.sort(list, new Comparator<Map<String, Object>>() {
                    public int compare(Map<String, Object> object1,
                    Map<String, Object> object2) {
                        return ((Long) object1.get(KEY_SIZE)).compareTo((Long) object2.get(KEY_SIZE));
                    }
                });
            }
        }
        return list;
    }

    private ThumbnailAdapter1 getFileListAdapter(String path) {
        if (path.equals(FileListManager.STORAGE)) {
            return new ThumbnailAdapter1(ThumbnailView1.this,
                getDeviceListData(),
                R.layout.gridview_item,
                new String[]{
                KEY_TYPE,
                KEY_SELE,
                KEY_NAME},
                new int[]{
                R.id.itemImage,
                R.id.itemMark,
                R.id.itemText});
        }
        else {
            return new ThumbnailAdapter1(ThumbnailView1.this,
                getFileListData(path),
                R.layout.gridview_item,
                new String[]{
                KEY_TYPE,
                KEY_SELE,
                KEY_NAME},
                new int[]{
                R.id.itemImage,
                R.id.itemMark,
                R.id.itemText});
        }
    }

    private ThumbnailAdapter1 getFileListAdapterSorted(String path, String sort_type) {
        if (path.equals(FileListManager.STORAGE)) {
            return new ThumbnailAdapter1(ThumbnailView1.this,
                getDeviceListData(),
                R.layout.gridview_item,
                new String[]{
                KEY_TYPE,
                KEY_SELE,
                KEY_NAME},
                new int[]{
                R.id.itemImage,
                R.id.itemMark,
                R.id.itemText});
        }
        else {
            return new ThumbnailAdapter1(ThumbnailView1.this,
                getFileListDataSorted(path, sort_type),
                R.layout.gridview_item,
                new String[]{
                KEY_TYPE,
                KEY_SELE,
                KEY_NAME},
                new int[]{
                R.id.itemImage,
                R.id.itemMark,
                R.id.itemText});
        }
    }

    private final BroadcastReceiver mReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent.getAction().equals(ThumbnailScannerService.ACTION_THUMBNAIL_SCANNER_FINISHED)) {
                if (cur_path != null && !cur_path.equals(FileListManager.STORAGE)) {
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
            }
        }
    };

    private BroadcastReceiver mMountReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Uri uri = intent.getData();
            String path = uri.getPath();

            if (action == null || path == null)
                return;

            if (path.startsWith(SD_PATH_EQUAL)) {
                path = path.replace(SD_PATH_EQUAL, FileListManager.NAND);
            }

            if (action.equals(Intent.ACTION_MEDIA_EJECT)) {
                if (cur_path.startsWith(path)) {
                    cur_path = FileListManager.STORAGE;
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                if (cur_path.equals(FileListManager.STORAGE)) {
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                FileOp.cleanFileMarks("thumbnail1");

                ThumbnailOpUtils.stopThumbnailSanner(getBaseContext());
                if (FileOp.IsBusy) {
                    FileOp.copy_cancel = true;
                }
            }
            else if ((action.equals ("com.droidvold.action.MEDIA_UNMOUNTED")
                || action.equals ("com.droidvold.action.MEDIA_EJECT")) && !path.equals("/dev/null")) {
                if (cur_path.startsWith(path)) {
                    cur_path = FileListManager.STORAGE;
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                if (cur_path.equals(FileListManager.STORAGE)) {
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                FileOp.cleanFileMarks("thumbnail1");
            }
            else if (action.equals(Intent.ACTION_MEDIA_MOUNTED) || action.equals ("com.droidvold.action.MEDIA_MOUNTED")) {
                if (cur_path.equals(FileListManager.STORAGE)) {
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
            }
            else if (action.equals(Intent.ACTION_MEDIA_UNMOUNTED)) {
                if (cur_path.startsWith(path)) {
                    cur_path = FileListManager.STORAGE;
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                if (cur_path.equals(FileListManager.STORAGE)) {
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
                FileOp.cleanFileMarks("thumbnail1");
            }
            else if(action.equals(Intent.ACTION_SCREEN_OFF)) {
                if(sort_dialog != null)
                    sort_dialog.dismiss();
                if(help_dialog != null)
                    help_dialog.dismiss();
            }
        }
    };

    private final BroadcastReceiver mMediaScannerReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                mMediaScannerRunning = true;
            } else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                mMediaScannerRunning = false;
            }
        }
    };

    @Override
    public void onConfigurationChanged(Configuration config) {
        super.onConfigurationChanged(config);
        //ignore orientation change
    }

    /** Called when the activity is first created or resumed. */
    @Override
    public void onResume() {
        super.onResume();
        IntentFilter intentFilter = new IntentFilter(ThumbnailScannerService.ACTION_THUMBNAIL_SCANNER_FINISHED);
        //intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        //intentFilter.addDataScheme("file");
        registerReceiver(mReceiver, intentFilter);
        //StorageManager m_storagemgr = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        //m_storagemgr.registerListener(mListener);

        //install an intent filter to receive SD card related events.
        intentFilter = new IntentFilter(Intent.ACTION_MEDIA_MOUNTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_EJECT);
        intentFilter.addAction(Intent.ACTION_MEDIA_UNMOUNTED);
        intentFilter.addAction(Intent.ACTION_SCREEN_OFF);
        intentFilter.addAction ("com.droidvold.action.MEDIA_UNMOUNTED");
        intentFilter.addAction ("com.droidvold.action.MEDIA_MOUNTED");
        intentFilter.addAction ("com.droidvold.action.MEDIA_EJECT");
        intentFilter.addDataScheme("file");
        registerReceiver(mMountReceiver, intentFilter);

        intentFilter = new IntentFilter(Intent.ACTION_MEDIA_SCANNER_STARTED);
        intentFilter.addAction(Intent.ACTION_MEDIA_SCANNER_FINISHED);
        intentFilter.addDataScheme("file");
        Intent intent = registerReceiver(mMediaScannerReceiver, intentFilter);
        mMediaScannerRunning = false;
        if (intent != null) {
            if (intent.getAction().equals(Intent.ACTION_MEDIA_SCANNER_STARTED))
                mMediaScannerRunning = true;
        }

        if(mListLoaded == true) {
            mListLoaded = false;
        }

        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
        isInFileBrowserView=true;
    }

    @Override
    public void onPause() {
        super.onPause();
        mLoadCancel = true;
        ThumbnailOpUtils.stopThumbnailSanner(getBaseContext());
        unregisterReceiver(mMediaScannerReceiver);
        unregisterReceiver(mReceiver);
        unregisterReceiver(mMountReceiver);
        //StorageManager m_storagemgr = (StorageManager) getSystemService(Context.STORAGE_SERVICE);
        //m_storagemgr.unregisterListener(mListener);
        //update sharedPref
        SharedPreferences settings = getSharedPreferences("settings", Activity.MODE_PRIVATE);
        SharedPreferences.Editor editor = settings.edit();
        editor.putString("cur_path", cur_path);
        editor.putBoolean("isChecked", btn_mode.isChecked());
        editor.commit();

        if (load_dialog != null)
            load_dialog.dismiss();

        if(mListLoaded==true)
            mListLoaded = false;

        if(!local_mode){
            db.deleteAllFileMark();
        }
        db.close();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.thumbnail);
        mFileListManager = new FileListManager(this);
        Bundle bundle = this.getIntent().getExtras();
        if(!bundle.getString("sort_flag").equals("")){
            lv_sort_flag=bundle.getString("sort_flag");
        }
        PowerManager pm = (PowerManager)getSystemService(Context.POWER_SERVICE);
        mWakeLock = pm.newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, TAG);

        ThumbnailView = (GridView)findViewById(R.id.mygridview);
        /*get cur path form listview*/
        SharedPreferences settings = getSharedPreferences("settings", Activity.MODE_PRIVATE);
        cur_path = settings.getString("cur_path", FileListManager.STORAGE);
        if (cur_path != null) {
            File file = new File(cur_path);
            if (!file.exists())
                cur_path = FileListManager.STORAGE;
        }
        else
            cur_path = FileListManager.STORAGE;

        /* setup database */
        FileOp.SetMode(false);
        db = new FileBrowerDatabase(this);
        local_mode = false;

        mList = new ArrayList<Map<String, Object>>();

        if (cur_path == null) cur_path = FileListManager.STORAGE;
        if (cur_path.equals(FileListManager.STORAGE)) {
            //ThumbnailOpUtils.deleteAllThumbnails(getBaseContext(), db);
            ThumbnailOpUtils.cleanThumbnails(getBaseContext());
            //ThumbnailOpUtils.updateThumbnailsForAllDev(getBaseContext());
        }
        else {
            //ThumbnailOpUtils.deleteAllThumbnails(getBaseContext(), db);
            ThumbnailOpUtils.cleanThumbnails(getBaseContext());
            //ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), cur_path);
            //ThumbnailOpUtils.updateThumbnailsForAllDev(getBaseContext());
        }

        /** edit process bar handler
         *  mProgressHandler.sendMessage(Message.obtain(mProgressHandler, msg.what, msg.arg1, msg.arg2));
         */
        mProgressHandler = new Handler() {
            @Override
            public void handleMessage(Message msg) {
                super.handleMessage(msg);

                if(false==isInFileBrowserView)
                    return;

                ProgressBar pb = null;
                TextView tvForPaste=null;
                if (edit_dialog != null) {
                    pb = (ProgressBar) edit_dialog.findViewById(R.id.edit_progress_bar);
                    tvForPaste=(TextView)edit_dialog.findViewById(R.id.text_view_paste);
                }

                switch(msg.what) {
                    case 0: 	//set invisible
                        if ((edit_dialog != null) && (pb != null)&&(tvForPaste!=null)) {
                            pb.setVisibility(View.INVISIBLE);
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 1:		//set progress_bar1
                        if ((edit_dialog != null) && (pb != null)&&(tvForPaste!=null)) {
                            pb.setProgress(msg.arg1);
                        }
                    break;
                    case 2:		//set progress_bar2
                        if ((edit_dialog != null) && (pb != null)) {
                            pb.setSecondaryProgress(msg.arg1);
                        }
                    break;
                    case 3:		//set visible
                        if ((edit_dialog != null) && (pb != null)&&(tvForPaste!=null)) {
                            pb.setProgress(0);
                            pb.setSecondaryProgress(0);
                            pb.setVisibility(View.VISIBLE);

                            tvForPaste.setVisibility(View.VISIBLE);
                            tvForPaste.setText(getText(R.string.edit_dialog_paste_file)+"\n"+FileOp.getMarkFileName("thumbnail1"));
                        }
                    break;
                    case 4:		//file paste ok
                        updateThumbnials();

                        db.deleteAllFileMark();
                        //GetCurrentFilelist(cur_path,cur_sort_type);
                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        scanAll();
                        //if (!mMediaScannerRunning)
                        //ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), cur_path);
                        //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path,cur_sort_type));
                        Toast.makeText(ThumbnailView1.this,
                        getText(R.string.Toast_msg_paste_ok),
                        Toast.LENGTH_SHORT).show();
                        FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();

                        if (tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 5:		//file paste err
                        updateThumbnials();
                        Toast.makeText(ThumbnailView1.this,
                        getText(R.string.Toast_msg_paste_nofile),
                        Toast.LENGTH_SHORT).show();
                        FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();

                        if (tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 6:
                        if (!cur_path.equals(FileListManager.STORAGE))
                            ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                    break;
                    case 7:		//dir cannot write
                        Toast.makeText(ThumbnailView1.this,
                            getText(R.string.Toast_msg_paste_writeable),
                            Toast.LENGTH_SHORT).show();
                        //FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();

                        if (tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 8:		//no free space
                        db.deleteAllFileMark();
                        if (!mMediaScannerRunning)
                            ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), cur_path);
                        Toast.makeText(ThumbnailView1.this,
                            getText(R.string.Toast_msg_paste_nospace),
                            Toast.LENGTH_SHORT).show();
                        FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();

                        if(tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 9:		//file copy cancel
                        if ((FileOp.copying_file != null) && (FileOp.copying_file.exists())) {
                            try {
                                if(FileOp.copying_file.isDirectory())
                                    FileUtils.deleteDirectory(FileOp.copying_file);
                                else
                                    FileOp.copying_file.delete();
                            }
                            catch (Exception e) {
                                Log.e("Exception when delete",e.toString());
                            }
                        }
                        Toast.makeText(ThumbnailView1.this,
                            getText(R.string.Toast_copy_fail),
                            Toast.LENGTH_SHORT).show();
                        FileOp.copy_cancel = false;
                        FileOp.copying_file = null;
                        db.deleteAllFileMark();
                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        if (!mMediaScannerRunning)
                            ThumbnailOpUtils.updateThumbnailsForDir(getBaseContext(), cur_path);
                        FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();
                        scanAll();
                        if(tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                    case 10:    //update list
                        //((BaseAdapter) ThumbnailView.getAdapter()).notifyDataSetChanged();
                        if(mListLoaded == false) {
                            break;
                        }
                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        mListLoaded = false;
                        if (load_dialog != null)
                            load_dialog.dismiss();
                    break;

                    case 11:	//destination dir is sub folder of src dir
                        Toast.makeText(ThumbnailView1.this,
                        getText(R.string.Toast_msg_paste_sub_folder),
                        Toast.LENGTH_SHORT).show();
                        //FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                        if (edit_dialog != null)
                            edit_dialog.dismiss();
                        if (mWakeLock.isHeld())
                            mWakeLock.release();

                        if(tvForPaste!=null) {
                            tvForPaste.setText("");
                            tvForPaste.setVisibility(View.GONE);
                        }
                    break;
                }
            }
        };

        ///ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
        /* btn_mode default checked */
        btn_mode = (ToggleButton) findViewById(R.id.btn_thumbmode);
        btn_mode.setChecked(settings.getBoolean("isChecked", false));

        ThumbnailView.setOnItemClickListener(new OnItemClickListener() {
            public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                Map<String, Object> item = (Map<String, Object>)parent.getItemAtPosition(pos);
                String file_path = (String) item.get(KEY_PATH);
                File file = new File(file_path);
                if(!file.exists()){
                    //finish();
                    return;
                }

                if(Intent.ACTION_GET_CONTENT.equalsIgnoreCase(ThumbnailView1.this.getIntent().getAction())) {
                    if (file.isDirectory()) {
                        cur_path = file_path;
                        //GetCurrentFilelist(cur_path,cur_sort_type);
                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path,cur_sort_type));
                    }
                    else {
                        ThumbnailView1.this.setResult(Activity.RESULT_OK,new Intent(null, Uri.fromFile(file)));
                        ThumbnailView1.this.finish();
                    }
                }
                else {
                    ToggleButton btn_mode = (ToggleButton) findViewById(R.id.btn_thumbmode);
                    if (!btn_mode.isChecked()) {
                        if (file.isDirectory()) {
                            cur_path = file_path;
                            //GetCurrentFilelist(cur_path,cur_sort_type);
                            ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                            //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path,cur_sort_type));
                        }
                        else {
                            openFile(file_path);
                        }
                    }
                    else {
                        if (!cur_path.equals(FileListManager.STORAGE) && (item.get("item_sel") != null)) {
                            if (item.get("item_sel").equals(R.drawable.item_img_unsel)) {
                                FileOp.updateFileStatus(file_path, 1,"thumbnail1");
                                item.put("item_sel", R.drawable.item_img_sel);
                            }
                            else if (item.get("item_sel").equals(R.drawable.item_img_sel)) {
                                FileOp.updateFileStatus(file_path, 0,"thumbnail1");
                                item.put("item_sel", R.drawable.item_img_unsel);
                            }

                            ((BaseAdapter) ThumbnailView.getAdapter()).notifyDataSetChanged();
                        }
                        else {
                            cur_path = file_path;
                            ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                        }
                    }
                }
            }
        });

        //button click listener
        /*home button*/
        Button btn_thumbhome = (Button) findViewById(R.id.btn_thumbhome);
        btn_thumbhome.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                cur_path = FileListManager.STORAGE;
                ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
            }
        });

        /*updir button*/
        Button btn_thumbparent = (Button) findViewById(R.id.btn_thumbparent);
        btn_thumbparent.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (!cur_path.equals(FileListManager.STORAGE)) {
                    File file = new File(cur_path);
                    String parent_path = file.getParent();
                    if (cur_path.equals(FileListManager.NAND) || parent_path.equals(FileListManager.MEDIA_RW))
                        cur_path = FileListManager.STORAGE;
                    else
                        cur_path = parent_path;
                    ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                }
            }
        });

        /*edit button*/
        Button btn_thumbsort = (Button) findViewById(R.id.btn_thumbsort);
        btn_thumbsort.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (!cur_path.equals(FileListManager.STORAGE))
                    showDialog(SORT_DIALOG_ID);
                else {
                    Toast.makeText(ThumbnailView1.this,
                        getText(R.string.Toast_msg_sort_noopen),
                        Toast.LENGTH_SHORT).show();
                }
            }
        });

        /*edit button*/
        /*Button btn_thumbedit = (Button) findViewById(R.id.btn_thumbedit);
        btn_thumbedit.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if (!cur_path.equals(FileListManager.STORAGE))
                    showDialog(EDIT_DIALOG_ID);
                else {
                    Toast.makeText(ThumbnailView1.this,
                        getText(R.string.Toast_msg_edit_noopen),
                        Toast.LENGTH_SHORT).show();
                }
            }
        });*/

        /* btn_help_listener */
        Button btn_thumbhelp = (Button) findViewById(R.id.btn_thumbhelp);
        btn_thumbhelp.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                showDialog(HELP_DIALOG_ID);
            }
        });

        /*switch_button*/
        Button btn_thumbswitch = (Button) findViewById(R.id.btn_thumbswitch);
        btn_thumbswitch.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                FileOp.SetMode(true);
                Intent intent = new Intent();
                intent.setClass(ThumbnailView1.this, FileBrower.class);
                Bundle bundle = new Bundle();
                bundle.putString("sort_flag", lv_sort_flag);
                intent.putExtras(bundle);
                local_mode = true;
                ThumbnailView1.this.finish();
                startActivity(intent);
            }
        });
    }

    public void onDestroy() {
        super.onDestroy();
        isInFileBrowserView=false;
        if(!local_mode){
            db.deleteAllFileMark();
        }
        db.close();
    }

    protected void onActivityResult(int requestCode, int resultCode,Intent data) {
        // TODO Auto-generated method stub
        super.onActivityResult(requestCode, resultCode, data);
        switch (resultCode) {
            case RESULT_OK:
                Bundle bundle = data.getExtras();
                cur_path = bundle.getString("cur_path");
                break;
            default:
                break;
        }
    }

    private void openFile(String file_path) {
        // TODO Auto-generated method stub
        File file = new File(file_path);
        Intent intent = new Intent();
        intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        intent.setAction(android.content.Intent.ACTION_VIEW);
        String type = "*/*";
        type = mFileListManager.CheckMediaType(file);
        intent.setDataAndType(Uri.fromFile(file),type);
        try {
            startActivity(intent);
        }
        catch (ActivityNotFoundException e){
            Toast.makeText(ThumbnailView1.this,
                getText(R.string.Toast_msg_no_applicaton),
                Toast.LENGTH_SHORT).show();
        }
    }

    protected Dialog onCreateDialog(int id) {
        LayoutInflater inflater = (LayoutInflater) ThumbnailView1.this.getSystemService(LAYOUT_INFLATER_SERVICE);
        switch (id) {
            case SORT_DIALOG_ID:
                View layout_sort = inflater.inflate(R.layout.sort_dialog_layout, (ViewGroup) findViewById(R.id.layout_root_sort));
                sort_dialog = new AlertDialog.Builder(ThumbnailView1.this)
                    .setView(layout_sort)
                    .setTitle(R.string.btn_sort_str)
                    .create();
                return sort_dialog;

            case EDIT_DIALOG_ID:
                View layout_edit = inflater.inflate(R.layout.edit_dialog_layout, (ViewGroup) findViewById(R.id.layout_root_edit));
                edit_dialog = new AlertDialog.Builder(ThumbnailView1.this)
                    .setView(layout_edit)
                    .setTitle(R.string.btn_edit_str)
                    .create();
                return edit_dialog;

            case HELP_DIALOG_ID:
                View layout_help = inflater.inflate(R.layout.help_dialog_layout, (ViewGroup) findViewById(R.id.layout_root_help));
                help_dialog = new AlertDialog.Builder(ThumbnailView1.this)
                    .setView(layout_help)
                    .setTitle(R.string.btn_help_str)
                    .create();
                return help_dialog;

            case LOAD_DIALOG_ID:
                if(load_dialog==null) {
                    load_dialog = new ProgressDialog(this);
                    load_dialog.setMessage(getText(R.string.load_dialog_msg_str));
                    load_dialog.setIndeterminate(true);
                    load_dialog.setCancelable(true);
                }
            return load_dialog;
        }
        return null;
    }

    protected void onPrepareDialog(int id, Dialog dialog) {
        WindowManager wm = getWindowManager();
        Display display = wm.getDefaultDisplay();
        LayoutParams lp = dialog.getWindow().getAttributes();
        switch (id) {
            case SORT_DIALOG_ID:
                if (display.getHeight() > display.getWidth()) {
                    lp.width = (int) (display.getWidth() * 1.0);
                }
                else {
                    lp.width = (int) (display.getWidth() * 0.5);
                }
                dialog.getWindow().setAttributes(lp);

                sort_lv = (ListView) sort_dialog.getWindow().findViewById(R.id.sort_listview);
                sort_lv.setAdapter(getDialogListAdapter(SORT_DIALOG_ID));
                sort_lv.setOnItemClickListener(new OnItemClickListener() {
                    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                        if (!cur_path.equals(FileListManager.STORAGE)) {
                            if (pos == 0) {
                                //GetCurrentFilelist(cur_path,"by_name");
                                lv_sort_flag = "by_name";
                                ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                                //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path, "by_name"));
                                cur_sort_type = "by_name";
                            }
                            else if (pos == 1) {
                                //GetCurrentFilelist(cur_path,"by_date");
                                lv_sort_flag = "by_date";
                                ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                                //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path, "by_date"));
                                cur_sort_type = "by_date";
                            }
                            else if (pos == 2) {
                                //GetCurrentFilelist(cur_path,"by_size");
                                lv_sort_flag = "by_size";
                                ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                                //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path, "by_size"));
                                cur_sort_type = "by_size";
                            }
                        }
                        sort_dialog.dismiss();
                    }
                });
            break;

            case EDIT_DIALOG_ID:
                if (display.getHeight() > display.getWidth()) {
                    lp.width = (int) (display.getWidth() * 1.0);
                }
                else {
                    lp.width = (int) (display.getWidth() * 0.5);
                }
                dialog.getWindow().setAttributes(lp);

                if(mProgressHandler == null) return;
                mProgressHandler.sendMessage(Message.obtain(mProgressHandler, 0));
                edit_lv = (ListView) edit_dialog.getWindow().findViewById(R.id.edit_listview);
                if(edit_lv == null) return;
                edit_lv.setAdapter(getDialogListAdapter(EDIT_DIALOG_ID));
                //edit_dialog.setCanceledOnTouchOutside(false);
                edit_dialog.setOnDismissListener(new OnDismissListener(){
                public void onDismiss(DialogInterface dialog) {
                FileOp.copy_cancel = true;
                }
                });

                edit_lv.setOnItemClickListener(new OnItemClickListener() {
                    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                        if (!cur_path.equals(FileListManager.STORAGE)) {
                            if(FileOp.IsBusy){
                                return;
                            }
                            File wFile = new File(cur_path);
                            if (!wFile.canWrite()) {
                                Toast.makeText(ThumbnailView1.this,
                                    getText(R.string.Toast_msg_no_write),
                                    Toast.LENGTH_SHORT).show();
                            } else {
                                if (pos == 0) {
                                    try {
                                        myCursor = db.getFileMark();
                                        if (myCursor.getCount() > 0) {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_cut_todo),
                                                Toast.LENGTH_SHORT).show();
                                            FileOp.file_op_todo = FileOpTodo.TODO_CUT;
                                        } else {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_cut_nofile),
                                                Toast.LENGTH_SHORT).show();
                                            FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                                        }
                                    } finally {
                                        myCursor.close();
                                    }
                                    edit_dialog.dismiss();
                                }
                                else if (pos == 1) {
                                    try {
                                        myCursor = db.getFileMark();
                                        if (myCursor.getCount() > 0) {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_cpy_todo),
                                                Toast.LENGTH_SHORT).show();
                                            FileOp.file_op_todo = FileOpTodo.TODO_CPY;
                                        }
                                        else {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_cpy_nofile),
                                                Toast.LENGTH_SHORT).show();
                                            FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                                        }
                                    }
                                    finally {
                                        myCursor.close();
                                    }
                                    edit_dialog.dismiss();
                                }
                                else if (pos == 2) {
                                    if (!mWakeLock.isHeld())
                                        mWakeLock.acquire();

                                    if (cur_path.startsWith(FileListManager.NAND)) {
                                        if (Environment.getExternalStorageState().equals(Environment.MEDIA_MOUNTED)) {
                                            new Thread () {
                                                public void run () {
                                                    try {
                                                        FileOp.pasteSelectedFile("thumbnail1");
                                                    } catch(Exception e) {
                                                        Log.e("Exception when paste file", e.toString());
                                                    }
                                                }
                                            }.start();
                                        }
                                        else {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_no_sdcard),
                                                Toast.LENGTH_SHORT).show();
                                        }
                                    }
                                    else {
                                        new Thread () {
                                            public void run () {
                                                try {
                                                    FileOp.pasteSelectedFile("thumbnail1");
                                                }
                                                catch(Exception e) {
                                                    Log.e("Exception when paste file", e.toString());
                                                }
                                            }
                                        }.start();
                                    }
                                }
                                else if (pos == 3) {
                                    FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                                    updateThumbnials();
                                    if (FileOpReturn.SUCCESS == FileOp.deleteSelectedFile("thumbnail1")) {
                                        db.deleteAllFileMark();
                                        //GetCurrentFilelist(cur_path,cur_sort_type);
                                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                                        scanAll();
                                        //ThumbnailView.setAdapter(getThumbnailAdapter(cur_path,null));
                                        Toast.makeText(ThumbnailView1.this,
                                            getText(R.string.Toast_msg_del_ok),
                                            Toast.LENGTH_SHORT).show();
                                    }
                                    else {
                                        Toast.makeText(ThumbnailView1.this,
                                            getText(R.string.Toast_msg_del_nofile),
                                            Toast.LENGTH_SHORT).show();
                                    }
                                    edit_dialog.dismiss();
                                }
                                else if (pos == 4) {
                                    FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                                    myCursor = db.getFileMark();
                                    if (myCursor.getCount() > 0) {
                                        if (myCursor.getCount() > 1) {
                                            String fullPath=FileOp.getMarkFilePath("thumbnail1");
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_rename_morefile)+"\n"+fullPath,
                                                Toast.LENGTH_LONG).show();
                                        }
                                        else {
                                            String fullPath=FileOp.getSingleMarkFilePath("thumbnail1");
                                            if (null != fullPath) {
                                                String dirPath=fullPath.substring(0, fullPath.lastIndexOf('/'));

                                                if (cur_path.equals(dirPath)) {
                                                    if (true != fileRename()) {
                                                        Toast.makeText(ThumbnailView1.this,
                                                            getText(R.string.Toast_msg_rename_error),
                                                            Toast.LENGTH_SHORT).show();
                                                    }
                                                }
                                                else {
                                                    Toast.makeText(ThumbnailView1.this,
                                                        getText(R.string.Toast_msg_rename_diffpath)+"\n"+dirPath,
                                                        Toast.LENGTH_LONG).show();
                                                }
                                            }
                                            else if(true!=fileRename()) {
                                                Toast.makeText(ThumbnailView1.this,
                                                    getText(R.string.Toast_msg_rename_error),
                                                    Toast.LENGTH_SHORT).show();
                                            }
                                        }
                                    }
                                    else {
                                        Toast.makeText(ThumbnailView1.this,
                                            getText(R.string.Toast_msg_rename_nofile),
                                            Toast.LENGTH_SHORT).show();
                                    }
                                    edit_dialog.dismiss();
                                }
                                else if (pos == 5) {
                                    FileOp.file_op_todo = FileOpTodo.TODO_NOTHING;
                                    myCursor = db.getFileMark();
                                    if (myCursor.getCount() > 0) {
                                        int ret = shareFile();
                                        if (ret <= 0) {
                                            Toast.makeText(ThumbnailView1.this,
                                                getText(R.string.Toast_msg_share_nofile),
                                                Toast.LENGTH_SHORT).show();
                                        }
                                    }
                                    else {
                                        Toast.makeText(ThumbnailView1.this,
                                            getText(R.string.Toast_msg_share_nofile),
                                            Toast.LENGTH_SHORT).show();
                                    }
                                    edit_dialog.dismiss();
                                }
                            }
                        }
                        else {
                            Toast.makeText(ThumbnailView1.this,
                                getText(R.string.Toast_msg_paste_wrongpath),
                                Toast.LENGTH_SHORT).show();
                            edit_dialog.dismiss();
                        }
                    }
                });
            break;

            case HELP_DIALOG_ID:
                if (display.getHeight() > display.getWidth()) {
                    lp.width = (int) (display.getWidth() * 1.0);
                }
                else {
                    lp.width = (int) (display.getWidth() * 0.5);
                }
                dialog.getWindow().setAttributes(lp);

                help_lv = (ListView) help_dialog.getWindow().findViewById(R.id.help_listview);
                help_lv.setAdapter(getDialogListAdapter(HELP_DIALOG_ID));
                help_lv.setOnItemClickListener(new OnItemClickListener() {
                    public void onItemClick(AdapterView<?> parent, View view, int pos, long id) {
                        help_dialog.dismiss();
                    }
                });
            break;

            case LOAD_DIALOG_ID:
                if (display.getHeight() > display.getWidth()) {
                    lp.width = (int) (display.getWidth() * 1.0);
                }
                else {
                    lp.width = (int) (display.getWidth() * 0.5);
                }
                dialog.getWindow().setAttributes(lp);

                dialog.setOnCancelListener(new DialogInterface.OnCancelListener() {
                    public void onCancel(DialogInterface dialog) {
                        mLoadCancel = true;
                    }
                });
                mLoadCancel = false;
            break;
        }
    }

    private Dialog mRenameDialog;
    private String name=null;
    private String path=null;
    private boolean fileRename() {
        LayoutInflater inflater = (LayoutInflater) getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View v = inflater.inflate(R.layout.file_rename, null);

        path=FileOp.getSingleMarkFilePath("thumbnail1");
        if (null != path) {
            int index=-1;
            index=path.lastIndexOf("/");
            if(index>=0) {
                name=path.substring(index+1);
                if(null==name) {
                    return false;
                }
            }
            else {
                return false;
            }
        }
        else {
            return false;
        }

        final EditText mRenameEdit = (EditText) v.findViewById(R.id.editTextRename);
        final File mRenameFile = new File(path);
        mRenameEdit.setText(name);

        Button buttonOK = (Button) v.findViewById(R.id.buttonOK);
        Button buttonCancel = (Button) v.findViewById(R.id.buttonCancel);

        buttonOK.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if( null != mRenameDialog ) {
                    mRenameDialog.dismiss();
                    mRenameDialog = null;
                }

                String newFileName = String.valueOf(mRenameEdit.getText());
                if(!name.equals(newFileName)) {
                    newFileName = path.substring(0, path.lastIndexOf('/') + 1) + newFileName;

                    if(mRenameFile.renameTo(new File(newFileName))) {
                        db.deleteAllFileMark();
                        ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                    }
                    else {
                        Toast.makeText(ThumbnailView1.this,
                            getText(R.string.Toast_msg_rename_error),
                            Toast.LENGTH_SHORT).show();
                    }
                }
            }
        });

        buttonCancel.setOnClickListener(new OnClickListener() {
            public void onClick(View v) {
                if( null != mRenameDialog){
                    mRenameDialog.dismiss();
                    mRenameDialog = null;
                }
            }
        });

        mRenameDialog = new AlertDialog.Builder(ThumbnailView1.this)
                                    .setView(v)
                                    .show();
        return true;
    }

    private int shareFile() {
        ArrayList<Uri> uris = new ArrayList<Uri>();
        Intent intent = new Intent();
        String type = "*/*";

        BluetoothAdapter ba = BluetoothAdapter.getDefaultAdapter();
        if(ba == null) {
            Toast.makeText(ThumbnailView1.this,
                getText(R.string.Toast_msg_share_nodev),
                Toast.LENGTH_SHORT).show();
            return 0xff;
        }

        uris = FileOp.getMarkFilePathUri("thumbnail1");
        final int size = uris.size();

        if(size > 0) {
            if (size > 1) {
                intent.setAction(Intent.ACTION_SEND_MULTIPLE).setType(type);
                intent.putParcelableArrayListExtra(Intent.EXTRA_STREAM, uris);
            } else {
                intent.setAction(Intent.ACTION_SEND).setType(type);
                intent.putExtra(Intent.EXTRA_STREAM, uris.get(0));
            }
            intent.setType(type);
            startActivity(intent);
        }
        return size;
    }

    /** getDialogListAdapter */
    private SimpleAdapter getDialogListAdapter(int id) {
        return new SimpleAdapter(ThumbnailView1.this,
            getDialogListData(id),
            R.layout.dialog_item,
            new String[]{
            "item_type",
            "item_name",
            "item_sel",},
            new int[]{
            R.id.dialog_item_type,
            R.id.dialog_item_name,
            R.id.dialog_item_sel,
        });
    }

    /** getFileListData */
    private List<Map<String, Object>> getDialogListData(int id) {
        List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        Map<String, Object> map;

        switch (id) {
            case SORT_DIALOG_ID:
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_name);
                map.put("item_name", getText(R.string.sort_dialog_name_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_date);
                map.put("item_name", getText(R.string.sort_dialog_date_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_size);
                map.put("item_name", getText(R.string.sort_dialog_size_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                break;

            case EDIT_DIALOG_ID:
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_cut);
                map.put("item_name", getText(R.string.edit_dialog_cut_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_copy);
                map.put("item_name", getText(R.string.edit_dialog_copy_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_paste);
                map.put("item_name", getText(R.string.edit_dialog_paste_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_delete);
                map.put("item_name", getText(R.string.edit_dialog_delete_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_rename);
                map.put("item_name", getText(R.string.edit_dialog_rename_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_item_type_size);
                map.put("item_name", getText(R.string.edit_dialog_share_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                break;

            case HELP_DIALOG_ID:
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_home);
                map.put("item_name", getText(R.string.dialog_help_item_home_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_mode);
                map.put("item_name", getText(R.string.dialog_help_item_mode_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_edit);
                map.put("item_name", getText(R.string.dialog_help_item_edit_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_sort);
                map.put("item_name", getText(R.string.dialog_help_item_sort_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_parent);
                map.put("item_name", getText(R.string.dialog_help_item_parent_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                /*map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_thumb);
                map.put("item_name", getText(R.string.dialog_help_item_thumb_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map); */
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_list);
                map.put("item_name", getText(R.string.dialog_help_item_list_str));
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                /*
                map = new HashMap<String, Object>();
                map.put("item_type", R.drawable.dialog_help_item_close);
                String ver_str = " ";
                try {
                    ver_str += getPackageManager().getPackageInfo("com.droidlogic.FileBrower", 0).versionName;
                } catch (NameNotFoundException e) {
                    // TODO Auto-generated catch block
                    e.printStackTrace();
                }
                map.put("item_name", getText(R.string.dialog_help_item_close_str) + ver_str);
                map.put("item_sel", R.drawable.dialog_item_img_unsel);
                list.add(map);
                */
            break;
        }
        return list;
    }

    //option menu
    public boolean onCreateOptionsMenu(Menu menu) {
        String ver_str = null;
        try {
            ver_str = getPackageManager().getPackageInfo("com.droidlogic.FileBrower", 0).versionName;
        }
        catch (NameNotFoundException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }
        menu.add(0, 0, 0, getText(R.string.app_name) + " v" + ver_str);
        return true;
    }

    private void scanAll(){
        Intent intent = new Intent();
        intent.setClassName("com.android.providers.media","com.android.providers.media.MediaScannerService");
        Bundle argsa = new Bundle();
        argsa.putString("path", FileListManager.NAND);
        argsa.putString("volume","external");
        startService(intent.putExtras(argsa));
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            if (!cur_path.equals(FileListManager.STORAGE)) {
                File file = new File(cur_path);
                String parent_path = file.getParent();
                if (cur_path.equals(FileListManager.NAND) || parent_path.equals(FileListManager.MEDIA_RW)) {
                    cur_path = FileListManager.STORAGE;
                }
                else {
                    cur_path = parent_path;
                }
                ThumbnailView.setAdapter(getFileListAdapterSorted(cur_path, lv_sort_flag));
                return true;
            }
        }
        return super.onKeyDown(keyCode, event);
    }
}
