/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description:
 *     AMLOGIC SubtitleManager
 */

package com.droidlogic.app;

import android.app.Application;
import android.content.Context;
import android.content.ContentResolver;
import android.database.Cursor;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.RemoteException;

import android.os.SystemProperties;

import android.provider.MediaStore;
import android.util.Log;
import android.graphics.*;

import android.widget.TextView;
import android.widget.ImageView;
import android.widget.FrameLayout;
import android.graphics.Bitmap.Config;

import android.view.*;
import android.os.Process;

import android.content.ComponentName;
import android.content.Intent;
import android.util.DisplayMetrics;

import android.content.res.AssetManager;
import java.io.InputStream;
import java.io.FileOutputStream;
import java.io.OutputStream;
import java.io.IOException;
import java.util.zip.ZipFile;
import java.util.zip.ZipException;
import java.util.zip.ZipEntry;
import java.util.Enumeration;
import java.lang.Integer;
import java.lang.reflect.Method;
import java.lang.Thread;
import java.io.BufferedReader;
import java.io.BufferedWriter;
import java.io.File;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.FileNotFoundException;
import java.util.ArrayList;
import java.util.List;
import android.app.ActivityManager;
import java.io.UnsupportedEncodingException;


import com.droidlogic.app.MediaPlayerExt;

public class SubtitleManager {
    static private final String TAG = "SubtitleManager";
    private static String unZipDirStr = "vendorfont";
    private static String uncryptDirStr = "font";
    private static String fonts = "font";
    static private final int RETRY_MAX = 10;
    static private final int TV_SUB_MPEG2 = 0;  //close caption mpeg2
    static private final int TV_SUB_H264 = 2;    //close caption h264
    static private final int TV_SUB_SCTE27 = 3;
    static private final int TV_SUB_DVB = 4;

    static public final int TYPE_SUBTITLE_INVALID = -1;
    static public final int TYPE_SUBTITLE_VOB = 0;
    static public final int TYPE_SUBTITLE_PGS = 1;
    static public final int TYPE_SUBTITLE_MKV_STR = 2;
    static public final int TYPE_SUBTITLE_SSA = 3;
    static public final int TYPE_SUBTITLE_MKV_VOB = 4;
    static public final int TYPE_SUBTITLE_DVB = 5;
    static public final int TYPE_SUBTITLE_TMD_TXT = 7;
    static public final int TYPE_SUBTITLE_IDX_SUB = 8;
    static public final int TYPE_SUBTITLE_DVB_TELETEXT = 9;
    static public final int TYPE_SUBTITLE_CLOSED_CATPTION = 10;
    static public final int TYPE_SUBTITLE_SCTE27 = 11;
    static public final int TYPE_SUBTITLE_EXTERNAL = 15;
    static public final int TYPE_SUBTITLE_MAX = 13;
    //for the subtitle display type:text or image
    static public final int SUBTITLE_TXT =1;
    static public final int SUBTITLE_IMAGE =2;
    static public final int SUBTITLE_CC_JASON = 3;
    static public final int SUBTITLE_IMAGE_CENTER = 4;

    /* UI Commands, need keep the same as subtitleserver Itypes */
    static public final int CMD_UI_SHOW = 0;
    static public final int CMD_UI_HIDE = 1;
    static public final int CMD_UI_SET_TEXTCOLOR = 2;
    static public final int CMD_UI_SET_TEXTSIZE = 3;
    static public final int CMD_UI_SET_GRAVITY = 4;
    static public final int CMD_UI_SET_TEXTSTYLE = 5;
    static public final int CMD_UI_SET_POSHEIGHT = 6;
    static public final int CMD_UI_SET_IMGRATIO = 7;
    static public final int CMD_UI_SET_SUBDEMISION = 8;
    static public final int CMD_UI_SET_SURFACERECT = 9;


    /*subtitle info callback type*/
    static public final int SUBTITLE_INFO_TELETEXT_LOAD_STATE = 900;


    //TELETEXT load state
    static public final int TT2_DISPLAY_STATE = 0;
    static public final int TT2_SEARCH_STATE  = 1;


    private boolean mDebug = true;
    private MediaPlayerExt mMediaPlayer = null;
    private boolean mInvokeFromMp = false;
    private boolean mThreadStop = true;
    private String mPath = null;
    private Thread mThread = null;

    private Context mContext;

    private SubtitleDataListener mHidlCallback;
    private FallbackDisplayListener mHidlFallbackDisplay;

    Class<?> objActivityThread = null;
    Method currentApplication = null;

    //IOType, should sync with \vendor\amlogic\apps\SubTitle\jni\subtitle\sub_io.h IOType
    //public static final int IO_TYPE_DEV = 0;
    public static final int IO_TYPE_FMQ = 0;
    public static final int IO_TYPE_DEV = 1;
    public static final int IO_TYPE_FILE =2;
    private int mIOType = IO_TYPE_FMQ;
    private ArrayList<Integer> mInnerTrackIdx = null;

    //store closecaption channelId
    private ArrayList<Integer> mChalIdList = null;

    private Intent intent = null;

    private SubtitleViewAdaptor mUI;
    private boolean mShowFlag = false;
    private Handler mHandler;

    //ext sub
    private SubtitleUtils mSubtitleUtils;
    private static final String[] mExternalExtension = {
        ".txt",".srt", ".smi", ".sami",".rt", ".ssa", ".ass",".lrc", ".xml",
        ".idx",".sub", ".pjs",".aqt", ".mpl", ".vtt",".js", ".jss"};

    private int mDisplayType = -1;
    private int mCurrentTrack = 0;
    private int mCurrentCCchannel = 15;
    private int mMonitorCCchannel = 15;

    //teletext loading resource id
    private int mResId = 0;


    private int mSubTotal = 0;

    private Rect mDisplayRect;

    static {
        try {
            System.load("/vendor/lib/libsubtitlemanager_jni.so");
        } catch (UnsatisfiedLinkError e) {
            Log.d(TAG, "Error try system!", e);
            try {
                System.load("/system/lib/libsubtitlemanager_jni.so");
            } catch (UnsatisfiedLinkError e2) {
                Log.d(TAG, "Error try product!", e2);
                System.load("/product/lib/libsubtitlemanager_jni.so");
            }
        }
    }

    private native void nativeInit(boolean isFallback);
    private native void nativeUpdateVideoPos(int pos);
    private native boolean nativeOpen(String path, int ioType);
    private native void nativeClose();
//    private native void nativeSetIoType(int type);
    private native void nativeDestroy();
    private native void nativeSelectCcChannel(int chn);
    private native void nativeOption();
    private native int nativeTotalSubtitles();
    private native int nativeInnerSubtitles();
    private native void nativeNext();
    private native void nativePrevious();
    private native void nativeDisplay();
    private native void nativeHide();
    private native void nativeClean();
    private native void nativeResetForSeek();
    private native int nativeGetSubWidth();
    private native int nativeGetSubHeight();
    private native void nativeSetSubType(int type);
    private native void nativeSetPlayerType(int type);
    private native void nativeSetSctePid(int pid);
    private native int nativeGetSubType();
    private native String nativeGetSubLanguage(int idx);
    private native String nativeGetCurName();
    private native int nativeGetSubTypeDetial();
    private native int nativeTtGoHome();
    private native int nativeTtGotoPage(int pageNo, int subPageNo);
    private native int nativeTtNextPage(int dir);
    private native int nativeTtNextSubPage(int dir);
    private native void nativeLoad(String path);
    private native void nativeSetSurfaceViewParam(int x, int y, int w, int h);
    private native void nativeUnCrypt(String src,String dest);


    public SubtitleManager() {
        init(getContext(), false);
    }

    public SubtitleManager(Context context) {
        init(context, false);
    }

    // This API is used internal only.
    public SubtitleManager(Context context, boolean isFallback) {
        init(context, true);
    }

    public SubtitleManager(Context context, boolean isFallback, int resId) {
        mResId = resId;
        init(context, true);
    }


    public SubtitleManager(Context context, MediaPlayerExt mp) {
        mMediaPlayer = mp;
        init(context, false);
    }

    public String getProcessName(Context context) {
        String process = null;
        if (context != null) {
            int pid = android.os.Process.myPid();
            ActivityManager am = (ActivityManager) context
                    .getSystemService(Context.ACTIVITY_SERVICE);
            List<ActivityManager.RunningAppProcessInfo> infoList = null;
            try {
                infoList = am.getRunningAppProcesses();
                for (ActivityManager.RunningAppProcessInfo apps : infoList) {
                    if (apps.pid == pid) {
                        process = apps.processName;
                    }
                }
            } catch (SecurityException e) {
                e.printStackTrace();
            }
        }
        return process;
    }

    /**
      * isFallback should not exported, only used for internal fallback display.
      * default is false.
     */
    private void init(Context context, boolean isFallback) {
        mContext = context;
        mChalIdList = new ArrayList<Integer>();
        mInnerTrackIdx = new ArrayList<Integer>();
        mDebug = false;
        checkDebug();
        initDefaultResolution();
        nativeInit(isFallback);
        initFont();
        runOnMainThread(() -> {
            mUI = new SubtitleViewAdaptor(mContext);
            mShowFlag = true;
        });
    }

    public Context getContext () {
        Context context = null;
        LOGI("[getContext]");
        try {
            objActivityThread = Class.forName("android.app.ActivityThread");
            currentApplication = objActivityThread.getMethod("currentApplication");
            context = ((Application) (currentApplication.invoke(objActivityThread))).getApplicationContext();
        }
        catch (Exception ex) {
            Log.e(TAG, "getContext failed:" + ex);
        }
        return context;
    }
    private void initFont() {
        File uncryteDir = new File(mContext.getDataDir(),uncryptDirStr);
        if (uncryteDir == null || uncryteDir.listFiles() == null || uncryteDir.listFiles().length == 0 ) {
            unzipUncry(uncryteDir);
         }
    }

    /**
     *   Default resolution, init to fullscreen mode. the caller need call setDisplayRect to
     *   set the actual display Rect [in screen] when required.
     */
    private void initDefaultResolution(){
        WindowManager wm = (WindowManager)mContext.getSystemService (Context.WINDOW_SERVICE);
        DisplayMetrics dm = new DisplayMetrics();
        wm.getDefaultDisplay().getMetrics(dm);
        mDisplayRect = new Rect(0, 0, dm.widthPixels, dm.heightPixels);
        //Log.e(TAG, "initResolution mScreenWidth:" + mScreenWidth +";mScreenHeight = "+mScreenHeight);
    }

    //get the subtilte display type by parse type : image or text
    private int subtitleTextOrImage(int type) {
        int displayType = -1;
        switch (type) {
            case TYPE_SUBTITLE_DVB:
                displayType =  SUBTITLE_IMAGE;
                break;
            case TYPE_SUBTITLE_EXTERNAL:
            case TYPE_SUBTITLE_SSA:
                displayType = SUBTITLE_TXT;
                break;
            case TYPE_SUBTITLE_CLOSED_CATPTION:
                displayType = SUBTITLE_CC_JASON;
                break;
            case TYPE_SUBTITLE_DVB_TELETEXT:
                displayType = SUBTITLE_IMAGE_CENTER;
                break;
            default:
                displayType = SUBTITLE_IMAGE;
        }
        final int fType = displayType;
        mDisplayType = fType;
        runOnMainThread(() -> { mUI.setDisplayType(fType); mUI.setSubtitleType(type);});
        return displayType;
    }

    private void updateChannedId(int event, int channelId) {
        int idx = -1;
        mDisplayType = SUBTITLE_CC_JASON;
        LOGI("[updateChannedId]event:" + event + ",channedId:" + channelId);
        if (event == 1 && !mChalIdList.contains(channelId)) { //1:add
            mChalIdList.add(channelId);
            if (mMonitorCCchannel == channelId && mCurrentCCchannel != channelId) {
                nativeSelectCcChannel(channelId);
                mCurrentCCchannel = channelId;
            }
        } else if(event == 0 && mChalIdList.contains(channelId)) { //0:remvoe
            idx = mChalIdList.indexOf(channelId);
            mChalIdList.remove(idx);
            if (mCurrentCCchannel == channelId) {
                nativeSelectCcChannel(15);
                mCurrentCCchannel = 15;
            }
        }
    }

    private void notifyAvailable(int avail) {
        LOGI("[notifyAvailable]avail:" + avail);
    }

    public int getDisplayType () {
        return mDisplayType;
    }

    private void runOnMainThread(Runnable r) {
        // Android MainThread is the first thread, same as pid.
        if (Process.myPid() != Process.myTid()) {
            synchronized(this) {
                if (mHandler == null) {
                    mHandler = new Handler(Looper.getMainLooper());
                }
            }
            mHandler.post(r);
        } else {
            r.run();
        }
    }

    private void runOnMainThreadImmidiate(Runnable r) {
        if (Process.myPid() != Process.myTid()) {
            synchronized(this) {
                if (mHandler == null) {
                    mHandler = new Handler(Looper.getMainLooper());
                }
            }
            mHandler.postAtFrontOfQueue(r);
        } else {
            r.run();
        }
    }

    private void processSubtileEvent(int type, Object data, byte[] subdata, int x, int y,
            int width ,int height, int videoWidth, int videoHeight, boolean show) {
        Log.d(TAG, "in SubtitleManager.java onSubtitleEvent:" + type+"; height="+height
                +"; width="+width+", show="+show+";videoWidth="+videoWidth+";videoHeight="+videoHeight);
        runOnMainThread(() -> {
            if (!mShowFlag) {
                Log.d(TAG, "processSubtileEvent:: the subtitle has stop!" );
                return;
            }
            mUI.setDisplayType(type);
            String text = null;
            switch (type) {
                case SUBTITLE_TXT:
                      Log.d(TAG, "startSubtitle TEXT: " + isExtSubtitle() + ", getExtSubCharset:" + mSubtitleUtils.getExtSubCharset());
                      if (isExtSubtitle()) {
                          try {
                              text = new String (subdata, mSubtitleUtils.getExtSubCharset());
                          }
                          catch (UnsupportedEncodingException e) {
                              LOGI("ext subtitle byte to string err!!!");
                              e.printStackTrace();
                          }
                      } else {
                          text = new String (subdata);
                      }
                      Log.d(TAG, "startSubtitle TEXT = " + text);
                      mUI.showSubtitleString(text, show);
                      break;

                case SUBTITLE_IMAGE:
                case SUBTITLE_IMAGE_CENTER:
                    if ((width <= 0) || (height <= 0)) {
                        if (!show) mUI.showBitmap(null, 1, 1, false);
                        return;
                    }
                    try {
                        int[] array = (int[])data;
                        mUI.setCordinate(x, y);
                        Bitmap bitmap = Bitmap.createBitmap(array, width, height, Config.ARGB_8888);
                        // scaling.
                        float scaleW = ((mDisplayRect.right-mDisplayRect.left)*1.0f)/(float)videoWidth;
                        float scaleH = ((mDisplayRect.bottom-mDisplayRect.top)*1.0f)/(float)videoHeight;
                        Log.d(TAG, "DisplayRect=" + mDisplayRect +" show bitmap scaleW:" + scaleW+", scaleH:"+scaleH);
                        mUI.showBitmap(bitmap, scaleW, scaleH, show);
                    } catch(Exception e) {
                        e.printStackTrace();
                    }
                    break;

                    case SUBTITLE_CC_JASON:
                        text = new String (subdata);
                        Log.d(TAG, "startSubtitle TEXT CC_text= " + text);
                        mUI.showCaptionClose(text);
                        break;
            }
        });
    }

    // TODO: how to design API use default impl
    public boolean startSubtitle() {
        mHidlCallback = new SubtitleDataListener() {
            public void onSubtitleEvent(int type, Object data, byte[] subdata, int x, int y,
                    int width ,int height, int videoWidth, int videoHeight, boolean show) {
                Log.d(TAG, "in SubtitleManager.java onSubtitleEvent:" + type+"; height="+height+"; width="+width+", show="+show);

                processSubtileEvent(type, data, subdata, x, y, width, height, videoWidth, videoHeight, show);
            }
        };
        // check window created or not
        runOnMainThread(() -> {
            mShowFlag = true;
            if (!mUI.isDisplayWindowAdded()) {
                mUI.addSubtitleView("In-App-subtitle");
            }
        });
        return true;
    }

    public boolean startFallbackDisplay() {
        Log.d(TAG, "startFallbackDisplay 3", new Throwable());
        mHidlFallbackDisplay = new FallbackDisplayListener() {
            public void onSubtitleEvent(int type, Object data, byte[] subdata, int x, int y,
                    int width ,int height, int videoWidth, int videoHeight, boolean show) {
                Log.d(TAG, "here, FallbackDisplayListener: onSubtitleEvent");

                // Check subtitle view created or not, if not, create it
                runOnMainThread(() -> {
                    mShowFlag = true;
                    if (!mUI.isDisplayWindowAdded()) {
                        mUI.addSystemSubtitleView("Fallback-system-overlay-subtitle");
                    }
                });

                processSubtileEvent(type, data, subdata, x, y, width, height, videoWidth, videoHeight, show);
            }
            public void onUiCommandEvent(int cmd, int params[]) {
                Log.d(TAG, "receive message:" + cmd);
                switch (cmd) {
                    case CMD_UI_SHOW:
                        enableDisplay();
                        display();
                        break;
                    case CMD_UI_HIDE:
                        hide();
                        disableDisplay();
                        break;
                    case CMD_UI_SET_TEXTCOLOR:
                        setTextColor(params[0]);
                        break;
                    case CMD_UI_SET_TEXTSIZE:
                        setTextSize(params[0]);
                        break;
                    case CMD_UI_SET_TEXTSTYLE:
                        setTextStyle(params[0]);
                        break;
                    case CMD_UI_SET_GRAVITY:
                        setGravity(params[0]);
                        break;
                    case CMD_UI_SET_POSHEIGHT:
                        setPosHeight(params[0]);
                        break;
                    case CMD_UI_SET_IMGRATIO:
                        setImgSubRatio((float)params[0], (float)params[1], params[2], params[3]);
                        break;
                    case CMD_UI_SET_SUBDEMISION:
                        Log.e(TAG, "Error! should not here!");
                        break;
                    case CMD_UI_SET_SURFACERECT:
                        setDisplayRect(params[0], params[1], params[2], params[3]);
                        runOnMainThread(() -> {
                            mUI.hideView(); // View Rect changed, hide, let refresh when received new subtitle
                            mUI.setSurfaceDisplayRect(params[0], params[1], params[2], params[3]);
                        });
                        break;
                    default:
                        Log.e(TAG, "not recognized ui cmd message:" + cmd);
                }
            }

            public void onSubtitleInfo(int what, int extra) {
                Log.d(TAG, "onSubtitleInfo what:" + what + ",extra:" + extra);
                switch (what) {
                    case SUBTITLE_INFO_TELETEXT_LOAD_STATE:
                        if (extra == TT2_DISPLAY_STATE) {
                            runOnMainThread(() -> {
                                mUI.stopTtxLoading();
                            });
                        } else if (extra == TT2_SEARCH_STATE) {
                            runOnMainThread(() -> {
                                mUI.startTtxLoading(mResId);
                            });
                        }
                        break;
                    default:
                        Log.e(TAG, "not recognized subtitle info callback:" + what);
                }

            }

        };
        return true;
    }

    // TODO: how to design api?
    public boolean stopSubtitle() {
         hide();
         runOnMainThreadImmidiate(() -> {
            mShowFlag = false;
            mUI.removeSubtitleView();
            mUI.setDisplayFlag(false);
         });
         mHidlCallback = null;
         mHidlFallbackDisplay = null;
         mChalIdList.clear();
         mThreadStop = true;
         return true;
    }

    // customize how to show.
    public boolean startSubtitle(SubtitleDataListener listener) {
        mHidlCallback = listener;
        return true;
    }

    //for CTC
    public void showSub(int pos) {
        nativeUpdateVideoPos(pos);
    }


    public void setMediaPlayer (MediaPlayerExt mp) {
        mMediaPlayer = mp;
    }

    private boolean disable() {
        boolean ret = false;
        try {
            ret = (boolean)Class.forName("android.os.SystemProperties")
                .getMethod("getBoolean", new Class[] { String.class, Boolean.TYPE })
                .invoke(null, new Object[] { "vendor.sys.subtitle.disable", false });
        } catch (Exception e) {
            Log.e(TAG,"[start]Exception e:" + e);
        }
        return ret;
    }

    private  boolean isExtSubtitle() {
        Log.i(TAG,"[isExtSubtitle]mPath:" + mPath);
        if (mPath != null) {
            String name = mPath.toLowerCase();
                for (String ext : mExternalExtension) {
                if (name.endsWith(ext))
                return true;
            }
        }
        return false;
    }

    private void checkDebug() {
        try {
            if (SystemProperties.getBoolean ("vendor.sys.subtitle.debug", true) ) {
                mDebug = true;
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
    }

    private int getIoType() {
        mIOType = SystemProperties.getInt("vendor.sys.subtitle.ioType", 0);
        Log.i(TAG, "##checkIoType:" + mIOType);
        return mIOType;
    }


    private boolean optionEnable() {
        boolean ret = false;
        try {
            if (SystemProperties.getBoolean ("vendor.sys.subtitleOption.enable", false) ) {
                ret = true;
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
        return ret;
    }

    private void LOGI(String msg) {
        if (mDebug) Log.i(TAG, msg);
    }

    private void LOGE(String msg) {
        /*if (mDebug)*/ Log.e(TAG, msg);
    }

    public void setInvokeFromMp (boolean fromMediaPlayer) {
        mInvokeFromMp = fromMediaPlayer;
    }

    public boolean getInvokeFromMp() {
        return mInvokeFromMp;
    }


    public void setSource (Context context, Uri uri) {
        if (context == null) {
            return;
        }

        if (uri == null) {
            return;
        }

        mPath = uri.getPath();

        String scheme = uri.getScheme();
        if (scheme == null || scheme.equals ("file") ) {
            mPath = uri.getPath();
            return;
        }

        try {
            ContentResolver resolver = context.getContentResolver();
            //add for subtitle service
            String mediaStorePath = uri.getPath();
            String[] cols = new String[] {
                MediaStore.Video.Media._ID,
                MediaStore.Video.Media.DATA
            };

            if (scheme.equals ("content") ) {
                int idx_check = (uri.toString() ).indexOf ("media/external/video/media");

                if (idx_check > -1) {
                    int idx = mediaStorePath.lastIndexOf ("/");
                    String idStr = mediaStorePath.substring (idx + 1);
                    int id = Integer.parseInt (idStr);
                    LOGI("[setSource]id:" + id);

                    String where = MediaStore.Video.Media._ID + "=" + id;
                    Cursor cursor = resolver.query (MediaStore.Video.Media.EXTERNAL_CONTENT_URI, cols, where , null, null);
                    if (cursor != null && cursor.getCount() == 1) {
                        int colidx = cursor.getColumnIndexOrThrow (MediaStore.Video.Media.DATA);
                        cursor.moveToFirst();
                        mPath = cursor.getString (colidx);
                        LOGI("[setSource]mediaStorePath:" + mediaStorePath + ",mPath:" + mPath);
                    }
                }
            }
        } catch (SecurityException ex) {
            LOGE("[setSource]SecurityException ex:" + ex);
        }
    }

    public void setSource (String path) {
        if (path == null) {
            return;
        }
        try {
            final Uri uri = Uri.parse(path);
            if ("file".equals (uri.getScheme())) {
                path = uri.getPath();
            }
                mPath = path;
        } catch (Exception e) {
            Log.e(TAG, "Exception:" +e);
        }
    }

    public boolean open(String path, int ioType) {
        boolean r = false;
        Log.d(TAG, "[open] path:" + path, new Throwable());

        r = nativeOpen(path, ioType);

        LOGI("[open] innerTotal:" + innerTotal() +", mIOType:" + mIOType);
        if (mInnerTrackIdx != null && mInnerTrackIdx.size() > 0 && mIOType != IO_TYPE_FILE && mMediaPlayer != null) {
            mMediaPlayer.selectTrack(mInnerTrackIdx.get(mCurrentTrack));
        }

        return r;
    }

    public boolean open(String path) {
        return open(path, IO_TYPE_FMQ);
    }

    public boolean open() {
        return open(null);
    }

    public void openIdx(int idx) {
        LOGI("[openIdx] idx:" + idx);
        if (idx < 0) {
            return;
        }
        //cc channelId switch
        if (mDisplayType == SUBTITLE_CC_JASON && idx < mChalIdList.size()) {
            mUI.clearContent();
            nativeSelectCcChannel(mChalIdList.get(idx));
            return;
        }
        //subtitle track switch
        if (idx < innerTotal() && mIOType == IO_TYPE_FMQ && mMediaPlayer != null) {
            nativeResetForSeek();
            mMediaPlayer.selectTrack(mInnerTrackIdx.get(idx));
            mCurrentTrack = idx;
            runOnMainThread(() -> { mUI.clearContent(); });
        }
    }

    public void startCCchanel(int channel) {
        LOGI("[startCCchanel] channel:" + channel);
      if ((channel&0xff) < 0 || (channel&0xff) > 15) {
            return;
        }

        mMonitorCCchannel = channel;
        for (int idx = 0; idx < mChalIdList.size(); idx ++) {
            if (mChalIdList.get(idx) == channel) {
                runOnMainThread(() -> { mUI.clearContent(); });
                nativeSelectCcChannel(channel);
                mCurrentCCchannel = channel;
                break;
            }
        }
       if ((channel >> 8 ) == 1) {
           runOnMainThread(() -> { mUI.clearContent(); });
           nativeSelectCcChannel(channel);
           mCurrentCCchannel = channel;
       }
    }

    public void start() {
        LOGI("[start]mPath:" + mPath);

        if (disable()) {
            return;
        }

        boolean result = false;
        if (mPath != null) {
            try {
                int iotype = IO_TYPE_FMQ;
                mSubtitleUtils = new SubtitleUtils(mPath);
                if (mSubtitleUtils.getSubID(0) != null) {
                    iotype = IO_TYPE_FILE;
                    mPath = mSubtitleUtils.getSubID(0).mFileName;
                    LOGI("[start]ext sub path:" + mPath);
                }
                result = open(mPath, iotype);
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }

            if (result) {
                show();

                if (optionEnable() ) {
                    option();//show subtitle select option add for debug
                }
            }
        }
    }

    public void close() {
        mDisplayType = -1;
        mCurrentTrack = 0;
        mCurrentCCchannel = 15;
        mMonitorCCchannel = 15;
        nativeClose();
    }

    public void destory() {
        Log.d(TAG, "destory:", new Throwable());
        mDisplayType = -1;
        mCurrentTrack = 0;
        mCurrentCCchannel = 15;
        mMonitorCCchannel = 15;
        nativeDestroy();
    }

    private void show() {
        LOGI("[show]mSubtitleUtils:" + mSubtitleUtils);
        if (mSubtitleUtils != null && mSubtitleUtils.getExSubTotal() > 0) {
            if (mThread == null || mThreadStop == true) {
                mThreadStop = false;
                mThread = new Thread (runnable);
                mThread.start();
            }
        }
    }

    private boolean isTvSubtile() {
        int isTvType = -1;
        isTvType = SystemProperties.getInt("vendor.sys.subtitleService.tvType", 3);
        if (isTvType == TV_SUB_H264 ||
            isTvType == TV_SUB_MPEG2 ||
            isTvType == TV_SUB_SCTE27 ||
            isTvType == TV_SUB_DVB) {
            return true;
        }
        return false;
    }

    public void option() {
        nativeOption();
    }

    public int total() {
        int extTotal = 0;
        if (mDisplayType == SUBTITLE_CC_JASON) {
            return mChalIdList.size();
        }

        if (mSubtitleUtils != null) {
            extTotal = mSubtitleUtils.getExSubTotal();
        }
        mSubTotal = nativeTotalSubtitles() + extTotal;
        LOGI("[total]mSubTotal:" + mSubTotal + ", extTotal:" + extTotal);
        return mSubTotal;
    }

    public int innerTotal() {
        return nativeInnerSubtitles();
    }

    public void next() {
        nativeNext();
    }

    public void previous() {
        nativePrevious();
    }

    public void hide() {
        runOnMainThread(() -> mUI.hideView());
    }

    public void enableDisplay() {
        runOnMainThread(() -> { mUI.clearContent(); mUI.setDisplayFlag(false); });
    }

    public void disableDisplay() {
        runOnMainThread(() -> mUI.setDisplayFlag(true));
    }

    public void display() {
        runOnMainThread(() -> mUI.displayView());
    }

    public void clear() {
        nativeClean();
    }

    public void resetForSeek() {
        runOnMainThread(() -> mUI.resetForSeek());
        nativeResetForSeek();
    }

    public int getSubHeight() {
        return nativeGetSubHeight();
    }

    public int getSubWidth() {
        return nativeGetSubWidth();
    }

    public void setSubType(int type) {
        nativeSetSubType(type);
    }

    public int getSubType() {
        return nativeGetSubType();
    }

    public String getSubTypeStr() {
        String type = null;
        Log.d(TAG, "TO BE IMPL..", new Throwable());

/*        try {
            if (mProxy != null) {
                type = mProxy.getSubTypeStr();
            }
        } catch (RemoteException e) {
            Log.i(TAG, ", getService:");
            getService();
        }
*/
        LOGI("[getSubTypeStr]type:" + type);
        return type;
    }

    public String getSubLanguage(int idx) {
        return nativeGetSubLanguage(idx);
    }

    public String getCurName() {
        return mPath;
    }

    public void release() {
        try {
            hide();
            close();
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
    }

    public void setPlayerType(int type) {
        nativeSetPlayerType(type);
    }
    public void setSubPid(int pid) {
        nativeSetSctePid(pid);
    }
    public int getSubTypeDetial() {
        return nativeGetSubTypeDetial();
    }

    public void setTextColor(int color) {
        runOnMainThread(() -> { mUI.setTextColor(color); });
    }

    public void setTextSize(int size) {
        runOnMainThread(() -> { mUI.setTextSize(size); });
    }

    public void setGravity(int gravity) {
        runOnMainThread(() -> { mUI.setGravity(gravity); });
    }

    public void setTextStyle(int style) {
        runOnMainThread(() -> { mUI.setTextStype(style); });
    }

    public void setPosHeight(int height) {
        runOnMainThread(() -> { mUI.setPosHeight(height); });
    }

    public void setImgSubRatio(float ratioW, float ratioH, int maxW, int maxH) {
        runOnMainThread(() -> { mUI.setImgSubRatio(ratioW, ratioH, maxW, maxH); });
    }

    public int ttGoHome() {
        return nativeTtGoHome();
    }

    public int ttGotoPage(int pageNo, int subPageNo) {
        return nativeTtGotoPage(pageNo, subPageNo);
    }
    public int ttNextPage(int dir) {
        return nativeTtNextPage(dir);
    }

    public int ttNextSubPage(int dir) {
        return nativeTtNextSubPage(dir);
    }

    public void load(String path) {
        nativeLoad(path);
    }

    public void setDisplayRect(int x, int y, int w, int h) {
        runOnMainThread(() -> {
            mDisplayRect.left = x;
            mDisplayRect.right = x + w;
            mDisplayRect.top = y;
            mDisplayRect.bottom = y + h;
            Log.d(TAG, "setDisplayRect=" + mDisplayRect);
        });
    }

    public void resetTrackIdx() {
        try {
            if (mInnerTrackIdx != null) {
                mInnerTrackIdx.clear();
            }
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
    }

    public void storeTrackIdx(int idx) {
        LOGI("[storeTrackIdx] idx:" + idx);
        try {
            mInnerTrackIdx.add(idx);
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }
        }

    public void setIOType(int type) {
        LOGI("[setIOType] type:" + type);
        Log.d(TAG, "Obsoleted Not IMPL..", new Throwable());
        //nativeSetIoType(type);
        /*try {
            if (mProxy != null) {
               // mProxy.setIOType(type);
            }
        } catch (RemoteException e) {
            Log.i(TAG, "getServer:");
            getService();
        }*/
    }

/*    private void setIOType() {
        LOGI("[setIOType]mMediaPlayer:" + mMediaPlayer);

        try {
            mIOType = getIoType();       //default amnuplayer
        if (mMediaPlayer != null) {
            String typeStr = mMediaPlayer.getStringParameter(mMediaPlayer.KEY_PARAMETER_AML_PLAYER_TYPE_STR);
            LOGI("[setIOType]typeStr:" + typeStr);
            if (typeStr != null && typeStr.equals("AMNU_PLAYER")) {
                mIOType = IO_TYPE_FMQ;
            }
        }
            setIOType(mIOType);
        } catch (Exception e) {
            Log.e(TAG, "Exception:" + e);
        }
    }
*/
    private int getIOType() {
        return mIOType;
    }

    private static final int AML_SUBTITLE_START = 800; // random value
    private class EventHandler extends Handler {
        public EventHandler (Looper looper) {
            super (looper);
        }

        @Override
        public void handleMessage (Message msg) {
            try {
                switch (msg.arg1) {
                case AML_SUBTITLE_START:
                    LOGI("[handleMessage]AML_SUBTITLE_START mPath:" + mPath);
                    if (mPath != null) {
                    boolean ret = open(mPath);

                        if (ret) {
                            show();

                            if (optionEnable() ) {
                                option();//show subtitle select option add for debug
                            }
                        }
                    }
                    break;
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }
        }
    }

    private String readSysfs(String path) {
        if (!new File (path).exists() ) {
            Log.e (TAG, "File not found: " + path);
            return null;
        }

        String str = null;
        StringBuilder value = new StringBuilder();

        try {
            FileReader fr = new FileReader (path);
            BufferedReader br = new BufferedReader (fr);

            try {
                while ( (str = br.readLine() ) != null) {
                    if (str != null) {
                        value.append (str);
                    }
                }
                fr.close();
                br.close();
                if (value != null) {
                    return value.toString();
                } else {
                    return null;
                }
            } catch (IOException e) {
                e.printStackTrace();
                return null;
            }
        } catch (FileNotFoundException e) {
            e.printStackTrace();
            return null;
        }
    }

    private Runnable runnable = new Runnable() {
        @Override
        public void run() {
            int pos = 0;

            while (!mThreadStop) {
                if (disable()) {
                    mThreadStop = true;
                    break;
                }

                //show subtitle
                if (mMediaPlayer != null && mMediaPlayer.isPlaying() ) {
                    pos = mMediaPlayer.getCurrentPosition() * 90;//convert to pts
                    //LOGI("[runnable]showSub:" + pos);
                }

                if (!mThreadStop) {
                    nativeUpdateVideoPos(pos);
                }

                try {
                    Thread.sleep (300 - (pos % 300) );
                } catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }
    };

    public void setSubtitleDataListner(SubtitleDataListener cb) {
        mHidlCallback= cb;
    }

    private void notifySubtitleEvent(int data[], byte[] subdata, int type, int x, int y,
            int width , int height, int videoWidth, int videoHeight, boolean show) {
        if (mHidlCallback != null) {
            Log.d(TAG, "onSubtitleEvent: mHidlCallback=" + mHidlCallback);
            mHidlCallback.onSubtitleEvent(type, data, subdata, x, y, width, height, videoWidth, videoHeight, show);
        } else if (mHidlFallbackDisplay != null) {
            Log.d(TAG, "onSubtitleEvent: mHidlFallbackDisplay=" + mHidlFallbackDisplay);
            mHidlFallbackDisplay.onSubtitleEvent(type, data, subdata, x, y, width, height, videoWidth, videoHeight, show);
        } else {
            Log.e(TAG, "Cannot handle events!");
        }
    }

    private void notifySubtitleUIEvent(int cmd, int params[]) {
        if (mHidlFallbackDisplay != null) {
            Log.d(TAG, "SubtitleUIEvent: mHidlFallbackDisplay=" + mHidlFallbackDisplay);
            mHidlFallbackDisplay.onUiCommandEvent(cmd, params);
        } else {
            Log.e(TAG, "Cannot handle events! only available in FallbackDisplay");
        }
    }

    private void notifySubtitleInfo(int what, int extra) {
        if (mHidlCallback != null) {
            Log.d(TAG, "onSubtitleEvent: mHidlCallback=" + mHidlCallback);
            //mHidlCallback.onSubtitleInfo(what, extra);
        } else if (mHidlFallbackDisplay != null) {
            Log.d(TAG, "onSubtitleEvent: mHidlFallbackDisplay=" + mHidlFallbackDisplay);
            mHidlFallbackDisplay.onSubtitleInfo(what, extra);
        } else {
            Log.e(TAG, "Cannot handle events!");
        }
    }

    void unzipUncry(File uncryteDir) {
        File zipFile = new File(mContext.getDataDir(),"fonts.zip");
        File unZipDir = new File(mContext.getDataDir(),unZipDirStr);
        try {
            copyToFileOrThrow(mContext.getAssets().open(fonts),zipFile);
            upZipFile(zipFile,unZipDir.getCanonicalPath());
            uncryteDir.mkdirs();
            nativeUnCrypt(unZipDir.getCanonicalPath()+"/", uncryteDir.getCanonicalPath()+"/");
        } catch(IOException ex) {
            ex.printStackTrace();
        }
    }

    private  void copyToFileOrThrow(InputStream inputStream, File destFile)
            throws IOException {
        if (destFile.exists()) {
            destFile.delete();
        }
        FileOutputStream out = new FileOutputStream(destFile);
        try {
            byte[] buffer = new byte[4096];
            int bytesRead;
            while ((bytesRead = inputStream.read(buffer)) >= 0) {
                out.write(buffer, 0, bytesRead);
            }
        } finally {
            out.flush();
            try {
                out.getFD().sync();
            } catch (IOException e) {
            }
            out.close();
        }
    }

    private  void upZipFile(File zipFile, String folderPath)
            throws ZipException, IOException {
            File desDir = new File(folderPath);
            if (!desDir.exists()) {
                desDir.mkdirs();
            }
            ZipFile zf = new ZipFile(zipFile);
            for (Enumeration<?> entries = zf.entries(); entries.hasMoreElements();) {
                ZipEntry entry = ((ZipEntry) entries.nextElement());
                InputStream in = zf.getInputStream(entry);
                String str = folderPath;
                File desFile = new File(str, java.net.URLEncoder.encode(
                        entry.getName(), "UTF-8"));
                if (!desFile.exists()) {
                    File fileParentDir = desFile.getParentFile();
                    if (!fileParentDir.exists()) {
                        fileParentDir.mkdirs();
                    }
                }
                OutputStream out = new FileOutputStream(desFile);
                byte buffer[] = new byte[1024 * 1024];
                int realLength = in.read(buffer);
                while (realLength != -1) {
                    out.write(buffer, 0, realLength);
                    realLength = in.read(buffer);
                }

                out.close();
                in.close();

            }
    }
    public interface SubtitleDataListener {
        /**
         * TO BE DEFINED LATER:
         * event: show or hidden event
         * serial: subtext id
         * type: sub types, such as string, bitmap, cc...
         * Object: data
         */
        public void onSubtitleEvent(int type, Object data, byte[] subdata, int x, int y,
            int width ,int height, int videoW, int videoH, boolean showOrHide);
    }


    /** This api not export to 3rd use **/
    public interface FallbackDisplayListener {
        // Received subtitle data
        public void onSubtitleEvent(int type, Object data, byte[] subdata, int x, int y,
                int width ,int height, int videoW, int videoH, boolean showOrHide);

        public void onSubtitleInfo(int what, int extra);

        public void onUiCommandEvent(int type, int params[]);
    }
}
