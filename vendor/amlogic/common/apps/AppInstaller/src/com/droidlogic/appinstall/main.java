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
package com.droidlogic.appinstall;


import java.io.File;
import java.io.FileFilter;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;
import java.util.Iterator;

import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.ProgressDialog;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.PackageInfo;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
//import android.os.storage.VolumeInfo;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.MotionEvent;
import android.view.View;
import android.view.ViewGroup;
import android.widget.AdapterView;
import android.widget.AdapterView.OnItemSelectedListener;
import android.widget.Button;
import android.widget.EditText;
import android.widget.ImageButton;
import android.widget.ListView;
import android.widget.TextView.BufferType;
import android.widget.TextView;
import android.widget.Toast;
//import android.content.pm.IPackageInstallObserver2;
//import android.content.pm.IPackageDeleteObserver;
import android.os.PowerManager;
import android.os.HandlerThread;
import android.os.storage.StorageManager;
import android.provider.Settings;
import android.content.Context;
import android.view.KeyEvent;
import android.content.res.Configuration;
import android.content.DialogInterface;
import java.lang.String;
import android.os.StatFs;
import android.os.Environment;
import android.content.BroadcastReceiver;
import android.content.IntentFilter;
import com.droidlogic.app.FileListManager;

public class main extends Activity {
        private String TAG = "Appinstall";
        private String mVersion = "V1.1.3";
        private String mReleaseDate = "2017.09.26";

        public static final String KEY_NAME = "key_name";
        public static final String KEY_PATH = "key_path";

        //UI INFO
        protected String mScanRoot = null;
        protected CheckAbleList m_list = null;
        protected TextView m_info = null;
        protected PackageAdapter pkgadapter = null;
        protected TextView m_DirEdit = null;
        protected OperationDialog mScanDiag = null;
        protected OperationDialog mHandleDiag = null;

        //DATA
        protected ArrayList<APKInfo> mApkList = new ArrayList<APKInfo>();
        protected boolean m_configchanged = false;
        public final static int END_OPERATION = 0;
        public final static int NEW_APK = 1;
        public final static int HANDLE_PKG_NEXT = 2;
        public final static int HANDLE_PKG_FAIL = 3;
        private static final int DLG_UNKNOWN_APPS = 1;
        private PowerManager.WakeLock mScreenLock = null;
        protected ScanOperation m_scanop = new ScanOperation();
        protected InstallOperation m_installop = new InstallOperation();
        protected String mDevs[] = null;
        protected String mDevStrs[] = null;

        //the status of app
        private static int SCAN_APKS = 0;
        private static int VIEW_APKS = 1;
        private static int INSTALL_APKS = 2;
        protected int mStatus = -1;
        private int item_position_selected, item_position_first, fromtop_piexl;

        private static FileListManager mFileListManager;
        private List<Map<String, Object>> mVolumes;
        private boolean recvFlag = false;

        private boolean mIsAppInsatlledFlg = false;
        private boolean mIsClickPathFlg = true;
        /** Called when the activity is first created. */
        @Override
        public void onCreate (Bundle savedInstanceState) {
            Log.d (TAG, "[onCreate]");
            super.onCreate (savedInstanceState);
            setContentView (R.layout.main);

            mFileListManager = new FileListManager(this);

            //keep system awake
            mScreenLock = ( (PowerManager) this.getSystemService (Context.POWER_SERVICE)).newWakeLock (PowerManager.SCREEN_BRIGHT_WAKE_LOCK, TAG);
            m_info = (TextView) findViewById (R.id.ScanInfo);
            m_info.setText (R.string.no_apk_found);
            m_info.setVisibility (android.view.View.INVISIBLE);
            //init the listview
            m_list = (CheckAbleList) findViewById (R.id.APKList);
            m_list.setChoiceMode (ListView.CHOICE_MODE_MULTIPLE);
            //create dialog process dialog
            mScanDiag = new OperationDialog (this, m_scanop, getResources().getString (R.string.scanning_init));
            mHandleDiag = new OperationDialog (this, m_installop, getResources().getString (R.string.handling_selected_package_init));
            pkgadapter = new PackageAdapter (this, R.layout.listitem, R.id.appname, R.id.apk_filepath, R.id.InstallState, R.id.APKIcon, R.id.Select, mApkList, m_list);
            m_list.setOnItemClickListener (new AdapterView.OnItemClickListener() {
                public void onItemClick (AdapterView<?> parent, View view, int position, long id) {
                    APKInfo apkinfo = mApkList.get (position);
                    //if(apkinfo.isInstalled() == true)
                        //uninstall_apk(apkinfo.pCurPkgName);
                    item_position_first = m_list.getFirstVisiblePosition();
                    item_position_selected = m_list.getSelectedItemPosition();
                    View v = m_list.getChildAt(item_position_selected - item_position_first);
                    fromtop_piexl = (v == null) ? 0 :v.getTop();
                    install_apk (apkinfo.filepath);

                }
            });
            //change dir button
            m_DirEdit = (TextView) findViewById (R.id.Dir);
            m_DirEdit.setText (" ", TextView.BufferType.NORMAL);
            m_DirEdit.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    showChooseDev();
                }
            });
            //exit button
            ImageButton hexit = (ImageButton) findViewById (R.id.Exit);
            hexit.setOnClickListener (new View.OnClickListener() {
                public void onClick (View v) {
                    main.this.finish();
                }
            });
            hexit.requestFocus();
            showChooseDev();
        }

        private BroadcastReceiver mMountReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive (Context context, Intent intent) {
                String action = intent.getAction();
                String data = intent.getDataString();
                Uri uri = intent.getData();
                String spath = uri.getPath();
                if (action == null) {
                    return;
                }
                if (mScanRoot == null) {
                    return;
                }
                if (action.equals (Intent.ACTION_MEDIA_UNMOUNTED)) {
                    int index = data.indexOf (FileListManager.STORAGE);
                    if (index >= 0) {
                        String path = data.substring (index);
                        if (path.equals (mScanRoot)) {
                            //m_list.setAdapter (null);
                            m_DirEdit.setText (" ", TextView.BufferType.NORMAL);
                            mApkList.clear();
                            pkgadapter.notifyDataSetChanged();
                        }
                    }
                    else { //exception handle
                        startScanOp();
                    }
                }
                else if (action.equals ("com.droidvold.action.MEDIA_EJECT") && !spath.equals("/dev/null")) {
                    int index = data.indexOf (FileListManager.STORAGE);
                    int index1 = data.indexOf (FileListManager.MEDIA_RW);
                    if (index >= 0) {
                        String path = data.substring (index);
                        if (path.equals (mScanRoot)) {
                            //m_list.setAdapter (null);
                            m_DirEdit.setText (" ", TextView.BufferType.NORMAL);
                            mApkList.clear();
                            pkgadapter.notifyDataSetChanged();
                        }
                    } else if (index1 >= 0) {
                        String path = data.substring (index1);
                        if (path.equals (mScanRoot)) {
                            //m_list.setAdapter (null);
                            m_DirEdit.setText (" ", TextView.BufferType.NORMAL);
                            mApkList.clear();
                            pkgadapter.notifyDataSetChanged();
                        }
                    } else { //exception handle
                        startScanOp();
                    }
                }
                else if ((action.equals (Intent.ACTION_MEDIA_MOUNTED) || action.equals ("com.droidvold.action.MEDIA_MOUNTED")) && !spath.equals("/dev/null")) {
                    int index = data.indexOf (FileListManager.STORAGE);
                    int index1 = data.indexOf (FileListManager.MEDIA_RW);
                    if (index >= 0) {
                        String path = data.substring (index);
                        if (path.equals (mScanRoot)) {
                            for (int i = 0; i < mVolumes.size(); i++) {
                                Map<String, Object> map = mVolumes.get(i);
                                if ((String)map.get(KEY_PATH) == mScanRoot) {
                                    m_DirEdit.setText ((String)map.get(KEY_NAME));
                                }
                            }
                            startScanOp();
                        }
                    } else if (index1 >= 0) {
                        String path = data.substring (index1);
                        if (path.equals (mScanRoot)) {
                            for (int i = 0; i < mVolumes.size(); i++) {
                                Map<String, Object> map = mVolumes.get(i);
                                if ((String)map.get(KEY_PATH) == mScanRoot) {
                                    m_DirEdit.setText ((String)map.get(KEY_NAME));
                                }
                            }
                            startScanOp();
                        }
                    } else {
                        startScanOp();
                    }
                }
            }
        };

        public void onResume() {
            Log.d (TAG, "[onResume]");
            m_scanop.setHandler (mainhandler);
            m_installop.setHandler (mainhandler);
            IntentFilter intentFilter = new IntentFilter (Intent.ACTION_MEDIA_MOUNTED);
            intentFilter.addAction (Intent.ACTION_MEDIA_EJECT);
            intentFilter.addAction (Intent.ACTION_MEDIA_UNMOUNTED);
            intentFilter.addAction ("com.droidvold.action.MEDIA_UNMOUNTED");
            intentFilter.addAction ("com.droidvold.action.MEDIA_MOUNTED");
            intentFilter.addAction ("com.droidvold.action.MEDIA_EJECT");
            intentFilter.addDataScheme ("file");
            if (!recvFlag) {
                registerReceiver (mMountReceiver, intentFilter);
                recvFlag = true;
            }
            super.onResume();
            if ( mScanRoot != null && mScanRoot.length() > 0 && mIsClickPathFlg) {
                showChooseDev();
            } else {
                pkgadapter.notifyDataSetChanged();
            }
        }

        public void onPause() {
            Log.d (TAG, "[onPause]");
            //disable the operation message
            m_scanop.setHandler (null);
            m_installop.setHandler (null);
            mainhandler.removeMessages (END_OPERATION);
            mainhandler.removeMessages (NEW_APK);
            mainhandler.removeMessages (HANDLE_PKG_NEXT);
            //diable dialog
            if (mScanDiag != null) {
                mScanDiag.dismiss();
            }
            if (mHandleDiag != null) {
                mHandleDiag.dismiss();
            }
            //release the wakelock
            super.onPause();
            if (recvFlag) {
                unregisterReceiver (mMountReceiver);
                recvFlag = false;
            }
        }
        public void onStop() {
            Log.d (TAG, "[onStop]");
            super.onStop();
        }
        protected void onDestroy() {
            Log.d (TAG, "[onDestroy]");
//            unregisterReceiver(mScanListener);
            if (m_configchanged == false) {
                m_scanop.stop();
                m_installop.stop();
            }
            KeepSystemAwake (false);
            super.onDestroy();
        }

        protected void KeepSystemAwake (boolean bkeep) {
            if (bkeep == true) {
                if (mScreenLock.isHeld() == false) {
                    mScreenLock.acquire();
                }
            }
            else {
                if (mScreenLock.isHeld() == true) {
                    mScreenLock.release();
                }
            }
        }

        public Handler mainhandler = new Handler() {
            public void handleMessage (Message msg) {
                // TODO Auto-generated method stub
                switch (msg.what) {
                    case END_OPERATION:
                        if (mStatus == SCAN_APKS) {
                            if (mScanDiag != null) {
                                mScanDiag.dismiss();
                            }
                            if (mApkList.size() > 0) {
                                m_list.setAdapter (pkgadapter);
                                m_list.setSelectionFromTop(item_position_selected, fromtop_piexl);
                                m_list.setVisibility (android.view.View.VISIBLE);
                                m_info.setVisibility (android.view.View.INVISIBLE);
                            }
                            else {
                                m_list.setVisibility (android.view.View.INVISIBLE);
                                m_info.setVisibility (android.view.View.VISIBLE);
                            }
                        }
                        else {
                            Log.d (TAG, "END_HANDLE_PKG");
                            if (mHandleDiag != null) {
                                mHandleDiag.dismiss();
                            }
                            pkgadapter.notifyDataSetChanged();
                        }
                        mStatus = VIEW_APKS;
                        KeepSystemAwake (false);
                        break;
                    case NEW_APK:
                        showScanDiag (msg.arg1, msg.arg2, msg.obj);
                        break;
                    case HANDLE_PKG_NEXT:
                        String hanlemsg = msg.getData().getString ("showstr");
                        showHandleDiag (hanlemsg, (int) msg.arg1 + 1, msg.arg2);
                        break;
                    case HANDLE_PKG_FAIL:
                        String failemsg = msg.getData().getString ("showstr");
                        Toast.makeText (main.this, failemsg, Toast.LENGTH_SHORT).show();
                        break;
                    default:
                        break;
                }
            }
        };

        public boolean isAppInstalled(Context context, String packageName) {
            PackageManager packageManager = context.getPackageManager();
            List<PackageInfo> pinfo = packageManager.getInstalledPackages(0);
            List<String> pName = new ArrayList<>();
            if (pinfo != null) {
                for (int i = 0; i < pinfo.size(); i++) {
                    String pn = pinfo.get(i).packageName;
                    pName.add(pn);
                }
            }
            return pName.contains(packageName);
        }

        //option menu
        protected final int MENU_INSTALL = 0;
        protected final int MENU_UNINSTALL = 1;
        protected final int MENU_SELECT_ALL = 2;
        protected final int MENU_UNSELECT_ALL = 3;
        protected final int MENU_FRESH = 4;
        protected final int MENU_ABOUT = 5;
        public boolean onCreateOptionsMenu (Menu menu) {
            //menu.add (0, MENU_INSTALL, 0, R.string.install);
            //menu.add (0, MENU_UNINSTALL, 0, R.string.uninstall);
            //menu.add (0, MENU_SELECT_ALL, 0, R.string.selectall);
            //menu.add (0, MENU_UNSELECT_ALL, 0, R.string.unselect_all);
            menu.add (0, MENU_FRESH, 0, R.string.refresh);
            menu.add (0, MENU_ABOUT, 0, R.string.about);
            return true;
        }

        private final int opInstall = 0;
        private final int opUninstall = 1;
        private int menuSelect = opInstall;

        public boolean onOptionsItemSelected (MenuItem item) {
            switch (item.getItemId()) {
                case MENU_INSTALL:
                    if (!isInstallingUnknownAppsAllowed()) {
                        //ask user to enable setting first
                        showDialogInner (DLG_UNKNOWN_APPS);
                        mIsClickPathFlg = false;
                        return true;
                    }
                    menuSelect = opInstall;
                    startHandleOp();
                    return true;
                case MENU_UNINSTALL:
                    menuSelect = opUninstall;
                    startHandleOp();
                    return true;
                case MENU_SELECT_ALL:
                    m_list.setAllItemChecked (true);
                    return true;
                case MENU_UNSELECT_ALL:
                    m_list.setAllItemChecked (false);
                    return true;
                case MENU_FRESH:
                    m_list.setAdapter (null);
                    startScanOp();
                    return true;
                case MENU_ABOUT:
                    String aboutinfo = getResources().getString (R.string.about_appInstaller);
                    aboutinfo += getResources().getString (R.string.about_version) + mVersion + " ";
                    aboutinfo += getResources().getString (R.string.about_date) + mReleaseDate + " ";
                    AlertDialog.Builder builder = new AlertDialog.Builder (this);
                    builder.setMessage (aboutinfo);
                    AlertDialog about = builder.create();
                    about.show();
                    return true;
            }
            return false;
        }

        public void onConfigurationChanged (Configuration newConfig) {
            super.onConfigurationChanged (newConfig);
        }

        //user functions
        public void showChooseDev() {
            Log.d (TAG, "[showChooseDev]");
            int num = 0;
            int devCnt = 0;
            int selid = 0;
            mVolumes = mFileListManager.getDevices();
            devCnt = mVolumes.size();
            mDevs = new String[devCnt];
            mDevStrs = new String[devCnt];

            for (int i = 0; i < devCnt; i++) {
                Map<String, Object> map = mVolumes.get(i);
                mDevs[i] = (String)map.get(KEY_PATH);
                mDevStrs[i] = (String)map.get(KEY_NAME);
            }

            for (int idx = 0; idx < devCnt; idx++) {
                if (mDevs[idx].equals (mScanRoot)) {
                    selid = idx;
                }
            }
            //show dialog to choose dialog
            new AlertDialog.Builder (main.this)
            .setTitle (R.string.alertdialog_title)
            .setSingleChoiceItems (mDevStrs, selid, new DialogInterface.OnClickListener() {
                public void onClick (DialogInterface dialog, int which) {
                    dialog.dismiss();
                    mIsClickPathFlg = true;
                    m_DirEdit.setText (mDevStrs[which]);
                    String devpath = mDevs[which] == null ? null : mDevs[which].toString();
                    if (devpath == null) {
                        Toast.makeText (main.this, "invalid dir path", Toast.LENGTH_SHORT).show();
                        return;
                    }
                    File pfile = new File (devpath);
                    if (pfile != null && pfile.isDirectory() == true) {
                        mScanRoot = devpath;
                        startScanOp();
                    }
                    else {
                        Toast.makeText (main.this, "invalid dir path", Toast.LENGTH_SHORT).show();
                    }
                }
            })
            .setOnCancelListener (new DialogInterface.OnCancelListener() {
                public void onCancel (DialogInterface dialog) {
                    if (mScanRoot == null) {
                        main.this.finish();
                    }
                }
            })
            .show();
        }


        //===================================================================
        //functions for installing and uninstalling
        public void install_apk (String apk_filepath) {
            Intent installintent = new Intent();
            installintent.setAction (Intent.ACTION_VIEW);
            installintent.setDataAndType(Uri.fromFile (new File (apk_filepath)), "application/vnd.android.package-archive");
            startActivity (installintent);
            mIsClickPathFlg = false;
        }

        public void uninstall_apk (String apk_pkgname) {
            Intent uninstallintent = new Intent();
            uninstallintent.setAction (Intent.ACTION_UNINSTALL_PACKAGE);
            uninstallintent.setData (Uri.fromParts ("package", apk_pkgname, null));
            startActivity (uninstallintent);
            mIsClickPathFlg = false;
        }

        //===================================================================
        //base class for operations thread
        class OperationThread {
                protected Handler      m_handler = null;
                protected boolean      m_bstop = false;
                protected boolean      m_bOpEnd = false;
                protected Object       m_syncobj = new Object();
                protected int          m_iOp = 0;
                public void start() { //overide it to new and start a thread
                    m_bstop = false;
                    m_bOpEnd = false;
                }

                public void stop() {
                    synchronized (m_syncobj) {
                        m_bstop = true;
                    }
                }

                public boolean isOpEnd() {
                    synchronized (m_syncobj) {
                        return  m_bOpEnd;
                    }
                }

                public void setHandler (Handler phandler) {
                    synchronized (m_syncobj) {
                        m_handler = phandler;
                    }
                }

                public void sendEndMsg() {
                    if (m_handler != null) {
                        Message endmsg = Message.obtain();
                        endmsg.what = END_OPERATION;
                        endmsg.arg1 = m_iOp;
                        m_handler.sendMessage (endmsg);
                    }
                }
        };

        class OperationDialog extends ProgressDialog {
                public boolean m_bOpStop;
                public OperationThread m_pOp;
                public CharSequence m_sInitMsg;
                public OperationDialog (Context context, OperationThread operation, CharSequence initMessage) {
                    super (context);
                    m_bOpStop = false;
                    m_pOp = operation;
                    setCancelable (false);
                    m_sInitMsg = initMessage;
                }

                public void start() {
                    //show with empty message
                    m_bOpStop = false;
                    setMessage (m_sInitMsg);
                    this.show();//first show() is invert-action to dismiss()
                    this.show();//second show() is invert-action to hide()
                }

                public void setMessage (CharSequence message) {
                    if (m_bOpStop == true) {
                        return;
                    }
                    else {
                        super.setMessage (message);
                    }
                }

                public void dismiss() {
                    hide();//first to hide() it and in start to show() it , this is let the animation  in dialog to restart
                    super.dismiss();
                }

                public boolean onKeyDown (int keyCode, KeyEvent event) {
                    if (keyCode == KeyEvent.KEYCODE_BACK || keyCode == KeyEvent.KEYCODE_POWER) {
                        m_pOp.stop();
                        setMessage ("stopping...\n");
                        m_bOpStop = true;
                    }
                    return super.onKeyDown (keyCode, event);
                }

        };

        // Check free space for installation
        private static long checkFreeSpace (String path) {
            long nSDFreeSize = 0;
            if (path != null) {
                StatFs statfs = new StatFs (path);
                long nBlocSize = statfs.getBlockSize();
                long nAvailaBlock = statfs.getAvailableBlocks();
                nSDFreeSize = nAvailaBlock * nBlocSize;
            }
            return (nSDFreeSize / (1024 * 1024));
        }

        //===================================================================
        //functions for installing and uninstalling in slient mode
        class InstallOperation extends OperationThread {
            public long []                  m_checkeditems = null;
            protected installthread         m_thread;
            private long                    m_handleitem;
            private Handler                 m_selfhandler;
            protected ApkHandleTask         m_apkhandltsk = new ApkHandleTask();
            InstallOperation() {
                super();
                m_iOp = INSTALL_APKS;
            }
            public void start() { //overide it to new and start a thread
                super.start();
                m_handleitem = 0;
                m_thread = new installthread ("multi-apk-handler");
                m_thread.start();
            }

            class installthread extends HandlerThread {
                installthread (String name) {
                    super (name);
                }
                protected void onLooperPrepared() {
                    synchronized (m_syncobj) {
                        m_selfhandler = new Handler();
                        m_selfhandler.post (m_apkhandltsk);
                        m_handleitem = 0;
                    }
                }
            }

            class ApkHandleTask implements Runnable {
                /*class PackageInstallObserver extends IPackageInstallObserver2.Stub {
                    String apkpath = null;
                    public void onPackageInstalled (String basePackageName, int returnCode, String msg, Bundle extras) {
                        Log.d (TAG, "packageInstalled " + String.valueOf (returnCode));
                        synchronized (m_syncobj) {
                            if (returnCode != 1) { //fail
                                Message endmsg = Message.obtain();
                                endmsg.what = HANDLE_PKG_FAIL;
                                Bundle data = new Bundle();
                                data.putString ("showstr", "Install " + apkpath + " fail!");
                                endmsg.setData (data);
                                m_handler.sendMessage (endmsg);
                            }
                            m_handleitem++;
                            m_selfhandler.post (m_apkhandltsk);
                        }
                    }

                    public void onUserActionRequired (Intent intent) {

                    }
                }*/

                /*class PackageDeleteObserver extends IPackageDeleteObserver.Stub {
                    String pkgpath = null;
                    public void packageDeleted (String packageName, int returnCode) {
                        Log.d (TAG, "packageDeleted " + String.valueOf (returnCode));
                        synchronized (m_syncobj) {
                            if (returnCode != PackageManager.DELETE_SUCCEEDED) {
                                Message endmsg = Message.obtain();
                                endmsg.what = HANDLE_PKG_FAIL;
                                Bundle data = new Bundle();
                                data.putString ("showstr", "Uninstall " + pkgpath + " fail!");
                                endmsg.setData (data);
                                m_handler.sendMessage (endmsg);
                            }
                            m_handleitem++;
                            m_selfhandler.post (m_apkhandltsk);
                        }
                    }
                }*/
                public void install_apk_slient (String apk_filepath) {
                    //PackageManager pm = getPackageManager();
                    //PackageInstallObserver observer = new PackageInstallObserver();
                    //observer.apkpath = apk_filepath;
                    //pm.installPackage (Uri.fromFile (new File (apk_filepath)), observer, pm.INSTALL_REPLACE_EXISTING, null);
                }
                public void uninstall_apk_slient (String apk_pkgname) {
                    /*PackageDeleteObserver observer = new PackageDeleteObserver();
                    observer.pkgpath = apk_pkgname;
                    PackageManager pm = getPackageManager();
                    if (isAppInstalled(main.this, apk_pkgname)) {
                        pm.deletePackage (apk_pkgname, observer, 0);
                    } else {
                        Toast.makeText (main.this, R.string.apk_have_not_been_installed , Toast.LENGTH_SHORT).show();
                    }*/
                }
                public void run() {
                    synchronized (m_syncobj) {
                        if ( (m_bstop == true) || m_handleitem == m_checkeditems.length) {
                            sendEndMsg();
                            m_bOpEnd = true;
                            m_thread.quit();
                            return ;
                        }
                    }
                    String hanlemsg = null;
                    int actionid = 0;
                    String actionpara = null;
                    APKInfo pinfo = mApkList.get ( (int) m_checkeditems[ (int) m_handleitem]);
                    if (pinfo != null) {
                        //if (pinfo.isInstalled()==false)
                        if (menuSelect == opInstall) {
                            actionid = 0;
                            actionpara = pinfo.filepath;
                            hanlemsg = "Installing  \"";
                        }
                        else {
                            actionid = 1;
                            actionpara = pinfo.pCurPkgName;
                            hanlemsg = "Uninstalling  \"";
                        }
                        hanlemsg += pinfo.pAppName + "\"\n";
                        synchronized (m_syncobj) {
                            if (m_handler != null) {
                                Message endmsg = Message.obtain();
                                endmsg.what = HANDLE_PKG_NEXT;
                                endmsg.arg1 = (int) m_handleitem;
                                endmsg.arg2 = m_checkeditems.length;
                                Bundle data = new Bundle();
                                data.putString ("showstr", hanlemsg);
                                endmsg.setData (data);
                                m_handler.sendMessageDelayed (endmsg, 2000); //add a delay, for systeme need time to release cache.
                            }
                        }
                        if (actionid == 0) {
                            boolean hasSpace = checkFreeSpace ("/data/app") > 50;
                            if (!hasSpace) {
                                Log.w (TAG, "no enough space for installation, force stop!");
                                Message endmsg = Message.obtain();
                                endmsg.what = HANDLE_PKG_FAIL;
                                Bundle data = new Bundle();
                                data.putString ("showstr", getResources().getString (R.string.no_space));
                                endmsg.setData (data);
                                m_handler.sendMessage (endmsg);
                                sendEndMsg();
                                m_bOpEnd = true;
                                m_thread.quit();
                                return ;
                            }
                            //install_apk_slient (actionpara);
                        }
                        else {
                            uninstall_apk_slient (actionpara);
                        }
                        Log.d (TAG, "install a singel apk end");
                    }
                    else {
                        Log.e (TAG, "got a null apkinfo, this is strange!");
                    }
                }
            }
        }

        public void startHandleOp() {
            long [] checkeditems = m_list.getCheckedItemIds();
            String pChkItems;
            boolean isChkItemInstallFlg = true;
            if (checkeditems.length == 0) {
                Toast.makeText (main.this, R.string.no_select_apks , Toast.LENGTH_SHORT).show();
            }
            else {
                KeepSystemAwake (true);
                mStatus = INSTALL_APKS;
                m_installop.m_checkeditems = checkeditems.clone();
                if (menuSelect == opUninstall) {
                    for (int i = 0; i < checkeditems.length; i++) {
                        APKInfo pinfo = mApkList.get ( (int) checkeditems[i]);
                        pChkItems = pinfo.pCurPkgName;
                        if (isAppInstalled(main.this, pChkItems))
                            continue;
                        else
                            isChkItemInstallFlg = false;
                    }
                    if (isChkItemInstallFlg)
                        mHandleDiag.start();
                } else {
                    mHandleDiag.start();
                }
                m_installop.setHandler (mainhandler);
                m_installop.start();
            }
        }

        protected void showHandleDiag (String handlemsg, int curpkg, int totalpkg) {
            String msg = getResources().getString (R.string.handling_selected_package);
            msg += String.valueOf (curpkg) + "/" + String.valueOf (totalpkg) + "\n";
            msg += handlemsg;
            mHandleDiag.setMessage (msg);
            //  mHandleDiag.show();
        }

        //===================================================================
        //functions for scanning apks
        protected void startScanOp() {
            KeepSystemAwake (true);
            mStatus = SCAN_APKS;
            showScanDiag (0, 0, null);
            mScanDiag.start();
            m_scanop.start();
            m_scanop.setHandler (mainhandler);
        }

        class ScanOperation extends OperationThread {
            protected scanthread       m_thread;
            ScanOperation() {
                //super();
                m_iOp = SCAN_APKS;
            }
            public void start() { //overide it to new and start a thread
                super.start();
                m_thread = new scanthread();
                m_thread.start();
            }

            class scanthread extends Thread {
                public void run() {
                    if (mScanRoot != null) {
                        scandir (mScanRoot);
                    }

                    synchronized (m_syncobj) {
                        sendEndMsg();
                        m_bOpEnd = true;
                    }
                }

                protected void scandir (String directory) {
                    mApkList.clear();
                    int dirs = 0, apks = 0;
                    //to scan dirs
                    ArrayList<String> pdirlist = new ArrayList<String>();
                    List<Map<String, Object>> files;
                    pdirlist.add (directory);
                    while (pdirlist.isEmpty() == false) {
                        String dirstr = pdirlist.get(0);
                        synchronized (m_syncobj) {
                            if (m_bstop == true) {
                                break;
                            }
                            dirs++;
                            if (m_handler != null) {
                                Message dirmsg = Message.obtain();
                                dirmsg.what = NEW_APK;
                                dirmsg.arg1 = dirs;
                                dirmsg.arg2 = apks;
                                dirmsg.obj = dirstr;
                                m_handler.sendMessage (dirmsg);
                            }
                        }
                        String headpath = pdirlist.remove (0);
                        File pfile = new File (headpath);
                        if (pfile.exists() == true) {
                            files = mFileListManager.getDirs(headpath, ".apk");
                            for (int i = 0; i < files.size(); i++) {
                                Map<String, Object> map = files.get(i);
                                String value = map.get(KEY_PATH).toString();
                                if (!value.endsWith(".apk")) {
                                    pdirlist.add(value);
                                } else {
                                    APKInfo apkinfo = new APKInfo (main.this, value);
                                    if (apkinfo.pCurPkgName != null) {
                                        mApkList.add(apkinfo);
                                        apks++;
                                        synchronized (m_syncobj) {
                                            if (m_handler != null) {
                                                Message apkmsg = Message.obtain();
                                                apkmsg.what = NEW_APK;
                                                apkmsg.arg1 = dirs;
                                                apkmsg.arg2 = apks;
                                                apkmsg.obj = dirstr;
                                                m_handler.sendMessage (apkmsg);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        };

    protected void showScanDiag (int dirs, int apks, Object obj) {
        String msg = getResources().getString (R.string.scanning);
        //String path = (String)obj;
        //msg += "dir : " + String.valueOf (dirs) + ", path:" + path + "\n";
        //msg += "dir:" + path + "\n";
        msg += "apk : " + String.valueOf (apks) + "\n";
        mScanDiag.setMessage (msg);
        //  mScanDiag.show();
    }

    private void showDialogInner (int id) {
        // TODO better fix for this? Remove dialog so that it gets created again
        removeDialog (id);
        showDialog (id);
    }

    @Override
    public Dialog onCreateDialog (int id, Bundle bundle) {
        switch (id) {
            case DLG_UNKNOWN_APPS:
                return new AlertDialog.Builder (this)
               .setTitle (R.string.unknown_apps_dlg_title)
               .setMessage (R.string.unknown_apps_dlg_text)
                .setNegativeButton (R.string.cancel, new DialogInterface.OnClickListener() {
                    public void onClick (DialogInterface dialog, int which) {
                    }
                })
                .setPositiveButton (R.string.settings, new DialogInterface.OnClickListener() {
                    public void onClick (DialogInterface dialog, int which) {
                        launchSettingsAppAndFinish();
                    }
                }) .create();
        }
        return null;
    }

    private void launchSettingsAppAndFinish() {
        //Create an intent to launch SettingsTwo activity
        Intent launchSettingsIntent = new Intent (Settings.ACTION_SECURITY_SETTINGS);
        startActivity (launchSettingsIntent);
    }

    private boolean isInstallingUnknownAppsAllowed() {
        return Settings.Secure.getInt (getContentResolver(),
            Settings.Secure.INSTALL_NON_MARKET_APPS, 0) > 0;
    }
}
