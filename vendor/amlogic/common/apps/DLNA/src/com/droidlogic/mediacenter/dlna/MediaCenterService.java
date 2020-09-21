/******************************************************************
*
*Copyright (C) 2016  Amlogic, Inc.
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
package com.droidlogic.mediacenter.dlna;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.InputStream;

import org.amlogic.upnp.AmlogicCP;
import org.amlogic.upnp.MediaRendererDevice;
import org.cybergarage.net.HostInterface;
import org.cybergarage.upnp.device.InvalidDescriptionException;
import org.cybergarage.util.Debug;
import org.cybergarage.upnp.UPnP;
import com.droidlogic.mediacenter.R;

import android.app.ActivityManager;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.ConnectivityManager;
import android.net.NetworkInfo;
import android.net.wifi.WifiInfo;
import android.net.wifi.WifiManager;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.PowerManager;
import android.net.wifi.WifiManager;
import android.net.wifi.WifiManager.MulticastLock;
import org.amlogic.upnp.DesUtils;

/**
 * @ClassName MediaCenterService
 * @Description TODO
 * @Date 2013-8-27
 * @Email
 * @Author
 * @Version V1.0
 */
public class MediaCenterService extends Service {

    private static final String TAG = "MediaCenterService";
    private static final String CONFIG_DIR = "DMR-Intel/";
    private static final String DESCRIPTION_FILENAME = "description.xml";
    private static final String SERVICE_AVT_FILENAME = "INTEL.AVTransport.scpd.xml";
    private static final String SERVICE_RC_FILENAME = "INTEL.RenderingControl.scpd.xml";
    private static final String SERVICE_CM_FILENMAE = "INTEL.ConnectionManager.scpd.xml";
    private static final String IMAGE_DIR = "images/";
    private static final String IMAGE_FILENAME_JPG = "dlna.jpg";
    private static final String IMAGE_FILENAME_PNG = "dlna.png";
    private PrefUtils mPref;
    private DMRDevice mDmrDevice;
    private static final int DMR_CMD = 101;
    private static final int DMR_DEV_START = 102;
    private static final int DMR_DEV_STOP = 103;
    public static final String SERVICE_NAME_CHANGE = "com.android.mediacenter.servicename";
    public static final String DEVICE_STATUS_CHANGE = "com.android.mediacenter.devicestatus";
    private MulticastLock multicastLock;

    /*
     * (non-Javadoc)
     *
     * @see android.app.Service#onBind(android.content.Intent)
     */
    @Override
    public IBinder onBind(Intent arg0) {
        return null;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mPref = new PrefUtils(this);
        initDMR();
        WifiManager wifiManager = (WifiManager) getSystemService(Context.WIFI_SERVICE);
        multicastLock = wifiManager.createMulticastLock("multicast.test");
        multicastLock.acquire();
    }

    private void initDMR() {
        UPnP.setEnable(UPnP.USE_ONLY_IPV4_ADDR);
        String mAbsPath = getFilesDir().getAbsolutePath();
        if (!new File(mAbsPath, CONFIG_DIR + SERVICE_CM_FILENMAE).exists()) {
            // showStartImage
            copyAsset(mAbsPath, CONFIG_DIR + DESCRIPTION_FILENAME);
            copyAsset(mAbsPath, CONFIG_DIR + SERVICE_AVT_FILENAME);
            copyAsset(mAbsPath, CONFIG_DIR + SERVICE_RC_FILENAME);
            copyAsset(mAbsPath, CONFIG_DIR + SERVICE_CM_FILENMAE);
            copyAsset(mAbsPath, IMAGE_DIR + IMAGE_FILENAME_JPG);
            copyAsset(mAbsPath, IMAGE_DIR + IMAGE_FILENAME_PNG);
        }
        if (mDmrDevice == null) {
            try {
                mDmrDevice = new DMRDevice(mAbsPath + "/" + DESCRIPTION_FILENAME);
                String mac = getNetWorkMac();
                PrefUtils mPrefUtils = new PrefUtils(this);
                String serviceName = mPrefUtils.getString(PrefUtils.SERVICE_NAME, null);
                if (serviceName != null) {
                    mDmrDevice.setFriendlyName("DLNA-" + serviceName);
                } else {
                    mDmrDevice.setFriendlyName(
                            "DLNA-" + MediaCenterService.this.getResources().getString(R.string.config_default_name));
                }
                mDmrDevice.setUDN(mDmrDevice.getUDN() + mac);
            } catch (InvalidDescriptionException e) {
                // TODO Auto-generated catch block
                e.printStackTrace();
            }
        }
    }

    /**
     * @Description TODO
     * @return
     */
    private String getNetWorkMac() {
        ConnectivityManager connectivityManager = (ConnectivityManager) this
                .getSystemService(Context.CONNECTIVITY_SERVICE);
        NetworkInfo activeNetInfo = connectivityManager.getActiveNetworkInfo();
        int netType = ConnectivityManager.TYPE_WIFI;
        if (activeNetInfo != null) {
            netType = activeNetInfo.getType();
        }
        String mac = null;
        if (netType == ConnectivityManager.TYPE_WIFI) {
            WifiManager wifi = (WifiManager) getSystemService(Context.WIFI_SERVICE);
            WifiInfo info = wifi.getConnectionInfo();
            mac = info.getMacAddress();
        } else if (netType == ConnectivityManager.TYPE_ETHERNET) {
            File fl = new File("/sys/class/net/eth0/address");
            if (fl.exists()) {
                try {
                    FileReader fr = new FileReader(fl);
                    char[] buf = new char[32];
                    int len = fr.read(buf);
                    if (len > 0) {
                        if (len > 17) {
                            len = 17;
                        }
                        mac = new String(buf, 0, len);
                        fr.close();
                    }
                } catch (Exception e) {
                }
            }
        }
        if (mac == null) {
            mac = (String)PrefUtils.getProperties("ubootenv.var.ethaddr", "UNKNOWN");
        }
        HostInterface.resetInterface();
        if (!mac.isEmpty() && mac.contains(":")) {
            mac = mac.replaceAll(":", "");
        }
        return mac;
    }

    private void copyAsset(String absPath, String filename) {
        InputStream i_s = null;
        FileOutputStream fos;
        String sourcefilename = absPath + "/" + filename;
        File f = new File(sourcefilename);
        try {
            i_s = getAssets().open(filename);
            fos = openFileOutput(f.getName(), Context.MODE_PRIVATE);
            while (i_s.available() > 0) {
                byte[] b = new byte[1024];
                int bytesread = i_s.read(b);
                fos.write(b, 0, bytesread);
            }
            fos.close();
            i_s.close();
        } catch (FileNotFoundException e2) {
            e2.printStackTrace();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        if (mDmrDevice != null) {
            mPref.setBoolean(PrefUtils.FISAT_START, false);
            mDmrDevice.stopDMR();
            mDmrDevice.startDMR();
        }
        return super.onStartCommand(intent, flags, startId);
    }

    class DMRDevice extends MediaRendererDevice {
        public boolean isDMRStart = false;
        private Handler mHandler = null;
        private HandlerThread mDMRDemoThread;
        private PowerManager.WakeLock mWakeLock;
        private boolean isRegistReceiver = false;

        private BroadcastReceiver mDMRServiceListener = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                if (isDMRStart) {
                    Debug.d(TAG, "onReceive:" + intent.getAction());
                    if (intent.getAction() == SERVICE_NAME_CHANGE) {
                        mDmrDevice.stopDMR();
                        mDmrDevice.setFriendlyName("DLNA-" + intent.getStringExtra("service_name"));
                        mDmrDevice.startDMR();
                    } else if (intent.getAction() == DEVICE_STATUS_CHANGE) {
                        String status = intent.getStringExtra("status");
                        if (status == null) {
                            status = "TRANSITIONING";
                        } else {
                        }
                        mDmrDevice.changeStatus(status);
                    } else {
                        Message msg = new Message();
                        msg.what = DMR_CMD;
                        msg.obj = intent;
                        mHandler.sendMessage(msg);
                    }
                }
            }
        };

        /**
         * @Description TODO
         * @param string
         */
        public DMRDevice(String conf) throws InvalidDescriptionException {
            super(conf);
            mDMRDemoThread = new HandlerThread("DMRDevice");
            mDMRDemoThread.start();
            mHandler = new Handler(mDMRDemoThread.getLooper()) {
                public void handleMessage(Message msg) {
                    switch (msg.what) {
                        case DMR_CMD:
                            Intent intent = (Intent) msg.obj;
                            DMRDevice.this.handleMessage(intent);
                            break;
                        case DMR_DEV_START:
                            startDMRInternal();
                            break;
                        case DMR_DEV_STOP:
                            stopDMRInternal();
                            break;
                    }
                }
            };
        }

        @Override
        public void notifyDMR(Intent intent) {
            String action = intent.getAction();
            if (AmlogicCP.UPNP_PLAY_ACTION.equals(action)) {
                String type = intent.getStringExtra(AmlogicCP.EXTRA_MEDIA_TYPE);
                ActivityManager mactivitymanager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
                ComponentName cn = mactivitymanager.getRunningTasks(1).get(0).topActivity;
                String name = cn.getClassName();
                if (!MusicPlayer.isShowingForehand && MediaRendererDevice.TYPE_AUDIO.equals(type)) {
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
                    intent.setClass(MediaCenterService.this, MusicPlayer.class);
                    VideoPlayer.running = false;
                    ImageFromUrl.isShowingForehand = false;
                    stopMediaPlayer();
                    startActivity(intent);
                    return;
                } else if (!VideoPlayer.running && MediaRendererDevice.TYPE_VIDEO.equals(type)) {
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
                    intent.setClass(MediaCenterService.this, VideoPlayer.class);
                    ImageFromUrl.isShowingForehand = false;
                    MusicPlayer.isShowingForehand = false;
                    stopMediaPlayer();
                    startActivity(intent);
                    return;
                } else if (!ImageFromUrl.isShowingForehand && MediaRendererDevice.TYPE_IMAGE.equals(type)) {
                    intent.addFlags(Intent.FLAG_ACTIVITY_NO_HISTORY);
                    intent.addFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                    intent.setClass(MediaCenterService.this, ImageFromUrl.class);
                    MusicPlayer.isShowingForehand = false;
                    VideoPlayer.running = false;
                    startActivity(intent);
                    return;
                }
            }
            sendBroadcast(intent);
        }

        /**
         * @Description TODO
         */
        private void stopMediaPlayer() {
            ActivityManager mactivitymanager = (ActivityManager) getSystemService(ACTIVITY_SERVICE);
            ComponentName cn = mactivitymanager.getRunningTasks(1).get(0).topActivity;
            String name = cn.getClassName();
            if (name.equals("com.farcore.videoplayer.playermenu")) {
                mactivitymanager.restartPackage(cn.getPackageName());
            } else if (name.equals("org.geometerplus.android.fbreader.FBReader")) {
                Intent intent = new Intent();
                intent.setAction("com.android.music.musicservicecommand.pause");
                intent.putExtra("command", "stop");
                // intent.putExtra("targetpkg_music", "yes");
                // intent.putExtra("targetpkg_musicplayer", "yes");
                sendBroadcast(intent);
            }
        }

        private void startDMRInternal() {
            if (isDMRStart) {
                //already run;
                return;
            }
            if (super.isRunning()) {
                super.stop(false);
            }
            if (!super.isRunning()) {
                super.start();
                isDMRStart = true;
            }
        }

        public void stopDMRInternal() {
            if (super.isRunning() && isDMRStart) {
                super.stop();
                isDMRStart = false;
            }
        }

        public void startDMR() {
            if (mHandler == null) {
                return;
            }
            mHandler.sendEmptyMessage(DMR_DEV_START);
            if (!isRegistReceiver) {
                IntentFilter f = new IntentFilter();
                f.addAction(AmlogicCP.PLAY_POSTION_REFRESH);
                f.addAction(MediaRendererDevice.PLAY_STATE_PAUSED);
                f.addAction(MediaRendererDevice.PLAY_STATE_PLAYING);
                f.addAction(MediaRendererDevice.PLAY_STATE_STOPPED);
                f.addAction(MediaRendererDevice.PLAY_STATE_SETVOLUME);
                f.addAction(SERVICE_NAME_CHANGE);
                f.addAction(DEVICE_STATUS_CHANGE);
                registerReceiver(mDMRServiceListener, f);
                isRegistReceiver = true;
            }
        }

        public void stopDMR() {
            if (isRegistReceiver) {
                isRegistReceiver = false;
                unregisterReceiver(mDMRServiceListener);
            }
            if (mHandler != null) {
                mHandler.sendEmptyMessage(DMR_DEV_STOP);
            }
        }

        /*
         * public void stopDreaming(){ Intent intent = new
         * Intent("android.intent.action.DREAMING_STOPPED");
         * sendBroadcast(intent); }
         */

    }

    @Override
    public void onDestroy() {
        multicastLock.release();
        super.onDestroy();
        if (mDmrDevice != null) {
            mDmrDevice.stopDMR();
        }
    }

    public static int getAndroidOSVersion() {
        int osVersion;
        try {
            osVersion = Integer.valueOf(android.os.Build.VERSION.SDK);
        } catch (NumberFormatException e) {
            osVersion = 0;
        }
        return osVersion;
    }
}
