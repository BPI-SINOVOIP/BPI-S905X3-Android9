/*
* Copyright (c) 2014 Amlogic, Inc. All rights reserved.
*
* This source code is subject to the terms and conditions defined in the
* file 'LICENSE' which is part of this source code package.
*
* Description: java file
*/
package com.droidlogic.SubTitleService;

import android.util.Log;
import android.util.DisplayMetrics;

import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.ContextWrapper;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.IBinder;
import java.lang.ref.WeakReference;
import android.view.*;
import android.view.ViewGroup;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;
import android.view.WindowManager.LayoutParams;
import android.view.View.OnClickListener;
import android.widget.AdapterView.OnItemClickListener;
import android.graphics.*;
import com.subtitleparser.*;
import com.subtitleview.SubtitleView;
import com.tv.TvSubtitle;
import android.os.Environment;
import android.os.Handler;
import android.os.Message;
import android.os.RemoteException;
import android.os.SystemProperties;
import android.app.AlertDialog;
import android.webkit.URLUtil;
import android.widget.*;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.PrintWriter;
import java.lang.RuntimeException;
import java.net.URL;
import java.net.URLConnection;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;


import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.ISubTitleService;
import com.droidlogic.app.ISubTitleServiceCallback;

public class SubTitleService extends ISubTitleService.Stub {
    private String langCharset = "GBK";
    private static final String TAG = "SubTitleService";
    private static final String SUBTITLE_WIN_TITLE = "com.droidlogic.subtitle.win";
    private static final String DISPLAY_MODE_SYSFS = "/sys/class/display/mode";
    private SystemControlManager mSystemControl;

    private static final int OPEN = 0xF0; //random value
    private static final int SHOW_CONTENT = 0xF1;
    private static final int CLOSE = 0xF2;
    private static final int OPT_SHOW = 0xF3;
    private static final int SET_TXT_COLOR = 0xF4;
    private static final int SET_TXT_SIZE = 0xF5;
    private static final int SET_TXT_STYLE = 0xF6;
    private static final int SET_GRAVITY = 0xF7;
    private static final int SET_POS_HEIGHT = 0xF8;
    private static final int HIDE = 0xF9;
    private static final int DISPLAY = 0xFA;
    private static final int CLEAR = 0xFB;
    private static final int RESET_FOR_SEEK = 0xFC;
    private static final int LOAD = 0xFD;

    private static final int SET_IO_TYPE = 0xFE;
    private static final int TV_SUB_START = 0xE1;
    private static final int TV_SUB_STOP = 0xE2;

    private static final int SUB_OFF = 0;
    private static final int SUB_ON = 1;
    private static final int SUB_HIDE = 2;
    private int subShowState = SUB_OFF;

    private boolean mDebug = SystemProperties.getBoolean("sys.subtitleService.debug", true);
    private Context mContext;
    private View mSubView;
    private Display mDisplay;
    private WindowManager mWindowManager;
    private WindowManager.LayoutParams mWindowLayoutParams;
    private boolean isViewAdded = false;

    //avoid close continuously
    private boolean mIsClosed = false;

   //subtitleservice callback
   private ISubTitleServiceCallback mCallback = null;

    //for subtitle
    private SubtitleUtils mSubtitleUtils;
    private SubtitleView subTitleView = null;
    private int mSubTotal = -1;
    private int mCurSubId = 0;
    private int mSetSubId = -1;

    //for subtitle option
    private AlertDialog mOptionDialog;
    private ListView mListView;
    private int mCurOptSelect = 0;

    //for subtitle img ratio
    private boolean mRatioSet = false;

    //for window scale
    private float mRatio = 1.000f;
    private int mTextSize = 20;
    private int mBottomMargin = 50; //should sync with SubtitleView.xml

    //tv subtitle
    private TvSubtitle mTvSubtitle = null;

    private static final int TV_SUB_NONE = -1;
    private static final int TV_SUB_MPEG2 = 0;  //close caption mpeg2
    private static final int TV_SUB_H264 = 2;    //close caption h264
    private static final int TV_SUB_SCTE27 = 3;
    private static final int TV_SUB_DVB = 4;
    private int tvType = TV_SUB_NONE;

    //add for cc
    private int mVfmt= TV_SUB_H264;
    private int mChannelId = 15;  //detect closecaption

    private int mPid = -1;
    private int mPageId = -1;
    private int mAncPageId = -1;

    public SubTitleService(Context context) {
        LOGI("[SubTitleService]");
        mContext = context;
        init();
    }

    private void LOGI(String msg) {
        if (mDebug) Log.i(TAG, msg);
    }

    private boolean supportTvSubtile() {
        LOGI("[supportTvSubtile]tvType:" + tvType);
        if (tvType != TV_SUB_H264 &&
            tvType != TV_SUB_MPEG2 &&
            tvType != TV_SUB_SCTE27 &&
            tvType != TV_SUB_DVB) {
            return false;
        }
        return true;
    }

    private void setProp() {
        if (SystemProperties.getBoolean("vendor.sys.subtitleService.tvsub.enable", false)) {
            tvType = SystemProperties.getInt("vendor.sys.subtitleService.tvType", 2);
            mVfmt = SystemProperties.getInt("vendor.sys.subtitleService.closecaption.vfmt", 2);
            mChannelId = SystemProperties.getInt("vendor.sys.subtitleService.closecaption.channelId", 15);
            mPid = SystemProperties.getInt("vendor.sys.subtitleService.pid", -1);
            mAncPageId = SystemProperties.getInt("vendor.sys.subtitleService.ancPageId", 1);
            mPageId = SystemProperties.getInt("vendor.sys.subtitleService.pageId", 1);
        }
    }

    public void setClosedCaptionVfmt(int vfmt) {
         LOGI("[setClosedCaptionVfmt]vfmt:" + vfmt);
         mVfmt = vfmt;
    }
    public void setChannelId(int channelId) {
         LOGI("[setChannelId]channelId:" + channelId);
         mChannelId = channelId;
    }

    public void setAncPageId(int ancPageId) {
         LOGI("[setAncPageId]ancPageId:" + ancPageId);
         mAncPageId = ancPageId;
    }

    public void setPageId(int pageId) {
         LOGI("[setPageId]pageId:" + pageId);
         mPageId = pageId;
    }


    public void setSubPid(int pid) {
        LOGI("[setSubPid]pid:" + pid);
        mPid = pid;
    }

    public void setSubType(int type) {
        LOGI("[setSubType]type:" + type);
        tvType = type;
    }

    private void init() {
        LOGI("[init]");
        //get system control
        //mSystemControl = new SystemControlManager.getInstance();

        //init view
        mSubView = LayoutInflater.from(mContext).inflate(R.layout.subtitleview, null);
        long time2 = System.currentTimeMillis();

        subTitleView = (SubtitleView) mSubView.findViewById(R.id.subtitle);
        subTitleView.clear();
        subTitleView.setTextColor(Color.WHITE);
        subTitleView.setTextSize(20);
        subTitleView.setTextStyle(Typeface.NORMAL);
        subTitleView.setViewStatus(true);

        new Thread(new Runnable() {
            public void run() {
                subTitleView.startSocketServer();
            }
        }).start();

        //prepare window
        mWindowManager = (WindowManager)mContext.getSystemService (Context.WINDOW_SERVICE);
        mDisplay = mWindowManager.getDefaultDisplay();
        mWindowLayoutParams = new WindowManager.LayoutParams();
        mWindowLayoutParams.type = LayoutParams.TYPE_SYSTEM_DIALOG;
        mWindowLayoutParams.format = PixelFormat.TRANSLUCENT;
        mWindowLayoutParams.flags = LayoutParams.FLAG_NOT_TOUCH_MODAL
                  | LayoutParams.FLAG_NOT_FOCUSABLE
                  | LayoutParams.FLAG_LAYOUT_NO_LIMITS;
        mWindowLayoutParams.gravity = Gravity.LEFT | Gravity.TOP;
        mWindowLayoutParams.setTitle(SUBTITLE_WIN_TITLE);
        mWindowLayoutParams.x = 0;
        mWindowLayoutParams.y = 0;
        mWindowLayoutParams.width = mDisplay.getWidth();
        mWindowLayoutParams.height = mDisplay.getHeight();
    }

    private void addView() {
        LOGI("[addView]isViewAdded:" + isViewAdded);
        if (!isViewAdded) {
            if (SystemProperties.getBoolean("sys.subtitleservice.enableview", true)) {
                mWindowManager.addView(mSubView, mWindowLayoutParams);
                isViewAdded = true;
            }
        }
    }

    private void removeView() {
        LOGI("[removeView]isViewAdded:" + isViewAdded);
        if (isViewAdded) {
            mWindowManager.removeView(mSubView);
            isViewAdded = false;
        }
    }

    private void disableMediaSub() {
        boolean enable = true;
        if (!SystemProperties.getBoolean("vendor.sys.subtitleService.enable", true)) {
            mSubTotal = 0;
        }
    }
    public void open (String path) {
        LOGI("[open] path: " + path);
        mIsClosed = false;
        if (mOptionDialog != null) {
            mOptionDialog.dismiss();
        }
        setProp();
        StartTvSub();
        if (path != null && !path.equals("")) {
            File file = new File(path);
            String name = file.getName();
            if (name == null || (name != null && -1 == name.lastIndexOf('.'))) {
                return;
            }
            mSubtitleUtils = new SubtitleUtils(path);
        }
        else {
            mSubtitleUtils = new SubtitleUtils();
        }

        mSubTotal = mSubtitleUtils.getSubTotal();
        if (mSubTotal > 0) {
            subTitleView.SubtitleDecoderCountDown();
        }
        //disableMediaSub();
        //mCurSubId = mSubtitleUtils.getCurrentInSubtitleIndexByJni(); //get inner subtitle current index as default, 0 is always, if there is no inner subtitle, 0 indicate the first external subtitle
        if (mSetSubId >= 0) {
            mCurSubId = mSetSubId;
            mSetSubId = -1;
        }
        LOGI("[open] mCurSubId: " + mCurSubId+ ",mSubTotal:"+mSubTotal);
        if (mSubTotal > 0) {
            sendOpenMsg(mCurSubId);
        }

        //load("http://milleni.ercdn.net/9_test/double_lang_test.xml"); for test
        //sendOptionMsg();
    }

    class StopSubSocketThread extends Thread {
        public void run() {
            synchronized(this) {
                mHandler.post(new Runnable() {
                    @Override
                    public void run() {
                        subTitleView.stopSocketServer();
                        if (mSubTotal > 0) {
                            subTitleView.stopSubThread(); //close insub parse thread
                            subTitleView.closeSubtitle();
                            subTitleView.clear();
                        }
                        removeView();
                        mSubTotal = -1;
                    }
                });
            }
        }
     }


    public void close() {
        LOGI("[close]");
        if (supportTvSubtile() && mTvSubtitle != null)
            StopTvSub();
        if (mHandler != null) {
            mHandler.removeMessages (TV_SUB_START);
        }
        removeView();
        //flag mIsClosed replace flag subShowState, otherwise result to 168720
        if (mIsClosed)
            return;
        mIsClosed = true;

        if (mSubtitleUtils != null) {
            //mSubtitleUtils.setSubtitleNumber(0);
            mSubtitleUtils = null;
        }
        new StopSubSocketThread().start();
        mSetSubId = -1;
        subShowState = SUB_OFF;
        mRatioSet = false;
    }

    public void destory() {
        LOGI("[destory]");
        if (supportTvSubtile() && mTvSubtitle != null) {
            removeView();
            mTvSubtitle.stopAll();
        }
    }

    public int getSubTotal() {
        LOGI("[getSubTotal] mSubTotal:" + mSubTotal);
        return mSubTotal;
    }

    public int getInnerSubTotal() {
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getInSubTotal();
        }
        return 0;
    }

    public int getExternalSubTotal() {
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getExSubTotal();
        }
        return 0;
    }

    public void nextSub() { // haven't test
        LOGI("[nextSub]mCurSubId:" + mCurSubId + ",mSubTotal:" + mSubTotal);
        if (mSubTotal > 0) {
            mCurSubId++;
            if (mCurSubId >= mSubTotal) {
                mCurSubId = 0;
            }
            sendOpenMsg(mCurSubId);
        }
    }

    public void preSub() { // haven't test
        LOGI("[preSub]mCurSubId:" + mCurSubId + ",mSubTotal:" + mSubTotal);
        if (mSubTotal > 0) {
            mCurSubId--;
            if (mCurSubId < 0) {
                mCurSubId = mSubTotal - 1;
            }
            sendOpenMsg(mCurSubId);
        }
    }

    public void registerCallback(ISubTitleServiceCallback cb) throws RemoteException {
        LOGI("[registerCallback]");
        //mCallback = cb;
        //SubtitleView.setCallback(cb);
        //TvSubtitle.setCallback(cb);
    }

    public void unregisterCallback() throws RemoteException {
        mCallback = null;
    }

    public void openIdx(int idx) {
        LOGI("[openIdx]idx:" + idx);
        if (supportTvSubtile() && mTvSubtitle != null && idx >= 0) {
            mTvSubtitle.switchCloseCaption(tvType, mVfmt, mChannelId);
            return;
        }
        if (idx >= 0 && idx < mSubTotal) {
            mCurSubId = idx;
            sendOpenMsg(idx);
        } else {
            mSetSubId = idx;
        }
    }

    public boolean isInnerSub() {
        boolean ret = false;
        if (getSubTotal() > 0) {
            if (getInnerSubTotal() > 0 && mCurSubId < getInnerSubTotal()) {
                ret = true;
            }
        }
        LOGI("[isInnerSub]ret:" + ret);
        return ret;
    }

    //for scte27 height
    public int getSubHeight() {
        if (supportTvSubtile() && mTvSubtitle != null) {
            return mTvSubtitle.getSubHeight();
        }
        return -1;
    }
    //for scte27 width
    public int getSubWidth() {
        if (supportTvSubtile() && mTvSubtitle != null) {
            return mTvSubtitle.getSubWidth();
        }
        return -1;
    }

    public void showSub(int position) {
        LOGI("[showSub]position:" + position);
        if (position >= 0) {
            sendShowSubMsg(position);
        }
    }

    public int getSubType() {
        return subTitleView.getSubType();
    }

    public String getSubTypeStr() {
        return subTitleView.getSubTypeStr();
    }

    public String getExtSubTypeAll() {
        //return subTitleView.getSubTypeStr();
        LOGI("[getExtSubTypeStrAll]");
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getExtSubTypeStrAll();
        }
        return null;
    }

    public String getInBmpTxtType() {
        LOGI("[getInBmpTxtType]");
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getInBmpTxtType();
        }
        return null;
    }

    public String getInSubLanAll() {
        LOGI("[getInSubLanStr]");
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getInSubLanStr();
        }
        return null;
    }

    public String getExtBmpTxtType() {
        LOGI("[getInBmpTxtType]");
        if (mSubtitleUtils != null) {
            return mSubtitleUtils.getExtBmpTxtType();
        }
        return null;
    }

    public int getSubTypeDetial() {
        return subTitleView.getSubTypeDetial();
    }

    public void setTextColor (int color) {
        sendSetTxtColorMsg(color);
    }

    public void setTextSize (int size) {
        mTextSize = size;
        sendSetTxtSizeMsg(size);
    }

    public void setTextStyle (int style) {
        sendSetTxtStyleMsg(style);
    }

    public void setGravity (int gravity) {
        sendSetGravityMsg(gravity);
    }

    public void setPosHeight (int height) {
        sendSetPosHeightMsg(height);
    }

    public void hide() {
        sendHideMsg();
    }

    public void display() {
        sendDisplayMsg();
    }

    public void clear() {
        sendClearMsg();
    }

    public void resetForSeek() {
        sendResetForSeekMsg();
    }

    public void setIOType(int type) {
        sendSetIOTypeMsg(type);
    }

    public String getPcrscr() {
        return subTitleView.getPcrscr();
    }

    public void option() {
        sendOptionMsg();
    }

    public void setImgSubRatio (float ratioW, float ratioH, int maxW, int maxH) {
        LOGI("[setImgSubRatio] ratioW:" + ratioW + ", ratioH:" + ratioH + ",maxW:" + maxW + ",maxH:" + maxH);
        //subTitleView.setImgSubRatio(ratioW, ratioH, maxW, maxH);
    }

    public String getCurName() {
        SubID subID = mSubtitleUtils.getSubID(mCurSubId);
        if (subID != null) {
            LOGI("[getCurName]name:" + subID.filename);
            return subID.filename;
        }
        return null;
    }

    public String getSubName(int idx) {
        String name = null;

        if (idx >= 0 && idx < mSubTotal && mSubtitleUtils != null) {
            name = mSubtitleUtils.getSubPath(idx);
            if (name != null) {
                int index = name.lastIndexOf("/");
                if (index >= 0) {
                    name = name.substring(index + 1);
                }
                else {
                    if (name.equals("INSUB")) {
                        name = mSubtitleUtils.getInSubName(idx);
                    }
                }
            }
        }
        LOGI("[getSubName]idx:" + idx + ",name:" + name);
        return name;
    }

    public String getSubLanguage(int idx) {
        String language = null;
        if (tvType != TV_SUB_NONE && mTvSubtitle != null) {
            LOGI("[getSubLanguage]");
            return mTvSubtitle.getSubLanguage();
        }
        int index = 0;

        if (idx >= 0 && idx < mSubTotal && mSubtitleUtils != null) {
            language = mSubtitleUtils.getSubPath(idx);
            if (language != null) {
                index = language.lastIndexOf(".");
                if (index >= 0) {
                    language = language.substring(0, index);
                    index = language.lastIndexOf(".");
                    if (index >= 0) {
                        language = language.substring(index + 1);
                    }
                }
            }
            if (language.equals("INSUB")) {
                language = mSubtitleUtils.getInSubLanguage(idx);
            }
        }
        if (language != null) { // if no language, getSubName (skip file path)
            index = language.lastIndexOf("/");
            if (index >= 0) {
                language = getSubName(idx);
            }
        }
        return language;
    }

    public void StartTvSub() {
        if (supportTvSubtile() && mTvSubtitle == null) {
            //setTvSubtitleType();
            mTvSubtitle = new TvSubtitle(mContext, mSubView);
            sendStartTvSubMsg();
        }
    }

    public void StopTvSub() {
        sendStopTvSubMsg();
    }

    private void sendStartTvSubMsg() {
        Message msg = mHandler.obtainMessage(TV_SUB_START);
        mHandler.sendMessage(msg);
    }

    private void sendStopTvSubMsg() {
        Message msg = mHandler.obtainMessage(TV_SUB_STOP);
        mHandler.sendMessage(msg);
    }

    private void sendOpenMsg(int idx) {
        if (mSubtitleUtils != null) {
            Message msg = mHandler.obtainMessage(OPEN);
            SubID subID = mSubtitleUtils.getSubID(idx);
            if (subID != null) {
                msg.obj = subID;
                mHandler.sendMessage(msg);
            }
        }
    }

    private void sendCloseMsg() {
        Message msg = mHandler.obtainMessage(CLOSE);
        mHandler.sendMessage(msg);
    }

    private void sendShowSubMsg(int pos) {
        Message msg = mHandler.obtainMessage(SHOW_CONTENT);
        msg.arg1 = pos;
        mHandler.sendMessage(msg);
    }

    private void sendOptionMsg() {
        Message msg = mHandler.obtainMessage(OPT_SHOW);
        if (mSubTotal > 0) {
            mHandler.sendMessage(msg);
        }
    }

    private void sendLoadMsg(String path) {
        Message msg = mHandler.obtainMessage(LOAD);
        if (path != null) {
            msg.obj = path;
            mHandler.sendMessage(msg);
        }
    }

    private void sendSetTxtColorMsg(int color) {
        Message msg = mHandler.obtainMessage(SET_TXT_COLOR);
        msg.arg1 = color;
        mHandler.sendMessage(msg);
    }

    private void sendSetTxtSizeMsg(int size) {
        Message msg = mHandler.obtainMessage(SET_TXT_SIZE);
        msg.arg1 = size;
        mHandler.sendMessage(msg);
    }

    private void sendSetTxtStyleMsg(int style) {
        Message msg = mHandler.obtainMessage(SET_TXT_STYLE);
        msg.arg1 = style;
        mHandler.sendMessage(msg);
    }

    private void sendSetGravityMsg(int gravity) {
        Message msg = mHandler.obtainMessage(SET_GRAVITY);
        msg.arg1 = gravity;
        mHandler.sendMessage(msg);
    }

    private void sendSetPosHeightMsg(int height) {
        Message msg = mHandler.obtainMessage(SET_POS_HEIGHT);
        msg.arg1 = height;
        mHandler.sendMessage(msg);
    }

    private void sendHideMsg() {
        Message msg = mHandler.obtainMessage (HIDE);
        mHandler.sendMessage(msg);
    }

    private void sendDisplayMsg() {
        Message msg = mHandler.obtainMessage(DISPLAY);
        mHandler.sendMessage(msg);
    }

    private void sendClearMsg() {
        Message msg = mHandler.obtainMessage(CLEAR);
        mHandler.sendMessage(msg);
    }

    private void sendResetForSeekMsg() {
        Message msg = mHandler.obtainMessage(RESET_FOR_SEEK);
        mHandler.sendMessage(msg);
    }

    private void sendSetIOTypeMsg(int type) {
        Message msg = mHandler.obtainMessage(SET_IO_TYPE);
        msg.arg1 = type;
        mHandler.sendMessage(msg);
    }

    private void removeMsg() {
        mHandler.removeMessages(SHOW_CONTENT);
        mHandler.removeMessages(OPT_SHOW);
        mHandler.removeMessages(OPEN);
        mHandler.removeMessages(LOAD);
    }

    //http://milleni.ercdn.net/9_test/double_lang_test.xml
    private String downloadXmlFile(String strURL) {
        String filePath = null;

        //"/storage/sdcard0/Download";
        String dirPath = Environment.getExternalStorageDirectory().getPath() + "/" + Environment.DIRECTORY_DOWNLOADS;
        File baseFile = new File(dirPath);
        if (!baseFile.isDirectory() && !baseFile.mkdir() ) {
            Log.e(TAG, "[downloadXmlFile] unable to create external downloads directory " + baseFile.getPath());
            return null;
        }

        try {
            if (!URLUtil.isNetworkUrl(strURL)) {
                LOGI("[downloadXmlFile] is not network Url, strURL: " + strURL);
            } else {
                URL myURL = new URL(strURL);
                URLConnection conn = myURL.openConnection();
                conn.connect();
                InputStream is = conn.getInputStream();
                if (is == null) {
                    Log.e(TAG, "[downloadXmlFile] stream is null");
                }

                String fileName = strURL.substring(strURL.lastIndexOf("/") + 1);
                if (fileName != null) {
                    filePath = dirPath + "/" + fileName;
                    LOGI("[downloadXmlFile] filePath: " + filePath);
                    File file = new File(filePath);
                    if (!file.exists()) {
                        file.createNewFile();
                    } else {
                        file.delete();
                    }
                    FileOutputStream fos = new FileOutputStream(filePath);
                    byte buf[] = new byte[128];
                    do {
                        int numread = is.read(buf);
                        if (numread <= 0) {
                            break;
                        }
                        fos.write(buf, 0, numread);
                    } while (true);
                    is.close();
                    fos.close();
                }
            }
        } catch (Exception e) {
            e.printStackTrace();
        }
        return filePath;
    }

    public boolean load(String path) {
        boolean ret = false;
        final String urlPath = path; // for subtitle which should download from net

        if ((path.startsWith("http://") || path.startsWith("https://")) /*&& (path.endsWith("xml"))*/) {
            ret = true; // url subtitle return true
            Runnable r = new Runnable() {
                public void run() {
                    try {
                        String pathTmp = downloadXmlFile(urlPath);
                        String pathExtTmp = pathTmp.substring(pathTmp.lastIndexOf ('.') + 1);
                        //LOGI("[load] pathExtTmp: " + pathExtTmp + ", pathTmp:" + pathTmp);
                        for (String ext : SubtitleUtils.extensions) {
                            if (pathExtTmp.toLowerCase().equals(ext)) {
                                sendLoadMsg(pathTmp);
                                break;
                            }
                        }
                    } catch (Exception e) {
                        Log.e (TAG, e.getMessage(), e);
                    }
                }
            };
            new Thread (r).start();
        } else {
            String pathExt = path.substring(path.lastIndexOf ('.') + 1);
            //LOGI("[load] pathExt: " + pathExt + ", path:" + path);
            for (String ext : SubtitleUtils.extensions) {
                if (pathExt.toLowerCase().equals(ext) ) {
                    sendLoadMsg(path);
                    ret = true;
                    break;
                }
            }
        }
        return ret;
    }

    public void setSurfaceViewParam(int x, int y, int w, int h) {
        String mode = mSystemControl.readSysFs(DISPLAY_MODE_SYSFS).replaceAll("\n","");
        int[] curPosition = mSystemControl.getPosition(mode);
        int modeW = curPosition[2];
        int modeH = curPosition[3];
        int fbW = mDisplay.getWidth();
        int fbH = mDisplay.getHeight();

        if (modeW == 0 || modeH == 0 || w == 0 || h  == 0) {
            return;
        }

        removeView();//view will add in message show content handler

        float ratioViewW = ((float)w) / modeW;
        float ratioViewH = ((float)h) / modeH;
        float ratioFBW = ((float)fbW) /modeW;
        float ratioFBH = ((float)fbH) /modeH;

        // update image subtitle size
        if (ratioViewW >= ratioViewH) {
            mRatio = ratioViewW;
        }
        else {
            mRatio = ratioViewH;
        }
        RelativeLayout.LayoutParams layoutParams = (RelativeLayout.LayoutParams) subTitleView.getLayoutParams();
        layoutParams.bottomMargin=(int)(mBottomMargin * ratioFBH * ratioViewH);
        subTitleView.setLayoutParams(layoutParams);
        mWindowLayoutParams.x = (int)(x * ratioFBW);
        mWindowLayoutParams.y = (int)(y * ratioFBH);
        mWindowLayoutParams.width = (int)(w * ratioFBW);
        mWindowLayoutParams.height = (int)(h * ratioFBH);
        LOGI("[setSurfaceViewParam]x:" + mWindowLayoutParams.x + ",y:" + mWindowLayoutParams.y + ",width:" + mWindowLayoutParams.width + ",height:" + mWindowLayoutParams.height);
        subTitleView.setImgSubRatio(mRatio);

        // update text subtitle size
        setTextSize((int)(mTextSize * mRatio));
        LOGI("[setSurfaceViewParam]mRatio:" + mRatio + ", mTextSize:" + mTextSize);
    }

    private void adjustImgRatioDft() {
        float ratio = 1.000f;
        float ratioMax = 2.000f;
        float ratioMin = 0.800f;

        if (mRatioSet) {
            return;
        }

        int originW = subTitleView.getOriginW();
        int originH = subTitleView.getOriginH();
        LOGI("[adjustImgRatioDft] originW: " + originW + ", originH:" + originH);
        if (originW <= 0 || originH <= 0) {
            return;
        }

        //String mode = mSystemWriteManager.readSysfs(DISPLAY_MODE_SYSFS).replaceAll("\n","");
        //int[] curPosition = mMboxOutputModeManager.getPosition(mode);
        //int modeW = curPosition[2];
        //int modeH = curPosition[3];
        //LOGI("[adjustImgRatioDft] modeW: " + modeW + ", modeH:" + modeH);
        DisplayMetrics dm = new DisplayMetrics();
        dm = mContext.getResources().getDisplayMetrics();
        int frameW = dm.widthPixels;
        int frameH = dm.heightPixels;
        LOGI("[adjustImgRatioDft] frameW: " + frameW + ", frameH:" + frameH);

        if (frameW > 0 && frameH > 0) {
            float ratioW = ((float)frameW / (float)originW);
            float ratioH = ((float)frameH / (float)originH);
            LOGI("[adjustImgRatioDft] ratioW: " + ratioW + ", ratioH:" + ratioH);
            if (ratioW > ratioH) {
                ratio = ratioH;
            }
            else if (ratioW <= ratioH) {
                ratio = ratioW;
            }

            if (ratio >= ratioMax) {
                ratio = ratioMax;
            }
            else if (ratio <= ratioMin) {
                ratio = ratioMin;
            }
        }
        LOGI("[adjustImgRatioDft] ratio: " + ratio);
        if (ratio > 0) {
            subTitleView.setImgSubRatio(ratio * mRatio);
        }
        mRatioSet = true;
    }

    public String setSublanguage() {
        String type = null;
        String able = mContext.getResources().getConfiguration().locale.getCountry();
        LOGI("[setSublanguage] able: " + able);
        if (able.equals("TW")) {
            type = "BIG5";
        } else if (able.equals("JP")) {
            type = "cp932";
        } else if (able.equals("KR")) {
            type = "cp949";
        } else if (able.equals("IT") || able.equals("FR") || able.equals("DE")) {
            type = "iso88591";
        } else if (able.equals("TR")) {
            type = "cp1254";
        } else if (able.equals("PC")) {
            type = "cp1098";// "cp1097";
        } else if (able.equals("EG")) {
            type = "CP1256";
        } else if (able.equals("PT")) {
            type = "cp1252";
        } else if (able.equals("IL")) {
            type = "Windows-1255";
        } else {
            type = "GBK";
        }
        LOGI("[setSublanguage] type: " + type);
        return type;
    }

    /*public void setSublanguage(String type) {
        langCharset = type;
        sendOpenMsg(mCurSubId);
        ///SubID subID1 = mSubtitleUtils.getSubID(mCurSubId);
    }*/

    private void openFile(SubID subID) {
        Log.e(TAG, "[openFile]subID.index:"+subID.index);
        if (subID == null) {
            return;
        }
        try {
            ///String able = mContext.getResources().getConfiguration().locale.getCountry();
            if (subTitleView.setFile(subID, setSublanguage()) == Subtitle.SUBTYPE.SUB_INVALID) {
                return;
            }
            if (mSubtitleUtils != null) {
                //mSubtitleUtils.setSubtitleNumber(subID.index);
            }
        } catch (Exception e) {
            Log.e(TAG, "open:error");
            e.printStackTrace();
        }
    }

    @Override
    public void dump(FileDescriptor fd, PrintWriter pw, String[] args) {
        //LOGI("[dump]fd:" + fd.getInt$() + ",mSubtitleUtils:" + mSubtitleUtils);
        /*if (mSubtitleUtils != null) {
            mSubtitleUtils.nativeDump(fd.getInt$());
        }*/
    }

    private Handler mHandler = new Handler() {
        @Override
        public void handleMessage (Message msg) {

            switch (msg.what) {
                case SHOW_CONTENT:
                    if (subShowState == SUB_ON && mSubTotal > 0) {
                        addView();
                        adjustImgRatioDft();
                        subTitleView.setInnerSub(isInnerSub());
                        subTitleView.tick (msg.arg1);
                    }
                    break;

                case OPEN:
                    if (subShowState == SUB_ON) {
                        removeView();
                        subTitleView.stopSubThread(); //close insub parse thread
                        subTitleView.closeSubtitle();
                        subTitleView.clear();
                        mRatioSet = false;
                    }

                    subTitleView.startSubThread(); //open insub parse thread
                    openFile((SubID)msg.obj);
                    subTitleView.setVisibility(View.VISIBLE);
                    subShowState = SUB_ON;
                    break;

                case CLOSE:
                    //if (subShowState == SUB_ON) {
                        removeView();
                        subTitleView.stopSubThread(); //close insub parse thread
                        subTitleView.closeSubtitle();
                        subTitleView.clear();
                        subShowState = SUB_OFF;
                        mRatioSet = false;
                    //}
                    break;

                case SET_TXT_COLOR:
                    subTitleView.setTextColor(msg.arg1);
                    break;

                case SET_TXT_SIZE:
                    subTitleView.setTextSize(msg.arg1);
                    break;

                case SET_TXT_STYLE:
                    subTitleView.setTextStyle(msg.arg1);
                    break;

                case SET_GRAVITY:
                    subTitleView.setGravity(msg.arg1);
                    break;

                case SET_POS_HEIGHT:
                    subTitleView.setPadding (
                        subTitleView.getPaddingLeft(),
                        subTitleView.getPaddingTop(),
                        subTitleView.getPaddingRight(),
                        msg.arg1);
                    break;

                case HIDE:
                    if (View.VISIBLE == subTitleView.getVisibility()) {
                        subTitleView.setVisibility (View.GONE);
                        subShowState = SUB_HIDE;
                    }
                    if (supportTvSubtile() && mTvSubtitle != null) {
                        mTvSubtitle.hide();
                    }
                    break;

                case DISPLAY:
                    if (View.VISIBLE != subTitleView.getVisibility()) {
                        subTitleView.setVisibility (View.VISIBLE);
                        subShowState = SUB_ON;
                    }
                    if (supportTvSubtile() && mTvSubtitle != null) {
                        mTvSubtitle.show();
                    }
                    break;

                case CLEAR:
                    subTitleView.clear();
                    break;

                case RESET_FOR_SEEK:
                    subTitleView.resetForSeek();
                    break;

                case OPT_SHOW:
                    showOptionOverlay();
                    break;

                case LOAD:
                    if (subShowState == SUB_ON) {
                        removeView();
                        subTitleView.stopSubThread(); //close insub parse thread
                        subTitleView.closeSubtitle();
                        subTitleView.clear();
                        mRatioSet = false;
                    }

                    try {
                        ///String able = mContext.getResources().getConfiguration().locale.getCountry();
                        subTitleView.loadSubtitleFile((String)msg.obj, setSublanguage());
                        subShowState = SUB_ON;
                    } catch (Exception e) {
                        Log.e(TAG, "load:error");
                        e.printStackTrace();
                    }
                    break;

                case SET_IO_TYPE:
                    subTitleView.setIOType(msg.arg1);

                case TV_SUB_START:
                    if (supportTvSubtile() && mTvSubtitle != null) {
                        addView();
                        mPid = (tvType == TV_SUB_SCTE27 ||tvType == TV_SUB_DVB)  ? mPid : 0;
                        mTvSubtitle.start(tvType, mVfmt, mChannelId, mPid, mPageId, mAncPageId);
                    }
                    break;

               case TV_SUB_STOP:
                    if (supportTvSubtile() && mTvSubtitle != null) {
                        removeView();
                        mTvSubtitle.stopAll();
                    }
                    break;

                default:
                    Log.e(TAG, "[handleMessage]error msg.");
                    break;
            }
        }
    };

    private void showOptionOverlay() {
        LOGI("[showOptionOverlay]");

        //show dialog
        View view = View.inflate(mContext, R.layout.option, null);
        AlertDialog.Builder builder = new AlertDialog.Builder (mContext);
        builder.setView(view);
        builder.setTitle(R.string.option_title_str);
        mOptionDialog = builder.create();
        mOptionDialog.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        mOptionDialog.show();

        //adjust Attributes
        LayoutParams lp = mOptionDialog.getWindow().getAttributes();
        int w = mDisplay.getWidth();
        int h = mDisplay.getHeight();
        if (h > w) {
            lp.width = (int) (w * 1.0);
        } else {
            lp.width = (int) (w * 0.5);
        }
        mOptionDialog.getWindow().setAttributes(lp);

        mListView = (ListView) view.findViewById(R.id.list_view);
        SimpleAdapter adapter = new SimpleAdapter(mContext,
            getListData(),
            R.layout.list_item,
            new String[] {"item_text", "item_img"},
            new int[] {R.id.item_text, R.id.item_img});
        mListView.setAdapter(adapter);

        /*set listener */
        mListView.setOnItemClickListener (new OnItemClickListener() {
            public void onItemClick (AdapterView<?> parent, View view, int pos, long id) {
                LOGI("[option select]select subtitle " + (pos - 1) );
                if (pos == 0) { //first is close subtitle showing
                    sendHideMsg();
                } else if (pos > 0) {
                    mCurSubId = (pos - 1);
                    sendOpenMsg(pos - 1);
                }
                mCurOptSelect = pos;
                updateListDisplay();
                mOptionDialog.dismiss();
            }
        });
    }

    private List<Map<String, Object>> getListData() {
        List<Map<String, Object>> list = new ArrayList<Map<String, Object>>();
        boolean clsItmAdded = false;
        String trackStr = mContext.getResources().getString (R.string.opt_sub_track);
        String closeStr = mContext.getResources().getString (R.string.opt_close);

        for (int i = 0; i < mSubTotal; i++) {
            if (!clsItmAdded) {
                //add close subtitle item
                Map<String, Object> mapCls = new HashMap<String, Object>();
                clsItmAdded = true;
                mapCls.put ("item_text", closeStr);
                if (mCurOptSelect == 0) {
                    mapCls.put("item_img", R.drawable.item_img_sel);
                } else {
                    mapCls.put("item_img", R.drawable.item_img_unsel);
                }
                list.add (mapCls);
            }

            Map<String, Object> map = new HashMap<String, Object>();
            String subTrackStr = trackStr + Integer.toString(i);
            map.put ("item_text", subTrackStr);
            //LOGI("[getListData]map.put[" + i + "]:" + subTrackStr + ",mCurOptSelect:" + mCurOptSelect);

            if (mCurOptSelect == (i + 1)) {
                map.put("item_img", R.drawable.item_img_sel);
            } else {
                map.put("item_img", R.drawable.item_img_unsel);
            }
            list.add(map);
        }
        return list;
    }

    private void updateListDisplay() {
        Map<String, Object> list_item;
        for (int i = 0; i < mListView.getAdapter().getCount(); i++) {
            list_item = (Map<String, Object>) mListView.getAdapter().getItem(i);
            if (mCurOptSelect == i) {
                list_item.put("item_img", R.drawable.item_img_sel);
            } else {
                list_item.put("item_img", R.drawable.item_img_unsel);
            }
        }
        ((BaseAdapter) mListView.getAdapter()).notifyDataSetChanged();
    }
}
