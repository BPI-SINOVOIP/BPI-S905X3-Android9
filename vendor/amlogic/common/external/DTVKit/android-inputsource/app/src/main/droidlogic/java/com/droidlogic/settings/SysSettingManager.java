package com.droidlogic.settings;

import android.util.Log;
import android.text.TextUtils;
import android.content.Context;

import java.lang.reflect.Method;
import java.util.Map;
import java.util.HashMap;
import java.util.List;
import java.util.ArrayList;
import java.io.File;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.FileListManager;

import org.dtvkit.inputsource.R;

public class SysSettingManager {

    private static final String TAG = "SysSettingManager";
    private static final boolean DEBUG = true;

    public static final String PVR_DEFAULT_PATH = "/data/data/org.dtvkit.inputsource";

    protected SystemControlManager mSystemControlManager;
    private FileListManager mFileListManager;
    private Context mContext;

    public SysSettingManager(Context context) {
        mContext = context;
        mSystemControlManager = SystemControlManager.getInstance();
        mFileListManager = new FileListManager(context);
    }

    public String readSysFs(String sys) {
        String result = mSystemControlManager.readSysFs(sys);
        if (DEBUG) {
            Log.d(TAG, "readSysFs sys = " + sys + ", result = " + result);
        }
        return result;
    }

    public boolean writeSysFs(String sys, String val) {
        return mSystemControlManager.writeSysFs(sys, val);
    }

    public String getVideoFormatFromSys() {
        String result = "";
        String height = readSysFs(ConstantManager.SYS_HEIGHT_PATH);
        String frameFormat = parseFrameFormatStrFromDi0Path();
        if (!TextUtils.isEmpty(height) && !"NA".equals(height)) {
            result = height + frameFormat;
        }
        if (DEBUG) {
            Log.d(TAG, "getVideoFormatFromSys result = " + result);
        }
        return result;
    }

    public String getVideodecodeInfo() {
        String result = "";
        result = readSysFs(ConstantManager.SYS_VIDEO_DECODE_PATH);
        if (DEBUG) {
            Log.d(TAG, "getVideodecodeInfo result = " + result);
        }
        return result;
    }

    public int parseWidthFromVdecStatus(String vdecInfo) {
        int result = 0;
        String temp = parseMatchedInfoFromVdecStatus(ConstantManager.SYS_VIDEO_DECODE_VIDEO_WIDTH_PREFIX, ConstantManager.SYS_VIDEO_DECODE_VIDEO_WIDTH_SUFFIX, vdecInfo);
        if (!TextUtils.isEmpty(temp) && TextUtils.isDigitsOnly(temp)) {
            result = Integer.valueOf(temp);
        }
        if (DEBUG) {
            Log.d(TAG, "parseWidthFromVdecStatus result = " + result);
        }
        return result;
    }

    public int parseHeightFromVdecStatus(String vdecInfo) {
        int result = 0;
        String temp = parseMatchedInfoFromVdecStatus(ConstantManager.SYS_VIDEO_DECODE_VIDEO_HEIGHT_PREFIX, ConstantManager.SYS_VIDEO_DECODE_VIDEO_HEIGHT_SUFFIX, vdecInfo);
        if (!TextUtils.isEmpty(temp) && TextUtils.isDigitsOnly(temp)) {
            result = Integer.valueOf(temp);
        }
        if (DEBUG) {
            Log.d(TAG, "parseHeightFromVdecStatus result = " + result);
        }
        return result;
    }

    public float parseFrameRateStrFromVdecStatus(String vdecInfo) {
        float result = 0f;
        String temp = parseMatchedInfoFromVdecStatus(ConstantManager.SYS_VIDEO_DECODE_VIDEO_FRAME_RATE_PREFIX, ConstantManager.SYS_VIDEO_DECODE_VIDEO_FRAME_RATE_SUFFIX, vdecInfo);
        try {
            result = Float.valueOf(temp);
        } catch (Exception e) {
            Log.e(TAG, "parseFrameRateStrFromVdecStatus Exception = " + e.getMessage());
        }
        if (DEBUG) {
            Log.d(TAG, "parseFrameRateStrFromVdecStatus result = " + result);
        }
        return result;
    }

    private String parseMatchedInfoFromVdecStatus(String startStr, String endStr, String value) {
        /*vdec_status example:
            vdec channel 0 statistics:
              device name : ammvdec_mpeg12
              frame width : 720
             frame height : 576
               frame rate : 25 fps
                 bit rate : 0 kbps
                   status : 6
                frame dur : 3840
               frame data : 12 KB
              frame count : 426441
               drop count : 0
            fra err count : 62
             hw err count : 0
               total data : 2742089 KB
            ratio_control : 9000
        */
        String result = "";
        if (!TextUtils.isEmpty(startStr) && !TextUtils.isEmpty(endStr) && !TextUtils.isEmpty(value)) {
            int start = value.indexOf(startStr);//example:"frame rate : "
            int end = value.indexOf(endStr, start);//example:" fps"
            //deal diffrent next line symbol
            if (start != -1 && end != -1) {
                String sub = value.substring(start, end);
                if (sub != null) {
                    byte[] byteValue = sub.getBytes();
                    if (byteValue != null && byteValue.length >= 2) {
                        byte temp1 = byteValue[byteValue.length - 1];
                        byte temp2 = byteValue[byteValue.length - 2];
                        if ('\r' == temp2 && '\n' == temp1) {
                            end = end -2;
                        } else if ('\r' != temp2 && '\n' == temp1) {
                            end = end -1;
                        }
                    }
                }
            }
            int preLength = startStr.length();//example:"frame rate : "
            if (start != -1 && end != -1 && (start + preLength) < end) {
                String sub = value.substring(start + preLength, end);
                if (sub != null && sub.length() > 0) {
                    sub = sub.trim();
                }
                //Log.d(TAG, "parseMatchedInfo = " + sub);
                result = sub;
            }
        }
        return result;
    }

    public String parseFrameFormatStrFromDi0Path() {
        String result = "";
        String frameFormat = readSysFs(ConstantManager.SYS_PI_PATH);
        if (!TextUtils.isEmpty(frameFormat) && !"null".equals(frameFormat) && !"NA".equals(frameFormat)) {
            if (frameFormat.startsWith(ConstantManager.CONSTANT_FORMAT_INTERLACE)) {
                result = ConstantManager.PI_TO_VIDEO_FORMAT_MAP.get(ConstantManager.CONSTANT_FORMAT_INTERLACE);
            } else if (frameFormat.startsWith(ConstantManager.CONSTANT_FORMAT_PROGRESSIVE)) {
                result = ConstantManager.PI_TO_VIDEO_FORMAT_MAP.get(ConstantManager.CONSTANT_FORMAT_PROGRESSIVE);
            } else if (frameFormat.startsWith(ConstantManager.CONSTANT_FORMAT_COMRPESSED)) {//Compressed may exist with progressive or interlace
                result = ConstantManager.PI_TO_VIDEO_FORMAT_MAP.get(ConstantManager.CONSTANT_FORMAT_PROGRESSIVE);
            } else {
                result = ConstantManager.PI_TO_VIDEO_FORMAT_MAP.get(ConstantManager.CONSTANT_FORMAT_PROGRESSIVE);
            }
        } else {
            result = ConstantManager.PI_TO_VIDEO_FORMAT_MAP.get(ConstantManager.CONSTANT_FORMAT_PROGRESSIVE);
        }
        if (DEBUG) {
            Log.d(TAG, "parseFrameFormatStrFromDi0Path result = " + result);
        }
        return result;
    }

    public List<String> getStorageDeviceNameList() {
        List<String> result = new ArrayList<String>();
        List<Map<String, Object>> mapList = getStorageDevices();
        String name = "";
        if (mapList != null && mapList.size() > 0) {
            for (Map<String, Object> map : mapList) {
                name = getStorageName(map);
                result.add(name);
            }
        }
        return result;
    }

    public List<String> getStorageDevicePathList() {
        List<String> result = new ArrayList<String>();
        List<Map<String, Object>> mapList = getStorageDevices();
        String name = "";
        if (mapList != null && mapList.size() > 0) {
            for (Map<String, Object> map : mapList) {
                name = getStoragePath(map);
                result.add(name);
            }
        }
        return result;
    }

    private List<Map<String, Object>> getStorageDevices() {
        List<Map<String, Object>> result = new ArrayList<Map<String, Object>>();
        Map<String, Object> map = new HashMap<String, Object>();
        map.put(FileListManager.KEY_NAME, mContext.getString(R.string.strSettingsPvrDefault));
        map.put(FileListManager.KEY_PATH, PVR_DEFAULT_PATH);
        result.add(map);
        result.addAll(getWriteableDevices());
        return result;
    }

    private List<Map<String, Object>> getWriteableDevices() {
        List<Map<String, Object>> result = new ArrayList<Map<String, Object>>();
        List<Map<String, Object>> readList = mFileListManager.getDevices();
        if (readList != null && readList.size() > 0) {
            for (Map<String, Object> map : readList) {
                String storagePath = (String) map.get(FileListManager.KEY_PATH);
                if (storagePath != null && storagePath.startsWith("/storage/emulated")) {
                    Log.d(TAG, "getWriteableDevices add inner sdcard " + storagePath);
                    result.add(map);
                } else if (storagePath != null && storagePath.startsWith("/storage")) {
                    String uuid = null;
                    int idx = storagePath.lastIndexOf("/");
                    if (idx >= 0) {
                        uuid = storagePath.substring(idx + 1);
                    }
                    if (uuid != null) {
                        Log.d(TAG, "getWriteableDevices add storage /mnt/media_rw/" + uuid);
                        map.put(FileListManager.KEY_PATH, "/mnt/media_rw/" + uuid);
                        result.add(map);
                    } else {
                        Log.d(TAG, "getWriteableDevices empty uuid");
                    }
                } else {
                    Log.d(TAG, "getWriteableDevices ukown device " + storagePath);
                }
            }
        } else {
            Log.d(TAG, "getWriteableDevices device not found");
        }
        return result;
    }

    public static boolean isDeviceExist(String devicePath) {
        boolean result = false;
        if (devicePath != null) {
            try {
                File file = new File(devicePath);
                if (file != null && file.exists() && file.isDirectory()) {
                    result = true;
                }
            } catch (Exception e) {
                Log.e(TAG, "isDeviceExist Exception = " + e.getMessage());
                e.printStackTrace();
            }
        }
        return result;
    }

    public static boolean isMovableDevice(String devicePath) {
        boolean result = false;
        if (devicePath != null && !devicePath.startsWith("/storage/emulated") && devicePath.startsWith("/storage")) {
            Log.d(TAG, "isMovableDevice " + devicePath);
            result = true;
        }
        return result;
    }

    public String getStorageName(Map<String, Object> map) {
        String result = null;
        if (map != null) {
            result = (String) map.get(FileListManager.KEY_NAME);
        }
        return result;
    }

    public String getStoragePath(Map<String, Object> map) {
        String result = null;
        if (map != null) {
            result = (String) map.get(FileListManager.KEY_PATH);
        }
        return result;
    }

    public String getAppDefaultPath() {
        return PVR_DEFAULT_PATH;
    }

    public static String convertMediaPathToMountedPath(String mediaPath) {
        String result = null;
        if (isMediaPath(mediaPath)) {
            String[] split = mediaPath.split("/");
            if (split != null && split.length > 0) {
                result = "/storage/" + split[split.length - 1];
            }
        }
        return result;
    }

    public static boolean isMediaPath(String mediaPath) {
        boolean result = false;
        if (mediaPath != null && mediaPath.startsWith("/mnt/media_rw/")) {
            result = true;
        }
        return result;
    }
}
