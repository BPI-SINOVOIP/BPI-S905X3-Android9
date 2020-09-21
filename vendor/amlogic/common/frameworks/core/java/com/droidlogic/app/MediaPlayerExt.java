/*
 * AMLOGIC Media Player.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 *
 * Author:  Wang Jian <jian.wang@amlogic.com>
 * Modified: XiaoLiang.Wang <xiaoliang.wang@amlogic.com 20151202>
 */
package com.droidlogic.app;

import android.content.Context;
import android.net.Uri;
import android.media.MediaPlayer;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;
import android.os.Message;
import android.os.Parcel;
import android.os.RemoteException;
import android.util.Log;
import java.lang.Thread.State;
import java.lang.reflect.Method;

import java.util.Map;
import android.content.ContentResolver;
import android.content.res.AssetFileDescriptor;
import java.io.IOException;
import java.io.File;
import java.io.FileDescriptor;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.lang.reflect.Modifier;
import java.lang.reflect.InvocationTargetException;


/**
 * Created by wangjian on 2014/4/17.
 */
public class MediaPlayerExt extends MediaPlayer {
    private static final String TAG = "MediaPlayerExt";
    private static final boolean DEBUG = true;
    private static final int FF_PLAY_TIME = 5000;
    private static final int FB_PLAY_TIME = 5000;
    private static final int BASE_SLEEP_TIME = 500;
    private static final int ON_FF_COMPLETION = 1;
    private int mStep = 0;
    private boolean mIsFF = true;
    private boolean mStopFast = true;
    private int mPos = -1;
    private Thread mThread = null;
    private OnCompletionListener mOnCompletionListener = null;
    private OnSeekCompleteListener mOnSeekCompleteListener = null;
    private OnErrorListener mOnErrorListener = null;
    private OnBlurayListener mOnBlurayInfoListener = null;

    private EventHandler mEventHandler;

    private MethodHandle h1;
    private MethodHandles.Lookup lookup = MethodHandles.lookup();

    //must sync with IMediaPlayer.cpp (av\media\libmedia)
    private IBinder mIBinder = null; //IMediaPlayer
    private static final String SYS_TOKEN                   = "android.media.IMediaPlayer";
    private static final int DISCONNECT                     = IBinder.FIRST_CALL_TRANSACTION;
    private static final int SET_DATA_SOURCE_URL            = IBinder.FIRST_CALL_TRANSACTION + 1;
    private static final int SET_DATA_SOURCE_FD             = IBinder.FIRST_CALL_TRANSACTION + 2;
    private static final int SET_DATA_SOURCE_STREAM         = IBinder.FIRST_CALL_TRANSACTION + 3;
    private static final int SET_DATA_SOURCE_CALLBACK       = IBinder.FIRST_CALL_TRANSACTION + 4;
    private static final int PREPARE_ASYNC                  = IBinder.FIRST_CALL_TRANSACTION + 5;
    private static final int START                          = IBinder.FIRST_CALL_TRANSACTION + 6;
    private static final int STOP                           = IBinder.FIRST_CALL_TRANSACTION + 7;
    private static final int IS_PLAYING                     = IBinder.FIRST_CALL_TRANSACTION + 8;
    private static final int SET_PLAYBACK_SETTINGS          = IBinder.FIRST_CALL_TRANSACTION + 9;
    private static final int GET_PLAYBACK_SETTINGS          = IBinder.FIRST_CALL_TRANSACTION + 10;
    private static final int SET_SYNC_SETTINGS              = IBinder.FIRST_CALL_TRANSACTION + 11;
    private static final int GET_SYNC_SETTINGS              = IBinder.FIRST_CALL_TRANSACTION + 12;
    private static final int PAUSE                          = IBinder.FIRST_CALL_TRANSACTION + 13;
    private static final int SEEK_TO                        = IBinder.FIRST_CALL_TRANSACTION + 14;
    private static final int GET_CURRENT_POSITION           = IBinder.FIRST_CALL_TRANSACTION + 15;
    private static final int GET_DURATION                   = IBinder.FIRST_CALL_TRANSACTION + 16;
    private static final int RESET                          = IBinder.FIRST_CALL_TRANSACTION + 17;
    private static final int SET_AUDIO_STREAM_TYPE          = IBinder.FIRST_CALL_TRANSACTION + 18;
    private static final int SET_LOOPING                    = IBinder.FIRST_CALL_TRANSACTION + 19;
    private static final int SET_VOLUME                     = IBinder.FIRST_CALL_TRANSACTION + 20;
    private static final int INVOKE                         = IBinder.FIRST_CALL_TRANSACTION + 21;
    private static final int SET_METADATA_FILTER            = IBinder.FIRST_CALL_TRANSACTION + 22;
    private static final int GET_METADATA                   = IBinder.FIRST_CALL_TRANSACTION + 23;
    private static final int SET_AUX_EFFECT_SEND_LEVEL      = IBinder.FIRST_CALL_TRANSACTION + 24;
    private static final int ATTACH_AUX_EFFECT              = IBinder.FIRST_CALL_TRANSACTION + 25;
    private static final int SET_VIDEO_SURFACETEXTURE       = IBinder.FIRST_CALL_TRANSACTION + 26;
    private static final int SET_PARAMETER                  = IBinder.FIRST_CALL_TRANSACTION + 27;
    private static final int GET_PARAMETER                  = IBinder.FIRST_CALL_TRANSACTION + 28;
    private static final int SET_RETRANSMIT_ENDPOINT        = IBinder.FIRST_CALL_TRANSACTION + 29;
    private static final int GET_RETRANSMIT_ENDPOINT        = IBinder.FIRST_CALL_TRANSACTION + 30;
    private static final int SET_NEXT_PLAYER                = IBinder.FIRST_CALL_TRANSACTION + 31;

    private final static String IMEDIA_PLAYER = "android.media.IMediaPlayer";
    private static final int INVOKE_ID_GET_AM_TRACK_INFO        = 11;
    private static final int INVOKE_ID_USE_CUSTOMIZED_EXTRACTOR  = 1001;
    private static final int INVOKE_ID_GET_HDR_TYPE  = 1002;
    private static final int INVOKE_ID_SET_SOUND_TRACK  = 1003;

    //must sync with IMediaPlayerService.cpp (av\media\libmedia)
    private IBinder mIBinderService = null; //IMediaPlayerService
    private static final String SYS_TOKEN_SERVICE           = "android.media.IMediaPlayerService";
    private static final int CREATE                         = IBinder.FIRST_CALL_TRANSACTION;
    private static final int SET_MEDIA_PLAYER_CLIENT        = IBinder.FIRST_CALL_TRANSACTION + 9;

    //must sync with Mediaplayer.h (av\include\media)
    public static final int KEY_PARAMETER_AML_VIDEO_POSITION_INFO           = 2000;
    public static final int KEY_PARAMETER_AML_PLAYER_TYPE_STR               = 2001;
    public static final int KEY_PARAMETER_AML_PLAYER_VIDEO_OUT_TYPE         = 2002;
    public static final int KEY_PARAMETER_AML_PLAYER_SWITCH_SOUND_TRACK     = 2003;             //refer to lmono,rmono,stereo,set only
    public static final int KEY_PARAMETER_AML_PLAYER_SWITCH_AUDIO_TRACK     = 2004;             //refer to audio track index,set only
    public static final int KEY_PARAMETER_AML_PLAYER_TRICKPLAY_FORWARD      = 2005;             //refer to forward:speed
    public static final int KEY_PARAMETER_AML_PLAYER_TRICKPLAY_BACKWARD     = 2006;             //refer to backward:speed
    public static final int KEY_PARAMETER_AML_PLAYER_FORCE_HARD_DECODE      = 2007;             //refer to mp3,etc.
    public static final int KEY_PARAMETER_AML_PLAYER_FORCE_SOFT_DECODE      = 2008;             //refer to mp3,etc.
    public static final int KEY_PARAMETER_AML_PLAYER_GET_MEDIA_INFO         = 2009;             //get media info
    public static final int KEY_PARAMETER_AML_PLAYER_FORCE_SCREEN_MODE      = 2010;             //set screen mode
    public static final int KEY_PARAMETER_AML_PLAYER_SET_DISPLAY_MODE       = 2011;             //set display mode
    public static final int KEY_PARAMETER_AML_PLAYER_GET_DTS_ASSET_TOTAL    = 2012;             //get dts asset total number
    public static final int KEY_PARAMETER_AML_PLAYER_SET_DTS_ASSET          = 2013;             //set dts asset
    public static final int KEY_PARAMETER_AML_PLAYER_SWITCH_VIDEO_TRACK     = 2015;             //string,refer to video track index,set only
    public static final int KEY_PARAMETER_AML_PLAYER_HWBUFFER_STATE         = 3001;             //refer to stream buffer info, hardware decoder buffer infos,get only
    public static final int KEY_PARAMETER_AML_PLAYER_RESET_BUFFER           = 8000;             //top level seek..player need to reset & clearbuffers
    public static final int KEY_PARAMETER_AML_PLAYER_FREERUN_MODE           = 8002;             //play ASAP...
    public static final int KEY_PARAMETER_AML_PLAYER_ENABLE_OSDVIDEO        = 8003;             //play enable osd video for this player....
    public static final int KEY_PARAMETER_AML_PLAYER_DIS_AUTO_BUFFER        = 8004;             //play ASAP...
    public static final int KEY_PARAMETER_AML_PLAYER_ENA_AUTO_BUFFER        = 8005;             //play ASAP...
    public static final int KEY_PARAMETER_AML_PLAYER_USE_SOFT_DEMUX         = 8006;             //play use soft demux
    public static final int KEY_PARAMETER_AML_PLAYER_PR_CUSTOM_DATA         = 9001;             //string, playready, set only

    public MediaPlayerExt() {
        Looper looper;
        if ((looper = Looper.myLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else if ((looper = Looper.getMainLooper()) != null) {
            mEventHandler = new EventHandler(this, looper);
        } else {
            mEventHandler = null;
        }

        setMediaPlayerClient(MediaPlayerClient.create(this));
    }

    private void setMediaPlayerClient(IBinder clientBinder) {
        if (mIBinderService == null) {
            try {
                Object object = Class.forName("android.os.ServiceManager")
                    .getMethod("getService", new Class[] { String.class })
                    .invoke(null, new Object[] { "media.player" });
                mIBinderService = (IBinder)object;
                if (DEBUG) Log.i(TAG,"[MediaPlayerExt]mIBinderService:" + mIBinderService);
            }
            catch (Exception ex) {
                Log.e(TAG, "get IMediaPlayerService:" + ex);
            }
        }

        try {
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInterfaceToken(SYS_TOKEN_SERVICE);
            data.writeStrongBinder(clientBinder);
            mIBinderService.transact(SET_MEDIA_PLAYER_CLIENT, data, reply, 0);
            data.recycle();
            reply.recycle();
        } catch (RemoteException ex) {
            Log.e(TAG, "setMediaPlayerClient:" + ex);
        }
    }

    /**
     * The function should be invoked after a player was created. (after setDataSource)
     * In this case, the client (data.writeStrongBinder(null)) will get the running one.
     * Otherwise will throw "unknow error"
     */
    private void prepareIBinder() {
        if (mIBinderService == null) {
            try {
                Object object = Class.forName("android.os.ServiceManager")
                    .getMethod("getService", new Class[] { String.class })
                    .invoke(null, new Object[] { "media.player" });
                mIBinderService = (IBinder)object;
                if (DEBUG) Log.i(TAG,"[MediaPlayerExt]mIBinderService:" + mIBinderService);
            }
            catch (Exception ex) {
                Log.e(TAG, "get IMediaPlayerService:" + ex);
            }
        }

        try {
            Parcel data = Parcel.obtain();
            Parcel reply = Parcel.obtain();
            data.writeInterfaceToken(SYS_TOKEN_SERVICE);
            data.writeStrongBinder(null);
            data.writeInt(0);
            mIBinderService.transact(CREATE, data, reply, 0);
            mIBinder = reply.readStrongBinder();
            reply.recycle();
            data.recycle();
            if (DEBUG) Log.i(TAG,"[MediaPlayerExt]mIBinder:" + mIBinder);
        } catch (RemoteException ex) {
            Log.e(TAG, "get IMediaPlayer :" + ex);
        }
    }

    private void getParameter(int key, Parcel reply) {
        if (DEBUG) Log.i(TAG,"[getParameter]mIBinder:" + mIBinder + ",key:" + key + ",reply:" + reply);
        if (null == mIBinder) {
            prepareIBinder();
        }

        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                data.writeInterfaceToken(SYS_TOKEN);
                data.writeInt(key);
                mIBinder.transact(GET_PARAMETER, data, reply, 0);
                data.recycle();
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "getParameter:" + ex);
        }
    }

    public Parcel getParcelParameter(int key) {
        Parcel p = Parcel.obtain();
        getParameter(key, p);
        return p;
    }

    public String getStringParameter(int key) {
        Parcel p = Parcel.obtain();
        getParameter(key, p);
        String ret = p.readString();
        p.recycle();
        return ret;
    }

    public int getIntParameter(int key) {
        Parcel p = Parcel.obtain();
        getParameter(key, p);
        int ret = p.readInt();
        p.recycle();
        return ret;
    }

    private boolean setParameter(int key, Parcel value) {
        boolean ret = false;
        if (DEBUG) Log.i(TAG,"[setParameter]mIBinder:" + mIBinder + ",key:" + key + ",Parcel value:" + value);
        if (null == mIBinder) {
            prepareIBinder();
        }

        try {
            if (null != mIBinder) {
                Parcel data = Parcel.obtain();
                Parcel reply = Parcel.obtain();
                data.writeInterfaceToken(SYS_TOKEN);
                data.writeInt(key);
                if (value.dataSize() > 0) {
                    data.appendFrom(value, 0, value.dataSize());
                }
                mIBinder.transact(SET_PARAMETER, data, reply, 0);
                if (0 == reply.readInt()) { //OK
                    ret = true;
                }
                else {
                    ret = false;
                }
                reply.recycle();
                data.recycle();
                return ret;
            }
        } catch (RemoteException ex) {
            Log.e(TAG, "setParameter:" + ex);
        }

        return false;
    }

    public boolean setParameter(int key, String value) {
        if (DEBUG) Log.i(TAG,"[setParameter]mIBinder:" + mIBinder + ",key:" + key + ",String value:" + value);
        Parcel p = Parcel.obtain();
        p.writeString(value);
        boolean ret = setParameter(key, p);
        p.recycle();
        return ret;
    }

    public boolean setParameter(int key, int value) {
        if (DEBUG) Log.i(TAG,"[setParameter]mIBinder:" + mIBinder + ",key:" + key + ",int value:" + value);
        Parcel p = Parcel.obtain();
        p.writeInt(value);
        boolean ret = setParameter(key, p);
        p.recycle();
        return ret;
    }

    public void fastForward (int step) {
        if (DEBUG) Log.i (TAG, "fastForward:" + step);
        synchronized (this) {
            String playerTypeStr = getStringParameter(KEY_PARAMETER_AML_PLAYER_TYPE_STR);
            if ( (playerTypeStr != null) && (playerTypeStr.equals ("AMLOGIC_PLAYER"))) {
                String str = Integer.toString (step);
                StringBuilder builder = new StringBuilder();
                builder.append ("forward:" + str);
                if (DEBUG) Log.i (TAG, "[HW]" + builder.toString());
                setParameter(KEY_PARAMETER_AML_PLAYER_TRICKPLAY_FORWARD,builder.toString());
                return;
            }
            mStep = step;
            mIsFF = true;
            mStopFast = false;
            if (mThread == null) {
                mThread = new Thread (runnable);
                mThread.start();
            }
            else {
                if (mThread.getState() == State.TERMINATED) {
                    mThread = new Thread (runnable);
                    mThread.start();
                }
            }
        }
    }

    public void fastBackward (int step) {
        if (DEBUG) Log.i (TAG, "fastBackward:" + step);
        synchronized (this) {
            String playerTypeStr = getStringParameter(KEY_PARAMETER_AML_PLAYER_TYPE_STR);
            if ( (playerTypeStr != null) && (playerTypeStr.equals ("AMLOGIC_PLAYER"))) {
                String str = Integer.toString (step);
                StringBuilder builder = new StringBuilder();
                builder.append ("backward:" + str);
                if (DEBUG) Log.i (TAG, "[HW]" + builder.toString());
                setParameter(KEY_PARAMETER_AML_PLAYER_TRICKPLAY_BACKWARD,builder.toString());
                return;
            }
            mStep = step;
            mIsFF = false;
            mStopFast = false;
            if (mThread == null) {
                mThread = new Thread (runnable);
                mThread.start();
            }
            else {
                if (mThread.getState() == State.TERMINATED) {
                    mThread = new Thread (runnable);
                    mThread.start();
                }
            }
        }
    }

    public boolean isPlaying() {
        if (!mStopFast) {
            return true;
        }
        return super.isPlaying();
    }

    public void reset() {
        mIBinder = null;
        mStopFast = true;
        super.reset();

        if (mEventHandler != null) {
            mEventHandler.removeCallbacksAndMessages(null);
        }
    }

    public void start() {
        mStopFast = true;
        super.start();
    }

    public void pause() {
        mStopFast = true;
        super.pause();
    }

    public void stop() {
        mStopFast = true;
        super.stop();
    }

    public void release() {
        mStopFast = true;
        mOnBlurayInfoListener = null;
        super.release();
    }

    private void superPause() {
        super.pause();
    }

    private void superStart() {
        super.start();
    }

    public class VideoInfo{
        public int index;
        public int id;
        public String vformat;
        public int width;
        public int height;
    }

    public class AudioInfo{
        public int index;
        public int id; //id is useless for application
        public int aformat;
        public int channel;
        public int sample_rate;
        public String audioMime;
    }

    public class SubtitleInfo{
        public int index;
        public int id;
        public int sub_type;
        public String sub_language;
    }

    public class TsProgrameInfo{
        public int v_pid;
        public String title;
    }

    public class MediaInfo{
        public String filename;
        public int duration;
        public String file_size;
        public int bitrate;
        public int type;
        //public int fps;
        public int cur_video_index;
        public int cur_audio_index;
        public int cur_sub_index;

        public int total_video_num;
        public VideoInfo[] videoInfo;

        public int total_audio_num;
        public AudioInfo[] audioInfo;

        public int total_sub_num;
        public SubtitleInfo[] subtitleInfo;

        public int total_ts_num;
        public TsProgrameInfo[] tsprogrameInfo;
    }
    public void setUseLocalExtractor(MediaPlayerExt mp) {
        /* setUseLocalExtractor use INVOKE_ID_USE_CUSTOMIZED_EXTRACTOR */
        /* para: 1: enable ffmpeg extractor force                      */
        /*       0: disable ffmpeg extractor force                     */
        Parcel request = Parcel.obtain();
        Parcel p = Parcel.obtain();
        request.writeInterfaceToken(IMEDIA_PLAYER);
        request.writeInt(INVOKE_ID_USE_CUSTOMIZED_EXTRACTOR);
        request.writeInt(1);
        MediaPlayerInvoke(request, p, mp);
    }

   /* getHDRType by invoke use INVOKE_ID_GET_HDR_TYPE */
   /* return: int : 0 : not HDR                       */
   /*               1 : HLG                           */
   /*               2 : HDR                           */
    public int getHDRType(MediaPlayerExt mp) {
        Parcel request = Parcel.obtain();
        Parcel p = Parcel.obtain();
        request.writeInterfaceToken(IMEDIA_PLAYER);
        request.writeInt(INVOKE_ID_GET_HDR_TYPE);
        MediaPlayerInvoke(request, p, mp);
        int type = p.readInt();
        return type;
   }
   public void setSoundTrack(MediaPlayerExt mp, int mode) {
        Parcel request = Parcel.obtain();
        Parcel p = Parcel.obtain();
        request.writeInterfaceToken(IMEDIA_PLAYER);
        request.writeInt(INVOKE_ID_SET_SOUND_TRACK);
        request.writeInt(mode);
        MediaPlayerInvoke(request, p, mp);
   }

   //getMediaInfo by invoke instead of getParameter (WL)
    public MediaInfo getMediaInfo(MediaPlayerExt mp) {
        MediaInfo mediaInfo = new MediaInfo();
        Parcel request = Parcel.obtain();
        Parcel p = Parcel.obtain();
        request.writeInterfaceToken(IMEDIA_PLAYER);
        request.writeInt(INVOKE_ID_GET_AM_TRACK_INFO);
        MediaPlayerInvoke(request, p, mp);
        //super.invoke(request, p);
        //Parcel p = Parcel.obtain();
        //getParameter(KEY_PARAMETER_AML_PLAYER_GET_MEDIA_INFO, p);
        mediaInfo.filename = p.readString();
        mediaInfo.duration = p.readInt();
        mediaInfo.file_size = p.readString();
        mediaInfo.bitrate = p.readInt();
        mediaInfo.type = p.readInt();
        //mediaInfo.fps = p.readInt();
        mediaInfo.cur_video_index = p.readInt();
        mediaInfo.cur_audio_index = p.readInt();
        mediaInfo.cur_sub_index = p.readInt();
        if (DEBUG) Log.i(TAG,"[getMediaInfo]filename:"+mediaInfo.filename+",duration:"+mediaInfo.duration+",file_size:"+mediaInfo.file_size+",bitrate:"+mediaInfo.bitrate+",type:"+mediaInfo.type);
        if (DEBUG) Log.i(TAG,"[getMediaInfo]cur_video_index:"+mediaInfo.cur_video_index+",cur_audio_index:"+mediaInfo.cur_audio_index+",cur_sub_index:"+mediaInfo.cur_sub_index);

        //----video info----
        mediaInfo.total_video_num = p.readInt();
        if (DEBUG) Log.i(TAG,"[getMediaInfo]mediaInfo.total_video_num:"+mediaInfo.total_video_num);
        mediaInfo.videoInfo = new VideoInfo[mediaInfo.total_video_num];
        for (int i=0;i<mediaInfo.total_video_num;i++) {
            mediaInfo.videoInfo[i] = new VideoInfo();
            mediaInfo.videoInfo[i].index = p.readInt();
            mediaInfo.videoInfo[i].id = p.readInt();
            mediaInfo.videoInfo[i].vformat = p.readString();
            mediaInfo.videoInfo[i].width = p.readInt();
            mediaInfo.videoInfo[i].height = p.readInt();
            if (DEBUG) Log.i(TAG,"[getMediaInfo]videoInfo i:"+i+",index:"+mediaInfo.videoInfo[i].index+",id:"+mediaInfo.videoInfo[i].id);
            if (DEBUG) Log.i(TAG,"[getMediaInfo]videoInfo i:"+i+",vformat:"+mediaInfo.videoInfo[i].vformat);
            if (DEBUG) Log.i(TAG,"[getMediaInfo]videoInfo i:"+i+",width:"+mediaInfo.videoInfo[i].width+",height:"+mediaInfo.videoInfo[i].height);
        }

        //----audio info----
        mediaInfo.total_audio_num = p.readInt();
        if (DEBUG) Log.i(TAG,"[getMediaInfo]mediaInfo.total_audio_num:"+mediaInfo.total_audio_num);
        mediaInfo.audioInfo = new AudioInfo[mediaInfo.total_audio_num];
        for (int j=0;j<mediaInfo.total_audio_num;j++) {
            mediaInfo.audioInfo[j] = new AudioInfo();
            mediaInfo.audioInfo[j].index = p.readInt();
            mediaInfo.audioInfo[j].id = p.readInt();
            mediaInfo.audioInfo[j].aformat = p.readInt();
            mediaInfo.audioInfo[j].channel = p.readInt();
            mediaInfo.audioInfo[j].sample_rate = p.readInt();
            mediaInfo.audioInfo[j].audioMime= p.readString();
            if (DEBUG) Log.i(TAG,"[getMediaInfo]audioInfo j:"+j+",index:"+mediaInfo.audioInfo[j].index+",id:"+mediaInfo.audioInfo[j].id+",aformat:"+mediaInfo.audioInfo[j].aformat);
            if (DEBUG) Log.i(TAG,"[getMediaInfo]audioInfo j:"+j+",channel:"+mediaInfo.audioInfo[j].channel+",sample_rate:"+mediaInfo.audioInfo[j].sample_rate);
        }

        //----subtitle info----
        mediaInfo.total_sub_num = p.readInt();
        if (DEBUG) Log.i(TAG,"[getMediaInfo]mediaInfo.total_sub_num:"+mediaInfo.total_sub_num);
        mediaInfo.subtitleInfo = new SubtitleInfo[mediaInfo.total_sub_num];
        for (int k=0;k<mediaInfo.total_sub_num;k++) {
            mediaInfo.subtitleInfo[k] = new SubtitleInfo();
            mediaInfo.subtitleInfo[k].index = p.readInt();
            mediaInfo.subtitleInfo[k].id = p.readInt();
            mediaInfo.subtitleInfo[k].sub_type = p.readInt();
            mediaInfo.subtitleInfo[k].sub_language = p.readString();
            if (DEBUG) Log.i(TAG,"[getMediaInfo]subtitleInfo k:"+k+",index:"+mediaInfo.subtitleInfo[k].index+",id:"+mediaInfo.subtitleInfo[k].id+",sub_type:"+mediaInfo.subtitleInfo[k].sub_type);
            if (DEBUG) Log.i(TAG,"[getMediaInfo]subtitleInfo k:"+k+",sub_language:"+mediaInfo.subtitleInfo[k].sub_language);
        }

        //----ts programe info----
        mediaInfo.total_ts_num = p.readInt();
        if (DEBUG) Log.i(TAG,"[getMediaInfo]mediaInfo.total_ts_num:"+mediaInfo.total_ts_num);
        mediaInfo.tsprogrameInfo = new TsProgrameInfo[mediaInfo.total_ts_num];
        for (int l=0;l<mediaInfo.total_ts_num;l++) {
            mediaInfo.tsprogrameInfo[l] = new TsProgrameInfo();
            mediaInfo.tsprogrameInfo[l].v_pid = p.readInt();
            mediaInfo.tsprogrameInfo[l].title = p.readString();
            if (DEBUG) {
                Log.i(TAG,"[getMediaInfo]tsprogrameInfo l:"+l+",v_pid:"+mediaInfo.tsprogrameInfo[l].v_pid+",title:"+mediaInfo.tsprogrameInfo[l].title);
                byte[] data = (mediaInfo.tsprogrameInfo[l].title).getBytes();
                for (int m = 0; m < data.length; m++) {
                    Log.i(TAG,"[getMediaInfo]data["+m+"]:"+data[m] + "("+(String.format("0x%x", (0xff & data[m]))) + ")");
                }
                Log.i(TAG,"[getMediaInfo]=======================================");
            }
        }

        p.recycle();

        return mediaInfo;
    }

    public void MediaPlayerInvoke(Parcel p1, Parcel p2, MediaPlayerExt mp) {
        Log.i(TAG,"[getMediaInfobyInvoke]mp:"+mp);
        try {
            Field allowedModes = MethodHandles.Lookup.class.getDeclaredField("allowedModes");
            allowedModes.setAccessible(true);
            allowedModes.set(lookup, -1);
            h1 = lookup.findSpecial(MediaPlayer.class, "invoke", MethodType.methodType(void.class, Parcel.class, Parcel.class), MediaPlayerExt.class);
            h1.invoke(mp, p1, p2);
        } catch(ClassNotFoundException e) {
            e.printStackTrace();
        } catch (IllegalAccessException e) {
            e.printStackTrace();
        } catch (NoSuchMethodException ex) {
            ex.printStackTrace();
        } catch (NoSuchFieldException ex) {
            ex.printStackTrace();
        } catch (IllegalArgumentException e) {
            e.printStackTrace();
        } catch (InvocationTargetException e) {
            e.printStackTrace();
        } catch(Throwable t) {
        }
    }

    public void setOnSeekCompleteListener (OnSeekCompleteListener listener) {
        mOnSeekCompleteListener = listener;
        super.setOnSeekCompleteListener (mMediaPlayerSeekCompleteListener);
    }

    public void setOnCompletionListener (OnCompletionListener listener) {
        mOnCompletionListener = listener;
        super.setOnCompletionListener (mMediaPlayerCompletionListener);
    }

    public void setOnErrorListener (OnErrorListener listener) {
        mOnErrorListener = listener;
        super.setOnErrorListener (mMediaPlayerErrorListener);
    }

    /**
     * Interface definition of a callback to be invoked when a
     * bluray infomation is available for update.
     */
    public interface OnBlurayListener {
        /**
         * Called to indicate an avaliable bluray info
         *
         * @param mp             the MediaPlayer associated with this callback
         * @param ext1           the bluray info message arg1
         * @param ext2           the bluray info message arg2
         * @param obj            the bluray info message obj
         */
        public void onBlurayInfo(MediaPlayer mp, int ext1, int ext2, Object obj);
    }

    /**
     * Register a callback to be invoked when a bluray info is available
     * for update.
     *
     * @param listener the callback that will be run
     */
    public void setOnBlurayInfoListener(OnBlurayListener listener) {
        mOnBlurayInfoListener = listener;
    }

    //must different with message value defined in MediaPlayer.java
    private static final int MEDIA_BLURAY_INFO = 203;
    private class EventHandler extends Handler {
        private MediaPlayer mMediaPlayer;

        public EventHandler(MediaPlayer mp, Looper looper) {
            super(looper);
            mMediaPlayer = mp;
        }

        @Override
        public void handleMessage(Message msg) {
            if (DEBUG) Log.i(TAG, "[handleMessage]msg: " + msg);
            switch (msg.what) {
                case MEDIA_BLURAY_INFO:
                    if (mOnBlurayInfoListener == null)
                        return;
                    mOnBlurayInfoListener.onBlurayInfo(mMediaPlayer, msg.arg1, msg.arg2, msg.obj);
                    return;
                default:
                    Log.e(TAG, "Unknown message: " + msg.what);
                    break;
            }
        }
    }

    public void postEvent(int msg, int ext1, int ext2, Object obj) {
        /*if (DEBUG) Log.i(TAG, "[postEvent]msg: " + msg);
        if (mEventHandler != null) {
            Message m = mEventHandler.obtainMessage(msg, ext1, ext2, obj);
            mEventHandler.sendMessage(m);
        }*/
    }

    private void OnFFCompletion() {
        if (mOnCompletionListener != null) {
            if (DEBUG) Log.i (TAG, "mOnCompletionListener.onCompletion");
            mOnCompletionListener.onCompletion (this);
        }
    }

    private Runnable runnable = new Runnable() {
        @Override
        public void run() {
            int pos;
            int duration = getDuration ();
            int sleepTime = BASE_SLEEP_TIME;
            int seekPos = 0;
            int stepMini = getDuration() / 100;//100 should sync with ProgressBar.java mMax, separate progress bar to 100 slices
            superPause();
            while (!mStopFast) {
                if (mStep < 1) {
                    mStopFast = true;
                    superStart();
                    break;
                }
                pos = getCurrentPosition();
                if (!mIsFF && (pos <= 1000 || pos <= stepMini)) {//1000 means position smaller than 1s
                    mStopFast = true;
                    superStart();
                    break;
                }
                if (pos == duration || pos == mPos) {
                    stop();
                    Message newMsg = Message.obtain();
                    newMsg.what = ON_FF_COMPLETION;
                    mMainHandler.sendMessage (newMsg);
                    break;
                }
                mPos = pos;
                if (mIsFF) {
                    int jumpTime = mStep * FF_PLAY_TIME;
                    int baseTime = 0;
                    if (mPos < seekPos + sleepTime) {
                        baseTime = seekPos + sleepTime;
                    }
                    else {
                        baseTime = mPos;
                    }
                    seekPos = (baseTime + jumpTime) > duration ? duration : (baseTime + jumpTime);
                    seekTo (seekPos);
                }
                else {
                    int jumpTime = mStep * FB_PLAY_TIME;
                    seekPos = (mPos - jumpTime) < 0 ? 0 : (mPos - jumpTime);
                    seekTo (seekPos);
                }
                try {
                    Thread.sleep (sleepTime);
                }
                catch (InterruptedException e) {
                    Thread.currentThread().interrupt();
                }
            }
        }
    };
    private MediaPlayer.OnSeekCompleteListener mMediaPlayerSeekCompleteListener =
    new MediaPlayer.OnSeekCompleteListener() {
        public void onSeekComplete (MediaPlayer mp) {
            if (mStopFast) {
                if (mOnSeekCompleteListener != null) {
                    mOnSeekCompleteListener.onSeekComplete (mp);
                }
            }
        }
    };

    private MediaPlayer.OnCompletionListener mMediaPlayerCompletionListener =
    new MediaPlayer.OnCompletionListener() {
        public void onCompletion (MediaPlayer mp) {
            if (!mStopFast) {
                mStopFast = true;
            }
            if (mOnCompletionListener != null) {
                mOnCompletionListener.onCompletion (mp);
            }
        }
    };

    private MediaPlayer.OnErrorListener mMediaPlayerErrorListener =
    new MediaPlayer.OnErrorListener() {
        public boolean onError (MediaPlayer mp, int what, int extra) {
            if (!mStopFast) {
                mStopFast = true;
            }
            if (mOnErrorListener != null) {
                return mOnErrorListener.onError (mp, what, extra);
            }
            return true;
        }
    };
    private Handler mMainHandler = new Handler() {
        public void handleMessage (Message msg) {
            switch (msg.what) {
                case ON_FF_COMPLETION:
                    OnFFCompletion();
                    break;
            }
        }
    };
}

