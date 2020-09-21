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

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.Map;

import android.content.res.AssetManager;
import android.content.Context;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.res.Resources;
import android.graphics.drawable.Drawable;
import android.os.Handler;
import android.os.Message;
import android.os.storage.StorageManager;
//import android.os.storage.VolumeInfo;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.CheckBox;
import android.widget.CompoundButton;
import android.widget.ImageView;
import android.widget.ListView;
import android.widget.TextView;
import android.os.UserHandle;
import com.droidlogic.app.FileListManager;

class APKInfo extends Object {
        static protected PackageManager pkgmgr = null;
        static protected FileListManager mFileListManager = null;
        Class<?> cls;
        Object mAssetMgr;

        APKInfo (Context pcontext, String apkpath) {
            pkgmgr = pcontext.getPackageManager();
            PackageInfo pPkgInfo = pkgmgr.getPackageArchiveInfo (apkpath, PackageManager.GET_ACTIVITIES);
            if (pPkgInfo != null) {
                //get package's name in system
                String[] curpkgnames = pkgmgr.canonicalToCurrentPackageNames (new String[] {pPkgInfo.packageName});
                if ( (curpkgnames != null) && (curpkgnames.length > 0) && (curpkgnames[0] != null)) {
                    pCurPkgName = curpkgnames[0];
                }
                else {
                    pCurPkgName = pPkgInfo.packageName;
                }
                Resources pPkgRes = null;
                getAssetManagerInstance();
                if (0 != addAssetPath (apkpath)) {
                    pPkgRes = new Resources ((AssetManager) mAssetMgr, pcontext.getResources().getDisplayMetrics(), pcontext.getResources().getConfiguration());
                }
                if (pPkgRes != null) {
                    getApplicationName_Internal (pPkgRes, pPkgInfo);
                    getApkIcon_Internal (pPkgRes, pPkgInfo);
                    filepath = apkpath;
                }
                assetManagerClose();
            }
        }

        private int addAssetPath(String apkpath) {
            try {
                Method mAddAssetPath = cls.getMethod("addAssetPath", String.class);
                int count = (int) mAddAssetPath.invoke(mAssetMgr, apkpath);
                return count;
            } catch (Exception e) {
                e.printStackTrace();
            }
            return 0;
        }

        private void assetManagerClose() {
            try {
                Method mAddAssetPath = cls.getMethod("close");
                mAddAssetPath.invoke(mAssetMgr);
            } catch (Exception e) {
                e.printStackTrace();
            }
        }

        private Object getAssetManagerInstance() {
            try {
                cls= Class.forName("android.content.res.AssetManager");
                mAssetMgr = cls.newInstance();
                return mAssetMgr;
            } catch (Exception e) {
                e.printStackTrace();
            }
            return null;
        }
        public boolean beValid() {
            if (filepath == null) {
                return false;
            }
            else {
                return true;
            }
        }

        public String filepath;
        public String  pCurPkgName = null;
        public CharSequence  pAppName = null;
        public Drawable pAppIcon = null;
        public boolean bIsinstalled = false;

        public boolean checkInstalled() {
            ApplicationInfo appinfo = null;
            try {
                appinfo = pkgmgr.getApplicationInfo (pCurPkgName, PackageManager.GET_META_DATA);
            }
            catch (NameNotFoundException e) {
                appinfo = null;
            }
            if (appinfo == null) {
                bIsinstalled = false;
            }
            else {
                bIsinstalled = true;
            }
            return bIsinstalled;
        }
        public boolean isInstalled() {
            return bIsinstalled;
        }
        public CharSequence getApplicationName() {
            return pAppName;
        }
        public Drawable getApkIcon() {
            return pAppIcon;
        }

        public CharSequence getApplicationName_Internal (Resources PkgRes, PackageInfo PkgInfo) {
            if (pAppName == null) {
                if (PkgRes != null) {
                    try {
                        pAppName = PkgRes.getText (PkgInfo.applicationInfo.labelRes);
                    }
                    catch (Resources.NotFoundException resnotfound) {
                        pAppName = pCurPkgName;
                    }
                }
                else {
                    pAppName = pCurPkgName;
                }
            }
            return pAppName;
        }

        public Drawable getApkIcon_Internal (Resources PkgRes, PackageInfo PkgInfo) {
            if (pAppIcon == null) {
                if (PkgRes != null) {
                    try {
                        pAppIcon = PkgRes.getDrawable (PkgInfo.applicationInfo.icon);
                    }
                    catch (Resources.NotFoundException resnotfound) {
                        pAppIcon = pkgmgr.getApplicationIcon (PkgInfo.applicationInfo);
                    }
                }
                else {
                    pAppIcon = pkgmgr.getApplicationIcon (PkgInfo.applicationInfo);
                }
            }
            return pAppIcon;
        }

}


public class PackageAdapter extends BaseAdapter {

        protected int m_Layout_APKListItem;
        protected int m_TextView_AppName;
        protected int m_TextView_FileName;
        protected int m_ImgView_APPIcon;
        protected int m_CheckBox_InstallState;
        protected int m_CheckBox_SelState;
        private   LayoutInflater mInflater;
        ///protected ArrayList<APKInfo>  m_apklist = null;
        protected List<APKInfo>  m_apklist = null;
        protected ListView m_list = null;
        ///private StorageManager mStorageManager;
        private FileListManager mFileListManager;

        PackageAdapter (Context context, int Layout_Id, int text_app_id, int text_filename_id, int checkbox_id, int img_appicon_id, int checkbox_sel_id, ArrayList<APKInfo> apklist, ListView list) {
            ///mStorageManager = (StorageManager)context.getSystemService(Context.STORAGE_SERVICE);
            mFileListManager = new FileListManager(context);
            mInflater = LayoutInflater.from (context);
            m_Layout_APKListItem = Layout_Id;
            m_TextView_FileName = text_filename_id;
            m_TextView_AppName = text_app_id;
            m_CheckBox_InstallState = checkbox_id;
            m_ImgView_APPIcon = img_appicon_id;
            m_CheckBox_SelState = checkbox_sel_id;
            m_apklist = apklist;
            m_list = list;
        }

        public int getCount() {
            if (m_apklist != null) {
                return m_apklist.size();
            }
            else {
                return 0;
            }
        }

        public Object getItem (int position) {
            if (m_apklist != null) {
                return m_apklist.get (position);
            }
            else {
                return null;
            }
        }

        public long getItemId (int position) {
            return position;
        }


        class SelStateListener implements CompoundButton.OnCheckedChangeListener {
                int PosInList = 0;
                SelStateListener (int pos) {
                    PosInList = pos;
                }

                public void onCheckedChanged (CompoundButton buttonView, boolean isChecked) {
                    if (m_list.isItemChecked (PosInList) != isChecked) {
                        m_list.setItemChecked (PosInList, isChecked);
                    }
                }
        }

        private String getTransPath(String inPath) {
            String outPath = inPath;
            String pathLast;
            String pathVol;
            int idx = -1;
            int len;
            return outPath;
        }

        public View getView (int position, View convertView, ViewGroup parent) {
            View layoutview = null;
            if (convertView == null) {
                layoutview = mInflater.inflate (m_Layout_APKListItem, parent, false);
            }
            else {
                layoutview = convertView;
            }
            TextView FileName = (TextView) layoutview.findViewById (m_TextView_FileName);
            TextView AppName = (TextView) layoutview.findViewById (m_TextView_AppName);
            CheckBox InstallState = (CheckBox) layoutview.findViewById (m_CheckBox_InstallState);
            ImageView Appicon = (ImageView) layoutview.findViewById (m_ImgView_APPIcon);
            CheckBox SelState = (CheckBox) layoutview.findViewById (m_CheckBox_SelState);
            SelState.setOnCheckedChangeListener (new SelStateListener (position));
            APKInfo pinfo = (APKInfo) getItem (position);
            FileName.setText (getTransPath(pinfo.filepath));
            AppName.setText (pinfo.getApplicationName());
            InstallState.setChecked (pinfo.checkInstalled());
            Appicon.setImageDrawable (pinfo.getApkIcon());
            SelState.setChecked (m_list.isItemChecked (position));
            return layoutview;
        }

        public boolean hasStableIds() {
            return true;
        }

}
