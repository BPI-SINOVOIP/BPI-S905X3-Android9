/******************************************************************
*
*Copyright(C) 2012 Amlogic, Inc.
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

import android.content.Context;
import android.content.SharedPreferences;
import android.os.Build;
import android.os.Environment;
import android.util.Log;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileWriter;
import java.io.IOException;
import com.amlogic.update.DownloadUpdateTask;

import android.os.storage.StorageManager;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Locale;
import java.util.List;
import java.util.Set;
import java.util.ArrayList;
import java.util.Collections;
import java.io.InputStream;
import java.io.OutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.nio.channels.FileChannel;
import com.amlogic.update.OtaUpgradeUtils;
import java.lang.reflect.Method;
import java.lang.reflect.Field;
import java.lang.reflect.Constructor;

/**
 * @ClassName PrefUtils
 * @Description TODO
 * @Date 2013-7-16
 * @Email
 * @Author
 * @Version V1.0
 */
public class PrefUtils implements DownloadUpdateTask.CheckPathCallBack{
        public static Boolean DEBUG = false;
        public static final String TAG = "OTA";
        public static final String EXTERNAL_STORAGE = "/external_storage/";
        private static final String PREFS_DOWNLOAD_FILELIST = "download_filelist";
        private static final String PREFS_UPDATE_FILEPATH = "update_file_path";
        private static final String PREFS_UPDATE_SCRIPT = "update_with_script";
        private static final String PREFS_UPDATE_FILESIZE = "update_file_size";
        private static final String PREFS_UPDATE_DESC = "update_desc";
        public static final String DEV_PATH = "/storage/external_storage";
        public static final String PREF_START_RESTORE = "retore_start";
        public static final String PREF_AUTO_CHECK = "auto_check";
        static final String FlagFile = ".wipe_record";
        public static final String DATA_UPDATE = "/data/droidota/update.zip";
        private Context mContext;
        private SharedPreferences mPrefs;

        PrefUtils ( Context context ) {
            mPrefs = context.getSharedPreferences ( "update", Context.MODE_PRIVATE );
            mContext = context;
        }

        private void setString ( String key, String Str ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putString ( key, Str );
            mEditor.commit();
        }

        private void setStringSet ( String key, Set<String> downSet ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putStringSet ( key, downSet );
            mEditor.commit();
        }

        private void setInt ( String key, int Int ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putInt ( key, Int );
            mEditor.commit();
        }

        public void setDescrib ( String desc ) {
            setString ( PREFS_UPDATE_DESC, desc );
        }

        public void clearData() {
            String filePath = getUpdatePath();
            File file = null;
            if ( filePath != null ) {
                file = new File(filePath);
                if (file.exists()) {
                    file.delete();
                }
            }
            file = new File(DATA_UPDATE);
            if (file.exists()) {
                file.delete();
            }
        }

        public String getDescri() {
            return mPrefs.getString ( PREFS_UPDATE_DESC, "" );
        }

        private void setLong ( String key, long Long ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putLong ( key, Long );
            mEditor.commit();
        }

        void setBoolean ( String key, Boolean bool ) {
            SharedPreferences.Editor mEditor = mPrefs.edit();
            mEditor.putBoolean ( key, bool );
            mEditor.commit();
        }

        public void setScriptAsk ( boolean bool ) {
            setBoolean ( PREFS_UPDATE_SCRIPT, bool );
        }

        public boolean getScriptAsk() {
            return mPrefs.getBoolean ( PREFS_UPDATE_SCRIPT, false );
        }

        void setDownFileList ( Set<String> downlist ) {
            if ( downlist.size() > 0 ) {
                setStringSet ( PREFS_DOWNLOAD_FILELIST, downlist );
            }
        }

        Set<String> getDownFileSet() {
            return mPrefs.getStringSet ( PREFS_DOWNLOAD_FILELIST, null );
        }

        boolean getBooleanVal ( String key, boolean def ) {
            return mPrefs.getBoolean ( key, def );
        }

        public void setUpdatePath ( String path ) {
            setString ( PREFS_UPDATE_FILEPATH, path );
        }

        public String getUpdatePath() {
            String path = mPrefs.getString ( PREFS_UPDATE_FILEPATH, null );
            if (path != null) {
                path = onExternalPathSwitch(path);
            }
            return path;
        }

        public long getFileSize() {
            return mPrefs.getLong ( PREFS_UPDATE_FILESIZE, 0 );
        }

        public void saveFileSize ( long fileSize ) {
            setLong ( PREFS_UPDATE_FILESIZE, fileSize );
        }

        public static Object getProperties(String key, String def) {
            String defVal = def;
            try {
                Class properClass = Class.forName("android.os.SystemProperties");
                Method getMethod = properClass.getMethod("get",String.class,String.class);
                defVal = (String)getMethod.invoke(null,key,def);
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                Log.d(TAG,"getProperty:"+key+" defVal:"+defVal);
                return defVal;
            }

        }
        static boolean isUserVer() {
            String userVer = (String)getProperties("ro.secure",null);//SystemProperties.get ( "ro.secure", "" );
            String userDebug = (String)getProperties("ro.debuggable","0");//SystemProperties.get ( "ro.debuggable", "" );
            String hideLocalUp = (String)getProperties("ro.otaupdate.local",null);//SystemProperties.get ( "ro.otaupdate.local", "" );
            if ( ( hideLocalUp != null ) && hideLocalUp.equals ( "1" ) ) {
                if ( ( userVer != null ) && ( userVer.length() > 0 ) ) {
                    return ( userVer.trim().equals ( "1" ) ) && ( userDebug.equals ( "0" ) );
                }
            }
            return false;
        }

        public static boolean getAutoCheck() {
            String auto = (String)getProperties("ro.product.update.autocheck","false");
            return ( "true" ).equals ( auto );
        }

        private ArrayList<File> getExternalStorageListOnN(){
            Class<?> volumeInfoC = null;
            Method getvolume = null;
            Method isMount = null;
            Method getType = null;
            Method getPath = null;
            List<?> mVolumes = null;
            StorageManager mStorageManager = (StorageManager)mContext.getSystemService(Context.STORAGE_SERVICE);
            ArrayList<File> devList = new ArrayList<File>();
            try {
                volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
                getvolume = StorageManager.class.getMethod("getVolumes");
                isMount = volumeInfoC.getMethod("isMountedReadable");
                getType = volumeInfoC.getMethod("getType");
                getPath = volumeInfoC.getMethod("getPath");
                mVolumes = (List<?>)getvolume.invoke(mStorageManager);

                for (Object vol : mVolumes) {
                    if (vol != null && (boolean)isMount.invoke(vol) && (int)getType.invoke(vol) == 0) {
                        devList.add((File)getPath.invoke(vol));
                        Log.d(TAG, "path.getName():" + getPath.invoke(vol));
                    }
                }
            }catch (Exception ex) {
                ex.printStackTrace();
            }finally {
                return devList;
            }
        }
        public ArrayList<File> getStorageList(boolean extern) {

            if ( Build.VERSION.SDK_INT >= 26 ) {
                Class<?>  fileListClass = null;
                Method getdev = null;
                ArrayList<File> devList = new ArrayList<File>();
                try {
                    fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                    getdev = fileListClass.getMethod("getDevices");
                    Constructor constructor =fileListClass.getConstructor(new Class[]{Context.class});
                    Object fileListObj = constructor.newInstance(mContext);
                    ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String,Object>>();
                    devices = (ArrayList<HashMap<String, Object>>)getdev.invoke(fileListObj);

                    for (HashMap<String, Object> dev : devices) {
                        Log.d(TAG,"getDevice:"+dev.get("key_name")+"getType:"+dev.get("key_type"));
                        String name = (String)dev.get("key_name");
                        if (name.equals("Local Disk") && extern) {
                            continue;
                        }
                        devList.add(new File((String)dev.get("key_path")));
                    }
                }catch (Exception ex) {
                    ex.printStackTrace();
                }finally {
                    return devList;
                }
            }else if(extern) {
                return getExternalStorageListOnN();
            }else {
                return getMainDeviceListonN();
            }
        }
        private ArrayList<File> getMainDeviceListonN(){
            Class<?> volumeInfoC = null;
            Method getvolume = null;
            Method isMount = null;
            Method getType = null;
            Method getPath = null;
            List<?> mVolumes = null;
            StorageManager mStorageManager = (StorageManager)mContext.getSystemService(Context.STORAGE_SERVICE);
            ArrayList<File> devList = new ArrayList<File>();
            try {
                volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
                getvolume = StorageManager.class.getMethod("getVolumes");
                isMount = volumeInfoC.getMethod("isMountedReadable");
                getType = volumeInfoC.getMethod("getType");
                getPath = volumeInfoC.getMethod("getPathForUser",int.class);
                mVolumes = (List<?>)getvolume.invoke(mStorageManager);

                for (Object vol : mVolumes) {
                    if (vol != null && (boolean)isMount.invoke(vol) && ((int)getType.invoke(vol) == 0 || (int)getType.invoke(vol) == 2) ) {
                        devList.add((File)getPath.invoke(vol,0));
                        Log.d(TAG, "path.getName():" + getPath.invoke(vol,0));
                    }
                }
            }catch (Exception ex) {
                ex.printStackTrace();
            }finally {
                return devList;
            }
        }
        public Object getDiskInfo(String filePath){
            StorageManager mStorageManager = (StorageManager)mContext.getSystemService(Context.STORAGE_SERVICE);
            Class<?> volumeInfoC = null;
            Class<?> deskInfoC = null;
            Method getvolume = null;
            Method getDisk = null;
            Method isMount = null;
            Method getPath = null;
            Method getType = null;
            List<?> mVolumes = null;
            try {
                volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
                deskInfoC = Class.forName("android.os.storage.DiskInfo");
                getvolume = StorageManager.class.getMethod("getVolumes");
                mVolumes = (List<?>)getvolume.invoke(mStorageManager);//mStorageManager.getVolumes();
                isMount = volumeInfoC.getMethod("isMountedReadable");
                getDisk = volumeInfoC.getMethod("getDisk");
                getPath = volumeInfoC.getMethod("getPath");
                getType = volumeInfoC.getMethod("getType");
                for (Object vol : mVolumes) {
                    if (vol != null && (boolean)isMount.invoke(vol) && ((int)getType.invoke(vol) == 0)) {
                        Object info = getDisk.invoke(vol);
                        Log.d(TAG, "getDiskInfo" +((File)getPath.invoke(vol)).getAbsolutePath());
                        if ( info != null && filePath.contains(((File)getPath.invoke(vol)).getAbsolutePath()) ) {
                            Log.d(TAG, "getDiskInfo path.getName():" +((File)getPath.invoke(vol)).getAbsolutePath());
                            return info;
                        }
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            }
            return null;

        }
        public String getTransPath(String inPath) {
            if ( Build.VERSION.SDK_INT >= 26 ) {
                Class<?>  fileListClass = null;
                Method getdev = null;
                String outPath = inPath;
                try {
                    fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                    getdev = fileListClass.getMethod("getDevices");
                    Constructor constructor =fileListClass.getConstructor(new Class[]{Context.class});
                    Object fileListObj = constructor.newInstance(mContext);
                    ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String,Object>>();
                    devices = (ArrayList<HashMap<String, Object>>)getdev.invoke(fileListObj);

                    for (HashMap<String, Object> dev : devices) {
                        String name = (String)dev.get("key_name");
                        String volName = (String)dev.get("key_path");
                        if (outPath.contains(volName)) {
                            outPath = outPath.replace(volName,name);
                        }
                    }
                }catch (Exception ex) {
                    ex.printStackTrace();
                }finally {
                    return outPath;
                }
            } else {
                return getNickName(inPath);
            }
        }
        private String getNickName(String inPath) {
            String outPath = inPath;
            String pathLast;
            String pathVol;
            int idx = -1;
            int len;
            Class<?> volumeInfoC = null;
            Method getBestVolumeDescription = null;
            Method getVolumes = null;
            Method getType = null;
            Method isMount = null;
            Method getPath = null;
            List<?> volumes = null;
            StorageManager storageManager = (StorageManager)mContext.getSystemService(Context.STORAGE_SERVICE);
            try {
                volumeInfoC = Class.forName("android.os.storage.VolumeInfo");
                getVolumes = StorageManager.class.getMethod("getVolumes");
                volumes = (List)getVolumes.invoke(storageManager);
                isMount = volumeInfoC.getMethod("isMountedReadable");
                getType = volumeInfoC.getMethod("getType");
                getPath = volumeInfoC.getMethod("getPath");
                for (Object vol : volumes) {
                    if (vol != null && (boolean)isMount.invoke(vol) && (int)getType.invoke(vol) == 0) {
                        pathVol = ((File)getPath.invoke(vol)).getAbsolutePath();
                        idx = inPath.indexOf(pathVol);
                        if (idx != -1) {
                            len = pathVol.length();
                            pathLast = inPath.substring(idx + len);
                            getBestVolumeDescription = StorageManager.class.getMethod("getBestVolumeDescription",volumeInfoC);

                            outPath = ((String)getBestVolumeDescription.invoke(storageManager,vol)) + pathLast;
                        }
                    }
                }
            } catch (Exception ex) {
                ex.printStackTrace();
            } finally {
                return outPath;
            }

        }
        private String getCanWritePath(){
            ArrayList<File> externalDevs =  getStorageList(true);
            String filePath = "";
            for ( int j = 0; (externalDevs != null) && j < externalDevs.size(); j++ ) {
                File dir = externalDevs.get(j);
                if ( dir.isDirectory() && dir.canWrite() ) {
                    filePath = dir.getAbsolutePath();
                    filePath += "/";
                    break;
                }
            }
            return filePath;
        }
        private String getAttribute(String inPath) {
            if ( Build.VERSION.SDK_INT >= 26 ) {
                Class<?>  fileListClass = null;
                Method getdev = null;
                try {
                    fileListClass = Class.forName("com.droidlogic.app.FileListManager");
                    getdev = fileListClass.getMethod("getDevices");
                    Constructor constructor =fileListClass.getConstructor(new Class[]{Context.class});
                    Object fileListObj = constructor.newInstance(mContext);
                    ArrayList<HashMap<String, Object>> devices = new ArrayList<HashMap<String,Object>>();
                    devices = (ArrayList<HashMap<String, Object>>)getdev.invoke(fileListObj);

                    for (HashMap<String, Object> dev : devices) {
                        String name = (String)dev.get("key_name");
                        String volName = (String)dev.get("key_path");
                        if (inPath.contains(volName)) {
                            String type = (String)dev.get("key_type");
                            if (type == "type_udisk") return "/udisk/";
                            else if(type == "type_sdcard") return "/sdcard/";
                        }
                    }
                }catch (Exception ex) {
                    ex.printStackTrace();
                }
                return "/cache/";
            } else {
                return getAttributeonN(inPath);
            }
        }
        private String getAttributeonN(String inPath) {
            String res ="";
            Class<?> deskInfoClass = null;
            Method isSd = null;
            Method isUsb = null;
            Object info = getDiskInfo(inPath);
            if (info == null) {
                res += "/cache/";
            } else {
                try {
                    deskInfoClass = Class.forName("android.os.storage.DiskInfo");
                    isSd = deskInfoClass.getMethod("isSd");
                    isUsb = deskInfoClass.getMethod("isUsb");
                    if ( info != null ) {
                        if ( (boolean)isSd.invoke(info) ) {
                            res += "/sdcard/";
                        }else if ( (boolean)isUsb.invoke(info) ) {
                            res += "/udisk/";
                        }else {
                            res += "/cache/";
                        }
                    } else {
                        res += "/cache/";
                    }

                } catch (Exception ex) {
                    ex.printStackTrace();
                    res += "/cache/";
                }
            }
            return res;
        }
        public boolean inLocal(String fullpath) {
            String updateFilePath = getAttribute(fullpath);
            if (updateFilePath.startsWith("/data") || updateFilePath.startsWith("/cache")
                        || updateFilePath.startsWith("/sdcard") ) {
                 return true;
            }
            return false;
        }
        public int createAmlScript(String fullpath, boolean wipe_data, boolean wipe_cache) {

            File file;
            String res = "";
            int UpdateMode = OtaUpgradeUtils.UPDATE_UPDATE;
            file = new File("/cache/recovery/command");

            try {
                File parent = file.getParentFile();
                if (file.exists()) {
                    file.delete();
                }
                if (!parent.exists()) {
                    parent.mkdirs();
                }

                if (!file.exists()) {
                    file.createNewFile();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
            res += "--update_package=";
            String updateFilePath = getAttribute(fullpath);
            res += updateFilePath;
            if (updateFilePath.startsWith("/cache/"))
                UpdateMode = OtaUpgradeUtils.UPDATE_OTA;
            res += new File(fullpath).getName();
            res += ("\n--locale=" + Locale.getDefault().toString());
            res += (wipe_data? "\n--wipe_data" : "");
            res += (wipe_cache? "\n--wipe_media" : "");

            //res += (mWipeCache.isChecked() ? "\n--wipe_cache" : "");
            try {
                FileOutputStream fout = new FileOutputStream(file);
                byte[] buffer = res.getBytes();
                fout.write(buffer);
                fout.close();
            } catch (IOException e) {
                e.printStackTrace();
                Log.d(TAG, "IOException:" + this.getClass());
            }
            return UpdateMode;
        }

        void write2File() {
            String flagParentPath = getCanWritePath();
            if ( flagParentPath.isEmpty() ) {
                return;
            }
            File flagFile = new File ( flagParentPath, FlagFile );
            if ( !flagFile.exists() ) {
                try {
                    flagFile.createNewFile();
                } catch ( IOException excep ) {
                }
            }
            if ( !flagFile.canWrite() ) {
                return;
            }

            FileWriter fw = null;
            try {
                fw = new FileWriter ( flagFile );
            } catch ( IOException excep ) {
            }
            BufferedWriter output = new BufferedWriter ( fw );
            Set<String> downfiles = mPrefs.getStringSet ( PREFS_DOWNLOAD_FILELIST,
                                    null );
            if ( ( downfiles != null ) && ( downfiles.size() > 0 ) ) {
                String[] downlist = downfiles.toArray ( new String[0] );
                for ( int i = 0; i < downlist.length; i++ ) {
                    try {
                        output.write ( downlist[i] );
                        output.newLine();
                    } catch ( IOException ex ) {
                    }
                }
            }
            try {
                output.close();
            } catch ( IOException e ) {
            }
        }

        public void copyBackup(boolean outside){
            String backupInrFile = "/data/data/com.droidlogic.otaupgrade/BACKUP";
            String backupOutFile = getCanWritePath();


            File dev = new File ( backupOutFile );
            if ( !new File(backupInrFile).exists() ) { return; }
            if ( backupOutFile.isEmpty() || dev == null || !dev.canWrite() ) {
                return;
            }
           if ( dev.isDirectory() && dev.canWrite() && !dev.getName().startsWith(".") ) {
                backupOutFile = dev.getAbsolutePath();
                backupOutFile += "/BACKUP";
                if ( !backupOutFile.equals ( "" ) ) {
                    try {
                        if ( outside )
                            copyFile ( backupInrFile, backupOutFile );
                        else
                            copyFile ( backupOutFile, backupInrFile);
                    } catch ( Exception ex ) {
                        ex.printStackTrace();
                    }
                }
            }
        }

        public void copyBKFile() {
            copyBackup(true);
        }



        public String onExternalPathSwitch(String filePath) {
            if ( filePath.contains(EXTERNAL_STORAGE) || filePath.contains(EXTERNAL_STORAGE.toUpperCase()) ) {
                String path = getCanWritePath();
                if ( path != null && !path.isEmpty() ) {
                    filePath = filePath.replace(EXTERNAL_STORAGE,path);
                }
            }
            return filePath;
        }


        public static  void copyFile ( String fileFromPath, String fileToPath ) throws Exception {

            FileInputStream fi = null;
            FileOutputStream fo = null;
            FileChannel in = null;
            FileChannel out = null;
            Log.d(TAG,"copyFile from "+fileFromPath+" to "+fileToPath);
            try {
                fi = new FileInputStream ( new File ( fileFromPath ) );
                in = fi.getChannel();
                fo = new FileOutputStream ( new File ( fileToPath ) );
                out = fo.getChannel();
                in.transferTo ( 0, in.size(), out );
            } finally {
                try{
                    fi.close();
                    fo.close();
                    in.close();
                    out.close();
                }catch(Exception ex){
                }
            }
        }

}
