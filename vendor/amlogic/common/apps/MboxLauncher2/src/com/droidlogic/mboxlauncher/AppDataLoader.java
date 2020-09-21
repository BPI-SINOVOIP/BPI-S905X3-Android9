/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/

package com.droidlogic.mboxlauncher;

import android.app.ActivityManager;
import android.os.Handler;
import android.os.Message;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.LauncherActivityInfo;
import android.content.pm.LauncherApps;
import android.graphics.drawable.Drawable;
import android.util.ArrayMap;
import android.util.Log;

import java.io.File;
import java.io.FileReader;
import java.io.BufferedReader;
import java.io.FileWriter;
import java.io.BufferedWriter;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.util.Comparator;
import java.util.List;
import java.util.ArrayList;
import java.util.Collections;
import java.text.Collator;


public class AppDataLoader {
    private final static String TAG = "AppDataLoader";
    public final static String NAME = "name";
    public final static String INTENT = "intent";
    public final static String ICON = "icon";
    public final static String COMPONENT_NAME = "component name";

    public String SHORTCUT_PATH ;//= "/data/data/com.droidlogic.mboxlauncher/shortcut.cfg";
    public final static int DEFAULT_SHORTCUR_PATH = R.raw.default_shortcut;
    public final static String HOME_SHORTCUT_HEAD = "Home_Shortcut:";
    public final static String VIDEO_SHORTCUT_HEAD = "Video_Shortcut:";
    public final static String RECOMMEND_SHORTCUT_HEAD = "Recommend_Shortcut:";
    public final static String MUSIC_SHORTCUT_HEAD = "Music_shortcut:";
    public final static String LOCAL_SHORTCUT_HEAD = "Local_Shortcut:";

    private Context mContext;
    private LauncherApps mLauncherApps;
    private ActivityManager mActivityManager;
    private String str_homeShortcut;
    private String str_videoShortcut;
    private String str_recommendShortcut;
    private String str_musicShortcut;
    private String str_localShortcut;

    private String[] list_homeShortcut;
    private String[] list_videoShortcut;
    private String[] list_recommendShortcut;
    private String[] list_musicShortcut;
    private String[] list_localShortcut;

    List<ArrayMap<String, Object>> homeShortCuts = new ArrayList<ArrayMap<String, Object>>();
    List<ArrayMap<String, Object>> videoShortCuts= new ArrayList<ArrayMap<String, Object>>();
    List<ArrayMap<String, Object>> recommendShorts = new ArrayList<ArrayMap<String, Object>>();
    List<ArrayMap<String, Object>> appShortCuts = new ArrayList<ArrayMap<String, Object>>();
    List<ArrayMap<String, Object>> musicShortCuts = new ArrayList<ArrayMap<String, Object>>();
    List<ArrayMap<String, Object>> localShortCuts = new ArrayList<ArrayMap<String, Object>>();

    private boolean isLoaded = false;
    private Object mLock;

    public AppDataLoader (Context context) {
        mContext = context;
	SHORTCUT_PATH = mContext.getFilesDir() + "/shortcut.cfg";
        mLauncherApps = (LauncherApps)mContext.getSystemService(Context.LAUNCHER_APPS_SERVICE);
        mActivityManager = (ActivityManager)mContext.getSystemService(Context.ACTIVITY_SERVICE);
        mLock = ((Launcher)mContext).getLock();
    }

    public void update() {
        isLoaded = false;
        new Thread(new Runnable() {
            public void run() {
                synchronized (mLock) {
                    loadCustomApps();
                    loadShortcutList();
                    isLoaded = true;
                }
            }
        }).start();
    }

    private String[] loadCustomApps(){
        String[] list = null;
        File mFile = new File(SHORTCUT_PATH);
        if (!mFile.exists()) {
            getShortcutFromDefault(DEFAULT_SHORTCUR_PATH, SHORTCUT_PATH);
            mFile = new File(SHORTCUT_PATH);
        } else{
            try {
                BufferedReader b = new BufferedReader(new FileReader(mFile));
                if (b.read() == -1) {
                    getShortcutFromDefault(DEFAULT_SHORTCUR_PATH, SHORTCUT_PATH);
                }
                if (b != null)
                    b.close();
            } catch (IOException e) {
            }
        }

        BufferedReader br = null;
        try {
            if (mFile.length() > 10) {
                br = new BufferedReader(new FileReader(mFile));
            } else {
                //copying file error, avoid this error
                br = new BufferedReader(new InputStreamReader(mContext.getResources().openRawResource(R.raw.default_shortcut)));
                getShortcutFromDefault(DEFAULT_SHORTCUR_PATH, SHORTCUT_PATH);
            }

            String str = null;
            while ((str=br.readLine()) != null ) {
                if (str.startsWith(HOME_SHORTCUT_HEAD)) {
                    str_homeShortcut = str.replaceAll(HOME_SHORTCUT_HEAD, "");
                    list_homeShortcut = str_homeShortcut.split(";");
                } else if (str.startsWith(VIDEO_SHORTCUT_HEAD)) {
                    str_videoShortcut = str.replaceAll(VIDEO_SHORTCUT_HEAD, "");
                    list_videoShortcut = str_videoShortcut.split(";");
                }  else if (str.startsWith(RECOMMEND_SHORTCUT_HEAD)) {
                    str_recommendShortcut = str.replaceAll(RECOMMEND_SHORTCUT_HEAD, "");
                    list_recommendShortcut = str_recommendShortcut.split(";");
                }  else if (str.startsWith(MUSIC_SHORTCUT_HEAD)) {
                    str_musicShortcut = str.replaceAll(MUSIC_SHORTCUT_HEAD, "");
                    list_musicShortcut = str_musicShortcut.split(";");
                }  else if (str.startsWith(LOCAL_SHORTCUT_HEAD)) {
                    str_localShortcut = str.replaceAll(LOCAL_SHORTCUT_HEAD, "");
                    list_localShortcut= str_localShortcut.split(";");
                }
            }

        }
        catch (Exception e) {
            Log.d(TAG,""+e);
        } finally {
            try {
                if (br != null)
                    br.close();
            } catch (IOException e) {
            }
        }
        return list;
    }

    public void saveShortcut(int mode, String str_apps){
        synchronized (mLock) {
            File mFile = new File(SHORTCUT_PATH);
            if (!mFile.exists()) {
                try {
                    mFile.createNewFile();
                }
                catch (Exception e) {
                    Log.e(TAG, e.getMessage().toString());
                }
            }

            BufferedReader br = null;
            BufferedWriter bw = null;
            try {
                br = new BufferedReader(new FileReader(mFile));
                String str = null;
                List list = new ArrayList();

                while ( (str=br.readLine()) != null ) {
                    list.add(str);
                }

                if (list.size() == 0) {
                    list.add(HOME_SHORTCUT_HEAD);
                    list.add(VIDEO_SHORTCUT_HEAD);
                    list.add(RECOMMEND_SHORTCUT_HEAD);
                    list.add(MUSIC_SHORTCUT_HEAD);
                    list.add(LOCAL_SHORTCUT_HEAD);
                }
                bw = new BufferedWriter(new FileWriter(mFile));
                for ( int i = 0;i < list.size(); i++ ) {
                    if (list.get(i).toString().startsWith(parseShortcutHead(mode))) {
                        str_apps =  parseShortcutHead(mode) + str_apps;
                        bw.write(str_apps);
                    } else {
                        bw.write(list.get(i).toString());
                    }
                    bw.newLine();
                }
                bw.flush();
                bw.close();
            }
            catch (Exception e) {
                Log.d(TAG, "   " + e);
            } finally {
                try {
                    if (br != null)
                        br.close();
                } catch (IOException e) {
                }
                try {
                    if (bw != null)
                        bw.close();
                } catch (IOException e) {
                }
            }
        }
    }

    public  void getShortcutFromDefault(int srcPath, String desPath){
        File desFile = new File(desPath);
        if (!desFile.exists()) {
            try {
                desFile.createNewFile();
            }
            catch (Exception e) {
                Log.e(TAG, e.getMessage().toString());
            }
        }

        BufferedReader br = null;
        BufferedWriter bw = null;
        try {
            br = new BufferedReader(new InputStreamReader(mContext.getResources().openRawResource(srcPath)));
            String str = null;
            List list = new ArrayList();

            while ((str=br.readLine()) != null ) {
                list.add(str);
            }
            bw = new BufferedWriter(new FileWriter(desFile));
            for ( int i = 0;i < list.size(); i++ ) {
                bw.write(list.get(i).toString());
                bw.newLine();
            }
            bw.flush();
            bw.close();
        }
        catch (Exception e) {
            Log.d(TAG, "   " + e);
        } finally {
            try {
                if (br != null)
                    br.close();
            } catch (IOException e) {
            }
            try {
                if (bw != null)
                    bw.close();
            } catch (IOException e) {
            }
        }
    }

    private static final Comparator<LauncherActivityInfo> getAppNameComparator() {
        final Collator collator = Collator.getInstance();
        return new Comparator<LauncherActivityInfo>() {
            public final int compare(LauncherActivityInfo a, LauncherActivityInfo b) {
                if (a.getUser().equals(b.getUser())) {
                    int result = collator.compare(a.getLabel().toString(), b.getLabel().toString());
                    if (result == 0) {
                        result = a.getName().compareTo(b.getName());
                    }
                    return result;
                } else {
                    // TODO: Order this based on profile type rather than string compares.
                    return a.getUser().toString().compareTo(b.getUser().toString());
                }
            }
        };
    }

   private void loadShortcutList() {
        homeShortCuts.clear();
        videoShortCuts.clear();
        recommendShorts.clear();
        musicShortCuts.clear();
        appShortCuts.clear();
        localShortCuts.clear();

        final List<LauncherActivityInfo> apps = mLauncherApps.getActivityList(null, android.os.Process.myUserHandle());
        Collections.sort(apps, getAppNameComparator());
        final int iconDpi = mActivityManager.getLauncherLargeIconDensity();

        if (apps != null) {
            for (int i = 0; i < apps.size(); i++) {
                ApplicationInfo application = new ApplicationInfo();
                LauncherActivityInfo info = apps.get(i);

                application.title = info.getLabel().toString();
                application.setActivity(info.getComponentName(),
                        Intent.FLAG_ACTIVITY_NEW_TASK
                        | Intent.FLAG_ACTIVITY_RESET_TASK_IF_NEEDED);
                application.icon = info.getBadgedIcon(iconDpi);
                if (info.getComponentName().getPackageName().equals("com.android.gallery3d")
                        && application.intent.toString().contains("camera")) {
                    continue;
                }

                if (list_homeShortcut != null) {
                    for (int j = 0; j < list_homeShortcut.length; j++) {
                        if (info.getComponentName().getPackageName().equals(list_homeShortcut[j])) {
                            homeShortCuts.add(buildShortcutMap(application.title.toString(),
                                    application.intent,application.icon, application.componentName));
                            break;
                        }
                    }
                }

                if (list_videoShortcut != null) {
                    for (int j = 0; j < list_videoShortcut.length; j++) {
                        if (info.getComponentName().getPackageName().equals(list_videoShortcut[j])) {
                            videoShortCuts.add(buildShortcutMap(application.title.toString(),
                                    application.intent, application.icon, application.componentName));
                            break;
                        }
                    }
                }

                if (list_recommendShortcut != null) {
                    for (int j = 0; j < list_recommendShortcut.length; j++) {
                        if (info.getComponentName().getPackageName().equals(list_recommendShortcut[j])) {
                            recommendShorts.add(buildShortcutMap(application.title.toString(),
                                    application.intent, application.icon, application.componentName));
                            break;
                        }
                    }
                }

                if (list_musicShortcut != null) {
                    for (int j = 0; j < list_musicShortcut.length; j++) {
                        if (info.getComponentName().getPackageName().equals(list_musicShortcut[j])) {
                            musicShortCuts.add(buildShortcutMap(application.title.toString(),
                                    application.intent, application.icon, application.componentName));
                            break;
                        }
                    }
                }

                if (list_localShortcut != null) {
                    for (int j = 0; j < list_localShortcut.length; j++) {
                        if (info.getComponentName().getPackageName().equals(list_localShortcut[j])) {
                            localShortCuts.add(buildShortcutMap(application.title.toString(),
                                    application.intent, application.icon, application.componentName));
                            break;
                        }
                    }
                }

                appShortCuts.add(buildShortcutMap(application.title.toString(),
                        application.intent,application.icon, application.componentName));
                application.icon.setCallback(null);
            }
        }
        homeShortCuts.add(buildAddMap());
        videoShortCuts.add(buildAddMap());
        musicShortCuts.add(buildAddMap());
        localShortCuts.add(buildAddMap());
    }

    private ArrayMap<String, Object> buildShortcutMap(String name, Intent i, Drawable icon, ComponentName c) {
        ArrayMap<String, Object> map = new ArrayMap<String, Object>();
        map.put(NAME, name);
        map.put(INTENT, i);
        int resId = parsePackageIcon(c.getPackageName());
        if (resId == -1) {
            map.put(ICON, icon);
        } else {
            map.put(ICON, mContext.getResources().getDrawable(resId));
        }
        map.put(COMPONENT_NAME, c);
        return map;
    }

    private ArrayMap<String, Object> buildAddMap(){
        ArrayMap<String, Object> map = new ArrayMap<String, Object>();
        map.put(NAME, mContext.getResources().getString(R.string.str_add));
        map.put(INTENT, null);
        map.put(ICON, R.drawable.item_img_add);

        return map;
    }

    public List<ArrayMap<String, Object>> getShortcutList(int mode) {
        synchronized (mLock) {
            switch (mode) {
                case Launcher.MODE_HOME:
                    return homeShortCuts;
                case Launcher.MODE_VIDEO:
                    return videoShortCuts;
                case Launcher.MODE_RECOMMEND:
                    return recommendShorts;
                case Launcher.MODE_MUSIC:
                    return musicShortCuts;
                case Launcher.MODE_APP:
                    return appShortCuts;
                case Launcher.MODE_LOCAL:
                    return localShortCuts;
            }
        }
        return null;
    }

    public String getShortcutString(int mode) {
        synchronized (mLock) {
            switch (mode) {
                case Launcher.MODE_HOME:
                    return str_homeShortcut;
                case Launcher.MODE_VIDEO:
                    return str_videoShortcut;
                case Launcher.MODE_RECOMMEND:
                    return str_recommendShortcut;
                case Launcher.MODE_MUSIC:
                    return str_musicShortcut;
                case Launcher.MODE_LOCAL:
                    return str_localShortcut;
            }
        }
        return null;
    }
    public boolean isDataLoaded() {
        return isLoaded;
    }

    private String parseShortcutHead (int mode) {
        switch (mode) {
            case Launcher.MODE_HOME:
                return HOME_SHORTCUT_HEAD;
            case Launcher.MODE_VIDEO:
                return VIDEO_SHORTCUT_HEAD;
            case Launcher.MODE_RECOMMEND:
                return RECOMMEND_SHORTCUT_HEAD;
            case Launcher.MODE_MUSIC:
                return MUSIC_SHORTCUT_HEAD;
            case Launcher.MODE_LOCAL:
                return LOCAL_SHORTCUT_HEAD;
        }
        return null;
    }

    private int parsePackageIcon(String packageName){
        if (packageName.equals("com.droidlogic.FileBrower")) {
            return R.drawable.icon_filebrowser;
        } else if (packageName.equals("com.android.browser")) {
            return R.drawable.icon_browser;
        } else if (packageName.equals("com.droidlogic.appinstall")) {
            return R.drawable.icon_appinstaller;
        } else if (packageName.equals("com.android.tv.settings")) {
            return R.drawable.icon_setting;
        } else if (packageName.equals("com.droidlogic.mediacenter")){
            return R.drawable.icon_mediacenter;
        } else if (packageName.equals("com.droidlogic.otaupgrade")) {
            return R.drawable.icon_backupandupgrade;
        } else if (packageName.equals("com.android.gallery3d")) {
            return R.drawable.icon_pictureplayer;
        } else if (packageName.equals("com.droidlogic.miracast")) {
            return R.drawable.icon_miracast;
        } else if (packageName.equals("com.droidlogic.PPPoE")) {
            return R.drawable.icon_pppoe;
        } else if (packageName.equals("com.android.music")) {
            return R.drawable.icon_music;
        } else if (packageName.equals("com.android.camera2")) {
            return R.drawable.icon_camera;
        }
        return -1;
    }
}
