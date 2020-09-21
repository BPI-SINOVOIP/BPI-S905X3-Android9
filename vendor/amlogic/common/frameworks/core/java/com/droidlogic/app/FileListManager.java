/*
 * Copyright (C) 2009 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *  @author   XiaoLiang.Wang
 *  @version  1.0
 *  @date     2017/09/20
 *  @par function description:
 *  - 1 get amlogic volume files list
 */

package com.droidlogic.app;

import android.content.Context;
import android.util.Log;
import android.os.Environment;
import android.os.SystemProperties;
import android.os.storage.StorageManager;

import java.io.File;
import java.io.FileFilter;
import java.lang.reflect.Method;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.StringTokenizer;
import java.util.NoSuchElementException;
import java.lang.String;

import android.os.Bundle;
import android.os.HwBinder;
import android.os.Parcel;
import android.os.Parcelable;
import android.os.RemoteException;

import com.droidlogic.app.SystemControlManager;

public class FileListManager {
    private String TAG = "FileListManager";
    private boolean mDebug = false;
    private Context mContext;

    private StorageManager mStorageManager;
    private static SystemControlManager mSystemControl;
    //private IDroidVold mDroidVold;

    public static final String STORAGE = "/storage";
    public static final String MEDIA_RW = "/mnt/media_rw";
    public static final String NAND = Environment.getExternalStorageDirectory().getPath();//storage/emulated/0

    private static String ISOpath = null;
    private static final String iso_mount_dir_s = "/mnt/loop";

    private List<Map<String, Object>> mListDev = new ArrayList<Map<String, Object>>();
    private List<Map<String, Object>> mListFile = new ArrayList<Map<String, Object>>();
    private List<Map<String, Object>> mListDir = new ArrayList<Map<String, Object>>();
    private Map<String, Object> mMap;
    //key for map
    public static final String KEY_NAME = "key_name";
    public static final String KEY_PATH = "key_path";
    public static final String KEY_TYPE = "key_type";
    public static final String KEY_DATE = "key_date";
    public static final String KEY_SIZE = "key_size";
    public static final String KEY_SELE = "key_sele";
    public static final String KEY_RDWR = "key_rdwr";

    //type for storage
    public static final String TYPE_NAND = "type_nand";
    public static final String TYPE_UDISK = "type_udisk";
    public static final String TYPE_SDCARD = "type_sdcard";

    //selected flag
    public static final String SELE_NO = "sele_no";
    public static final String SELE_YES = "sele_yes";

    /* file type extensions */
    //video
    private static final String video_extensions = "3gp,asf,avi,dat,divx,f4v,flv,h264,lst,m2ts,m4v,mkv,mp2,mp4,mov,mpe,mpeg,mpg,mts,rm,rmvb,ts,tp,mvc,vc1,vob,wm,wmv,webm,m2v,pmp,bit,h265,3g2,mlv,hm10,ogm,vp9,trp,bin,ivf";

    //music
    private static final String[] music_extensions = {
        ".mp3", ".wma", ".m4a", ".aac", ".ape", ".mp2", ".ogg", ".flac", ".alac", ".wav",
        ".mid", ".xmf", ".mka", ".aiff", ".aifc", ".aif", ".pcm", ".adpcm"
    };

    //photo
    private static final String[] photo_extensions = {
        ".jpg", ".jpeg", ".bmp", ".tif", ".tiff", ".png", ".gif", ".giff", ".jfi", ".jpe", ".jif",
        ".jfif", ".mpo", ".webp", ".3dg", "3dp"
    };

    private static final String[] plain_extensions = {".txt",".c",".cpp",".java",",conf",".h",
        ".log",".rc"
    };

    public FileListManager(Context context) {
        mContext = context;
        mDebug = false;
        mStorageManager = (StorageManager) mContext.getSystemService(Context.STORAGE_SERVICE);
        mSystemControl =  SystemControlManager.getInstance();
        checkDebug();
    }

    private void checkDebug() {
        if (SystemProperties.getBoolean("sys.filelistanager.debug", false)) {
            mDebug = true;
        }
    }

    public static boolean isVideo(String filename) {
        String name = filename.toLowerCase();
        String videos[] = video_extensions.split(",");
        for (String ext : videos) {
            if (name.endsWith(ext))
                return true;
        }
        return false;
    }

    public static boolean isMusic(String filename) {
        String name = filename.toLowerCase();
        for (String ext : music_extensions) {
            if (name.endsWith(ext))
                return true;
        }
        return false;
    }

    public static boolean isPhoto(String filename) {
        String name = filename.toLowerCase();
        for (String ext : photo_extensions) {
            if (name.endsWith(ext))
                return true;
        }
        return false;
    }

    public static boolean isApk(String filename) {
        String name = filename.toLowerCase();
        if (name.endsWith(".apk"))
            return true;
        return false;
    }

    public static boolean isHtm(String filename) {
        String name = filename.toLowerCase();
        if (name.endsWith(".htm") || name.endsWith(".shtml") || name.endsWith(".bin"))
            return true;
        return false;
    }

    public static boolean isPdf(String filename) {
        String name = filename.toLowerCase();
        if (name.endsWith(".pdf"))
            return true;
        return false;
    }

    public static boolean isPlain(String filename) {
        String name = filename.toLowerCase();
        for (String ext : plain_extensions) {
            if (name.endsWith(ext))
                return true;
        }
        return false;
    }

    private static void mount(String path) {
        mSystemControl.loopMountUnmount(false, path);
        mSystemControl.loopMountUnmount(true, path);
    }

    public static boolean isISOFile (File file) {
        String fname = file.getName();
        String sname = ".iso";
        if (fname == "") {
            return false;
        }
        if (file.isFile() && fname.toLowerCase().endsWith (sname)) {
            return true;
        }
        return false;
    }

    private static boolean isHasDir(File[] files, String name) {
       for (File file : files) {
           if (name != null && name.equals(file.getName()) && file.isDirectory())
               return true;
       }
       return false;
    }

    public static boolean isBDFile(File file) {
        if (file.isDirectory()) {
            File[] rootFiles = file.listFiles();
            if (rootFiles != null && rootFiles.length >= 1 && isHasDir(rootFiles, "BDMV")) {
                File bdDir = new File(file.getPath(), "BDMV");
                String[] files = bdDir.list();
                ArrayList<String> names = new ArrayList<String>();
                for (int i = 0; i < files.length; i++)
                    names.add(files[i]);
                if (names.contains("index.bdmv") && names.contains("PLAYLIST")
                    && names.contains("CLIPINF") && names.contains("STREAM"))
                    return true;
              }
          } else if (isISOFile(file)) {
              ISOpath = file.getPath();
              mount(ISOpath);
              File isofile = new File(iso_mount_dir_s);
              if (isofile.exists() && isofile.isDirectory()) {
                  File[] rootFiles = isofile.listFiles();
                  if (rootFiles != null && rootFiles.length >= 1 && isHasDir(rootFiles, "BDMV")) {
                      File bdfiles = new File(iso_mount_dir_s, "BDMV");
                      String[] bdmvFiles = bdfiles.list();
                      ArrayList<String> names = new ArrayList<String>();
                      for (int i = 0; i < bdmvFiles.length; i++)
                          names.add(bdmvFiles[i]);
                      if (names.contains("index.bdmv") && names.contains("PLAYLIST")
                          && names.contains("CLIPINF") && names.contains("STREAM")) {
                          return true;
                      }
                  }
              }
          }
          return false;
      }

    public static String CheckMediaType(File file){
        String typeStr="application/*";
        String filename = file.getName();

        if (isVideo(filename))
            typeStr = "video/*";
        else if (isMusic(filename))
            typeStr = "audio/*";
        else if (isPhoto(filename))
            typeStr = "image/*";
        else if (isApk(filename))
            typeStr = "application/vnd.android.package-archive";
        else if (isHtm(filename))
            typeStr = "text/html";
        else if (isPdf(filename))
            typeStr = "application/pdf";
        else if (isPlain(filename))
            typeStr = "text/plain";
        else {
            typeStr = "application/*";
        }
        return typeStr;
    }

    public class MyFilter implements FileFilter {
        private String extensions;
        public MyFilter (String extensions) {
            this.extensions = extensions;
        }
        public boolean accept (File file) {
            StringTokenizer st = new StringTokenizer (this.extensions, ",");
            if (file.isDirectory()) {
                return true;
            }
            String name = file.getName();
            if (st.countTokens() == 1) {
                String str = st.nextToken();
                String filenamelowercase = name.toLowerCase();
                return filenamelowercase.endsWith (str);
            } else {
                int index = name.lastIndexOf (".");
                if (index == -1) {
                    return false;
                }
                else if (index == (name.length() - 1)) {
                    return false;
                }
                else {
                    //for(int i = 0; i<st.countTokens(); i++)
                    while (st.hasMoreElements()) {
                        String extension = st.nextToken();
                        if (extension.equals (name.substring (index + 1).toLowerCase())) {
                            return true;
                        }
                    }
                    return false;
                }
            }
        }
    }

    public List<Map<String, Object>> getDevices() {
        Map<String, Object> map;
        //local disk (nand)
        mListDev.clear();
        File file = new File(NAND);
        if (file != null && file.exists() && file.isDirectory() && file.listFiles() != null && file.listFiles().length > 0) {
            map = new HashMap<String, Object>();
            map.put(KEY_NAME, "Local Disk");
            map.put(KEY_PATH, NAND);
            map.put(KEY_DATE, 0);
            map.put(KEY_SIZE, 1);
            map.put(KEY_SELE, SELE_NO);
            map.put(KEY_TYPE, TYPE_NAND);
            map.put(KEY_RDWR, null);

            if (mDebug) {
                Log.i(TAG, "[getDevices]nand path:" + NAND);
            }
            mListDev.add(map);
        }

        //external storage
        Class<?> volumeInfoClazz = null;
        Method getDescriptionComparator = null;
        Method getBestVolumeDescription = null;
        Method getVolumes = null;
        Method isMountedReadable = null;
        Method getType = null;
        Method getPath = null;
        Method getDisk = null;
        List<?> volumes = null;
        try {
            volumeInfoClazz = Class.forName("android.os.storage.VolumeInfo");
            getDescriptionComparator = volumeInfoClazz.getMethod("getDescriptionComparator");
            getBestVolumeDescription = StorageManager.class.getMethod("getBestVolumeDescription", volumeInfoClazz);
            getVolumes = StorageManager.class.getMethod("getVolumes");
            isMountedReadable = volumeInfoClazz.getMethod("isMountedReadable");
            getType = volumeInfoClazz.getMethod("getType");
            getPath = volumeInfoClazz.getMethod("getPath");
            getDisk = volumeInfoClazz.getMethod("getDisk");
            volumes = (List<?>)getVolumes.invoke(mStorageManager);
            //Collections.sort(volumes, getDescriptionComparator.invoke());

            for (Object vol : volumes) {
                if (vol != null && (boolean)isMountedReadable.invoke(vol) && (int)getType.invoke(vol) == 0) {
                    File pathFile = (File)getPath.invoke(vol);
                    String path = pathFile.getAbsolutePath();
                    String name = (String)getBestVolumeDescription.invoke(mStorageManager, vol);

                    map = new HashMap<String, Object>();
                    map.put(KEY_NAME, name);
                    map.put(KEY_PATH, path);
                    map.put(KEY_DATE, 0);
                    map.put(KEY_SIZE, 1);
                    map.put(KEY_SELE, SELE_NO);
                    map.put(KEY_RDWR, null);
                    /*DiskInfo disk = (DiskInfo)getDisk.invoke(vol);
                    if (disk.isUsb()) {
                        map.put(KEY_TYPE, TYPE_UDISK);
                    }
                    else */{
                        map.put(KEY_TYPE, TYPE_SDCARD);
                    }

                    if (mDebug) {
                        Log.i(TAG, "[getDevices]volume path:" + path);
                    }
                    mListDev.add(map);
                }
            }
        } catch (Exception ex) {
            Log.e(TAG, "[getDevices]Exception ex:" + ex);
            ex.printStackTrace();
        }

        //check storage
        file = new File(STORAGE);
        if (file != null && file.exists() && file.isDirectory()) {
            // TODO: needn't check right now
        }

        //check /mnt/media_rw/
        file = new File(MEDIA_RW);
        if (file != null && file.exists() && file.isDirectory()) {
            Map<String, Object> maptmp;
            boolean skipFlag = false;
            File[] fileList = file.listFiles();
            if (fileList != null && fileList.length > 0) {
                for (File filetmp : fileList) {
                    skipFlag = false;
                    String path = filetmp.getAbsolutePath();
                    String name = filetmp.getName();
                    if (mDebug) {
                        Log.i(TAG, "[getDevices]name:" + name + ", path:" + path);
                    }
                    if (mListDev != null && !mListDev.isEmpty()) {
                        for (int i = 0; i < mListDev.size(); i++) {
                            //get uuid to compare with list files' name
                            String uuid = null;
                            maptmp = mListDev.get(i);
                            if (maptmp != null) {
                                String pathtmp = (String) maptmp.get(KEY_PATH);
                                if (pathtmp != null) {
                                    int idx = pathtmp.lastIndexOf("/");
                                    if (idx >= 0) {
                                        uuid = pathtmp.substring(idx + 1);
                                    }
                                }
                            }

                            if (mDebug) {
                                Log.i(TAG, "[getDevices]uuid:" + uuid);
                            }
                            if (uuid != null && uuid.equals(name)) {
                                skipFlag = true;
                                break;
                            }
                        }
                    }

                    if (false) {
                        skipFlag = true;//reset skip flag
                        path = "/storage/" + name;
                        map = new HashMap<String, Object>();
                        map.put(KEY_NAME, name);
                        map.put(KEY_PATH, path);
                        map.put(KEY_DATE, 0);
                        map.put(KEY_SIZE, 1);
                        map.put(KEY_SELE, SELE_NO);
                        map.put(KEY_RDWR, null);
                        int diskFlag = 0;
                        Log.i(TAG, "getDevices() diskFlag:" + diskFlag);
                        if (diskFlag == 8) {
                            map.put(KEY_TYPE, TYPE_UDISK);
                        }
                        else if (diskFlag == 4){
                            map.put(KEY_TYPE, TYPE_SDCARD);
                        }
                        if (mDebug) {
                            Log.i(TAG, "[getDevices]/mnt/media_rw path:" + path);
                        }
                        mListDev.add(map);
                    }
                }
            }
        }

        if (mDebug) {
            Log.i(TAG, "[getDevices]====start print list==========================");
            if (mListDev != null && !mListDev.isEmpty()) {
                Map<String, Object> maptmp;
                int listSize = mListDev.size();
                for (int i = 0; i < listSize; i++) {
                    maptmp = mListDev.get(i);
                    if (maptmp != null) {
                        String nametmp = (String) maptmp.get(KEY_NAME);
                        String pathtmp = (String) maptmp.get(KEY_PATH);
                    }
                }
            }
            Log.i(TAG, "[getDevices]====end print list==========================");
        }

        return mListDev;
    }

    public List<Map<String, Object>> getDirs(String directory, String strs) {
        Map<String, Object> map;
        String extensions = video_extensions;
        mListDir.clear();
        File pfile = new File(directory);
        File[] files = null;
        if (strs.indexOf(".") == 0) {
            files = pfile.listFiles(new MyFilter(strs));
        } else if (strs.equals("video")) {
            files = pfile.listFiles(new MyFilter(extensions));
        }
        if (files != null && (files.length > 0)) {
            for (int i = 0; i < files.length; i++) {
                String pathtmp = files[i].getAbsolutePath();
                String nametmp = files[i].getName();
                String str = null;
                File tempF = files[i];
                str = files[i].toString();
                map = new HashMap<String, Object>();
                map.put(KEY_NAME, nametmp);
                map.put(KEY_PATH, pathtmp);
                map.put(KEY_DATE, 0);
                map.put(KEY_SIZE, 1);
                map.put(KEY_SELE, SELE_NO);
                map.put(KEY_RDWR, null);
                map.put(KEY_TYPE, null);
                if (mDebug) {
                    Log.i(TAG, "[getDirs]volume pathtmp:" + pathtmp);
                }
                if (strs == ".apk") {
                    mListDir.add(map);
                } else if (strs == "video" && (tempF.isFile() && !isISOFile(tempF))) {
                    mListDir.add(map);
                } else if (strs == "video") {
                    mListDir.add(map);
                }
            }
        }
        return mListDir;
    }

    public List<Map<String, Object>> getFiles(String path) {
        Map<String, Object> map;
        mListFile.clear();
        File file = new File(path);
        if (file != null && file.exists() && file.isDirectory()) {
            File[] fileList = file.listFiles();
            if (fileList != null && fileList.length > 0) {
                for (File filetmp : fileList) {
                    String pathtmp = filetmp.getAbsolutePath();
                    String nametmp = filetmp.getName();
                    map = new HashMap<String, Object>();
                    map.put(KEY_NAME, nametmp);
                    map.put(KEY_PATH, pathtmp);
                    map.put(KEY_DATE, 0);
                    map.put(KEY_SIZE, 1);
                    map.put(KEY_SELE, SELE_NO);
                    map.put(KEY_RDWR, null);
                    map.put(KEY_TYPE, null);
                    if (mDebug) {
                        Log.i(TAG, "[getFiles]volume pathtmp:" + pathtmp);
                    }
                    mListFile.add(map);
                }
            }
        }
        return mListFile;
    }
}
