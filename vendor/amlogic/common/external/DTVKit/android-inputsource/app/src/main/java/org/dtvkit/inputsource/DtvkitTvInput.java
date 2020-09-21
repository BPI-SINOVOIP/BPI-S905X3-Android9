package org.dtvkit.inputsource;

import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.ContentValues;
import android.content.Context;
import android.database.ContentObserver;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.graphics.Matrix;
import android.graphics.Rect;
import android.graphics.PorterDuff;
import android.graphics.Paint;
import android.database.Cursor;
import android.graphics.RectF;

import android.media.PlaybackParams;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputService;
import android.media.tv.TvTrackInfo;
import android.graphics.Color;
import android.widget.LinearLayout;
import android.widget.TextView;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.RelativeLayout;
import android.util.TypedValue;
import android.graphics.Typeface;

import android.content.IntentFilter;
import android.content.BroadcastReceiver;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.content.ContentProviderClient;
import android.content.ContentProviderOperation;
import android.content.ContentValues;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvInputManager.Hardware;
import android.media.tv.TvInputManager.HardwareCallback;
import android.media.tv.TvInputHardwareInfo;
import android.media.tv.TvStreamConfig;
import android.text.TextUtils;
import java.io.IOException;
import org.xmlpull.v1.XmlPullParserException;
import android.content.Intent;

import android.media.AudioManager;
import android.media.AudioRoutesInfo;
import android.media.AudioSystem;
import android.media.IAudioRoutesObserver;
import android.media.IAudioService;

import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.Looper;
import android.support.annotation.Nullable;
import android.support.annotation.RequiresApi;
import android.util.Log;
import android.util.LongSparseArray;
import android.view.Surface;
import android.view.View;
import android.view.KeyEvent;
import android.os.HandlerThread;
import android.text.TextUtils;
import android.widget.FrameLayout;
import android.view.accessibility.CaptioningManager;
import android.os.IBinder;
import android.os.RemoteException;
import android.os.ServiceManager;
import android.app.AlertDialog;
import android.content.DialogInterface;
import android.view.WindowManager;
import android.widget.Button;

import org.dtvkit.companionlibrary.EpgSyncJobService;
import org.dtvkit.companionlibrary.model.Channel;
import org.dtvkit.companionlibrary.model.InternalProviderData;
import org.dtvkit.companionlibrary.model.Program;
import org.dtvkit.companionlibrary.model.RecordedProgram;
import org.dtvkit.companionlibrary.utils.TvContractUtils;
import org.droidlogic.dtvkit.DtvkitGlueClient;
import org.json.JSONArray;
import org.json.JSONException;
import org.json.JSONObject;
/*
dtvkit
 */
import com.droidlogic.settings.PropSettingManager;
import com.droidlogic.settings.ConvertSettingManager;
import com.droidlogic.settings.SysSettingManager;
import com.droidlogic.settings.ConstantManager;
import com.droidlogic.fragment.ParameterMananer;

//import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.SystemControlManager;

import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.Semaphore;
import java.util.Objects;
import java.util.Iterator;
import java.util.Map;
import java.util.HashMap;
import java.util.Arrays;
import java.util.Objects;

import android.media.MediaCodec;
import android.widget.TextView;
import android.util.TypedValue;


public class DtvkitTvInput extends TvInputService implements SystemControlManager.DisplayModeListener  {
    private static final String TAG = "DtvkitTvInput";
    private LongSparseArray<Channel> mChannels;
    private ContentResolver mContentResolver;

    protected String mInputId = null;
   // private TvControlManager Tcm = null;
    private static final int MSG_DO_TRY_SCAN = 0;
    private static final int RETRY_TIMES = 10;
    private static final int ASPECT_MODE_AUTO = 0;
    private static final int ASPECT_MODE_CUSTOM = 5;
    private static final int DISPLAY_MODE_NORMAL = 6;
    private int retry_times = RETRY_TIMES;

    TvInputInfo mTvInputInfo = null;
    protected Hardware mHardware;
    protected TvStreamConfig[] mConfigs;
    private TvInputManager mTvInputManager;
    private Surface mSurface;
    private SysSettingManager mSysSettingManager = null;
    private DtvkitTvInputSession mSession;
    protected HandlerThread mHandlerThread = null;
    protected Handler mInputThreadHandler = null;

    /*associate audio*/
    protected boolean mAudioADAutoStart = false;
    protected int mAudioADMixingLevel = 50;

    private boolean mDvbNetworkChangeSearchStatus = false;

    private enum PlayerState {
        STOPPED, PLAYING
    }
    private enum RecorderState {
        STOPPED, STARTING, RECORDING
    }
    private RecorderState timeshiftRecorderState = RecorderState.STOPPED;
    private boolean timeshifting = false;
    private int numRecorders = 0;
    private int numActiveRecordings = 0;
    private boolean scheduleTimeshiftRecording = false;
    private Handler scheduleTimeshiftRecordingHandler = null;
    private Runnable timeshiftRecordRunnable;
    private long mDtvkitTvInputSessionCount = 0;
    private long mDtvkitRecordingSessionCount = 0;
    private DataMananer mDataMananer;
    private ParameterMananer mParameterMananer;
    private MediaCodec mMediaCodec1;
    private MediaCodec mMediaCodec2;
    private MediaCodec mMediaCodec3;
    SystemControlManager mSystemControlManager;
    //Define file type
    private final int SYSFS = 0;
    private final int PROP = 1;

    private boolean recordingPending = false;

    private IAudioService mAudioService;
    private AudioRoutesInfo mCurAudioRoutesInfo;
    private Runnable mHandleTvAudioRunnable;
    private int mDelayTime;
    private int mCurAudioFmt;
    private int mCurSubAudioFmt = -2;
    private int mCurSubAudioPid;
    final IAudioRoutesObserver.Stub mAudioRoutesObserver = new IAudioRoutesObserver.Stub() {
        @Override
        public void dispatchAudioRoutesChanged(final AudioRoutesInfo newRoutes) {
            if ((mSession != null && mSession.mTunedChannel != null &&
                     mSession.mHandlerThreadHandle != null) &&
                    ((newRoutes.mainType != mCurAudioRoutesInfo.mainType) ||
                        (!newRoutes.toString().equals(mCurAudioRoutesInfo.toString())))) {
                mCurAudioRoutesInfo = newRoutes;
                mSession.mHandlerThreadHandle.removeCallbacks(mHandleTvAudioRunnable);
                mDelayTime = newRoutes.mainType != AudioRoutesInfo.MAIN_HDMI ? 500 : 1500;
                mHandleTvAudioRunnable = new Runnable() {
                    public void run() {
                        AudioSystem.setParameters("tuner_in=dtv");
                        AudioSystem.setParameters("fmt=" + mCurAudioFmt);
                        AudioSystem.setParameters("has_dtv_video=" +
                                (mSession.mTunedChannel.getServiceType().equals("SERVICE_TYPE_AUDIO_VIDEO") ? "1" : "0"));
                        AudioSystem.setParameters("cmd=1");
                        if (mCurSubAudioFmt != -2) {
                            if (mAudioADAutoStart) {
                                if (mCurSubAudioPid == 0) {
                                    AudioSystem.setParameters("dual_decoder_support=0");
                                    AudioSystem.setParameters("associate_audio_mixing_enable=0");
                                } else if (mCurSubAudioPid != 0 && mCurSubAudioPid != 0x1fff) {
                                    AudioSystem.setParameters("dual_decoder_support=1");
                                    AudioSystem.setParameters("associate_audio_mixing_enable=1");
                                    AudioSystem.setParameters("dual_decoder_mixing_level=" + mAudioADMixingLevel + "");
                                }
                            }
                            AudioSystem.setParameters("cmd=5");
                            AudioSystem.setParameters("subafmt="+mCurSubAudioFmt);
                            AudioSystem.setParameters("subapid="+mCurSubAudioPid);
                        }
                    }
                };
                try {
                    mSession.mHandlerThreadHandle.postDelayed(mHandleTvAudioRunnable,
                            mAudioService.isBluetoothA2dpOn() ? mDelayTime + 2000 : mDelayTime);
                } catch (RemoteException e) {
                    e.printStackTrace();
                }
            } else {
                mCurAudioRoutesInfo = newRoutes;
            }
        }
    };

    public DtvkitTvInput() {
        Log.i(TAG, "DtvkitTvInput");
    }

    protected final BroadcastReceiver mParentalControlsBroadcastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (action.equals(TvInputManager.ACTION_BLOCKED_RATINGS_CHANGED)
                   || action.equals(TvInputManager.ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED)) {
               boolean isParentControlEnabled = mTvInputManager.isParentalControlsEnabled();
               Log.d(TAG, "BLOCKED_RATINGS_CHANGED isParentControlEnabled = " + isParentControlEnabled);
               /*if (isParentControlEnabled != getParentalControlOn()) {
                   setParentalControlOn(isParentControlEnabled);
               }
               if (isParentControlEnabled && mSession != null) {
                   mSession.syncParentControlSetting();
               }*/
            }
        }
    };

    protected final BroadcastReceiver mBootBroadcastReceiver = new BroadcastReceiver() {

        @Override
        public void onReceive(Context context, Intent intent) {
            if (intent != null) {
                String action = intent.getAction();
                if (action.equals(Intent.ACTION_LOCKED_BOOT_COMPLETED)) {
                    Log.d(TAG, "onReceive ACTION_LOCKED_BOOT_COMPLETED");
                    //can't init here as storage may not be ready
                } else if (action.equals(Intent.ACTION_BOOT_COMPLETED )) {
                    Log.d(TAG, "onReceive ACTION_BOOT_COMPLETED");
                    initInThread();
                }
            }
        }
    };

    @RequiresApi(api = Build.VERSION_CODES.N)
    @Override
    public void onCreate() {
        super.onCreate();
        Log.i(TAG, "onCreate");
        // Create background thread
        mHandlerThread = new HandlerThread("DtvkitInputWorker");
        mHandlerThread.start();
        mTvInputManager = (TvInputManager)this.getSystemService(Context.TV_INPUT_SERVICE);
        mContentResolver = getContentResolver();
        mContentResolver.registerContentObserver(TvContract.Channels.CONTENT_URI, true, mContentObserver);

        IntentFilter intentFilter = new IntentFilter();
        intentFilter.addAction(TvInputManager.ACTION_BLOCKED_RATINGS_CHANGED);
        intentFilter.addAction(TvInputManager.ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED);
        registerReceiver(mParentalControlsBroadcastReceiver, intentFilter);

        IntentFilter intentFilter1 = new IntentFilter();
        intentFilter1.addAction(Intent.ACTION_LOCKED_BOOT_COMPLETED);
        intentFilter1.addAction(Intent.ACTION_BOOT_COMPLETED);
        registerReceiver(mBootBroadcastReceiver, intentFilter1);
        /*for AudioRoutes change*/
        IBinder b = ServiceManager.getService(Context.AUDIO_SERVICE);
        mAudioService = IAudioService.Stub.asInterface(b);
        try {
            mCurAudioRoutesInfo = mAudioService.startWatchingRoutes(mAudioRoutesObserver);
        } catch (RemoteException e) {
            e.printStackTrace();
        }
        chcekTvProviderInThread();
    }

    private boolean mIsInited = false;
    private boolean mIsUpdated = false;
    private int mCheckTvProviderTimeOut = 5 * 1000;//10s

    private boolean chcekTvProviderAvailable() {
        boolean result = false;
        ContentProviderClient tvProvider = this.getContentResolver().acquireContentProviderClient(TvContract.AUTHORITY);
        if (tvProvider != null) {
            result = true;
        }
        return result;
    }

    private void chcekTvProviderInThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "chcekTvProviderInThread start");
                while (true) {
                    if (chcekTvProviderAvailable()) {
                        Log.d(TAG, "chcekTvProviderInThread ok");
                        init();
                        break;
                    }
                    try {
                        Thread.sleep(10);
                    } catch (Exception e) {
                        Log.d(TAG, "chcekTvProviderInThread sleep error");
                        break;
                    }
                    mCheckTvProviderTimeOut -= 10;
                    if (mCheckTvProviderTimeOut < 0) {
                        Log.d(TAG, "chcekTvProviderInThread timeout");
                        break;
                    } else if (mCheckTvProviderTimeOut % 500 == 0) {//update per 500ms
                        Log.d(TAG, "chcekTvProviderInThread wait count = " + ((5 * 1000 - mCheckTvProviderTimeOut) / 10));
                    }
                }
                Log.d(TAG, "chcekTvProviderInThread end");
            }
        }).start();
    }

    private synchronized void init() {
        if (mIsInited) {
            Log.d(TAG, "init already");
            return;
        }
        Log.d(TAG, "init start");
        mSysSettingManager = new SysSettingManager(this);
        mDataMananer = new DataMananer(this);
        onChannelsChanged();
        Log.d(TAG, "init stage 1");
        updateRecorderNumber();
        Log.d(TAG, "init stage 2");
        mSystemControlManager = SystemControlManager.getInstance();
        mSystemControlManager.setDisplayModeListener(this);
        DtvkitGlueClient.getInstance().setSystemControlHandler(mSysControlHandler);
        DtvkitGlueClient.getInstance().registerSignalHandler(mRecordingManagerHandler);
        mParameterMananer = new ParameterMananer(this, DtvkitGlueClient.getInstance());
        checkDtvkitSatelliteUpdateStatusInThread();
        initInputThreadHandler();
        Log.d(TAG, "init end");
        mIsInited = true;
    }

    private void initInThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "initInThread start");
                init();
                Log.d(TAG, "initInThread end");
            }
        }).start();
    }

    private synchronized void updateRecorderNumber() {
        if (mIsUpdated) {
            Log.d(TAG, "updateRecorderNumber already");
            return;
        }
        Log.d(TAG, "updateRecorderNumber start");
        TvInputInfo.Builder builder = new TvInputInfo.Builder(getApplicationContext(), new ComponentName(getApplicationContext(), DtvkitTvInput.class));
        numRecorders = recordingGetNumRecorders();
        if (numRecorders > 0) {
            builder.setCanRecord(true)
                    .setTunerCount(numRecorders);
            mContentResolver.registerContentObserver(TvContract.RecordedPrograms.CONTENT_URI, true, mRecordingsContentObserver);
            onRecordingsChanged();
        } else {
            builder.setCanRecord(false)
                    .setTunerCount(1);
        }
        getApplicationContext().getSystemService(TvInputManager.class).updateTvInputInfo(builder.build());
    }

    private void updateRecorderNumberInThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                Log.d(TAG, "updateRecorderNumberInThread start");
                updateRecorderNumber();
                Log.d(TAG, "updateRecorderNumberInThread end");
            }
        }).start();
    }

    private void checkDtvkitSatelliteUpdateStatusInThread() {
        new Thread(new Runnable() {
            @Override
            public void run() {
                if (mDataMananer != null && mParameterMananer != null) {
                    if (mDataMananer.getIntParameters(DataMananer.DTVKIT_IMPORT_SATELLITE_FLAG) > 0) {
                        Log.d(TAG, "checkDtvkitSatelliteUpdateStatus has imported already");
                    } else {
                        if (mParameterMananer.importDatabase(ConstantManager.DTVKIT_SATELLITE_DATA)) {
                            mDataMananer.saveIntParameters(DataMananer.DTVKIT_IMPORT_SATELLITE_FLAG, 1);
                        }
                    }
                }
            }
        }).start();
    }

    //input work handler define
    protected static final int MSG_UPDATE_RECORDING_PROGRAM = 1;
    protected static final int MSG_UPDATE_RECORDING_PROGRAM_DELAY = 200;//200ms

    private void initInputThreadHandler() {
        mInputThreadHandler = new Handler(mHandlerThread.getLooper(), new Handler.Callback() {
            @Override
            public boolean handleMessage(Message msg) {
                Log.d(TAG, "mInputThreadHandler handleMessage " + msg.what + " start");
                switch (msg.what) {
                    case MSG_UPDATE_RECORDING_PROGRAM:{
                        syncRecordingProgramsWithDtvkit((JSONObject)msg.obj);
                        break;
                    }
                    default:
                        break;
                }
                Log.d(TAG, "mInputThreadHandler handleMessage " + msg.what + " over");
                return true;
            }
        });
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        Log.i(TAG, "onDestroy");
        unregisterReceiver(mParentalControlsBroadcastReceiver);
        unregisterReceiver(mBootBroadcastReceiver);
        mContentResolver.unregisterContentObserver(mContentObserver);
        mContentResolver.unregisterContentObserver(mRecordingsContentObserver);
        DtvkitGlueClient.getInstance().unregisterSignalHandler(mRecordingManagerHandler);
        DtvkitGlueClient.getInstance().disConnectDtvkitClient();
        DtvkitGlueClient.getInstance().setSystemControlHandler(null);
        mHandlerThread.getLooper().quitSafely();
        mHandlerThread = null;
        mInputThreadHandler.removeCallbacksAndMessages(null);
        mInputThreadHandler = null;
    }

    private boolean setTVAspectMode(int mode) {
        boolean used=false;
        try {
            JSONArray args = new JSONArray();
            args.put(mode);
            used = DtvkitGlueClient.getInstance().request("Dvb.setTVAspectMode", args).getBoolean("data");
            Log.i(TAG, "dvb setTVAspectMode, used:" + used);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        return used;
    }

    @Override
    public void onSetDisplayMode(int mode) {
        Log.i(TAG, "onSetDisplayMode " + mode);
        if (mode == DISPLAY_MODE_NORMAL)
            setTVAspectMode(ASPECT_MODE_AUTO);
        else
            setTVAspectMode(ASPECT_MODE_CUSTOM);
    }

    @Override
    public final Session onCreateSession(String inputId) {
        Log.i(TAG, "onCreateSession " + inputId);
        init();
        if (mSession != null) {
            mSession.releaseSignalHandler();
            mSession.finalReleaseWorkThread();
        }
        mSession = new DtvkitTvInputSession(this);
        mSystemControlManager.SetDtvKitSourceEnable(1);
        return mSession;
    }

    protected void setInputId(String name) {
        mInputId = name;
        Log.d(TAG, "set input id to " + mInputId);
    }

    private class DtvkitOverlayView extends FrameLayout {

        private NativeOverlayView nativeOverlayView;
        private CiMenuView ciOverlayView;

        private TextView mText;
        private RelativeLayout mRelativeLayout;
        private int w;
        private int h;

        private boolean mhegTookKey = false;

        public DtvkitOverlayView(Context context) {
            super(context);

            Log.i(TAG, "onCreateDtvkitOverlayView");
            ciOverlayView = new CiMenuView(getContext());
            nativeOverlayView = new NativeOverlayView(getContext());
            ciOverlayView = new CiMenuView(getContext());

            this.addView(nativeOverlayView);
            this.addView(ciOverlayView);
            initRelativeLayout();
        }

        public void destroy() {
            ciOverlayView.destroy();
            removeView(nativeOverlayView);
            removeView(ciOverlayView);
            removeView(mRelativeLayout);
        }

        public void hideOverLay() {
            if (nativeOverlayView != null) {
                nativeOverlayView.setVisibility(View.GONE);
            }
            if (ciOverlayView != null) {
                ciOverlayView.setVisibility(View.GONE);
            }
            if (mText != null) {
                mText.setVisibility(View.GONE);
            }
            if (mRelativeLayout != null) {
                mRelativeLayout.setVisibility(View.GONE);
            }
            setVisibility(View.GONE);
        }

        public void showOverLay() {
            if (nativeOverlayView != null) {
                nativeOverlayView.setVisibility(View.VISIBLE);
            }
            if (ciOverlayView != null) {
                ciOverlayView.setVisibility(View.VISIBLE);
            }
            if (mText != null) {
                mText.setVisibility(View.VISIBLE);
            }
            if (mRelativeLayout != null) {
                mRelativeLayout.setVisibility(View.VISIBLE);
            }
            setVisibility(View.VISIBLE);
        }

        private void initRelativeLayout() {
            if (mText == null && mRelativeLayout == null) {
                Log.d(TAG, "initRelativeLayout");
                mRelativeLayout = new RelativeLayout(getContext());
                mText = new TextView(getContext());
                ViewGroup.LayoutParams linearLayoutParams = new ViewGroup.LayoutParams(LinearLayout.LayoutParams.FILL_PARENT, LinearLayout.LayoutParams.FILL_PARENT);
                mRelativeLayout.setLayoutParams(linearLayoutParams);
                mRelativeLayout.setBackgroundColor(Color.TRANSPARENT);

                //add scrambled text
                RelativeLayout.LayoutParams textLayoutParams = new RelativeLayout.LayoutParams(LinearLayout.LayoutParams.WRAP_CONTENT, LinearLayout.LayoutParams.WRAP_CONTENT);
                textLayoutParams.addRule(RelativeLayout.CENTER_IN_PARENT, RelativeLayout.TRUE);
                mText.setTextSize(TypedValue.COMPLEX_UNIT_SP, 30);
                mText.setGravity(Gravity.CENTER);
                mText.setTypeface(Typeface.DEFAULT_BOLD);
                mText.setTextColor(Color.WHITE);
                //mText.setText("RelativeLayout");
                mText.setVisibility(View.GONE);
                mRelativeLayout.addView(mText, textLayoutParams);
                this.addView(mRelativeLayout);
            } else {
                Log.d(TAG, "initRelativeLayout already");
            }
        }

        public void setTeletextMix(boolean status){
            RectF srcRect, dstRect;
            float scalex, scaley, px, py;
        if (status) {
            srcRect = new RectF(0, 0, 1920, 1080);
            dstRect = new RectF(960, 0, 1920, 1080);
            scalex = 0.5f;
            scaley = 1.0f;
            px = 1920;
            py = 1080;
        } else {
            srcRect = new RectF(0, 0, 1920, 1080);
            dstRect = new RectF(0, 0, 1920, 1080);
            scalex = 1.0f;
            scaley = 1.0f;
            px = 0;
            py = 0;
        }
            Matrix matrix = new Matrix();

            matrix.setRectToRect(srcRect, dstRect, Matrix.ScaleToFit.CENTER);
            matrix.setScale(scalex, scaley, px, py);

            //nativeOverlayView.setTransform(matrix);
        }

        public void showScrambledText(String text) {
            if (mText != null && mRelativeLayout != null) {
                Log.d(TAG, "showText");
                mText.setText(text);
                if (mText.getVisibility() != View.VISIBLE) {
                    mText.setVisibility(View.VISIBLE);
                }
            }
        }

        public void hideScrambledText() {
            if (mText != null && mRelativeLayout != null) {
                Log.d(TAG, "hideText");
                mText.setText("");
                if (mText.getVisibility() != View.GONE) {
                    mText.setVisibility(View.GONE);
                }
            }
        }

        public void setSize(int width, int height) {
            w = width;
            h = height;
            nativeOverlayView.setSize(width, height);
        }

        public boolean handleKeyDown(int keyCode, KeyEvent event) {
            boolean result;
            if (ciOverlayView.handleKeyDown(keyCode, event)) {
                mhegTookKey = false;
                result = true;
            }
            else if (mhegKeypress(keyCode)){
                mhegTookKey = true;
                result = true;
            }
            else {
                mhegTookKey = false;
                result = false;
            }
            return result;
        }

        public boolean handleKeyUp(int keyCode, KeyEvent event) {
            boolean result;

            if (ciOverlayView.handleKeyUp(keyCode, event) || mhegTookKey) {
                result = true;
            } else {
                result = false;
            }
            mhegTookKey = false;

            return result;
        }

        private boolean mhegKeypress(int keyCode) {
            boolean used=false;
            try {
                JSONArray args = new JSONArray();
                args.put(keyCode);
                used = DtvkitGlueClient.getInstance().request("Mheg.notifyKeypress", args).getBoolean("data");
                Log.i(TAG, "Mheg keypress, used:" + used);
            } catch (Exception e) {
                Log.e(TAG, e.getMessage());
            }
            return used;
        }
    }

    private boolean checkDtvkitRecordingsExists(String uri, JSONArray recordings) {
        boolean exists = false;
        if (recordings != null) {
            for (int i = 0; i < recordings.length(); i++) {
                try {
                    String u = recordings.getJSONObject(i).getString("uri");
                    if (uri.equals(u)) {
                        exists = true;
                        break;
                    }
                } catch (JSONException e) {
                    Log.e("TAG", "checkDtvkitRecordingsExists JSONException " + e.getMessage());
                }
            }
        }
        return exists;
    }

    private final DtvkitGlueClient.SignalHandler mRecordingManagerHandler = new DtvkitGlueClient.SignalHandler() {
        @RequiresApi(api = Build.VERSION_CODES.M)
        @Override
        public void onSignal(String signal, JSONObject data) {
            if (signal.equals("RecordingsChanged")) {
                Log.i(TAG, "mRecordingManagerHandler onSignal: " + signal + " : " + data.toString());
                if (mInputThreadHandler != null) {
                    mInputThreadHandler.removeMessages(MSG_UPDATE_RECORDING_PROGRAM);
                    Message mess = mInputThreadHandler.obtainMessage(MSG_UPDATE_RECORDING_PROGRAM, 0, 0, data);
                    boolean info = mInputThreadHandler.sendMessageDelayed(mess, MSG_UPDATE_RECORDING_PROGRAM_DELAY);
                    Log.d(TAG, "sendMessage MSG_UPDATE_RECORDING_PROGRAM " + info);
                }
            }
        }
    };

    private void syncRecordingProgramsWithDtvkit(JSONObject data) {
        ArrayList<RecordedProgram> recordingsInDB = new ArrayList();
        ArrayList<RecordedProgram> recordingsResetInvalidInDB = new ArrayList();
        ArrayList<RecordedProgram> recordingsResetValidInDB = new ArrayList();

        Cursor cursor = mContentResolver.query(TvContract.RecordedPrograms.CONTENT_URI, RecordedProgram.PROJECTION, null, null, TvContract.RecordedPrograms._ID + " DESC");
        if (cursor != null) {
            while (cursor.moveToNext()) {
                recordingsInDB.add(RecordedProgram.fromCursor(cursor));
            }
        }
        JSONArray recordings = recordingGetListOfRecordings();

        Log.d(TAG, "recordings: db[" + recordingsInDB.size() + "] dtvkit[" + recordings.length() + "]");
        for (RecordedProgram rec : recordingsInDB) {
            Log.d(TAG, "db: " + rec.getRecordingDataUri());
        }
        for (int i = 0; i < recordings.length(); i++) {
            try {
                Log.d(TAG, "dtvkit: " + recordings.getJSONObject(i).getString("uri"));
            } catch (JSONException e) {
                Log.d(TAG, e.getMessage());
            }
        }

        if (recordingsInDB.size() != 0) {
            for (RecordedProgram recording : recordingsInDB) {
                String uri = recording.getRecordingDataUri();
                if (checkDtvkitRecordingsExists(uri, recordings)) {
                    if (recording.getRecordingDataBytes() <= 0) {
                        Log.d(TAG, "make recording valid: "+uri);
                        recordingsResetValidInDB.add(recording);
                    }
                } else {
                    if (recording.getRecordingDataBytes() > 0) {
                        Log.d(TAG, "make recording invalid: "+uri);
                        recordingsResetInvalidInDB.add(recording);
                    }
                }
            }

            ArrayList<ContentProviderOperation> ops = new ArrayList();
            for (RecordedProgram recording : recordingsResetInvalidInDB) {
                Uri uri = TvContract.buildRecordedProgramUri(recording.getId());
                String dataUri = recording.getRecordingDataUri();
                ContentValues update = new ContentValues();
                update.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_BYTES, 0l);
                ops.add(ContentProviderOperation.newUpdate(uri)
                    .withValues(update)
                    .build());
            }
            for (RecordedProgram recording : recordingsResetValidInDB) {
                Uri uri = TvContract.buildRecordedProgramUri(recording.getId());
                String dataUri = recording.getRecordingDataUri();
                ContentValues update = new ContentValues();
                update.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_BYTES, 1024 * 1024l);
                ops.add(ContentProviderOperation.newUpdate(uri)
                    .withValues(update)
                    .build());
            }
            try {
                mContentResolver.applyBatch(TvContract.AUTHORITY, ops);
            } catch (Exception e) {
                Log.e(TAG, "recordings DB update failed.");
            }
        }
    }

    class NativeOverlayView extends View
    {
        Bitmap overlay1 = null;
        Bitmap overlay2 = null;
        Bitmap overlay_update = null;
        Bitmap overlay_draw = null;
        Bitmap region = null;
        int region_width = 0;
        int region_height = 0;
        int left = 0;
        int top = 0;
        int width = 0;
        int height = 0;
        Rect src, dst;

        Semaphore sem = new Semaphore(1);

        private final DtvkitGlueClient.OverlayTarget mTarget = new DtvkitGlueClient.OverlayTarget() {
            @Override
            public void draw(int src_width, int src_height, int dst_x, int dst_y, int dst_width, int dst_height, byte[] data) {
                if (overlay1 == null) {
                    /* TODO The overlay size should come from the tif (and be updated on onOverlayViewSizeChanged) */
                    /* Create 2 layers for double buffering */
                    overlay1 = Bitmap.createBitmap(1920, 1080, Bitmap.Config.ARGB_8888);
                    overlay2 = Bitmap.createBitmap(1920, 1080, Bitmap.Config.ARGB_8888);

                    overlay_draw = overlay1;
                    overlay_update = overlay2;

                    /* Clear the overlay that will be drawn initially */
                    Canvas canvas = new Canvas(overlay_draw);
                    canvas.drawColor(0, PorterDuff.Mode.CLEAR);
                }

                /* TODO Temporary private usage of API. Add explicit methods if keeping this mechanism */
                if (src_width == 0 || src_height == 0) {
                    if (dst_width == 9999) {
                        /* 9999 dst_width indicates the overlay should be cleared */
                        Canvas canvas = new Canvas(overlay_update);
                        canvas.drawColor(0, PorterDuff.Mode.CLEAR);
                    }
                    else if (dst_height == 9999) {
                        /* 9999 dst_height indicates the drawn regions should be displayed on the overlay */
                        /* The update layer is now ready to be displayed so switch the overlays
                         * and use the other one for the next update */
                        sem.acquireUninterruptibly();
                        Bitmap temp = overlay_draw;
                        overlay_draw = overlay_update;
                        src = new Rect(0, 0, overlay_draw.getWidth(), overlay_draw.getHeight());
                        overlay_update = temp;
                        sem.release();
                        postInvalidate();
                        return;
                    }
                    else {
                        /* 0 dst_width and 0 dst_height indicates to add the region to overlay */
                        if (region != null) {
                            Canvas canvas = new Canvas(overlay_update);
                            Rect src = new Rect(0, 0, region_width, region_height);
                            Rect dst = new Rect(left, top, left + width, top + height);
                            Paint paint = new Paint();
                            paint.setAntiAlias(true);
                            paint.setFilterBitmap(true);
                            paint.setDither(true);
                            canvas.drawBitmap(region, src, dst, paint);
                            region = null;
                        }
                    }
                    return;
                }

                int part_bottom = 0;
                if (region == null) {
                    /* TODO Create temporary region buffer using region_width and overlay height */
                    region_width = src_width;
                    region_height = src_height;
                    region = Bitmap.createBitmap(1920, 1080, Bitmap.Config.ARGB_8888);
                    left = dst_x;
                    top = dst_y;
                    width = dst_width;
                    height = dst_height;
                }
                else {
                    part_bottom = region_height;
                    region_height += src_height;
                }

                /* Build an array of ARGB_8888 pixels as signed ints and add this part to the region */
                int[] colors = new int[src_width * src_height];
                for (int i = 0, j = 0; i < src_width * src_height; i++, j += 4) {
                   colors[i] = (((int)data[j]&0xFF) << 24) | (((int)data[j+1]&0xFF) << 16) |
                      (((int)data[j+2]&0xFF) << 8) | ((int)data[j+3]&0xFF);
                }
                Bitmap part = Bitmap.createBitmap(colors, 0, src_width, src_width, src_height, Bitmap.Config.ARGB_8888);
                Canvas canvas = new Canvas(region);
                canvas.drawBitmap(part, 0, part_bottom, null);
            }
        };

        public NativeOverlayView(Context context) {
            super(context);
            DtvkitGlueClient.getInstance().setOverlayTarget(mTarget);
        }

        public void setSize(int width, int height) {
            dst = new Rect(0, 0, width, height);
        }

        @Override
        protected void onDraw(Canvas canvas)
        {
            super.onDraw(canvas);
            sem.acquireUninterruptibly();
            if (overlay_draw != null) {
                canvas.drawBitmap(overlay_draw, src, dst, null);
            }
            sem.release();
        }
    }

    // We do not indicate recording capabilities. TODO for recording.
    @RequiresApi(api = Build.VERSION_CODES.N)
    public TvInputService.RecordingSession onCreateRecordingSession(String inputId)
    {
        Log.i(TAG, "onCreateRecordingSession");
        init();
        mDtvkitRecordingSessionCount++;
        return new DtvkitRecordingSession(this, inputId);
    }

    @RequiresApi(api = Build.VERSION_CODES.N)
    class DtvkitRecordingSession extends TvInputService.RecordingSession {
        private static final String TAG = "DtvkitRecordingSession";
        private Uri mChannel = null;
        private Uri mProgram = null;
        private Context mContext = null;
        private String mInputId = null;
        private long startRecordTimeMillis = 0;
        private long endRecordTimeMillis = 0;
        private long startRecordSystemTimeMillis = 0;
        private long endRecordSystemTimeMillis = 0;
        private String recordingUri = null;
        private Uri mRecordedProgramUri = null;
        private boolean tunedNotified = false;
        private boolean mTuned = false;
        private boolean mStarted = false;
        private int mPath = -1;
        private Handler mRecordingProcessHandler = null;
        private long mCurrentRecordIndex = 0;
        private boolean mRecordStopAndSaveReceived = false;

        protected static final int MSG_RECORD_ONTUNE = 0;
        protected static final int MSG_RECORD_ONSTART = 1;
        protected static final int MSG_RECORD_ONSTOP = 2;
        protected static final int MSG_RECORD_UPDATE_RECORDING = 3;
        protected static final int MSG_RECORD_ONRELEASE = 4;
        protected static final int MSG_RECORD_DO_FINALRELEASE = 5;
        private final String[] MSG_STRING = {"RECORD_ONTUNE", "RECORD_ONSTART", "RECORD_ONSTOP", "RECORD_UPDATE_RECORDING", "RECORD_ONRELEASE", "DO_FINALRELEASE"};

        protected static final int MSG_RECORD_UPDATE_RECORD_PERIOD = 3000;

        @RequiresApi(api = Build.VERSION_CODES.N)
        public DtvkitRecordingSession(Context context, String inputId) {
            super(context);
            mContext = context;
            mInputId = inputId;
            mCurrentRecordIndex = mDtvkitRecordingSessionCount;
            if (numRecorders == 0) {
                updateRecorderNumber();
            }
            initRecordWorkThread();
            Log.i(TAG, "DtvkitRecordingSession");
        }

        protected void initRecordWorkThread() {
            mRecordingProcessHandler = new Handler(mHandlerThread.getLooper(), new Handler.Callback() {
                @Override
                public boolean handleMessage(Message msg) {
                    if (msg.what >= 0 && MSG_STRING.length > msg.what) {
                        Log.d(TAG, "handleMessage " + MSG_STRING[msg.what] + " start, index = " + mCurrentRecordIndex);
                    } else {
                        Log.d(TAG, "handleMessage " + msg.what + " start, index = " + mCurrentRecordIndex);
                    }
                    switch (msg.what) {
                        case MSG_RECORD_ONTUNE:{
                            Uri uri = (Uri)msg.obj;
                            doTune(uri);
                            break;
                        }
                        case MSG_RECORD_ONSTART:{
                            Uri uri = (Uri)msg.obj;
                            doStartRecording(uri);
                            break;
                        }
                        case MSG_RECORD_ONSTOP:{
                            doStopRecording();
                            break;
                        }
                        case MSG_RECORD_UPDATE_RECORDING:{
                            boolean insert = (msg.arg1 == 1);
                            boolean check = (msg.arg2 == 1);
                            updateRecordingToDb(insert, check);
                            break;
                        }
                        case MSG_RECORD_ONRELEASE:{
                            doRelease();
                            break;
                        }
                        case MSG_RECORD_DO_FINALRELEASE:{
                            doFinalReleaseByThread();
                            break;
                        }
                        default:
                            break;
                    }
                    if (msg.what >= 0 && MSG_STRING.length > msg.what) {
                        Log.d(TAG, "handleMessage " + MSG_STRING[msg.what] + " over , index = " + mCurrentRecordIndex);
                    } else {
                        Log.d(TAG, "handleMessage " + msg.what + " over , index = " + mCurrentRecordIndex);
                    }
                    return true;
                }
            });
        }

        @Override
        public void onTune(Uri uri) {
            if (mRecordingProcessHandler != null) {
                mRecordingProcessHandler.removeMessages(MSG_RECORD_ONTUNE);
                Message mess = mRecordingProcessHandler.obtainMessage(MSG_RECORD_ONTUNE, 0, 0, uri);
                boolean result = mRecordingProcessHandler.sendMessage(mess);
                Log.d(TAG, "onTune sendMessage result " + result + ", index = " + mCurrentRecordIndex);
            } else {
                Log.i(TAG, "onTune null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
            }
        }

        @Override
        public void onStartRecording(@Nullable Uri uri) {
            if (mRecordingProcessHandler != null) {
                mRecordingProcessHandler.removeMessages(MSG_RECORD_ONSTART);
                Message mess = mRecordingProcessHandler.obtainMessage(MSG_RECORD_ONSTART, 0, 0, uri);
                boolean result = mRecordingProcessHandler.sendMessage(mess);
                Log.d(TAG, "onStartRecording sendMessage result " + result + ", index = " + mCurrentRecordIndex);
            } else {
                Log.i(TAG, "onStartRecording null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
            }
        }

        @Override
        public void onStopRecording() {
            if (mRecordingProcessHandler != null) {
                mRecordingProcessHandler.removeMessages(MSG_RECORD_ONSTOP);
                boolean result = mRecordingProcessHandler.sendEmptyMessage(MSG_RECORD_ONSTOP);
                Log.d(TAG, "onStopRecording sendMessage result " + result + ", index = " + mCurrentRecordIndex);
            } else {
                Log.i(TAG, "onStopRecording null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
            }
        }

        @Override
        public void onRelease() {
            if (mRecordingProcessHandler != null) {
                mRecordingProcessHandler.removeMessages(MSG_RECORD_ONRELEASE);
                boolean result = mRecordingProcessHandler.sendEmptyMessage(MSG_RECORD_ONRELEASE);
                Log.d(TAG, "onRelease sendMessage result " + result + ", index = " + mCurrentRecordIndex);
            } else {
                Log.i(TAG, "onRelease null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
            }
        }

        public void doTune(Uri uri) {
            Log.i(TAG, "doTune for recording " + uri);
            if (ContentUris.parseId(uri) == -1) {
                Log.e(TAG, "DtvkitRecordingSession doTune invalid uri = " + uri);
                notifyError(TvInputManager.RECORDING_ERROR_UNKNOWN);
                return;
            }

            tunedNotified = false;
            mTuned = false;
            mStarted = false;

            removeScheduleTimeshiftRecordingTask();
            numActiveRecordings = recordingGetNumActiveRecordings();
            Log.i(TAG, "numActiveRecordings: " + numActiveRecordings);

            /*want of resource*/
            if (numActiveRecordings >= numRecorders
                || numActiveRecordings >= getNumRecordersLimit()) {
                if (getFeatureSupportTimeshifting()
                        && timeshiftRecorderState != RecorderState.STOPPED
                        && getFeatureTimeshiftingPriorityHigh()) {
                    Log.i(TAG, "No recording path available, no recorder");
                    Bundle event = new Bundle();
                    event.putString(ConstantManager.KEY_INFO, "No recording path available, no recorder");
                    notifySessionEvent(ConstantManager.EVENT_RESOURCE_BUSY, event);
                    notifyError(TvInputManager.RECORDING_ERROR_RESOURCE_BUSY);
                    return;
                } else {
                    recordingPending = true;

                    boolean returnToLive = timeshifting;
                    Log.i(TAG, "stopping timeshift [return live:"+returnToLive+"]");
                    timeshiftRecorderState = RecorderState.STOPPED;
                    scheduleTimeshiftRecording = false;
                    timeshifting = false;
                    playerStopTimeshiftRecording(returnToLive);
                }
            }

            boolean available = false;

            mChannel = uri;
            Channel channel = getChannel(uri);
            if (recordingCheckAvailability(getChannelInternalDvbUri(channel))) {
                Log.i(TAG, "recording path available");
                StringBuffer tuneResponse = new StringBuffer();

                DtvkitGlueClient.getInstance().registerSignalHandler(mRecordingHandler);

                JSONObject tuneStat = recordingTune(getChannelInternalDvbUri(channel), tuneResponse);
                if (tuneStat != null) {
                    mTuned = getRecordingTuned(tuneStat);
                    mPath = getRecordingTunePath(tuneStat);
                    if (mTuned) {
                        Log.i(TAG, "recording tuned ok.");
                        notifyTuned(uri);
                        tunedNotified = true;
                    } else {
                        Log.i(TAG, "recording (path:" + mPath + ") tunning...");
                    }
                    available = true;
                } else {
                    if (tuneResponse.toString().equals("Failed to get a tuner to record")) {
                        Log.i(TAG, "record error no tuner to record");
                    }
                    else if (tuneResponse.toString().equals("Invalid resource")) {
                        Log.i(TAG, "record error invalid channel");
                    }
                }
            }

            if (!available) {
                Log.i(TAG, "No recording path available, no tuner/demux");
                Bundle event = new Bundle();
                event.putString(ConstantManager.KEY_INFO, "No recording path available, no tuner/demux");
                notifySessionEvent(ConstantManager.EVENT_RESOURCE_BUSY, event);
                notifyError(TvInputManager.RECORDING_ERROR_RESOURCE_BUSY);
            }
        }

        private void doStartRecording(@Nullable Uri uri) {
            Log.i(TAG, "doStartRecording " + uri);
            mProgram = uri;

            String dvbUri;
            long durationSecs = 0;
            Program program = getProgram(uri);
            if (program != null) {
                startRecordTimeMillis = program.getStartTimeUtcMillis();
                startRecordSystemTimeMillis = System.currentTimeMillis();
                dvbUri = getProgramInternalDvbUri(program);
            } else {
                startRecordTimeMillis = PropSettingManager.getCurrentStreamTime(true);
                startRecordSystemTimeMillis = System.currentTimeMillis();
                dvbUri = getChannelInternalDvbUri(getChannel(mChannel));
                durationSecs = 3 * 60 * 60; // 3 hours is maximum recording duration for Android
            }
            StringBuffer recordingResponse = new StringBuffer();
            Log.i(TAG, "startRecording path:" + mPath);
            if (!recordingStartRecording(dvbUri, mPath, recordingResponse)) {
                if (recordingResponse.toString().equals("May not be enough space on disk")) {
                    Log.i(TAG, "record error insufficient space");
                    notifyError(TvInputManager.RECORDING_ERROR_INSUFFICIENT_SPACE);
                }
                else if (recordingResponse.toString().equals("Limited by minimum free disk space")) {
                    Log.i(TAG, "record error min free limited");
                    notifyError(TvInputManager.RECORDING_ERROR_INSUFFICIENT_SPACE);
                }
                else {
                    Log.i(TAG, "record error unknown");
                    notifyError(TvInputManager.RECORDING_ERROR_UNKNOWN);
                }
            }
            else
            {
                mStarted = true;
                recordingUri = recordingResponse.toString();
                Log.i(TAG, "Recording started:"+recordingUri);
                updateRecordProgramInfo(recordingUri);
            }
            recordingPending = false;
        }

        private void doStopRecording() {
            Log.i(TAG, "doStopRecording");

            DtvkitGlueClient.getInstance().unregisterSignalHandler(mRecordingHandler);
            if (!recordingStopRecording(recordingUri)) {
                notifyError(TvInputManager.RECORDING_ERROR_UNKNOWN);
            } else {
                if (mRecordingProcessHandler != null) {
                    mRecordingProcessHandler.removeMessages(MSG_RECORD_UPDATE_RECORDING);
                    boolean result = mRecordingProcessHandler.sendMessage(mRecordingProcessHandler.obtainMessage(MSG_RECORD_UPDATE_RECORDING, 0, 0));
                    Log.d(TAG, "doStopRecording sendMessage MSG_RECORD_UPDATE_RECORDING = " + result + ", index = " + mCurrentRecordIndex);
                    if (result) {
                        mRecordStopAndSaveReceived = true;
                    }
                } else {
                    Log.i(TAG, "doStopRecording sendMessage MSG_RECORD_UPDATE_RECORDING null, index = " + mCurrentRecordIndex);
                }
            }
            mStarted = false;
            recordingPending = false;

            /*if there's a live play,
              should lock here, may run into a race condition*/
            if ((mSession != null && mSession.mTunedChannel != null &&
                     mSession.mHandlerThreadHandle != null)) {
               mSession.sendMsgTryStartTimeshift();
            }
        }

        private void updateRecordingToDb(boolean insert, boolean check) {
            endRecordTimeMillis = PropSettingManager.getCurrentStreamTime(true);
            endRecordSystemTimeMillis = System.currentTimeMillis();
            scheduleTimeshiftRecording = true;
            Log.d(TAG, "updateRecordingToDb:"+recordingUri);
            if (recordingUri != null)
            {
                long recordingDurationMillis = endRecordSystemTimeMillis - startRecordSystemTimeMillis;
                RecordedProgram.Builder builder = null;
                InternalProviderData data = null;
                Program program = getProgram(mProgram);
                Channel channel = getChannel(mChannel);
                if (program == null) {
                    program = getCurrentStreamProgram(mChannel, PropSettingManager.getCurrentStreamTime(true));
                }
                if (insert) {
                    if (program == null) {
                        long id = -1;
                        if (channel != null) {
                            id = channel.getId();
                        }
                        builder = new RecordedProgram.Builder()
                                .setChannelId(id)
                                .setTitle(channel != null ? channel.getDisplayName() : null)
                                .setStartTimeUtcMillis(startRecordTimeMillis)
                                .setEndTimeUtcMillis(startRecordTimeMillis + recordingDurationMillis/*endRecordTimeMillis*/);//stream card may playback
                    } else {
                        builder = new RecordedProgram.Builder(program);
                    }
                    data = new InternalProviderData();
                    String currentPath = mDataMananer.getStringParameters(DataMananer.KEY_PVR_RECORD_PATH);
                    int pathExist = 0;
                    if (SysSettingManager.isDeviceExist(currentPath)) {
                        pathExist = 1;
                    }
                    if (SysSettingManager.isMediaPath(currentPath)) {
                        currentPath = SysSettingManager.convertMediaPathToMountedPath(currentPath);
                    }
                    try {
                        data.put(RecordedProgram.RECORD_FILE_PATH, currentPath);
                        data.put(RecordedProgram.RECORD_STORAGE_EXIST, pathExist);
                    } catch (Exception e) {
                        Log.e(TAG, "updateRecordingToDb update InternalProviderData Exception = " + e.getMessage());
                    }
                }
                if (insert) {
                    Log.i(TAG, "updateRecordingToDb insert");
                    RecordedProgram recording = builder.setInputId(mInputId)
                            .setRecordingDataUri(recordingUri)
                            .setRecordingDataBytes(1024 * 1024l)
                            .setRecordingDurationMillis(recordingDurationMillis > 0 ? recordingDurationMillis : -1)
                            .setInternalProviderData(data)
                            .build();
                    mRecordedProgramUri = mContext.getContentResolver().insert(TvContract.RecordedPrograms.CONTENT_URI,
                        recording.toContentValues());
                    Bundle event = new Bundle();
                    event.putString(ConstantManager.EVENT_RECORD_DATA_URI, recordingUri != null ? recordingUri : null);
                    event.putString(ConstantManager.EVENT_RECORD_PROGRAM_URI, mRecordedProgramUri != null ? mRecordedProgramUri.toString() : null);
                    notifySessionEvent(ConstantManager.EVENT_RECORD_PROGRAM_URI, event);
                } else {
                    Log.i(TAG, "updateRecordingToDb update");
                    if (mRecordedProgramUri != null) {
                        ContentValues values = new ContentValues();
                        if (endRecordTimeMillis > -1) {
                            values.put(TvContract.RecordedPrograms.COLUMN_END_TIME_UTC_MILLIS, startRecordTimeMillis + recordingDurationMillis/*endRecordTimeMillis*/);//stream card may playback
                        }
                        if (recordingDurationMillis > 0) {
                            values.put(TvContract.RecordedPrograms.COLUMN_RECORDING_DURATION_MILLIS, recordingDurationMillis);
                        }
                        mContext.getContentResolver().update(mRecordedProgramUri,
                                values, null, null);
                    } else {
                        Log.i(TAG, "updateRecordingToDb update mRecordedProgramUri null");
                    }
                }
                //recordingUri = null;
                if (!check) {
                    Log.i(TAG, "updateRecordingToDb notifystop mRecordedProgramUri = " + mRecordedProgramUri + " index = " + mCurrentRecordIndex);
                    notifyRecordingStopped(mRecordedProgramUri);
                    if (mRecordStopAndSaveReceived || mStarted) {
                        if (mRecordingProcessHandler != null) {
                            mRecordingProcessHandler.removeMessages(MSG_RECORD_UPDATE_RECORDING);
                            boolean result = mRecordingProcessHandler.sendEmptyMessage(MSG_RECORD_DO_FINALRELEASE);
                            Log.d(TAG, "updateRecordingToDb sendMessage result = " + result + ", index = " + mCurrentRecordIndex);
                        } else {
                            Log.i(TAG, "updateRecordingToDb sendMessage null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
                        }
                    }
                }
                if (mRecordingProcessHandler != null) {
                    if (check) {
                        boolean result = mRecordingProcessHandler.sendMessageDelayed(mRecordingProcessHandler.obtainMessage(MSG_RECORD_UPDATE_RECORDING, 0, 1), MSG_RECORD_UPDATE_RECORD_PERIOD);
                        Log.d(TAG, "updateRecordingToDb continue sendMessage MSG_RECORD_UPDATE_RECORDING = " + result + ", index = " + mCurrentRecordIndex);
                    }
                } else {
                    Log.i(TAG, "updateRecordingToDb sendMessage MSG_RECORD_UPDATE_RECORDING null, index = " + mCurrentRecordIndex);
                }
            }
            //PropSettingManager.resetRecordFrequencyFlag();
        }

        private void doRelease() {
            Log.i(TAG, "doRelease");

            recordingPending = false;

            String uri = "";
            if (mProgram != null) {
                uri = getProgramInternalDvbUri(getProgram(mProgram));
            } else if (mChannel != null) {
                uri = getChannelInternalDvbUri(getChannel(mChannel)) + ";0000";
            } else {
                return;
            }

            DtvkitGlueClient.getInstance().unregisterSignalHandler(mRecordingHandler);
            if (mStarted && !mRecordStopAndSaveReceived) {
                if (mRecordingProcessHandler != null) {
                    mRecordingProcessHandler.removeMessages(MSG_RECORD_UPDATE_RECORDING);
                    mRecordingProcessHandler.removeMessages(MSG_RECORD_DO_FINALRELEASE);
                    boolean result1 = mRecordingProcessHandler.sendMessage(mRecordingProcessHandler.obtainMessage(MSG_RECORD_UPDATE_RECORDING, 0, 0));
                    boolean result2 = mRecordingProcessHandler.sendEmptyMessage(MSG_RECORD_DO_FINALRELEASE);
                    Log.d(TAG, "doRelease sendMessage result1 = " + result1 + ", result2 = " + result1 + ", index = " + mCurrentRecordIndex);
                } else {
                    Log.i(TAG, "doRelease sendMessage null mRecordingProcessHandler" + ", index = " + mCurrentRecordIndex);
                }
            }
            if (!mStarted && mTuned) {
                recordingUntune(mPath);
                return;
            }

            JSONArray scheduledRecordings = recordingGetListOfScheduledRecordings();
            if (scheduledRecordings != null) {
                for (int i = 0; i < scheduledRecordings.length(); i++) {
                    try {
                        if (getScheduledRecordingUri(scheduledRecordings.getJSONObject(i)).equals(uri)) {
                            Log.i(TAG, "removing recording uri from schedule: " + uri);
                            recordingRemoveScheduledRecording(uri);
                            break;
                        }
                    } catch (JSONException e) {
                        Log.e(TAG, e.getMessage());
                    }
                }
            }
        }

        private void doFinalReleaseByThread() {
            new Thread(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "doFinalReleaseByThread start index = " + mCurrentRecordIndex);
                    if (mRecordingProcessHandler != null) {
                        mRecordingProcessHandler.removeCallbacksAndMessages(null);
                    }
                    mRecordingProcessHandler = null;
                    Log.d(TAG, "doFinalReleaseByThread end  index = " + mCurrentRecordIndex);
                }
            }).start();
        }

        private Program getProgram(Uri uri) {
            Program program = null;
            if (uri != null) {
                Cursor cursor = mContext.getContentResolver().query(uri, Program.PROJECTION, null, null, null);
                if (cursor != null) {
                    while (cursor.moveToNext()) {
                        program = Program.fromCursor(cursor);
                    }
                }
            }
            return program;
        }

        private Channel getChannel(Uri uri) {
            Channel channel = null;
            if (uri != null) {
                Cursor cursor = mContext.getContentResolver().query(uri, Channel.PROJECTION, null, null, null);
                if (cursor != null) {
                    while (cursor.moveToNext()) {
                        channel = Channel.fromCursor(cursor);
                    }
                }
            }
            return channel;
        }

        private void updateRecordProgramInfo(String recordDataUri) {
            RecordedProgram searchedRecordedProgram = getRecordProgram(recordDataUri);
            if (mRecordingProcessHandler != null) {
                mRecordingProcessHandler.removeMessages(MSG_RECORD_UPDATE_RECORDING);
                int arg1 = 0;
                int arg2 = 0;
                if (searchedRecordedProgram != null) {
                    Log.d(TAG, "updateRecordProgramInfo update " + recordDataUri);
                    arg1 = 0;
                    arg2 = 1;
                } else {
                    Log.d(TAG, "updateRecordProgramInfo insert " + recordDataUri);
                    arg1 = 1;
                    arg2 = 1;
                }
                boolean result = mRecordingProcessHandler.sendMessage(mRecordingProcessHandler.obtainMessage(MSG_RECORD_UPDATE_RECORDING, arg1, arg2));
                Log.d(TAG, "updateRecordProgramInfo sendMessage MSG_RECORD_UPDATE_RECORDING = " + result + ", index = " + mCurrentRecordIndex);
            } else {
                Log.i(TAG, "updateRecordProgramInfo sendMessage MSG_RECORD_UPDATE_RECORDING null, index = " + mCurrentRecordIndex);
            }
        }

        private RecordedProgram getRecordProgram(String recordDataUri) {
            RecordedProgram result = null;
            if (recordDataUri != null) {
                Cursor cursor = mContext.getContentResolver().query(TvContract.RecordedPrograms.CONTENT_URI, RecordedProgram.PROJECTION,
                        TvContract.RecordedPrograms.COLUMN_RECORDING_DATA_URI + "=?", new String[]{recordDataUri}, null);
                if (cursor != null) {
                    while (cursor.moveToNext()) {
                        result = RecordedProgram.fromCursor(cursor);
                        break;
                    }
                }
            }
            return result;
        }

        private Program getCurrentProgram(Uri channelUri) {
            return TvContractUtils.getCurrentProgram(mContext.getContentResolver(), channelUri);
        }

        private Program getCurrentStreamProgram(Uri channelUri, long streamTime) {
            return TvContractUtils.getCurrentProgramExt(mContext.getContentResolver(), channelUri, streamTime);
        }

        private final DtvkitGlueClient.SignalHandler mRecordingHandler = new DtvkitGlueClient.SignalHandler() {
            @RequiresApi(api = Build.VERSION_CODES.M)
            @Override
            public void onSignal(String signal, JSONObject data) {
                Log.i(TAG, "recording onSignal: " + signal + " : " + data.toString());
                if (signal.equals("TuneStatusChanged")) {
                    String state = "fail";
                    int path = -1;
                    try {
                        state = data.getString("state");
                        path = data.getInt("path");
                    } catch (JSONException ignore) {
                    }
                    Log.i(TAG, "tune to: "+ path + ", " + state);
                    if (path != mPath)
                        return;

                    switch (state) {
                        case "ok":
                            if (!tunedNotified)
                                notifyTuned(mChannel);
                        break;
                    }
                } else if (signal.equals("RecordingStatusChanged")) {
                    if (!recordingIsRecordingPathActive(data, mPath)) {
                        Log.d(TAG, "RecordingStatusChanged, stopped[path:"+mPath+"]");
                        if (mRecordingProcessHandler != null) {
                            mRecordingProcessHandler.removeMessages(MSG_RECORD_UPDATE_RECORDING);
                            boolean result = mRecordingProcessHandler.sendMessage(mRecordingProcessHandler.obtainMessage(MSG_RECORD_UPDATE_RECORDING, 0, 0));
                            Log.d(TAG, "doStopRecording sendMessage MSG_RECORD_UPDATE_RECORDING = " + result + ", index = " + mCurrentRecordIndex);
                            if (result) {
                                mRecordStopAndSaveReceived = true;
                            }
                        } else {
                            Log.i(TAG, "doStopRecording sendMessage MSG_RECORD_UPDATE_RECORDING null, index = " + mCurrentRecordIndex);
                        }
                    }
                } else if (signal.equals("RecordingDiskFull")) {
                    notifyError(TvInputManager.RECORDING_ERROR_INSUFFICIENT_SPACE);
                }
            }
        };
    }

    class DtvkitTvInputSession extends TvInputService.Session               {
        private static final String TAG = "DtvkitTvInputSession";
        private static final int ADEC_START_DECODE = 1;
        private static final int ADEC_PAUSE_DECODE = 2;
        private static final int ADEC_RESUME_DECODE = 3;
        private static final int ADEC_STOP_DECODE = 4;
        private static final int ADEC_SET_DECODE_AD = 5;
        private static final int ADEC_SET_VOLUME = 6;
        private static final int ADEC_SET_MUTE = 7;
        private static final int ADEC_SET_OUTPUT_MODE = 8;
        private static final int ADEC_SET_PRE_GAIN = 9;
        private static final int ADEC_SET_PRE_MUTE = 10;
        private boolean mhegTookKey = false;
        private Channel mTunedChannel = null;
        private List<TvTrackInfo> mTunedTracks = null;
        protected final Context mContext;
        private RecordedProgram recordedProgram = null;
        private long originalStartPosition = TvInputManager.TIME_SHIFT_INVALID_TIME;
        private long startPosition = TvInputManager.TIME_SHIFT_INVALID_TIME;
        private long currentPosition = TvInputManager.TIME_SHIFT_INVALID_TIME;
        private float playSpeed = 0;
        private PlayerState playerState = PlayerState.STOPPED;
        private boolean timeshiftAvailable = false;
        private int timeshiftBufferSizeMins = 60;
        DtvkitOverlayView mView = null;
        private long mCurrentDtvkitTvInputSessionIndex = 0;
        protected Handler mHandlerThreadHandle = null;
        protected Handler mMainHandle = null;
        private boolean mIsMain = false;
        private final CaptioningManager mCaptioningManager;
        private boolean mKeyUnlocked = false;
        private boolean mBlocked = false;
        private int mSignalStrength = 0;
        private int mSignalQuality = 0;
        private boolean mMediaCodecPlay = false;
        private static final String TV_TF_DTV_MEDIACODECPLAY = "tv.tf.dtv.mediaCodecPlay";


        private int m_surface_left = 0;
        private int m_surface_right = 0;
        private int m_surface_top = 0;
        private int m_surface_bottom = 0;


        DtvkitTvInputSession(Context context) {
            super(context);
            mContext = context;
            Log.i(TAG, "DtvkitTvInputSession creat");
            setOverlayViewEnabled(true);
            mCaptioningManager =
                (CaptioningManager) context.getSystemService(Context.CAPTIONING_SERVICE);
            numActiveRecordings = recordingGetNumActiveRecordings();
            Log.i(TAG, "numActiveRecordings: " + numActiveRecordings);

            String mcodec = mSystemControlManager.getProperty(TV_TF_DTV_MEDIACODECPLAY);
            mMediaCodecPlay = Boolean.parseBoolean(mcodec);
            Log.i(TAG, "media codec used:" + mMediaCodecPlay + " str:"+mcodec);
            if (numRecorders == 0) {
                updateRecorderNumber();
            }
            if (numActiveRecordings < numRecorders) {
                timeshiftAvailable = true;
            } else {
                timeshiftAvailable = false;
            }

            timeshiftRecordRunnable = new Runnable() {
                @RequiresApi(api = Build.VERSION_CODES.M)
                @Override
                public void run() {
                    Log.i(TAG, "timeshiftRecordRunnable running");
                    resetRecordingPath();
                    tryStartTimeshifting();
                }
            };

            playerSetTimeshiftBufferSize(timeshiftBufferSizeMins);
            resetRecordingPath();
            mDtvkitTvInputSessionCount++;
            mCurrentDtvkitTvInputSessionIndex = mDtvkitTvInputSessionCount;
            initWorkThread();
        }

        @Override
        public void onSetMain(boolean isMain) {
            Log.d(TAG, "onSetMain, isMain: " + isMain +" mCurrentDtvkitTvInputSessionIndex is " + mCurrentDtvkitTvInputSessionIndex);
            mIsMain = isMain;
            //isMain status may be set to true by livetv after switch to luncher
            if (mCurrentDtvkitTvInputSessionIndex < mDtvkitTvInputSessionCount) {
                mIsMain = false;
            }
            if (!mIsMain) {
                if (null != mSysSettingManager)
                    mSysSettingManager.writeSysFs("/sys/class/video/disable_video", "1");
                //if (null != mView)
                    //layoutSurface(0, 0, mView.w, mView.h);
            } else {
                if (null != mSysSettingManager)
                    mSysSettingManager.writeSysFs("/sys/class/video/video_inuse", "1");
            }
        }

        @Override
        public void onRelease() {
            Log.i(TAG, "onRelease index = " + mCurrentDtvkitTvInputSessionIndex);
            //must destory mview,!!! we
            //will regist handle to client when
            //creat ciMenuView,so we need destory and
            //unregist handle.
            releaseSignalHandler();
            if (mDtvkitTvInputSessionCount == mCurrentDtvkitTvInputSessionIndex || mIsMain) {
                //release by message queue for current session
                if (mMainHandle != null) {
                    mMainHandle.sendEmptyMessage(MSG_MAIN_HANDLE_DESTROY_OVERLAY);
                } else {
                    Log.i(TAG, "onRelease mMainHandle == null");
                }
            } else {
                //release directly as new session has created
                finalReleaseWorkThread();
                doDestroyOverlay();
            }
            //send MSG_RELEASE_WORK_THREAD after dealing destroy overlay
            Log.i(TAG, "onRelease over index = " + mCurrentDtvkitTvInputSessionIndex);
        }

        private void doDestroyOverlay() {
            Log.i(TAG, "doDestroyOverlayr index = " + mCurrentDtvkitTvInputSessionIndex);
            if (mView != null) {
                mView.destroy();
            }
        }

        private void doRelease() {
            Log.i(TAG, "doRelease index = " + mCurrentDtvkitTvInputSessionIndex);
            releaseSignalHandler();
            removeScheduleTimeshiftRecordingTask();
            scheduleTimeshiftRecording = false;
            timeshiftRecorderState = RecorderState.STOPPED;
            timeshifting = false;
            mhegStop();
            playerStopTimeshiftRecording(false);
            playerStop();
            playerSetSubtitlesOn(false);
            playerSetTeletextOn(false, -1);
            mCurAudioFmt = -2;
            mCurSubAudioFmt = -2;
            Log.i(TAG, "doRelease over index = " + mCurrentDtvkitTvInputSessionIndex);
        }

        private void releaseSignalHandler() {
            DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
        }

        private void doSetSurface(Map<String, Object> surfaceInfo) {
            Log.i(TAG, "doSetSurface index = " + mCurrentDtvkitTvInputSessionIndex);
            if (surfaceInfo == null) {
                Log.d(TAG, "doSetSurface null parameter");
                return;
            } else {
                Surface surface = (Surface)surfaceInfo.get(ConstantManager.KEY_SURFACE);
                TvStreamConfig config = (TvStreamConfig)surfaceInfo.get(ConstantManager.KEY_TV_STREAM_CONFIG);
                Log.d(TAG, "doSetSurface surface = " + surface + ", config = " + config);
                mHardware.setSurface(surface, config);
            }
        }

        private void sendSetSurfaceMessage(Surface surface, TvStreamConfig config) {
            Map<String, Object> surfaceInfo = new HashMap<String, Object>();
            surfaceInfo.put(ConstantManager.KEY_SURFACE, surface);
            surfaceInfo.put(ConstantManager.KEY_TV_STREAM_CONFIG, config);
            if (mHandlerThreadHandle != null) {
                boolean result = mHandlerThreadHandle.sendMessage(mHandlerThreadHandle.obtainMessage(MSG_SET_SURFACE, surfaceInfo));
                Log.d(TAG, "sendSetSurfaceMessage status = " + result + ", surface = " + surface + ", config = " + config);
            } else {
                Log.d(TAG, "sendSetSurfaceMessage null mHandlerThreadHandle");
            }
        }

        private void sendDoReleaseMessage() {
            if (mHandlerThreadHandle != null) {
                boolean result = mHandlerThreadHandle.sendEmptyMessage(MSG_DO_RELEASE);
                Log.d(TAG, "sendDoReleaseMessage status = " + result);
            } else {
                Log.d(TAG, "sendDoReleaseMessage null mHandlerThreadHandle");
            }
        }

        @Override
        public boolean onSetSurface(Surface surface) {
            Log.i(TAG, "onSetSurface " + surface + ", mIsMain = " + mIsMain + ", mDtvkitTvInputSessionCount = " + mDtvkitTvInputSessionCount + ", mCurrentDtvkitTvInputSessionIndex = " + mCurrentDtvkitTvInputSessionIndex);
            if (null != mHardware && mConfigs.length > 0) {
                if (null == surface) {
                    //isMain status may be set to true by livetv after switch to luncher
                    if (mDtvkitTvInputSessionCount == mCurrentDtvkitTvInputSessionIndex || mIsMain) {
                        setOverlayViewEnabled(false);
                        //mHardware.setSurface(null, null);
                        sendSetSurfaceMessage(null, null);
                        Log.d(TAG, "onSetSurface null");
                        mSurface = null;
                        //doRelease();
                        sendDoReleaseMessage();
                        if (null != mSysSettingManager)
                            mSysSettingManager.writeSysFs("/sys/class/video/video_inuse", "0");
                    }
                } else {
                    if (mSurface != null && mSurface != surface) {
                        Log.d(TAG, "TvView swithed,  onSetSurface null first");
                        //doRelease();
                        sendDoReleaseMessage();
                        //mHardware.setSurface(null, null);
                        sendSetSurfaceMessage(null, null);
                    }
                    //mHardware.setSurface(surface, mConfigs[0]);
                    createDecoder();
                    decoderRelease();
                    sendSetSurfaceMessage(surface, mConfigs[0]);
                    mSurface = surface;
                }
            }
            //set surface to mediaplayer
            //DtvkitGlueClient.getInstance().setDisplay(surface);
            return true;
        }

        @Override
        public void onSurfaceChanged(int format, int width, int height) {
            Log.i(TAG, "onSurfaceChanged " + format + ", " + width + ", " + height + ", index = " + mCurrentDtvkitTvInputSessionIndex);
            //playerSetRectangle(0, 0, width, height);
        }

        public View onCreateOverlayView() {
            Log.i(TAG, "onCreateOverlayView index = " + mCurrentDtvkitTvInputSessionIndex);
            if (mView == null) {
                mView = new DtvkitOverlayView(mContext);
            }
            return mView;
        }

        @Override
        public void onOverlayViewSizeChanged(int width, int height) {
            if (mView == null) {
                mView = new DtvkitOverlayView(mContext);
            }
            playerSetRectangle(0, 0, width, height);
            mView.setSize(width, height);
        }

        @RequiresApi(api = Build.VERSION_CODES.M)
        @Override
        public boolean onTune(Uri channelUri) {
            Log.i(TAG, "onTune " + channelUri + "index = " + mCurrentDtvkitTvInputSessionIndex);
            if (ContentUris.parseId(channelUri) == -1) {
                Log.e(TAG, "DtvkitTvInputSession onTune invalid channelUri = " + channelUri);
                return false;
            }

            if (mMainHandle != null) {
                mMainHandle.sendEmptyMessage(MSG_HIDE_SCAMBLEDTEXT);
            }

            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeMessages(MSG_ON_TUNE);
                Message mess = mHandlerThreadHandle.obtainMessage(MSG_ON_TUNE, 0, 0, channelUri);
                boolean info = mHandlerThreadHandle.sendMessage(mess);
                Log.d(TAG, "sendMessage " + info);
            }

            mTunedChannel = getChannel(channelUri);

            Log.i(TAG, "onTune will be Done in onTuneByHandlerThreadHandle");
            return mTunedChannel != null;
        }

        // For private app params. Default calls onTune
        //public boolean onTune(Uri channelUri, Bundle params)

        @Override
        public void onSetStreamVolume(float volume) {
            Log.i(TAG, "onSetStreamVolume " + volume + ", mute " + (volume == 0.0f) + "index = " + mCurrentDtvkitTvInputSessionIndex);
            //playerSetVolume((int) (volume * 100));
            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeMessages(MSG_BLOCK_MUTE_OR_UNMUTE);
                Message mess = mHandlerThreadHandle.obtainMessage(MSG_BLOCK_MUTE_OR_UNMUTE, (volume == 0.0f ? 1 : 0), 0);
                mHandlerThreadHandle.sendMessageDelayed(mess, MSG_BLOCK_MUTE_OR_UNMUTE_PERIOD);
            }
        }

        @Override
        public void onSetCaptionEnabled(boolean enabled) {
            Log.i(TAG, "onSetCaptionEnabled " + enabled + ", index = " + mCurrentDtvkitTvInputSessionIndex);
            if (true) {
                Log.i(TAG, "caption switch will be controlled by mCaptionManager switch");
                return;
            }
            /*Log.i(TAG, "onSetCaptionEnabled " + enabled);
            // TODO CaptioningManager.getLocale()
            playerSetSubtitlesOn(enabled);//start it in select track or gettracks in onsignal*/
        }

        @Override
        public boolean onSelectTrack(int type, String trackId) {
            Log.i(TAG, "onSelectTrack " + type + ", " + trackId + ", index = " + mCurrentDtvkitTvInputSessionIndex);
            if (type == TvTrackInfo.TYPE_AUDIO) {
                if (playerSelectAudioTrack((null == trackId) ? 0xFFFF : Integer.parseInt(trackId))) {
                    notifyTrackSelected(type, trackId);
                    //check trackinfo update
                    if (mHandlerThreadHandle != null) {
                        mHandlerThreadHandle.removeMessages(MSG_UPDATE_TRACKINFO);
                        mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_UPDATE_TRACKINFO, MSG_UPDATE_TRACKINFO_DELAY);
                    }
                    return true;
                }
            } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
                String sourceTrackId = trackId;
                int subType = 4;//default sub
                int isTele = 0;//default subtitle
                if (!TextUtils.isEmpty(trackId) && !TextUtils.isDigitsOnly(trackId)) {
                    String[] nameValuePairs = trackId.split("&");
                    if (nameValuePairs != null && nameValuePairs.length == 5) {
                        String[] nameValue = nameValuePairs[0].split("=");
                        String[] typeValue = nameValuePairs[1].split("=");
                        String[] teleValue = nameValuePairs[2].split("=");
                        if (nameValue != null && nameValue.length == 2 && TextUtils.equals(nameValue[0], "id") && TextUtils.isDigitsOnly(nameValue[1])) {
                            trackId = nameValue[1];//parse id
                        }
                        if (typeValue != null && typeValue.length == 2 && TextUtils.equals(typeValue[0], "type") && TextUtils.isDigitsOnly(typeValue[1])) {
                            subType = Integer.parseInt(typeValue[1]);//parse type
                        }
                        if (teleValue != null && teleValue.length == 2 && TextUtils.equals(teleValue[0], "teletext") && TextUtils.isDigitsOnly(teleValue[1])) {
                            isTele = Integer.parseInt(teleValue[1]);//parse type
                        }
                    }
                    if (TextUtils.isEmpty(trackId) || !TextUtils.isDigitsOnly(trackId)) {
                        //notifyTrackSelected(type, sourceTrackId);
                        Log.d(TAG, "need trackId that only contains number sourceTrackId = " + sourceTrackId + ", trackId = " + trackId);
                        return false;
                    }
                }
                if (mCaptioningManager.isEnabled() && selectSubtitleOrTeletext(isTele, subType, trackId)) {
                    notifyTrackSelected(type, sourceTrackId);
                } else {
                    Log.d(TAG, "onSelectTrack mCaptioningManager closed or invlid sub");
                    notifyTrackSelected(type, null);
                }
            }
            return false;
        }

        private boolean selectSubtitleOrTeletext(int istele, int type, String indexId) {
            boolean result = false;
            Log.d(TAG, "selectSubtitleOrTeletext istele = " + istele + ", type = " + type + ", indexId = " + indexId);
            if (TextUtils.isEmpty(indexId)) {//stop
                if (playerGetSubtitlesOn()) {
                    playerSetSubtitlesOn(false);//close if opened
                    Log.d(TAG, "selectSubtitleOrTeletext off setSubOff");
                }
                if (playerIsTeletextOn()) {
                    boolean setTeleOff = playerSetTeletextOn(false, -1);//close if opened
                    Log.d(TAG, "selectSubtitleOrTeletext off setTeleOff = " + setTeleOff);
                }
                boolean stopSub = playerSelectSubtitleTrack(0xFFFF);
                boolean stopTele = playerSelectTeletextTrack(0xFFFF);
                Log.d(TAG, "selectSubtitleOrTeletext stopSub = " + stopSub + ", stopTele = " + stopTele);
                result = true;
            } else if (TextUtils.isDigitsOnly(indexId)) {
                if (type == 4) {//sub
                    if (playerIsTeletextOn()) {
                        boolean setTeleOff = playerSetTeletextOn(false, -1);
                        Log.d(TAG, "selectSubtitleOrTeletext onsub setTeleOff = " + setTeleOff);
                    }
                    if (!playerGetSubtitlesOn()) {
                        playerSetSubtitlesOn(true);
                        Log.d(TAG, "selectSubtitleOrTeletext onsub setSubOn");
                    }
                    boolean startSub = playerSelectSubtitleTrack(Integer.parseInt(indexId));
                    Log.d(TAG, "selectSubtitleOrTeletext startSub = " + startSub);
                } else if (type == 6) {//teletext
                    if (playerGetSubtitlesOn()) {
                        playerSetSubtitlesOn(false);
                        Log.d(TAG, "selectSubtitleOrTeletext ontele setSubOff");
                    }
                    if (!playerIsTeletextOn()) {
                        boolean setTeleOn = playerSetTeletextOn(true, Integer.parseInt(indexId));
                        Log.d(TAG, "selectSubtitleOrTeletext start setTeleOn = " + setTeleOn);
                    } else {
                        boolean startTele = playerSelectTeletextTrack(Integer.parseInt(indexId));
                        Log.d(TAG, "selectSubtitleOrTeletext set setTeleOn = " + startTele);
                    }
                }
                result = true;
            } else {
                result = false;
                Log.d(TAG, "selectSubtitleOrTeletext unkown case");
            }
            return result;
        }

        private boolean initSubtitleOrTeletextIfNeed() {
            boolean isSubOn = playerGetSubtitlesOn();
            boolean isTeleOn = playerIsTeletextOn();
            String subTrackId = playerGetSelectedSubtitleTrackId();
            String teleTrackId = playerGetSelectedTeleTextTrackId();
            int subIndex = playerGetSelectedSubtitleTrack();
            int teleIndex = playerGetSelectedTeleTextTrack();
            Log.d(TAG, "initSubtitleOrTeletextIfNeed isSubOn = " + isSubOn + ", isTeleOn = " + isTeleOn + ", subTrackId = " + subTrackId + ", teleTrackId = " + teleTrackId);
            if (isSubOn) {
                notifyTrackSelected(TvTrackInfo.TYPE_SUBTITLE, subTrackId);
            } else if (isTeleOn) {
                notifyTrackSelected(TvTrackInfo.TYPE_SUBTITLE, teleTrackId);
            } else {
                notifyTrackSelected(TvTrackInfo.TYPE_SUBTITLE, null);
            }
            return true;
        }

        @Override
        public void onUnblockContent(TvContentRating unblockedRating) {
            super.onUnblockContent(unblockedRating);
            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeMessages(MSG_SET_UNBLOCK);
                Message mess = mHandlerThreadHandle.obtainMessage(MSG_SET_UNBLOCK,unblockedRating);
                boolean result = mHandlerThreadHandle.sendMessage(mess);
                Log.d(TAG, "onUnblockContent sendMessage result " + result);
            }
        }

        @Override
        public void notifyVideoAvailable() {
            super.notifyVideoAvailable();
            if (mMainHandle != null) {
                mMainHandle.sendEmptyMessage(MSG_HIDE_SCAMBLEDTEXT);
            }
        }

        @Override
        public void notifyVideoUnavailable(final int reason) {
            super.notifyVideoUnavailable(reason);
        }

        @Override
        public void onAppPrivateCommand(String action, Bundle data) {
            Log.i(TAG, "onAppPrivateCommand " + action + ", " + data + ", index = " + mCurrentDtvkitTvInputSessionIndex);
            if ("action_teletext_start".equals(action) && data != null) {
                boolean start = data.getBoolean("action_teletext_start", false);
                Log.d(TAG, "do private cmd: action_teletext_start: "+ start);
            } else if ("action_teletext_up".equals(action) && data != null) {
                boolean actionup = data.getBoolean("action_teletext_up", false);
                Log.d(TAG, "do private cmd: action_teletext_up: "+ actionup);
                playerNotifyTeletextEvent(16);
            } else if ("action_teletext_down".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("action_teletext_down", false);
                Log.d(TAG, "do private cmd: action_teletext_down: "+ actiondown);
                playerNotifyTeletextEvent(15);
            } else if ("action_teletext_number".equals(action) && data != null) {
                int number = data.getInt("action_teletext_number", -1);
                Log.d(TAG, "do private cmd: action_teletext_number: "+ number);
                final int TT_EVENT_0 = 4;
                final int TT_EVENT_9 = 13;
                int hundred = (number % 1000) / 100;
                int decade = (number % 100) / 10;
                int unit = (number % 10);
                if (number >= 100) {
                    playerNotifyTeletextEvent(hundred + TT_EVENT_0);
                    playerNotifyTeletextEvent(decade + TT_EVENT_0);
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                } else if (number >= 10 && number < 100) {
                    playerNotifyTeletextEvent(decade + TT_EVENT_0);
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                } else if (number < 10) {
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                }
            } else if ("action_teletext_country".equals(action) && data != null) {
                int number = data.getInt("action_teletext_country", -1);
                Log.d(TAG, "do private cmd: action_teletext_country: "+ number);
                final int TT_EVENT_0 = 4;
                final int TT_EVENT_9 = 13;
                int hundred = (number % 1000) / 100;
                int decade = (number % 100) / 10;
                int unit = (number % 10);
                if (number >= 100) {
                    playerNotifyTeletextEvent(hundred + TT_EVENT_0);
                    playerNotifyTeletextEvent(decade + TT_EVENT_0);
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                } else if (number >= 10 && number < 100) {
                    playerNotifyTeletextEvent(decade + TT_EVENT_0);
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                } else if (number < 10) {
                    playerNotifyTeletextEvent(unit + TT_EVENT_0);
                }
            } else if ("quick_navigate_1".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("quick_navigate_1", false);
                Log.d(TAG, "do private cmd: quick_navigate_1: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(0);
                }
            } else if ("quick_navigate_2".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("quick_navigate_2", false);
                Log.d(TAG, "do private cmd: quick_navigate_2: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(1);
                }
            } else if ("quick_navigate_3".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("quick_navigate_3", false);
                Log.d(TAG, "do private cmd: quick_navigate_3: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(2);
                }
            } else if ("quick_navigate_4".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("quick_navigate_4", false);
                Log.d(TAG, "do private cmd: quick_navigate_4: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(3);
                }
            } else if ("previous_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("previous_page", false);
                Log.d(TAG, "do private cmd: previous_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(16);
                }
            } else if ("next_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("next_page", false);
                Log.d(TAG, "do private cmd: next_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(15);
                }
            } else if ("index_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("index_page", false);
                Log.d(TAG, "do private cmd: index_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(14);
                }
            } else if ("next_sub_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("next_sub_page", false);
                Log.d(TAG, "do private cmd: next_sub_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(17);
                }
            } else if ("previous_sub_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("previous_sub_page", false);
                Log.d(TAG, "do private cmd: previous_sub_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(18);
                }
            } else if ("back_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("back_page", false);
                Log.d(TAG, "do private cmd: back_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(19);
                }
            } else if ("forward_page".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("forward_page", false);
                Log.d(TAG, "do private cmd: forward_page: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(20);
                }
            } else if ("hold".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("hold", false);
                Log.d(TAG, "do private cmd: hold: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(21);
                }
            } else if ("reveal".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("reveal", false);
                Log.d(TAG, "do private cmd: reveal: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(22);
                }
            } else if ("clear".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("clear", false);
                Log.d(TAG, "do private cmd: clear: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(23);
                }
            } else if ("mix_video".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("mix_video", false);
                Log.d(TAG, "do private cmd: mix_video: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(24);
                }
            } else if ("double_height".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("double_height", false);
                Log.d(TAG, "do private cmd: double_height: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(25);
                }
            } else if ("double_scroll_up".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("double_scroll_up", false);
                Log.d(TAG, "do private cmd: double_scroll_up: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(26);
                }
            } else if ("double_scroll_down".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("double_scroll_down", false);
                Log.d(TAG, "do private cmd: double_scroll_down: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(27);
                }
            } else if ("timer".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("timer", false);
                Log.d(TAG, "do private cmd: timer: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(28);
                }
            } else if ("clock".equals(action) && data != null) {
                boolean actiondown = data.getBoolean("clock", false);
                Log.d(TAG, "do private cmd: clock: "+ actiondown);
                if (actiondown) {
                    playerNotifyTeletextEvent(29);
                }
            } else if (ConstantManager.ACTION_TIF_CONTROL_OVERLAY.equals(action)) {
                boolean show = data.getBoolean(ConstantManager.KEY_TIF_OVERLAY_SHOW_STATUS, false);
                Log.d(TAG, "do private cmd:"+ ConstantManager.ACTION_TIF_CONTROL_OVERLAY + ", show:" + show);
                //not needed at the moment
                /*if (!show) {
                    if (mView != null) {
                        mView.hideOverLay();
                    };
                } else {
                    if (mView != null) {
                        mView.showOverLay();
                    };
                }*/
            } else if (TextUtils.equals(DataMananer.ACTION_AD_MIXING_LEVEL, action)) {
                mAudioADMixingLevel = data.getInt(DataMananer.PARA_VALUE1);
                setAdFunction(MSG_MIX_AD_MIX_LEVEL, mAudioADMixingLevel);
            }
        }

        @RequiresApi(api = Build.VERSION_CODES.M)
        public void onTimeShiftPlay(Uri uri) {
            Log.i(TAG, "onTimeShiftPlay " + uri);

            recordedProgram = getRecordedProgram(uri);
            if (recordedProgram != null) {
                playerState = PlayerState.PLAYING;
                playerStop();
                DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
                mAudioADAutoStart = mDataMananer.getIntParameters(DataMananer.TV_KEY_AD_SWITCH) == 1;
                if (playerPlay(recordedProgram.getRecordingDataUri(), mAudioADAutoStart).equals("ok"))
                {
                    if (mHandlerThreadHandle != null) {
                        mHandlerThreadHandle.removeMessages(MSG_CHECK_PARENTAL_CONTROL);
                        mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_PARENTAL_CONTROL, MSG_CHECK_PARENTAL_CONTROL_PERIOD);
                    }
                    DtvkitGlueClient.getInstance().setAudioHandler(AHandler);
                }
                else
                {
                    DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
                    notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                }
            }
        }

        public void onTimeShiftPause() {
            Log.i(TAG, "onTimeShiftPause timeshiftRecorderState:"+timeshiftRecorderState+" timeshifting:"+timeshifting);
            if (timeshiftRecorderState == RecorderState.RECORDING && !timeshifting) {
                Log.i(TAG, "starting pause playback ");
                timeshifting = true;

                /*
                  The mheg may hold an external_control in the dtvkit,
                  which upset the normal av process following, so stop it first,
                  thus, mheg will not be valid since here to the next onTune.
                */
                mhegStop();

                playerPlayTimeshiftRecording(true, true);
            }
            else {
                Log.i(TAG, "player pause ");
                if (playerPause())
                {
                    playSpeed = 0;
                }
            }
        }

        public void onTimeShiftResume() {
            Log.i(TAG, "onTimeShiftResume ");
            playerState = PlayerState.PLAYING;
            if (playerResume())
            {
                playSpeed = 1;
            }
        }

        public void onTimeShiftSeekTo(long timeMs) {
            Log.i(TAG, "onTimeShiftSeekTo:  " + timeMs);
            if (timeshiftRecorderState == RecorderState.RECORDING && !timeshifting) /* Watching live tv while recording */ {
                timeshifting = true;
                boolean seekToBeginning = false;

                if (timeMs == startPosition) {
                    seekToBeginning = true;
                }
                playerPlayTimeshiftRecording(false, !seekToBeginning);
            } else if (timeshiftRecorderState == RecorderState.RECORDING && timeshifting) {
                playerSeekTo((timeMs - (originalStartPosition + PropSettingManager.getStreamTimeDiff())) / 1000);
            } else {
                playerSeekTo(timeMs / 1000);
            }
        }

        @RequiresApi(api = Build.VERSION_CODES.M)
        public void onTimeShiftSetPlaybackParams(PlaybackParams params) {
            Log.i(TAG, "onTimeShiftSetPlaybackParams");

            float speed = params.getSpeed();
            Log.i(TAG, "speed: " + speed);
            if (speed != playSpeed) {
                if (timeshiftRecorderState == RecorderState.RECORDING && !timeshifting) {
                    timeshifting = true;
                    playerPlayTimeshiftRecording(false, true);
                }

                if (playerSetSpeed(speed))
                {
                    playSpeed = speed;
                }
            }
        }

        public long onTimeShiftGetStartPosition() {
            if (timeshiftRecorderState != RecorderState.STOPPED) {
                Log.i(TAG, "requesting timeshift recorder status");
                long length = 0;
                JSONObject timeshiftRecorderStatus = playerGetTimeshiftRecorderStatus();
                if (originalStartPosition != 0 && originalStartPosition != TvInputManager.TIME_SHIFT_INVALID_TIME) {
                    startPosition = originalStartPosition + PropSettingManager.getStreamTimeDiff();
                }
                if (timeshiftRecorderStatus != null) {
                    try {
                        length = timeshiftRecorderStatus.getLong("length");
                    } catch (JSONException e) {
                        Log.e(TAG, e.getMessage());
                    }
                }

                if (length > (timeshiftBufferSizeMins * 60)) {
                    startPosition = originalStartPosition + ((length - (timeshiftBufferSizeMins * 60)) * 1000);
                    Log.i(TAG, "new start position: " + startPosition);
                }
            }
            Log.i(TAG, "onTimeShiftGetStartPosition startPosition:" + startPosition + ", as date = " + ConvertSettingManager.convertLongToDate(startPosition));
            return startPosition;
        }

        public long onTimeShiftGetCurrentPosition() {
            if (startPosition == 0) /* Playing back recorded program */ {
                if (playerState == PlayerState.PLAYING) {
                    currentPosition = playerGetElapsed() * 1000;
                    Log.i(TAG, "playing back record program. current position: " + currentPosition);
                }
            } else if (timeshifting) {
                currentPosition = (playerGetElapsed() * 1000) + originalStartPosition + PropSettingManager.getStreamTimeDiff();
                Log.i(TAG, "timeshifting. current position: " + currentPosition);
            } else if (startPosition == TvInputManager.TIME_SHIFT_INVALID_TIME) {
                currentPosition = TvInputManager.TIME_SHIFT_INVALID_TIME;
                Log.i(TAG, "Invalid time. Current position: " + currentPosition);
            } else {
                currentPosition = /*System.currentTimeMillis()*/PropSettingManager.getCurrentStreamTime(true);
                Log.i(TAG, "live tv. current position: " + currentPosition);
            }
            Log.d(TAG, "onTimeShiftGetCurrentPosition currentPosition = " + currentPosition + ", as date = " + ConvertSettingManager.convertLongToDate(currentPosition));
            return currentPosition;
        }

        private RecordedProgram getRecordedProgram(Uri uri) {
            RecordedProgram recordedProgram = null;
            if (uri != null) {
                Cursor cursor = mContext.getContentResolver().query(uri, RecordedProgram.PROJECTION, null, null, null);
                if (cursor != null) {
                    while (cursor.moveToNext()) {
                        recordedProgram = RecordedProgram.fromCursor(cursor);
                    }
                }
            }
            return recordedProgram;
        }

        @Override
        public boolean onKeyDown(int keyCode, KeyEvent event) {
            boolean used;

            Log.i(TAG, "onKeyDown " + event);
            if (mDvbNetworkChangeSearchStatus) {
                Log.i(TAG, "onKeyDown skip as search action is raised");
                return true;
            }
            /* It's possible for a keypress to be registered before the overlay is created */
            if (mView == null) {
                used = super.onKeyDown(keyCode, event);
            }
            else {
                if (mView.handleKeyDown(keyCode, event)) {
                    used = true;
                } else if (isTeletextNeedKeyCode(keyCode) && playerIsTeletextOn()) {
                    used = true;
                } else {
                   used = super.onKeyDown(keyCode, event);
                }
            }

            return used;
        }

        @Override
        public boolean onKeyUp(int keyCode, KeyEvent event) {
            boolean used;

            Log.i(TAG, "onKeyUp " + event);
            if (mDvbNetworkChangeSearchStatus) {
                Log.i(TAG, "onKeyUp skip as search action is raised");
                return true;
            }
            /* It's possible for a keypress to be registered before the overlay is created */
            if (mView == null) {
                used = super.onKeyUp(keyCode, event);
            }
            else {
                if (mView.handleKeyUp(keyCode, event)) {
                    used = true;
                } else if (isTeletextNeedKeyCode(keyCode) && playerIsTeletextOn()) {
                    dealTeletextKeyCode(keyCode);
                    used = true;
                } else {
                    used = super.onKeyUp(keyCode, event);
                }
            }
            return used;
        }

        private boolean isTeletextNeedKeyCode(int keyCode) {
            return keyCode == KeyEvent.KEYCODE_BACK ||
                    keyCode == KeyEvent.KEYCODE_PROG_RED ||
                    keyCode == KeyEvent.KEYCODE_PROG_GREEN ||
                    keyCode == KeyEvent.KEYCODE_PROG_YELLOW ||
                    keyCode == KeyEvent.KEYCODE_PROG_BLUE ||
                    keyCode == KeyEvent.KEYCODE_CHANNEL_UP ||
                    keyCode == KeyEvent.KEYCODE_DPAD_UP ||
                    keyCode == KeyEvent.KEYCODE_CHANNEL_DOWN ||
                    keyCode == KeyEvent.KEYCODE_DPAD_DOWN ||
                    keyCode == KeyEvent.KEYCODE_0 ||
                    keyCode == KeyEvent.KEYCODE_1 ||
                    keyCode == KeyEvent.KEYCODE_2 ||
                    keyCode == KeyEvent.KEYCODE_3 ||
                    keyCode == KeyEvent.KEYCODE_4 ||
                    keyCode == KeyEvent.KEYCODE_5 ||
                    keyCode == KeyEvent.KEYCODE_6 ||
                    keyCode == KeyEvent.KEYCODE_7 ||
                    keyCode == KeyEvent.KEYCODE_8 ||
                    keyCode == KeyEvent.KEYCODE_9;
        }

        private void dealTeletextKeyCode(int keyCode) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_BACK:
                    Log.d(TAG, "dealTeletextKeyCode close teletext");
                    playerSetTeletextOn(false, -1);
                    notifyTrackSelected(TvTrackInfo.TYPE_SUBTITLE, null);
                    break;
                case KeyEvent.KEYCODE_PROG_RED:
                    Log.d(TAG, "dealTeletextKeyCode quick_navigate_1");
                    playerNotifyTeletextEvent(0);
                    break;
                case KeyEvent.KEYCODE_PROG_GREEN:
                    Log.d(TAG, "dealTeletextKeyCode quick_navigate_2");
                    playerNotifyTeletextEvent(1);
                    break;
                case KeyEvent.KEYCODE_PROG_YELLOW:
                    Log.d(TAG, "dealTeletextKeyCode quick_navigate_3");
                    playerNotifyTeletextEvent(2);
                    break;
                case KeyEvent.KEYCODE_PROG_BLUE:
                    Log.d(TAG, "dealTeletextKeyCode quick_navigate_4");
                    playerNotifyTeletextEvent(3);
                    break;
                case KeyEvent.KEYCODE_CHANNEL_UP:
                case KeyEvent.KEYCODE_DPAD_UP:
                    Log.d(TAG, "dealTeletextKeyCode previous_page");
                    playerNotifyTeletextEvent(16);
                    break;
                case KeyEvent.KEYCODE_CHANNEL_DOWN:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    Log.d(TAG, "dealTeletextKeyCode next_page");
                    playerNotifyTeletextEvent(15);
                    break;
                case KeyEvent.KEYCODE_0:
                case KeyEvent.KEYCODE_1:
                case KeyEvent.KEYCODE_2:
                case KeyEvent.KEYCODE_3:
                case KeyEvent.KEYCODE_4:
                case KeyEvent.KEYCODE_5:
                case KeyEvent.KEYCODE_6:
                case KeyEvent.KEYCODE_7:
                case KeyEvent.KEYCODE_8:
                case KeyEvent.KEYCODE_9: {
                    final int TT_EVENT_0 = 4;
                    int number = keyCode - KeyEvent.KEYCODE_0;
                    Log.d(TAG, "dealTeletextKeyCode number = " + number);
                    playerNotifyTeletextEvent(number + TT_EVENT_0);
                    break;
                }
                default:
                    break;
            }
        }

        private final DtvkitGlueClient.AudioHandler AHandler = new DtvkitGlueClient.AudioHandler() {
            @RequiresApi(api = Build.VERSION_CODES.M)
            @Override
            public void onEvent(String signal, JSONObject data) {
                Log.i(TAG, "onSignal: " + signal + " : " + data.toString() + ", index = " + mCurrentDtvkitTvInputSessionIndex);
                if (signal.equals("AudioParamCB")) {
                    int cmd = 0, param1 = 0, param2 = 0;
                    AudioManager audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
                    try {
                        cmd = data.getInt("audio_status");
                        param1 = data.getInt("audio_param1");
                        param2 = data.getInt("audio_param2");
                    } catch (JSONException ignore) {
                        Log.e(TAG, ignore.getMessage());
                    }
                    //Log.d(TAG, "cmd ="+cmd+" param1 ="+param1+" param2 ="+param2);
                    switch (cmd) {
                        case ADEC_START_DECODE:
                            audioManager.setParameters("fmt="+param1);
                            audioManager.setParameters("has_dtv_video="+param2);
                            audioManager.setParameters("cmd="+cmd);
                            Log.d(TAG, "AHandler setParameters ADEC_START_DECODE:fmt=" + param1
                                    + ",has_dtv_video=" + param2 + "," + "cmd=" + cmd);
                            mCurAudioFmt = param1;
                            break;
                        case ADEC_PAUSE_DECODE:
                            audioManager.setParameters("cmd="+cmd);
                            Log.d(TAG, "AHandler setParameters ADEC_PAUSE_DECODE:cmd=" + cmd);
                            break;
                        case ADEC_RESUME_DECODE:
                            audioManager.setParameters("cmd="+cmd);
                            Log.d(TAG, "AHandler setParameters ADEC_RESUME_DECODE:cmd=" + cmd);
                            break;
                        case ADEC_STOP_DECODE:
                            audioManager.setParameters("cmd="+cmd);
                            Log.d(TAG, "AHandler setParameters ADEC_STOP_DECODE:cmd=" + cmd);
                            break;
                        case ADEC_SET_DECODE_AD:
                            if (mAudioADAutoStart) {
                                if (param2 == 0) {
                                    //close ad
                                    setAdFunction(MSG_MIX_AD_DUAL_SUPPORT, 0);
                                    setAdFunction(MSG_MIX_AD_MIX_SUPPORT, 0);
                                } else if (param2 != 0 && param2 != 0x1fff) {
                                    //valid ad pid
                                    setAdFunction(MSG_MIX_AD_DUAL_SUPPORT, 1);
                                    setAdFunction(MSG_MIX_AD_MIX_SUPPORT, 1);
                                    setAdFunction(MSG_MIX_AD_MIX_LEVEL, mAudioADMixingLevel);
                                } else {
                                    Log.d(TAG, "AHandler setParameters ADEC_SET_DECODE_AD unkown pid");
                                }
                            }
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("subafmt="+param1);
                            audioManager.setParameters("subapid="+param2);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_DECODE_AD:cmd=" + cmd
                                    + ",subafmt=" + param1 + ",subapid=" + param2);
                            mCurAudioFmt = param1;
                            mCurSubAudioFmt = param1;
                            mCurSubAudioPid = param2;
                            break;
                        case ADEC_SET_VOLUME:
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("vol="+param1);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_VOLUME:cmd=" + cmd
                                    + ",vol=" + param1);
                            break;
                        case ADEC_SET_MUTE:
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("mute="+param1);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_MUTE:cmd=" + cmd
                                    + ",mute=" + param1);
                            break;
                        case ADEC_SET_OUTPUT_MODE:
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("mode="+param1);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_OUTPUT_MODE:cmd=" + cmd
                                    + ",mode=" + param1);
                            break;
                        case ADEC_SET_PRE_GAIN:
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("gain="+param1);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_PRE_GAIN:cmd=" + cmd
                                    + ",gain=" + param1);
                            break;
                        case ADEC_SET_PRE_MUTE:
                            audioManager.setParameters("cmd="+cmd);
                            audioManager.setParameters("mute="+param1);
                            Log.d(TAG, "AHandler setParameters ADEC_SET_PRE_MUTE:cmd=" + cmd
                                    + ",mute=" + param1);
                            break;
                        default:
                            Log.i(TAG,"AHandler setParameters unkown audio cmd!");
                            break;
                    }
                }
            }
        };

        private final DtvkitGlueClient.SignalHandler mHandler = new DtvkitGlueClient.SignalHandler() {
            @RequiresApi(api = Build.VERSION_CODES.M)
            @Override
            public void onSignal(String signal, JSONObject data) {
                Log.i(TAG, "onSignal: " + signal + " : " + data.toString() + ", index = " + mCurrentDtvkitTvInputSessionIndex);
                // TODO notifyTracksChanged(List<TvTrackInfo> tracks)
                if (signal.equals("PlayerStatusChanged")) {
                    String state = "off";
                    String dvbUri = "";
                    try {
                        state = data.getString("state");
                        dvbUri= data.getString("uri");
                    } catch (JSONException ignore) {
                    }
                    Log.i(TAG, "signal: "+state);
                    switch (state) {
                        case "playing":
                            String type = "dvblive";
                            try {
                                type = data.getString("type");
                            } catch (JSONException e) {
                                Log.e(TAG, e.getMessage());
                            }
                            if (type.equals("dvblive")) {
                                if (mTunedChannel.getServiceType().equals(TvContract.Channels.SERVICE_TYPE_AUDIO)) {
                                    notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY);
                                } else {
                                    notifyVideoAvailable();
                                }

                                if (mCaptioningManager != null && mCaptioningManager.isEnabled()) {
                                    playerSetSubtitlesOn(true);
                                }

                                List<TvTrackInfo> tracks = playerGetTracks(mTunedChannel, false);
                                if (!tracks.equals(mTunedTracks)) {
                                    mTunedTracks = tracks;
                                    // TODO Also for service changed event
                                    Log.d(TAG, "update new mTunedTracks");
                                }
                                notifyTracksChanged(mTunedTracks);

                                if (mTunedChannel.getServiceType().equals(TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO)) {
                                    if (mHandlerThreadHandle != null) {
                                        mHandlerThreadHandle.removeMessages(MSG_CHECK_RESOLUTION);
                                        mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_RESOLUTION, MSG_CHECK_RESOLUTION_PERIOD);//check resolution later
                                    }
                                }
                                if (mHandlerThreadHandle != null) {
                                    mHandlerThreadHandle.removeMessages(MSG_GET_SIGNAL_STRENGTH);
                                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_GET_SIGNAL_STRENGTH, MSG_GET_SIGNAL_STRENGTH_PERIOD);//check signal per 1s
                                    mHandlerThreadHandle.removeMessages(MSG_UPDATE_TRACKINFO);
                                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_UPDATE_TRACKINFO, MSG_UPDATE_TRACKINFO_DELAY);
                                }
                                Log.i(TAG, "audio track selected: " + playerGetSelectedAudioTrack());
                                notifyTrackSelected(TvTrackInfo.TYPE_AUDIO, Integer.toString(playerGetSelectedAudioTrack()));
                                initSubtitleOrTeletextIfNeed();

                                resetRecordingPath();
                                tryStartTimeshifting();
                                monitorRecordingPath(true);
                            }
                            else if (type.equals("dvbrecording")) {
                                startPosition = originalStartPosition = 0; // start position is always 0 when playing back recorded program
                                currentPosition = playerGetElapsed(data) * 1000;
                                Log.i(TAG, "dvbrecording currentPosition = " + currentPosition + "as date = " + ConvertSettingManager.convertLongToDate(startPosition));
                                notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_AVAILABLE);
                            }
                            else if (type.equals("dvbtimeshifting")) {
                                if (mTunedChannel.getServiceType().equals(TvContract.Channels.SERVICE_TYPE_AUDIO)) {
                                    notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY);
                                } else {
                                    notifyVideoAvailable();
                                }
                            }
                            playerState = PlayerState.PLAYING;
                            break;
                        case "blocked":
                            String Rating = "";
                            try {
                                Rating = String.format("DVB_%d", data.getInt("rating"));
                            } catch (JSONException ignore) {
                                Log.e(TAG, ignore.getMessage());
                            }
                            notifyContentBlocked(TvContentRating.createRating("com.android.tv", "DVB", Rating));
                            break;
                        case "badsignal":
                            notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL);
                            boolean isTeleOn = playerIsTeletextOn();
                            if (isTeleOn) {
                                playerSetTeletextOn(false, -1);
                                notifyTrackSelected(TvTrackInfo.TYPE_SUBTITLE, null);
                                Log.d(TAG, "close teletext when badsignal received");
                            }
                            break;
                        case "off":
                            if (timeshiftRecorderState != RecorderState.STOPPED) {
                                removeScheduleTimeshiftRecordingTask();
                                scheduleTimeshiftRecording = false;
                                playerStopTimeshiftRecording(false);
                                timeshiftRecorderState = RecorderState.STOPPED;
                                notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
                            }
                            playerState = PlayerState.STOPPED;
                            if (recordedProgram != null) {
                                currentPosition = recordedProgram.getEndTimeUtcMillis();
                            }

                            break;
                        case "starting":
                           Log.i(TAG, "mhegStart " + dvbUri);
                           if (mhegStartService(dvbUri) != -1)
                           {
                              Log.i(TAG, "mhegStarted");
                           }
                           else
                           {
                              Log.i(TAG, "mheg failed to start");
                           }
                           break;
                        case "scambled":
                            /*notify scambled*/
                            Log.i(TAG, "** scambled **");
                            if (mMainHandle != null) {
                                mMainHandle.sendEmptyMessage(MSG_SHOW_SCAMBLEDTEXT);
                            } else {
                                Log.d(TAG, "mMainHandle is null");
                            }
                            break;
                        default:
                            Log.i(TAG, "Unhandled state: " + state);
                            break;
                    }
                } else if (signal.equals("PlayerTimeshiftRecorderStatusChanged")) {
                    switch (playerGetTimeshiftRecorderState(data)) {
                        case "recording":
                            timeshiftRecorderState = RecorderState.RECORDING;
                            startPosition = /*System.currentTimeMillis()*/PropSettingManager.getCurrentStreamTime(true);
                            originalStartPosition = PropSettingManager.getCurrentStreamTime(false);//keep the original time
                            Log.i(TAG, "recording originalStartPosition as date = " + ConvertSettingManager.convertLongToDate(originalStartPosition) + ", startPosition = " + ConvertSettingManager.convertLongToDate(startPosition));
                            notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_AVAILABLE);
                            break;
                        case "off":
                            timeshiftRecorderState = RecorderState.STOPPED;
                            startPosition = originalStartPosition = TvInputManager.TIME_SHIFT_INVALID_TIME;
                            timeshifting = false;
                            notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
                            break;
                    }
                } else if (signal.equals("RecordingStatusChanged")) {
                    JSONArray activeRecordings = recordingGetActiveRecordings(data);
                    if (activeRecordings != null && activeRecordings.length() < numRecorders &&
                            timeshiftRecorderState == RecorderState.STOPPED && scheduleTimeshiftRecording) {
                        timeshiftAvailable = true;
                        /*no scheduling, taken over by MSG_CHECK_REC_PATH*/
                        //scheduleTimeshiftRecordingTask();
                    }
                }
                else if (signal.equals("DvbUpdatedEventPeriods"))
                {
                    Log.i(TAG, "DvbUpdatedEventPeriods");
                    ComponentName sync = new ComponentName(mContext, DtvkitEpgSync.class);
                    EpgSyncJobService.requestImmediateSync(mContext, mInputId, false, sync);
                }
                else if (signal.equals("DvbUpdatedEventNow"))
                {
                    Log.i(TAG, "DvbUpdatedEventNow");
                    ComponentName sync = new ComponentName(mContext, DtvkitEpgSync.class);
                    EpgSyncJobService.requestImmediateSync(mContext, mInputId, true, sync);
                    //notify update parent contrl
                    if (mHandlerThreadHandle != null) {
                        mHandlerThreadHandle.removeMessages(MSG_CHECK_PARENTAL_CONTROL);
                        mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_PARENTAL_CONTROL, MSG_CHECK_PARENTAL_CONTROL_PERIOD);
                    }
                }
                else if (signal.equals("DvbUpdatedChannel"))
                {
                    Log.i(TAG, "DvbUpdatedChannel");
                    ComponentName sync = new ComponentName(mContext, DtvkitEpgSync.class);
                    EpgSyncJobService.requestImmediateSync(mContext, mInputId, false, true, sync);
                }
                else if (signal.equals("DvbNetworkChange") || signal.equals("DvbUpdatedService"))
                {
                    Log.i(TAG, "DvbNetworkChange or DvbUpdatedService");
                    if (!mDvbNetworkChangeSearchStatus) {
                        mDvbNetworkChangeSearchStatus = true;
                        sendDoReleaseMessage();
                        if (mHandlerThreadHandle != null) {
                            mHandlerThreadHandle.removeMessages(MSG_SEND_DISPLAY_STREAM_CHANGE_DIALOG);
                            mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_SEND_DISPLAY_STREAM_CHANGE_DIALOG, MSG_SEND_DISPLAY_STREAM_CHANGE_DIALOG);
                        }
                    }
                }
                else if (signal.equals("DvbUpdatedChannelData"))
                {
                    Log.i(TAG, "DvbUpdatedChannelData");
                    List<TvTrackInfo> tracks = playerGetTracks(mTunedChannel, false);
                    if (!tracks.equals(mTunedTracks)) {
                        mTunedTracks = tracks;
                        notifyTracksChanged(mTunedTracks);
                    }
                    Log.i(TAG, "audio track selected: " + playerGetSelectedAudioTrack());
                    notifyTrackSelected(TvTrackInfo.TYPE_AUDIO, Integer.toString(playerGetSelectedAudioTrack()));
                    initSubtitleOrTeletextIfNeed();
                    //check trackinfo update
                    if (mHandlerThreadHandle != null) {
                        //mHandlerThreadHandle.removeMessages(MSG_UPDATE_TRACKINFO);
                        mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_UPDATE_TRACKINFO, MSG_UPDATE_TRACKINFO_DELAY);
                    }
                }
                else if (signal.equals("MhegAppStarted"))
                {
                   Log.i(TAG, "MhegAppStarted");
                   notifyVideoAvailable();
                }
                else if (signal.equals("AppVideoPosition"))
                {
                   Log.i(TAG, "AppVideoPosition");
                   SystemControlManager SysContManager = SystemControlManager.getInstance();
                   int UiSettingMode = SysContManager.GetDisplayMode(SystemControlManager.SourceInput.XXXX.toInt());
                   int left,top,right,bottom;
                   left = 0;
                   top = 0;
                   right = 1920;
                   bottom = 1080;
                   int voff0, hoff0, voff1, hoff1;
                   voff0 = 0;
                   hoff0 = 0;
                   voff1 = 0;
                   hoff1 = 0;
                   try {
                      left = data.getInt("left");
                      top = data.getInt("top");
                      right = data.getInt("right");
                      bottom = data.getInt("bottom");
                      voff0 = data.getInt("crop-voff0");
                      hoff0 = data.getInt("crop-hoff0");
                      voff1 = data.getInt("crop-voff1");
                      hoff1 = data.getInt("crop-hoff1");
                   } catch (JSONException e) {
                      Log.e(TAG, e.getMessage());
                   }
                   if (mHandlerThreadHandle != null) {
                       mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_RESOLUTION, MSG_CHECK_RESOLUTION_PERIOD);
                   }
                   if (mIsMain) {
                       String crop = new StringBuilder()
                           .append(voff0).append(" ")
                           .append(hoff0).append(" ")
                           .append(voff1).append(" ")
                           .append(hoff1).toString();

                           if (UiSettingMode != 6) {//not afd
                               Log.i(TAG, "Not AFD mode!");
                           } else {
                               SysContManager.writeSysFs("/sys/class/video/crop", crop);
                           }
                       layoutSurface(left,top,right,bottom);
                       m_surface_left = left;
                       m_surface_right = right;
                       m_surface_top = top;
                       m_surface_bottom = bottom;
                       if (mHandlerThreadHandle != null) {
                          mHandlerThreadHandle.removeMessages(MSG_ENABLE_VIDEO);
                          mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_ENABLE_VIDEO, 40);
                       }
                   }
                }
                else if (signal.equals("ServiceRetuned"))
                {
                   String dvbUri = "";
                   Channel channel;
                   Uri retuneUri;
                   boolean found = false;
                   int i;
                   long id=0;
                   try {
                      dvbUri= data.getString("uri");
                   } catch (JSONException ignore) {
                   }
                   Log.i(TAG, "ServiceRetuned " + dvbUri);
                   //find the channel URI that matches the dvb uri of the retune
                   for (i = 0;i < mChannels.size();i++)
                   {
                      channel = mChannels.get(mChannels.keyAt(i));
                      if (dvbUri.equals(getChannelInternalDvbUri(channel))) {
                         found = true;
                         id = mChannels.keyAt(i);
                         break;
                      }
                   }
                   if (found)
                   {
                      //rebuild the Channel URI from the current channel + the new ID
                      retuneUri = Uri.parse("content://android.media.tv/channel");
                      retuneUri = ContentUris.withAppendedId(retuneUri,id);
                      Log.i(TAG, "Retuning to " + retuneUri);

                      if (mHandlerThreadHandle != null)
                          mHandlerThreadHandle.obtainMessage(MSG_ON_TUNE, 1/*mhegTune*/, 0, retuneUri).sendToTarget();
                   }
                   else
                   {
                      //if we couldn't find the channel uri for some reason,
                      // try restarting MHEG on the new service anyway
                      mhegSuspend();
                      mhegStartService(dvbUri);
                   }
                }
                else if (signal.equals("RecordingDiskFull"))
                {
                    /*free disk space excceds the property's setting*/
                }
                else if (signal.equals("tt_mix_separate"))
                {
                    mHandlerThreadHandle.sendEmptyMessage(MSG_SET_TELETEXT_MIX_SEPARATE);
                }
                else if (signal.equals("tt_mix_normal"))
                {
                    mHandlerThreadHandle.sendEmptyMessage(MSG_SET_TELETEXT_MIX_NORMAL);
                }
            }
        };

        protected static final int MSG_ON_TUNE = 1;
        protected static final int MSG_CHECK_RESOLUTION = 2;
        protected static final int MSG_CHECK_PARENTAL_CONTROL = 3;
        protected static final int MSG_BLOCK_MUTE_OR_UNMUTE = 4;
        protected static final int MSG_SET_SURFACE = 5;
        protected static final int MSG_DO_RELEASE = 6;
        protected static final int MSG_RELEASE_WORK_THREAD = 7;
        protected static final int MSG_GET_SIGNAL_STRENGTH = 8;
        protected static final int MSG_UPDATE_TRACKINFO = 9;
        protected static final int MSG_ENABLE_VIDEO = 10;
        protected static final int MSG_SET_UNBLOCK = 11;
        protected static final int MSG_SET_TELETEXT_MIX_NORMAL = 12;
        protected static final int MSG_SET_TELETEXT_MIX_SEPARATE = 13;
        protected static final int MSG_CHECK_REC_PATH = 14;
        protected static final int MSG_SEND_DISPLAY_STREAM_CHANGE_DIALOG = 15;
        protected static final int MSG_TRY_START_TIMESHIFT = 16;

        //audio ad
        public static final int MSG_MIX_AD_DUAL_SUPPORT = 20;
        public static final int MSG_MIX_AD_MIX_SUPPORT = 21;
        public static final int MSG_MIX_AD_MIX_LEVEL = 22;
        public static final int MSG_MIX_AD_SET_MAIN = 23;
        public static final int MSG_MIX_AD_SET_ASSOCIATE = 24;

        protected static final int MSG_CHECK_RESOLUTION_PERIOD = 1000;//MS
        protected static final int MSG_UPDATE_TRACKINFO_DELAY = 2000;//MS
        protected static final int MSG_CHECK_PARENTAL_CONTROL_PERIOD = 2000;//MS
        protected static final int MSG_BLOCK_MUTE_OR_UNMUTE_PERIOD = 100;//MS
        protected static final int MSG_GET_SIGNAL_STRENGTH_PERIOD = 1000;//MS
        protected static final int MSG_CHECK_REC_PATH_PERIOD = 1000;//MS
        protected static final int MSG_SHOW_STREAM_CHANGE_DELAY = 500;//MS

        protected static final int MSG_MAIN_HANDLE_DESTROY_OVERLAY = 1;
        protected static final int MSG_SHOW_SCAMBLEDTEXT = 2;
        protected static final int MSG_HIDE_SCAMBLEDTEXT = 3;
        protected static final int MSG_DISPLAY_STREAM_CHANGE_DIALOG = 4;

        protected void initWorkThread() {
            Log.d(TAG, "initWorkThread");
            mHandlerThreadHandle = new Handler(mHandlerThread.getLooper(), new Handler.Callback() {
                @Override
                public boolean handleMessage(Message msg) {
                    Log.d(TAG, "mHandlerThreadHandle [[[:" + msg.what + ", sessionIndex = " + mCurrentDtvkitTvInputSessionIndex);
                    switch (msg.what) {
                        case MSG_ON_TUNE:
                            Uri channelUri = (Uri)msg.obj;
                            boolean mhegTune = msg.arg1 == 0 ? false : true;
                            if (channelUri != null) {
                                onTuneByHandlerThreadHandle(channelUri, mhegTune);
                            }
                            break;
                        case MSG_CHECK_RESOLUTION:
                            if (!checkRealTimeResolution()) {
                                if (mHandlerThreadHandle != null)
                                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_RESOLUTION, MSG_CHECK_RESOLUTION_PERIOD);
                            }
                            break;
                        case MSG_CHECK_PARENTAL_CONTROL:
                            updateParentalControlExt();
                            if (mHandlerThreadHandle != null) {
                                mHandlerThreadHandle.removeMessages(MSG_CHECK_PARENTAL_CONTROL);
                                mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_PARENTAL_CONTROL, MSG_CHECK_PARENTAL_CONTROL_PERIOD);
                            }
                            break;
                        case MSG_BLOCK_MUTE_OR_UNMUTE:
                            boolean mute = msg.arg1 == 0 ? false : true;
                            setBlockMute(mute);
                            break;
                        case MSG_SET_SURFACE:
                            doSetSurface((Map<String, Object>)msg.obj);
                            break;
                        case MSG_DO_RELEASE:
                            doRelease();
                            break;
                        case MSG_RELEASE_WORK_THREAD:
                            finalReleaseInThread();
                            break;
                        case MSG_GET_SIGNAL_STRENGTH:
                            sendCurrentSignalInfomation();
                            if (mHandlerThreadHandle != null) {
                                mHandlerThreadHandle.removeMessages(MSG_GET_SIGNAL_STRENGTH);
                                mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_GET_SIGNAL_STRENGTH, MSG_GET_SIGNAL_STRENGTH_PERIOD);//check signal per 1s
                            }
                            break;
                        case MSG_UPDATE_TRACKINFO:
                            if (!checkTrackInfoUpdate()) {
                                if (mHandlerThreadHandle != null) {
                                    mHandlerThreadHandle.removeMessages(MSG_UPDATE_TRACKINFO);
                                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_UPDATE_TRACKINFO, MSG_UPDATE_TRACKINFO_DELAY);
                                }
                            }
                            break;
                        case MSG_ENABLE_VIDEO:
                            if (null != mSysSettingManager)
                                mSysSettingManager.writeSysFs("/sys/class/video/video_global_output", "1");
                            break;
                        case MSG_SET_UNBLOCK:
                            if (msg.obj != null && msg.obj instanceof TvContentRating) {
                                setUnBlock((TvContentRating)msg.obj);
                            }
                            break;
                        case MSG_SET_TELETEXT_MIX_NORMAL:
                            mView.setTeletextMix(false);
                            layoutSurface(m_surface_left,m_surface_top,m_surface_right,m_surface_bottom);
                            break;
                        case MSG_SET_TELETEXT_MIX_SEPARATE:
                            mView.setTeletextMix(true);
                            layoutSurface(m_surface_left,m_surface_bottom/3,m_surface_right/2,m_surface_bottom/3*2);
                            break;
                        case MSG_CHECK_REC_PATH:
                            if (resetRecordingPath()) {
                                /* path changed */
                                tryStartTimeshifting();
                            }
                            if (mHandlerThreadHandle != null) {
                                mHandlerThreadHandle.removeMessages(MSG_CHECK_REC_PATH);
                                mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_REC_PATH, MSG_CHECK_REC_PATH_PERIOD);
                            }
                            break;
                        case MSG_SEND_DISPLAY_STREAM_CHANGE_DIALOG:
                            if (mMainHandle != null) {
                                mMainHandle.removeMessages(MSG_DISPLAY_STREAM_CHANGE_DIALOG);
                                mMainHandle.sendEmptyMessage(MSG_DISPLAY_STREAM_CHANGE_DIALOG);
                            }
                            break;
                        case MSG_TRY_START_TIMESHIFT:
                            tryStartTimeshifting();
                            break;
                        default:
                            Log.d(TAG, "mHandlerThreadHandle initWorkThread default");
                            break;
                    }
                    Log.d(TAG, "mHandlerThreadHandle    " + msg.what + ", sessionIndex" + mCurrentDtvkitTvInputSessionIndex + "done]]]");
                    return true;
                }
            });
            mMainHandle = new MainHandler();
        }

        private void syncParentControlSetting() {
            int current_age, setting_min_age;

            current_age = getParentalControlAge();
            setting_min_age = getCurrentMinAgeByBlockedRatings();
            if (setting_min_age == 0xFF && getParentalControlOn()) {
                setParentalControlOn(false);
                notifyContentAllowed();
            }else if (setting_min_age >= 4 && setting_min_age <= 18 && current_age != setting_min_age) {
                setParentalControlAge(setting_min_age);
                if (current_age < setting_min_age) {
                    Log.e(TAG, "rating changed, oldAge < newAge : [" +current_age+ " < " +setting_min_age+ "], so will allow");
                    notifyContentAllowed();
                }
            }
        }

        private int getContentRatingsOfCurrentProgram() {
            int age = 0;
            String pc_rating;
            String rating_system;
            long currentStreamTime = 0;
            TvContentRating[] ratings;

            currentStreamTime = PropSettingManager.getCurrentStreamTime(true);
            if (currentStreamTime == 0)
                return 0;
            Log.d(TAG, "currentStreamTime:("+currentStreamTime+")");
            Program program = TvContractUtils.getCurrentProgramExt(mContext.getContentResolver(), TvContract.buildChannelUri(mTunedChannel.getId()), currentStreamTime);

            ratings = program == null ? null : program.getContentRatings();
            if (ratings != null)
            {
               Log.d(TAG, "ratings:["+ratings[0].flattenToString()+"]");
               pc_rating = ratings[0].getMainRating();
               rating_system = ratings[0].getRatingSystem();
               if (rating_system.equals("DVB"))
               {
                   String[] ageArry = pc_rating.split("_", 2);
                   if (ageArry[0].equals("DVB"))
                   {
                       age = Integer.valueOf(ageArry[1]);
                   }
               }
            }

            return age;
        }

        private int getContentRatingsOfCurrentPlayingRecordedProgram() {
            int age = 0;
            String pc_rating;
            String rating_system;
            long currentStreamTime = 0;
            TvContentRating[] ratings;

            Log.d(TAG, "getContentRatingsOfCurrentPlayingRecordedProgram = " + recordedProgram);

            ratings = recordedProgram == null ? null : recordedProgram.getContentRatings();
            if (ratings != null)
            {
               Log.d(TAG, "ratings:["+ratings[0].flattenToString()+"]");
               pc_rating = ratings[0].getMainRating();
               rating_system = ratings[0].getRatingSystem();
               if (rating_system.equals("DVB"))
               {
                   String[] ageArry = pc_rating.split("_", 2);
                   if (ageArry[0].equals("DVB"))
                   {
                       age = Integer.valueOf(ageArry[1]);
                   }
               }
            }

            return age;
        }

        private void updateParentalControlExt() {
            int age = 0;
            int rating;
            boolean isParentControlEnabled = mTvInputManager.isParentalControlsEnabled();
            Log.d(TAG, "updateParentalControlExt isParentControlEnabled = " + isParentControlEnabled + ", mKeyUnlocked = " + mKeyUnlocked);
            if (isParentControlEnabled && !mKeyUnlocked) {
                try {
                    JSONArray args = new JSONArray();
                    rating = getCurrentMinAgeByBlockedRatings();
                    age = recordedProgram == null ? getContentRatingsOfCurrentProgram() : getContentRatingsOfCurrentPlayingRecordedProgram();
                    Log.e(TAG, "updateParentalControlExt current program age["+ age +"] setting_rating[" +rating+ "]");
                    if ((rating < 4 || rating > 18 || age < rating) && mBlocked)
                    {
                        notifyContentAllowed();
                        setBlockMute(false);
                        mBlocked = false;
                    }
                    else if (rating >= 4 && rating <= 18 && age >= rating)
                    {
                        String Rating = "";
                        Rating = String.format("DVB_%d", age);
                        notifyContentBlocked(TvContentRating.createRating("com.android.tv", "DVB", Rating));
                        if (!mBlocked)
                        {
                            setBlockMute(true);
                            mBlocked = true;
                        }
                    }
                } catch (Exception e) {
                    Log.e(TAG, "updateParentalControlExt = " + e.getMessage());
                }
            }
            else if (mBlocked)
            {
                notifyContentAllowed();
                setBlockMute(false);
                mBlocked = false;
            }
        }

        private void updateParentalControl() {
            int age = 0;
            int rating;
            int pc_age;
            boolean isParentControlEnabled = mTvInputManager.isParentalControlsEnabled();
            if (isParentControlEnabled) {
                try {
                    JSONArray args = new JSONArray();
                    //age = DtvkitGlueClient.getInstance().request("Player.getCurrentProgramRatingAge", args).getInt("data");
                    rating = getCurrentMinAgeByBlockedRatings();
                    pc_age = getParentalControlAge();
                    age = recordedProgram == null ? getContentRatingsOfCurrentProgram() : getContentRatingsOfCurrentPlayingRecordedProgram();
                    Log.e(TAG, "updateParentalControl current program age["+ age +"] setting_rating[" +rating+ "] pc_age[" +pc_age+ "]");
                    if (getParentalControlOn()) {
                        if (rating < 4 || rating > 18 || age == 0) {
                            setParentalControlOn(false);
                            notifyContentAllowed();
                        }else if (rating >= 4 && rating <= 18 && age >= rating) {
                            if (pc_age != rating)
                                setParentalControlAge(rating);
                            }
                    }else {
                        if (rating >= 4 && rating <= 18 && age != 0) {
                            Log.e(TAG, "P_C false, but age isn't 0, so set P_C enbale rating:" + rating);
                            if (pc_age != rating || age >= rating) {
                                setParentalControlOn(true);
                                setParentalControlAge(rating);
                            }
                        }
                    }
                } catch (Exception e) {
                    Log.e(TAG, "updateParentalControl = " + e.getMessage());
                }
            }
        }

        private void setBlockMute(boolean mute) {
            Log.d(TAG, "setBlockMute = " + mute + ", index = " + mCurrentDtvkitTvInputSessionIndex);
            AudioManager audioManager = null;
            if (mContext != null) {
                audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            }
            if (audioManager != null) {
                Log.d(TAG, "setBlockMute = " + mute);
                if (mute) {
                    audioManager.setParameters("parental_control_av_mute=true");
                } else {
                    audioManager.setParameters("parental_control_av_mute=false");
                }
            } else {
                Log.i(TAG, "setBlockMute can't get audioManager");
            }
        }

        private void setUnBlock(TvContentRating unblockedRating) {
            Log.i(TAG, "setUnBlock " + unblockedRating);
            mKeyUnlocked = true;
            mBlocked = false;
            setParentalControlOn(false);
            notifyContentAllowed();
            setBlockMute(false);
        }

        private boolean playerInitAssociateDualSupport() {
            boolean result = false;
            //mAudioADAutoStart = mDataMananer.getIntParameters(DataMananer.TV_KEY_AD_SWITCH) == 1;
            mAudioADMixingLevel = mDataMananer.getIntParameters(DataMananer.TV_KEY_AD_MIX);
            boolean adOn = playergetAudioDescriptionOn();
            Log.d(TAG, "playerInitAssociateDualSupport mAudioADAutoStart = " + mAudioADAutoStart + ", mAudioADMixingLevel = " + mAudioADMixingLevel);
            if (mAudioADAutoStart) {
                //setAdFunction(MSG_MIX_AD_DUAL_SUPPORT, 1);
                //setAdFunction(MSG_MIX_AD_MIX_SUPPORT, 1);
                //setAdFunction(MSG_MIX_AD_MIX_LEVEL, mAudioADMixingLevel);
                //if (!adOn) {
                    //setAdFunction(MSG_MIX_AD_MIX_LEVEL, mAudioADMixingLevel);
                    //setAdFunction(MSG_MIX_AD_SET_ASSOCIATE, 1);
                //}
            } else {
                //setAdFunction(MSG_MIX_AD_MIX_SUPPORT, 0);
                //setAdFunction(MSG_MIX_AD_DUAL_SUPPORT, 0);
                //if (adOn) {
                    //setAdFunction(MSG_MIX_AD_SET_ASSOCIATE, 0);
                //}
            }
            result = true;
            return result;
        }

        private boolean playerResetAssociateDualSupport() {
            boolean result = false;
            setAdFunction(MSG_MIX_AD_SET_ASSOCIATE, 0);
            //setAdFunction(MSG_MIX_AD_MIX_SUPPORT, 0);
            //setAdFunction(MSG_MIX_AD_DUAL_SUPPORT, 0);
            result = true;
            return result;
        }

        private class MainHandler extends Handler {
            public void handleMessage(Message msg) {
                Log.d(TAG, "MainHandler [[[:" + msg.what + ", sessionIndex = " + mCurrentDtvkitTvInputSessionIndex);
                switch (msg.what) {
                    case MSG_MAIN_HANDLE_DESTROY_OVERLAY:
                        doDestroyOverlay();
                        if (mHandlerThreadHandle != null) {
                            mHandlerThreadHandle.sendEmptyMessage(MSG_RELEASE_WORK_THREAD);
                        }
                        break;
                    case MSG_SHOW_SCAMBLEDTEXT:
                        if (mView != null) {
                            mView.showScrambledText("Scambled");
                        }
                        break;
                    case MSG_HIDE_SCAMBLEDTEXT:
                        if (mView != null) {
                            mView.hideScrambledText();
                        }
                        break;
                    case MSG_DISPLAY_STREAM_CHANGE_DIALOG:
                        if (mHandlerThreadHandle != null) {
                            mHandlerThreadHandle.removeCallbacksAndMessages(null);
                        }
                        showSearchConfirmDialog(DtvkitTvInput.this, mTunedChannel);
                        break;
                    default:
                        Log.d(TAG, "MainHandler default");
                        break;
                }
                Log.d(TAG, "MainHandler    " + msg.what + ", sessionIndex = " + mCurrentDtvkitTvInputSessionIndex + "done]]]");
            }
        }

        private void finalReleaseInThread() {
            Log.d(TAG, "finalReleaseInThread index = " + mCurrentDtvkitTvInputSessionIndex);
            new Thread(new Runnable() {
                @Override
                public void run() {
                    Log.d(TAG, "finalReleaseInThread start");
                    finalReleaseWorkThread();
                    Log.d(TAG, "finalReleaseInThread end");
                }
            }).start();
        }

        private synchronized void finalReleaseWorkThread() {
            Log.d(TAG, "finalReleaseWorkThread index = " + mCurrentDtvkitTvInputSessionIndex);
            if (mMainHandle != null) {
                mMainHandle.removeCallbacksAndMessages(null);
            }
            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeCallbacksAndMessages(null);
            }
            mMainHandle = null;
            mHandlerThreadHandle = null;
            Log.d(TAG, "finalReleaseWorkThread over");
        }

        protected boolean onTuneByHandlerThreadHandle(Uri channelUri, boolean mhegTune) {
            Log.i(TAG, "onTuneByHandlerThreadHandle " + channelUri + "index = " + mCurrentDtvkitTvInputSessionIndex);
            if (ContentUris.parseId(channelUri) == -1) {
                Log.e(TAG, "onTuneByHandlerThreadHandle invalid channelUri = " + channelUri);
                return false;
            }
            removeScheduleTimeshiftRecordingTask();
            if (timeshiftRecorderState != RecorderState.STOPPED) {
                Log.i(TAG, "reset timeshiftState to STOPPED.");
                timeshiftRecorderState = RecorderState.STOPPED;
                timeshifting = false;
                scheduleTimeshiftRecording = false;
                playerStopTimeshiftRecording(false);
            }

            mTunedChannel = getChannel(channelUri);
            final String dvbUri = getChannelInternalDvbUri(mTunedChannel);

            if (mhegTune) {
                mhegSuspend();
                if (mhegGetNextTuneInfo(dvbUri) == 0)
                    notifyChannelRetuned(channelUri);
            } else {
                mhegStop();
            }

            if (null != mSysSettingManager)
                mSysSettingManager.writeSysFs("/sys/class/video/video_global_output", "0");

            notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING);
            playerStopTeletext();//no need to save teletext select status
            playerStop();
            playerSetSubtitlesOn(false);
            playerSetTeletextOn(false, -1);
            setParentalControlOn(false);
            //playerResetAssociateDualSupport();
            mKeyUnlocked = false;
            mDvbNetworkChangeSearchStatus = false;
            if (mBlocked)
            {
                notifyContentAllowed();
                setBlockMute(false);
                mBlocked = false;
            }
            if (mTvInputManager != null) {
                boolean parentControlSwitch = mTvInputManager.isParentalControlsEnabled();
                if (parentControlSwitch)
                {
                    updateParentalControlExt();
                }
                /*boolean parentControlStatus = getParentalControlOn();
                if (parentControlSwitch != parentControlStatus) {
                    setParentalControlOn(parentControlSwitch);
                }
                if (parentControlSwitch) {
                    syncParentControlSetting();
                }*/
            }
            mAudioADAutoStart = mDataMananer.getIntParameters(DataMananer.TV_KEY_AD_SWITCH) == 1;
            DtvkitGlueClient.getInstance().registerSignalHandler(mHandler);
            if (playerPlay(dvbUri, mAudioADAutoStart).equals("ok")) {
                if (mHandlerThreadHandle != null) {
                    mHandlerThreadHandle.removeMessages(MSG_CHECK_PARENTAL_CONTROL);
                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_PARENTAL_CONTROL, MSG_CHECK_PARENTAL_CONTROL_PERIOD);
                }
                DtvkitGlueClient.getInstance().setAudioHandler(AHandler);
                /*
                if (mCaptioningManager != null && mCaptioningManager.isEnabled()) {
                    playerSetSubtitlesOn(true);
                }
                */
                playerInitAssociateDualSupport();
            } else {
                mTunedChannel = null;
                notifyVideoUnavailable(TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN);
                notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);

                Log.e(TAG, "No play path available");
                Bundle event = new Bundle();
                event.putString(ConstantManager.KEY_INFO, "No play path available");
                notifySessionEvent(ConstantManager.EVENT_RESOURCE_BUSY, event);
                DtvkitGlueClient.getInstance().unregisterSignalHandler(mHandler);
            }
            Log.i(TAG, "onTuneByHandlerThreadHandle Done");
            return mTunedChannel != null;
        }

        private boolean checkRealTimeResolution() {
            boolean result = false;
            if (mTunedChannel == null) {
                return true;
            }
            String serviceType = mTunedChannel.getServiceType();
            //update video track resolution
            if (TvContract.Channels.SERVICE_TYPE_AUDIO_VIDEO.equals(serviceType)) {
                int[] videoSize = playerGetDTVKitVideoSize();
                String realtimeVideoFormat = mSysSettingManager.getVideoFormatFromSys();
                result = !TextUtils.isEmpty(realtimeVideoFormat);
                if (result) {
                    Bundle formatbundle = new Bundle();
                    formatbundle.putString(ConstantManager.PI_FORMAT_KEY, realtimeVideoFormat);
                    notifySessionEvent(ConstantManager.EVENT_STREAM_PI_FORMAT, formatbundle);
                    Log.d(TAG, "checkRealTimeResolution notify realtimeVideoFormat = " + realtimeVideoFormat + ", videoSize width = " + videoSize[0] + ", height = " + videoSize[1]);
                }
            }
            return result;
        }

        private boolean checkTrackInfoUpdate() {
            boolean result = false;
            if (mTunedChannel == null || mTunedTracks == null) {
                Log.d(TAG, "checkTrackinfoUpdate no need");
                return result;
            }
            List<TvTrackInfo> tracks = playerGetTracks(mTunedChannel, true);
            boolean needCheckAgain = false;
            if (tracks != null && tracks.size() > 0) {
                for (TvTrackInfo temp : tracks) {
                    int type = temp.getType();
                    switch (type) {
                        case TvTrackInfo.TYPE_AUDIO:
                            String currentAudioId = Integer.toString(playerGetSelectedAudioTrack());
                            if (TextUtils.equals(currentAudioId, temp.getId())) {
                                if (temp.getAudioSampleRate() == 0 || temp.getAudioChannelCount() == 0) {
                                    Log.d(TAG, "checkTrackinfoUpdate audio need check");
                                    needCheckAgain = true;
                                }
                            }
                            break;
                        case TvTrackInfo.TYPE_VIDEO:
                            if (temp.getVideoWidth() == 0 || temp.getVideoHeight() == 0 || temp.getVideoFrameRate() == 0f) {
                                Log.d(TAG, "checkTrackinfoUpdate video need check");
                                needCheckAgain = true;
                            }
                            break;
                        default:
                            break;
                    }
                }
            }
            if (tracks != null && tracks.size() > 0 && mTunedTracks != null && mTunedTracks.size() > 0) {
                if (!Objects.equals(mTunedTracks, tracks)) {
                    mTunedTracks = tracks;
                    notifyTracksChanged(mTunedTracks);
                    Log.d(TAG, "checkTrackinfoUpdate update new mTunedTracks");
                    result = true;
                }
            }
            if (needCheckAgain) {
                result = false;
            }
            return result;
        }

        private boolean sendCurrentSignalInfomation() {
            boolean result = false;
            if (mTunedChannel == null) {
                return result;
            }
            int[] signalInfo = getSignalStatus();
            if (true/*mSignalStrength != signalInfo[0] || mSignalQuality != signalInfo[1]*/) {
                mSignalStrength = signalInfo[0];
                mSignalQuality = signalInfo[1];
                Bundle signalbundle = new Bundle();
                signalbundle.putInt(ConstantManager.KEY_SIGNAL_STRENGTH, signalInfo[0]);
                signalbundle.putInt(ConstantManager.KEY_SIGNAL_QUALITY, signalInfo[1]);
                notifySessionEvent(ConstantManager.EVENT_SIGNAL_INFO, signalbundle);
                result = true;
                Log.d(TAG, "sendCurrentSignalInfomation notify signalStrength = " + signalInfo[0] + ", signalQuality = " + signalInfo[1]);
            }
            return result;
        }

        private boolean setAdFunction(int msg, int param1) {
            boolean result = false;
            AudioManager audioManager = null;
            if (mContext != null) {
                audioManager = (AudioManager) mContext.getSystemService(Context.AUDIO_SERVICE);
            }
            if (audioManager == null) {
                Log.i(TAG, "setAdFunction null audioManager");
                return result;
            }
            //Log.d(TAG, "setAdFunction msg = " + msg + ", param1 = " + param1);
            switch (msg) {
                case MSG_MIX_AD_DUAL_SUPPORT://dual_decoder_surport for ad & main mix on/off
                    if (param1 > 0) {
                        audioManager.setParameters("dual_decoder_support=1");
                    } else {
                        audioManager.setParameters("dual_decoder_support=0");
                    }
                    Log.d(TAG, "setAdFunction MSG_MIX_AD_DUAL_SUPPORT setParameters:"
                            + "dual_decoder_support=" + (param1 > 0 ? 1 : 0));
                    result = true;
                    break;
                case MSG_MIX_AD_MIX_SUPPORT://Associated audio mixing on/off
                    if (param1 > 0) {
                        audioManager.setParameters("associate_audio_mixing_enable=1");
                    } else {
                        audioManager.setParameters("associate_audio_mixing_enable=0");
                    }
                    Log.d(TAG, "setAdFunction MSG_MIX_AD_MIX_SUPPORT setParameters:"
                            + "associate_audio_mixing_enable=" + (param1 > 0 ? 1 : 0));
                    result = true;
                    break;
                case MSG_MIX_AD_MIX_LEVEL://Associated audio mixing level
                    audioManager.setParameters("dual_decoder_mixing_level=" + param1 + "");
                    Log.d(TAG, "setAdFunction MSG_MIX_AD_MIX_LEVEL setParameters:"
                            + "dual_decoder_mixing_level=" + param1);
                    result = true;
                    break;
                case MSG_MIX_AD_SET_MAIN://set Main Audio by handle
                    result = playerSelectAudioTrack(param1);
                    Log.d(TAG, "setAdFunction MSG_MIX_AD_SET_MAIN result=" + result
                            + ", setAudioStream " + param1);
                    break;
                case MSG_MIX_AD_SET_ASSOCIATE://set Associate Audio by handle
                    result = playersetAudioDescriptionOn(param1 == 1);
                    Log.d(TAG, "setAdFunction MSG_MIX_AD_SET_ASSOCIATE result=" + result
                            + "setAudioDescriptionOn " + (param1 == 1));
                    break;
                default:
                    Log.i(TAG,"setAdFunction unkown  msg = " + msg + ", param1 = " + param1);
                    break;
            }
            return result;
        }

        private void tryStartTimeshifting() {
            if (getFeatureSupportTimeshifting()) {
                if (timeshiftRecorderState == RecorderState.STOPPED) {
                    numActiveRecordings = recordingGetNumActiveRecordings();
                    Log.i(TAG, "numActiveRecordings: " + numActiveRecordings);
                    if (recordingPending) {
                        numActiveRecordings += 1;
                        Log.i(TAG, "recordingPending: +1");
                    }
                    if (numActiveRecordings < numRecorders
                        && numActiveRecordings < getNumRecordersLimit()) {
                        timeshiftAvailable = true;
                    } else {
                        timeshiftAvailable = false;
                    }
                }
                Log.i(TAG, "timeshiftAvailable: " + timeshiftAvailable);
                Log.i(TAG, "timeshiftRecorderState: " + timeshiftRecorderState);
                if (timeshiftAvailable) {
                    if (timeshiftRecorderState == RecorderState.STOPPED) {
                        if (playerStartTimeshiftRecording()) {
                            /*
                              The onSignal callback may be triggerd before here,
                              and changes the state to a further value.
                              so check the state first, in order to prevent getting it reset.
                            */
                            if (timeshiftRecorderState != RecorderState.RECORDING) {
                                timeshiftRecorderState = RecorderState.STARTING;
                            }
                        } else {
                            notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
                        }
                    }
                }
            } else {
                notifyTimeShiftStatusChanged(TvInputManager.TIME_SHIFT_STATUS_UNAVAILABLE);
            }
        }

        private void monitorRecordingPath(boolean on) {
            /*monitor the rec path*/
            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeMessages(MSG_CHECK_REC_PATH);
                if (on)
                    mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_CHECK_REC_PATH, MSG_CHECK_REC_PATH_PERIOD);
            }
        }

        public void sendMsgTryStartTimeshift() {
            if (mHandlerThreadHandle != null) {
                mHandlerThreadHandle.removeMessages(MSG_TRY_START_TIMESHIFT);
                mHandlerThreadHandle.sendEmptyMessageDelayed(MSG_TRY_START_TIMESHIFT, 1000);
            }
        }
    }

    private boolean resetRecordingPath() {
        String newPath = mDataMananer.getStringParameters(DataMananer.KEY_PVR_RECORD_PATH);
        boolean changed = false;
        if (!SysSettingManager.isDeviceExist(newPath)) {
            Log.d(TAG, "removable device has been moved and use default path");
            newPath = DataMananer.PVR_DEFAULT_PATH;
            mDataMananer.saveStringParameters(DataMananer.KEY_PVR_RECORD_PATH, newPath);
            changed = true;
        }
        recordingAddDiskPath(newPath);
        recordingSetDefaultDisk(newPath);
        return changed;
    }

    private void onChannelsChanged() {
        mChannels = TvContractUtils.buildChannelMap(mContentResolver, mInputId);
    }

    private Channel getChannel(Uri channelUri) {
        if (mChannels != null) {
            return mChannels.get(ContentUris.parseId(channelUri));
        } else {
            return null;
        }
    }

    private String getChannelInternalDvbUri(Channel channel) {
        try {
            return channel.getInternalProviderData().get("dvbUri").toString();
        } catch (Exception e) {
            Log.e(TAG, "getChannelInternalDvbUri Exception = " + e.getMessage());
            return "dvb://0000.0000.0000";
        }
    }

    private String getProgramInternalDvbUri(Program program) {
        try {
            String uri = program.getInternalProviderData().get("dvbUri").toString();
            return uri;
        } catch (InternalProviderData.ParseException e) {
            Log.e(TAG, "getChannelInternalDvbUri ParseException = " + e.getMessage());
            return "dvb://current";
        }
    }

    private void playerSetVolume(int volume) {
        try {
            JSONArray args = new JSONArray();
            args.put(volume);
            DtvkitGlueClient.getInstance().request("Player.setVolume", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSetVolume = " + e.getMessage());
        }
    }

    private void playerSetMute(boolean mute) {
        try {
            JSONArray args = new JSONArray();
            args.put(mute);
            DtvkitGlueClient.getInstance().request("Player.setMute", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSetMute = " + e.getMessage());
        }
    }

    private void playerSetSubtitlesOn(boolean on) {
        try {
            JSONArray args = new JSONArray();
            args.put(on);
            DtvkitGlueClient.getInstance().request("Player.setSubtitlesOn", args);
            Log.i(TAG, "playerSetSubtitlesOn on =  " + on);
        } catch (Exception e) {
            Log.e(TAG, "playerSetSubtitlesOn=  " + e.getMessage());
        }
    }

    private String playerPlay(String dvbUri, boolean ad_enable) {
        try {
            JSONArray args = new JSONArray();
            args.put(dvbUri);
            args.put(ad_enable);
            AudioManager audioManager = (AudioManager)getSystemService(Context.AUDIO_SERVICE);
            audioManager.setParameters("tuner_in=dtv");
            Log.d(TAG, "dtv player.play: "+dvbUri);

            JSONObject resp = DtvkitGlueClient.getInstance().request("Player.play", args);
            boolean ok = resp.optBoolean("data");
            if (ok)
                return "ok";
            else
                return resp.optString("data", "");
        } catch (Exception e) {
            Log.e(TAG, "playerPlay = " + e.getMessage());
            return "unknown error";
        }
    }

    private void playerStop() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.stop", args);
        } catch (Exception e) {
            Log.e(TAG, "playerStop = " + e.getMessage());
        }
    }

    private boolean playerPause() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.pause", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerPause = " + e.getMessage());
            return false;
        }
    }

    private boolean playerResume() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.resume", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerResume = " + e.getMessage());
            return false;
        }
    }

    private void playerFastForward() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.fastForward", args);
        } catch (Exception e) {
            Log.e(TAG, "playerFastForwards" + e.getMessage());
        }
    }

    private void playerFastRewind() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.fastRewind", args);
        } catch (Exception e) {
            Log.e(TAG, "playerFastRewind = " + e.getMessage());
        }
    }

    private boolean playerSetSpeed(float speed) {
        try {
            JSONArray args = new JSONArray();
            args.put((long)(speed * 100.0));
            DtvkitGlueClient.getInstance().request("Player.setPlaySpeed", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerSetSpeed = " + e.getMessage());
            return false;
        }
    }

    private boolean playerSeekTo(long positionSecs) {
        try {
            JSONArray args = new JSONArray();
            args.put(positionSecs);
            DtvkitGlueClient.getInstance().request("Player.seekTo", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerSeekTo = " + e.getMessage());
            return false;
        }
    }

    private boolean playerStartTimeshiftRecording() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.startTimeshiftRecording", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerStartTimeshiftRecording = " + e.getMessage());
            return false;
        }
    }

    private boolean playerStopTimeshiftRecording(boolean returnToLive) {
        try {
            JSONArray args = new JSONArray();
            args.put(returnToLive);
            DtvkitGlueClient.getInstance().request("Player.stopTimeshiftRecording", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
            return false;
        }
    }

    private boolean playerPlayTimeshiftRecording(boolean startPlaybackPaused, boolean playFromCurrent) {
        try {
            JSONArray args = new JSONArray();
            args.put(startPlaybackPaused);
            args.put(playFromCurrent);
            DtvkitGlueClient.getInstance().request("Player.playTimeshiftRecording", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerStopTimeshiftRecording = " + e.getMessage());
            return false;
        }
    }

    private void playerSetRectangle(int x, int y, int width, int height) {
        try {
            JSONArray args = new JSONArray();
            args.put(x);
            args.put(y);
            args.put(width);
            args.put(height);
            DtvkitGlueClient.getInstance().request("Player.setRectangle", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSetRectangle = " + e.getMessage());
        }
    }

    private List<TvTrackInfo> playerGetTracks(Channel tunedChannel, boolean detailsAvailable) {
        List<TvTrackInfo> tracks = new ArrayList<>();
        tracks.addAll(getVideoTrackInfoList(tunedChannel, detailsAvailable));
        tracks.addAll(getAudioTrackInfoList(detailsAvailable));
        tracks.addAll(getSubtitleTrackInfoList());
        return tracks;
    }

    private List<TvTrackInfo> getVideoTrackInfoList(Channel tunedChannel, boolean detailsAvailable) {
        List<TvTrackInfo> tracks = new ArrayList<>();
        //video tracks
        try {
            TvTrackInfo.Builder track = new TvTrackInfo.Builder(TvTrackInfo.TYPE_VIDEO, Integer.toString(0));
            Bundle bundle = new Bundle();
            if (detailsAvailable) {
                //get values that need to get from sys and need wait for at least 1s after play
                String decodeInfo = mSysSettingManager.getVideodecodeInfo();
                int videoWidth = mSysSettingManager.parseWidthFromVdecStatus(decodeInfo);
                int videoHeight = mSysSettingManager.parseHeightFromVdecStatus(decodeInfo);
                float videoFrameRate = mSysSettingManager.parseFrameRateStrFromVdecStatus(decodeInfo);
                String videoFrameFormat = mSysSettingManager.parseFrameFormatStrFromDi0Path();
                //set value
                track.setVideoWidth(videoWidth);
                track.setVideoHeight(videoHeight);
                track.setVideoFrameRate(videoFrameRate);
                bundle.putInt(ConstantManager.KEY_TVINPUTINFO_VIDEO_WIDTH, videoWidth);
                bundle.putInt(ConstantManager.KEY_TVINPUTINFO_VIDEO_HEIGHT, videoHeight);
                bundle.putFloat(ConstantManager.KEY_TVINPUTINFO_VIDEO_FRAME_RATE, videoFrameRate);
                bundle.putString(ConstantManager.KEY_TVINPUTINFO_VIDEO_FRAME_FORMAT, videoFrameFormat != null ? videoFrameFormat : "");
                //video format framework example "VIDEO_FORMAT_360P" "VIDEO_FORMAT_576I"
                String videoFormat = tunedChannel != null ? tunedChannel.getVideoFormat() : "";
                if (tunedChannel != null) {
                    //update video format such as VIDEO_FORMAT_1080P VIDEO_FORMAT_1080I
                    String buildVideoFormat = "VIDEO_FORMAT_" + videoHeight + videoFrameFormat;
                    if (videoHeight > 0 && !TextUtils.equals(buildVideoFormat, tunedChannel.getVideoFormat())) {
                        videoFormat = buildVideoFormat;
                        bundle.putString(ConstantManager.KEY_TVINPUTINFO_VIDEO_FORMAT, videoFormat != null ? videoFormat : "");
                        TvContractUtils.updateSingleChannelColumn(DtvkitTvInput.this.getContentResolver(), tunedChannel.getId(), TvContract.Channels.COLUMN_VIDEO_FORMAT, buildVideoFormat);
                    }
                }
            }
            //get values from db
            String videoCodec = tunedChannel != null ? tunedChannel.getVideoCodec() : "";
            //set value
            bundle.putString(ConstantManager.KEY_TVINPUTINFO_VIDEO_CODEC, videoCodec != null ? videoCodec : "");
            track.setExtra(bundle);
            //buid track
            tracks.add(track.build());
            Log.d(TAG, "getVideoTrackInfoList track bundle = " + bundle.toString());
        } catch (Exception e) {
            Log.e(TAG, "getVideoTrackInfoList Exception = " + e.getMessage());
        }
        return tracks;
    }

    private List<TvTrackInfo> getAudioTrackInfoList(boolean detailsAvailable) {
        List<TvTrackInfo> tracks = new ArrayList<>();
        //audio tracks
        try {
            List<TvTrackInfo> audioTracks = new ArrayList<>();
            JSONArray args = new JSONArray();
            JSONArray audioStreams = DtvkitGlueClient.getInstance().request("Player.getListOfAudioStreams", args).getJSONArray("data");
            int undefinedIndex = 1;
            for (int i = 0; i < audioStreams.length(); i++)
            {
                JSONObject audioStream = audioStreams.getJSONObject(i);
                Log.d(TAG, "getAudioTrackInfoList audioStream = " + audioStream.toString());
                TvTrackInfo.Builder track = new TvTrackInfo.Builder(TvTrackInfo.TYPE_AUDIO, Integer.toString(audioStream.getInt("index")));
                Bundle bundle = new Bundle();
                String audioLang = audioStream.getString("language");
                if (TextUtils.isEmpty(audioLang) || ConstantManager.CONSTANT_UND_FLAG.equals(audioLang)) {
                    audioLang = ConstantManager.CONSTANT_UND_VALUE + undefinedIndex;
                    undefinedIndex++;
                } else if (ConstantManager.CONSTANT_QAA.equalsIgnoreCase(audioLang)) {
                    audioLang = ConstantManager.CONSTANT_ORIGINAL_AUDIO;
                } else if (ConstantManager.CONSTANT_QAD.equalsIgnoreCase(audioLang)) {
                    audioLang = ConstantManager.CONSTANT_FRENCH;
                }
                track.setLanguage(audioLang);
                bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_AUDIO_AD, false);
                if (audioStream.getBoolean("ad")) {
                    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
                        track.setDescription("AD");
                        bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_AUDIO_AD, true);
                    }
                }
                String codes = audioStream.getString("codec");
                int pid = audioStream.getInt("pid");
                if (!TextUtils.isEmpty(codes)) {
                    bundle.putString(ConstantManager.KEY_AUDIO_CODES_DES, codes);
                    bundle.putString(ConstantManager.KEY_TVINPUTINFO_AUDIO_CODEC, codes);
                } else {
                    bundle.putString(ConstantManager.KEY_TVINPUTINFO_AUDIO_CODEC, "");
                }
                bundle.putInt(ConstantManager.KEY_TRACK_PID, pid);
                if (detailsAvailable) {
                    //can only be gotten after decode finished
                    int sampleRate = getAudioSamplingRateFromAudioPatch(pid);
                    int audioChannel = getAudioChannelFromAudioPatch(pid);
                    bundle.putInt(ConstantManager.KEY_TVINPUTINFO_AUDIO_SAMPLING_RATE, sampleRate);
                    bundle.putInt(ConstantManager.KEY_TVINPUTINFO_AUDIO_CHANNEL, audioChannel);
                    bundle.putInt(ConstantManager.AUDIO_PATCH_COMMAND_GET_AUDIO_CHANNEL_CONFIGURE, getAudioChannelConfigureFromAudioPatch(pid));
                    track.setAudioChannelCount(audioChannel);
                    track.setAudioSampleRate(sampleRate);
                }
                bundle.putInt(ConstantManager.KEY_TVINPUTINFO_AUDIO_INDEX, audioStream.getInt("index"));
                track.setExtra(bundle);
                audioTracks.add(track.build());
                Log.d(TAG, "getAudioTrackInfoList track bundle = " + bundle.toString());
            }
            ConstantManager.ascendTrackInfoOderByPid(audioTracks);
            tracks.addAll(audioTracks);
        } catch (Exception e) {
            Log.e(TAG, "getAudioTrackInfoList Exception = " + e.getMessage());
        }
        return tracks;
    }

    private int getAudioSamplingRateFromAudioPatch(int audioPid) {
        int result = 0;
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        String temp = audioManager.getParameters(ConstantManager.AUDIO_PATCH_COMMAND_GET_SAMPLING_RATE);
        //sampling rate result "sample_rate=4800"
        //Log.d(TAG, "getAudioSamplingRateFromAudioPatch result = " + temp);
        try {
            if (temp != null) {
                String[] splitInfo = temp.split("=");
                if (splitInfo != null && splitInfo.length == 2 && "sample_rate".equals(splitInfo[0])) {
                    result = Integer.valueOf(splitInfo[1]);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "getAudioSamplingRateFromAudioPatch Exception = " + e.getMessage());
        }
        Log.d(TAG, "getAudioSamplingRateFromAudioPatch result = " + result);
        return result;
    }

    private int getAudioChannelFromAudioPatch(int audioPid) {
        int result = 0;
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        String temp = audioManager.getParameters(ConstantManager.AUDIO_PATCH_COMMAND_GET_AUDIO_CHANNEL);
        //channel num result "channel_nums=2"
        //Log.d(TAG, "getAudioChannelFromAudioPatch result = " + temp);
        try {
            if (temp != null) {
                String[] splitInfo = temp.split("=");
                if (splitInfo != null && splitInfo.length == 2 && "channel_nums".equals(splitInfo[0])) {
                    result = Integer.valueOf(splitInfo[1]);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "getAudioChannelFromAudioPatch Exception = " + e.getMessage());
        }
        Log.d(TAG, "getAudioChannelFromAudioPatch result = " + result);
        return result;
    }

    private int getAudioChannelConfigureFromAudioPatch(int audioPid) {
        int result = 0;
        //defines in audio patch
        //typedef enum {
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_UNKNOWN = 0,
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_C,/**< Center */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_MONO = TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_C,
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_R,/**< Left and right speakers */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_STEREO = TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_R,
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R,/**< Left, center and right speakers */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_R_S,/**< Left, right and surround speakers */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_S,/**< Left,center right and surround speakers */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_R_SL_RS,/**< Left, right, surround left and surround right */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_SL_SR,/**< Left, center, right, surround left and surround right */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_SL_SR_LFE,/**< Left, center, right, surround left, surround right and lfe*/
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_5_1 = TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_SL_SR_LFE,
        //   TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_SL_SR_RL_RR_LFE, /**< Left, center, right, surround left, surround right, rear left, rear right and lfe */
        //    TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_7_1 = TIF_HAL_PLAYBACK_AUDIO_SOURCE_CHANNEL_CONFIGURATION_L_C_R_SL_SR_RL_RR_LFE
        //} TIF_HAL_Playback_AudioSourceChannelConfiguration_t;
        //channel_configuration result = "channel_configuration=1";
        AudioManager audioManager = (AudioManager) this.getSystemService(Context.AUDIO_SERVICE);
        String temp = audioManager.getParameters(ConstantManager.AUDIO_PATCH_COMMAND_GET_AUDIO_CHANNEL_CONFIGURE);
        //Log.d(TAG, "getAudioChannelConfigureFromAudioPatch need add command in audio patch");
        try {
            if (temp != null) {
                String[] splitInfo = temp.split("=");
                if (splitInfo != null && splitInfo.length == 2 && "channel_configuration".equals(splitInfo[0])) {
                    result = Integer.valueOf(splitInfo[1]);
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "getAudioChannelConfigureFromAudioPatch Exception = " + e.getMessage());
        }
        Log.d(TAG, "getAudioChannelConfigureFromAudioPatch result = " + result);
        return result;
    }

    private List<TvTrackInfo> getSubtitleTrackInfoList() {
        List<TvTrackInfo> tracks = new ArrayList<>();
        //subtile tracks
        try {
            List<TvTrackInfo> subTracks = new ArrayList<>();
            JSONArray args = new JSONArray();
            JSONArray subtitleStreams = DtvkitGlueClient.getInstance().request("Player.getListOfSubtitleStreams", args).getJSONArray("data");
            int undefinedIndex = 1;
            for (int i = 0; i < subtitleStreams.length(); i++)
            {
                Bundle bundle = new Bundle();
                JSONObject subtitleStream = subtitleStreams.getJSONObject(i);
                Log.d(TAG, "getSubtitleTrackInfoList subtitleStream = " + subtitleStream.toString());
                String trackId = null;
                if (subtitleStream.getBoolean("teletext")) {
                    bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_TELETEXT, true);
                    int teletexttype = subtitleStream.getInt("teletext_type");
                    if (teletexttype == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE) {
                        bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING, false);
                        trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=1&hearing=0&flag=TTX";
                    } else if (teletexttype == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING) {
                        bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING, true);
                        trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=1&hearing=1&flag=TTX-H.O.H";
                    } else {
                        continue;
                    }
                } else {
                    bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_TELETEXT, false);
                    int subtitletype = subtitleStream.getInt("subtitle_type");
                    bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING, false);
                    if (subtitletype >= ConstantManager.ADB_SUBTITLE_TYPE_DVB && subtitletype <= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HD) {
                        trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=0&flag=none";//TYPE_DTV_CC
                    } else if (subtitletype >= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HARD_HEARING && subtitletype <= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_HD) {
                        bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING, true);
                        trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=1&flag=H.O.H";//TYPE_DTV_CC
                    } else {
                        trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=0&flag=none";//TYPE_DTV_CC
                    }
                }
                TvTrackInfo.Builder track = new TvTrackInfo.Builder(TvTrackInfo.TYPE_SUBTITLE, trackId);
                String subLang = subtitleStream.getString("language");
                if (TextUtils.isEmpty(subLang) || ConstantManager.CONSTANT_UND_FLAG.equals(subLang)) {
                    subLang = ConstantManager.CONSTANT_UND_VALUE + undefinedIndex;
                    undefinedIndex++;
                }
                track.setLanguage(subLang);
                int pid = subtitleStream.getInt("pid");
                bundle.putInt(ConstantManager.KEY_TRACK_PID, pid);
                bundle.putInt(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_INDEX, subtitleStream.getInt("index"));
                track.setExtra(bundle);
                subTracks.add(track.build());
            }
            ConstantManager.ascendTrackInfoOderByPid(subTracks);
            tracks.addAll(subTracks);
            List<TvTrackInfo> teleTracks = new ArrayList<>();
            JSONArray args1 = new JSONArray();
            JSONArray teletextStreams = DtvkitGlueClient.getInstance().request("Player.getListOfTeletextStreams", args1).getJSONArray("data");
            undefinedIndex = 1;
            for (int i = 0; i < teletextStreams.length(); i++)
            {
                Bundle bundle = new Bundle();
                JSONObject teletextStream = teletextStreams.getJSONObject(i);
                Log.d(TAG, "getSubtitleTrackInfoList teletextStream = " + teletextStream.toString());
                String trackId = null;
                int teletextType = teletextStream.getInt("teletext_type");
                if (teletextType == 1 || teletextType == 3 || teletextType == 4) {
                    bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_TELETEXT, true);
                    bundle.putBoolean(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_IS_HARD_HEARING, false);
                    trackId = "id=" + Integer.toString(teletextStream.getInt("index")) + "&" + "type=" + "6" + "&teletext=1&hearing=0&flag=none";//TYPE_DTV_TELETEXT_IMG
                } else {
                    continue;
                }
                TvTrackInfo.Builder track = new TvTrackInfo.Builder(TvTrackInfo.TYPE_SUBTITLE, trackId);
                String teleLang = teletextStream.getString("language");
                if (TextUtils.isEmpty(teleLang) || ConstantManager.CONSTANT_UND_FLAG.equals(teleLang)) {
                    teleLang = ConstantManager.CONSTANT_UND_VALUE + undefinedIndex;
                    undefinedIndex++;
                }
                track.setLanguage(teleLang);
                int pid = teletextStream.getInt("pid");
                bundle.putInt(ConstantManager.KEY_TRACK_PID, pid);
                bundle.putInt(ConstantManager.KEY_TVINPUTINFO_SUBTITLE_INDEX, teletextStream.getInt("index"));
                track.setExtra(bundle);
                teleTracks.add(track.build());
            }
            ConstantManager.ascendTrackInfoOderByPid(teleTracks);
            tracks.addAll(teleTracks);
        } catch (Exception e) {
            Log.e(TAG, "getSubtitleTrackInfoList Exception = " + e.getMessage());
        }
        return tracks;
    }

    private boolean playerSelectAudioTrack(int index) {
        try {
            JSONArray args = new JSONArray();
            args.put(index);
            DtvkitGlueClient.getInstance().request("Player.setAudioStream", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSelectAudioTrack = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playersetAudioDescriptionOn(boolean on) {
        try {
            JSONArray args = new JSONArray();
            args.put(on);
            DtvkitGlueClient.getInstance().request("Player.setAudioDescriptionOn", args);
        } catch (Exception e) {
            Log.e(TAG, "playersetAudioDescriptionOn = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playergetAudioDescriptionOn() {
        boolean result = false;
        try {
            JSONArray args = new JSONArray();
            result = DtvkitGlueClient.getInstance().request("Player.getAudioDescriptionOn", args).getBoolean("data");
        } catch (Exception e) {
            Log.e(TAG, "playergetAudioDescriptionOn = " + e.getMessage());
            return result;
        }
        return result;
    }

    private boolean playerHasDollyAssociateAudioTrack() {
        boolean result = false;
        List<TvTrackInfo> allAudioTrackList = getAudioTrackInfoList(false);
        if (allAudioTrackList == null || allAudioTrackList.size() < 2) {
            return result;
        }
        Iterator<TvTrackInfo> iterator = allAudioTrackList.iterator();
        while (iterator.hasNext()) {
            TvTrackInfo a = iterator.next();
            if (a != null) {
                Bundle trackBundle = a.getExtra();
                if (trackBundle != null) {
                    String audioCodec = trackBundle.getString(ConstantManager.KEY_TVINPUTINFO_AUDIO_CODEC, null);
                    boolean isAd = trackBundle.getBoolean(ConstantManager.KEY_TVINPUTINFO_AUDIO_AD, false);
                    if (isAd && (TextUtils.equals(audioCodec, "AC3") || TextUtils.equals(audioCodec, "E-AC3"))) {
                        result = true;
                        break;
                    }
                }
            } else {
                continue;
            }
        }
        return result;
    }

    private boolean playerAudioIndexIsAssociate(int index) {
        boolean result = false;
        List<TvTrackInfo> allAudioTrackList = getAudioTrackInfoList(false);
        if (allAudioTrackList == null || allAudioTrackList.size() <= index) {
            return result;
        }
        TvTrackInfo track = allAudioTrackList.get(index);
        if (track != null) {
            Bundle trackBundle = track.getExtra();
            if (trackBundle != null && trackBundle.getBoolean(ConstantManager.KEY_TVINPUTINFO_AUDIO_AD, false)) {
                result = true;
            }
        }
        return result;
    }

    private boolean playerSelectSubtitleTrack(int index) {
        try {
            JSONArray args = new JSONArray();
            args.put(index);
            DtvkitGlueClient.getInstance().request("Player.setSubtitleStream", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSelectSubtitleTrack = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playerSelectTeletextTrack(int index) {
        try {
            JSONArray args = new JSONArray();
            args.put(index);
            DtvkitGlueClient.getInstance().request("Player.setTeletextStream", args);
        } catch (Exception e) {
            Log.e(TAG, "playerSelectTeletextTrack = " + e.getMessage());
            return false;
        }
        return true;
    }

    //called when teletext on
    private boolean playerStartTeletext(int index) {
        try {
            JSONArray args = new JSONArray();
            args.put(index);
            DtvkitGlueClient.getInstance().request("Player.startTeletext", args);
        } catch (Exception e) {
            Log.e(TAG, "playerStartTeletext = " + e.getMessage());
            return false;
        }
        return true;
    }

    //called when teletext off
    private boolean playerStopTeletext() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.stopTeletext", args);
        } catch (Exception e) {
            Log.e(TAG, "playerStopTeletext = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playerPauseTeletext() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.pauseTeletext", args);
        } catch (Exception e) {
            Log.e(TAG, "playerPauseTeletext = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playerResumeTeletext() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Player.resumeTeletext", args);
        } catch (Exception e) {
            Log.e(TAG, "playerResumeTeletext = " + e.getMessage());
            return false;
        }
        return true;
    }

    private boolean playerIsTeletextDisplayed() {
        boolean on = false;
        try {
            JSONArray args = new JSONArray();
            on = DtvkitGlueClient.getInstance().request("Player.isTeletextDisplayed", args).getBoolean("data");
        } catch (Exception e) {
            Log.e(TAG, "playerResumeTeletext = " + e.getMessage());
        }
        return on;
    }

    private boolean playerIsTeletextStarted() {
        boolean on = false;
        try {
            JSONArray args = new JSONArray();
            on = DtvkitGlueClient.getInstance().request("Player.isTeletextStarted", args).getBoolean("data");
        } catch (Exception e) {
            Log.e(TAG, "playerIsTeletextStarted = " + e.getMessage());
        }
        Log.d(TAG, "playerIsTeletextStarted on = " + on);
        return on;
    }

    private boolean playerSetTeletextOn(boolean on, int index) {
        boolean result = false;
        if (on) {
            result = playerStartTeletext(index);
        } else {
            result = playerStopTeletext();
        }
        return result;
    }

    private boolean playerIsTeletextOn() {
        return playerIsTeletextStarted();
    }

    private boolean playerNotifyTeletextEvent(int event) {
        boolean on = false;
        try {
            JSONArray args = new JSONArray();
            args.put(event);
            DtvkitGlueClient.getInstance().request("Player.notifyTeletextEvent", args);
        } catch (Exception e) {
            Log.e(TAG, "playerNotifyTeletextEvent = " + e.getMessage());
            return false;
        }
        return true;
    }

    private int[] playerGetDTVKitVideoSize() {
        int[] result = {0, 0};
        try {
            JSONArray args = new JSONArray();
            JSONObject videoStreams = DtvkitGlueClient.getInstance().request("Player.getDTVKitVideoResolution", args);
            if (videoStreams != null) {
                videoStreams = (JSONObject)videoStreams.get("data");
                if (!(videoStreams == null || videoStreams.length() == 0)) {
                    Log.d(TAG, "playerGetDTVKitVideoSize videoStreams = " + videoStreams.toString());
                    result[0] = (int)videoStreams.get("width");
                    result[1] = (int)videoStreams.get("height");
                }
            } else {
                Log.d(TAG, "playerGetDTVKitVideoSize then get null");
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetDTVKitVideoSize = " + e.getMessage());
        }
        return result;
    }

    private String playerGetDTVKitVideoCodec() {
        String result = "";
        try {
            JSONArray args = new JSONArray();
            JSONObject videoStreams = DtvkitGlueClient.getInstance().request("Player.getDTVKitVideoCodec", args);
            if (videoStreams != null) {
                videoStreams = (JSONObject)videoStreams.get("data");
                if (!(videoStreams == null || videoStreams.length() == 0)) {
                    Log.d(TAG, "playerGetDTVKitVideoCodec videoStreams = " + videoStreams.toString());
                    //int videoPid = (int)videoStreams.get("video_pid");
                    result = videoStreams.getString("video_codec");
                }
            } else {
                Log.d(TAG, "playerGetDTVKitVideoCodec then get null");
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetDTVKitVideoCodec = " + e.getMessage());
        }
        return result;
    }

    private int playerGetSelectedSubtitleTrack() {
        int index = 0xFFFF;
        try {
            JSONArray args = new JSONArray();
            JSONArray subtitleStreams = DtvkitGlueClient.getInstance().request("Player.getListOfSubtitleStreams", args).getJSONArray("data");
            for (int i = 0; i < subtitleStreams.length(); i++)
            {
                JSONObject subtitleStream = subtitleStreams.getJSONObject(i);
                if (subtitleStream.getBoolean("selected")) {
                    boolean isTele = subtitleStream.getBoolean("teletext");
                    if (isTele) {
                        int teleType = subtitleStream.getInt("teletext_type");
                        if (teleType != ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE && teleType != ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING) {
                            Log.i(TAG, "playerGetSelectedSubtitleTrack skip teletext");
                            continue;
                        }
                    }
                    index = subtitleStream.getInt("index");
                    Log.i(TAG, "playerGetSelectedSubtitleTrack index = " + index);
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetSelectedSubtitleTrack = " + e.getMessage());
        }
        Log.i(TAG, "playerGetSelectedSubtitleTrack index = " + index);
        return index;
    }

    private String playerGetSelectedSubtitleTrackId() {
        String trackId = null;
        try {
            JSONArray args = new JSONArray();
            JSONArray subtitleStreams = DtvkitGlueClient.getInstance().request("Player.getListOfSubtitleStreams", args).getJSONArray("data");
            for (int i = 0; i < subtitleStreams.length(); i++)
            {
                JSONObject subtitleStream = subtitleStreams.getJSONObject(i);
                if (subtitleStream.getBoolean("selected")) {
                    boolean isTele = subtitleStream.getBoolean("teletext");
                    if (isTele) {
                        int teletexttype = subtitleStream.getInt("teletext_type");
                        if (teletexttype == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE) {
                            trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=1&hearing=0&flag=TTX";//tele sub
                        } else if (teletexttype == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING) {
                            trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=1&hearing=1&flag=TTX-H.O.H";//tele sub
                        } else {
                            continue;
                        }
                    } else {
                        int subtitletype = subtitleStream.getInt("subtitle_type");
                        if (subtitletype >= ConstantManager.ADB_SUBTITLE_TYPE_DVB && subtitletype <= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HD) {
                            trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=0&flag=none";//TYPE_DTV_CC
                        } else if (subtitletype >= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HARD_HEARING && subtitletype <= ConstantManager.ADB_SUBTITLE_TYPE_DVB_HARD_HEARING_HD) {
                            trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=1&flag=H.O.H";//TYPE_DTV_CC
                        } else {
                            trackId = "id=" + Integer.toString(subtitleStream.getInt("index")) + "&" + "type=" + "4" + "&teletext=0&hearing=0&flag=none";//TYPE_DTV_CC
                        }
                    }
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetSelectedSubtitleTrackId = " + e.getMessage());
        }
        Log.i(TAG, "playerGetSelectedTeleTextTrack trackId = " + trackId);
        return trackId;
    }

    private int playerGetSelectedTeleTextTrack() {
        int index = 0xFFFF;
        try {
            JSONArray args = new JSONArray();
            JSONArray teletextStreams = DtvkitGlueClient.getInstance().request("Player.getListOfTeletextStreams", args).getJSONArray("data");
            for (int i = 0; i < teletextStreams.length(); i++)
            {
                JSONObject teletextStream = teletextStreams.getJSONObject(i);
                if (teletextStream.getBoolean("selected")) {
                    int teleType = teletextStream.getInt("teletext_type");
                    if (teleType == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE || teleType == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING) {
                        Log.i(TAG, "playerGetSelectedTeleTextTrack skip tele sub");
                        continue;
                    }
                    index = teletextStream.getInt("index");
                    Log.i(TAG, "playerGetSelectedTeleTextTrack index = " + index);
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetSelectedTeleTextTrack Exception = " + e.getMessage());
        }
        return index;
    }

    private String playerGetSelectedTeleTextTrackId() {
        String trackId = null;
        try {
            JSONArray args = new JSONArray();
            JSONArray teletextStreams = DtvkitGlueClient.getInstance().request("Player.getListOfTeletextStreams", args).getJSONArray("data");
            for (int i = 0; i < teletextStreams.length(); i++)
            {
                JSONObject teletextStream = teletextStreams.getJSONObject(i);
                if (teletextStream.getBoolean("selected")) {
                    int teleType = teletextStream.getInt("teletext_type");
                    if (teleType == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE || teleType == ConstantManager.ADB_TELETEXT_TYPE_SUBTITLE_HARD_HEARING) {
                        Log.i(TAG, "playerGetSelectedTeleTextTrackId skip tele sub");
                        continue;
                    }
                    trackId = "id=" + Integer.toString(teletextStream.getInt("index")) + "&type=6" + "&teletext=1&hearing=0&flag=TTX";//TYPE_DTV_TELETEXT_IMG
                    Log.i(TAG, "playerGetSelectedTeleTextTrackId trackId = " + trackId);
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetSelectedTeleTextTrackId Exception = " + e.getMessage());
        }
        Log.i(TAG, "playerGetSelectedTeleTextTrackId trackId = " + trackId);
        return trackId;
    }

    private int playerGetSelectedAudioTrack() {
        int index = 0xFFFF;
        try {
            JSONArray args = new JSONArray();
            JSONArray audioStreams = DtvkitGlueClient.getInstance().request("Player.getListOfAudioStreams", args).getJSONArray("data");
            for (int i = 0; i < audioStreams.length(); i++)
            {
                JSONObject audioStream = audioStreams.getJSONObject(i);
                if (audioStream.getBoolean("selected")) {
                    index = audioStream.getInt("index");
                    Log.i(TAG, "playerGetSelectedAudioTrack index = " + index);
                    break;
                }
            }
        } catch (Exception e) {
            Log.e(TAG, "playerGetSelectedAudioTrack = " + e.getMessage());
        }
        return index;
    }

    private boolean playerGetSubtitlesOn() {
        boolean on = false;
        try {
            JSONArray args = new JSONArray();
            on = DtvkitGlueClient.getInstance().request("Player.getSubtitlesOn", args).getBoolean("data");
        } catch (Exception e) {
            Log.e(TAG, "playerGetSubtitlesOn = " + e.getMessage());
        }
        Log.i(TAG, "playerGetSubtitlesOn on = " + on);
        return on;
    }

    private int getParentalControlAge() {
        int age = 0;

        try {
            JSONArray args = new JSONArray();
            age = DtvkitGlueClient.getInstance().request("Dvb.getParentalControlAge", args).getInt("data");
            Log.i(TAG, "getParentalControlAge:" + age);
        } catch (Exception e) {
            Log.e(TAG, "getParentalControlAge " + e.getMessage());
        }
        return age;
    }

    private void setParentalControlAge(int age) {

        try {
            JSONArray args = new JSONArray();
            args.put(age);
            DtvkitGlueClient.getInstance().request("Dvb.setParentalControlAge", args);
            Log.i(TAG, "setParentalControlAge:" + age);
        } catch (Exception e) {
            Log.e(TAG, "setParentalControlAge " + e.getMessage());
        }
    }

    private boolean getParentalControlOn() {
        boolean parentalctrl_enabled = false;
        try {
            JSONArray args = new JSONArray();
            parentalctrl_enabled = DtvkitGlueClient.getInstance().request("Dvb.getParentalControlOn", args).getBoolean("data");;
            Log.i(TAG, "getParentalControlOn:" + parentalctrl_enabled);
        } catch (Exception e) {
            Log.e(TAG, "getParentalControlOn " + e.getMessage());
        }
        return parentalctrl_enabled;
    }

    private void setParentalControlOn(boolean parentalctrl_enabled) {
        try {
            JSONArray args = new JSONArray();
            args.put(parentalctrl_enabled);
            DtvkitGlueClient.getInstance().request("Dvb.setParentalControlOn", args);
            Log.i(TAG, "setParentalControlOn:" + parentalctrl_enabled);
        } catch (Exception e) {
            Log.e(TAG, "setParentalControlOn " + e.getMessage());
        }
    }

    private int getCurrentMinAgeByBlockedRatings() {
        List<TvContentRating> ratingList = mTvInputManager.getBlockedRatings();
        String rating_system;
        String parentcontrol_rating;
        int min_age = 0xFF;
        for (int i = 0; i < ratingList.size(); i++)
        {
            parentcontrol_rating = ratingList.get(i).getMainRating();
            rating_system = ratingList.get(i).getRatingSystem();
            if (rating_system.equals("DVB"))
            {
                String[] ageArry = parentcontrol_rating.split("_", 2);
                if (ageArry[0].equals("DVB"))
                {
                   int age_temp = Integer.valueOf(ageArry[1]);
                   min_age = min_age < age_temp ? min_age : age_temp;
                }
            }
        }
        return min_age;
    }

    private void mhegSuspend() {
        Log.e(TAG, "Mheg suspending");
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Mheg.suspend", args);
            Log.e(TAG, "Mheg suspended");
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
    }

    private int mhegGetNextTuneInfo(String dvbUri) {
        int quiet = -1;
        try {
            JSONArray args = new JSONArray();
            args.put(dvbUri);
            quiet = DtvkitGlueClient.getInstance().request("Mheg.getTuneInfo", args).getInt("data");
            Log.e(TAG, "Tune info: "+ quiet);
        } catch (Exception e) {
            Log.e(TAG, e.getMessage());
        }
        return quiet;
    }

    private JSONObject playerGetStatus() {
        JSONObject response = null;
        try {
            JSONArray args = new JSONArray();
            response = DtvkitGlueClient.getInstance().request("Player.getStatus", args).getJSONObject("data");
        } catch (Exception e) {
            Log.e(TAG, "playerGetStatus = " + e.getMessage());
        }
        return response;
    }

    private int[] getSignalStatus() {
        int[] result = {0, 0};
        try {
            JSONArray args1 = new JSONArray();
            args1.put(0);
            JSONObject jsonObj = DtvkitGlueClient.getInstance().request("Dvb.getFrontend", args1);
            if (jsonObj != null) {
                Log.d(TAG, "getSignalStatus resultObj:" + jsonObj.toString());
            } else {
                Log.d(TAG, "getSignalStatus then get null");
            }
            JSONObject data = null;
            if (jsonObj != null) {
                data = (JSONObject)jsonObj.get("data");
            }
            if (data == null || data.length() == 0) {
                return result;
            } else {
                result[0] = (int)(data.get("strength"));
                result[1] = (int)(data.get("integrity"));
                return result;
            }
        } catch (Exception e) {
            Log.d(TAG, "getSignalStatus Exception " + e.getMessage() + ", trace=" + e.getStackTrace());
            e.printStackTrace();
        }
        return result;
    }

    private long playerGetElapsed() {
        return playerGetElapsed(playerGetStatus());
    }

    private long playerGetElapsed(JSONObject playerStatus) {
        long elapsed = 0;
        if (playerStatus != null) {
            try {
                JSONObject content = playerStatus.getJSONObject("content");
                if (content.has("elapsed")) {
                    elapsed = content.getLong("elapsed");
                }
            } catch (JSONException e) {
                Log.e(TAG, "playerGetElapsed = " + e.getMessage());
            }
        }
        return elapsed;
    }

    private JSONObject playerGetTimeshiftRecorderStatus() {
        JSONObject response = null;
        try {
            JSONArray args = new JSONArray();
            response = DtvkitGlueClient.getInstance().request("Player.getTimeshiftRecorderStatus", args).getJSONObject("data");
        } catch (Exception e) {
            Log.e(TAG, "playerGetTimeshiftRecorderStatus = " + e.getMessage());
        }
        return response;
    }

    private int playerGetTimeshiftBufferSize() {
        int timeshiftBufferSize = 0;
        try {
            JSONArray args = new JSONArray();
            timeshiftBufferSize = DtvkitGlueClient.getInstance().request("Player.getTimeshiftBufferSize", args).getInt("data");
        } catch (Exception e) {
            Log.e(TAG, "playerGetTimeshiftBufferSize = " + e.getMessage());
        }
        return timeshiftBufferSize;
    }

    private boolean playerSetTimeshiftBufferSize(int timeshiftBufferSize) {
        try {
            JSONArray args = new JSONArray();
            args.put(timeshiftBufferSize);
            DtvkitGlueClient.getInstance().request("Player.setTimeshiftBufferSize", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "playerSetTimeshiftBufferSize = " + e.getMessage());
            return false;
        }
    }

    private boolean recordingSetDefaultDisk(String disk_path) {
        try {
            Log.d(TAG, "setDefaultDisk: " + disk_path);
            JSONArray args = new JSONArray();
            args.put(disk_path);
            DtvkitGlueClient.getInstance().request("Recording.setDefaultDisk", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "recordingSetDefaultDisk = " + e.getMessage());
            return false;
        }
    }

    private boolean recordingAddDiskPath(String diskPath) {
        try {
            Log.d(TAG, "addDiskPath: " + diskPath);
            JSONArray args = new JSONArray();
            args.put(diskPath);
            DtvkitGlueClient.getInstance().request("Recording.addDiskPath", args);
            return true;
        } catch (Exception e) {
            Log.e(TAG, "recordingAddDiskPath = " + e.getMessage());
            return false;
        }
    }

    private String playerGetTimeshiftRecorderState(JSONObject playerTimeshiftRecorderStatus) {
        String timeshiftRecorderState = "off";
        if (playerTimeshiftRecorderStatus != null) {
            try {
                if (playerTimeshiftRecorderStatus.has("timeshiftrecorderstate")) {
                    timeshiftRecorderState = playerTimeshiftRecorderStatus.getString("timeshiftrecorderstate");
                }
            } catch (JSONException e) {
                Log.e(TAG, "playerGetTimeshiftRecorderState = " + e.getMessage());
            }
        }

        return timeshiftRecorderState;
    }

    private int mhegStartService(String dvbUri) {
        int quiet = -1;
        try {
            JSONArray args = new JSONArray();
            args.put(dvbUri);
            quiet = DtvkitGlueClient.getInstance().request("Mheg.start", args).getInt("data");
            Log.e(TAG, "Mheg started");
        } catch (Exception e) {
            Log.e(TAG, "mhegStartService = " + e.getMessage());
        }
        return quiet;
    }

    private void mhegStop() {
        try {
            JSONArray args = new JSONArray();
            DtvkitGlueClient.getInstance().request("Mheg.stop", args);
            Log.e(TAG, "Mheg stopped");
        } catch (Exception e) {
            Log.e(TAG, "mhegStop = " + e.getMessage());
        }
    }
    private boolean mhegKeypress(int keyCode) {
      boolean used=false;
        try {
            JSONArray args = new JSONArray();
            args.put(keyCode);
            used = DtvkitGlueClient.getInstance().request("Mheg.notifyKeypress", args).getBoolean("data");
            Log.e(TAG, "Mheg keypress, used:" + used);
        } catch (Exception e) {
            Log.e(TAG, "mhegKeypress = " + e.getMessage());
        }
        return used;
    }

    private final ContentObserver mContentObserver = new ContentObserver(new Handler()) {
        @Override
        public void onChange(boolean selfChange) {
            onChannelsChanged();
        }
    };

   private boolean recordingAddRecording(String dvbUri, boolean eventTriggered, long duration, StringBuffer response) {
       try {
           JSONArray args = new JSONArray();
           args.put(dvbUri);
           args.put(eventTriggered);
           args.put(duration);
           response.insert(0, DtvkitGlueClient.getInstance().request("Recording.addScheduledRecording", args).getString("data"));
           return true;
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           response.insert(0, e.getMessage());
           return false;
       }
   }

   private boolean checkActiveRecording() {
        return checkActiveRecording(recordingGetStatus());
   }

   private boolean checkActiveRecording(JSONObject recordingStatus) {
        boolean active = false;

        if (recordingStatus != null) {
            try {
                active = recordingStatus.getBoolean("active");
            } catch (JSONException e) {
                Log.e(TAG, e.getMessage());
            }
        }
        return active;
   }

   private JSONObject recordingGetStatus() {
       JSONObject response = null;
       try {
           JSONArray args = new JSONArray();
           response = DtvkitGlueClient.getInstance().request("Recording.getStatus", args).getJSONObject("data");
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
       }
       return response;
   }

   private boolean recordingStartRecording(String dvbUri, int path, StringBuffer response) {
       try {
           JSONArray args = new JSONArray();
           args.put(dvbUri);
           args.put(path);
           response.insert(0, DtvkitGlueClient.getInstance().request("Recording.startRecording", args).getString("data"));
           return true;
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           response.insert(0, e.getMessage());
           return false;
       }
   }

   private boolean recordingStopRecording(String dvbUri) {
       try {
           JSONArray args = new JSONArray();
           args.put(dvbUri);
           DtvkitGlueClient.getInstance().request("Recording.stopRecording", args);
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           return false;
       }
       return true;
   }

   private boolean recordingCheckAvailability(String dvbUri) {
       try {
           JSONArray args = new JSONArray();
           args.put(dvbUri);
           DtvkitGlueClient.getInstance().request("Recording.checkAvailability", args);
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           return false;
       }
       return true;
   }

   private JSONObject recordingTune(String dvbUri, StringBuffer response) {
       JSONObject tune = null;
       try {
           JSONArray args = new JSONArray();
           args.put(dvbUri);
           tune = DtvkitGlueClient.getInstance().request("Recording.tune", args).getJSONObject("data");
           Log.d(TAG, "recordingTune: "+ tune.toString());
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           response.insert(0, e.getMessage());
       }
       return tune;
   }

   private boolean recordingUntune(int path) {
       try {
           JSONArray args = new JSONArray();
           args.put(path);
           DtvkitGlueClient.getInstance().request("Recording.unTune", args).get("data");
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
       }
       return true;
   }


   private int getRecordingTunePath(JSONObject tuneStat) {
      int path = -1;
      try {
          path = (tuneStat != null ? tuneStat.getInt("path") : 255);
      } catch (Exception e) {
      }
      return path;
   }

   private boolean getRecordingTuned(JSONObject tuneStat) {
      boolean tuned = false;
      try {
          tuned = (tuneStat != null ? tuneStat.getBoolean("tuned") : false);
      } catch (Exception e) {
      }
      return tuned;
   }

   private String getProgramInternalRecordingUri() {
        return getProgramInternalRecordingUri(recordingGetStatus());
   }

   private String getProgramInternalRecordingUri(JSONObject recordingStatus) {
        String uri = "dvb://0000.0000.0000.0000;0000";
        if (recordingStatus != null) {
           try {
               JSONArray activeRecordings = recordingStatus.getJSONArray("activerecordings");
               if (activeRecordings.length() == 1)
               {
                   uri = activeRecordings.getJSONObject(0).getString("uri");
               }
           } catch (JSONException e) {
               Log.e(TAG, e.getMessage());
           }
       }
       return uri;
   }

   private JSONArray recordingGetActiveRecordings() {
       return recordingGetActiveRecordings(recordingGetStatus());
   }

   private JSONArray recordingGetActiveRecordings(JSONObject recordingStatus) {
       JSONArray activeRecordings = null;
       if (recordingStatus != null) {
            try {
                activeRecordings = recordingStatus.getJSONArray("activerecordings");
            } catch (JSONException e) {
                Log.e(TAG, e.getMessage());
            }
       }
       return activeRecordings;
   }

   private boolean recordingIsRecordingPathActive(JSONObject recordingStatus, int path) {
       boolean active = false;
       if (recordingStatus != null) {
            try {
                JSONArray activeRecordings = null;
                activeRecordings = recordingStatus.getJSONArray("activerecordings");
                if (activeRecordings != null) {
                    for (int i = 0; i < activeRecordings.length(); i++) {
                        int activePath = activeRecordings.getJSONObject(i).getInt("path");
                        if (activePath == path) {
                            active = true;
                            break;
                        }
                    }
                }
            } catch (JSONException e) {
                Log.e(TAG, e.getMessage());
            }
       }
       return active;
   }


   private int recordingGetNumActiveRecordings() {
        int numRecordings = 0;
        JSONArray activeRecordings = recordingGetActiveRecordings();
        if (activeRecordings != null) {
            numRecordings = activeRecordings.length();
        }
        return numRecordings;
   }

   private int recordingGetNumRecorders() {
       int numRecorders = 0;
       try {
           JSONArray args = new JSONArray();
           numRecorders = DtvkitGlueClient.getInstance().request("Recording.getNumberOfRecorders", args).getInt("data");
           Log.i(TAG, "numRecorders: " + numRecorders);
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
       }
       return numRecorders;
   }

   private JSONArray recordingGetListOfRecordings() {
       JSONArray recordings = null;
       try {
           JSONArray args = new JSONArray();
           recordings = DtvkitGlueClient.getInstance().request("Recording.getListOfRecordings", args).getJSONArray("data");
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
       }
       return recordings;
   }

   private boolean recordingRemoveRecording(String uri) {
       try {
           JSONArray args = new JSONArray();
           args.put(uri);
           DtvkitGlueClient.getInstance().request("Recording.removeRecording", args);
           return true;
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           return false;
       }
   }

   private boolean checkRecordingExists(String uri, Cursor cursor) {
        boolean recordingExists = false;
        if (cursor != null && cursor.moveToFirst()) {
            do {
                RecordedProgram recordedProgram = RecordedProgram.fromCursor(cursor);
                if (recordedProgram.getRecordingDataUri().equals(uri)) {
                    recordingExists = true;
                    break;
                }
            } while (cursor.moveToNext());
        }
        return recordingExists;
   }

   private JSONArray recordingGetListOfScheduledRecordings() {
       JSONArray scheduledRecordings = null;
       try {
           JSONArray args = new JSONArray();
           scheduledRecordings = DtvkitGlueClient.getInstance().request("Recording.getListOfScheduledRecordings", args).getJSONArray("data");
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
       }
       return scheduledRecordings;
   }

   private String getScheduledRecordingUri(JSONObject scheduledRecording) {
        String uri = "dvb://0000.0000.0000;0000";
        if (scheduledRecording != null) {
            try {
                uri = scheduledRecording.getString("uri");
            } catch (JSONException e) {
                Log.e(TAG, e.getMessage());
            }
        }
        return uri;
   }

   private boolean recordingRemoveScheduledRecording(String uri) {
       try {
           JSONArray args = new JSONArray();
           args.put(uri);
           DtvkitGlueClient.getInstance().request("Recording.removeScheduledRecording", args);
           return true;
       } catch (Exception e) {
           Log.e(TAG, e.getMessage());
           return false;
       }
   }

    private final ContentObserver mRecordingsContentObserver = new ContentObserver(new Handler()) {
        @RequiresApi(api = Build.VERSION_CODES.N)
        @Override
        public void onChange(boolean selfChange) {
            onRecordingsChanged();
        }
    };

   @RequiresApi(api = Build.VERSION_CODES.N)
   private void onRecordingsChanged() {
       Log.i(TAG, "onRecordingsChanged");

       new Thread(new Runnable() {
           @Override
           public void run() {
               Cursor cursor = mContentResolver.query(TvContract.RecordedPrograms.CONTENT_URI, RecordedProgram.PROJECTION, null, null, TvContract.RecordedPrograms._ID + " DESC");
               JSONArray recordings = recordingGetListOfRecordings();
               JSONArray activeRecordings = recordingGetActiveRecordings();

               if (recordings != null && cursor != null) {
                   for (int i = 0; i < recordings.length(); i++) {
                       try {
                           String uri = recordings.getJSONObject(i).getString("uri");

                           if (activeRecordings != null && activeRecordings.length() > 0) {
                               boolean activeRecording = false;
                               for (int j = 0; j < activeRecordings.length(); j++) {
                                   if (uri.equals(activeRecordings.getJSONObject(j).getString("uri"))) {
                                       activeRecording = true;
                                       break;
                                   }
                               }
                               if (activeRecording) {
                                   continue;
                               }
                           }

                           if (!checkRecordingExists(uri, cursor)) {
                               Log.d(TAG, "remove invalid recording: "+uri);
                               recordingRemoveRecording(uri);
                           }

                       } catch (JSONException e) {
                           Log.e(TAG, e.getMessage());
                       }
                   }
               }
           }
       }).start();
    }

    private void scheduleTimeshiftRecordingTask() {
       final long SCHEDULE_TIMESHIFT_RECORDING_DELAY_MILLIS = 1000 * 2;
       Log.i(TAG, "calling scheduleTimeshiftRecordingTask");
       if (scheduleTimeshiftRecordingHandler == null) {
            scheduleTimeshiftRecordingHandler = new Handler(Looper.getMainLooper());
       } else {
            scheduleTimeshiftRecordingHandler.removeCallbacks(timeshiftRecordRunnable);
       }
       scheduleTimeshiftRecordingHandler.postDelayed(timeshiftRecordRunnable, SCHEDULE_TIMESHIFT_RECORDING_DELAY_MILLIS);
    }

    private void removeScheduleTimeshiftRecordingTask() {
        Log.i(TAG, "calling removeScheduleTimeshiftRecordingTask");
        if (scheduleTimeshiftRecordingHandler != null) {
            scheduleTimeshiftRecordingHandler.removeCallbacks(timeshiftRecordRunnable);
        }
    }

    private HardwareCallback mHardwareCallback = new HardwareCallback(){
        @Override
        public void onReleased() {
            Log.d(TAG, "onReleased");
            mHardware = null;
        }

        @Override
        public void onStreamConfigChanged(TvStreamConfig[] configs) {
            Log.d(TAG, "onStreamConfigChanged");
            mConfigs = configs;
        }
    };

    public ResolveInfo getResolveInfo(String cls_name) {
        if (TextUtils.isEmpty(cls_name))
            return null;
        ResolveInfo ret_ri = null;
        PackageManager pm = getApplicationContext().getPackageManager();
        List<ResolveInfo> services = pm.queryIntentServices(new Intent(TvInputService.SERVICE_INTERFACE),
                PackageManager.GET_SERVICES | PackageManager.GET_META_DATA);
        for (ResolveInfo ri : services) {
            ServiceInfo si = ri.serviceInfo;
            if (!android.Manifest.permission.BIND_TV_INPUT.equals(si.permission)) {
                continue;
            }
            Log.d(TAG, "cls_name = " + cls_name + ", si.name = " + si.name);
            if (cls_name.equals(si.name)) {
                ret_ri = ri;
                break;
            }
        }
        return ret_ri;
    }

    public TvInputInfo onHardwareAdded(TvInputHardwareInfo hardwareInfo) {
        Log.d(TAG, "onHardwareAdded ," + "DeviceId :" + hardwareInfo.getDeviceId());
        if (hardwareInfo.getDeviceId() != 19)
            return null;
        ResolveInfo rinfo = getResolveInfo(DtvkitTvInput.class.getName());
        if (rinfo != null) {
            try {
            mTvInputInfo = TvInputInfo.createTvInputInfo(getApplicationContext(), rinfo, hardwareInfo, null, null);
            } catch (XmlPullParserException e) {
                //TODO: handle exception
            } catch (IOException e) {
                //TODO: handle exception
            }
        }
        setInputId(mTvInputInfo.getId());
        mHardware = mTvInputManager.acquireTvInputHardware(19,mHardwareCallback,mTvInputInfo);
        return mTvInputInfo;
    }

    public String onHardwareRemoved(TvInputHardwareInfo hardwareInfo) {
        Log.d(TAG, "onHardwareRemoved");
        if (hardwareInfo.getDeviceId() != 19)
            return null;
        String id = null;
        if (mTvInputInfo != null) {
            id = mTvInputInfo.getId();
            mTvInputInfo = null;
        }
        return id;
    }


    private boolean getFeatureSupportTimeshifting() {
        return !PropSettingManager.getBoolean(PropSettingManager.TIMESHIFT_DISABLE, false);
    }

    private boolean getFeatureCasReady() {
        return PropSettingManager.getBoolean("vendor.tv.dtv.cas.ready", false);
    }

    private boolean getFeatureDmxNoLimit() {
        return PropSettingManager.getBoolean("vendor.tv.dtv.dmx.nolimit", false);
    }

    private boolean getFeatureCompliance() {
        return PropSettingManager.getBoolean("vendor.tv.dtv.compiliance", true);
    }

    private boolean needForceCompliance() {
        return !getFeatureDmxNoLimit() && (getFeatureCasReady() || getFeatureCompliance());
    }

    private int getNumRecordersLimit() {
        if (numRecorders > 0)
            return needForceCompliance() ? 1 : numRecorders;
        return numRecorders;
    }

    private boolean getFeatureTimeshiftingPriorityHigh() {
        return !needForceCompliance();
    }

    private boolean createDecoder() {
        String str = "OMX.amlogic.avc.decoder.awesome.secure";
        try {
            mMediaCodec1 = MediaCodec.createByCodecName(str);
            mMediaCodec2 = MediaCodec.createByCodecName(str);
            mMediaCodec3 = MediaCodec.createByCodecName(str);
        } catch (Exception exception) {
            Log.d(TAG, "Exception during decoder creation", exception);
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

    private final DtvkitGlueClient.SystemControlHandler mSysControlHandler = new DtvkitGlueClient.SystemControlHandler() {
        @RequiresApi(api = Build.VERSION_CODES.M)
        @Override
        public String onReadSysFs(int ftype, String name) {
            String value = null;
            if (mSystemControlManager != null) {
                if (ftype == SYSFS) {
                    value = mSystemControlManager.readSysFs(name);
                } else if (ftype == PROP) {
                    value = mSystemControlManager.getProperty(name);
                } else {
                    //printf error log
                }
           }
           return value;
        }

        @RequiresApi(api = Build.VERSION_CODES.M)
        @Override
        public void onWriteSysFs(int ftype, String name, String cmd) {
            if (mSystemControlManager != null) {
                if (ftype == SYSFS) {
                    mSystemControlManager.writeSysFs(name, cmd);
                } else if (ftype == PROP) {
                    mSystemControlManager.setProperty(name, cmd);
                } else {
                       //printf error log
                }
           }
        }
    };

    private void showSearchConfirmDialog(final Context context, final Channel channel) {
        if (context == null || channel == null) {
            Log.d(TAG, "showSearchConfirmDialog null context or input");
            return;
        }

        final AlertDialog.Builder builder = new AlertDialog.Builder(context);
        final AlertDialog alert = builder.create();
        final View dialogView = View.inflate(context, R.layout.confirm_search, null);
        final TextView title = (TextView) dialogView.findViewById(R.id.dialog_title);
        final Button confirm = (Button) dialogView.findViewById(R.id.confirm);
        final Button cancel = (Button) dialogView.findViewById(R.id.cancel);
        final int[] tempStatus = new int[1];//0 for flag that exit is pressed by user, 1 for exit by search over
        final DtvkitSingleFrequencySetup.SingleFrequencyCallback callback = new DtvkitSingleFrequencySetup.SingleFrequencyCallback() {
            @Override
            public void onMessageCallback(JSONObject mess) {
                if (mess != null) {
                    Log.d(TAG, "onMessageCallback " + mess.toString());
                    String status = null;
                    try {
                        status = mess.getString(DtvkitSingleFrequencySetup.SINGLE_FREQUENCY_STATUS_ITEM);
                    } catch (Exception e) {
                        Log.i(TAG, "onMessageCallback SINGLE_FREQUENCY_STATUS_ITEM Exception " + e.getMessage());
                    }
                    switch (status) {
                        case DtvkitSingleFrequencySetup.SINGLE_FREQUENCY_STATUS_SAVE_FINISH: {
                            tempStatus[0] = 1;
                            if (alert != null) {
                                alert.dismiss();
                            }
                            if (mSession != null) {
                                Channel newChannel = null;
                                Uri channelUri = null;
                                String serviceName = null;
                                try {
                                    serviceName = mess.getString(DtvkitSingleFrequencySetup.SINGLE_FREQUENCY_CHANNEL_NAME);
                                } catch (Exception e) {
                                    Log.i(TAG, "onMessageCallback SINGLE_FREQUENCY_CHANNEL_NAME Exception " + e.getMessage());
                                }
                                if (!TextUtils.isEmpty(serviceName)) {
                                    newChannel = TvContractUtils.getChannelByDisplayName(context.getContentResolver(), serviceName);
                                }
                                if (newChannel != null) {
                                    channelUri = TvContract.buildChannelUri(newChannel.getId());
                                }
                                if (channelUri != null) {
                                    mSession.onTune(channelUri);
                                    mSession.notifyChannelRetuned(channelUri);
                                    Log.d(TAG, "onMessageCallback notifyChannelRetuned " + channelUri);
                                } else {
                                    Log.d(TAG, "onMessageCallback none channels");
                                }
                            }
                            break;
                        }
                        default:
                            break;
                    }
                }
            }
        };
        String channelSignalType = null;
        int frequency = -1;
        try {
            channelSignalType = channel.getInternalProviderData().get("channel_signal_type").toString();
            frequency = Integer.valueOf(channel.getInternalProviderData().get("frequency").toString());
        } catch (Exception e) {
            Log.i(TAG, "onMessageCallback channelSignalType Exception " + e.getMessage());
        }
        if (!TextUtils.equals(channelSignalType, Channel.FIXED_SIGNAL_TYPE_DVBC) && !TextUtils.equals(channelSignalType, Channel.FIXED_SIGNAL_TYPE_DVBT) && !TextUtils.equals(channelSignalType, Channel.FIXED_SIGNAL_TYPE_DVBT2)) {
            Log.d(TAG, "showSearchConfirmDialog not dvbc or dvbt and is " + channelSignalType);
            return;
        }
        final DtvkitSingleFrequencySetup setup = new DtvkitSingleFrequencySetup(context, channelSignalType, frequency, channel.getInputId(), callback);
        setup.startSearch();
        title.setText(R.string.dvb_network_change);
        confirm.setVisibility(View.GONE);
        cancel.setVisibility(View.GONE);
        /*confirm.requestFocus();
        cancel.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                alert.dismiss();
            }
        });
        confirm.setOnClickListener(new Button.OnClickListener() {
            @Override
            public void onClick(View v) {
                alert.dismiss();
                Intent setupIntent = input.createSetupIntent();
                setupIntent.putExtra(TvInputInfo.EXTRA_INPUT_ID, input.getId());
                setupIntent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
                context.startActivity(setupIntent);
            }
        });*/
        alert.setOnDismissListener(new DialogInterface.OnDismissListener() {
            @Override
            public void onDismiss(DialogInterface dialog) {
                Log.d(TAG, "showSearchConfirmDialog onDismiss");
                if (tempStatus[0] != 1 && setup != null) {
                    Log.d(TAG, "showSearchConfirmDialog need to stop search");
                    setup.stopSearch();
                }
            }
        });
        alert.getWindow().setType(WindowManager.LayoutParams.TYPE_SYSTEM_ALERT);
        alert.setView(dialogView);
        alert.show();
        WindowManager.LayoutParams params = alert.getWindow().getAttributes();
        params.width = (int)TypedValue.applyDimension(TypedValue.COMPLEX_UNIT_DIP, 500, context.getResources().getDisplayMetrics());
        params.height = WindowManager.LayoutParams.WRAP_CONTENT;
        alert.getWindow().setAttributes(params);
        alert.getWindow().setBackgroundDrawableResource(R.drawable.dialog_background);
  }

}
