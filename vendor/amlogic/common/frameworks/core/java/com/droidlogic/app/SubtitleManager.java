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
//import android.os.ServiceManager;
import android.os.SystemProperties;
import android.provider.MediaStore;
import android.util.Log;
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

import android.content.ComponentName;
import android.content.Intent;
import android.content.ServiceConnection;


import com.droidlogic.app.MediaPlayerExt;

public class SubtitleManager {
        private String TAG = "SubtitleManager";
        private boolean mDebug = false;
        private MediaPlayerExt mMediaPlayer = null;
        private ISubTitleService mService = null;
        private boolean mInvokeFromMp = false;
        private boolean mThreadStop = true;
        private String mPath = null;
        private Thread mThread = null;
        private int RETRY_MAX = 10;
        private int TV_SUB_MPEG2 = 0;  //close caption mpeg2
        private int TV_SUB_H264 = 2;    //close caption h264
        private int TV_SUB_SCTE27 = 3;
        private int TV_SUB_DVB = 4;
        private boolean mOpen = false;

        private Context mContext;

        public static SubtitleDataListener mHidlCallback;

        Class<?> objActivityThread = null;
        Method currentApplication = null;

        //IOType, should sync with \vendor\amlogic\apps\SubTitle\jni\subtitle\sub_io.h IOType
        public static final int IO_TYPE_DEV = 0;
        public static final int IO_TYPE_SOCKET = 1;
        private int mIOType = IO_TYPE_DEV;
        private ArrayList<Integer> mInnerTrackIdx = null;

        private Intent intent = null;

        public SubtitleManager () {
            getContext();
            mInnerTrackIdx = new ArrayList<Integer>();
            mDebug = false;
            checkDebug();
            if (!disable()) {
                getService();
            }
        }

        public SubtitleManager (Context context) {
            mContext = context;
            //mMediaPlayer = mp;
            mInnerTrackIdx = new ArrayList<Integer>();
            mDebug = false;
            checkDebug();
            if (!disable()) {
                getService();
            }
        }

        public SubtitleManager (Context context, MediaPlayerExt mp) {
            mContext = context;
            mMediaPlayer = mp;
            mInnerTrackIdx = new ArrayList<Integer>();
            mDebug = false;
            checkDebug();
            if (!disable()) {
                getService();
            }
        }

        public void getContext () {
            Log.i(TAG, "[getContext]");
            try {
                objActivityThread = Class.forName("android.app.ActivityThread");
                currentApplication = objActivityThread.getMethod("currentApplication");
                mContext = ((Application) (currentApplication.invoke(objActivityThread))).getApplicationContext();
            }
            catch (Exception ex) {
                Log.e(TAG, "getContext failed:" + ex);
            }

        }

        //for CTC
        public void showSub(int pos) {
            LOGI("[showSub]mService:" + mService);
            try {
                if (mService != null) {
                    mService.showSub(pos);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
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

        private void checkDebug() {
            try {
                if (SystemProperties.getBoolean ("vendor.sys.subtitle.debug", true) ) {
                    mDebug = true;
                }
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }
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

        public ISubTitleServiceCallback mCallback = new ISubTitleServiceCallback.Stub() {
            @Override
            public void onSubTitleEvent(int event, int id) throws RemoteException {
                Log.i(TAG, "[onSubTitleEvent]event:" + event + ",id:" + id);
                if (mHidlCallback != null) {
                    Log.i(TAG, "[onSubTitleEvent]-1-");
                    mHidlCallback.onSubTitleEvent(event, id);
                }
                //SubtitleServerManager.subTitleIdCallback(event, id);
            }

            @Override
            public void onSubTitleAvailable(int available) throws RemoteException {
                Log.i(TAG, "[onSubTitleAvailable]available:" + available);
                if (mHidlCallback != null) {
                    mHidlCallback.onSubTitleAvailable(available);
                }
                //SubtitleServerManager.subTitleAvailableCallback(available);
            }
        };

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
                final Uri uri = Uri.parse (path);
                if ("file".equals (uri.getScheme())) {
                    path = uri.getPath();
                }
                    mPath = path;
            } catch (Exception e) {
                Log.e(TAG, "Exception:" +e);
            }
        }

        public int open (String path) {
            int ret = -1;
            LOGI("[open] path:" + path + ", mService:" + mService);
            if (path.startsWith ("/data/") || path.equals ("") ) {
                ret = -1;
            }

            try {
                if (mService != null) {
                    mService.open (path);
                    ret = 0;
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
            LOGI("[open] innerTotal:" + innerTotal() +", mIOType:" + mIOType);
            if (innerTotal() > 0 && mIOType == IO_TYPE_SOCKET && mMediaPlayer != null) {
                mMediaPlayer.selectTrack(mInnerTrackIdx.get(0));
            }

            return ret;
        }

        public void openIdx (int idx) {
            LOGI("[openIdx] idx:" + idx +", mService:" + mService);
            if (idx < 0) {
                return;
            }

            try {
                if (mService != null) {
                    mService.openIdx (idx);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            if (idx < innerTotal() && mIOType == IO_TYPE_SOCKET && mMediaPlayer != null) {
                mMediaPlayer.selectTrack(mInnerTrackIdx.get(idx));
            }
        }

        public void start() {
            LOGI("[start]mPath:" + mPath);

            if (disable()) {
                return;
            }

            //mThreadStop = false;
            if (mPath != null) {
                if (!mOpen) {
                try {
                    setIOType();//first set io type to distinguish subtitle buffer source from dev or socket
                    mOpen = (open (mPath) == 0) ? true : false;
                } catch (Exception e) {
                    Log.e(TAG, "Exception:" + e);
                }
            }
                if (mOpen) {
                    show();

                    if (optionEnable() ) {
                        option();//show subtitle select option add for debug
                    }
                }
            }
        }

        public void close() {
            LOGI("[close]mService:" + mService + ", mThread:" + mThread);
            if (mThread != null) {
                mThreadStop = true;
                mThread = null;
            }

            mOpen= false;
            try {
                if (mService != null) {
                    mService.close();
                }
                /*if (mContext != null && serConn != null) {
                    mContext.unbindService(serConn);
                }*/
            } catch (Exception e) {
                LOGE("Exception:" + e);
            }
        }

        //add for skyworth
        public void destory() {
            LOGI("[destory]mService:" + mService);
            try {
                if (mService != null) {
                    mService.destory();
                }
                if (mContext != null && serConn != null) {
                    mContext.unbindService(serConn);
                }
            } catch (Exception e) {
                LOGE("unbindService not registered");
            }
        }

        private void show() {
            LOGI("[show]total:" + total() + ", mThread:" + mThread);
            if (total() > 0) {
                if (mThread == null) {
                    mThreadStop = false;
                    mThread = new Thread (runnable);
                    mThread.start();
                }
            } else {
                if (!isTvSubtile()) {
                    Log.i("SubtitleManager","no sub close subtitle.");
                    close();
                    mService = null;
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
            LOGI("[option]mService:" + mService);
            try {
                if (mService != null) {
                    mService.option();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public int total() {
            LOGI("[total]mService:" + mService);
            int ret = 0;

            try {
                if (mService != null) {
                    ret = mService.getSubTotal();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
            LOGI("[total]ret:" + ret);
            return ret;
        }

        public int innerTotal() {
            LOGI("[innerTotal]mService:" + mService);
            int ret = 0;

            try {
                if (mService != null) {
                    ret = mService.getInnerSubTotal();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
            LOGI("[innerTotal]ret:" + ret);
            return ret;
        }

        public void next() {
            LOGI("[next]mService:" + mService);
            try {
                if (mService != null) {
                    mService.nextSub();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void previous() {
            LOGI("[previous]mService:" + mService);
            try {
                if (mService != null) {
                    mService.preSub();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void hide() {
            LOGI("[hide]mService:" + mService);
            try {
                if (mService != null) {
                    mService.hide();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void display() {
            LOGI("[display]mService:" + mService);
            try {
                if (mService != null) {
                    mService.display();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void clear() {
            LOGI("[clear]mService:" + mService);
            try {
                if (mService != null) {
                    mService.clear();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void resetForSeek() {
            LOGI("[resetForSeek]mService:" + mService);
            try {
                if (mService != null) {
                    mService.resetForSeek();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setSubPid(int pid) {
            LOGI("[setSubPid]mService:" + mService);
            try {
                if (mService != null) {
                    mService.setSubPid(pid);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public int getSubHeight() {
            LOGI("[getSubHeight]mService:" + mService);
            int ret = 0;
            try {
                if (mService != null) {
                    ret = mService.getSubHeight();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
            return ret;
        }

        public int getSubWidth() {
            LOGI("[getSubWidth]mService:" + mService);
            int ret = 0;
            try {
                if (mService != null) {
                    ret = mService.getSubWidth();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
            return ret;
        }

        public void setSubType(int type) {
            LOGI("[setSubType]mService:" + mService);
            try {
                if (mService != null) {
                    mService.setSubType(type);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public int getSubType() {
            LOGI("[getSubType]mService:" + mService);
            int ret = 0;

            try {
                if (mService != null) {
                    ret = mService.getSubType();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getSubType]ret:" + ret);
            return ret;
        }

        public String getSubTypeStr() {
            LOGI("[getSubTypeStr]mService:" + mService);
            String type = null;

            try {
                if (mService != null) {
                    type = mService.getSubTypeStr();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getSubTypeStr]type:" + type);
            return type;
        }

        public List<String> getExtSubTypeAll() {
            LOGI("[getSubTypeStr]mService:" + mService);
            String subTypeAll = null;

            try {
                if (mService != null) {
                    subTypeAll = mService.getExtSubTypeAll();
                }
            } catch (RemoteException e) {
                throw new RuntimeException (e);
            }

            LOGI("[getSubTypeStr]subTypeAll:" + subTypeAll);
            return string2List(subTypeAll);
        }

        public List<String> string2List(String str) {
            LOGI("[stringConvert2List]str:" + str);
            if (str != null && !str.equals("")) {
                String[] strArray = str.split(",");
                ArrayList<String> typeStrs = new ArrayList<String>();
                for (int i = 0; i < strArray.length; i++) {
                    typeStrs.add(strArray[i]);
                }
                return typeStrs;
            }
            return null;
        }

        public List<String> getInBmpTxtType() {
            LOGI("[getInBmpTxtType]mService:" + mService);
            String bmpTxtType = null;
            try {
                if (mService != null) {
                    bmpTxtType = mService.getInBmpTxtType();
                }
            } catch (RemoteException e) {
                throw new RuntimeException (e);
            }
            return string2List(bmpTxtType);
        }

        public List<String> getInSubLanAll() {
            LOGI("[getInSubLanAll]mService:" + mService);
            String subLanStr = null;
            try {
                if (mService != null) {
                    subLanStr = mService.getInSubLanAll();
                }
            } catch (RemoteException e) {
                throw new RuntimeException (e);
            }
            return string2List(subLanStr);
        }

        public List<String> getExtBmpTxtType() {
            LOGI("[getExtBmpTxtType]mService:" + mService);
            String bmpTxtType = null;
            try {
                if (mService != null) {
                    bmpTxtType = mService.getExtBmpTxtType();
                }
            } catch (RemoteException e) {
                throw new RuntimeException (e);
            }
            return string2List(bmpTxtType);
        }

        public String getSubName (int idx) {
            LOGI("[getSubName]mService:" + mService);
            String name = null;

            try {
                if (mService != null) {
                    name = mService.getSubName (idx);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getSubName]name[" + idx + "]:" + name);
            return name;
        }

        public String getSubLanguage (int idx) {
            LOGI("[getSubLanguage]mService:" + mService);
            String language = null;

            try {
                if (mService != null) {
                    language = mService.getSubLanguage (idx);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getSubLanguage]language[" + idx + "]:" + language);
            return language;
        }

        public String getCurName() {
            LOGI("[getCurName]mService:" + mService);
            String name = null;

            try {
                if (mService != null) {
                    name = mService.getCurName();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getCurName] name:" + name);
            return name;
        }

        private void getService() {
            LOGI("[getService]");
            int retry = RETRY_MAX;
            boolean mIsBind = false;
            try {
                synchronized (this) {
                    while (true) {
                        //IBinder b = ServiceManager.getService ("subtitle_service"/*Context.SUBTITLE_SERVICE*/);
                        intent = new Intent();
                        intent.setAction("com.droidlogic.SubTitleServer");
                        intent.setPackage("com.droidlogic.SubTitleService");
                        mIsBind = mContext.bindService(intent, serConn, mContext.BIND_AUTO_CREATE);
                        LOGI("[getService] mIsBind:" + mIsBind + ", retry:" + retry);
                        if (mIsBind || retry <= 0) {
                            break;
                        }
                        retry --;
                        Thread.sleep(500);
                    }
                }
            }catch(InterruptedException e){}
        }

        private ServiceConnection serConn = new ServiceConnection() {
            @Override
            public void onServiceDisconnected(ComponentName name) {
                LOGI("[onServiceDisconnected]mService:"+mService);
                mService = null;
            }
            @Override
            public void onServiceConnected(ComponentName name, IBinder service) {
                mService = ISubTitleService.Stub.asInterface(service);
                LOGI("SubTitleClient.onServiceConnected()..mService:"+mService);
                try {
                    mService.registerCallback(mCallback);
                    LOGI("SubTitleClient.onServiceConnected()..registerCallback:");
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            }
        };

        public void release() {
            try {
                close();
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }
        }

        public int getSubTypeDetial() {
            LOGI("[getSubTypeDetial] mService:" + mService);
            int ret = 0;

            try {
                if (mService != null) {
                    ret = mService.getSubTypeDetial();
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }

            LOGI("[getSubTypeDetial] ret:" + ret);
            return ret;
        }

        public void setTextColor (int color) {
            LOGI("[setTextColor] color:" + color + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setTextColor (color);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setTextSize (int size) {
            LOGI("[setTextSize] size:" + size + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setTextSize (size);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setGravity (int gravity) {
            LOGI("[setGravity] gravity:" + gravity + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setGravity (gravity);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setTextStyle (int style) {
            LOGI("[setTextStyle] style:" + style + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setTextStyle (style);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setPosHeight (int height) {
            LOGI("[setPosHeight] height:" + height + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setPosHeight (height);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setImgSubRatio (float ratioW, float ratioH, int maxW, int maxH) {
            LOGI("[setImgSubRatio] ratioW:" + ratioW + ", ratioH:" + ratioH + ",maxW:" + maxW + ",maxH:" + maxH + ", mService:" + mService);
            try {
                if (mService != null) {
                    mService.setImgSubRatio (ratioW, ratioH, maxW, maxH);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void load(String path) {
            LOGI("[load] path:" + path);
            try {
                if (mService != null) {
                    mService.load(path);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
        }

        public void setSurfaceViewParam(int x, int y, int w, int h) {
            LOGI("[setSurfaceViewParam] x:" + x + ", y:" + y + ", w:" + w + ",h:" + h);
            try {
                if (mService != null) {
                    mService.setSurfaceViewParam(x, y, w, h);
                }
            } catch (RemoteException e) {
                Log.i(TAG, ", getService:");
                getService();
            }
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
            try {
                if (mService != null) {
                    mService.setIOType(type);
                }
            } catch (RemoteException e) {
                Log.i(TAG, "getServer:");
                getService();
            }
        }

        private void setIOType() {
            LOGI("[setIOType]mMediaPlayer:" + mMediaPlayer);

            try {
                mIOType = IO_TYPE_SOCKET;       //default amnuplayer
            if (mMediaPlayer != null) {
                String typeStr = mMediaPlayer.getStringParameter(mMediaPlayer.KEY_PARAMETER_AML_PLAYER_TYPE_STR);
                LOGI("[setIOType]typeStr:" + typeStr);
                if (typeStr != null && typeStr.equals("AMNU_PLAYER")) {
                    mIOType = IO_TYPE_SOCKET;
                }
            }
                setIOType(mIOType);
            } catch (Exception e) {
                Log.e(TAG, "Exception:" + e);
            }
        }

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
                                int ret = open (mPath);

                                if (ret == 0) {
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

        private String readSysfs (String path) {
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
                    };

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

        private int getCurrentPcr() {
            int pcr = 0;
            long pcrl = 0;
            String str = null;

            if (mIOType == IO_TYPE_DEV) {
                str = readSysfs ("/sys/class/tsync/pts_pcrscr");
            }
            else if (mIOType == IO_TYPE_SOCKET) {
                try {
                    if (mService != null) {
                        str = mService.getPcrscr();
                    }
                } catch (RemoteException e) {
                    throw new RuntimeException(e);
                }
            }

            LOGI("[getCurrentPcr]str:" + str);
            str = str.substring (2); // skip 0x

            if (str != null) {
                //pcr = (Integer.parseInt(str, 16));//90;// change to ms
                pcrl = (Long.parseLong (str, 16) );
                pcr = (int) (pcrl / 90);
            }

            LOGI("[getCurrentPcr]pcr:" + pcr);
            return pcr;
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
                    LOGI("[runnable]showSub mService:" + mService);

                    //show subtitle
                    try {
                        if (mService != null) {
                            if (mMediaPlayer != null && mMediaPlayer.isPlaying() ) {
                                if (getSubTypeDetial() == 6 || getSubTypeDetial() == 9) { //6:dvb type 9:teletext type
                                    pos = getCurrentPcr();
                                } else {
                                    pos = mMediaPlayer.getCurrentPosition();
                                }
                                LOGI("[runnable]showSub:" + pos);
                            }
                            if (!mThreadStop) {
                                mService.showSub (pos);
                            }
                            //mService.showSub (pos);
                        } else {
                            mThreadStop = true;
                            break;
                        }
                    } catch (RemoteException e) {
                        throw new RuntimeException (e);
                    }

                    try {
                        Thread.sleep (300 - (pos % 300) );
                    } catch (InterruptedException e) {
                        Thread.currentThread().interrupt();
                    }
                }
            }
        };

        public static void setHidlCallback (SubtitleDataListener cb) {
            mHidlCallback= cb;
        }

        public interface SubtitleDataListener {
            public void onSubTitleEvent(int event, int id);
            public void onSubTitleAvailable(int available);
        }
}
