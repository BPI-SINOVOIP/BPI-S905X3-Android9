/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC NetflixService
 */

package com.droidlogic;

import android.app.ActivityManager;
import android.app.ActivityManager.RunningAppProcessInfo;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.Service;
import android.content.Context;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Intent;
import android.content.IntentFilter;
import android.hardware.hdmi.HdmiControlManager;
import android.hardware.hdmi.HdmiControlManager.HotplugEventListener;
import android.hardware.hdmi.HdmiHotplugEvent;
import android.media.AudioManager;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.IBinder;
import android.os.Process;
import android.os.SystemProperties;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.lang.StringBuffer;
import java.util.List;
import java.util.Scanner;
import com.droidlogic.app.SystemControlManager;

import com.droidlogic.R;

public class NetflixService extends Service {
    private static final String TAG = "NetflixService";

    /**
     * Feature for {@link #getSystemAvailableFeatures} and {@link #hasSystemFeature}:
     * This device supports netflix
     */
    public static final String FEATURE_SOFTWARE_NETFLIX     = "droidlogic.software.netflix";

    public static final String NETFLIX_PKG_NAME             = "com.netflix.ninja";
    public static final String NETFLIX_STATUS_CHANGE        = "com.netflix.action.STATUS_CHANGE";
    public static final String NETFLIX_DIAL_STOP            = "com.netflix.action.DIAL_STOP";
    private static final String VIDEO_SIZE_DEVICE           = "/sys/class/video/device_resolution";
    private static final String SYS_AUDIO_CAP               = "/sys/class/amhdmitx/amhdmitx0/aud_cap";
    private static final String AUDIO_MS12LIB_PATH          = "/vendor/lib/libdolbyms12.so";
    private static final String WAKEUP_REASON_DEVICE        = "/sys/devices/platform/aml_pm/suspend_reason";
    private static final String NRDP_PLATFORM_CAP           = "nrdp_platform_capabilities";
    private static final String NRDP_AUDIO_PLATFORM_CAP     = "nrdp_audio_platform_capabilities";
    private static final String NRDP_PLATFORM_CONFIG_DIR    = "/vendor/etc/";
    private static final int WAKEUP_REASON_CUSTOM           = 9;
    private static boolean mLaunchDialService               = true;

    private boolean mIsNetflixFg = false;
    private boolean mHasMS12 = false;
    private boolean mAtomsEnabled = false;
    private Context mContext;
    SystemControlManager mSCM;
    HdmiControlManager mHdmiControlManager;
    AudioManager mAudioManager;

    private BroadcastReceiver mReceiver = new BroadcastReceiver(){
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            Log.i(TAG, "action:" + action);

            if (action.equals(NETFLIX_DIAL_STOP)) {
                int pid = getNetflixPid();
                if (pid > 0) {
                    Log.i (TAG, "Killing active Netflix Service PID: " + pid);
                    android.os.Process.killProcess (pid);
                }
            }
        }
    };

    HotplugEventListener mHPListener = new HotplugEventListener() {
        @Override
        public void onReceived(HdmiHotplugEvent event) {
            if (!event.isConnected())
                return;

            String audioSinkCap = mSCM.readSysFs(SYS_AUDIO_CAP);
            boolean atmosSupported = audioSinkCap.contains("Dobly_Digital+/ATMOS");
            Log.i (TAG, "ATMOS: " + atmosSupported + ", audioSinkCap: " + audioSinkCap);
            setAtmosEnabled(atmosSupported);
        }
    };

    @Override
    public void onCreate() {
        super.onCreate();
        mContext = this;
        mSCM = SystemControlManager.getInstance();
        mAudioManager = (AudioManager) getSystemService(Context.AUDIO_SERVICE);

        IntentFilter filter = new IntentFilter(NETFLIX_DIAL_STOP);
        filter.setPriority (IntentFilter.SYSTEM_HIGH_PRIORITY);
        mContext.registerReceiver (mReceiver, filter);

        String buildDate = SystemProperties.get("ro.build.version.incremental", "");
        boolean needUpdate = !buildDate.equals(SettingsPref.getSavedBuildDate(mContext));

        setNrdpCapabilitesIfNeed(NRDP_PLATFORM_CAP, needUpdate);
        setNrdpCapabilitesIfNeed(NRDP_AUDIO_PLATFORM_CAP, needUpdate);
        if (needUpdate) {
            SettingsPref.setSavedBuildDate(mContext, buildDate);
        }

        mHasMS12 = new File(AUDIO_MS12LIB_PATH).exists();
        if (mHasMS12) {
            mHdmiControlManager = (HdmiControlManager) getSystemService(Context.HDMI_CONTROL_SERVICE);
            mHdmiControlManager.addHotplugEventListener(mHPListener);
        }

        startNetflixIfNeed();

        new ObserverThread ("NetflixObserver").start();
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        return super.onStartCommand(intent, flags, startId);
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
    }

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    private void startNetflixIfNeed() {
        Scanner scanner = null;
        int reason = -1;
        try {
            scanner = new Scanner (new File(WAKEUP_REASON_DEVICE));
            reason = scanner.nextInt();
            scanner.close();
        } catch (Exception e) {
            if (scanner != null)
                scanner.close();
            e.printStackTrace();
            return;
        }

        if (reason == WAKEUP_REASON_CUSTOM) {
            /* response slowly while system start, use startActivity instead
            Intent netflixIntent = new Intent();
            netflixIntent.setAction("com.netflix.ninja.intent.action.NETFLIX_KEY");
            netflixIntent.setPackage("com.netflix.ninja");
            netflixIntent.putExtra("power_on", true);
            netflixIntent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);

            Log.i(TAG, "start netflix by power on");
            mContext.sendBroadcast(netflixIntent,"com.netflix.ninja.permission.NETFLIX_KEY");
            */
            Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("nflx://www.netflix.com/"));
            intent.putExtra("deeplink", "&source_type=19");
            intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
            intent.addFlags(Intent.FLAG_INCLUDE_STOPPED_PACKAGES);
            mContext.startActivity(intent);
        }
    }

    private void setNrdpCapabilitesIfNeed(String capName, boolean needUpdate) {
        String cap = Settings.Global.getString(getContentResolver(), capName);
        Log.i(TAG, capName + ":\n" + cap);
        if (!needUpdate && !TextUtils.isEmpty(cap)) {
            return;
        }

        try {
            Scanner scanner = new Scanner(new File(NRDP_PLATFORM_CONFIG_DIR + capName + ".json"));
            StringBuffer sb = new StringBuffer();

            while (scanner.hasNextLine()) {
                sb.append(scanner.nextLine());
                sb.append('\n');
            }

            Settings.Global.putString(getContentResolver(), capName, sb.toString());
            scanner.close();
        } catch (java.io.FileNotFoundException e) {
            Log.d(TAG, e.getMessage());
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    public int getNetflixPid() {
        int retry = 10;
        ActivityManager manager = (ActivityManager) mContext.getSystemService (Context.ACTIVITY_SERVICE);

        do {
            List<RunningAppProcessInfo> services = manager.getRunningAppProcesses();
            for (int i = 0; i < services.size(); i++) {
                String servicename = services.get (i).processName;
                if (servicename.contains (NETFLIX_PKG_NAME)) {
                    Log.i (TAG, "find process: " + servicename + " pid: " + services.get (i).pid);
                    return services.get (i).pid;
                }
            }
        } while (--retry > 0);

        return -1;
    }

    public boolean netflixActivityRunning() {
        ActivityManager am = (ActivityManager) mContext.getSystemService (Context.ACTIVITY_SERVICE);
        List< ActivityManager.RunningTaskInfo > task = am.getRunningTasks (1);

        if (task.size() != 0) {
            if (mLaunchDialService) {
                mLaunchDialService = false;

                try {
                    Intent intent = new Intent();
                    intent.setClassName ("com.netflix.dial", "com.netflix.dial.NetflixDialService");
                    mContext.startService (intent);
                } catch (SecurityException e) {
                    Log.i (TAG, "Initial launching dial Service failed");
                }
            }

            ComponentName componentInfo = task.get (0).topActivity;
            if (componentInfo.getPackageName().equals (NETFLIX_PKG_NAME) ) {
                //Log.i (TAG, "netflix running as top activity");
                return true;
            }
        }
        return false;
    }

    public static String setSystemProperty(String key, String defValue) {
        String getValue = defValue;
        try {
            Class[] typeArgs = new Class[2];
            typeArgs[0] = String.class;
            typeArgs[1] = String.class;

            Object[] valueArgs = new Object[2];
            valueArgs[0] = key;
            valueArgs[1] = defValue;

            getValue = (String)Class.forName("android.os.SystemProperties")
                    .getMethod("set", typeArgs)
                    .invoke(null, valueArgs);
        } catch (Exception e) {
            e.printStackTrace();
        }

        return getValue;
    }

    private void setAtmosEnabled(boolean enabled) {
        String audioCap = Settings.Global.getString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP);
        if (audioCap == null)
            return;

        int keyPos, valPos;
        if ((keyPos = audioCap.indexOf("\"atmos\"")) < 0
            || (keyPos = audioCap.indexOf("\"enabled\"", keyPos)) < 0)
            return;

        valPos = audioCap.indexOf("true", keyPos);
        if (valPos < 0 || (valPos - keyPos) > 16)
            valPos = audioCap.indexOf("false", keyPos);

        if (valPos > 0 && (valPos - keyPos) <= 16) {
            boolean isEnabled = (audioCap.charAt(valPos) == 't');
            if (isEnabled ^ enabled) {
                Log.i(TAG, "ATMOS enabled -> " + enabled);
                String newCap = audioCap.substring(0, valPos)
                        + enabled + audioCap.substring(valPos + String.valueOf(isEnabled).length());
                Settings.Global.putString(getContentResolver(), NRDP_AUDIO_PLATFORM_CAP, newCap);
            }
        }
        mAtomsEnabled = enabled;
    }

    class ObserverThread extends Thread {
        public ObserverThread (String name) {
            super (name);
        }

        public void run() {
            while (true) {
                try {
                    Thread.sleep (1000);
                } catch (InterruptedException e) {
                    e.printStackTrace();
                }

                boolean fg = netflixActivityRunning();
                if (fg ^ mIsNetflixFg) {
                    Log.i (TAG, "Netflix status changed from " + (mIsNetflixFg?"fg":"bg")+ " -> " + (fg?"fg":"bg"));

                    mIsNetflixFg = fg;
                    Intent intent = new Intent();
                    intent.setAction (NETFLIX_STATUS_CHANGE);
                    intent.putExtra ("status", fg ? 1 : 0);
                    intent.putExtra ("pid", fg?getNetflixPid():-1);
                    mContext.sendBroadcast (intent);

                    mAudioManager.setParameters("continuous_audio_mode=" + (fg ? "1" : "0"));
                    mSCM.setProperty ("vendor.netflix.state", fg ? "fg" : "bg");
                }

/* move setting display-size code to systemcontrol
                if (SystemProperties.getBoolean ("sys.display-size.check", true) ||
                    SystemProperties.getBoolean ("vendor.display-size.check", true)) {
                    try {
                        Scanner sc = new Scanner (new File(VIDEO_SIZE_DEVICE));
                        if (sc.hasNext("\\d+x\\d+")) {
                            String[] parts = sc.nextLine().split ("x");
                            int w = Integer.parseInt (parts[0]);
                            int h = Integer.parseInt (parts[1]);
                            //Log.i(TAG, "Video resolution: " + w + "x" + h);

                            String nexflixProps[] = {"sys.display-size", "vendor.display-size"};
                            for (String propName:nexflixProps) {
                                String prop = SystemProperties.get (propName, "0x0");
                                String[] parts_prop = prop.split ("x");
                                int wd = Integer.parseInt (parts_prop[0]);
                                int wh = Integer.parseInt (parts_prop[1]);

                                if ((w != wd) || (h != wh)) {
                                    mSCM.setProperty(propName, String.format("%dx%d", w, h));
                                    //setSystemProperty(propName, String.format("%dx%d", w, h));
                                    //SystemProperties.set (propName, String.format("%dx%d", w, h));
                                    //Log.i(TAG, "set sys.display-size property to " + String.format("%dx$d", w, h));
                                }
                            }
                        } else {
                            //Log.i(TAG, "Video resolution no pattern found" + sc.nextLine());
                        }
                        sc.close();

                    } catch (Exception e) {
                        Log.i(TAG, "Error parsing video size device node");
                    }
                }
*/
            }
        }
    }
}

