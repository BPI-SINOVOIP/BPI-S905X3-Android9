/*
 * Copyright (c) 2014 Amlogic, Inc. All rights reserved.
 *
 * This source code is subject to the terms and conditions defined in the
 * file 'LICENSE' which is part of this source code package.
 *
 * Description: JAVA file
 */

package com.droidlogic.app.tv;

import java.io.IOException;
import java.util.List;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.Collections;
import java.util.Comparator;

import com.droidlogic.app.tv.AudioEffectManager;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.DroidLogicHdmiCecManager;
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.TvInSignalInfo;
import com.droidlogic.app.SystemControlManager;

import android.provider.Settings;
import android.content.Context;
import android.content.ComponentName;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.pm.PackageManager.NameNotFoundException;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvStreamConfig;
import android.media.tv.TvInputManager.Hardware;
import android.media.tv.TvInputManager.HardwareCallback;
import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.util.SparseArray;
import android.media.tv.TvContract.Channels;
import android.app.ActivityManager;
import android.view.Surface;
import android.net.Uri;
//import com.android.internal.os.SomeArgs;
import android.os.Handler;
import android.os.Message;
import android.os.SystemClock;
import android.hardware.hdmi.HdmiControlManager;
import android.provider.Settings.Global;
import android.hardware.hdmi.HdmiDeviceInfo;
import java.util.HashMap;

import org.xmlpull.v1.XmlPullParserException;

import android.provider.Settings;
import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.provider.Settings.Secure;
import android.content.ContentResolver;
import android.media.MediaCodec;
import android.media.AudioManager;
import android.text.TextUtils;
import android.os.UserHandle;
import java.util.Map;

public class DroidLogicTvInputService extends TvInputService implements
        TvInSignalInfo.SigInfoChangeListener, TvControlManager.StorDBEventListener,
        TvControlManager.ScanningFrameStableListener, TvControlManager.StatusSourceConnectListener {
    private static final String TAG = DroidLogicTvInputService.class.getSimpleName();
    private static final boolean DEBUG = true;

    private SparseArray<TvInputInfo> mInfoList = new SparseArray<>();

    private TvInputBaseSession mSession;
    private String mCurrentInputId;
    public Hardware mHardware;
    public TvStreamConfig[] mConfigs;
    private int mDeviceId = -1;
    private int mSourceType = DroidLogicTvUtils.SOURCE_TYPE_OTHER;
    private String mChildClassName;
    private SurfaceHandler mSessionHandler;
    private static final int MSG_DO_TUNE = 0;
    private static final int MSG_DO_RELEASE = 1;
    private static final int MSG_DO_SET_SURFACE = 3;
    private static int mSelectPort = -1;
    private static String ACCESSIBILITY_CAPTIONING_PRESET = "accessibility_captioning_preset";
    protected Surface mSurface;
    protected int ACTION_FAILED = -1;
    protected int ACTION_SUCCESS = 1;
    protected int ACTION_PENDING = 0;

    private Context mContext;
    private int mCurrentSessionId = 0;

    private TvInputManager mTvInputManager;
    private TvControlManager mTvControlManager;
    private TvControlDataManager mTvControlDataManager = null;
    private SystemControlManager mSystemControlManager;
    private TvStoreManager mTvStoreManager;
    private PendingTuneEvent mPendingTune = new PendingTuneEvent();
    private ContentResolver mContentResolver;
    private MediaCodec mMediaCodec1;
    private MediaCodec mMediaCodec2;
    private MediaCodec mMediaCodec3;
    private AudioManager mAudioManager;
    private AudioEffectManager mAudioEffectManager;
    private static int mCurrentUserId = 0;//UserHandle.USER_SYSTEM
    private ActivityManager mActivityManager;

    private HardwareCallback mHardwareCallback = new HardwareCallback(){
        @Override
        public void onReleased() {
            if (DEBUG)
                Log.d(TAG, "onReleased");

            mHardware = null;
        }

        @Override
        public void onStreamConfigChanged(TvStreamConfig[] configs) {
            if (DEBUG)
                Log.d(TAG, "onStreamConfigChanged");
            mConfigs = configs;
            if (DEBUG && mConfigs != null)
                Log.d(TAG, "mConfigs.length:"+mConfigs.length);
            else
                Log.d(TAG, "mConfigs = null");

            if (mHardware != null && mConfigs != null) {
                if (mConfigs.length == 0) {
                    mHardware.setSurface(null, null);
                    /*SystemControlManager mSystemControlManager = new SystemControlManager(mContext);
                    mSystemControlManager.SetCurrentSourceInputInfo(SystemControlManager.SourceInput.values()[mDeviceId]);
                    if (mSession != null) {
                        Log.d(TAG, " Plug out & Input is not conneted,show overlay infomations");
                        mSession.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL);
                    }*/
                }  else if (mConfigs.length > 0 && mSurface != null){
                    Log.d(TAG, "open source");
                    createDecoder();
                    decoderRelease();
                    mHardware.setSurface(mSurface, mConfigs[0]);
                }
            } else {
                if (DEBUG)
                    Log.d(TAG, "Hardware or config is not prepared ,discard processing it");
            }
        }
    };

    public void registerChannelScanStartReceiver() {
        IntentFilter filter= new IntentFilter();
        filter.addAction(DroidLogicTvUtils.ACTION_DTV_AUTO_SCAN);
        filter.addAction(DroidLogicTvUtils.ACTION_DTV_MANUAL_SCAN);
        filter.addAction(DroidLogicTvUtils.ACTION_ATV_AUTO_SCAN);
        filter.addAction(DroidLogicTvUtils.ACTION_ATV_MANUAL_SCAN);
        registerReceiver(mChannelScanStartReceiver, filter);
    }

    public void unRegisterChannelScanStartReceiver() {
        unregisterReceiver(mChannelScanStartReceiver);
    }

    @Override
    public void onCreate() {
        super.onCreate();
        mTvInputManager = (TvInputManager)this.getSystemService(Context.TV_INPUT_SERVICE);
        mSystemControlManager = SystemControlManager.getInstance();
        mTvControlManager = TvControlManager.getInstance();
        mAudioManager = (AudioManager)this.getSystemService (Context.AUDIO_SERVICE);
        mTvControlDataManager = TvControlDataManager.getInstance(this);
        mContentResolver = this.getContentResolver();
        mActivityManager = (ActivityManager) getSystemService(Context.ACTIVITY_SERVICE);
        initTvPlaySetting();
        startTvServices();
        mAudioEffectManager = AudioEffectManager.getInstance(getApplicationContext());
    }

    /**
     * inputId should get from subclass which must invoke {@link super#onCreateSession(String)}
     */
    @Override
    public Session onCreateSession(String inputId) {
        registerInput(inputId);
        return null;
    }

    public void registerInput(String inputId) {
        Log.d(TAG, "register Input:"+inputId);
        mContext = getApplicationContext();
        mCurrentInputId = inputId;
        if (mSessionHandler == null)
            mSessionHandler = new SurfaceHandler();
    }

    protected void initInputService(int sourceType, String className) {
        mSourceType = sourceType;
        mChildClassName = className;
    }

    protected void acquireHardware(TvInputInfo info){
        mDeviceId = getHardwareDeviceId(info.getId());
        mCurrentInputId = info.getId();
        mHardware = mTvInputManager.acquireTvInputHardware(mDeviceId, info, mHardwareCallback);
        Log.d(TAG, "acquireHardware mDeviceId="+mDeviceId+",  mCurrentInputId="+mCurrentInputId+", mHardware: " + mHardware);
    }

    protected void releaseHardware(){
        Log.d(TAG, "releaseHardware , mHardware: " + mHardware);
        if (mHardware != null && mDeviceId != -1) {
            mConfigs = null;
            mHardware = null;
        }

        //if hdmi signal is unstable from stable, disconnect cec.
        if (mDeviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1
                && mDeviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI4) {
            DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
            if (hdmi_cec.getInputSourceDeviceId() == mDeviceId)
                selectHdmiDevice(0, 0, 0);
        }
    }

    /**
     * get session has been created by {@code onCreateSession}, and input id of session.
     * @param session {@link HdmiInputSession} or {@link AVInputSession}
     */
    protected void registerInputSession(TvInputBaseSession session) {
        mSession = session;
        if (session == null) {
            return;
        }
        mCurrentSessionId = session.mId;
        Log.d(TAG, "registerInputSession:  inputId="+mCurrentInputId+  " sessioniId=" + session.mId);

        if (mCurrentInputId != null) {
            final TvInputInfo currentinfo = mTvInputManager.getTvInputInfo(mCurrentInputId);
            if (currentinfo != null && currentinfo.getType() == TvInputInfo.TYPE_TUNER) {
                initTvStoreManager();
                resetScanStoreListener();
            }
        }

        mTvControlManager.SetSigInfoChangeListener(this);
        mTvControlManager.SetSourceConnectListener(this);
        mTvControlManager.setScanningFrameStableListener(this);
    }

    /**
     * update {@code mInfoList} when hardware device is added or removed.
     * @param hInfo {@linkHardwareInfo} get from HAL.
     * @param info {@link TvInputInfo} will be added or removed.
     * @param isRemoved {@code true} if you want to remove info. {@code false} otherwise.
     */
    protected void updateInfoListIfNeededLocked(TvInputHardwareInfo hInfo,
            TvInputInfo info, boolean isRemoved) {
        updateInfoListIfNeededLocked(hInfo.getDeviceId(), info, isRemoved);
    }

    protected void updateInfoListIfNeededLocked(int Id, TvInputInfo info,
            boolean isRemoved) {
        if (isRemoved) {
            mInfoList.remove(Id);
        } else {
            mInfoList.put(Id, info);
        }

        if (DEBUG) {
            for (int i = 0; i < mInfoList.size(); i++) {
                Log.d(TAG, "size of mInfoList NO." + i + ":" + mInfoList.get(mInfoList.keyAt(i)));
            }
        }
    }

    protected boolean hasInfoExisted(TvInputHardwareInfo hInfo) {
        return mInfoList.get(hInfo.getDeviceId()) == null ? false : true;
    }

    protected TvInputInfo getTvInputInfo(TvInputHardwareInfo hardwareInfo) {
        return mInfoList.get(hardwareInfo.getDeviceId());
    }

    protected TvInputInfo getTvInputInfo(int devId) {
        return mInfoList.get(devId);
    }

    protected int getHardwareDeviceId(String input_id) {
        int id = 0;
        TvInputInfo info = null;
        String parentId = null;
        for (int i = 0; i < mInfoList.size(); i++) {
            if (input_id.equals(mInfoList.valueAt(i).getId())) {
                info = mInfoList.valueAt(i);
                id = mInfoList.keyAt(i);
                break;
            }
        }
        if (id > DroidLogicTvUtils.DEVICE_ID_OFFSET) {
            parentId = info.getParentId();
            Log.d(TAG, input_id + "'s parentId: " + parentId);
            for (int i = 0; i < mInfoList.size(); i++) {
                if (parentId != null && parentId.equals(mInfoList.valueAt(i).getId())) {
                    id = mInfoList.keyAt(i);
                    break;
                }
            }
        }
        if (DEBUG)
            Log.d(TAG, "device id is " + id);
        return id;
    }

    protected String getTvInputInfoLabel(int device_id) {
        String label = null;
        switch (device_id) {
        case DroidLogicTvUtils.DEVICE_ID_ATV:
            label = ChannelInfo.LABEL_ATV;
            break;
        case DroidLogicTvUtils.DEVICE_ID_DTV:
            label = ChannelInfo.LABEL_DTV;
            break;
        case DroidLogicTvUtils.DEVICE_ID_AV1:
            label = ChannelInfo.LABEL_AV1;
            break;
        case DroidLogicTvUtils.DEVICE_ID_AV2:
            label = ChannelInfo.LABEL_AV2;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI1:
            label = ChannelInfo.LABEL_HDMI1;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI2:
            label = ChannelInfo.LABEL_HDMI2;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI3:
            label = ChannelInfo.LABEL_HDMI3;
            break;
        case DroidLogicTvUtils.DEVICE_ID_HDMI4:
            label = ChannelInfo.LABEL_HDMI4;
            break;
        case DroidLogicTvUtils.DEVICE_ID_SPDIF:
            label = ChannelInfo.LABEL_SPDIF;
            break;
        case DroidLogicTvUtils.DEVICE_ID_AUX:
            label = ChannelInfo.LABEL_AUX;
            break;
        case DroidLogicTvUtils.DEVICE_ID_ARC:
            label = ChannelInfo.LABEL_ARC;
            break;
        default:
            break;
        }
        return label;
    }

    protected ResolveInfo getResolveInfo(String cls_name) {
        if (TextUtils.isEmpty(cls_name))
            return null;
        ResolveInfo ret_ri = null;
        PackageManager pm = getApplicationContext().getPackageManager();
        List<ResolveInfo> services = pm.queryIntentServices(
                new Intent(TvInputService.SERVICE_INTERFACE),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);

        for (ResolveInfo ri : services) {
            ServiceInfo si = ri.serviceInfo;
            if (!android.Manifest.permission.BIND_TV_INPUT.equals(si.permission)) {
                continue;
            }

            if (DEBUG)
                Log.d(TAG, "cls_name = " + cls_name + ", si.name = " + si.name);

            if (cls_name.equals(si.name)) {
                ret_ri = ri;
                break;
            }
        }
        return ret_ri;
    }

    protected void stopTv() {
        Log.d(TAG, "stop tv, mCurrentInputId =" + mCurrentInputId);
        mTvControlManager.StopTv();
    }

    protected void releasePlayer() {
    }

    private String getInfoLabel() {
        final TvInputInfo tvinputinfo = mTvInputManager.getTvInputInfo(mCurrentInputId);
        String label = null;
        if (tvinputinfo != null) {
            label = tvinputinfo.loadLabel(this).toString();
        }
        return label;
    }

    @Override
    public void onSigChange(TvInSignalInfo signal_info) {
        TvInSignalInfo.SignalStatus status = signal_info.sigStatus;

        if (DEBUG)
            Log.d(TAG, "onSigChange: " + status.ordinal() + ", "+ status.toString());
        if (mSession == null) {
            Log.w(TAG, "mSession is null ,discard this signal!");
            return;
        }
        onSigChanged(signal_info);
        if (status == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NOSIG
                || status == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NULL
                || status == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_NOTSUP) {
            Log.d(TAG, "onSigChange-nosignal");
            mSession.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
        } else if (status == TvInSignalInfo.SignalStatus.TVIN_SIG_STATUS_STABLE) {
            int device_id = mSession.getDeviceId();
            if ((mTvControlManager.GetCurrentSourceInput() != DroidLogicTvUtils.DEVICE_ID_DTV)
                || (signal_info.reserved == 1))
                mSession.notifyVideoAvailable();

            String[] strings;
            Bundle bundle = new Bundle();
            switch (device_id) {
            case DroidLogicTvUtils.DEVICE_ID_HDMI1:
            case DroidLogicTvUtils.DEVICE_ID_HDMI2:
            case DroidLogicTvUtils.DEVICE_ID_HDMI3:
            case DroidLogicTvUtils.DEVICE_ID_HDMI4:
                if (DEBUG)
                    Log.d(TAG, "signal_info.fmt.toString() for hdmi=" + signal_info.sigFmt.toString());

                strings = signal_info.sigFmt.toString().split("_");
                TvInSignalInfo.SignalFmt fmt = signal_info.sigFmt;
                if (fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_60HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_120HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X480I_240HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_2880X480I_60HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_2880X480I_60HZ) {
                    strings[4] = "480I";
                } else if (fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_50HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_100HZ
                        || fmt == TvInSignalInfo.SignalFmt.TVIN_SIG_FMT_HDMI_1440X576I_200HZ) {
                    strings[4] = "576I";
                }

                bundle.putInt(DroidLogicTvUtils.SIG_INFO_TYPE, DroidLogicTvUtils.SIG_INFO_TYPE_HDMI);
                bundle.putString(DroidLogicTvUtils.SIG_INFO_LABEL, getInfoLabel());
                if (strings != null && strings.length <= 4) {
                    Log.d(TAG, "invalid signal");
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, " ");
                } else if (mTvControlManager.IsDviSignal()) {
                    Log.d(TAG, "dvi signal");
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, "DVI "+strings[4]
                            + "_" + signal_info.reserved + "HZ");
                    mAudioManager.setParameters("audio=linein");
                } else {
                    Log.d(TAG, "hdmi signal");
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, strings[4]
                            + "_" + signal_info.reserved + "HZ");
                    mAudioManager.setParameters("audio=hdmi");
                }

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, bundle);
                break;
            case DroidLogicTvUtils.DEVICE_ID_AV1:
            case DroidLogicTvUtils.DEVICE_ID_AV2:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for av=" + signal_info.sigFmt.toString());

                strings = signal_info.sigFmt.toString().split("_");
                bundle.putInt(DroidLogicTvUtils.SIG_INFO_TYPE, DroidLogicTvUtils.SIG_INFO_TYPE_AV);
                bundle.putString(DroidLogicTvUtils.SIG_INFO_LABEL, getInfoLabel());
                if (strings != null && strings.length <= 4)
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, "");
                else
                    bundle.putString(DroidLogicTvUtils.SIG_INFO_ARGS, strings[4]);
                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, bundle);
                break;
            case DroidLogicTvUtils.DEVICE_ID_ATV:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for atv=" + signal_info.sigFmt.toString());

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, null);
                break;
            case DroidLogicTvUtils.DEVICE_ID_DTV:
                if (DEBUG)
                    Log.d(TAG, "tmpInfo.fmt.toString() for dtv=" + signal_info.sigFmt.toString());

                mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EVENT, null);
                break;
            default:
                break;
            }
        }
    }

    public void onSigChanged(TvInSignalInfo signal_info) { }

    @Override
    public void onSourceConnectChange(TvControlManager.SourceInput source, int connectionState) {
        if (DEBUG)
            Log.d(TAG, "onSourceConnectChange:  source=" + source.toInt() + " connectionState=" + connectionState);

        if (mSession == null) {
            Log.w(TAG, "mSession is null ,discard onSourceConnectChange!");
            return;
        }

        if (mDeviceId == source.toInt() && connectionState == TvInSignalInfo.SOURCE_DISCONNECTED) {
            mSession.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
        }
    }

    @Override
    public void StorDBonEvent(TvControlManager.ScannerEvent event) {
        if (mTvStoreManager != null) {
            mTvStoreManager.onStoreEvent(event);
        } else {
            Log.w(TAG, "StorDBonEvent mTvStoreManager null, service-->mCurrentInputId:" + mCurrentInputId + ", mSourceType:"+mSourceType + ", mChildClassName =" + mChildClassName);
        }
    }

    public void resetScanStoreListener() {
        mTvControlManager.setStorDBListener(this);
        Log.d(TAG, "resetScanStoreListener service-->mCurrentInputId:" + mCurrentInputId + ", mSourceType:"+mSourceType + ", mChildClassName =" + mChildClassName);
    }

    @Override
    public void onFrameStable(TvControlManager.ScanningFrameStableEvent event) {
        Log.d(TAG, "scanning frame stable!");
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_C_FREQ, event.CurScanningFrq);
        if (mSession != null)
            mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_C_SCANNING_FRAME_STABLE_EVENT, bundle);
    }

    public void onUpdateCurrentChannel(ChannelInfo channel, boolean store) {}

    public final class SomeArgs {
        public Object arg1;
        public Object arg2;

        public SomeArgs() {

        }

        public void clear() {
            arg1 = null;
            arg2 = null;
        }
    }

    protected  boolean setSurfaceInService(Surface surface, TvInputBaseSession session ) {
        Log.d(TAG, "setSurfaceInService, surface: " + surface + " session: " + session);

        Message message = mSessionHandler.obtainMessage();
        message.what = MSG_DO_SET_SURFACE;
        synchronized(this) {
            SomeArgs args = new SomeArgs();
            args.arg1 = surface;
            args.arg2 = session;

            message.obj = args;
        }
        mSessionHandler.sendMessageAtFrontOfQueue(message);

        if (surface == null && session != null) {
            session.hideUI();
        } else if (surface != null) {
            setAudioDelay(true);
        }
        return false;
    }

    protected  boolean doTuneInService(Uri channelUri, int sessionId) {
        Log.d(TAG, "doTuneInService onTune channelUri=" + channelUri);

        Message message = mSessionHandler.obtainMessage(MSG_DO_TUNE, sessionId, 0, channelUri);
        mSessionHandler.sendMessageAtFrontOfQueue(message);
        if (mSession != null)
            mSession.hideUI();

        return false;
    }

    private final class SurfaceHandler extends Handler {
        @Override
        public void handleMessage(Message message) {
        if (DEBUG)
            Log.d(TAG, "handleMessage, msg.what=" + message.what);
        switch (message.what) {
            case MSG_DO_TUNE:
                mSessionHandler.removeMessages(MSG_DO_TUNE);
                doTune((Uri)message.obj, message.arg1);
                break;
            case MSG_DO_SET_SURFACE:
                SomeArgs args = (SomeArgs) message.obj;
                doSetSurface((Surface)args.arg1, (TvInputBaseSession)args.arg2);
                break;
            }
        }
    }

    private boolean createDecoder() {
        String str = "OMX.amlogic.avc.decoder.awesome.secure";
        try {
            mMediaCodec1 = MediaCodec.createByCodecName(str);
            } catch (Exception exception) {
            Log.d(TAG, "Exception during decoder1 creation", exception);
            decoderRelease();
            return false;
        }
        try {
            mMediaCodec2 = MediaCodec.createByCodecName(str);
            } catch (Exception exception) {
            Log.d(TAG, "Exception during decoder2 creation", exception);
            decoderRelease();
            return false;
        }
        try {
            mMediaCodec3 = MediaCodec.createByCodecName(str);
            } catch (Exception exception) {
            Log.d(TAG, "Exception during decoder3 creation", exception);
            decoderRelease();
            return false;
        }

        Log.d(TAG, "createDecoder done");
        return true;
    }

    private void decoderRelease() {
        if (mMediaCodec1 != null) {
            try {
                mMediaCodec1.stop();
                } catch (IllegalStateException exception) {
                mMediaCodec1.reset();
                // IllegalStateException happens when decoder fail to start.
                Log.d(TAG, "IllegalStateException during decoder1 stop", exception);
                } finally {
                    try {
                        mMediaCodec1.release();
                    } catch (IllegalStateException exception) {
                        Log.d(TAG, "IllegalStateException during decoder1 release", exception);
                    }
                    mMediaCodec1 = null;
            }
        }

        if (mMediaCodec2 != null) {
            try {
                mMediaCodec2.stop();
                } catch (IllegalStateException exception) {
                mMediaCodec2.reset();
                // IllegalStateException happens when decoder fail to start.
                Log.d(TAG, "IllegalStateException during decoder2 stop", exception);
                } finally {
                    try {
                        mMediaCodec2.release();
                    } catch (IllegalStateException exception) {
                        Log.d(TAG, "IllegalStateException during decoder2 release", exception);
                    }
                    mMediaCodec2 = null;
            }
        }

        if (mMediaCodec3 != null) {
            try {
                mMediaCodec3.stop();
                } catch (IllegalStateException exception) {
                mMediaCodec3.reset();
                // IllegalStateException happens when decoder fail to start.
                Log.d(TAG, "IllegalStateException during decoder3 stop", exception);
                } finally {
                    try {
                        mMediaCodec3.release();
                    } catch (IllegalStateException exception) {
                        Log.d(TAG, "IllegalStateException during decoder3 release", exception);
                    }
                    mMediaCodec3 = null;
            }
        }

        Log.d(TAG, "decoderRelease done");
    }


    private void doSetSurface(Surface surface, TvInputBaseSession session) {
        Log.d(TAG, "doSetSurface inputId=" + mCurrentInputId + " number=" + session.mId + " surface=" + surface);

        if (surface == null) {
            Log.d(TAG, "doSetSurface surface is null SwitchSourceTime = " + getUptimeSeconds());
            if (mHardware != null && mConfigs != null
                    && mSession != null && session.mId == mSession.mId) {
                Log.d(TAG, "exited TV source, so stop TV play");
                mSurface = null;
                stopTvPlay(session.mId);
                setAudioDelay(false);
            }
            Log.d(TAG, "surface is null finish, return SwitchSourceTime = " + getUptimeSeconds());
            return;
        }
        Log.d(TAG, "doSetSurface surface is not null SwitchSourceTime = " + getUptimeSeconds());
        if (surface != null && !surface.isValid()) {
            Log.d(TAG, "onSetSurface get invalid surface");
            return;
        } else if (surface != null) {
            if (mHardware != null && mSurface != null
                && (mSourceType >= DroidLogicTvUtils.DEVICE_ID_HDMI1)
                && (mSourceType <= DroidLogicTvUtils.DEVICE_ID_HDMI4)) {
                    //stopTvPlay(mSession.mId);
                    Log.e(TAG, "This should be never be print. If so, please check");
            }
            if (mSurface != null && surface != null && mSurface != surface && mSession != null) {
                Log.d(TAG, "TvView swithed,  stopTvPlay before tuning");
                stopTvPlay(mSession.mId);
            }
            registerInputSession(session);
            setCurrentSessionById(session.mId);
            mSurface = surface;
        }

        if (mHardware != null && mSurface != null && mConfigs.length > 0 && mSurface.isValid()) {
            createDecoder();
            decoderRelease();
            mHardware.setSurface(mSurface, mConfigs[0]);
        }

        if (mPendingTune.hasPendingEventToProcess(session.mId)) {
            doTune(mPendingTune.mUri, mPendingTune.mSessionId);
            mPendingTune.reset();
        }
    }

    public int doTune(Uri uri, int sessionId) {
        Log.d(TAG, "doTune, uri = " + uri + "SwitchSourceTime = " + getUptimeSeconds());
        int result = startTvPlay();
        Log.d(TAG, "startTvPlay,result= " + result);
        if (mConfigs.length == 0 || result != ACTION_SUCCESS) {
            doTuneFinish(ACTION_FAILED, uri, sessionId);
            if (result == ACTION_PENDING)
                mPendingTune.setPendingEvent(uri, sessionId);
            else if (result == ACTION_FAILED && mSession != null)
                mSession.notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
            return ACTION_FAILED;
        }
        if (mSystemControlManager != null) {
            mSystemControlManager.writeSysFs("/sys/class/deinterlace/di0/config", "hold_video 0");
            //ATV,HDMI,AV don't need tsync,or it will cause static frame.
            //online video and DTV will enable it automatically
            mSystemControlManager.writeSysFs("/sys/class/tsync/enable", "0");
        }

        doTuneFinish(ACTION_SUCCESS, uri, sessionId);
        return ACTION_SUCCESS;
    }

    private int startTvPlay() {
        Log.d(TAG, "startTvPlay mHardware=" + mHardware + " mConfigs.length=" + mConfigs.length);
        if (mHardware != null && mConfigs.length > 0) {
            if (mSurface == null) {
                return ACTION_PENDING;
            } else {
                return ACTION_SUCCESS;
            }
            //mHardware.setSurface(mSurface, mConfigs[0]);
            /*if ((mDeviceId >= DroidLogicTvUtils.DEVICE_ID_HDMI1)
                    && (mDeviceId <= DroidLogicTvUtils.DEVICE_ID_HDMI4)) {
                selectHdmiDevice(mDeviceId);
            }*/
        }
        return ACTION_FAILED;
    }

    private int stopTvPlay(int sessionId) {
        Log.d(TAG, "stopTvPlay:"+sessionId+" mHardware:"+mHardware);
        if (mHardware != null && mConfigs.length > 0) {
            mHardware.setSurface(null, mConfigs[0]);
            tvPlayStopped(sessionId);
        }
        //disconnectHdmiCec(0, 0, 0);
        return ACTION_SUCCESS;
    }
    public void setCurrentSessionById(int sessionId){}
    public void doTuneFinish(int result, Uri uri, int sessionId){};
    public void tvPlayStopped(int sessionId){};

    protected int getCurrentSessionId() {
        return mCurrentSessionId;
    }
    /**
     * select hdmi cec device.
     * @param deviceId the hardware device id of hdmi need to be selected.
     */
    public void selectHdmiDevice(final int deviceId, int logicAddr, int phyAddr) {
        DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
        hdmi_cec.selectHdmiDevice(deviceId, logicAddr, phyAddr);
    }

    /**
     * reset the status for hdmi cec device.
     * @param device the hardware device id of hdmi need to be selected.
     */
    public void disconnectHdmiCec(int deviceId, int logicAddr, int phyAddr) {
        DroidLogicHdmiCecManager hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
        hdmi_cec.selectHdmiDevice(deviceId, logicAddr, phyAddr);
    }

    private int getHdmiPortIndex(int phyAddr) {
        /* TODO: consider of tuner */
        return ((phyAddr & 0xf000) >> 12) - 1;
    }

    @Override
    public TvInputInfo onHdmiDeviceAdded(HdmiDeviceInfo deviceInfo) {
        if (deviceInfo == null) {
            return null;
        }
        int phyaddr = deviceInfo.getPhysicalAddress();
        int hdmiDeviceId = deviceInfo.getId();
        int keyDeviceId = hdmiDeviceId + DroidLogicTvUtils.DEVICE_ID_OFFSET;
        int sourceType = getHdmiPortIndex(phyaddr) + DroidLogicTvUtils.DEVICE_ID_HDMI1;
        if (sourceType < DroidLogicTvUtils.DEVICE_ID_HDMI1
                || sourceType > DroidLogicTvUtils.DEVICE_ID_HDMI4
                || sourceType != mSourceType)
            return null;
        Log.d(TAG, "onHdmiDeviceAdded, sourceType = " + sourceType + ", mSourceType = " + mSourceType);

        if (getTvInputInfo(keyDeviceId) != null) {
            Log.d(TAG, "onHdmiDeviceAdded, phyaddr:" + phyaddr + " already add");
            return null;
        }

        String parentId = null;
        TvInputInfo info = null;
        TvInputInfo parent = getTvInputInfo(sourceType);
        if (parent != null) {
            parentId = parent.getId();
        } else {
            Log.d(TAG, "onHdmiDeviceAdded, can't found parent");
            return null;
        }
        Log.d(TAG, "onHdmiDeviceAdded, logicAddr:" + hdmiDeviceId +
                    ", sourceType:" + sourceType + ", parentId: " + parentId);
        ResolveInfo rInfo = getResolveInfo(mChildClassName);
        if (rInfo != null) {
            try {
                info = TvInputInfo.createTvInputInfo(
                        getApplicationContext(),
                        rInfo,
                        deviceInfo,
                        parentId,
                        deviceInfo.getDisplayName(),
                        null);
            } catch (XmlPullParserException e) {
                // TODO: handle exception
            }catch (IOException e) {
                // TODO: handle exception
            }
        } else {
            return null;
        }

        Log.d(TAG, "createTvInputInfo, id:" + deviceInfo.toString()+",deviceId: "+deviceInfo.getDeviceId());
        setInputName(info, deviceInfo.getDisplayName());
        updateInfoListIfNeededLocked(keyDeviceId, info, false);
        //selectHdmiDevice(sourceType);
        selectHdmiDevice(sourceType, hdmiDeviceId, phyaddr);

        return info;
    }

    @Override
    public String onHdmiDeviceRemoved(HdmiDeviceInfo deviceInfo) {
        if (deviceInfo == null)
            return null;
        int phyaddr = deviceInfo.getPhysicalAddress();
        int hdmiDeviceId = deviceInfo.getId();
        int keyDeviceId = hdmiDeviceId + DroidLogicTvUtils.DEVICE_ID_OFFSET;
        int sourceType = getHdmiPortIndex(phyaddr) + DroidLogicTvUtils.DEVICE_ID_HDMI1;

        if (sourceType < DroidLogicTvUtils.DEVICE_ID_HDMI1
                || sourceType > DroidLogicTvUtils.DEVICE_ID_HDMI4
                || sourceType != mSourceType)
            return null;

        Log.d(TAG, "onHdmiDeviceRemoved, sourceType = " + sourceType + ", mSourceType = " + mSourceType);
        TvInputInfo info = getTvInputInfo(keyDeviceId);
        if (info == null)
            return null;

        String id = info.getId();
        Log.d(TAG, "onHdmiDeviceRemoved, id:" + id);
        setInputName(info, null);
        updateInfoListIfNeededLocked(keyDeviceId, info, true);
        disconnectHdmiCec(sourceType, 0, phyaddr);

        return id;
    }

    private void setInputName(TvInputInfo mInputInfo, CharSequence name) {
        if (mInputInfo == null)
            return;
        Map<String, String> mCustomLabels;
        mCustomLabels = TvInputInfo.TvInputSettings.getCustomLabels(getApplicationContext(), mCurrentUserId);
        if (TextUtils.isEmpty(name)) {
            mCustomLabels.remove(mInputInfo.getId());
        } else {
            mCustomLabels.put(mInputInfo.getId(), name.toString());
        }
        Log.d(TAG, "setInputName, Id:" + mInputInfo.getId() + " name: " + name);
        TvInputInfo.TvInputSettings
                .putCustomLabels(getApplicationContext(), mCustomLabels, mCurrentUserId);
    }

    protected final BroadcastReceiver mChannelScanStartReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
                 String action = intent.getAction();
                 Bundle bundle = intent.getBundleExtra(DroidLogicTvUtils.EXTRA_MORE);
                 String inputId = bundle.getString(TvInputInfo.EXTRA_INPUT_ID);
                 Log.d(TAG, "mCurrentInputId:"+mCurrentInputId+", inputId:"+inputId);
                 if (mCurrentInputId != null && mCurrentInputId.equals(inputId)) {
                     registerInput(inputId);
                     resetScanStoreListener();
                     initTvStoreManager();
                 }
            }
    };

    private void initTvStoreManager() {
            if (mTvStoreManager == null) {
                int channel_number_start = 1;
                if (mSystemControlManager != null)
                    channel_number_start = mSystemControlManager.getPropertyInt("tv.channel.number.start", 1);
                mTvStoreManager = new TvStoreManager(this, mCurrentInputId, channel_number_start) {
                @Override
                public void onEvent(String eventType, Bundle eventArgs) {
                    Log.d(TAG, "TvStoreManager.onEvent:"+eventType);
                    if (mSession != null)
                        mSession.notifySessionEvent(eventType, eventArgs);
                    else
                        Log.d(TAG, "mSession is null, no need to notify tvview.callback");
                }
                @Override
                public void onUpdateCurrent(ChannelInfo channel, boolean store) {
                    onUpdateCurrentChannel(channel, store);
                }
                @Override
                public void onDtvNumberMode(String mode) {
                    mTvControlDataManager.putString(DroidLogicTvInputService.this.getContentResolver(), DroidLogicTvUtils.TV_KEY_DTV_NUMBER_MODE, "lcn");
                }
                public void onScanEnd() {
                    mTvControlManager.DtvStopScan();
                }
            };
    }}

    private float getUptimeSeconds() {
       return  (float)SystemClock.uptimeMillis() / 1000;
    }

    public void notifyAppEasStatus(boolean isStarted){
        Log.d(TAG, "notifyAppEasStatus:"+isStarted);
        Bundle bundle = new Bundle();
        bundle.putInt(DroidLogicTvUtils.SIG_INFO_EAS_STATUS, isStarted ? 1 : 0);
        if (mSession != null) {
            mSession.notifySessionEvent(DroidLogicTvUtils.SIG_INFO_EAS_EVENT, bundle);
        }
    }

    private class PendingTuneEvent {
        private int mSessionId;
        private Uri mUri;
        private boolean hasPendingTune = false;

        public void PendingTuneEvent() {}
        public void setPendingEvent(Uri uri, int id) {
            if (DEBUG)
                Log.d(TAG, "PendingTune, uri=" + uri+ ",id:"+id);
            mSessionId = id;
            mUri = uri;
            hasPendingTune = true;
        }

        public boolean hasPendingEventToProcess(int id) {
            if (DEBUG)
                Log.d(TAG, "hasPendingEventToProcess, id=" + id+ ",mSessionId:"+mSessionId);
            return hasPendingTune && (mSessionId == id);
        }

        public void reset() {
            mUri = null;
            mSessionId = -1;
            hasPendingTune = false;
       }
    }

    public int getCaptionRawUserStyle() {
        return Secure.getInt(
                mContentResolver, ACCESSIBILITY_CAPTIONING_PRESET, 0);
    }

    private static final String INPUT_ID_ADTV = "com.droidlogic.tvinput/.services.ADTVInputService/HW16";
    private void initTvPlaySetting() {
        //tv_current_device_id
        if (mTvControlDataManager.getInt(getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID, -1) == -1) {
            mTvControlDataManager.putInt(getContentResolver(), DroidLogicTvUtils.TV_CURRENT_DEVICE_ID,
                      DroidLogicTvUtils.DEVICE_ID_ADTV);
        }

        //tv_current_inputid
        if (TextUtils.isEmpty(DroidLogicTvUtils.getCurrentInputId(this))) {
            DroidLogicTvUtils.setCurrentInputId(this, INPUT_ID_ADTV);
        }

        //tv_search_inputid
        if (TextUtils.isEmpty(DroidLogicTvUtils.getSearchInputId(this))) {
            mTvControlDataManager.putString(getContentResolver(), DroidLogicTvUtils.TV_SEARCH_INPUTID, INPUT_ID_ADTV);
        }

        //tv_search_type
        if (TextUtils.isEmpty(mTvControlDataManager.getString(getContentResolver(),
                DroidLogicTvUtils.TV_SEARCH_TYPE))) {
            String country = mTvControlManager.getTvDefaultCountry();
            if (!TextUtils.isEmpty(country)) {
                if (mTvControlManager.GetTvAtvSupport(country)) {
                    mTvControlDataManager.putString(getContentResolver(), DroidLogicTvUtils.TV_SEARCH_TYPE, "ATV");
                } else if (mTvControlManager.GetTvDtvSupport(country)) {
                    mTvControlDataManager.putString(getContentResolver(), DroidLogicTvUtils.TV_SEARCH_TYPE,
                            mTvControlManager.GetTvDtvSystem(country));
                }
            }
        }
    }

    private static final String SOUDND_EFFECT_PACKAGE_NAME ="com.droidlogic.tvinput";
    private static final String SOUDND_EFFECT_SERVICE_NAME ="com.droidlogic.tvinput.services.AudioEffectsService";
    private static final String CMD_START_SOUND_EFFECT = "com.droidlogic.tvinput.services.AudioEffectsService.STARTUP";

    private void startTvServices () {
        if  (isAppExist(SOUDND_EFFECT_PACKAGE_NAME) && !isServiceRunning(SOUDND_EFFECT_SERVICE_NAME)) {
            Intent intent = new Intent();
            intent.setComponent(new ComponentName(SOUDND_EFFECT_PACKAGE_NAME, SOUDND_EFFECT_SERVICE_NAME));
            intent.setAction(CMD_START_SOUND_EFFECT);
            startService(intent);
        }
    }

    private boolean isAppExist (String packageName) {
        if (TextUtils.isEmpty(packageName)) {
            return false;
        }

        try {
            ApplicationInfo info = this.getPackageManager().getApplicationInfo(packageName, 0);
            return info != null;
        } catch (NameNotFoundException e) {
            return false;
        }
    }

    private boolean isServiceRunning(String className){
        boolean isRunning = false ;
        List<ActivityManager.RunningServiceInfo> seviceList = mActivityManager.getRunningServices(200);
        if (seviceList.size() <= 0) {
            return false;
        }
        for (int i = 0 ;i < seviceList.size(); i++) {
            if (seviceList.get(i).service.getClassName().toString().equals(className)) {
                isRunning = true;
                break;
            }
        }
        return  isRunning;
    }

    // refresh audio delay param when select source or deselect source
    private void setAudioDelay(boolean isTvSource) {
        if (!getAudioDelayEnabled()) {
            return;
        }

        String searchType = DroidLogicTvUtils.getSearchType(mContext);
        int audioSource = 0;
        if (!isTvSource) {
            audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA;
        } else {
            if (DroidLogicTvUtils.DEVICE_ID_ADTV == mSourceType) {
                if (searchType.equals("ATV")) {
                    audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_ATV;
                } else {
                    audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_DTV;
                }
            } else if (DroidLogicTvUtils.DEVICE_ID_AV1 == mSourceType || DroidLogicTvUtils.DEVICE_ID_AV2 == mSourceType) {
                audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_AV;
            } else if (mSourceType >= DroidLogicTvUtils.DEVICE_ID_HDMI1 && mSourceType <= DroidLogicTvUtils.DEVICE_ID_HDMI4) {
                audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_HDMI;
            } else {
                audioSource = AudioEffectManager.AUDIO_OUTPUT_DELAY_SOURCE_MEDIA;
            }
        }
        Log.d(TAG, "setAudioDelay, audioSource:" + audioSource + ",speaker delay:" + mAudioEffectManager.getAudioOutputSpeakerDelay(audioSource)
                + ", spdif delay:"+ mAudioEffectManager.getAudioOutputSpdifDelay(audioSource));
        DroidLogicTvUtils.setTvSourceType(mContext, audioSource);
        mAudioEffectManager.setAudioOutputSpeakerDelay(audioSource, mAudioEffectManager.getAudioOutputSpeakerDelay(audioSource));
        mAudioEffectManager.setAudioOutputSpdifDelay(audioSource, mAudioEffectManager.getAudioOutputSpdifDelay(audioSource));
    }

    private boolean getAudioDelayEnabled () {
        return SystemControlManager.getInstance()
                .getPropertyBoolean(AudioEffectManager.PROP_AUDIO_DELAY_ENABLED, false);
    }
}
