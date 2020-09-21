/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
* *
This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
* *
Description: java file
*/

package com.droidlogic.mboxlauncher;

import android.os.Build;
import android.os.Environment;
import android.os.storage.DiskInfo;
import android.os.storage.StorageManager;
import android.os.storage.VolumeInfo;
import android.content.Context;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.ArrayMap;
import android.util.Log;

import android.net.wifi.WifiManager;
import android.net.wifi.WifiInfo;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.text.format.DateFormat;

import java.io.File;
import java.util.ArrayList;
import java.util.Locale;
import java.util.List;
import java.util.Calendar;
import java.util.Collections;
//import java.util.Map;

//import com.droidlogic.app.FileListManager;

public class StatusLoader {
    private final static String TAG = "StatusLoader";
    private final String STORAGE_PATH ="/storage";
    private final String SDCARD_FILE_NAME ="sdcard";
    private final String UDISK_FILE_NAME ="udisk";
    public static final String ICON ="item_icon";

    private Context mContext;
    private ConnectivityManager mConnectivityManager;
    private WifiManager mWifiManager;
    private StorageManager mStorageManager;
    //private FileListManager mFileListManager;

    //private int devCnt = 0;
    //private List<Map<String, Object>> listFiles = null;

    public StatusLoader (Context context) {
        //mFileListManager = new FileListManager(context);
        mContext = context;
        mConnectivityManager = (ConnectivityManager)mContext.getSystemService(Context.CONNECTIVITY_SERVICE);
        mWifiManager = (WifiManager)mContext.getSystemService(Context.WIFI_SERVICE);
        mStorageManager = (StorageManager)mContext.getSystemService(Context.STORAGE_SERVICE);
    }

    public  List<ArrayMap<String, Object>> getStatusData() {
        List<ArrayMap<String, Object>> list = new ArrayList<ArrayMap<String, Object>>();
        ArrayMap<String, Object> map = new ArrayMap<String, Object>();
        int wifi_level = getWifiLevel();

        if (wifi_level != -1) {
            switch (wifi_level + 1) {
                case 1:
                    map.put(ICON, R.drawable.wifi2);
                    break;
                case 2:
                    map.put(ICON, R.drawable.wifi3);
                    break;
                case 3:
                    map.put(ICON, R.drawable.wifi4);
                    break;
                case 4:
                    map.put(ICON, R.drawable.wifi5);
                    break;
                default:
                    break;
            }
            list.add(map);
        }

        if (isSdcardExist()) {
            map = new ArrayMap<String, Object>();
            map.put(ICON, R.drawable.img_status_sdcard);
            list.add(map);
        }

        if (isUdiskExist()) {
            map = new ArrayMap<String, Object>();
            map.put(ICON, R.drawable.img_status_usb);
            list.add(map);
        }

        boolean is_ethernet_on = isEthernetOn();
        if (is_ethernet_on == true) {
            map = new ArrayMap<String, Object>();
            map.put(ICON, R.drawable.img_status_ethernet);
            list.add(map);
        }

        return list;
    }

    private boolean isSdcardExist() {
        List<VolumeInfo> volumes = mStorageManager.getVolumes();
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        for (VolumeInfo vol : volumes) {
            if (vol != null && vol.isMountedReadable() && vol.getType() == VolumeInfo.TYPE_PUBLIC) {
                DiskInfo disk = vol.getDisk();
                if (disk.isSd()) {
                    return true;
                }
            }
        }
        return false;
    }

    private boolean isUdiskExist() {
        List<VolumeInfo> volumes = mStorageManager.getVolumes();
        Collections.sort(volumes, VolumeInfo.getDescriptionComparator());
        for (VolumeInfo vol : volumes) {
            if (vol != null && vol.isMountedReadable() && vol.getType() == VolumeInfo.TYPE_PUBLIC) {
                DiskInfo disk = vol.getDisk();
                if (disk.isUsb()) {
                    return true;
                }
            }
        }
        return false;
    }

    private int getWifiLevel(){
        NetworkInfo mWifi = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_WIFI);

        if (mWifi.isConnected()) {
            WifiInfo mWifiInfo = mWifiManager.getConnectionInfo();
            int wifi_rssi = mWifiInfo.getRssi();

            return WifiManager.calculateSignalLevel(wifi_rssi, 4);
        } else {
            return -1;
        }
    }
    private boolean isEthernetOn(){
        NetworkInfo info = mConnectivityManager.getNetworkInfo(ConnectivityManager.TYPE_ETHERNET);

        if (info != null && info.isConnected()) {
            return true;
        } else {
            return false;
        }
    }

    public  String getTime(){
        final Calendar c = Calendar.getInstance();
        int hour = c.get(Calendar.HOUR_OF_DAY);
        int minute = c.get(Calendar.MINUTE);

        if (!DateFormat.is24HourFormat(mContext) && hour > 12) {
            hour = hour - 12;
        }

        String time = "";
        if (hour >= 10) {
            time +=  Integer.toString(hour);
        }else {
            time += "0" + Integer.toString(hour);
        }
        time += ":";

        if (minute >= 10) {
            time +=  Integer.toString(minute);
        }else {
            time += "0" +  Integer.toString(minute);
        }

        return time;
    }

    public String getDate(){
        final Calendar c = Calendar.getInstance();
        int int_Month = c.get(Calendar.MONTH);
        String mDay = Integer.toString(c.get(Calendar.DAY_OF_MONTH));
        int int_Week = c.get(Calendar.DAY_OF_WEEK) -1;
        String str_week =  mContext.getResources().getStringArray(R.array.week)[int_Week];
        String mMonth =  mContext.getResources().getStringArray(R.array.month)[int_Month];

        String date;
        if (Locale.getDefault().getLanguage().equals("zh")) {
            date = str_week + ", " + mMonth + " " + mDay + mContext.getResources().getString(R.string.str_day);
        }else {
            date = str_week + ", " + mMonth + " " + mDay;
        }

        return date;
    }

}

