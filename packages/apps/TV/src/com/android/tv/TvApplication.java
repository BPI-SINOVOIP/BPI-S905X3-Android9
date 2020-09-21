/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.tv;

import android.annotation.TargetApi;
import android.app.Activity;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.media.tv.TvContract;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputManager.TvInputCallback;
import android.os.Build;
import android.os.Bundle;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.Log;
import android.view.KeyEvent;
import com.android.tv.common.BaseApplication;
import com.android.tv.common.concurrent.NamedThreadFactory;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.recording.RecordingStorageStatusManager;
import com.android.tv.common.ui.setup.animation.SetupAnimationHelper;
import com.android.tv.common.util.Clock;
import com.android.tv.common.util.Debug;
import com.android.tv.common.util.SharedPreferencesUtils;
import com.android.tv.common.util.SystemPropertiesProxy;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.PreviewDataManager;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.data.epg.EpgFetcher;
import com.android.tv.data.epg.EpgFetcherImpl;
import com.android.tv.dvr.DvrDataManager;
import com.android.tv.dvr.DvrDataManagerImpl;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.DvrScheduleManager;
import com.android.tv.dvr.DvrStorageStatusManager;
import com.android.tv.dvr.DvrWatchedPositionManager;
import com.android.tv.dvr.recorder.RecordingScheduler;
import com.android.tv.dvr.ui.browse.DvrBrowseActivity;
import com.android.tv.recommendation.ChannelPreviewUpdater;
import com.android.tv.recommendation.RecordedProgramPreviewUpdater;
import com.android.tv.tuner.TunerInputController;
import com.android.tv.tuner.util.TunerInputInfoUtils;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.Utils;
import com.android.tv.util.TvClock;

import com.droidlogic.app.SystemControlManager;
import com.droidlogic.app.tv.TvTime;
import com.droidlogic.app.tv.TvControlDataManager;

import java.util.List;
import java.util.concurrent.Executor;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * Live TV application.
 *
 * <p>This includes all the Google specific hooks.
 */
public abstract class TvApplication extends BaseApplication implements TvSingletons, Starter {
    private static final String TAG = "TvApplication";
    private static final boolean DEBUG = false;

    /** Namespace for LiveChannels configs. LiveChannels configs are kept in piper. */
    public static final String CONFIGNS_P4 = "configns:p4";

    /**
     * Broadcast Action: The user has updated LC to a new version that supports tuner input. {@link
     * TunerInputController} will receive this intent to check the existence of tuner input when the
     * new version is first launched.
     */
    public static final String ACTION_APPLICATION_FIRST_LAUNCHED =
            " com.android.tv.action.APPLICATION_FIRST_LAUNCHED";

    private static final String PREFERENCE_IS_FIRST_LAUNCH = "is_first_launch";

    private static final NamedThreadFactory THREAD_FACTORY = new NamedThreadFactory("tv-app-db");
    private static final ExecutorService DB_EXECUTOR =
            Executors.newSingleThreadExecutor(THREAD_FACTORY);

    private String mVersionName = "";

    private final MainActivityWrapper mMainActivityWrapper = new MainActivityWrapper();

    private SelectInputActivity mSelectInputActivity;
    private ChannelDataManager mChannelDataManager;
    private volatile ProgramDataManager mProgramDataManager;
    private PreviewDataManager mPreviewDataManager;
    private DvrManager mDvrManager;
    private DvrScheduleManager mDvrScheduleManager;
    private DvrDataManager mDvrDataManager;
    private DvrWatchedPositionManager mDvrWatchedPositionManager;
    private RecordingScheduler mRecordingScheduler;
    private RecordingStorageStatusManager mDvrStorageStatusManager;
    @Nullable private InputSessionManager mInputSessionManager;
    // STOP-SHIP: Remove this variable when Tuner Process is split to another application.
    // When this variable is null, we don't know in which process TvApplication runs.
    private Boolean mRunningInMainProcess;
    private TvInputManagerHelper mTvInputManagerHelper;
    private boolean mStarted;
    private EpgFetcher mEpgFetcher;
    private TunerInputController mTunerInputController;

    private SystemControlManager mSystemControlManager;
    private TvTime mTvTime;
    private TvControlDataManager mTvControlDataManager;
    private TvClock mTvClock;
    private boolean mRegisterDvrBroadcast = false;

    @Override
    public void onCreate() {
        super.onCreate();
        SharedPreferencesUtils.initialize(
                this,
                new Runnable() {
                    @Override
                    public void run() {
                        if (mRunningInMainProcess != null && mRunningInMainProcess) {
                            checkTunerServiceOnFirstLaunch();
                        }
                    }
                });
        try {
            PackageInfo pInfo = getPackageManager().getPackageInfo(getPackageName(), 0);
            mVersionName = pInfo.versionName;
        } catch (PackageManager.NameNotFoundException e) {
            Log.w(TAG, "Unable to find package '" + getPackageName() + "'.", e);
            mVersionName = "";
        }
        Log.i(TAG, "Starting Live TV " + getVersionName());

        // In SetupFragment, transitions are set in the constructor. Because the fragment can be
        // created in Activity.onCreate() by the framework, SetupAnimationHelper should be
        // initialized here before Activity.onCreate() is called.
        mEpgFetcher = EpgFetcherImpl.create(this);
        SetupAnimationHelper.initialize(this);
        getTvInputManagerHelper();

        handleInputCountChanged();//check inputlist in case that livetv can't receive callback

        Log.i(TAG, "Started Live TV " + mVersionName);
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("finish TvApplication.onCreate");
    }

    @Override
    public void onTerminate() {
        super.onTerminate();
        if (mRegisterDvrBroadcast && mDvrManager != null) {
            mRegisterDvrBroadcast = false;
            mDvrManager.unRegisterCommandReceiver();
        }
    }

    /** Initializes application. It is a noop if called twice. */
    @Override
    public void start() {
        if (mStarted) {
            return;
        }
        mStarted = true;
        mRunningInMainProcess = true;
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("start TvApplication.start");
        if (mRunningInMainProcess) {
            getTvInputManagerHelper()
                    .addCallback(
                            new TvInputCallback() {
                                @Override
                                public void onInputAdded(String inputId) {
                                    if (TvFeatures.TUNER.isEnabled(TvApplication.this)
                                            && TextUtils.equals(
                                                    inputId, getEmbeddedTunerInputId())) {
                                        TunerInputInfoUtils.updateTunerInputInfo(
                                                TvApplication.this);
                                    }
                                    handleInputCountChanged();
                                }

                                @Override
                                public void onInputRemoved(String inputId) {
                                    handleInputCountChanged();
                                }
                            });
            if (TvFeatures.TUNER.isEnabled(this)) {
                // If the tuner input service is added before the app is started, we need to
                // handle it here.
                TunerInputInfoUtils.updateTunerInputInfo(TvApplication.this);
            }
            if (CommonFeatures.DVR.isEnabled(this)) {
                mDvrScheduleManager = new DvrScheduleManager(this);
                mDvrManager = new DvrManager(this);
                mRecordingScheduler = RecordingScheduler.createScheduler(this);
                mDvrManager.registerCommandReceiver();
                mRegisterDvrBroadcast = false;
            }
            mEpgFetcher.startRoutineService();
            if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                     && !TextUtils.isEmpty(SystemPropertiesProxy.getString("ro.com.google.gmsversion", ""))) {
                ChannelPreviewUpdater.getInstance(this).startRoutineService();
                RecordedProgramPreviewUpdater.getInstance(this)
                        .updatePreviewDataForRecordedPrograms();
            }
        }
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("finish TvApplication.start");
    }

    private void checkTunerServiceOnFirstLaunch() {
        SharedPreferences sharedPreferences =
                this.getSharedPreferences(
                        SharedPreferencesUtils.SHARED_PREF_FEATURES, Context.MODE_PRIVATE);
        boolean isFirstLaunch = sharedPreferences.getBoolean(PREFERENCE_IS_FIRST_LAUNCH, true);
        if (isFirstLaunch) {
            if (DEBUG) Log.d(TAG, "Congratulations, it's the first launch!");
            getTunerInputController()
                    .onCheckingUsbTunerStatus(this, ACTION_APPLICATION_FIRST_LAUNCHED);
            SharedPreferences.Editor editor = sharedPreferences.edit();
            editor.putBoolean(PREFERENCE_IS_FIRST_LAUNCH, false);
            editor.apply();
        }
    }

    @Override
    public EpgFetcher getEpgFetcher() {
        return mEpgFetcher;
    }

    @Override
    public synchronized SetupUtils getSetupUtils() {
        return SetupUtils.createForTvSingletons(this);
    }

    /** Returns the {@link DvrManager}. */
    @Override
    public DvrManager getDvrManager() {
        return mDvrManager;
    }

    /** Returns the {@link DvrScheduleManager}. */
    @Override
    public DvrScheduleManager getDvrScheduleManager() {
        return mDvrScheduleManager;
    }

    /** Returns the {@link RecordingScheduler}. */
    @Override
    @Nullable
    public RecordingScheduler getRecordingScheduler() {
        return mRecordingScheduler;
    }

    /** Returns the {@link DvrWatchedPositionManager}. */
    /**
     * Returns the {@link SystemControlManager}.
     */
    @Override
    public SystemControlManager getSystemControlManager() {
        if (mSystemControlManager == null) {
            mSystemControlManager = SystemControlManager.getInstance();
        }
        return mSystemControlManager;
    }

    /**
     * Returns the {@link TvTime}.
     */
    @Override
    public TvTime getTvTime() {
        if (mTvTime == null) {
            mTvTime = new TvTime(getApplicationContext());
        }
        return mTvTime;
    }

    /**
     * Returns the {@link TvTime}.
     */
    @Override
    public TvClock getTvClock() {
        if (mTvClock == null) {
            mTvClock = new TvClock(getApplicationContext());
        }
        return mTvClock;
    }

    /**
     * Returns the {@link TvControlDataManager}.
     */
    @Override
    public TvControlDataManager getTvControlDataManager() {
        if (mTvControlDataManager == null) {
            mTvControlDataManager = TvControlDataManager.getInstance(getApplicationContext());
        }
        return mTvControlDataManager;
    }

    /**
     * Returns the {@link DvrWatchedPositionManager}.
     */
    @Override
    public DvrWatchedPositionManager getDvrWatchedPositionManager() {
        if (mDvrWatchedPositionManager == null) {
            mDvrWatchedPositionManager = new DvrWatchedPositionManager(this);
        }
        return mDvrWatchedPositionManager;
    }

    @Override
    @TargetApi(Build.VERSION_CODES.N)
    public InputSessionManager getInputSessionManager() {
        if (mInputSessionManager == null) {
            mInputSessionManager = new InputSessionManager(this);
        }
        return mInputSessionManager;
    }

    /** Returns {@link ChannelDataManager}. */
    @Override
    public ChannelDataManager getChannelDataManager() {
        if (mChannelDataManager == null) {
            mChannelDataManager = new ChannelDataManager(this, getTvInputManagerHelper());
            mChannelDataManager.start();
        }
        return mChannelDataManager;
    }

    @Override
    public boolean isChannelDataManagerLoadFinished() {
        return mChannelDataManager != null && mChannelDataManager.isDbLoadFinished();
    }

    /** Returns {@link ProgramDataManager}. */
    @Override
    public ProgramDataManager getProgramDataManager() {
        if (mProgramDataManager != null) {
            return mProgramDataManager;
        }
        Utils.runInMainThreadAndWait(
                new Runnable() {
                    @Override
                    public void run() {
                        if (mProgramDataManager == null) {
                            mProgramDataManager = new ProgramDataManager(TvApplication.this);
                            mProgramDataManager.start();
                        }
                    }
                });
        return mProgramDataManager;
    }

    @Override
    public boolean isProgramDataManagerCurrentProgramsLoadFinished() {
        return mProgramDataManager != null && mProgramDataManager.isCurrentProgramsLoadFinished();
    }

    /** Returns {@link PreviewDataManager}. */
    @TargetApi(Build.VERSION_CODES.O)
    @Override
    public PreviewDataManager getPreviewDataManager() {
        if (mPreviewDataManager == null) {
            mPreviewDataManager = new PreviewDataManager(this);
            mPreviewDataManager.start();
        }
        return mPreviewDataManager;
    }

    /** Returns {@link DvrDataManager}. */
    @TargetApi(Build.VERSION_CODES.N)
    @Override
    public DvrDataManager getDvrDataManager() {
        if (mDvrDataManager == null) {
            DvrDataManagerImpl dvrDataManager = new DvrDataManagerImpl(this, getTvClock()/*Clock.SYSTEM*/);
            mDvrDataManager = dvrDataManager;
            dvrDataManager.start();
        }
        return mDvrDataManager;
    }

    @Override
    @TargetApi(Build.VERSION_CODES.N)
    public RecordingStorageStatusManager getRecordingStorageStatusManager() {
        if (mDvrStorageStatusManager == null) {
            mDvrStorageStatusManager = new DvrStorageStatusManager(this);
        }
        return mDvrStorageStatusManager;
    }

    /** Returns the main activity information. */
    @Override
    public MainActivityWrapper getMainActivityWrapper() {
        return mMainActivityWrapper;
    }

    /** Returns {@link TvInputManagerHelper}. */
    @Override
    public TvInputManagerHelper getTvInputManagerHelper() {
        if (mTvInputManagerHelper == null) {
            mTvInputManagerHelper = new TvInputManagerHelper(this);
            mTvInputManagerHelper.start();
        }
        return mTvInputManagerHelper;
    }

    @Override
    public synchronized TunerInputController getTunerInputController() {
        if (mTunerInputController == null) {
            mTunerInputController =
                    new TunerInputController(
                            ComponentName.unflattenFromString(getEmbeddedTunerInputId()));
        }
        return mTunerInputController;
    }

    @Override
    public boolean isRunningInMainProcess() {
        return mRunningInMainProcess != null && mRunningInMainProcess;
    }

    /**
     * SelectInputActivity is set in {@link SelectInputActivity#onCreate} and cleared in {@link
     * SelectInputActivity#onDestroy}.
     */
    public void setSelectInputActivity(SelectInputActivity activity) {
        mSelectInputActivity = activity;
    }

    public void handleGuideKey() {
        if (!mMainActivityWrapper.isResumed()) {
            startActivity(new Intent(Intent.ACTION_VIEW, TvContract.Programs.CONTENT_URI));
        } else {
            mMainActivityWrapper.getMainActivity().getOverlayManager().toggleProgramGuide();
        }
    }

    /** Handles the global key KEYCODE_TV. */
    public void handleTvKey() {
        if (!mMainActivityWrapper.isResumed()) {
            startMainActivity(null);
        }
    }

    /** Handles the global key KEYCODE_DVR. */
    public void handleDvrKey() {
        startActivity(new Intent(this, DvrBrowseActivity.class));
    }

    /** Handles the global key KEYCODE_TV_INPUT. */
    public void handleTvInputKey() {
        TvInputManager tvInputManager = (TvInputManager) getSystemService(Context.TV_INPUT_SERVICE);
        List<TvInputInfo> tvInputs = tvInputManager.getTvInputList();
        int inputCount = 0;
        boolean hasTunerInput = false;
        for (TvInputInfo input : tvInputs) {
            if (input.isPassthroughInput()) {
                if (!input.isHidden(this)) {
                    ++inputCount;
                }
            } else if (!hasTunerInput) {
                hasTunerInput = true;
                ++inputCount;
            }
        }
        if (inputCount < 2) {
            return;
        }
        Activity activityToHandle =
                mMainActivityWrapper.isResumed()
                        ? mMainActivityWrapper.getMainActivity()
                        : mSelectInputActivity;
        if (activityToHandle != null) {
            // If startActivity is called, MainActivity.onPause is unnecessarily called. To
            // prevent it, MainActivity.dispatchKeyEvent is directly called.
            activityToHandle.dispatchKeyEvent(
                    new KeyEvent(KeyEvent.ACTION_DOWN, KeyEvent.KEYCODE_TV_INPUT));
            activityToHandle.dispatchKeyEvent(
                    new KeyEvent(KeyEvent.ACTION_UP, KeyEvent.KEYCODE_TV_INPUT));
        } else if (mMainActivityWrapper.isStarted()) {
            Bundle extras = new Bundle();
            extras.putString(Utils.EXTRA_KEY_ACTION, Utils.EXTRA_ACTION_SHOW_TV_INPUT);
            startMainActivity(extras);
        } else {
            startActivity(
                    new Intent(this, SelectInputActivity.class)
                            .setFlags(Intent.FLAG_ACTIVITY_NEW_TASK));
        }
    }

    private void startMainActivity(Bundle extras) {
        // The use of FLAG_ACTIVITY_NEW_TASK enables arbitrary applications to access the intent
        // sent to the root activity. Having said that, we should be fine here since such an intent
        // does not carry any important user data.
        Intent intent =
                new Intent(this, MainActivity.class).setFlags(Intent.FLAG_ACTIVITY_NEW_TASK);
        if (extras != null) {
            intent.putExtras(extras);
        }
        startActivity(intent);
    }

    /**
     * Returns the version name of the live channels.
     *
     * @see PackageInfo#versionName
     */
    public String getVersionName() {
        return mVersionName;
    }

    /**
     * Checks the input counts and enable/disable TvActivity. Also upda162 the input list in {@link
     * SetupUtils}.
     */
    @Override
    public void handleInputCountChanged() {
        handleInputCountChanged(false, false, false);
    }

    /**
     * Checks the input counts and enable/disable TvActivity. Also updates the input list in {@link
     * SetupUtils}.
     *
     * @param calledByTunerServiceChanged true if it is called when BaseTunerTvInputService is
     *     enabled or disabled.
     * @param tunerServiceEnabled it's available only when calledByTunerServiceChanged is true.
     * @param dontKillApp when TvActivity is enabled or disabled by this method, the app restarts by
     *     default. But, if dontKillApp is true, the app won't restart.
     */
    public void handleInputCountChanged(
            boolean calledByTunerServiceChanged, boolean tunerServiceEnabled, boolean dontKillApp) {
        TvInputManager inputManager = (TvInputManager) getSystemService(Context.TV_INPUT_SERVICE);
        boolean enable =
                (calledByTunerServiceChanged && tunerServiceEnabled)
                        || TvFeatures.UNHIDE.isEnabled(TvApplication.this);
        if (!enable) {
            List<TvInputInfo> inputs = inputManager.getTvInputList();
            boolean skipTunerInputCheck = false;
            // Enable the TvActivity only if there is at least one tuner type input.
            if (!skipTunerInputCheck) {
                for (TvInputInfo input : inputs) {
                    if (calledByTunerServiceChanged
                            && !tunerServiceEnabled
                            && getEmbeddedTunerInputId().equals(input.getId())) {
                        continue;
                    }
                    if (input.getType() == TvInputInfo.TYPE_TUNER) {
                        enable = true;
                        break;
                    }
                }
            }
            if (DEBUG) Log.d(TAG, "Enable MainActivity: " + enable);
        }
        PackageManager packageManager = getPackageManager();
        ComponentName name = new ComponentName(this, TvActivity.class);
        int newState =
                enable
                        ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                        : PackageManager.COMPONENT_ENABLED_STATE_DISABLED;
        if (packageManager.getComponentEnabledSetting(name) != newState) {
            packageManager.setComponentEnabledSetting(
                    name, newState, dontKillApp ? PackageManager.DONT_KILL_APP : 0);
            Log.i(TAG, (enable ? "Un-hide" : "Hide") + " Live TV.");
        }
        getSetupUtils().onInputListUpdated(inputManager);
    }

    @Override
    public Executor getDbExecutor() {
        return DB_EXECUTOR;
    }
}
