/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description:
*/
package com.droidlogic.videoplayer;

import android.os.storage.*;
import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Map;
import java.util.Iterator;

import com.droidlogic.videoplayer.R;

import android.app.AlertDialog;
import android.app.ListActivity;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.Manifest;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView;
import android.widget.Toast;
import android.view.Menu;
import android.view.MenuItem;
import java.io.IOException;
import java.io.InputStream;
import java.lang.reflect.Method;
import java.lang.System;
import android.database.Cursor;
import android.provider.MediaStore;
import android.widget.ProgressBar;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import android.os.Handler;
import java.util.Collections;
import java.util.Comparator;
import java.util.Timer;
import java.util.TimerTask;
import android.os.Message;
import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.FileListManager;
import com.droidlogic.app.FileListManager.MyFilter;

public class FileList extends ListActivity {
        private static final String SD_PATH             = "/storage/external_storage/sdcard1";
        private static final String SURPORT_BIN         = ",bin";
        private static final String NOT_SURPORT         = "";

        public static final String KEY_NAME = "key_name";
        public static final String KEY_PATH = "key_path";
        public static final String KEY_TYPE = "key_type";
        public static final String KEY_DATE = "key_date";
        public static final String KEY_SIZE = "key_size";
        public static final String KEY_SELE = "key_sele";
        public static final String KEY_RDWR = "key_rdwr";

        private static final int REQUEST_CODE_ASK_PERMISSIONS = 2;

        private Context mContext;
        private ApplicationInfo mAppInfo;

        private boolean listAllFiles = true;
        private boolean mFileFlag = false;
        private boolean mClickFlag = false;
        private List<Map<String, Object>> listFiles = null;
        private List<Map<String, Object>> listVideos = null;
        private List<String> items = null;
        private List<String> paths = null;
        private List<String> currentlist = null;
        private String currenturl = null;
        private String root_path = null;
        private String extensions ;
        private static String ISOpath = null;

        private ArrayList<String>  nfPath = new ArrayList<String> ();
        private ArrayList<String>  nfPathFileName = new ArrayList<String> ();

        private TextView tileText;
        private TextView nofileText;
        private TextView searchText;
        private ProgressBar sp;
        private boolean isScanning = false;
        private boolean isQuerying = false;
        private int scanCnt = 0;
        private int devCnt = 0;
        private File file;
        private static String TAG = "FileList";
        Timer timer = new Timer();
        Timer timerScan = new Timer();

        private int item_position_selected, item_position_first, fromtop_piexl;
        private ArrayList<Integer> fileDirectory_position_selected;
        private ArrayList<Integer> fileDirectory_position_piexl;
        private int pathLevel = 0;
        private final String iso_mount_dir = "/mnt/loop";
        private static final String iso_mount_dir_s = "/mnt/loop";
        private Uri uri;
        private SystemControlManager mSystemControl;
        private FileListManager mFileListManager;

        private void waitForBrowserIsoFile() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x4c:
                            BrowserFile (iso_mount_dir);
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x4c;
                    handler.sendMessage (message);
                }
            };
            timer.cancel();
            timer = new Timer();
            timer.schedule (task, 100);//add 100ms delay to wait fuse finish
        }

        private void waitForRescan() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x5c:
                            isScanning = false;
                            prepareFileForList();
                            timerScan.cancel();
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x5c;
                    handler.sendMessage (message);
                }
            };
            timer.cancel();
            timer = new Timer();
            timer.schedule (task, 500);
        }

        private void waitForScanFinish() {
            final Handler handler = new Handler() {
                public void handleMessage (Message msg) {
                    switch (msg.what) {
                        case 0x6c:
                            scanCnt--;
                            isScanning = false;
                            prepareFileForList();
                            break;
                    }
                    super.handleMessage (msg);
                }
            };
            TimerTask task = new TimerTask() {
                public void run() {
                    Message message = Message.obtain();
                    message.what = 0x6c;
                    handler.sendMessage (message);
                }
            };
            timerScan.cancel();
            timerScan = new Timer();
            timerScan.schedule (task, 20000);
        }

        private BroadcastReceiver mListener = new BroadcastReceiver() {
            @Override
            public void onReceive (Context context, Intent intent) {
                String action = intent.getAction();
                Uri uri = intent.getData();
                String path = uri.getPath();
                if (action == null || path == null) {
                    return;
                }
                if (action.equals(Intent.ACTION_MEDIA_EJECT)
                    || action.equals(Intent.ACTION_MEDIA_UNMOUNTED) ) {
                    if (listAllFiles) {
                        prepareFileForList();
                    }
                    else {
                        if (PlayList.getinstance().rootPath.startsWith (path)
                            || PlayList.getinstance().rootPath.equals (root_path)) {
                            BrowserFile(root_path);
                            if (action.equals(Intent.ACTION_MEDIA_EJECT) && mClickFlag) {
                                pathLevel = 0;
                            }
                        }
                    }
                }
                else if ((action.equals ("com.droidvold.action.MEDIA_UNMOUNTED")
                        || action.equals ("com.droidvold.action.MEDIA_EJECT")) && !path.equals("/dev/null")) {
                    if (PlayList.getinstance().rootPath.startsWith (path)
                        || PlayList.getinstance().rootPath.equals (root_path)) {
                        BrowserFile(root_path);
                        if (action.equals("com.droidvold.action.MEDIA_EJECT") && mClickFlag) {
                            pathLevel = 0;
                        }
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_MOUNTED) || action.equals ("com.droidvold.action.MEDIA_MOUNTED")) {
                    if (PlayList.getinstance().rootPath == null
                        || PlayList.getinstance().rootPath.equals(root_path)) {
                        BrowserFile (root_path);
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_SCANNER_STARTED)) {
                    if (!isScanning) {
                        isScanning = true;
                        setListAdapter (null);
                        showSpinner();
                        scanCnt++;
                        waitForScanFinish();
                    }
                }
                else if (action.equals(Intent.ACTION_MEDIA_SCANNER_FINISHED)) {
                    if (isScanning && (scanCnt == 1)) {
                        scanCnt--;
                        waitForRescan();
                    }
                }
            }
        };

        @Override
        public void onResume() {
            super.onResume();
            IntentFilter f = new IntentFilter();
            f.addAction (Intent.ACTION_MEDIA_EJECT);
            f.addAction (Intent.ACTION_MEDIA_MOUNTED);
            f.addAction (Intent.ACTION_MEDIA_UNMOUNTED);
            f.addAction ("com.droidvold.action.MEDIA_UNMOUNTED");
            f.addAction ("com.droidvold.action.MEDIA_MOUNTED");
            f.addAction ("com.droidvold.action.MEDIA_EJECT");
            f.addDataScheme ("file");

            if (!listAllFiles) {
                File file = null;
                if (PlayList.getinstance().rootPath != null) {
                    file = new File (PlayList.getinstance().rootPath);
                }
                if ( (file != null) && file.exists()) {
                    List<Map<String, Object>> the_Files = new ArrayList<Map<String, Object>>();
                    the_Files = mFileListManager.getDirs(PlayList.getinstance().rootPath, "video");
                    if (the_Files == null || the_Files.size() <= 0) {
                        PlayList.getinstance().rootPath = root_path;
                    }
                    BrowserFile (PlayList.getinstance().rootPath);
                }
                else {
                    PlayList.getinstance().rootPath = root_path;
                    BrowserFile (PlayList.getinstance().rootPath);
                }
                getListView().setSelectionFromTop (item_position_selected, fromtop_piexl);
            }
            else {
                f.addAction (Intent.ACTION_MEDIA_SCANNER_STARTED);
                f.addAction (Intent.ACTION_MEDIA_SCANNER_FINISHED);
            }

            registerReceiver(mListener, f);
        }

        public void onDestroy() {
            super.onDestroy();
        }

        @Override
        public void onPause() {
            super.onPause();
            if (listAllFiles) {
                isScanning = false;
                isQuerying = false;
                scanCnt = 0;
                timer.cancel();
                timerScan.cancel();
            }
            unregisterReceiver(mListener);
        }

        @Override
        protected void onCreate (Bundle icicle) {
            super.onCreate (icicle);
            extensions = getResources().getString (R.string.support_video_extensions);
            requestWindowFeature (Window.FEATURE_NO_TITLE);
            setContentView (R.layout.file_list);
            mSystemControl = SystemControlManager.getInstance();
            if (!mSystemControl.getPropertyBoolean("vendor.sys.videoplayer.surportbin", false)) {
                extensions = extensions.replaceAll(SURPORT_BIN, NOT_SURPORT);
            }
            mFileListManager = new FileListManager(this);
            root_path = FileListManager.STORAGE;
            mContext = this.getApplicationContext();
            mAppInfo = mContext.getApplicationInfo();
            PlayList.setContext (this);
            listAllFiles = mSystemControl.getPropertyBoolean("vendor.vplayer.listall.enable", false);
            currentlist = new ArrayList<String>();
            if (!listAllFiles) {
                try {
                    Bundle bundle = new Bundle();
                    bundle = this.getIntent().getExtras();
                    if (bundle != null) {
                        item_position_selected = bundle.getInt ("item_position_selected");
                        item_position_first = bundle.getInt ("item_position_first");
                        fromtop_piexl = bundle.getInt ("fromtop_piexl");
                        fileDirectory_position_selected = bundle.getIntegerArrayList ("fileDirectory_position_selected");
                        fileDirectory_position_piexl = bundle.getIntegerArrayList ("fileDirectory_position_piexl");
                        pathLevel = fileDirectory_position_selected.size();
                    }
                }
                catch (Exception e) {
                    e.printStackTrace();
                }
                if (PlayList.getinstance().rootPath == null) {
                    PlayList.getinstance().rootPath = root_path;
                }
                //BrowserFile (PlayList.getinstance().rootPath);
            }
            Button home = (Button) findViewById (R.id.Button_home);
            home.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    if (listAllFiles) {
                        if (!isScanning) {
                            reScanVideoFiles();
                        }
                    }
                    else {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                }
            });
            Button exit = (Button) findViewById (R.id.Button_exit);
            exit.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    if (listAllFiles) {
                        FileList.this.finish();
                        return;
                    }
                    if (paths == null) {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                    else {
                        if (paths.isEmpty() || paths.get (0) == null) {
                            FileList.this.finish();
                            PlayList.getinstance().rootPath = null;
                            return;
                        }
                        file = new File (paths.get (0).toString());
                        if (file.getParent().compareTo (iso_mount_dir) == 0 && ISOpath != null) {
                            file = new File (ISOpath + "/VIRTUAL_CDROM");
                            ISOpath = null;
                        }
                        currenturl = file.getParentFile().getParent();
                        if ( (file.getParent().compareToIgnoreCase (root_path) != 0) && (pathLevel > 0)) {
                            String path = file.getParent();
                            String parent_path = file.getParentFile().getParent();
                            if ( (path.equals (FileListManager.NAND) || parent_path.equals (FileListManager.MEDIA_RW)) && (pathLevel > 0)) {
                                pathLevel = 0;
                                BrowserFile (FileListManager.STORAGE);
                            }
                            else {
                                BrowserFile (currenturl);
                                pathLevel--;
                                getListView().setSelectionFromTop (fileDirectory_position_selected.get (pathLevel), fileDirectory_position_piexl.get (pathLevel));
                                fileDirectory_position_selected.remove (pathLevel);
                                fileDirectory_position_piexl.remove (pathLevel);
                            }
                        }
                        else {
                            FileList.this.finish();
                            PlayList.getinstance().rootPath = null;
                        }
                    }
                }
            });
            nofileText = (TextView) findViewById (R.id.TextView_nofile);
            searchText = (TextView) findViewById (R.id.TextView_searching);
            sp = (ProgressBar) findViewById (R.id.spinner);
            if (listAllFiles) {
                prepareFileForList();
            }
        }

        @Override
        public void onRequestPermissionsResult(int requestCode,
                    String permissions[], int[] grantResults) {
            switch (requestCode) {
                case REQUEST_CODE_ASK_PERMISSIONS: {
                    if (grantResults.length > 0
                            && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                        Log.d(TAG, "write external storage permission is granted");
                    } else {
                        Log.d(TAG, "write external storage permission permission is denied");
                        return;
                    }
                    break;
                }
            }
        }

        private void showSpinner() {
            if (listAllFiles) {
                if ( (isScanning) || (isQuerying)) {
                    sp.setVisibility (View.VISIBLE);
                    searchText.setVisibility (View.VISIBLE);
                    nofileText.setVisibility (View.INVISIBLE);
                }
                else {
                    sp.setVisibility (View.INVISIBLE);
                    searchText.setVisibility (View.INVISIBLE);
                    int total = paths.size();
                    if (total == 0) {
                        nofileText.setVisibility (View.VISIBLE);
                    }
                    else if (total > 0) {
                        nofileText.setVisibility (View.INVISIBLE);
                    }
                }
            }
            else {
                sp.setVisibility (View.GONE);
                nofileText.setVisibility (View.GONE);
                searchText.setVisibility (View.GONE);
            }
        }

        private void prepareFileForList() {
            if (listAllFiles) {
                //Intent intent = getIntent();
                //uri = intent.getData();
                String[] mCursorCols = new String[] {
                    MediaStore.Video.Media._ID,
                    MediaStore.Video.Media.DATA,
                    MediaStore.Video.Media.TITLE,
                    MediaStore.Video.Media.SIZE,
                    MediaStore.Video.Media.DURATION,
                    //              MediaStore.Video.Media.BOOKMARK,
                    //              MediaStore.Video.Media.PLAY_TIMES
                };
                String patht = null;
                String namet = null;
                paths = new ArrayList<String>();
                items = new ArrayList<String>();
                paths.clear();
                items.clear();
                setListAdapter (null);
                isQuerying = true;
                showSpinner();
                uri = MediaStore.Video.Media.EXTERNAL_CONTENT_URI;
                Cursor cursor = getContentResolver().query (uri, mCursorCols, null, null, null);
                cursor.moveToFirst();
                int colidx = cursor.getColumnIndexOrThrow (MediaStore.Video.Media.DATA);
                for (int i = 0; i < cursor.getCount(); i++) {
                    patht = cursor.getString (colidx);
                    int index = patht.lastIndexOf ("/");
                    if (index >= 0) {
                        namet = patht.substring (index);
                    }
                    items.add (namet);
                    paths.add (patht);
                    cursor.moveToNext();
                }
                tileText = (TextView) findViewById (R.id.TextView_path);
                tileText.setText (R.string.all_file);
                if (paths.size() > 0) {
                    setListAdapter (new MyAdapter (this, items, paths));
                }
                isQuerying = false;
                showSpinner();
                if (cursor != null) {
                    cursor.close();
                }
            }
        }

        private void BrowserFile (String filePath) {
            int i = 0;
            int dev_usb_count = 0;
            int dev_cd_count = 0;

            Log.d(TAG, "BrowserFile path=" + filePath + "  Check perm");

            if (checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE)
                    != PackageManager.PERMISSION_GRANTED) {
                requestPermissions(new String[]{Manifest.permission.WRITE_EXTERNAL_STORAGE},
                        REQUEST_CODE_ASK_PERMISSIONS);
                return;
            }

            file = new File (filePath);
            listFiles = new ArrayList<Map<String, Object>>();
            items = new ArrayList<String>();
            paths = new ArrayList<String>();
            String[] files = file.list();
            listFiles.clear();
            nfPathFileName.clear();
            nfPath.clear();
            searchFile (file);
            if (listFiles.isEmpty()) {
                Toast.makeText (FileList.this, R.string.str_no_file, Toast.LENGTH_SHORT).show();
                //paths =currentlist;
                paths.clear();
                paths.addAll (currentlist);
                return;
            }

            PlayList.getinstance().rootPath = filePath;

            //change device name;
            if (filePath.equals (FileListManager.STORAGE)) {
                listFiles = mFileListManager.getDevices();
                devCnt = listFiles.size();
                for (int j = 0; j < devCnt; j++) {
                    Map<String, Object> map = listFiles.get(j);
                    String keyName = (String)map.get(KEY_NAME);
                    items.add(keyName);
                    String keyPath = (String)map.get(KEY_PATH);
                    paths.add(keyPath);
                }
            }
            else {
                listFiles = mFileListManager.getDirs(filePath, "video");
                devCnt = listFiles.size();
                try {
                    Collections.sort(listFiles, new Comparator<Map<String, Object>>() {
                        public int compare(Map<String, Object> object1, Map<String, Object> object2) {
                            File file1 = new File((String) object1.get(KEY_PATH));
                            File file2 = new File((String) object2.get(KEY_PATH));

                            if (file1.isFile() && file2.isFile() || file1.isDirectory() && file2.isDirectory()) {
                                int k = ((String) object1.get(KEY_NAME)).toLowerCase()
                                    .compareTo(((String) object2.get(KEY_NAME)).toLowerCase());
                                return ((String) object1.get(KEY_NAME)).toLowerCase()
                                    .compareTo(((String) object2.get(KEY_NAME)).toLowerCase());
                            } else {
                                return file1.isFile() ? 1 : -1;
                            }
                        }
                    });
                }
                catch (IllegalArgumentException ex) {
                }
                for (int j = 0; j < devCnt; j++) {
                    Map<String, Object> map = listFiles.get(j);
                    String keyName = (String)map.get(KEY_NAME);
                    items.add(keyName);
                    String keyPath = (String)map.get(KEY_PATH);
                    paths.add(keyPath);
                }
            }
            tileText = (TextView) findViewById (R.id.TextView_path);
            tileText.setText (catShowFilePath (filePath));
            setListAdapter (new MyAdapter (this, items, paths));
        }

        private String changeDevName (String tmppath) {
            String path = "";
            String internal = getString (R.string.memory_device_str);
            String sdcard = getString (R.string.sdcard_device_str);
            String usb = getString (R.string.usb_device_str);
            String cdrom = getString (R.string.cdrom_device_str);
            String sdcardExt = getString (R.string.ext_sdcard_device_str);
            if (tmppath.equals ("flash")) {
                path = internal;
            }
            else if (tmppath.equals ("sdcard")) {
                path = sdcard;
            }
            else if (tmppath.equals ("usb")) {
                path = usb;
            }
            else if (tmppath.equals ("cd-rom")) {
                path = cdrom;
            }
            else if (tmppath.equals ("external_sdcard")) {
                path = sdcardExt;
            }
            else {
                path = tmppath;
            }
            return path;
        }

        private String catShowFilePath (String path) {
            String text = null;
            if (path.startsWith ("/mnt/flash")) {
                text = path.replaceFirst ("/mnt/flash", "/mnt/nand");
            }
            else if (path.startsWith ("/mnt/sda")) {
                text = path.replaceFirst ("/mnt/sda", "/mnt/usb sda");
            }
            else if (path.startsWith ("/mnt/sdb")) {
                text = path.replaceFirst ("/mnt/sdb", "/mnt/usb sdb");
            }
            //else if(path.startsWith("/mnt/sdcard"))
            //text=path.replaceFirst("/mnt/sdcard","sdcard");
            return text;
        }

        public void searchFile (File file) {
            String curPath = file.getPath();
            List<Map<String, Object>> the_Files;
            if (curPath.equals (root_path)) {
                the_Files = mFileListManager.getDevices();
            } else {
                the_Files = mFileListManager.getDirs(curPath, "video");
            }
            if (the_Files == null) {
                Toast.makeText (FileList.this, R.string.str_no_file, Toast.LENGTH_SHORT).show();
                return;
            }
            if (curPath.equals (root_path)) {
                listFiles = mFileListManager.getDevices();
                if (listFiles == null) {
                    Toast.makeText (FileList.this, R.string.str_no_file, Toast.LENGTH_SHORT).show();
                    return;
                }
                return;
            }
            listFiles = the_Files;
        }

        public static void execCmd (String cmd) {
            int ch;
            Process p = null;
            Log.d (TAG, "exec command: " + cmd);
            try {
                p = Runtime.getRuntime().exec (cmd);
                InputStream in = p.getInputStream();
                InputStream err = p.getErrorStream();
                StringBuffer sb = new StringBuffer (512);
                while ( (ch = in.read()) != -1) {
                    sb.append ( (char) ch);
                }
                if (sb.toString() != "") {
                    Log.d (TAG, "exec out:" + sb.toString());
                }
                while ( (ch = err.read()) != -1) {
                    sb.append ( (char) ch);
                }
                if (sb.toString() != "") {
                    Log.d (TAG, "exec error:" + sb.toString());
                }
            }
            catch (IOException e) {
                Log.d (TAG, "IOException: " + e.toString());
            }
        }

        @Override
        protected void onListItemClick (ListView l, View v, int position, long id) {
            mFileFlag = true;
            File file = new File (paths.get (position));
            currentlist.clear();
            currentlist.addAll (paths);
            //currentlist =paths;
            if (fileDirectory_position_selected == null) {
                fileDirectory_position_selected = new ArrayList<Integer>();
            }
            if (fileDirectory_position_piexl == null) {
                fileDirectory_position_piexl = new ArrayList<Integer>();
            }
            //if (false/*mFileListManager.isBDFile(file)*/) {  //not support BD file from p
                //mFileFlag = true;
            //} else {
            if (file.isDirectory()) {
                item_position_selected = getListView().getSelectedItemPosition();
                item_position_first = getListView().getFirstVisiblePosition();
                View cv = getListView().getChildAt (item_position_selected - item_position_first);
                if (cv != null) {
                    fromtop_piexl = cv.getTop();
                }
                BrowserFile (paths.get (position));
                if (!listFiles.isEmpty()) {
                    fileDirectory_position_selected.add (item_position_selected);
                    fileDirectory_position_piexl.add (fromtop_piexl);
                    pathLevel++;
                    mClickFlag = true;
                }
                mFileFlag = false;
            } else if (mFileListManager.isISOFile(file)) {
                waitForBrowserIsoFile();
                fileDirectory_position_selected.add (item_position_selected);
                fileDirectory_position_piexl.add (fromtop_piexl);
                pathLevel++;
                mFileFlag = false;
            } else
                mFileFlag = true;
            //}
            if (mFileFlag) {
                if (!listAllFiles) {
                    int pos = filterDir (file);
                    if (pos < 0) {
                        return;
                    }
                    PlayList.getinstance().rootPath = file.getParent();
                    PlayList.getinstance().setlist (paths, pos);
                    item_position_selected = getListView().getSelectedItemPosition();
                    item_position_first = getListView().getFirstVisiblePosition();
                    if (!listAllFiles) {
                        View cv = getListView().getChildAt (item_position_selected - item_position_first);
                        if (cv != null) {
                            fromtop_piexl = cv.getTop();
                        }
                    }
                }
                else {
                    PlayList.getinstance().setlist (paths, position);
                }
                showvideobar();
            }
        }
        public boolean onKeyDown (int keyCode, KeyEvent event) {
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                if (listAllFiles) {
                    FileList.this.finish();
                    return true;
                }
                if (paths == null) {
                    FileList.this.finish();
                    PlayList.getinstance().rootPath = null;
                }
                else {
                    if (paths.isEmpty() || paths.get (0) == null) {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                        return true;
                    }
                    file = new File (paths.get (0).toString());
                    if (file.getParent().compareTo (iso_mount_dir) == 0 && ISOpath != null) {
                        file = new File (ISOpath + "/VIRTUAL_CDROM");//"/VIRTUAL_CDROM" random add for path level reduce
                        ISOpath = null;
                    }
                    currenturl = file.getParentFile().getParent();
                    if ( (file.getParent().compareToIgnoreCase (root_path) != 0) && (pathLevel > 0)) {
                        String path = file.getParent();
                        String parent_path = file.getParentFile().getParent();
                        if ( (path.equals (FileListManager.NAND) || parent_path.equals (FileListManager.MEDIA_RW)) && (pathLevel > 0)) {
                            pathLevel = 0;
                            BrowserFile (FileListManager.STORAGE);
                        }
                        else {
                            if (pathLevel == 1) {
                                BrowserFile (FileListManager.STORAGE);
                            } else {
                                BrowserFile (currenturl);
                            }
//                            BrowserFile (currenturl);
                            pathLevel--;
                            getListView().setSelectionFromTop (fileDirectory_position_selected.get (pathLevel), fileDirectory_position_piexl.get (pathLevel));
                            fileDirectory_position_selected.remove (pathLevel);
                            fileDirectory_position_piexl.remove (pathLevel);
                        }
                    }
                    else {
                        FileList.this.finish();
                        PlayList.getinstance().rootPath = null;
                    }
                }
                return true;
            }
            return super.onKeyDown (keyCode, event);
        }
        private void showvideobar() {
            //* new an Intent object and ponit a class to start
            Intent intent = new Intent();
            Bundle bundle = new Bundle();
            if (!listAllFiles) {
                bundle.putInt ("item_position_selected", item_position_selected);
                bundle.putInt ("item_position_first", item_position_first);
                bundle.putInt ("fromtop_piexl", fromtop_piexl);
                bundle.putIntegerArrayList ("fileDirectory_position_selected", fileDirectory_position_selected);
                bundle.putIntegerArrayList ("fileDirectory_position_piexl", fileDirectory_position_piexl);
            }
            bundle.putBoolean ("backToOtherAPK", false);
            intent.setClass (FileList.this, VideoPlayer.class);
            intent.putExtras (bundle);
            ///wxl delete
            /*SettingsVP.setSystemWrite(sw);
            if (SettingsVP.chkEnableOSD2XScale() == true)
                this.setVisible(false);*/
            /*if (mAppInfo.targetSdkVersion >= Build.VERSION_CODES.M &&
                (PackageManager.PERMISSION_DENIED == ContextCompat.checkSelfPermission(mContext, Manifest.permission.READ_EXTERNAL_STORAGE))) {
                ActivityCompat.requestPermissions(FileList.this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE},
                    //MY_PERMISSIONS_REQUEST_READ_EXTERNAL_STORAGE//0);
            }
            else {
                startActivity (intent);
                FileList.this.finish();
            }*/
            startActivity (intent);
            //FileList.this.finish();
        }

        public int filterDir (File file) {
            int pos = -1;
            listVideos = new ArrayList<Map<String, Object>>();
            listVideos = mFileListManager.getDirs(file.getParent(), "video");
            if (listVideos == null) {
                return pos;
            }
            pos = 0;
            paths = new ArrayList<String>();
            try {
                Collections.sort(listVideos, new Comparator<Map<String, Object>>() {
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
            catch (IllegalArgumentException ex) {
            }
            for (int i = 0; i < listVideos.size(); i++) {
                Map<String, Object> fMap = listVideos.get(i);
                String tempF = (String)fMap.get(KEY_PATH);
                if (tempF.equals (file.getPath())) {
                    pos = i;
                }
                paths.add (tempF);
            }
            return pos;
        }

        //option menu
        private final int MENU_ABOUT = 0;
        public boolean onCreateOptionsMenu (Menu menu) {
            menu.add (0, MENU_ABOUT, 0, R.string.str_about);
            return true;
        }

        public boolean onOptionsItemSelected (MenuItem item) {
            switch (item.getItemId()) {
                case MENU_ABOUT:
                    try {
                        Toast.makeText (FileList.this, " VideoPlayer \n Version: " +
                                        FileList.this.getPackageManager().getPackageInfo ("com.droidlogic.videoplayer", 0).versionName,
                                        Toast.LENGTH_SHORT)
                        .show();
                    }
                    catch (NameNotFoundException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                    return true;
            }
            return false;
        }

        public void reScanVideoFiles() {
            Intent intent = new Intent (Intent.ACTION_MEDIA_MOUNTED, Uri.parse ("file://" + mFileListManager.STORAGE));
            this.sendBroadcast (intent);
        }

        public void stopMediaPlayer() { //stop the backgroun music player
            Intent intent = new Intent();
            intent.setAction ("com.android.music.musicservicecommand.pause");
            intent.putExtra ("command", "stop");
            this.sendBroadcast (intent);
        }
}
