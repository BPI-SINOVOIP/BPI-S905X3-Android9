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

import android.app.Activity;
import android.app.PendingIntent;
import android.app.SearchManager;
import android.content.ActivityNotFoundException;
import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.ContentResolver;
import android.content.ContentUris;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.pm.PackageManager;
import android.content.res.Configuration;
import android.database.Cursor;
import android.hardware.display.DisplayManager;
import android.hardware.hdmi.HdmiDeviceInfo;
import android.media.tv.TvContentRating;
import android.media.tv.TvContract;
import android.media.tv.TvContract.Channels;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.media.tv.TvInputManager.TvInputCallback;
import android.media.tv.TvTrackInfo;
import android.media.tv.TvView.OnUnhandledInputEventListener;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.os.PowerManager;
import android.provider.Settings;
import android.support.annotation.IntDef;
import android.support.annotation.NonNull;
import android.support.annotation.Nullable;
import android.text.TextUtils;
import android.util.ArraySet;
import android.util.Log;
import android.view.Display;
import android.view.Gravity;
import android.view.InputEvent;
import android.view.KeyEvent;
import android.view.View;
import android.view.ViewGroup;
import android.view.ViewTreeObserver;
import android.view.Window;
import android.view.WindowManager;
import android.view.accessibility.AccessibilityEvent;
import android.view.accessibility.AccessibilityManager;
import android.widget.FrameLayout;
import android.widget.Toast;

import android.support.v17.leanback.widget.GuidedAction;

import com.android.tv.analytics.SendChannelStatusRunnable;
import com.android.tv.analytics.SendConfigInfoRunnable;
import com.android.tv.analytics.Tracker;
import com.android.tv.common.BuildConfig;
import com.android.tv.common.CommonPreferences;
import com.android.tv.common.SoftPreconditions;
import com.android.tv.common.CommonConstants;
import com.android.tv.common.TvContentRatingCache;
import com.android.tv.common.WeakHandler;
import com.android.tv.common.feature.CommonFeatures;
import com.android.tv.common.memory.MemoryManageable;
import com.android.tv.common.ui.setup.OnActionClickListener;
import com.android.tv.common.util.CommonUtils;
import com.android.tv.common.util.ContentUriUtils;
import com.android.tv.common.util.Debug;
import com.android.tv.common.util.DurationTimer;
import com.android.tv.common.util.PermissionUtils;
import com.android.tv.common.util.SystemProperties;
import com.android.tv.common.util.SystemPropertiesProxy;
import com.android.tv.data.ChannelDataManager;
import com.android.tv.data.ChannelImpl;
import com.android.tv.data.ChannelNumber;
import com.android.tv.data.OnCurrentProgramUpdatedListener;
import com.android.tv.data.Program;
import com.android.tv.data.ProgramDataManager;
import com.android.tv.data.StreamInfo;
import com.android.tv.data.WatchedHistoryManager;
import com.android.tv.data.api.Channel;
import com.android.tv.dialog.HalfSizedDialogFragment;
import com.android.tv.dialog.PinDialogFragment;
import com.android.tv.dialog.PinDialogFragment.OnPinCheckedListener;
import com.android.tv.dialog.SafeDismissDialogFragment;
import com.android.tv.droidlogic.ChannelSearchActivity;
import com.android.tv.dvr.DvrManager;
import com.android.tv.dvr.data.ScheduledRecording;
import com.android.tv.dvr.recorder.ConflictChecker;
import com.android.tv.dvr.ui.DvrStopRecordingFragment;
import com.android.tv.dvr.ui.DvrStopOrContinueRecordingFragment;
import com.android.tv.dvr.ui.DvrUiHelper;
import com.android.tv.menu.Menu;
import com.android.tv.onboarding.OnboardingActivity;
import com.android.tv.parental.ContentRatingsManager;
import com.android.tv.parental.ParentalControlSettings;
import com.android.tv.perf.EventNames;
import com.android.tv.perf.PerformanceMonitor;
import com.android.tv.perf.TimerEvent;
import com.android.tv.recommendation.ChannelPreviewUpdater;
import com.android.tv.recommendation.NotificationService;
import com.android.tv.search.ProgramGuideSearchFragment;
import com.android.tv.ui.ChannelBannerView;
import com.android.tv.ui.InputBannerView;
import com.android.tv.ui.KeypadChannelSwitchView;
import com.android.tv.ui.SelectInputView;
import com.android.tv.ui.SelectInputView.OnInputSelectedCallback;
import com.android.tv.ui.TunableTvView;
import com.android.tv.ui.TunableTvView.BlockScreenType;
import com.android.tv.ui.TunableTvView.OnTuneListener;
import com.android.tv.ui.TvOverlayManager;
import com.android.tv.ui.TvViewUiManager;
import com.android.tv.ui.sidepanel.ClosedCaptionFragment;
import com.android.tv.ui.sidepanel.CustomizeChannelListFragment;
import com.android.tv.ui.sidepanel.DeveloperOptionFragment;
import com.android.tv.ui.sidepanel.DisplayModeFragment;
import com.android.tv.ui.sidepanel.MultiAudioFragment;
import com.android.tv.ui.sidepanel.SettingsFragment;
import com.android.tv.ui.sidepanel.SideFragment;
import com.android.tv.ui.sidepanel.parentalcontrols.ParentalControlsFragment;
import com.android.tv.util.AsyncDbTask;
import com.android.tv.util.CaptionSettings;
import com.android.tv.util.OnboardingUtils;
import com.android.tv.util.RecurringRunner;
import com.android.tv.util.SetupUtils;
import com.android.tv.util.TvClock;
import com.android.tv.util.TvInputManagerHelper;
import com.android.tv.util.TvSettings;
import com.android.tv.util.TvTrackInfoUtils;
import com.android.tv.util.Utils;
import com.android.tv.util.ViewCache;
import com.android.tv.util.account.AccountHelper;
import com.android.tv.util.images.ImageCache;
import com.android.tv.InputSessionManager;

import com.android.tv.droidlogic.QuickKeyInfo;
import com.android.tv.droidlogic.quickkeyui.MultiOptionFragment;
import com.android.tv.droidlogic.subtitleui.SubtitleFragment;
import com.android.tv.droidlogic.subtitleui.TeleTextFragment;
import com.android.tv.droidlogic.subtitleui.TeleTextAdvancedSettings;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.concurrent.Executor;
import java.util.concurrent.TimeUnit;
import java.util.Iterator;
import android.hardware.hdmi.HdmiTvClient;
import android.hardware.hdmi.HdmiTvClient.InputChangeListener;
import android.hardware.hdmi.HdmiControlManager;

/** The main activity for the Live TV app. */
import com.droidlogic.app.tv.DroidLogicTvUtils;
import com.droidlogic.app.tv.TvDataBaseManager;
import com.droidlogic.app.tv.TvControlManager;
import com.droidlogic.app.tv.ChannelInfo;
import com.droidlogic.app.tv.TvScanConfig;
import com.droidlogic.app.tv.DroidLogicHdmiCecManager;
import android.provider.Settings;
/**
 * The main activity for the Live TV app.
 */
public class MainActivity extends Activity implements OnActionClickListener, OnPinCheckedListener {
    private static final String TAG = "MainActivity";
    private static final boolean DEBUG = true;

    @Retention(RetentionPolicy.SOURCE)
    @IntDef({
        KEY_EVENT_HANDLER_RESULT_PASSTHROUGH,
        KEY_EVENT_HANDLER_RESULT_NOT_HANDLED,
        KEY_EVENT_HANDLER_RESULT_HANDLED,
        KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY
    })
    public @interface KeyHandlerResultType {}

    public static final int KEY_EVENT_HANDLER_RESULT_PASSTHROUGH = 0;
    public static final int KEY_EVENT_HANDLER_RESULT_NOT_HANDLED = 1;
    public static final int KEY_EVENT_HANDLER_RESULT_HANDLED = 2;
    public static final int KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY = 3;

    public static final boolean USE_DROIDLOIC_CUSTOMIZATION = true;
    private static final String CC_OPTION = "CC_OPTION";
    private static final String CC_TRACKID = "CC_TRACKID";
    private static final String CC_LANGUAGE = "CC_LANGUAGE";

    private static final boolean USE_BACK_KEY_LONG_PRESS = false;

    private static final float FRAME_RATE_FOR_FILM = 23.976f;
    private static final float FRAME_RATE_EPSILON = 0.1f;

    private static final int PERMISSIONS_REQUEST_READ_TV_LISTINGS = 1;
    private static final String PERMISSION_READ_TV_LISTINGS = "android.permission.READ_TV_LISTINGS";

    private static final String BROADCAST_CHANGED_SEARCH_TYPE = "android.action.livetv.change.search.type";
    private static final String BROADCAST_SKIP_ALL_CHANNELS = "android.action.skip.all.channels";
    private static final String BROADCAST_DELETE_ALL_CHANNELS = "action.delete.all.channels";
    private static final String SYSTEM_DIALOG_REASON_KEY = "reason";
    private static final String SYSTEM_DIALOG_REASON_HOME_KEY = "homekey";

    // Tracker screen names.
    public static final String SCREEN_NAME = "Main";
    private static final String SCREEN_PIP = "PIP";
    private static final String SCREEN_BEHIND_NAME = "Behind";

    private static final float REFRESH_RATE_EPSILON = 0.01f;
    private static final HashSet<Integer> BLACKLIST_KEYCODE_TO_TIS;
    // These keys won't be passed to TIS in addition to gamepad buttons.
    static {
        BLACKLIST_KEYCODE_TO_TIS = new HashSet<>();
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_TV_INPUT);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_MENU);
//        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_CHANNEL_UP);
//        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_CHANNEL_DOWN);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_VOLUME_UP);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_VOLUME_DOWN);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_VOLUME_MUTE);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_MUTE);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_SEARCH);
        BLACKLIST_KEYCODE_TO_TIS.add(KeyEvent.KEYCODE_WINDOW);
    }

    private static final IntentFilter SYSTEM_INTENT_FILTER = new IntentFilter();

    static {
        SYSTEM_INTENT_FILTER.addAction(TvInputManager.ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED);
        SYSTEM_INTENT_FILTER.addAction(Intent.ACTION_SCREEN_OFF);
        SYSTEM_INTENT_FILTER.addAction(Intent.ACTION_SCREEN_ON);
        SYSTEM_INTENT_FILTER.addAction(Intent.ACTION_TIME_CHANGED);
        SYSTEM_INTENT_FILTER.addAction(BROADCAST_CHANGED_SEARCH_TYPE);
        SYSTEM_INTENT_FILTER.addAction(BROADCAST_SKIP_ALL_CHANNELS);
        SYSTEM_INTENT_FILTER.addAction(BROADCAST_DELETE_ALL_CHANNELS);
        SYSTEM_INTENT_FILTER.addAction(Intent.ACTION_CLOSE_SYSTEM_DIALOGS);
    }

    private static final int REQUEST_CODE_START_SETUP_ACTIVITY = 1;
    private static final int REQUEST_CODE_NOW_PLAYING = 2;

    private static final String KEY_INIT_CHANNEL_ID = "com.android.tv.init_channel_id";

    // Change channels with key long press.
    private static final int CHANNEL_CHANGE_NORMAL_SPEED_DURATION_MS = 3000;
    private static final int CHANNEL_CHANGE_DELAY_MS_IN_MAX_SPEED = 50;
    private static final int CHANNEL_CHANGE_DELAY_MS_IN_NORMAL_SPEED = 200;
    private static final int CHANNEL_CHANGE_INITIAL_DELAY_MILLIS = 500;

    private static final int MSG_CHANNEL_DOWN_PRESSED = 1000;
    private static final int MSG_CHANNEL_UP_PRESSED = 1001;
    private static final int MSG_TUNE_CHANNEL = 1003;
    private static final int MSG_TUNE_CHANNEL_SECOND_STAGE = 1004;
    private static final int MSG_TUNE_TO_CEC_DEV = 1005;
    private static final int MSG_START_TV = 1006;
    private static final int MSG_STOP_TV = 1007;
    private static final int MSG_UPDTE_CHANNEL_BANNER = 1008;

    private static final int TVVIEW_SET_MAIN_TIMEOUT_MS = 3000;

    // Lazy initialization.
    // Delay 1 second in order not to interrupt the first tune.
    private static final long LAZY_INITIALIZATION_DELAY = TimeUnit.SECONDS.toMillis(1);

    private static final int UNDEFINED_TRACK_INDEX = -1;
    private static final long START_UP_TIMER_RESET_THRESHOLD_MS = TimeUnit.SECONDS.toMillis(3);

    private AccessibilityManager mAccessibilityManager;
    private ChannelDataManager mChannelDataManager;
    private ProgramDataManager mProgramDataManager;
    private TvInputManagerHelper mTvInputManagerHelper;
    private ChannelTuner mChannelTuner;
    private final TvOptionsManager mTvOptionsManager = new TvOptionsManager(this);
    private TvViewUiManager mTvViewUiManager;
    private TimeShiftManager mTimeShiftManager;
    private Tracker mTracker;
    private final DurationTimer mMainDurationTimer = new DurationTimer();
    private final DurationTimer mTuneDurationTimer = new DurationTimer();
    private DvrManager mDvrManager;
    private ConflictChecker mDvrConflictChecker;
    private SetupUtils mSetupUtils;

    private View mContentView;
    private TunableTvView mTvView;
    private Bundle mTuneParams;
    @Nullable private Uri mInitChannelUri;
    @Nullable private String mParentInputIdWhenScreenOff;
    private boolean mScreenOffIntentReceived;
    private boolean mShowProgramGuide;
    private boolean mShowSelectInputView;
    private TvInputInfo mInputToSetUp;
    private final List<MemoryManageable> mMemoryManageables = new ArrayList<>();
    private MediaSessionWrapper mMediaSessionWrapper;
    private final MyOnTuneListener mOnTuneListener = new MyOnTuneListener();

    private String mInputIdUnderSetup;
    private boolean mIsSetupActivityCalledByPopup;
    private AudioManagerHelper mAudioManagerHelper;
    private boolean mTunePending;
    private boolean mDebugNonFullSizeScreen;
    private boolean mActivityResumed;
    private boolean mActivityStarted;
    private boolean mShouldTuneToTunerChannel;
    private boolean mUseKeycodeBlacklist;
    private boolean mShowLockedChannelsTemporarily;
    private boolean mBackKeyPressed;
    private boolean mNeedShowBackKeyGuide;
    private boolean mVisibleBehind;
    private boolean mShowNewSourcesFragment = true;
    private String mTunerInputId;
    private boolean mOtherActivityLaunched;
    private PerformanceMonitor mPerformanceMonitor;

    // If tv is wake up by cec playback.
    private boolean mCareWakeByCec;

    private boolean mIsInPIPMode;
    private boolean mIsFilmModeSet;
    private float mDefaultRefreshRate;

    private TvOverlayManager mOverlayManager;
    private TvControlManager mTvControlManager = null ;
    private PowerManager mPowerManager;

    // mIsCurrentChannelUnblockedByUser and mWasChannelUnblockedBeforeShrunkenByUser are used for
    // keeping the channel unblocking status while TV view is shrunken.
    private boolean mIsCurrentChannelUnblockedByUser;
    private boolean mWasChannelUnblockedBeforeShrunkenByUser;
    private Channel mChannelBeforeShrunkenTvView;
    private boolean mIsCompletingShrunkenTvView;

    private TvContentRating mLastAllowedRatingForCurrentChannel;
    private TvContentRating mAllowedRatingBeforeShrunken;

    private CaptionSettings mCaptionSettings;
    // Lazy initialization
    private boolean mLazyInitialized;

    private static final int MAX_RECENT_CHANNELS = 5;
    private final ArrayDeque<Long> mRecentChannels = new ArrayDeque<>(MAX_RECENT_CHANNELS);

    private RecurringRunner mSendConfigInfoRecurringRunner;
    private RecurringRunner mChannelStatusRecurringRunner;

    private final Handler mHandler = new MainActivityHandler(this);
    private final Set<OnActionClickListener> mOnActionClickListeners = new ArraySet<>();

    //new add variable
    public QuickKeyInfo mQuickKeyInfo;

    private HdmiTvClient mHdmiTvClient = null;
    private InputChangeListener mInputListener = null;
    private static final int EAS_IN_PROGRESS = 1;
    private static final int EAS_NOT_IN_PROGRESS = 0;

    //for TvClock
    private TvClock mClock;

    //private long channelIndex = 0;
    private boolean isOKPressed = false;
    DroidLogicHdmiCecManager hdmi_cec = null;
    private boolean mNeedSkipChannelUpAndDown = false;

    private TeleTextAdvancedSettings mTeleTextAdvancedSettings;

    private final BroadcastReceiver mBroadcastReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            switch (intent.getAction()) {
                case Intent.ACTION_SCREEN_OFF:
                    if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_OFF");
                    // We need to stop TvView, when the screen is turned off. If not and TIS uses
                    // MediaPlayer, a device may not go to the sleep mode and audio can be heard,
                    // because MediaPlayer keeps playing media by its wake lock.
                    mScreenOffIntentReceived = true;
                    markCurrentChannelDuringScreenOff();
                    stopAll(true);
                    break;
                case Intent.ACTION_SCREEN_ON:
                    if (DEBUG) Log.d(TAG, "Received ACTION_SCREEN_ON");
                    if (!mActivityResumed && mVisibleBehind) {
                        // ACTION_SCREEN_ON is usually called after onResume. But, if media is
                        // played under launcher with requestVisibleBehind(true), onResume will
                        // not be called. In this case, we need to resume TvView explicitly.
                        resumeTvIfNeeded();
                    }
                    break;
                case TvInputManager.ACTION_PARENTAL_CONTROLS_ENABLED_CHANGED:
                    if (DEBUG) Log.d(TAG, "Received parental control settings change");
                    applyParentalControlSettings();
                    checkChannelLockNeeded(mTvView, null);
                    break;
                case Intent.ACTION_TIME_CHANGED:
                    // Re-tune the current channel to prevent incorrect behavior of trick-play.
                    // See: b/37393628
                    if (mChannelTuner.getCurrentChannel() != null && !mChannelTuner.getCurrentChannel().isPassthrough()) {
                        tune(true);
                    }
                    break;
                case BROADCAST_CHANGED_SEARCH_TYPE:
                    //[DroidLogic]
                    //When change search type, switch channel at the same time.
                    //channelIndex = getChannelIndex();
                    long channelIndex = getChannelIdForAtvDtvMode();
                    if (DEBUG) Log.d(TAG, "**********BROADCAST_CHANGED_SEARCH_TYPE, channelIndex: " + channelIndex);
                    //Channel toChannel = mChannelTuner.getChannelByIndex(channelIndex);
                    Channel toChannel = mChannelTuner.getChannelById(channelIndex);
                    if (toChannel == null) {
                       toChannel = mChannelTuner.findNearestBrowsableChannel(0);
                    }
                    tuneToChannel(toChannel);
                    break;
                case BROADCAST_DELETE_ALL_CHANNELS:
                    if (DEBUG) Log.d(TAG, "BROADCAST_DELETE_ALL_CHANNELS");
                    stopTv();
                    mTvView.showNoDataBaseHint();
                    break;
                case BROADCAST_SKIP_ALL_CHANNELS:
                    //[DroidLogic]
                    //when all channels are deleted or skipped, stopTv.
                    //stopTv();
                    if (DEBUG) Log.d(TAG, "BROADCAST_SKIP_ALL_CHANNELS");
                    String searchInput = DroidLogicTvUtils.getSearchInputId(MainActivity.this);
                    if (searchInput != null && searchInput.startsWith(QuickKeyInfo.DTVKIT_PACKAGE)) {
                        if (DEBUG) Log.d(TAG, "dtvkit need switch hidden channel by number key");
                        return;
                    } else {
                        mTvView.showNoDataBaseHint();
                    }
                    break;
                case Intent.ACTION_CLOSE_SYSTEM_DIALOGS:
                    String reason = intent.getStringExtra(SYSTEM_DIALOG_REASON_KEY);
                    if (SYSTEM_DIALOG_REASON_HOME_KEY.equals(reason) && mTvView != null && mTvView.isPlaying()) {
                        stopTvInHandler();
                    }
                    break;
            }
        }
    };

    private final OnCurrentProgramUpdatedListener mOnCurrentProgramUpdatedListener =
            new OnCurrentProgramUpdatedListener() {
                @Override
                public void onCurrentProgramUpdated(long channelId, Program program) {
                    // Do not update channel banner by this notification
                    // when the time shifting is available.
                    if (mTimeShiftManager.isAvailable()) {
                        return;
                    }
                    Channel channel = mTvView.getCurrentChannel();
                    if (channel != null && channel.getId() == channelId) {
                        mOverlayManager.updateChannelBannerAndShowIfNeeded(
                                TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_UPDATE_INFO);
                        mMediaSessionWrapper.update(mTvView.isBlocked(), channel, program);
                    }
                }
            };

    private final ChannelTuner.Listener mChannelTunerListener =
            new ChannelTuner.Listener() {
                @Override
                public void onLoadFinished() {
                    Debug.getTimer(Debug.TAG_START_UP_TIMER)
                            .log("MainActivity.mChannelTunerListener.onLoadFinished");
                    mSetupUtils.markNewChannelsBrowsable();
                    if (mActivityResumed) {
                        resumeTvIfNeeded();
                    }
                    //need both browse channels and skipped channels
                    //mOverlayManager.onBrowsableChannelsUpdated();
                    //When switched tv_search_type, it will update and filter channel list in resumeTvIfNeeded;
                    //When load finished, send broadcast to tune to specific channel.
                    Log.d(TAG, "onLoadFinished");
                    mQuickKeyInfo.printCallBackTraceIfNeeded("onLoadFinished");
                    checkNeedTuneWhenUpdated();
                }

                @Override
                public void onBrowsableChannelListChanged() {
                    //need both browse channels and skipped channels
                    //mOverlayManager.onBrowsableChannelsUpdated();
                    //in case that channel cannot update on time
                    if (USE_DROIDLOIC_CUSTOMIZATION && mQuickKeyInfo.hasSearchedChannel() && isActivityResumed()) {
                        Channel channel = mQuickKeyInfo.getFirstSearchedChannel();
                        List<Channel> channellist = mChannelTuner.getBrowsableChannelList();
                        if (channel != null && channellist != null && channellist.size() > 0) {
                            if (mActivityResumed && channellist.contains(channel)) {
                                tuneToChannel(channel);
                            }
                        }
                    } else if (USE_DROIDLOIC_CUSTOMIZATION && mQuickKeyInfo.getChannelChangeStatus()) {
                        Channel currentChannel = mChannelDataManager.getChannel(ContentUriUtils.safeParseId(mQuickKeyInfo.getReturnedChannelUri()));
                        if (isChannelChangeKeyDownReceived() || currentChannel == null) {
                            if (DEBUG) Log.d(TAG, "onBrowsableChannelListChanged returned channel hasn't found!");
                            return;
                        }
                        mChannelTuner.moveToChannel(currentChannel);
                        mTvView.setCurrentChannel(currentChannel);
                        //hide no channel ui when changed
                        mTvView.hideNoDataBaseHint();
                        mOverlayManager.updateChannelBannerAndShowIfNeeded(TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
                        mQuickKeyInfo.resetReturnedChannel();
                    }
                    //When browselist updated, send broadcast to tune to specific channel.
                    Log.d(TAG, "onBrowsableChannelListChanged");
                    checkNeedTuneWhenUpdated();
                    mQuickKeyInfo.printCallBackTraceIfNeeded("onBrowsableChannelListChanged");
                }

                @Override
                public void onCurrentChannelUnavailable(Channel channel) {
                    //[Droidlogic Start]: PD#174133: don't do anything when channel deleted
                    /*if (mChannelTuner.moveToAdjacentBrowsableChannel(true)) {
                        tune(true);
                    } else {
                        stopTv("onCurrentChannelUnavailable()", false);
                    }*/
                    //[Droidlogic END]
                    //play next channel for other source
                    if (channel != null && channel.isOtherChannel()) {
                        Channel newChannel = mChannelTuner.getAdjacentBrowsableChannel(true);
                        if (newChannel != null && newChannel.isOtherChannel()) {
                            if (mChannelTuner.moveToAdjacentBrowsableChannel(true)) {
                                tune(true);
                            }
                        }
                    }
                }

                @Override
                public void onChannelChanged(Channel previousChannel, Channel currentChannel) {}

                @Override
                public void onAllChannelsListChanged(){
                    //need both browse channels and skipped channels
                    mOverlayManager.onAllChannelsUpdated();
                }
            };

    private final Runnable mRestoreMainViewRunnable =
            new Runnable() {
                @Override
                public void run() {
                    restoreMainTvView();
                }
            };
    private ProgramGuideSearchFragment mSearchFragment;

    //parameters relate with hdmi switch
    private boolean mIgnoreHdmiCecTvInput = true;

    private final TvInputCallback mTvInputCallback =
            new TvInputCallback() {
                @Override
                public void onInputAdded(String inputId) {
                    if (TvFeatures.TUNER.isEnabled(MainActivity.this)
                            && mTunerInputId.equals(inputId)
                            && CommonPreferences.shouldShowSetupActivity(MainActivity.this)) {
                        Intent intent =
                                TvSingletons.getSingletons(MainActivity.this)
                                        .getTunerSetupIntent(MainActivity.this);
                        startActivity(intent);
                        CommonPreferences.setShouldShowSetupActivity(MainActivity.this, false);
                        mSetupUtils.markAsKnownInput(mTunerInputId);
                    }
                    }

                @Override
                public void onInputRemoved(String inputId) {
                    if (DEBUG) Log.d(TAG, "onInputRemoved " + inputId);
                }

                @Override
                public void onInputStateChanged(String inputId, int state) {
                    //INPUT_STATE_CONNECTED = 0, INPUT_STATE_CONNECTED_STANDBY = 1, INPUT_STATE_DISCONNECTED = 2
                    if (DEBUG) Log.d(TAG, "onInputStateChanged " + inputId + ", state: " + state);
                    final Channel channel = getCurrentChannel();
                    if (channel != null && channel.getInputId().equals(inputId)) {
                        if (state != TvInputManager.INPUT_STATE_CONNECTED) {
                            mQuickKeyInfo.setNoSingalTimeout();
                        } else {
                            mQuickKeyInfo.cancelNoSingalTimeout();
                        }
                    }
                }
            };

    //switch to cec by TvInputInfo
    private void tuneToCecDev(final TvInputInfo info) {
        Log.d(TAG,"tuneToCecDev:"+info);
        if (info != null && info.getParentId() != null) {

            Channel currentChannel = mChannelTuner.getCurrentChannel();
            String currentId = "";
            String devId = info.getId();
            String devParentId = info.getParentId();
            if (currentChannel != null) {
                currentId = currentChannel.getInputId();
            }
            Log.d(TAG,"tuneToCecDev currentChannel:" + currentId + " devId " + devId + " devParentId " + devParentId);

            if (currentId != devId && (currentId != devParentId)) {
                Log.d(TAG, "tuneToCecDev by ParentId = " + devParentId);
                DroidLogicTvUtils.setCurrentInputId(MainActivity.this, devId);
                mHandler.sendMessage(Message.obtain(mHandler, MSG_TUNE_TO_CEC_DEV, devParentId));
            }
        }
    }

    //get first cec tvinputinfo
    private TvInputInfo getFirstCecTvInputInfo() {
        TvInputInfo target = null;
        List<TvInputInfo> inputlist = mTvInputManagerHelper.getTvInputInfos(false, false);
        for (TvInputInfo tvinputinfo : inputlist) {
            HdmiDeviceInfo hdmidevinfo = tvinputinfo.getHdmiDeviceInfo();
            if (hdmidevinfo != null && hdmidevinfo.isCecDevice()) {
                target = tvinputinfo;
                break;
            }
        }
        return target;
    }

    private void checkNeedTuneWhenUpdated() {
        int searchTypeChanged = getSearchTypeChangedStatus();
        String lastplayuristring = Utils.getLastWatchedChannelUri(MainActivity.this);
        Uri lastplayuri = null;
        if (lastplayuristring != null) {
            lastplayuri = Uri.parse(lastplayuristring);
        }
        final Channel currentone = getCurrentChannel();
        Log.d(TAG, "checkNeedTuneWhenUpdated uri = " + lastplayuristring);
        if (searchTypeChanged == 1) {
            Log.d(TAG, "checkNeedTuneWhenUpdated searchTypeChanged : " + searchTypeChanged);
            long channelIndex = getChannelIdForAtvDtvMode();
            Channel toChannel = mChannelTuner.getChannelById(channelIndex);
            Channel preparechannel = null;
            if (channelIndex == -1) {
                /*List<Channel> channels = mChannelTuner.getBrowsableChannelList();
                for (int i = 0; i < channels.size(); i++) {
                    Log.d(TAG, "channels " + i + " = " + channels.get(i));
                }*/
                preparechannel = mChannelTuner.findNearestBrowsableChannel(0);
                Log.d(TAG, "preparechannel=" + preparechannel);
                if (preparechannel != null) {
                    if (!mQuickKeyInfo.isChannelMatchAtvDtvSource(preparechannel)) {
                        preparechannel = null;
                    }
                }
            }
            if (preparechannel == null && (toChannel == null || (lastplayuri != null && TvContract.isChannelUriForPassthroughInput(lastplayuri))
                    || (!toChannel.isPassthrough() && !mQuickKeyInfo.isChannelMatchAtvDtvSource(toChannel)))) {
                Log.d(TAG, "checkNeedTuneWhenUpdated channel has not prepared or is passthrough");
                return;
            }
            Intent intent = new Intent();
            intent.setAction(BROADCAST_CHANGED_SEARCH_TYPE);
            sendBroadcast(intent);
            resetSearchTypeChangedStatus();
        }/* else if (lastplayuri == null || (currentone == null && lastplayuri != null && !TvContract.isChannelUriForPassthroughInput(lastplayuri))
                    || (lastplayuri != null && !TvContract.isChannelUriForPassthroughInput(lastplayuri))) {
            Log.d(TAG, "checkNeedTuneWhenUpdated tune as channel may be available");
            long channelIndex = getChannelIdForAtvDtvMode();
            Channel toChannel = mChannelTuner.getChannelById(channelIndex);
            tuneToChannel(toChannel);
        }*/
    }

    public boolean useAtvDtvChannelIndex() {
        long channelindex = getChannelIdForAtvDtvMode();
        if (channelindex == Channel.INVALID_ID) {
            Log.w(TAG, "useAtvDtvChannelIndex need a invalid channeluri");
        }
        Uri channelUri = TvContract.buildChannelUri(channelindex);
        Utils.setLastWatchedChannelUri(this, channelUri.toString());

        return true;
    }

    public boolean usePassthroughIndex(String inputId) {
        if (inputId == null) {
            return false;
        }
        ChannelImpl passthroughchannel = ChannelImpl.createPassthroughChannel(inputId);
        Utils.setLastWatchedChannelUri(this, passthroughchannel.getUri().toString());

        return true;
    }

    private void applyParentalControlSettings() {
        boolean parentalControlEnabled =
                mTvInputManagerHelper.getParentalControlSettings().isParentalControlsEnabled();
        mTvView.onParentalControlChanged(parentalControlEnabled);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O
                && !TextUtils.isEmpty(SystemPropertiesProxy.getString("ro.com.google.gmsversion", ""))) {
            ChannelPreviewUpdater.getInstance(this).updatePreviewDataForChannelsImmediately();
        }
    }

    /*public void setChannelIndex(int deletedChannelIndex) {
        if (deletedChannelIndex <= channelIndex) {
            channelIndex--;
        }
        //saveChannelIndex(channelIndex);
        if (DEBUG) Log.d(TAG, "setChannelIndex=======channelIndex: " + channelIndex);
    }*/

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        mAccessibilityManager =
                (AccessibilityManager) getSystemService(Context.ACCESSIBILITY_SERVICE);
        TvSingletons tvSingletons = TvSingletons.getSingletons(this);
        mPerformanceMonitor = tvSingletons.getPerformanceMonitor();
        TimerEvent timer = mPerformanceMonitor.startTimer();
        DurationTimer startUpDebugTimer = Debug.getTimer(Debug.TAG_START_UP_TIMER);
        if (!startUpDebugTimer.isStarted()
                || startUpDebugTimer.getDuration() > START_UP_TIMER_RESET_THRESHOLD_MS) {
            // TvApplication can start by other reason before MainActivty is launched.
            // In this case, we restart the timer.
            startUpDebugTimer.start();
        }
        startUpDebugTimer.log("MainActivity.onCreate");
        if (DEBUG) {
            Log.d(TAG, "onCreate()");
        }
        Starter.start(this);
        super.onCreate(savedInstanceState);
        if (!tvSingletons.getTvInputManagerHelper().hasTvInputManager()) {
            Log.wtf(TAG, "Stopping because device does not have a TvInputManager");
            finishAndRemoveTask();
            return;
        }
        mPerformanceMonitor = tvSingletons.getPerformanceMonitor();
        mSetupUtils = tvSingletons.getSetupUtils();

        TvApplication tvApplication = (TvApplication) getApplication();
        mChannelDataManager = tvApplication.getChannelDataManager();
        // In API 23, TvContract.isChannelUriForPassthroughInput is hidden.
        boolean isPassthroughInput =
                TvContract.isChannelUriForPassthroughInput(getIntent().getData());
        boolean tuneToPassthroughInput =
                Intent.ACTION_VIEW.equals(getIntent().getAction()) && isPassthroughInput;
        boolean channelLoadedAndNoChannelAvailable =
                mChannelDataManager.isDbLoadFinished()
                        && mChannelDataManager.getChannelCount() <= 0;
        /*if ((OnboardingUtils.isFirstRunWithCurrentVersion(this)
                || channelLoadedAndNoChannelAvailable)
                && !tuneToPassthroughInput
                && !CommonUtils.isRunningInTest()) {
            startOnboardingActivity();
            return;
        }*/
        setContentView(R.layout.activity_tv);
        mProgramDataManager = tvApplication.getProgramDataManager();
        mTvInputManagerHelper = tvApplication.getTvInputManagerHelper();
        mTvView = (TunableTvView) findViewById(R.id.main_tunable_tv_view);
        mTvView.initialize(mProgramDataManager, mTvInputManagerHelper);
        mTvView.setOnUnhandledInputEventListener(
                new OnUnhandledInputEventListener() {
                    @Override
                    public boolean onUnhandledInputEvent(InputEvent event) {
                        if (DEBUG) {
                            Log.d(TAG, "onUnhandledInputEvent " + event);
                        }
                        if (isKeyEventBlocked()) {
                            Log.d(TAG, "need to transfer in passthrough");
                            //return true;
                        }
                        if (event instanceof KeyEvent) {
                            KeyEvent keyEvent = (KeyEvent) event;
                            if (keyEvent.getAction() == KeyEvent.ACTION_DOWN
                                    && keyEvent.isLongPress()) {
                                if (onKeyLongPress(keyEvent.getKeyCode(), keyEvent)) {
                                    return true;
                                }
                            }
                            if (keyEvent.getAction() == KeyEvent.ACTION_UP) {
                                return onKeyUp(keyEvent.getKeyCode(), keyEvent);
                            } else if (keyEvent.getAction() == KeyEvent.ACTION_DOWN) {
                                return onKeyDown(keyEvent.getKeyCode(), keyEvent);
                            }
                        }
                        return false;
                    }
                });
        //mTvView.setOnTalkBackDpadKeyListener(keycode -> handleUpDownKeys(keycode, null));
        long channelId = Utils.getLastWatchedChannelId(this);
        String inputId = Utils.getLastWatchedTunerInputId(this);
        if (!isPassthroughInput
                && inputId != null
                && channelId != Channel.INVALID_ID) {
            //mTvView.warmUpInput(inputId, TvContract.buildChannelUri(channelId));
        }

        tvApplication.getMainActivityWrapper().onMainActivityCreated(this);
        if (BuildConfig.ENG && SystemProperties.ALLOW_STRICT_MODE.getValue()) {
            Toast.makeText(this, "Using Strict Mode for eng builds", Toast.LENGTH_SHORT).show();
        }
        mTracker = tvApplication.getTracker();
        if (TvFeatures.TUNER.isEnabled(this)) {
            mTvInputManagerHelper.addCallback(mTvInputCallback);
        }
        mTunerInputId = tvSingletons.getEmbeddedTunerInputId();
        mProgramDataManager.addOnCurrentProgramUpdatedListener(
                Channel.INVALID_ID, mOnCurrentProgramUpdatedListener);
        mProgramDataManager.setPrefetchEnabled(true);
        mChannelTuner = new ChannelTuner(mChannelDataManager, mTvInputManagerHelper);
        mChannelTuner.addListener(mChannelTunerListener);
        mChannelTuner.start();
        mChannelTuner.setContext(this);
        mQuickKeyInfo = new QuickKeyInfo(MainActivity.this, mTvInputManagerHelper, mChannelTuner);
        mQuickKeyInfo.registerCommandReceiver();
        mMemoryManageables.add(mProgramDataManager);
        mMemoryManageables.add(ImageCache.getInstance());
        mMemoryManageables.add(TvContentRatingCache.getInstance());
        if (CommonFeatures.DVR.isEnabled(this)) {
            mDvrManager = tvApplication.getDvrManager();
        }
        mTvControlManager = TvControlManager.getInstance();
        mPowerManager = (PowerManager) getSystemService(Context.POWER_SERVICE);
        mTimeShiftManager =
                new TimeShiftManager(
                        this,
                        mTvView,
                        mProgramDataManager,
                        mTracker,
                        new OnCurrentProgramUpdatedListener() {
                            @Override
                            public void onCurrentProgramUpdated(long channelId, Program program) {
                                mMediaSessionWrapper.update(
                                        mTvView.isBlocked(), getCurrentChannel(), program);
                                switch (mTimeShiftManager.getLastActionId()) {
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_REWIND:
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_FAST_FORWARD:
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_PREVIOUS:
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_JUMP_TO_NEXT:
                                        mOverlayManager.updateChannelBannerAndShowIfNeeded(
                                                TvOverlayManager
                                                        .UPDATE_CHANNEL_BANNER_REASON_FORCE_SHOW);
                                        break;
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_PAUSE:
                                    case TimeShiftManager.TIME_SHIFT_ACTION_ID_PLAY:
                                    default:
                                        mOverlayManager.updateChannelBannerAndShowIfNeeded(
                                                TvOverlayManager
                                                        .UPDATE_CHANNEL_BANNER_REASON_UPDATE_INFO);
                                        break;
                                }
                            }
                        });

        DisplayManager displayManager = (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
        Display display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);
        mDefaultRefreshRate = display.getRefreshRate();

        if (!PermissionUtils.hasAccessWatchedHistory(this)) {
            WatchedHistoryManager watchedHistoryManager =
                    new WatchedHistoryManager(getApplicationContext());
            watchedHistoryManager.start();
            mTvView.setWatchedHistoryManager(watchedHistoryManager);
        }
        mTvViewUiManager =
                new TvViewUiManager(
                        this,
                        mTvView,
                        (FrameLayout) findViewById(android.R.id.content),
                        mTvOptionsManager);

        mContentView = findViewById(android.R.id.content);
        ViewGroup sceneContainer = (ViewGroup) findViewById(R.id.scene_container);
        ChannelBannerView channelBannerView =
                (ChannelBannerView)
                        getLayoutInflater().inflate(R.layout.channel_banner, sceneContainer, false);
        KeypadChannelSwitchView keypadChannelSwitchView =
                (KeypadChannelSwitchView)
                        getLayoutInflater()
                                .inflate(R.layout.keypad_channel_switch, sceneContainer, false);
        InputBannerView inputBannerView =
                (InputBannerView)
                        getLayoutInflater().inflate(R.layout.input_banner, sceneContainer, false);
        SelectInputView selectInputView =
                (SelectInputView)
                        getLayoutInflater().inflate(R.layout.select_input, sceneContainer, false);
        selectInputView.setOnInputSelectedCallback(
                new OnInputSelectedCallback() {
                    @Override
                    public void onTunerInputSelected() {
                        Channel currentChannel = mChannelTuner.getCurrentChannel();
                        if (currentChannel != null && !currentChannel.isPassthrough()) {
                            hideOverlays();
                        } else {
                            tuneToLastWatchedChannelForTunerInput();
                        }
                    }

                    @Override
                    public void onPassthroughInputSelected(@NonNull TvInputInfo input) {
                        Channel currentChannel = mChannelTuner.getCurrentChannel();
                        String currentInputId =
                                currentChannel == null ? null : currentChannel.getInputId();
                        if (TextUtils.equals(input.getId(), currentInputId)) {
                            hideOverlays();
                        } else {
                            tuneToChannel(ChannelImpl.createPassthroughChannel(input.getId()));
                        }
                    }

                    private void hideOverlays() {
                        getOverlayManager()
                                .hideOverlays(
                                        TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_DIALOG
                                                | TvOverlayManager
                                                        .FLAG_HIDE_OVERLAYS_KEEP_SIDE_PANELS
                                                | TvOverlayManager
                                                        .FLAG_HIDE_OVERLAYS_KEEP_PROGRAM_GUIDE
                                                | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_MENU
                                                | TvOverlayManager
                                                        .FLAG_HIDE_OVERLAYS_KEEP_FRAGMENT);
                    }
                });
        mSearchFragment = new ProgramGuideSearchFragment();
        mOverlayManager =
                new TvOverlayManager(
                        this,
                        mChannelTuner,
                        mTvView,
                        mTvOptionsManager,
                        keypadChannelSwitchView,
                        channelBannerView,
                        inputBannerView,
                        selectInputView,
                        sceneContainer,
                        mSearchFragment);
        mAccessibilityManager.addAccessibilityStateChangeListener(mOverlayManager);

        mAudioManagerHelper = new AudioManagerHelper(this, mTvView);
        Intent nowPlayingIntent = new Intent(this, MainActivity.class);
        PendingIntent pendingIntent =
                PendingIntent.getActivity(this, REQUEST_CODE_NOW_PLAYING, nowPlayingIntent, 0);
        mMediaSessionWrapper = new MediaSessionWrapper(this, pendingIntent);
        mAccessibilityManager =
                (AccessibilityManager) getSystemService(Context.ACCESSIBILITY_SERVICE);

        mTvViewUiManager.restoreDisplayMode(false);
        if (!handleIntent(getIntent())) {
            finish();
            return;
        }

        mSendConfigInfoRecurringRunner =
                new RecurringRunner(
                        this,
                        TimeUnit.DAYS.toMillis(1),
                        new SendConfigInfoRunnable(mTracker, mTvInputManagerHelper),
                        null);
        mSendConfigInfoRecurringRunner = new RecurringRunner(this, TimeUnit.DAYS.toMillis(1),
                new SendConfigInfoRunnable(mTracker, mTvInputManagerHelper), null);
        mSendConfigInfoRecurringRunner.start();
        mChannelStatusRecurringRunner =
                SendChannelStatusRunnable.startChannelStatusRecurringRunner(
                        this, mTracker, mChannelDataManager);

        // To avoid not updating Rating systems when changing language.
        mTvInputManagerHelper.getContentRatingsManager().update();
        if (CommonFeatures.DVR.isEnabled(this)
                && TvFeatures.SHOW_UPCOMING_CONFLICT_DIALOG.isEnabled(this)) {
            mDvrConflictChecker = new ConflictChecker(this);
        }
        initForTest();

        mInputListener = new InputChangeListener() {
            @Override
            public void onChanged(HdmiDeviceInfo info) {
                Log.d(TAG,"onChanged:"+info);
                if (info == null)
                    return ;
                /*
                if (mIgnoreHdmiCecTvInput) {
                    if (DEBUG)
                        Log.d(TAG,"mIgnoreHdmiCecTvInput: " + mIgnoreHdmiCecTvInput);
                    return;
                }
                */
                if (info.isCecDevice() && mTvView.getEasStatus() == EAS_NOT_IN_PROGRESS)  {
                    List<TvInputInfo> input_list = mTvInputManagerHelper.getTvInputList();
                    for (TvInputInfo tvinputinfo : input_list) {
                        Log.d(TAG,"tvinputinfo:"+tvinputinfo);
                        HdmiDeviceInfo hdmidevinfo = tvinputinfo.getHdmiDeviceInfo();
                        Log.d(TAG,"hdmidevinfo:"+hdmidevinfo);
                        if (hdmidevinfo != null && hdmidevinfo.isCecDevice() && hdmidevinfo.getLogicalAddress() == info.getLogicalAddress()) {
                            tuneToCecDev(tvinputinfo);
                            return;
                        }
                    }
                }
            }
        };
        if (mHdmiTvClient == null) {
            HdmiControlManager hdmiManager = (HdmiControlManager) getSystemService(Context.HDMI_CONTROL_SERVICE);
            if (hdmiManager == null) return;
            mHdmiTvClient = hdmiManager.getTvClient();
            if (mHdmiTvClient != null) {
                mHdmiTvClient.setInputChangeListener(mInputListener);
            }
        }
        hdmi_cec = DroidLogicHdmiCecManager.getInstance(this);
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("MainActivity.onCreate end");
        mPerformanceMonitor.stopTimer(timer, EventNames.MAIN_ACTIVITY_ONCREATE);

        mClock = new TvClock(this);
        mTeleTextAdvancedSettings = new TeleTextAdvancedSettings(this, mTvView.getTvView());
    }

    private void startOnboardingActivity() {
        startActivity(OnboardingActivity.buildIntent(this, getIntent()));
        finish();
    }

    @Override
    public void onConfigurationChanged(Configuration newConfig) {
        super.onConfigurationChanged(newConfig);
        float density = getResources().getDisplayMetrics().density;
        mTvViewUiManager.onConfigurationChanged(
                (int) (newConfig.screenWidthDp * density),
                (int) (newConfig.screenHeightDp * density));
    }

    @Override
    public void onRequestPermissionsResult(
            int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSIONS_REQUEST_READ_TV_LISTINGS) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                // Start reload of dependent data
                mChannelDataManager.reload();
                mProgramDataManager.reload();

                // Restart live channels.
                Intent intent = getIntent();
                finish();
                startActivity(intent);
            } else {
                Toast.makeText(
                                this,
                                R.string.msg_read_tv_listing_permission_denied,
                                Toast.LENGTH_LONG)
                        .show();
                finish();
            }
        }
    }

    @BlockScreenType
    private int getDesiredBlockScreenType() {
        if (!mActivityResumed) {
            return TunableTvView.BLOCK_SCREEN_TYPE_NO_UI;
        }
        if (isUnderShrunkenTvView()) {
            return TunableTvView.BLOCK_SCREEN_TYPE_SHRUNKEN_TV_VIEW;
        }
        if (mOverlayManager.needHideTextOnMainView()) {
            return TunableTvView.BLOCK_SCREEN_TYPE_NO_UI;
        }
        SafeDismissDialogFragment currentDialog = mOverlayManager.getCurrentDialog();
        if (currentDialog != null) {
            // If PIN dialog is shown for unblocking the channel lock or content ratings lock,
            // keeping the unlocking message is more natural instead of changing it.
            if (currentDialog instanceof PinDialogFragment) {
                int type = ((PinDialogFragment) currentDialog).getType();
                if (type == PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_CHANNEL
                        || type == PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_PROGRAM) {
                    return TunableTvView.BLOCK_SCREEN_TYPE_NORMAL;
                }
            }
            return TunableTvView.BLOCK_SCREEN_TYPE_NO_UI;
        }
        if (mOverlayManager.isSetupFragmentActive()
                || mOverlayManager.isNewSourcesFragmentActive()) {
            return TunableTvView.BLOCK_SCREEN_TYPE_NO_UI;
        }
        return TunableTvView.BLOCK_SCREEN_TYPE_NORMAL;
    }

    @Override
    protected void onNewIntent(Intent intent) {
        if (DEBUG) {
            Log.d(TAG, "onNewIntent(): " + intent);
        }
        if (mOverlayManager == null) {
            // It's called before onCreate. The intent will be handled at onCreate. b/30725058
            return;
        }
        mOverlayManager.getSideFragmentManager().hideAll(false);
        if (!handleIntent(intent) && !mActivityStarted) {
            // If the activity is stopped and not destroyed, finish the activity.
            // Otherwise, just ignore the intent.
            finish();
        }
    }

    @Override
    protected void onStart() {
        TimerEvent timer = mPerformanceMonitor.startTimer();
        if (DEBUG) {
            Log.d(TAG, "onStart()");
        }
        super.onStart();
        mScreenOffIntentReceived = false;
        mActivityStarted = true;
        mTracker.sendMainStart();
        mMainDurationTimer.start();

        applyParentalControlSettings();
        registerReceiver(mBroadcastReceiver, SYSTEM_INTENT_FILTER);

        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.O) {
            Intent notificationIntent = new Intent(this, NotificationService.class);
            notificationIntent.setAction(NotificationService.ACTION_SHOW_RECOMMENDATION);
            startService(notificationIntent);
        }
        TvSingletons singletons = TvSingletons.getSingletons(this);
        singletons.getTunerInputController().executeNetworkTunerDiscoveryAsyncTask(this);
        singletons.getEpgFetcher().fetchImmediatelyIfNeeded();
        mPerformanceMonitor.stopTimer(timer, EventNames.MAIN_ACTIVITY_ONSTART);
    }

    @Override
    protected void onResume() {
        TimerEvent timer = mPerformanceMonitor.startTimer();
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("MainActivity.onResume start");
        if (DEBUG) Log.d(TAG, "onResume()");
        super.onResume();
        mIsInPIPMode = false;
        if (!PermissionUtils.hasAccessAllEpg(this)
                && checkSelfPermission(PERMISSION_READ_TV_LISTINGS)
                        != PackageManager.PERMISSION_GRANTED) {
            requestPermissions(
                    new String[] {PERMISSION_READ_TV_LISTINGS},
                    PERMISSIONS_REQUEST_READ_TV_LISTINGS);
        }
        mTracker.sendScreenView(SCREEN_NAME);

        SystemProperties.updateSystemProperties();
        mNeedShowBackKeyGuide = true;
        mActivityResumed = true;
        mShowNewSourcesFragment = true;
        mOtherActivityLaunched = false;
        mAudioManagerHelper.requestAudioFocus();

        if (mTvView.isPlaying()) {
            // Every time onResume() is called the activity will be assumed to not have requested
            // visible behind.
            requestVisibleBehind(true);
        }
        Set<String> failedScheduledRecordingInfoSet =
                Utils.getFailedScheduledRecordingInfoSet(getApplicationContext());
        if (Utils.hasRecordingFailedReason(
                        getApplicationContext(), TvInputManager.RECORDING_ERROR_INSUFFICIENT_SPACE)
                && !failedScheduledRecordingInfoSet.isEmpty()) {
            runAfterAttachedToWindow(
                    new Runnable() {
                        @Override
                        public void run() {
                            DvrUiHelper.showDvrInsufficientSpaceErrorDialog(
                                    MainActivity.this, failedScheduledRecordingInfoSet);
                        }
                    });
        }

        if (mChannelTuner.areAllChannelsLoaded()) {
            mSetupUtils.markNewChannelsBrowsable();
            resumeTvIfNeeded();
        }
        mOverlayManager.showMenuWithTimeShiftPauseIfNeeded();

        // NOTE: The following codes are related to pop up an overlay UI after resume. When
        // the following code is changed, please modify willShowOverlayUiWhenResume() accordingly.
        if (mInputToSetUp != null) {
            startSetupActivity(mInputToSetUp, false);
            mInputToSetUp = null;
        } else if (mShowProgramGuide) {
            mShowProgramGuide = false;
            // This will delay the start of the animation until after the Live Channel app is
            // shown. Without this the animation is completed before it is actually visible on
            // the screen.
            mHandler.post(
                    new Runnable() {
                        @Override
                        public void run() {
                            mOverlayManager.showProgramGuide();
                        }
                    });
        } else if (mShowSelectInputView) {
            mShowSelectInputView = false;
            // mShowSelectInputView is true when the activity is started/resumed because the
            // TV_INPUT button was pressed in a different app.  This will delay the start of
            // the animation until after the Live Channel app is shown. Without this the
            // animation is completed before it is actually visible on the screen.
            mHandler.post(
                    new Runnable() {
                        @Override
                        public void run() {
                            mOverlayManager.showSelectInputView();
                        }
                    });
        }
        if (mDvrConflictChecker != null) {
            mDvrConflictChecker.start();
        }
        Debug.getTimer(Debug.TAG_START_UP_TIMER).log("MainActivity.onResume end");
        mPerformanceMonitor.stopTimer(timer, EventNames.MAIN_ACTIVITY_ONRESUME);
        mQuickKeyInfo.dealOnResume();
    }

    @Override
    protected void onPause() {
        if (DEBUG) Log.d(TAG, "onPause()");
        if (mDvrConflictChecker != null) {
            mDvrConflictChecker.stop();
        }
        finishChannelChangeIfNeeded();
        mActivityResumed = false;
        mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_DEFAULT);
        mTvView.setBlockScreenType(TunableTvView.BLOCK_SCREEN_TYPE_NO_UI);
        mBackKeyPressed = false;
        mShowLockedChannelsTemporarily = false;
        mShouldTuneToTunerChannel = false;
        if (!mVisibleBehind) {
            if (mIsInPIPMode) {
                mTracker.sendScreenView(SCREEN_PIP);
            } else {
                mTracker.sendScreenView("");
                mAudioManagerHelper.abandonAudioFocus();
                mMediaSessionWrapper.setPlaybackState(false);
            }
        } else {
            mTracker.sendScreenView(SCREEN_BEHIND_NAME);
        }
        TvSingletons.getSingletons(this).getExperimentLoader().asyncRefreshExperiments(this);
        mQuickKeyInfo.cancelNoSingalTimeout();
        mQuickKeyInfo.dealOnPause();
        super.onPause();
    }

    /** Returns true if {@link #onResume} is called and {@link #onPause} is not called yet. */
    public boolean isActivityResumed() {
        return mActivityResumed;
    }

    /** Returns true if {@link #onStart} is called and {@link #onStop} is not called yet. */
    public boolean isActivityStarted() {
        return mActivityStarted;
    }

    @Override
    public boolean requestVisibleBehind(boolean enable) {
        boolean state = super.requestVisibleBehind(enable);
        mVisibleBehind = state;
        return state;
    }

    @Override
    public void onPinChecked(boolean checked, int type, String rating) {
        if (checked) {
            switch (type) {
                case PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_CHANNEL:
                    blockOrUnblockScreenByUser(mTvView, false);
                    mIsCurrentChannelUnblockedByUser = true;
                    break;
                case PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_PROGRAM:
                    TvContentRating unblockedRating = TvContentRating.unflattenFromString(rating);
                    mLastAllowedRatingForCurrentChannel = unblockedRating;
                    mTvView.unblockContent(unblockedRating);
                    break;
                case PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN:
                    mOverlayManager
                            .getSideFragmentManager()
                            .show(new ParentalControlsFragment(), false);
                    // fall through.
                case PinDialogFragment.PIN_DIALOG_TYPE_NEW_PIN:
                    mOverlayManager.getSideFragmentManager().showSidePanel(true);
                    break;
                default: // fall out
            }
        } else if (type == PinDialogFragment.PIN_DIALOG_TYPE_ENTER_PIN) {
            mOverlayManager.getSideFragmentManager().hideAll(false);
        }
    }

    private void resumeTvIfNeeded() {
        if (DEBUG) Log.d(TAG, "resumeTvIfNeeded()");
        // Take consideration of cec wake situation here before resume tv with initial input.
        updateInitChannelForCecWake();
        if (!mTvView.isPlaying()
                || mInitChannelUri != null
                || (mShouldTuneToTunerChannel && mChannelTuner.isCurrentChannelPassthrough())) {
            if (TvContract.isChannelUriForPassthroughInput(mInitChannelUri)) {
                // The target input may not be ready yet, especially, just after screen on.
                String inputId = mInitChannelUri.getPathSegments().get(1);
                TvInputInfo input = mTvInputManagerHelper.getTvInputInfo(inputId);
                if (DEBUG) Log.d(TAG, "resumeTvIfNeeded() input = " + input);
                if (input == null) {
                    input = mTvInputManagerHelper.getTvInputInfo(mParentInputIdWhenScreenOff);
                    if (input == null) {
                        SoftPreconditions.checkState(false, TAG, "Input disappear.");
                        finish();
                    } else {
                        if (DEBUG) Log.d(TAG, "resumeTvIfNeeded() mInitChannelUri = " + mInitChannelUri);
                        mInitChannelUri =
                                TvContract.buildChannelUriForPassthroughInput(input.getId());
                    }
                }
            }
            mParentInputIdWhenScreenOff = null;
            //startTv(mInitChannelUri);
            startTvInHandler(mInitChannelUri);
            mInitChannelUri = null;
        }
        // Make sure TV app has the main TV view to handle the case that TvView is used in other
        // application.
        restoreMainTvView();
        mTvView.setBlockScreenType(getDesiredBlockScreenType());
    }

    private void updateInitChannelForCecWake() {
        Log.d(TAG, "updateInitChannelForCecWake " + mCareWakeByCec);
        if (!mTvInputManagerHelper.isFirstLaunch() && !mCareWakeByCec) {
            // When tv is not first boot and is not waked by cec either, no need to care more.
            return;
        }
        mCareWakeByCec = false;
        TvInputInfo cecWakeTvInput = mTvInputManagerHelper.getCecWakeInput(mTvControlManager);
        if (null == cecWakeTvInput) {
            Log.d(TAG, "not wake by cec");
            return;
        }
        String inputId = cecWakeTvInput.getId();
        Log.d(TAG, "updateInitChannelForCecWake inputId: " + inputId);
        mInitChannelUri = TvContract.buildChannelUriForPassthroughInput(inputId);
        DroidLogicTvUtils.setCurrentInputId(MainActivity.this, inputId);
    }

    public int getSearchTypeChangedStatus() {
        int result = Settings.System.getInt(this.getContentResolver(), DroidLogicTvUtils.TV_SEARCH_TYPE_CHANGED, 0) + Settings.System.getInt(this.getContentResolver(), DroidLogicTvUtils.TV_SEARCH_INPUTID_CHANGED, 0);
        return result > 1 ? 1 : result;
    }

    public int getSearchInputIdChangeStatus() {
        return DroidLogicTvUtils.getSearchInputIdChangeStatus(MainActivity.this);
    }

    public void resetSearchTypeChangedStatus() {
        Settings.System.putInt(MainActivity.this.getContentResolver(), DroidLogicTvUtils.TV_SEARCH_TYPE_CHANGED, 0);
        Settings.System.putInt(MainActivity.this.getContentResolver(), DroidLogicTvUtils.TV_SEARCH_INPUTID_CHANGED, 0);
    }

    /*private void saveChannelIndex(int channelIndex) {
        if (DroidLogicTvUtils.isAtscCountry(this)) {
            String currentSignalType = DroidLogicTvUtils.getCurrentSignalType(this) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
                ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(this);
            Settings.System.putInt(this.getContentResolver(), currentSignalType, channelIndex);
        } else {
            Settings.System.putInt(this.getContentResolver(), DroidLogicTvUtils.getSearchType(this) == 0
                ? DroidLogicTvUtils.ATV_CHANNEL_INDEX : DroidLogicTvUtils.DTV_CHANNEL_INDEX,
                channelIndex);
        }
    }*/

    public void saveChannelIdForAtvDtvMode(long channelid) {
        String inputid  = DroidLogicTvUtils.getSearchInputId(this);
        if (!DroidLogicTvUtils.isDroidLogicInput(inputid)) {
            Settings.System.putLong(this.getContentResolver(), inputid, channelid);
            Log.d(TAG, "saveChannelIdForAtvDtvMode other type " + inputid + ", channelid = " + channelid);
        } else if (DroidLogicTvUtils.isAtscCountry(this)) {
            String currentSignalType = DroidLogicTvUtils.getCurrentSignalType(this) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
                ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(this);
            Settings.System.putLong(this.getContentResolver(), currentSignalType, channelid);
        } else {
            Settings.System.putLong(this.getContentResolver(), DroidLogicTvUtils.getSearchType(this).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX))
                ? DroidLogicTvUtils.ATV_CHANNEL_INDEX : DroidLogicTvUtils.DTV_CHANNEL_INDEX,
                channelid);
            Log.d(TAG, "save atv = " + (DroidLogicTvUtils.getSearchType(this).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX))) + ", channelid = " + channelid);
        }
    }

    /*public int getChannelIndex() {
        String currentSignalType = DroidLogicTvUtils.getCurrentSignalType(this) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
            ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(this);
        int index = 0;
        if (DroidLogicTvUtils.isAtscCountry(this)) {
            Settings.System.getInt(this.getContentResolver(), currentSignalType, -1);
        } else {
            index = Settings.System.getInt(this.getContentResolver(), DroidLogicTvUtils.getSearchType(this) == 0
                ? DroidLogicTvUtils.ATV_CHANNEL_INDEX : DroidLogicTvUtils.DTV_CHANNEL_INDEX, -1);
        }
        if (index == -1) {
            index = 0;
        }
        return index;
    }*/

    public long getChannelIdForAtvDtvMode() {
        String currentSignalType = DroidLogicTvUtils.getCurrentSignalType(this) == DroidLogicTvUtils.SIGNAL_TYPE_ERROR
            ? TvContract.Channels.TYPE_ATSC_T : DroidLogicTvUtils.getCurrentSignalType(this);
        long index = 0;
        if (!DroidLogicTvUtils.isDroidLogicInput(DroidLogicTvUtils.getSearchInputId(this))) {
            index = Settings.System.getLong(this.getContentResolver(), DroidLogicTvUtils.getSearchInputId(this), -1);
        } else if (DroidLogicTvUtils.isAtscCountry(this)) {
            index = Settings.System.getLong(this.getContentResolver(), currentSignalType, -1);
        } else {
            index = Settings.System.getLong(this.getContentResolver(), DroidLogicTvUtils.getSearchType(this).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX))
                ? DroidLogicTvUtils.ATV_CHANNEL_INDEX : DroidLogicTvUtils.DTV_CHANNEL_INDEX, -1);
            Log.d(TAG, "atv = " + (DroidLogicTvUtils.getSearchType(this).equals(TvScanConfig.TV_SEARCH_TYPE.get(TvScanConfig.TV_SEARCH_TYPE_ATV_INDEX)) + ", index = " + index));
        }
        /*if (index == -1) {
            index = 0;
        }*/
        return index;
    }

    public ChannelInfo convertToChannelInfo(Channel channel, Context context) {
        Uri channelUri = null;
        TvDataBaseManager mTvDataBaseManager = new TvDataBaseManager(context);
        if (channel.isPassthrough()) {
            channelUri = TvContract.buildChannelUriForPassthroughInput(channel.getInputId());
        } else {
            channelUri = TvContract.buildChannelUri(channel.getId());
        }
        return mTvDataBaseManager.getChannelInfo(channelUri);
    }

    private void startTvInHandler(final Uri channelUri) {
        if (mHandler != null) {
            mHandler.removeMessages(MSG_START_TV);
            mHandler.sendMessage(mHandler.obtainMessage(MSG_START_TV, channelUri));
        }
    }

    private void startTv(Uri channelUri) {
        if (DEBUG) Log.d(TAG, "startTv Uri=" + channelUri);
        if ((channelUri == null || !TvContract.isChannelUriForPassthroughInput(channelUri))
                && mChannelTuner.isCurrentChannelPassthrough()) {
            // For passthrough TV input, channelUri is always given. If TV app is launched
            // by TV app icon in a launcher, channelUri is null. So if passthrough TV input
            // is playing, we stop the passthrough TV input.
            stopTv();
        }
        //prevent crash when select source from source menu without database
        if (!(TvContract.isChannelUriForPassthroughInput(channelUri)
                || mChannelTuner.areAllChannelsLoaded())) {
            return;
        }
        SoftPreconditions.checkState(
                TvContract.isChannelUriForPassthroughInput(channelUri)
                        || mChannelTuner.areAllChannelsLoaded(),
                TAG,
                "startTV assumes that ChannelDataManager is already loaded.");
        if (mTvView.isPlaying()) {
            // TV has already started.
            if (channelUri == null || channelUri.equals(mChannelTuner.getCurrentChannelUri())) {
                // Simply adjust the volume without tune.
                mAudioManagerHelper.setVolumeByAudioFocusStatus();
                return;
            }
            stopTv();
        }
        if (mChannelTuner.getCurrentChannel() != null) {
            Log.w(TAG, "The current channel should be reset before");
            mChannelTuner.resetCurrentChannel();
        }
        if (channelUri == null) {
            // If any initial channel id is not given, get saved channeluri no matter passthrough or not.
            String lastWatchedChannelUri = Utils.getLastWatchedChannelUri(this);
            if (lastWatchedChannelUri != null) {
                channelUri = Uri.parse(lastWatchedChannelUri);
            }
        }
        if (channelUri == null) {
            mChannelTuner.moveToChannel(mChannelTuner.findNearestBrowsableChannel(0));
        } else {
            if (TvContract.isChannelUriForPassthroughInput(channelUri)) {
                ChannelImpl channel = ChannelImpl.createPassthroughChannel(channelUri);
                mChannelTuner.moveToChannel(channel);
            } else {
                long channelId = ContentUris.parseId(channelUri);
                long savedChannelId = getChannelIdForAtvDtvMode();
                if (channelId != savedChannelId) {
                    Log.d(TAG, "startTv use saved channel id channelId = " + channelId + ", savedChannelId = " + savedChannelId);
                    channelId = savedChannelId;
                }
                Channel channel = mChannelDataManager.getChannel(channelId);
                if (channel == null || !mChannelTuner.moveToChannel(channel)) {
                    Channel nearone = mChannelTuner.findNearestBrowsableChannel(0);
                    if (nearone != null && !nearone.isPassthrough() && !mQuickKeyInfo.isChannelMatchAtvDtvSource(nearone)) {
                        Log.w(TAG,"the channel to be tuned is not matched to source, and wait for further update");
                    } else {
                        mChannelTuner.moveToChannel(nearone/*mChannelTuner.findNearestBrowsableChannel(0)*/);
                        Log.w(TAG,"The requested channel (id="
                                + channelId
                                + ") doesn't exist. "
                                + "The first channel will be tuned to.");
                    }
                }
            }
        }

        mTvView.start();
        mAudioManagerHelper.setVolumeByAudioFocusStatus();
        tune(true);
    }

    @Override
    protected void onStop() {
        if (DEBUG) Log.d(TAG, "onStop()");
        if (mScreenOffIntentReceived) {
            mScreenOffIntentReceived = false;
        } else {
            if (!mPowerManager.isInteractive()) {
                // We added to check isInteractive as well as SCREEN_OFF intent, because
                // calling timing of the intent SCREEN_OFF is not consistent. b/25953633.
                // If we verify that checking isInteractive is enough, we can remove the logic
                // for SCREEN_OFF intent.
                markCurrentChannelDuringScreenOff();
            }
        }
        mActivityStarted = false;
        stopAll(false);
        unregisterReceiver(mBroadcastReceiver);
        mTracker.sendMainStop(mMainDurationTimer.reset());
        hdmi_cec.selectHdmiDevice(0, 0, 0);
        hdmi_cec.setDeviceIdForCec(DroidLogicTvUtils.DEVICE_ID_ATV - 1);  //home
        if (mOverlayManager.getSideFragmentManager().getStartedByDroid()) {
            mOverlayManager.getSideFragmentManager().setStartedByDroid(false);
        }
        mOverlayManager.resetChannelBannerViewLockType();
        mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_WITHOUT_ANIMATION);
        if (mTvView != null) {
           mTvView.resetBlockUiStatus();
        }
        super.onStop();
    }

    /** Handles screen off to keep the current channel for next screen on. */
    private void markCurrentChannelDuringScreenOff() {
        Log.d(TAG, "markCurrentChannelDuringScreenOff");
        // When system goes to standby, we should care the cec wake situation again.
        mCareWakeByCec = true;
        mInitChannelUri = mChannelTuner.getCurrentChannelUri();
        if (mChannelTuner.isCurrentChannelPassthrough()) {
            // When ACTION_SCREEN_OFF is invoked, some CEC devices may be already
            // removed. So we need to get the input info from ChannelTuner instead of
            // TvInputManagerHelper.
            TvInputInfo input = mChannelTuner.getCurrentInputInfo();
            mParentInputIdWhenScreenOff = input.getParentId();
            if (DEBUG) Log.d(TAG, "Parent input: " + mParentInputIdWhenScreenOff);
        }
        /*
        TvInputInfo cectvinputinfo = getFirstCecTvInputInfo();
        if (cectvinputinfo != null) {
            String cecinputid = cectvinputinfo.getId();
            String cecparentinputid = cectvinputinfo.getParentId();
            if (cecinputid != null) {
                mInitChannelUri = TvContract.buildChannelUriForPassthroughInput(cecinputid);
                if (DEBUG) Log.d(TAG, "init cec inputid during screen off = " + cecinputid);
            }
            if (cecparentinputid != null) {
                mParentInputIdWhenScreenOff = cecparentinputid;
                if (DEBUG) Log.d(TAG, "init cec parentinputid during screen off = " + cecparentinputid);
            }
        }
        */
    }

    private void stopAll(boolean keepVisibleBehind) {
        mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_WITHOUT_ANIMATION);
        stopTv("stopAll()", keepVisibleBehind);
    }

    public TvInputManagerHelper getTvInputManagerHelper() {
        return mTvInputManagerHelper;
    }

    public Intent createDroidLogicSetupIntent(TvInputInfo input) {
        Intent setupIntent = null;
        if (input == null) {
            return null;
        }
        String searchedInputId = DroidLogicTvUtils.getSearchInputId(this);
        if (!TextUtils.equals(searchedInputId, input.getId())) {
            DroidLogicTvUtils.setSearchInputId(this, input.getId(), false);
            DroidLogicTvUtils.setCurrentInputId(this, input.getId());
        }
        if (input != null && input.getServiceInfo().packageName.equals(DroidLogicTvUtils.TV_DROIDLOGIC_PACKAGE)) {
            setupIntent = new Intent(this, ChannelSearchActivity.class);
            setupIntent.putExtra(CommonConstants.EXTRA_INPUT_ID, input.getId());
            setupIntent.putExtra(DroidLogicTvUtils.KEY_LIVETV_PVR_STATUS, getPvrScheduleStatus());
            return setupIntent;
        } else if (input != null && !input.isPassthroughInput()) {
            /*saveChannelIdForAtvDtvMode(-1);//reset channel id if searched
            Uri channelUri = TvContract.buildChannelUri(-1);
            Utils.setLastWatchedChannelUri(this, channelUri.toString());*/
        }
        Log.d(TAG, "createDroidLogicSetupIntent input = " + input);
        setupIntent = input.createSetupIntent();
        setupIntent.putExtra(CommonConstants.EXTRA_INPUT_ID, input.getId());
        setupIntent.putExtra(DroidLogicTvUtils.KEY_LIVETV_PVR_STATUS, getPvrScheduleStatus());
        return setupIntent;
    }

    /**
     * Starts setup activity for the given input {@code input}.
     *
     * @param calledByPopup If true, startSetupActivity is invoked from the setup fragment.
     */
    public void startSetupActivity(TvInputInfo input, boolean calledByPopup) {
        //Intent intent = TvCommonUtils.createSetupIntent(input);
        Intent intent = createDroidLogicSetupIntent(input);
        if (intent == null) {
            Toast.makeText(this, R.string.msg_no_setup_activity, Toast.LENGTH_SHORT).show();
            return;
        }
        // Even though other app can handle the intent, the setup launched by Live channels
        // should go through Live channels SetupPassthroughActivity.
        //intent.setComponent(new ComponentName(this, SetupPassthroughActivity.class));
        //send number search channel parameter
        if (input != null && mQuickKeyInfo.isNumberSearch()) {
            intent.putExtra(DroidLogicTvUtils.TV_NUMBER_SEARCH_MODE, true);
            intent.putExtra(DroidLogicTvUtils.TV_NUMBER_SEARCH_NUMBER, mQuickKeyInfo.getSearchNumber());
            mQuickKeyInfo.resetNumberSearch();
        }
        try {
            // Now we know that the user intends to set up this input. Grant permission for writing
            // EPG data.
            SetupUtils.grantEpgPermission(this, input.getServiceInfo().packageName);

            mInputIdUnderSetup = input.getId();
            mIsSetupActivityCalledByPopup = calledByPopup;
            // Call requestVisibleBehind(false) before starting other activity.
            // In Activity.requestVisibleBehind(false), this activity is scheduled to be stopped
            // immediately if other activity is about to start. And this activity is scheduled to
            // to be stopped again after onPause().

            //hide no channel ui
            if (mTvView.isPlaying() && mTvView.getCurrentChannel() != null) {
                mTvView.hideNoDataBaseHint();
            }
            mQuickKeyInfo.cancelNoSingalTimeout();

            stopTv("startSetupActivity()", false);
            startActivityForResult(intent, REQUEST_CODE_START_SETUP_ACTIVITY);
            /*if (input != null && input.getServiceInfo().packageName.equals("com.droidlogic.tvinput")) {
                mQuickKeyInfo.setSearchedChannelFlag(true);
            } else {
                Log.d(TAG, "other channel no need to set search flag!");
            }*/
            mQuickKeyInfo.setSearchedChannelFlag(true);//set flag for all source
        } catch (ActivityNotFoundException e) {
            mInputIdUnderSetup = null;
            Toast.makeText(
                            this,
                            getString(
                                    R.string.msg_unable_to_start_setup_activity,
                                    input.loadLabel(this)),
                            Toast.LENGTH_SHORT)
                    .show();
            return;
        }
        if (calledByPopup) {
            mOverlayManager.hideOverlays(
                    TvOverlayManager.FLAG_HIDE_OVERLAYS_WITHOUT_ANIMATION
                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_FRAGMENT);
        } else {
            mOverlayManager.hideOverlays(
                    TvOverlayManager.FLAG_HIDE_OVERLAYS_WITHOUT_ANIMATION
                            | TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SIDE_PANEL_HISTORY);
        }
    }

    public boolean hasCaptioningSettingsActivity() {
        return Utils.isIntentAvailable(this, new Intent(Settings.ACTION_CAPTIONING_SETTINGS));
    }

    public void startSystemCaptioningSettingsActivity() {
        Intent intent = new Intent(Settings.ACTION_CAPTIONING_SETTINGS);
        try {
            startActivitySafe(intent);
        } catch (ActivityNotFoundException e) {
            Toast.makeText(
                            this,
                            getString(R.string.msg_unable_to_start_system_captioning_settings),
                            Toast.LENGTH_SHORT)
                    .show();
        }
    }

    public ChannelDataManager getChannelDataManager() {
        return mChannelDataManager;
    }

    public ProgramDataManager getProgramDataManager() {
        return mProgramDataManager;
    }

    public TvOptionsManager getTvOptionsManager() {
        return mTvOptionsManager;
    }

    public TvViewUiManager getTvViewUiManager() {
        return mTvViewUiManager;
    }

    public TimeShiftManager getTimeShiftManager() {
        return mTimeShiftManager;
    }

    /** Returns the instance of {@link TvOverlayManager}. */
    public TvOverlayManager getOverlayManager() {
        return mOverlayManager;
    }

    /** Returns the {@link ConflictChecker}. */
    @Nullable
    public ConflictChecker getDvrConflictChecker() {
        return mDvrConflictChecker;
    }

    public Channel getCurrentChannel() {
        return mChannelTuner.getCurrentChannel();
    }

    public long getCurrentChannelId() {
        return mChannelTuner.getCurrentChannelId();
    }

    /**
     * Returns the current program which the user is watching right now.
     *
     * <p>It might be a live program. If the time shifting is available, it can be a past program,
     * too.
     */
    public Program getCurrentProgram() {
        if (!isChannelChangeKeyDownReceived() && mTimeShiftManager.isAvailable()) {
            // We shouldn't get current program from TimeShiftManager during channel tunning
            return mTimeShiftManager.getCurrentProgram();
        }
        return mProgramDataManager.getCurrentProgram(getCurrentChannelId());
    }

    /**
     * Returns the current playing time in milliseconds.
     *
     * <p>If the time shifting is available, the time is the playing position of the program,
     * otherwise, the system current time.
     */
    public long getCurrentPlayingPosition() {
        long currentTime = 0;
        if (mTimeShiftManager.isAvailable()) {
            currentTime = mTimeShiftManager.getCurrentPositionMs();
            Log.d(TAG, "getCurrentPlayingPosition timeshift = " + Utils.toTimeString(currentTime));
            //return mTimeShiftManager.getCurrentPositionMs();
        } else {
            currentTime = mClock.currentTimeMillis();
            Log.d(TAG, "getCurrentPlayingPosition tvclock = " + Utils.toTimeString(currentTime));
        }
        return currentTime;
        //return mClock.currentTimeMillis();
    }

    private Channel getBrowsableChannel() {
        Channel curChannel = mChannelTuner.getCurrentChannel();
        if (curChannel != null && curChannel.isBrowsable()) {
            return curChannel;
        } else {
            return mChannelTuner.getAdjacentBrowsableChannel(true);
        }
    }

    public TunableTvView getTvView() {
        return mTvView;
    }

    public void showTeleTextAdvancedSettings() {
        mTeleTextAdvancedSettings.creatTeletextAdvancedDialog();
    }

    public void hideTeleTextAdvancedSettings() {
        mTeleTextAdvancedSettings.hideTeletextAdvancedDialog();
    }

    /**
     * Call {@link Activity#startActivity} in a safe way.
     *
     * @see LauncherActivity
     */
    public void startActivitySafe(Intent intent) {
        LauncherActivity.startActivitySafe(this, intent);
    }

    /** Show settings fragment. */
    public void showSettingsFragment() {
        if (!mChannelTuner.areAllChannelsLoaded()) {
            // Show ChannelSourcesFragment only if all the channels are loaded.
            return;
        }
        mOverlayManager.getSideFragmentManager().show(new SettingsFragment());
    }

    public void showMerchantCollection() {
        startActivitySafe(OnboardingUtils.ONLINE_STORE_INTENT);
    }

    /**
     * It is called when shrunken TvView is desired, such as EditChannelFragment and
     * ChannelsLockedFragment.
     */
    public void startShrunkenTvView(
            boolean showLockedChannelsTemporarily, boolean willMainViewBeTunerInput) {
        mChannelBeforeShrunkenTvView = mTvView.getCurrentChannel();
        mWasChannelUnblockedBeforeShrunkenByUser = mIsCurrentChannelUnblockedByUser;
        mAllowedRatingBeforeShrunken = mLastAllowedRatingForCurrentChannel;
        mTvViewUiManager.startShrunkenTvView();

        if (showLockedChannelsTemporarily) {
            mShowLockedChannelsTemporarily = true;
            checkChannelLockNeeded(mTvView, null);
        }

        mTvView.setBlockScreenType(getDesiredBlockScreenType());
    }

    /**
     * It is called when shrunken TvView is no longer desired, such as EditChannelFragment and
     * ChannelsLockedFragment.
     */
    public void endShrunkenTvView() {
        mTvViewUiManager.endShrunkenTvView();
        mIsCompletingShrunkenTvView = true;

        Channel returnChannel = mChannelBeforeShrunkenTvView;
        if (returnChannel == null
                || (!returnChannel.isPassthrough() && ((!returnChannel.isOtherChannel() && !returnChannel.isBrowsable()) || (returnChannel.isOtherChannel() && returnChannel.IsHidden())))) {
            // Try to tune to the next best channel instead.
            returnChannel = getBrowsableChannel();
        }
        mShowLockedChannelsTemporarily = false;

        // The current channel is mTvView.getCurrentChannel() and need to tune to the returnChannel.
        if (!Objects.equals(mTvView.getCurrentChannel(), returnChannel)) {
            final Channel channel = returnChannel;
            Runnable tuneAction =
                    new Runnable() {
                        @Override
                        public void run() {
                            tuneToChannel(channel);
                            if (mChannelBeforeShrunkenTvView == null
                                    || !mChannelBeforeShrunkenTvView.equals(channel)) {
                                Utils.setLastWatchedChannel(MainActivity.this, channel);
                            }
                            mIsCompletingShrunkenTvView = false;
                            mIsCurrentChannelUnblockedByUser =
                                    mWasChannelUnblockedBeforeShrunkenByUser;
                            mTvView.setBlockScreenType(getDesiredBlockScreenType());
                        }
                    };
            mTvViewUiManager.fadeOutTvView(tuneAction);
            // Will automatically fade-in when video becomes available.
        } else {
            checkChannelLockNeeded(mTvView, null);
            mIsCompletingShrunkenTvView = false;
            mIsCurrentChannelUnblockedByUser = mWasChannelUnblockedBeforeShrunkenByUser;
            mTvView.setBlockScreenType(getDesiredBlockScreenType());
        }
    }

    private boolean isUnderShrunkenTvView() {
        return mTvViewUiManager.isUnderShrunkenTvView() || mIsCompletingShrunkenTvView;
    }

    public void updateBlockStatusForPreviousSavedChannelBeforeShrunkenTvViewiIfNeeded(long setChannelId, boolean blocked, boolean forceUpdate) {
        if (mShowLockedChannelsTemporarily && isUnderShrunkenTvView()) {
            if (forceUpdate || (mChannelBeforeShrunkenTvView != null && mChannelBeforeShrunkenTvView.getId() == setChannelId)) {
                Log.d(TAG, "updateBlockStatusForPreviousSavedChannelBeforeShrunkenTvViewiIfNeeded set blocked = " + blocked);
                mQuickKeyInfo.saveBooleanValue(DroidLogicTvUtils.TV_CURRENT_CHANNELBLOCK_STATUS, blocked);
            } else {
                Log.d(TAG, "updateBlockStatusForPreviousSavedChannelBeforeShrunkenTvViewiIfNeeded no need set");
            }
        } else {
            Log.d(TAG, "updateBlockStatusForPreviousSavedChannelBeforeShrunkenTvViewiIfNeeded not set");
        }
    }

    /**
     * Returns {@code true} if the tunable tv view is blocked by resource conflict or by parental
     * control, otherwise {@code false}.
     */
    public boolean isScreenBlockedByResourceConflictOrParentalControl() {
        return mTvView.getVideoUnavailableReason()
                        == TunableTvView.VIDEO_UNAVAILABLE_REASON_NO_RESOURCE
                || mTvView.isBlocked();
    }

    @Override
    public void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (DEBUG) Log.d(TAG, "onActivityResult requestCode = " + requestCode + ", resultCode = " + resultCode);
        switch (requestCode) {
            case REQUEST_CODE_START_SETUP_ACTIVITY:
                if (resultCode == RESULT_OK) {
                    /*int count = mChannelDataManager.getChannelCountForInput(mInputIdUnderSetup);
                    String text;
                    if (count > 0) {
                        text =
                                getResources()
                                        .getQuantityString(
                                                R.plurals.msg_channel_added, count, count);
                    } else {
                        text = getString(R.string.msg_no_channel_added);
                    }
                    Toast.makeText(MainActivity.this, text, Toast.LENGTH_SHORT).show();*/
                    mInputIdUnderSetup = null;
                    if (mChannelTuner.getCurrentChannel() == null) {
                        mChannelTuner.moveToAdjacentBrowsableChannel(true);
                        //Log.d(TAG, "onActivityResult mChannelTuner.getCurrentChannel() = " + mChannelTuner.getCurrentChannel());
                    }

                    //save first found channel to ensure tune to it
                    if (USE_DROIDLOIC_CUSTOMIZATION && mQuickKeyInfo.setSearchedChannelData(data)) {
                        Channel channel = mQuickKeyInfo.getFirstSearchedChannel();
                        Utils.setLastWatchedChannel(this, channel);
                        if (channel != null) {
                            saveChannelIdForAtvDtvMode(channel.getId());
                        }
                    } else if (mQuickKeyInfo.hasStartedSearch()) {//update dtvkit first searched channel
                        long id = mQuickKeyInfo.getDtvKitSearchedChannelId(data);
                        String searchedInputId = DroidLogicTvUtils.getSearchInputId(MainActivity.this);
                        if ((searchedInputId != null && !searchedInputId.startsWith(DroidLogicTvUtils.TV_DROIDLOGIC_PACKAGE) && id != -1) ||
                                mQuickKeyInfo.getDtvKitSearchedBrowseOrHiddenChannelNumber(data) > 0) {
                            saveChannelIdForAtvDtvMode(id);//reset channel id if searched
                            Uri channelUri = TvContract.buildChannelUri(id);
                            Utils.setLastWatchedChannelUri(MainActivity.this, channelUri.toString());
                        }
                    }

                    if (mTunePending) {
                        tune(true);
                    }
                } else {
                    mInputIdUnderSetup = null;
                    //update if change search type
                    if (getSearchTypeChangedStatus() == 1) {
                        useAtvDtvChannelIndex();
                        mChannelDataManager.handleUpdateChannels();
                    }
                }
                if (!mIsSetupActivityCalledByPopup) {
                    mOverlayManager.getSideFragmentManager().showSidePanel(false);
                }
                break;
            case REQUEST_CODE_NOW_PLAYING:
                // nothing needs to be done.  onResume will restore everything.
                break;
            default:
                // do nothing
        }
        if (data != null) {
            String errorMessage = data.getStringExtra(LauncherActivity.ERROR_MESSAGE);
            if (!TextUtils.isEmpty(errorMessage)) {
                Toast.makeText(MainActivity.this, errorMessage, Toast.LENGTH_SHORT).show();
            }
        }
    }

    @Override
    public boolean dispatchKeyEvent(KeyEvent event) {
        if (SystemProperties.LOG_KEYEVENT.getValue()) Log.d(TAG, "dispatchKeyEvent(" + event + ")");
        // If an activity is closed on a back key down event, back key down events with none zero
        // repeat count or a back key up event can be happened without the first back key down
        // event which should be ignored in this activity.
        if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
            if (event.getAction() == KeyEvent.ACTION_DOWN && event.getRepeatCount() == 0) {
                mBackKeyPressed = true;
            }
            if (!mBackKeyPressed) {
                return true;
            }
            if (event.getAction() == KeyEvent.ACTION_UP) {
                mBackKeyPressed = false;
            }
        }

        // When side panel is closing, it has the focus.
        // Keep the focus, but just don't deliver the key events.
        if ((mContentView.hasFocusable() && !mOverlayManager.getSideFragmentManager().isHiding())
                || mOverlayManager.getSideFragmentManager().isActive()) {
            if (SystemProperties.USE_KEY.getValue() || TvSingletons.getSingletons(this).getSystemControlManager().getPropertyBoolean("tv.platform.need_key", false)) {
                if (event.getAction() == KeyEvent.ACTION_UP) {
                    if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
                        // It takes long time for TV app to finish, so stop TV first.
                        stopAll(false);
                        super.onBackPressed();
                        return true;
                    }
                    onKeyUp(event.getKeyCode(), event);
                } else if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    onKeyDown(event.getKeyCode(), event);
                }
                return true;
            } else {
                return super.dispatchKeyEvent(event);
            }

        }
        if (BLACKLIST_KEYCODE_TO_TIS.contains(event.getKeyCode())
                || KeyEvent.isGamepadButton(event.getKeyCode())) {
            // If the event is in blacklisted or gamepad key, do not pass it to session.
            // Gamepad keys are blacklisted to support TV UIs and here's the detail.
            // If there's a TIS granted RECEIVE_INPUT_EVENT, TIF sends key events to TIS
            // and return immediately saying that the event is handled.
            // In this case, fallback key will be injected but with FLAG_CANCELED
            // while gamepads support DPAD_CENTER and BACK by fallback.
            // Since we don't expect that TIS want to handle gamepad buttons now,
            // blacklist gamepad buttons and wait for next fallback keys.
            // TODO: Need to consider other fallback keys (e.g. ESCAPE)
            if (SystemProperties.USE_KEY.getValue() || TvSingletons.getSingletons(this).getSystemControlManager().getPropertyBoolean("tv.platform.need_key", false)) {
                if (event.getAction() == KeyEvent.ACTION_UP) {
                    if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
                        // It takes long time for TV app to finish, so stop TV first.
                        stopAll(false);
                        super.onBackPressed();
                        return true;
                    }
                    onKeyUp(event.getKeyCode(), event);
                } else if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    onKeyDown(event.getKeyCode(), event);
                }
                return true;
            } else {
                return super.dispatchKeyEvent(event);
            }
        }
        return dispatchKeyEventToSession(event) || super.dispatchKeyEvent(event);
    }

    /** Notifies the key input focus is changed to the TV view. */
    public void updateKeyInputFocus() {
        mHandler.post(
                new Runnable() {
                    @Override
                    public void run() {
                        mTvView.setBlockScreenType(getDesiredBlockScreenType());
                    }
                });
    }

    // It should be called before onResume.
    private boolean handleIntent(Intent intent) {
        // Reset the closed caption settings when the activity is 1)created or 2) restarted.
        // And do not reset while TvView is playing.
        if (!mTvView.isPlaying()) {
            mCaptionSettings = new CaptionSettings(this);
        }
        mShouldTuneToTunerChannel = intent.getBooleanExtra(Utils.EXTRA_KEY_FROM_LAUNCHER, false);
        mInitChannelUri = null;

        String extraAction = intent.getStringExtra(Utils.EXTRA_KEY_ACTION);
        if (!TextUtils.isEmpty(extraAction)) {
            if (DEBUG) Log.d(TAG, "Got an extra action: " + extraAction);
            if (Utils.EXTRA_ACTION_SHOW_TV_INPUT.equals(extraAction)) {
                String lastWatchedChannelUri = Utils.getLastWatchedChannelUri(this);
                if (lastWatchedChannelUri != null) {
                    mInitChannelUri = Uri.parse(lastWatchedChannelUri);
                }
                mShowSelectInputView = true;
            }
        }

        if (TvInputManager.ACTION_SETUP_INPUTS.equals(intent.getAction())) {
            if (mQuickKeyInfo.handleUiCommand(intent)) {
                return true;
            }
            runAfterAttachedToWindow(
                    new Runnable() {
                        @Override
                        public void run() {
                            mOverlayManager.showSetupFragment();
                        }
                    });
        } else if (Intent.ACTION_VIEW.equals(intent.getAction())) {
            Uri uri = intent.getData();
            if (Utils.isProgramsUri(uri)) {
                // When the URI points to the programs (directory, not an individual item), go to
                // the program guide. The intention here is to respond to
                // "content://android.media.tv/program", not
                // "content://android.media.tv/program/XXX".
                // Later, we might want to add handling of individual programs too.
                mShowProgramGuide = true;
                return true;
            }
            // In case the channel is given explicitly, use it.
            mInitChannelUri = uri;
            if (DEBUG) Log.d(TAG, "ACTION_VIEW with " + mInitChannelUri);
            if (Channels.CONTENT_URI.equals(mInitChannelUri)) {
                // Tune to default channel.
                mInitChannelUri = null;
                mShouldTuneToTunerChannel = true;
                return true;
            }
            if ((!Utils.isChannelUriForOneChannel(mInitChannelUri)
                    && !Utils.isChannelUriForInput(mInitChannelUri))) {
                Log.w(
                        TAG,
                        "Malformed channel uri " + mInitChannelUri + " tuning to default instead");
                mInitChannelUri = null;
                return true;
            }
            mTuneParams = intent.getExtras();
            String programUriString = intent.getStringExtra(SearchManager.EXTRA_DATA_KEY);
            Uri programUriFromIntent =
                    programUriString == null ? null : Uri.parse(programUriString);
            long channelIdFromIntent = ContentUriUtils.safeParseId(mInitChannelUri);
            if (programUriFromIntent != null && channelIdFromIntent != Channel.INVALID_ID) {
                new AsyncQueryProgramTask(
                                TvSingletons.getSingletons(this).getDbExecutor(),
                                getContentResolver(),
                                programUriFromIntent,
                                Program.PROJECTION,
                                null,
                                null,
                                null,
                                channelIdFromIntent)
                        .executeOnDbThread();
            }
            if (mTuneParams == null) {
                mTuneParams = new Bundle();
            }
            if (Utils.isChannelUriForTunerInput(mInitChannelUri)) {
                mTuneParams.putLong(KEY_INIT_CHANNEL_ID, channelIdFromIntent);
            } else if (TvContract.isChannelUriForPassthroughInput(mInitChannelUri)) {
                // If mInitChannelUri is for a passthrough TV input.
                String inputId = mInitChannelUri.getPathSegments().get(1);
                TvInputInfo input = mTvInputManagerHelper.getTvInputInfo(inputId);
                if (input != null) {
                    HdmiDeviceInfo hdmiinfo = input.getHdmiDeviceInfo();
                    if (hdmiinfo != null && hdmiinfo.isCecDevice()) {
                        String parentid = input.getParentId();
                        if (parentid != null) {
                            input = mTvInputManagerHelper.getTvInputInfo(parentid);
                            mInitChannelUri = TvContract.buildChannelUriForPassthroughInput(parentid);
                        }
                    }
                }
                if (input == null) {
                    mInitChannelUri = null;
                    Toast.makeText(this, R.string.msg_no_specific_input, Toast.LENGTH_SHORT).show();
                    return false;
                } else if (!input.isPassthroughInput()) {
                    mInitChannelUri = null;
                    Toast.makeText(this, R.string.msg_not_passthrough_input, Toast.LENGTH_SHORT)
                            .show();
                    return false;
                }
            } else if (mInitChannelUri != null) {
                // Handle the URI built by TvContract.buildChannelsUriForInput().
                String inputId = mInitChannelUri.getQueryParameter("input");
                long channelId = Utils.getLastWatchedChannelIdForInput(this, inputId);
                if (channelId == Channel.INVALID_ID) {
                    String[] projection = {Channels._ID};
                    long time = mClock.currentTimeMillis();
                    try (Cursor cursor =
                            getContentResolver().query(uri, projection, null, null, null)) {
                        if (cursor != null && cursor.moveToNext()) {
                            channelId = cursor.getLong(0);
                        }
                    }
                    Debug.getTimer(Debug.TAG_START_UP_TIMER)
                            .log(
                                    "MainActivity queries DB for "
                                            + "last channel check ("
                                            + (mClock.currentTimeMillis() - time)
                                            + "ms)");
                }
                if (channelId == Channel.INVALID_ID) {
                    // Couldn't find any channel probably because the input hasn't been set up.
                    // Try to set it up.
                    mInitChannelUri = null;
                    mInputToSetUp = mTvInputManagerHelper.getTvInputInfo(inputId);
                } else {
                    mInitChannelUri = TvContract.buildChannelUri(channelId);
                    mTuneParams.putLong(KEY_INIT_CHANNEL_ID, channelId);
                }
            }
        }
        return true;
    }

    private class AsyncQueryProgramTask extends AsyncDbTask.AsyncQueryTask<Program> {
        private final long mChannelIdFromIntent;

        public AsyncQueryProgramTask(
                Executor executor,
                ContentResolver contentResolver,
                Uri uri,
                String[] projection,
                String selection,
                String[] selectionArgs,
                String orderBy,
                long channelId) {
            super(executor, contentResolver, uri, projection, selection, selectionArgs, orderBy);
            mChannelIdFromIntent = channelId;
        }

        @Override
        protected Program onQuery(Cursor c) {
            Program program = null;
            if (c != null && c.moveToNext()) {
                program = Program.fromCursor(c);
            }
            return program;
        }

        @Override
        protected void onPostExecute(Program program) {
            if (program == null || program.getStartTimeUtcMillis() <= mClock.currentTimeMillis()/*System.currentTimeMillis()*/) {
                // null or current program
                return;
            }
            Channel channel = mChannelDataManager.getChannel(mChannelIdFromIntent);
            if (channel != null) {
                ScheduledRecording scheduledRecording =
                        TvSingletons.getSingletons(MainActivity.this)
                                .getDvrDataManager()
                                .getScheduledRecordingForProgramId(program.getId());
                DvrUiHelper.checkStorageStatusAndShowErrorMessage(
                        MainActivity.this,
                        channel.getInputId(),
                        new Runnable() {
                            @Override
                            public void run() {
                                if (CommonFeatures.DVR.isEnabled(MainActivity.this)
                                        && scheduledRecording == null
                                        && mDvrManager.isProgramRecordable(program)) {
                                    DvrUiHelper.requestRecordingFutureProgram(
                                            MainActivity.this, program, false);
                                } else {
                                    DvrUiHelper.showProgramInfoDialog(MainActivity.this, program);
                                }
                            }
                        });
            }
        }
    }

    private void stopTvInHandler() {
        if (mHandler != null) {
            mHandler.removeMessages(MSG_STOP_TV);
            mHandler.sendEmptyMessage(MSG_STOP_TV);
        }
    }

    public void stopTv() {
        stopTv(null, false);
    }

    private void stopTv(String logForCaller, boolean keepVisibleBehind) {
        if (logForCaller != null) {
            Log.i(TAG, "stopTv is called at " + logForCaller + ".");
        } else {
            if (DEBUG) Log.d(TAG, "stopTv()");
        }
        if (mHandler != null) {
            mHandler.removeCallbacks(mRestoreMainViewRunnable);
        }
        if (mTvView.isPlaying()) {
            mTvView.stop();
            if (!keepVisibleBehind) {
                requestVisibleBehind(false);
            }
            //mAudioManagerHelper.abandonAudioFocus();
            mMediaSessionWrapper.setPlaybackState(false);
        }
        TvSingletons.getSingletons(this)
                .getMainActivityWrapper()
                .notifyCurrentChannelChange(this, null);
        mChannelTuner.resetCurrentChannel();
        mTunePending = false;
    }

    private void scheduleRestoreMainTvView() {
        mHandler.removeCallbacks(mRestoreMainViewRunnable);
        mHandler.postDelayed(mRestoreMainViewRunnable, TVVIEW_SET_MAIN_TIMEOUT_MS);
    }

    /** Says {@code text} when accessibility is turned on. */
    private void sendAccessibilityText(String text) {
        if (mAccessibilityManager.isEnabled()) {
            AccessibilityEvent event = AccessibilityEvent.obtain();
            event.setClassName(getClass().getName());
            event.setPackageName(getPackageName());
            event.setEventType(AccessibilityEvent.TYPE_ANNOUNCEMENT);
            event.getText().add(text);
            mAccessibilityManager.sendAccessibilityEvent(event);
        }
    }

    private void tune(boolean updateChannelBanner) {
        if (DEBUG) Log.d(TAG, "tune()");
        mQuickKeyInfo.printCallBackTraceIfNeeded("tune");
        mTuneDurationTimer.start();

        lazyInitializeIfNeeded();

        // Prerequisites to be able to tune.
        if (mInputIdUnderSetup != null) {
            mTunePending = true;
            return;
        }
        mTunePending = false;
        mProgramDataManager.stopUpdateCurrentPlayingProgram();//stop last update
        Channel channel = mChannelTuner.getCurrentChannel();
        /*support cec otp filter*/
        if (channel != null) {
            int deviceId = DroidLogicTvUtils.getHardwareDeviceId(channel.getInputId());
            if (deviceId != -1) {
                hdmi_cec.setDeviceIdForCec(deviceId);
            } else {
                hdmi_cec.setDeviceIdForCec(DroidLogicTvUtils.DEVICE_ID_ATV);
            }
        } else {
            //channel default is atv
            hdmi_cec.setDeviceIdForCec(DroidLogicTvUtils.DEVICE_ID_ATV);
        }
        /*support cec otp filter*/
        try {
            SoftPreconditions.checkState(channel != null);
        } catch (RuntimeException e) {
            Log.e(TAG, "tune checkstate: " + e.getMessage());
        }

        if (mQuickKeyInfo.hasSearchedChannel()) {
            Log.d(TAG, "tune has searched channel");
            Channel searchedchannel = mQuickKeyInfo.getFirstSearchedChannel();
            if (searchedchannel == null) {
                Log.e(TAG, "tune wait for channel update!");
                return;
            } else {
                if (!searchedchannel.hasSameReadOnlyInfo(channel)) {
                     channel = searchedchannel;
                     mChannelTuner.setCurrentChannel(channel);
                     mTvView.setCurrentChannel(channel);
                     if (DEBUG) Log.d(TAG, "tune to searched channel = " + (channel != null ? channel.getDisplayNumber() : null));
                }
            }
        }
        //avl module need realtime source id
        mQuickKeyInfo.sendSourceToAvlModule(channel);
        if (channel == null || (channel != null && !channel.isPassthrough() && !mQuickKeyInfo.isChannelMatchAtvDtvSource(channel))) {
            Log.d(TAG, "tune check channel = " + channel);
            mChannelTuner.resetCurrentChannel();
            //set current tvinputinfo to tuner as no channel
            if (channel == null) {
                mChannelTuner.setCurrentInputInfo(mQuickKeyInfo.getTunerInput());
            }
            //deal switch source when channel not update on time
            Channel preparechannel = null;
            if (getSearchTypeChangedStatus() == 1) {
                preparechannel = mQuickKeyInfo.getChannelFromDbById(getChannelIdForAtvDtvMode());
            }
            if (preparechannel == null) {
                long channelId = Utils.getLastWatchedChannelId(this);
                Log.d(TAG, "tune channelId = " + channelId);
                Uri channelUri = TvContract.buildChannelUri(channelId);
                Utils.setLastWatchedChannelUri(this, channelUri.toString());
                mTvView.showNoDataBaseHint();
                mQuickKeyInfo.setNoSingalTimeout();
                return;
            } else {
                if (DEBUG) Log.d(TAG, "tune channel is not available");
                return;
            }
        }
        if (!mChannelTuner.isCurrentChannelPassthrough()) {
            if (mTvInputManagerHelper.getTunerTvInputSize() == 0) {
                Toast.makeText(this, R.string.msg_no_input, Toast.LENGTH_SHORT).show();
                finish();
                return;
            }

            if (mSetupUtils.isFirstTune()) {
                if (!mChannelTuner.areAllChannelsLoaded()) {
                    // tune() will be called, once all channels are loaded.
                    stopTv("tune()", false);
                    return;
                }
                if (mChannelDataManager.getChannelCount() > 0) {
                    Log.d(TAG, "tune showIntroDialog");
                    mOverlayManager.showIntroDialog();
                } else if (USE_DROIDLOIC_CUSTOMIZATION) {
                     //show nothing even not tuned
                    Log.d(TAG, "tune show nothing even not tuned 1");
                    return;
                }else {
                    startOnboardingActivity();
                    return;
                }
            }
            mShowNewSourcesFragment = false;
            if (mChannelTuner.getBrowsableChannelCount() == 0
                    && mChannelDataManager.getChannelCount() > 0 && (USE_DROIDLOIC_CUSTOMIZATION && mChannelDataManager.getChannelCount() == 0)
                    && !mOverlayManager.getSideFragmentManager().isActive()) {
                if (!mChannelTuner.areAllChannelsLoaded()) {
                    Log.d(TAG, "tune all channels not loaded return");
                    return;
                }
                if (USE_DROIDLOIC_CUSTOMIZATION) {
                    //show nothing if no channel
                    Log.d(TAG, "tune show nothing even not tuned 2");
                    return;
                }
                if (mTvInputManagerHelper.getTunerTvInputSize() == 1) {
                    mOverlayManager
                            .getSideFragmentManager()
                            .show(new CustomizeChannelListFragment());
                } else {
                    mOverlayManager.showSetupFragment();
                }
                return;
            }
            if (!CommonUtils.isRunningInTest()
                    && mShowNewSourcesFragment
                    && mSetupUtils.hasUnrecognizedInput(mTvInputManagerHelper)) {
                // Show new channel sources fragment.
                runAfterAttachedToWindow(
                        new Runnable() {
                            @Override
                            public void run() {
                                mOverlayManager.runAfterOverlaysAreClosed(
                                        new Runnable() {
                                            @Override
                                            public void run() {
                                                mOverlayManager.showNewSourcesFragment();
                                            }
                                        });
                            }
                        });
            }
            Log.d(TAG, "tune mSetupUtils onTuned");
            mSetupUtils.onTuned();
            if (mTuneParams != null) {
                Long initChannelId = mTuneParams.getLong(KEY_INIT_CHANNEL_ID);
                if (initChannelId == channel.getId()) {
                    mTuneParams.remove(KEY_INIT_CHANNEL_ID);
                } else {
                    mTuneParams = null;
                }
            }
        }

        mIsCurrentChannelUnblockedByUser = false;
        if (!isUnderShrunkenTvView()) {
            mLastAllowedRatingForCurrentChannel = null;
        }
        // For every tune, we need to inform the tuned channel or input to a user,
        // if Talkback is turned on.
        sendAccessibilityText(
                mChannelTuner.isCurrentChannelPassthrough()
                        ? Utils.loadLabel(
                                this, mTvInputManagerHelper.getTvInputInfo(channel.getInputId()))
                        : channel.getDisplayText());

        /*int ccOptions = Settings.System.getInt(this.getContentResolver(), CC_OPTION, 1);
        if (ccOptions != 1) {
            String ccTrackId = Settings.System.getString(this.getContentResolver(), CC_TRACKID);
            String ccLanguage = Settings.System.getString(this.getContentResolver(), CC_LANGUAGE);
            selectSubtitleLanguage(ccOptions, ccLanguage, ccTrackId);
        }*/
        //sub index is saved in db of each channel
        resetCaptionSettings();
        resetAudioTrack();
        if (DEBUG) Log.d(TAG, "mTvView.tuneTo");
        boolean success = mTvView.tuneTo(channel, mTuneParams, mOnTuneListener);
        if (DEBUG) Log.d(TAG, "tune status = " + success + ", channeluri = " + (channel != null ? (channel.getUri() + "  " + channel.getDisplayNumber()) : null));
        mOnTuneListener.onTune(channel, isUnderShrunkenTvView());

        mTuneParams = null;
        if (!success) {
            Toast.makeText(this, R.string.msg_tune_failed, Toast.LENGTH_SHORT).show();
            return;
        }

        // Explicitly make the TV view main to make the selected input an HDMI-CEC active source.
        mTvView.setMain();

         /*filtered cec otp end*/
        scheduleRestoreMainTvView();
        //deal tune in tuneSecondStage as tvview need to response callback
        if (!SystemProperties.USE_DEBUG_TUNE_CONTROL.getValue()) {
            tuneSecondStage(channel, updateChannelBanner);
        } else {
            Log.d(TAG, "tune wait for event_tune_finished and then update livetv ui");
        }
    }

    public void sendSecondTuneStage() {
        if (mHandler != null) {
            mHandler.sendMessage(mHandler.obtainMessage(
                MSG_TUNE_CHANNEL_SECOND_STAGE, 1, 0, getCurrentChannel()));
        } else {
            Log.d(TAG, "sendSecondTuneStage null handler");
        }
    }

    private void tuneSecondStage(Channel channel, boolean updateChannelBanner) {
        if (channel == null || mTvView == null) {
            Log.d(TAG, "tuneSecondStage return as null channel");
            return;
        }
        if (SystemProperties.USE_DEBUG_TUNE_CONTROL.getValue() && !channel.isPassthrough()) {
            mOverlayManager.updateChannelBannerView();
        }
        if (!isUnderShrunkenTvView()) {
            if (!channel.isPassthrough()) {
                addToRecentChannels(channel.getId());
            }
            Utils.setLastWatchedChannel(MainActivity.this, channel);
            TvSingletons.getSingletons(MainActivity.this)
                    .getMainActivityWrapper()
                    .notifyCurrentChannelChange(MainActivity.this, channel);
        }
        // We have to provide channel here instead of using TvView's channel, because TvView's
        // channel might be null when there's tuner conflict. In that case, TvView will resets
        // its current channel onConnectionFailed().
        checkChannelLockNeeded(mTvView, channel);
        if (updateChannelBanner) {
            mQuickKeyInfo.canShowResolution(false);
            mOverlayManager.updateChannelBannerAndShowIfNeeded(
                    TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
        }
        if (mActivityResumed) {
            // requestVisibleBehind should be called after onResume() is called. But, when
            // launcher is over the TV app and the screen is turned off and on, tune() can
            // be called during the pause state by mBroadcastReceiver (Intent.ACTION_SCREEN_ON).
            requestVisibleBehind(true);
        }
        mMediaSessionWrapper.update(mTvView.isBlocked(), getCurrentChannel(), getCurrentProgram());
        mQuickKeyInfo.setSearchedChannelFlag(false);
        mTvView.hideNoDataBaseHint();//hide no channel after tune success
        mQuickKeyInfo.resetReturnedChannel();
        if (!channel.isPassthrough()) {
           saveChannelIdForAtvDtvMode(channel.getId());
           mProgramDataManager.sendUpdateCurrentPlayingProgramByChannelId(channel.getId());//start new update
        }
        resetSearchTypeChangedStatus();
    }

    // Runs the runnable after the activity is attached to window to show the fragment transition
    // animation.
    // The runnable runs asynchronously to show the animation a little better even when system is
    // busy at the moment it is called.
    // If the activity is paused shortly, runnable may not be called because all the fragments
    // should be closed when the activity is paused.
    private void runAfterAttachedToWindow(final Runnable runnable) {
        final Runnable runOnlyIfActivityIsResumed =
                new Runnable() {
                    @Override
                    public void run() {
                        if (mActivityResumed) {
                            runnable.run();
                        }
                    }
                };
        if (mContentView.isAttachedToWindow()) {
            mHandler.post(runOnlyIfActivityIsResumed);
        } else {
            mContentView
                    .getViewTreeObserver()
                    .addOnWindowAttachListener(
                            new ViewTreeObserver.OnWindowAttachListener() {
                                @Override
                                public void onWindowAttached() {
                                    mContentView
                                            .getViewTreeObserver()
                                            .removeOnWindowAttachListener(this);
                                    mHandler.post(runOnlyIfActivityIsResumed);
                                }

                                @Override
                                public void onWindowDetached() {}
                            });
        }
    }

    boolean isNowPlayingProgram(Channel channel, Program program) {
        return program == null
                ? (channel != null
                        && getCurrentProgram() == null
                        && channel.equals(getCurrentChannel()))
                : program.equals(getCurrentProgram());
    }

    private void addToRecentChannels(long channelId) {
        if (!mRecentChannels.remove(channelId)) {
            if (mRecentChannels.size() >= MAX_RECENT_CHANNELS) {
                mRecentChannels.removeLast();
            }
        }
        mRecentChannels.addFirst(channelId);
        mOverlayManager.getMenu().onRecentChannelsChanged();
    }

    /** Returns the recently tuned channels. */
    public ArrayDeque<Long> getRecentChannels() {
        return mRecentChannels;
    }

    private void checkChannelLockNeeded(TunableTvView tvView, Channel currentChannel) {
        if (currentChannel == null) {
            currentChannel = tvView.getCurrentChannel();
        }
        if (tvView.isPlaying() && currentChannel != null) {
            if (getParentalControlSettings().isParentalControlsEnabled()
                    && currentChannel.isLocked()
                    && !mShowLockedChannelsTemporarily
                    && !(isUnderShrunkenTvView()
                            && currentChannel.equals(mChannelBeforeShrunkenTvView)
                            && mWasChannelUnblockedBeforeShrunkenByUser)) {
                if (DEBUG) Log.d(TAG, "Channel " + currentChannel.getId() + " is locked");
                blockOrUnblockScreen(tvView, true);
            } else {
                blockOrUnblockScreen(tvView, false);
            }
        }
    }

    private void blockOrUnblockScreen(TunableTvView tvView, boolean blockOrUnblock) {
        blockOrUnblockScreenByType(tvView, blockOrUnblock, false);
    }

    private void blockOrUnblockScreenByUser(TunableTvView tvView, boolean blockOrUnblock) {
        blockOrUnblockScreenByType(tvView, blockOrUnblock, true);
    }

    private void blockOrUnblockScreenByType(TunableTvView tvView, boolean blockOrUnblock, boolean byUser) {
        boolean lastBlockStatus = mQuickKeyInfo.getBooleanValue(DroidLogicTvUtils.TV_CURRENT_CHANNELBLOCK_STATUS, false);
        if (!mShowLockedChannelsTemporarily && !isUnderShrunkenTvView() &&
                !byUser && tvView != null /*&& tvView.getResetBlockUiStatus()*/ /*&& !TextUtils.isEmpty(valueStr)*/) {
            long lastChannelId = getChannelIdForAtvDtvMode();
            Channel currentOne = tvView.getCurrentChannel();
            if (currentOne != null && currentOne.getId() == lastChannelId) {
                Log.d(TAG, "blockOrUnblockScreen init last channel lastBlockStatus = " + lastBlockStatus + ", blockOrUnblock = " + blockOrUnblock);
                blockOrUnblock = lastBlockStatus;
            }
        }
        tvView.blockOrUnblockScreen(blockOrUnblock);
        if (tvView == mTvView) {
            mOverlayManager.updateChannelBannerAndShowIfNeeded(
                    TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_LOCK_OR_UNLOCK);
            mMediaSessionWrapper.update(blockOrUnblock, getCurrentChannel(), getCurrentProgram());
        }
    }

    /** Hide the overlays when tuning to a channel from the menu (e.g. Channels). */
    public void hideOverlaysForTune() {
        mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SCENE);
    }

    public boolean needToKeepSetupScreenWhenHidingOverlay() {
        return mInputIdUnderSetup != null && mIsSetupActivityCalledByPopup;
    }

    // For now, this only takes care of 24fps.
    private void applyDisplayRefreshRate(float videoFrameRate) {
        boolean is24Fps = Math.abs(videoFrameRate - FRAME_RATE_FOR_FILM) < FRAME_RATE_EPSILON;
        if (mIsFilmModeSet && !is24Fps) {
            setPreferredRefreshRate(mDefaultRefreshRate);
            mIsFilmModeSet = false;
        } else if (!mIsFilmModeSet && is24Fps) {
            DisplayManager displayManager =
                    (DisplayManager) getSystemService(Context.DISPLAY_SERVICE);
            Display display = displayManager.getDisplay(Display.DEFAULT_DISPLAY);

            float[] refreshRates = display.getSupportedRefreshRates();
            for (float refreshRate : refreshRates) {
                // Be conservative and set only when the display refresh rate supports 24fps.
                if (Math.abs(videoFrameRate - refreshRate) < REFRESH_RATE_EPSILON) {
                    setPreferredRefreshRate(refreshRate);
                    mIsFilmModeSet = true;
                    return;
                }
            }
        }
    }

    private void setPreferredRefreshRate(float refreshRate) {
        Window window = getWindow();
        WindowManager.LayoutParams layoutParams = window.getAttributes();
        layoutParams.preferredRefreshRate = refreshRate;
        window.setAttributes(layoutParams);
    }

    private void applyMultiAudio() {
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_AUDIO);
        if (mQuickKeyInfo.isAtvSource()) {
           int mode = mQuickKeyInfo.getAtvAudioPriorityMode();
           int effectmode = mQuickKeyInfo.getAtvAudioOutMode();
           if (mode != effectmode) {
               mQuickKeyInfo.setAtvAudioOutmode(mode);
           }
        }
        if (tracks == null) {
            mTvOptionsManager.onMultiAudioChanged(null);
            return;
        }

        String id = TvSettings.getMultiAudioId(this);
        String language = TvSettings.getMultiAudioLanguage(this);
        int channelCount = TvSettings.getMultiAudioChannelCount(this);
        TvTrackInfo bestTrack = null;
        if (!(id == null && language == null && channelCount == 0)) {
            bestTrack = TvTrackInfoUtils.getBestTrackInfo(tracks, id, language, channelCount);
        }
        String selectedTrack = getSelectedTrack(TvTrackInfo.TYPE_AUDIO);
        TvTrackInfo selectedTrackInfo = null;
        for (TvTrackInfo track : tracks) {
            if (track.getId().equals(selectedTrack)) {
                selectedTrackInfo = track;
                break;
            }
        }
        if (bestTrack != null) {
            if (!bestTrack.getId().equals(selectedTrack)) {
                selectTrack(TvTrackInfo.TYPE_AUDIO, bestTrack, UNDEFINED_TRACK_INDEX);
            } else {
                mTvOptionsManager.onMultiAudioChanged(
                        Utils.getMultiAudioString(this, bestTrack, false));
            }
        } else {
            if (selectedTrackInfo != null) {
                mTvOptionsManager.onMultiAudioChanged(
                        Utils.getMultiAudioString(this, selectedTrackInfo, false));
            } else {
                mTvOptionsManager.onMultiAudioChanged(null);
            }
        }
    }

    private void applyClosedCaption() {
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_SUBTITLE);
        if (tracks == null) {
            mTvOptionsManager.onClosedCaptionsChanged(null, UNDEFINED_TRACK_INDEX);
            return;
        }

        boolean enabled = mCaptionSettings.isEnabled();
        mTvView.setClosedCaptionEnabled(enabled);

        String selectedTrackId = getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE);
        TvTrackInfo alternativeTrack = null;
        int alternativeTrackIndex = UNDEFINED_TRACK_INDEX;
        if (enabled) {
            String language = mCaptionSettings.getLanguage();
            String trackId = mCaptionSettings.getTrackId();
            for (int i = 0; i < tracks.size(); i++) {
                TvTrackInfo track = tracks.get(i);
                if (Utils.isEqualLanguage(track.getLanguage(), language)) {
                    if (track.getId().equals(trackId)) {
                        if (!track.getId().equals(selectedTrackId)) {
                            selectTrack(TvTrackInfo.TYPE_SUBTITLE, track, i);
                        } else {
                            // Already selected. Update the option string only.
                            mTvOptionsManager.onClosedCaptionsChanged(track, i);
                        }
                        if (DEBUG) {
                            Log.d(
                                    TAG,
                                    "Subtitle Track Selected {id="
                                            + track.getId()
                                            + ", language="
                                            + track.getLanguage()
                                            + "}");
                        }
                        return;
                    } else if (alternativeTrack == null) {
                        alternativeTrack = track;
                        alternativeTrackIndex = i;
                    }
                }
            }
            if (alternativeTrack != null) {
                if (!alternativeTrack.getId().equals(selectedTrackId)) {
                    selectTrack(TvTrackInfo.TYPE_SUBTITLE, alternativeTrack, alternativeTrackIndex);
                } else {
                    mTvOptionsManager.onClosedCaptionsChanged(
                            alternativeTrack, alternativeTrackIndex);
                }
                if (DEBUG) {
                    Log.d(
                            TAG,
                            "Subtitle Track Selected alternativeTrack {id="
                                    + alternativeTrack.getId()
                                    + ", language="
                                    + alternativeTrack.getLanguage()
                                    + "}");
                }
                return;
            }
        }
        if (selectedTrackId != null) {
            /*selectTrack(TvTrackInfo.TYPE_SUBTITLE, null, UNDEFINED_TRACK_INDEX);
            if (DEBUG) Log.d(TAG, "Subtitle Track Unselected");*/
            if (!mCaptionSettings.isEnabled()) {
                selectTrack(TvTrackInfo.TYPE_SUBTITLE, null, UNDEFINED_TRACK_INDEX);
            } else {
                if (DEBUG) Log.d(TAG, "Subtitle Track has selected by service");
            }
            return;
        }
        mTvOptionsManager.onClosedCaptionsChanged(null, UNDEFINED_TRACK_INDEX);
    }

    public void showProgramGuideSearchFragment() {
        getFragmentManager()
                .beginTransaction()
                .replace(R.id.fragment_container, mSearchFragment)
                .addToBackStack(null)
                .commit();
    }

    @Override
    protected void onSaveInstanceState(Bundle outState) {
        // Do not save instance state because restoring instance state when TV app died
        // unexpectedly can cause some problems like initializing fragments duplicately and
        // accessing resource before it is initialized.
    }

    @Override
    protected void onDestroy() {
        if (DEBUG) Log.d(TAG, "onDestroy()");
        Debug.getTimer(Debug.TAG_START_UP_TIMER).reset();
        SideFragment.releaseRecycledViewPool();
        ViewCache.getInstance().clear();
        if (mTvView != null) {
            mTvView.release();
        }
        if (mChannelTuner != null) {
            mChannelTuner.removeListener(mChannelTunerListener);
            mChannelTuner.stop();
        }

        if (mInputListener != null && mHdmiTvClient != null) {
            mHdmiTvClient.removeInputChangeListener();
        }

        TvApplication application = ((TvApplication) getApplication());
        if (mProgramDataManager != null) {
            mProgramDataManager.removeOnCurrentProgramUpdatedListener(
                    Channel.INVALID_ID, mOnCurrentProgramUpdatedListener);
            if (application.getMainActivityWrapper().isCurrent(this)) {
                mProgramDataManager.setPrefetchEnabled(false);
            }
        }
        if (mOverlayManager != null) {
            mAccessibilityManager.removeAccessibilityStateChangeListener(mOverlayManager);
            mOverlayManager.release();
        }
        mMemoryManageables.clear();
        if (mMediaSessionWrapper != null) {
            mMediaSessionWrapper.release();
        }
        if (mAudioManagerHelper != null) {
            mAudioManagerHelper.release();
        }
        mHandler.removeCallbacksAndMessages(null);
        application.getMainActivityWrapper().onMainActivityDestroyed(this);
        if (mSendConfigInfoRecurringRunner != null) {
            mSendConfigInfoRecurringRunner.stop();
            mSendConfigInfoRecurringRunner = null;
        }
        if (mChannelStatusRecurringRunner != null) {
            mChannelStatusRecurringRunner.stop();
            mChannelStatusRecurringRunner = null;
        }
        if (mTvInputManagerHelper != null) {
            mTvInputManagerHelper.clearTvInputLabels();
            if (TvFeatures.TUNER.isEnabled(this)) {
                mTvInputManagerHelper.removeCallback(mTvInputCallback);
            }
        }

        //new add release
        mQuickKeyInfo.unregisterCommandReceiver();
        mQuickKeyInfo.cancelNoSingalTimeout();
        hideTeleTextAdvancedSettings();

        super.onDestroy();
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (SystemProperties.LOG_KEYEVENT.getValue()) {
            Log.d(TAG, "onKeyDown(" + keyCode + ", " + event + ")");
        }

        if (!mPowerManager.isScreenOn()) {
            Log.d(TAG, "TV is sleeping, drop keyevent");
            return true;
        }

        if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
            isOKPressed = true;
        }
        switch (mOverlayManager.onKeyDown(keyCode, event)) {
            case KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY:
                Log.d(TAG, "onKeyDown KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY");
                return super.onKeyDown(keyCode, event);
            case KEY_EVENT_HANDLER_RESULT_HANDLED:
                Log.d(TAG, "onKeyDown KEY_EVENT_HANDLER_RESULT_HANDLED");
                return true;
            case KEY_EVENT_HANDLER_RESULT_NOT_HANDLED:
                Log.d(TAG, "onKeyDown KEY_EVENT_HANDLER_RESULT_NOT_HANDLED");
                return false;
            case KEY_EVENT_HANDLER_RESULT_PASSTHROUGH:
            default:
                // fall through
                Log.d(TAG, "onKeyDown fall through");
        }
        if (mSearchFragment.isVisible()) {
            return super.onKeyDown(keyCode, event);
        }
        if (!mChannelTuner.areAllChannelsLoaded()) {
            return false;
        }
        if (isChannelUpOrDown(event)) {
            if (isCurrentChannelDvrRecording()) {//deal dvr recording first
                mNeedSkipChannelUpAndDown = true;
                CheckNeedStopDvrFragment(event, false);
            } else {
                handleUpDownKeys(keyCode, event);
            }
            return true;
        }
        return super.onKeyDown(keyCode, event);
    }

    private boolean isChannelUpOrDown(KeyEvent event) {
        if (event != null) {
            int keycode = event.getKeyCode();
            if (keycode == KeyEvent.KEYCODE_CHANNEL_UP || keycode == KeyEvent.KEYCODE_DPAD_UP ||
                    keycode == KeyEvent.KEYCODE_CHANNEL_DOWN || keycode == KeyEvent.KEYCODE_DPAD_DOWN) {
                return true;
            }
        }
        return false;
    }

    private boolean handleUpDownKeys(int keyCode, @Nullable KeyEvent event) {
        if (!mChannelTuner.isCurrentChannelPassthrough()) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_CHANNEL_UP:
                case KeyEvent.KEYCODE_DPAD_UP:
                    if ((event == null || event.getRepeatCount() == 0)
                            && mChannelTuner.getBrowsableChannelCount() > 0) {
                        // message sending should be done before moving channel, because we use the
                        // existence of message to decide if users are switching channel.
                        if (event != null) {
                            mHandler.sendMessageDelayed(
                                    mHandler.obtainMessage(
                                            MSG_CHANNEL_UP_PRESSED, System.currentTimeMillis()),
                                    CHANNEL_CHANGE_INITIAL_DELAY_MILLIS);
                        }
                        mQuickKeyInfo.setSearchedChannelFlag(false);
                        //take handle message to deal
                        //moveToAdjacentChannel(true, false);
                        mChannelTuner.moveToAdjacentBrowsableChannel(true);
                        mTracker.sendChannelUp();
                    }
                    return true;
                case KeyEvent.KEYCODE_CHANNEL_DOWN:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    if ((event == null || event.getRepeatCount() == 0)
                            && mChannelTuner.getBrowsableChannelCount() > 0) {
                        // message sending should be done before moving channel, because we use the
                        // existence of message to decide if users are switching channel.
                        if (event != null) {
                            mHandler.sendMessageDelayed(
                                    mHandler.obtainMessage(
                                            MSG_CHANNEL_DOWN_PRESSED, System.currentTimeMillis()),
                                    CHANNEL_CHANGE_INITIAL_DELAY_MILLIS);
                        }
                        mQuickKeyInfo.setSearchedChannelFlag(false);
                        //take handle message to deal
                        //moveToAdjacentChannel(false, false);
                        mChannelTuner.moveToAdjacentBrowsableChannel(false);
                        mTracker.sendChannelDown();
                    }
                    return true;
                default: // fall out
            }
        }
        return false;
    }

    @Override
    public boolean onKeyUp(int keyCode, KeyEvent event) {
        /*
         * The following keyboard keys map to these remote keys or "debug actions"
         *  - --------
         *  A KEYCODE_MEDIA_AUDIO_TRACK
         *  D debug: show debug options
         *  E updateChannelBannerAndShowIfNeeded
         *  G debug: refresh cloud epg
         *  I KEYCODE_TV_INPUT
         *  O debug: show display mode option
         *  S KEYCODE_CAPTIONS: select subtitle
         *  W debug: toggle screen size
         *  V KEYCODE_MEDIA_RECORD debug: record the current channel for 30 sec
         */
        if (SystemProperties.LOG_KEYEVENT.getValue()) {
            Log.d(TAG, "onKeyUp(" + keyCode + ", " + event + ")");
        }

        if (!mPowerManager.isScreenOn()) {
            Log.d(TAG, "TV is sleeping, drop keyevent");
            return true;
        }

        if (isFinishing() || isDestroyed()) {
            return true;
        }

        //check no signal timeout status when any key press
        if (USE_DROIDLOIC_CUSTOMIZATION) {
            mQuickKeyInfo.cancelNoSingalTimeout();
            if (mTvView.getVideoUnavailableReason() == TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL ||
                    mTvView.getVideoUnavailableReason() == TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN/*!mTvView.isVideoOrAudioAvailable()*/) {
                //cancel it if set, then init it again
                mQuickKeyInfo.setNoSingalTimeout();
            }
            //check factory menu command
            if (mQuickKeyInfo.checkFactoryMenuStatus(keyCode)) {
                return true;
            }
        }

        // If we are in the middle of channel change, finish it before showing overlays.
        if (!mNeedSkipChannelUpAndDown) {
            finishChannelChangeIfNeeded();
        } else {
            mNeedSkipChannelUpAndDown = false;
        }

        if (event.getKeyCode() == KeyEvent.KEYCODE_SEARCH) {
            // Prevent MainActivity from being closed by onVisibleBehindCanceled()
            mOtherActivityLaunched = true;
            return false;
        }

        if (keyCode == KeyEvent.KEYCODE_MENU && USE_DROIDLOIC_CUSTOMIZATION) {
            //response menu key to startDroidSettings
            //mQuickKeyInfo.startDroidSettings();
            mQuickKeyInfo.startDroidSettings();
            return true;
        } else if (keyCode == KeyEvent.KEYCODE_TV_INPUT) {
            mQuickKeyInfo.startDroidlogicTvSource();
            return true;
        }

        switch (mOverlayManager.onKeyUp(keyCode, event)) {
            case KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY:
                Log.d(TAG, "onKeyUp KEY_EVENT_HANDLER_RESULT_DISPATCH_TO_OVERLAY");
                return super.onKeyUp(keyCode, event);
            case KEY_EVENT_HANDLER_RESULT_HANDLED:
                Log.d(TAG, "onKeyUp KEY_EVENT_HANDLER_RESULT_HANDLED");
                return true;
            case KEY_EVENT_HANDLER_RESULT_NOT_HANDLED:
                Log.d(TAG, "onKeyUp KEY_EVENT_HANDLER_RESULT_NOT_HANDLED");
                return false;
            case KEY_EVENT_HANDLER_RESULT_PASSTHROUGH:
            default:
                // fall through
                Log.d(TAG, "onKeyUp KEY_EVENT_HANDLER_RESULT_PASSTHROUGH");
        }

        if (mSearchFragment.isVisible()) {
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                getFragmentManager().popBackStack();
                return true;
            }
            return super.onKeyUp(keyCode, event);
        }
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            onBackPressed();
            return true;
        }

        if (!mChannelTuner.areAllChannelsLoaded()) {
            // Now channel map is under loading.
        } else if ((!USE_DROIDLOIC_CUSTOMIZATION && mChannelTuner.getBrowsableChannelCount() == 0) ||
                (USE_DROIDLOIC_CUSTOMIZATION && mChannelTuner.getBrowsableChannelCount() == 0 && !mChannelTuner.isCurrentChannelPassthrough())) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_CHANNEL_UP:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_CHANNEL_DOWN:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_E:
                case KeyEvent.KEYCODE_MENU:
                    if (!USE_DROIDLOIC_CUSTOMIZATION) {
                        showSettingsFragment();
                    }
                    return true;
                default: // fall out
                    if (USE_DROIDLOIC_CUSTOMIZATION && (mChannelTuner.getAllChannelListCount() > 0 || DroidLogicTvUtils.isAtscCountry(MainActivity.this)) &&
                            KeypadChannelSwitchView.isChannelNumberKey(keyCode)) {
                        Log.d(TAG, "onKeyUp number search or has hidden channels");
                        mOverlayManager.showKeypadChannelSwitch(keyCode);
                        return true;
                    }
            }
        } else {

            if (KeypadChannelSwitchView.isChannelNumberKey(keyCode) || (USE_DROIDLOIC_CUSTOMIZATION && ChannelNumber.isChannelNumberDelimiterKey(keyCode))) {
                if (!USE_DROIDLOIC_CUSTOMIZATION) {
                    mOverlayManager.showKeypadChannelSwitch(keyCode);
                } else {
                    Channel currentone = getCurrentChannel();
                    //deal DelimiterKey and number key
                    if (mChannelTuner.areAllChannelsLoaded()) {
                        if (currentone == null
                            || !currentone.isPassthrough()) {
                            mOverlayManager.showKeypadChannelSwitch(keyCode);
                        }
                    }
                }
                return true;
            }
            switch (keyCode) {
                case KeyEvent.KEYCODE_DPAD_RIGHT:
                    if (!mTvView.isVideoOrAudioAvailable()
                            && mTvView.getVideoUnavailableReason()
                                    == TunableTvView.VIDEO_UNAVAILABLE_REASON_NO_RESOURCE) {
                        DvrUiHelper.startSchedulesActivityForTuneConflict(
                                this, mChannelTuner.getCurrentChannel());
                        return true;
                    }
                    if (!PermissionUtils.hasModifyParentalControls(this)) {
                        return true;
                    }
                    PinDialogFragment dialog = null;
                    if (mTvView.isScreenBlocked()) {
                        dialog =
                                PinDialogFragment.create(
                                        PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_CHANNEL);
                    } else if (mTvView.isContentBlocked()) {
                        dialog =
                                PinDialogFragment.create(
                                        PinDialogFragment.PIN_DIALOG_TYPE_UNLOCK_PROGRAM,
                                        mTvView.getBlockedContentRating().flattenToString());
                    }
                    if (dialog != null) {
                        mOverlayManager.showDialogFragment(
                                PinDialogFragment.DIALOG_TAG, dialog, false);
                    }
                    return true;
                case KeyEvent.KEYCODE_WINDOW:
                    enterPictureInPictureMode();
                    return true;
                case KeyEvent.KEYCODE_ENTER:
                case KeyEvent.KEYCODE_NUMPAD_ENTER:
                case KeyEvent.KEYCODE_E:
                case KeyEvent.KEYCODE_DPAD_CENTER:
                case KeyEvent.KEYCODE_MENU:
                    if (event.isCanceled()) {
                        // Ignore canceled key.
                        // Note that if there's a TIS granted RECEIVE_INPUT_EVENT,
                        // fallback keys not blacklisted will have FLAG_CANCELED.
                        // See dispatchKeyEvent() for detail.
                        return true;
                    }
                    if (keyCode != KeyEvent.KEYCODE_MENU && keyCode != KeyEvent.KEYCODE_DPAD_CENTER) {
                        mOverlayManager.updateChannelBannerAndShowIfNeeded(
                                TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_FORCE_SHOW);
                    }
                    Channel currentone = getCurrentChannel();
                    if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER && isOKPressed) {
                        if (currentone != null && !currentone.isPassthrough()) {
                            //if it is in TV channel
                            mOverlayManager.showMenu(Menu.REASON_NONE);
                        }
                        isOKPressed = false;
                    }
                    return true;
                case KeyEvent.KEYCODE_CHANNEL_UP:
                case KeyEvent.KEYCODE_DPAD_UP:
                case KeyEvent.KEYCODE_CHANNEL_DOWN:
                case KeyEvent.KEYCODE_DPAD_DOWN:
                    // Channel change is already done in the head of this method.
                    return true;
                case KeyEvent.KEYCODE_S:
                    if (!SystemProperties.USE_DEBUG_KEYS.getValue()) {
                        break;
                    }
                    // fall through.
                case KeyEvent.KEYCODE_CAPTIONS:
                    mOverlayManager.getSideFragmentManager().setStartedByDroid(false);
                    mOverlayManager.getSideFragmentManager().show(new ClosedCaptionFragment());
                    return true;
                case KeyEvent.KEYCODE_ZOOM_IN:
                    List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_SUBTITLE);
                    boolean hasTeletext = false;
                    if (tracks != null) {
                        Iterator<TvTrackInfo> iter = tracks.iterator();
                        while (iter.hasNext()) {
                            TvTrackInfo a = iter.next();
                            if (a != null && mQuickKeyInfo.isTeletextSubtitleTrack(a.getId())) {
                                hasTeletext = true;
                                break;
                            }
                        }
                    }
                    if (!hasTeletext) {
                        Toast.makeText(
                                this,
                                R.string.subtitle_teletext_no_teletext,
                                Toast.LENGTH_SHORT)
                                .show();
                    }
                    boolean isGermany = QuickKeyInfo.COUNTRY_GERMANY.equals(mQuickKeyInfo.getCountry());
                    if (!isGermany && !hasTeletext) {
                        //teletext not needed
                        Log.d(TAG, "teletext only needed in European country or don't contain teletext");
                        return true;
                    }
                    mOverlayManager.getSideFragmentManager().show(new TeleTextFragment());
                    return true;
                case KeyEvent.KEYCODE_ZOOM_OUT:
                    String trackId = getSelectedTrack(TvTrackInfo.TYPE_SUBTITLE);
                    if (!TextUtils.isEmpty(trackId) && mQuickKeyInfo.isTeletextSubtitleTrack(trackId)) {
                        showTeleTextAdvancedSettings();
                    }
                    return true;
                case KeyEvent.KEYCODE_A:
                    if (!SystemProperties.USE_DEBUG_KEYS.getValue()) {
                        break;
                    }
                    // fall through.
                case KeyEvent.KEYCODE_MEDIA_AUDIO_TRACK:
                    mOverlayManager.getSideFragmentManager().setStartedByDroid(false);
                    mOverlayManager.getSideFragmentManager().show(new MultiAudioFragment());
                    return true;
                case KeyEvent.KEYCODE_INFO:
                    mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_KEEP_SCENE);
                    if (mOverlayManager.isInputBannerActive() || mOverlayManager.isChannelBannerViewActive()) {
                        mOverlayManager.goToEmptyBanner();
                    } else {
                        mOverlayManager.showBanner();
                    }
                    return true;
                case KeyEvent.KEYCODE_LAST_CHANNEL:
                    final Channel channel2 = getCurrentChannel();
                    if (channel2 != null && !channel2.isPassthrough()) {
                        mQuickKeyInfo.tuneToRecentChannel();
                    }
                    return true;
                case KeyEvent.KEYCODE_GUIDE:
                    final Channel channel3 = getCurrentChannel();
                    if (channel3 != null && !channel3.isPassthrough() && (channel3.isDigitalChannel() || channel3.isOtherChannel())) {
                        mQuickKeyInfo.QuickKeyAction(keyCode);
                    }
                    return true;
                case QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_VIEWMODE:
                case QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_VOICEMODE:
                case QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE:
                case QuickKeyInfo.KEYCODE_TV_SLEEP:
                    mQuickKeyInfo.QuickKeyAction(keyCode);
                    return true;
                case QuickKeyInfo.KEYCODE_FAV:{
                    final Channel channel = getCurrentChannel();
                    if (channel != null && !channel.isPassthrough() && channel.isOtherChannel()) {//add for dtvkit for the moment
                        mQuickKeyInfo.startFavSettings();
                        return true;
                    }
                }
                case QuickKeyInfo.KEYCODE_LIST:
                    final Channel channel1 = getCurrentChannel();
                    if (channel1 != null && !channel1.isPassthrough()) {
                        mOverlayManager.getSideFragmentManager().show(new MultiOptionFragment(keyCode));
                    }
                    return true;
                case KeyEvent.KEYCODE_MEDIA_RECORD:
                case KeyEvent.KEYCODE_V:
                    Channel currentChannel = getCurrentChannel();
                    if (currentChannel != null && mDvrManager != null) {
                        boolean isRecording =
                                mDvrManager.getCurrentRecording(currentChannel.getId()) != null;
                        if (!isRecording) {
                            if (!mDvrManager.isChannelRecordable(currentChannel)) {
                                Toast.makeText(
                                                this,
                                                R.string.dvr_msg_cannot_record_program,
                                                Toast.LENGTH_SHORT)
                                        .show();
                            } else {
                                Program program =
                                        mProgramDataManager.getCurrentProgram(
                                                currentChannel.getId());
                                DvrUiHelper.checkStorageStatusAndShowErrorMessage(
                                        this,
                                        currentChannel.getInputId(),
                                        new Runnable() {
                                            @Override
                                            public void run() {
                                                DvrUiHelper.requestRecordingCurrentProgram(
                                                        MainActivity.this,
                                                        currentChannel,
                                                        program,
                                                        false);
                                            }
                                        });
                            }
                        } else {
                            DvrUiHelper.showStopOrContinueRecordingDialog(
                                    this,
                                    currentChannel.getId(),
                                    DvrStopOrContinueRecordingFragment.REASON_USER_STOP,
                                    new HalfSizedDialogFragment.OnActionClickListener() {
                                        @Override
                                        public void onActionClick(long actionId) {
                                            if (actionId == DvrStopOrContinueRecordingFragment.ACTION_STOP) {
                                                ScheduledRecording currentRecording =
                                                        mDvrManager.getCurrentRecording(
                                                                currentChannel.getId());
                                                if (currentRecording != null) {
                                                    mDvrManager.stopRecording(currentRecording);
                                                }
                                            } else if (actionId == GuidedAction.ACTION_ID_CANCEL) {
                                                ScheduledRecording currentRecording =
                                                        mDvrManager.getCurrentRecording(
                                                                currentChannel.getId());
                                                if (currentRecording != null) {
                                                    mDvrManager.cancelRecording(currentRecording);
                                                }
                                            } else if (actionId == DvrStopOrContinueRecordingFragment.ACTION_CANCEL) {
                                                Log.d(TAG, "cancel record");
                                                ScheduledRecording currentRecording =
                                                        mDvrManager.getCurrentRecording(getCurrentChannelId());
                                                if (currentRecording != null) {
                                                     mDvrManager.cancelRecording(currentRecording);
                                                }
                                            } else if (actionId == DvrStopOrContinueRecordingFragment.ACTION_CONTINUE) {
                                                    Log.d(TAG, "continue record");
                                            }/*else if (actionId == GuidedAction.ACTION_ID_CANCEL) {
                                                ScheduledRecording currentRecording =
                                                        mDvrManager.getCurrentRecording(
                                                                currentChannel.getId());
                                                if (currentRecording != null) {
                                                    mDvrManager.cancelRecording(currentRecording);
                                                }
                                            }*/
                                        }
                                    });
                        }
                    }
                    return true;
                default: // fall out
            }
        }
        if (keyCode == KeyEvent.KEYCODE_WINDOW) {
            // Consumes the PIP button to prevent entering PIP mode
            // in case that TV isn't showing properly (e.g. no browsable channel)
            return true;
        }
        if (SystemProperties.USE_DEBUG_KEYS.getValue() || BuildConfig.ENG) {
            switch (keyCode) {
                case KeyEvent.KEYCODE_W:
                    mDebugNonFullSizeScreen = !mDebugNonFullSizeScreen;
                    if (mDebugNonFullSizeScreen) {
                        FrameLayout.LayoutParams params =
                                (FrameLayout.LayoutParams) mTvView.getLayoutParams();
                        params.width = 960;
                        params.height = 540;
                        params.gravity = Gravity.START;
                        mTvView.setTvViewLayoutParams(params);
                    } else {
                        FrameLayout.LayoutParams params =
                                (FrameLayout.LayoutParams) mTvView.getLayoutParams();
                        params.width = ViewGroup.LayoutParams.MATCH_PARENT;
                        params.height = ViewGroup.LayoutParams.MATCH_PARENT;
                        params.gravity = Gravity.CENTER;
                        mTvView.setTvViewLayoutParams(params);
                    }
                    return true;
                case KeyEvent.KEYCODE_CTRL_LEFT:
                case KeyEvent.KEYCODE_CTRL_RIGHT:
                    mUseKeycodeBlacklist = !mUseKeycodeBlacklist;
                    return true;
                case KeyEvent.KEYCODE_O:
                    mOverlayManager.getSideFragmentManager().show(new DisplayModeFragment());
                    return true;
                case KeyEvent.KEYCODE_D:
                    mOverlayManager.getSideFragmentManager().show(new DeveloperOptionFragment());
                    return true;
                default: // fall out
            }
        }
        return super.onKeyUp(keyCode, event);
    }

    @Override
    public boolean onKeyLongPress(int keyCode, KeyEvent event) {
        if (SystemProperties.LOG_KEYEVENT.getValue()) Log.d(TAG, "onKeyLongPress(" + event);
        if (USE_BACK_KEY_LONG_PRESS) {
            // Treat the BACK key long press as the normal press since we changed the behavior in
            // onBackPressed().
            if (keyCode == KeyEvent.KEYCODE_BACK) {
                // It takes long time for TV app to finish, so stop TV first.
                stopAll(false);
                super.onBackPressed();
                return true;
            }
        }
        return false;
    }

    @Override
    public void onBackPressed() {
        if (!TextUtils.isEmpty(SystemPropertiesProxy.getString("ro.com.google.gmsversion", ""))) {
            Log.d(TAG, "onBackPressed:  start Home intent");
            Intent startMain = new Intent(Intent.ACTION_MAIN);
            startMain.addCategory(Intent.CATEGORY_HOME);
            startActivity(startMain);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public void onUserInteraction() {
        super.onUserInteraction();
        if (mOverlayManager != null) {
            mOverlayManager.onUserInteraction();
        }
    }

    @Override
    public void enterPictureInPictureMode() {
        // We need to hide overlay first, before moving the activity to PIP. If not, UI will
        // be shown during PIP stack resizing, because UI and its animation is stuck during
        // PIP resizing.
        mIsInPIPMode = true;
        if (mOverlayManager.isOverlayOpened()) {
            mOverlayManager.hideOverlays(TvOverlayManager.FLAG_HIDE_OVERLAYS_WITHOUT_ANIMATION);
            mHandler.post(
                    new Runnable() {
                        @Override
                        public void run() {
                            MainActivity.super.enterPictureInPictureMode();
                        }
                    });
        } else {
            MainActivity.super.enterPictureInPictureMode();
        }
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (!hasFocus) {
            finishChannelChangeIfNeeded();
        }
    }

    /**
     * Returns {@code true} if one of the channel changing keys are pressed and not released yet.
     */
    public boolean isChannelChangeKeyDownReceived() {
        return mHandler.hasMessages(MSG_CHANNEL_UP_PRESSED)
                || mHandler.hasMessages(MSG_CHANNEL_DOWN_PRESSED);
    }

    private void finishChannelChangeIfNeeded() {
        if (!isChannelChangeKeyDownReceived()) {
            return;
        }
        mHandler.removeMessages(MSG_CHANNEL_UP_PRESSED);
        mHandler.removeMessages(MSG_CHANNEL_DOWN_PRESSED);
        mHandler.removeMessages(MSG_TUNE_CHANNEL);
        mHandler.removeMessages(MSG_UPDTE_CHANNEL_BANNER);
        if (mChannelTuner.getBrowsableChannelCount() > 0) {
            if (!mTvView.isPlaying()) {
                // We expect that mTvView is already played. But, it is sometimes not.
                // TODO: we figure out the reason when mTvView is not played.
                Log.w(TAG, "TV view isn't played in finishChannelChangeIfNeeded");
            }
            Channel prepareTuneChannel = mChannelTuner.getCurrentChannel();
            //if prepareTuneChannel is google channel or other type, delay 500ms to release related resource
            if (prepareTuneChannel != null && prepareTuneChannel.isOtherChannel() && !QuickKeyInfo.DTVKIT_PACKAGE.equals(prepareTuneChannel.getPackageName())) {//dtvkit is physical tuner and no need to wait
                mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_TUNE_CHANNEL, mChannelTuner.getCurrentChannel()), CHANNEL_CHANGE_INITIAL_DELAY_MILLIS);
            } else {
                //tuneToChannel(mChannelTuner.getCurrentChannel());
                mHandler.sendMessage(mHandler.obtainMessage(MSG_TUNE_CHANNEL, mChannelTuner.getCurrentChannel()));
            }
            mHandler.sendEmptyMessage(MSG_UPDTE_CHANNEL_BANNER);
        } else {
            if (!USE_DROIDLOIC_CUSTOMIZATION) {
                showSettingsFragment();
            }
        }
    }

    private boolean dispatchKeyEventToSession(final KeyEvent event) {
        if (SystemProperties.LOG_KEYEVENT.getValue()) {
            Log.d(TAG, "dispatchKeyEventToSession(" + event + ")");
        }

        if (USE_DROIDLOIC_CUSTOMIZATION && isQuickKeyNoNeedDispatchedToTvview(event.getKeyCode())) {
            if (SystemProperties.USE_KEY.getValue() || TvSingletons.getSingletons(this).getSystemControlManager().getPropertyBoolean("tv.platform.need_key", false)) {
                if (event.getAction() == KeyEvent.ACTION_UP) {
                    if (event.getKeyCode() == KeyEvent.KEYCODE_BACK) {
                        // It takes long time for TV app to finish, so stop TV first.
                        stopAll(false);
                        super.onBackPressed();
                        return true;
                    }
                    onKeyUp(event.getKeyCode(), event);
                } else if (event.getAction() == KeyEvent.ACTION_DOWN) {
                    onKeyDown(event.getKeyCode(), event);
                }
                return true;
            } else {
                return false;//deal these key in activity
            }
        }

        boolean handled = false;
        if (mTvView != null) {
            handled = mTvView.dispatchKeyEvent(event);
        }
        if (isKeyEventBlocked()) {
            if ((event.getKeyCode() == KeyEvent.KEYCODE_BACK
                            || event.getKeyCode() == KeyEvent.KEYCODE_BUTTON_B)
                    && mNeedShowBackKeyGuide) {
                // KeyEvent.KEYCODE_BUTTON_B is also used like the back button.
                Toast.makeText(this, R.string.msg_back_key_guide, Toast.LENGTH_SHORT).show();
                mNeedShowBackKeyGuide = false;
            }
            return true;
        }
        /*if (handled && USE_DROIDLOIC_CUSTOMIZATION) {
            return false;//deal these key in activity
        }*/
        return handled;
    }

    private boolean isKeyEventBlocked() {
        // If the current channel is a passthrough channel, we don't handle the key events in TV
        // activity. Instead, the key event will be handled by the passthrough TV input.
        return mChannelTuner.isCurrentChannelPassthrough();
    }

    private boolean isQuickKeyNoNeedDispatchedToTvview(int keycode) {
        return keycode == KeyEvent.KEYCODE_INFO ||
                keycode == QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_VIEWMODE ||
                keycode == QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_VOICEMODE ||
                keycode == QuickKeyInfo.KEYCODE_TV_SHORTCUTKEY_DISPAYMODE ||
                keycode == QuickKeyInfo.KEYCODE_TV_SLEEP;
    }

    public void tuneToLastWatchedChannelForTunerInput() {
        if (!mChannelTuner.isCurrentChannelPassthrough() && mChannelTuner.getCurrentChannel() != null) {
            if (USE_DROIDLOIC_CUSTOMIZATION && getSearchTypeChangedStatus() == 1) {
                String lastWatchedChannelUri = Utils.getLastWatchedChannelUri(this);
                Uri channelUri = null;
                if (lastWatchedChannelUri != null) {
                    channelUri = Uri.parse(lastWatchedChannelUri);
                }
                if (TvContract.isChannelUriForTunerInput(channelUri)) {
                    long channelId = ContentUris.parseId(channelUri);
                    Channel channel = mChannelTuner.getChannelById(channelId);
                    if (channel == null) {
                       channel = mQuickKeyInfo.getChannelFromDbById(channelId);
                    }
                    if (channelId != -1 && channel != null) {
                       if (getSearchInputIdChangeStatus() != 1) {
                           tuneToChannel(channel);
                           return;
                       }
                    }
                }
            } else {
                return;
            }
        }
        stopTv();
        startTv(null);
    }

    public void tuneToChannel(Channel channel) {
        Log.d(TAG, "tuneToChannel " + channel);
        if (channel == null) {
            if (mTvView.isPlaying()) {
                mTvView.reset();
                mTvView.showNoDataBaseHint();
            }
        } else {
            if (!channel.isPassthrough()) {
                //channelIndex = mChannelTuner.getChannelIndex(channel);
                //saveChannelIndex(channelIndex);
                //saveChannelIdForAtvDtvMode(channel.getId());
            }
            if (!mTvView.isPlaying()) {
                Log.d(TAG, "tuneToChannel tvview not play");
                startTv(channel.getUri());
            } else if (channel.equals(mTvView.getCurrentChannel())) {
                Log.d(TAG, "tuneToChannel TvView same channel");
                if (!mOverlayManager.isChannelBannerViewActive()) {
                    mOverlayManager.updateChannelBannerAndShowIfNeeded(
                            TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
                }
            } else if (channel.equals(mChannelTuner.getCurrentChannel())) {
                Log.d(TAG, "tuneToChannel ChannelTuner same channel");
                // Channel banner is already updated in moveToAdjacentChannel
                tune(false);
            } else if (mChannelTuner.moveToChannel(channel)) {
                Log.d(TAG, "tuneToChannel move to channel");
                // Channel banner would be updated inside of tune.
                tune(true);
            } else {
                if (!USE_DROIDLOIC_CUSTOMIZATION) {
                    showSettingsFragment();
                } else {
                    stopTv();
                }
            }
        }
    }

    public void CheckNeedStopDvrFragmentWhenTuneTo(Channel nextchannel) {
        Channel currentchanel = getCurrentChannel();
        if (currentchanel == null || nextchannel == null || mIsCheckingNeedStopDvr) {
            return;
        }
        boolean isSameFrequency = true;
        if (currentchanel != null && nextchannel != null && currentchanel.getFrequency() != nextchannel.getFrequency()) {
            isSameFrequency = false;
        }
        mIsCheckingNeedStopDvr = true;
        boolean isTooManySession = isRecordSessionLargerThanTuner(currentchanel);
        creatDvrConfirmInfoDialog(currentchanel, nextchannel, isTooManySession, !isSameFrequency, null);
    }

    private boolean mIsCheckingNeedStopDvr = false;

    public boolean isCheckingInDialog() {
        return mIsCheckingNeedStopDvr;
    }

    private void CheckNeedStopDvrFragment(KeyEvent keyevent, boolean exit) {
        long currentid = getCurrentChannelId();
        Channel currentchanel = getCurrentChannel();
        if (currentid == -1 || currentchanel == null || mIsCheckingNeedStopDvr) {
            return;
        }
        boolean up = false;
        if (keyevent.getKeyCode() == KeyEvent.KEYCODE_CHANNEL_UP || keyevent.getKeyCode() == KeyEvent.KEYCODE_DPAD_UP) {
            up = true;
        }
        Channel nextchannel = mChannelTuner.getAdjacentBrowsableChannel(up);
        boolean isSameFrequency = true;
        if (currentchanel != null && nextchannel != null && currentchanel.getFrequency() != nextchannel.getFrequency()) {
            isSameFrequency = false;
        }
        mIsCheckingNeedStopDvr = true;
        boolean isTooManySession = isRecordSessionLargerThanTuner(currentchanel);
        creatDvrConfirmInfoDialog(currentchanel, null, isTooManySession, !isSameFrequency, keyevent);
    }

    private boolean isRecordSessionLargerThanTuner(Channel nextchannel) {
        boolean result = false;
        TvInputInfo nextinput = null;
        InputSessionManager inputSessionManager = null;
        if (CommonFeatures.DVR.isEnabled(MainActivity.this) && nextchannel != null) {
            inputSessionManager = TvSingletons.getSingletons(MainActivity.this).getInputSessionManager();
            nextinput = mTvInputManagerHelper.getTvInputInfo(nextchannel.getInputId());
            if (nextinput != null) {
                int tunerCount = nextinput.getTunerCount();
                Log.d(TAG, "isRecordSessionLargerThanTuner tunerCount = " + tunerCount);
            }
        }
        if (inputSessionManager != null && nextinput != null && nextinput.canRecord()
                        && !inputSessionManager.isTunedForRecording(nextchannel.getUri())
                        && inputSessionManager.getTunedRecordingSessionCount(nextinput.getId()) >= nextinput.getTunerCount()) {
            result = true;
        }
        return result;
    }

    private void creatDvrConfirmInfoDialog(final Channel currentChannel, final Channel nextChannel, final boolean isSessionLragerThenTunerNumber, final boolean isDiffrentFrequency, final KeyEvent keyevent) {
        DvrUiHelper.showStopOrContinueRecordingDialog(
                this, currentChannel.getId(), DvrStopOrContinueRecordingFragment.REASON_USER_STOP,
                new HalfSizedDialogFragment.OnActionClickListener() {
                    @Override
                    public void onActionClick(long actionId) {
                        //Log.d(TAG, "onActionClick actionId = " + actionId);
                        if (actionId == DvrStopOrContinueRecordingFragment.ACTION_STOP) {
                            Log.d(TAG, "stop record");
                            ScheduledRecording currentRecording =
                                    mDvrManager.getCurrentRecording(currentChannel.getId());
                            if (currentRecording != null) {
                                 mDvrManager.stopRecording(currentRecording);
                            }
                        } else if (actionId == DvrStopOrContinueRecordingFragment.ACTION_CANCEL) {
                            Log.d(TAG, "cancel record");
                            ScheduledRecording currentRecording =
                                    mDvrManager.getCurrentRecording(currentChannel.getId());
                            if (currentRecording != null) {
                                 mDvrManager.cancelRecording(currentRecording);
                            }
                        } else if (actionId == DvrStopOrContinueRecordingFragment.ACTION_CONTINUE) {
                            Log.d(TAG, "continue record");
                            if (isSessionLragerThenTunerNumber) {
                                Log.d(TAG, "need to stop current record as too many record sessions");
                                Toast.makeText(MainActivity.this, R.string.pvr_too_many_sessions_tips, Toast.LENGTH_SHORT).show();
                            } else if (isDiffrentFrequency) {
                                Log.d(TAG, "need to stop record as next channel frequency will be other");
                                Toast.makeText(MainActivity.this, R.string.pvr_diffrent_frequency_tips, Toast.LENGTH_SHORT).show();
                            } else {
                                if (keyevent != null) {
                                    handleUpDownKeys(keyevent.getKeyCode(), keyevent);
                                    finishChannelChangeIfNeeded();
                                } else {
                                    //if currentChannel is google channel or other type, delay 500ms to release related resource
                                    if (mHandler != null) {
                                        mHandler.removeMessages(MSG_TUNE_CHANNEL);
                                        if (nextChannel != null && nextChannel.isOtherChannel() && !QuickKeyInfo.DTVKIT_PACKAGE.equals(nextChannel.getPackageName())) {//dtvkit is physical tuner and no need to wait
                                            mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_TUNE_CHANNEL, nextChannel), CHANNEL_CHANGE_INITIAL_DELAY_MILLIS);
                                        } else {
                                            //tuneToChannel(mChannelTuner.getCurrentChannel());
                                            mHandler.sendMessageDelayed(mHandler.obtainMessage(MSG_TUNE_CHANNEL, nextChannel), CHANNEL_CHANGE_DELAY_MS_IN_NORMAL_SPEED);
                                        }
                                    } else {
                                        Log.d(TAG, "creatDvrConfirmInfoDialog empty mHandler");
                                    }
                                }
                            }
                        }
                    }
                },
                new SafeDismissDialogFragment.DismissListener() {
                    @Override
                    public void onDismiss() {
                        Log.d(TAG, "DismissListener onDismiss");
                        mIsCheckingNeedStopDvr = false;
                    }
                });
    }

    public boolean isCurrentChannelDvrRecording() {
        boolean result = false;
        if (mDvrManager != null) {
            ScheduledRecording currentRecording =
                mDvrManager.getCurrentRecording(getCurrentChannelId());
            result = currentRecording != null;
        }
        return result;
    }

    private boolean isScheduleRecordingAvailable() {
        return mDvrManager != null && mDvrManager.hasScheduleRecordingAvailable();
    }

    private boolean isRecordProgramAvailable() {
        return mDvrManager != null && mDvrManager.hasRecordProgramAvailable();
    }

    private String getPvrScheduleStatus() {
        String pvrStatus = null;
        if (isScheduleRecordingAvailable()) {
            pvrStatus = "schedule";
        }
        if (isRecordProgramAvailable()) {
            if (pvrStatus == null) {
                pvrStatus = "program";
            } else {
                pvrStatus = pvrStatus + ",program";
            }
        }
        return pvrStatus;
    }

    /**
     * This method just moves the channel in the channel map and updates the channel banner, but
     * doesn't actually tune to the channel. The caller of this method should call {@link #tune} in
     * the end.
     *
     * @param channelUp {@code true} for channel up, and {@code false} for channel down.
     * @param fastTuning {@code true} if fast tuning is requested.
     */
    private void moveToAdjacentChannel(boolean channelUp, boolean fastTuning) {
        if (mChannelTuner.moveToAdjacentBrowsableChannel(channelUp)) {
            mOverlayManager.updateChannelBannerAndShowIfNeeded(
                    fastTuning
                            ? TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE_FAST
                            : TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
        }
    }

    /** Set the main TV view which holds HDMI-CEC active source based on the sound mode */
    private void restoreMainTvView() {
        mTvView.setMain();
    }

    @Override
    public void onVisibleBehindCanceled() {
        stopTv("onVisibleBehindCanceled()", false);
        mTracker.sendScreenView("");
        mAudioManagerHelper.abandonAudioFocus();
        mMediaSessionWrapper.setPlaybackState(false);
        mVisibleBehind = false;
        if (!mOtherActivityLaunched && Build.VERSION.SDK_INT == Build.VERSION_CODES.M) {
            // Workaround: in M, onStop is not called, even though it should be called after
            // onVisibleBehindCanceled is called. As a workaround, we call finish().
            finish();
        }
        super.onVisibleBehindCanceled();
    }

    @Override
    public void startActivityForResult(Intent intent, int requestCode) {
        mOtherActivityLaunched = true;
        if (intent.getCategories() == null
                || !intent.getCategories().contains(Intent.CATEGORY_HOME)) {
            // Workaround b/30150267
            requestVisibleBehind(false);
        }
        super.startActivityForResult(intent, requestCode);
    }

    public List<TvTrackInfo> getTracks(int type) {
        return mTvView.getTracks(type);
    }

    public String getSelectedTrack(int type) {
        return mTvView.getSelectedTrack(type);
    }

    public TvTrackInfo getCurrentTrackInfo() {
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_AUDIO);
        String selectedTrackId = getSelectedTrack(TvTrackInfo.TYPE_AUDIO);
        if (tracks != null) {
            for (TvTrackInfo track : tracks) {
                if (track.getId().equals(selectedTrackId)) {
                    return track;
                }
            }
        }
        return null;
    }

    private void selectTrack(int type, TvTrackInfo track, int trackIndex) {
        mTvView.selectTrack(type, track == null ? null : track.getId());
        if (type == TvTrackInfo.TYPE_AUDIO) {
            mTvOptionsManager.onMultiAudioChanged(
                    track == null ? null : Utils.getMultiAudioString(this, track, false));
        } else if (type == TvTrackInfo.TYPE_SUBTITLE) {
            mTvOptionsManager.onClosedCaptionsChanged(track, trackIndex);
        }
    }

    public void selectAudioTrack(String trackId) {
        saveMultiAudioSetting(trackId);
        applyMultiAudio();
    }

    private void saveMultiAudioSetting(String trackId) {
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_AUDIO);
        if (tracks != null) {
            for (TvTrackInfo track : tracks) {
                if (track.getId().equals(trackId)) {
                    TvSettings.setMultiAudioId(this, track.getId());
                    TvSettings.setMultiAudioLanguage(this, track.getLanguage());
                    TvSettings.setMultiAudioChannelCount(this, track.getAudioChannelCount());
                    return;
                }
            }
        }
        TvSettings.setMultiAudioId(this, null);
        TvSettings.setMultiAudioLanguage(this, null);
        TvSettings.setMultiAudioChannelCount(this, 0);
    }

    public void selectSubtitleTrack(int option, String trackId) {
        saveClosedCaptionSetting(option, trackId);
        applyClosedCaption();
    }

    public void selectSubtitleLanguage(int option, String language, String trackId) {
        mCaptionSettings.setEnableOption(option);
        mCaptionSettings.setLanguage(language);
        mCaptionSettings.setTrackId(trackId);
        applyClosedCaption();
    }

    private void saveClosedCaptionSetting(int option, String trackId) {
        mCaptionSettings.setEnableOption(option);
        if (option == CaptionSettings.OPTION_ON) {
            List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_SUBTITLE);
            if (tracks != null) {
                for (TvTrackInfo track : tracks) {
                    if (track.getId().equals(trackId)) {
                        mCaptionSettings.setLanguage(track.getLanguage());
                        mCaptionSettings.setTrackId(trackId);
                        //sub index is saved in db of each channel
                        /*Settings.System.putString(this.getContentResolver(), CC_LANGUAGE, track.getLanguage());
                        Settings.System.putString(this.getContentResolver(), CC_TRACKID, trackId);*/
                        return;
                    }
                }
            }
        }
    }

    public void updateCaptionSettings(String language, String trackId) {
        mCaptionSettings.setEnableOption(CaptionSettings.OPTION_ON);
        mCaptionSettings.setLanguage(language);
        mCaptionSettings.setTrackId(trackId);
        boolean found = false;
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_SUBTITLE);
        if (tracks != null) {
            for (TvTrackInfo track : tracks) {
                if (track.getId().equals(trackId)) {
                    Log.d(TAG, "updateCaptionSettings found " + trackId);
                    mTvOptionsManager.onClosedCaptionsChanged(track, tracks.indexOf(track));
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            Log.d(TAG, "updateCaptionSettings not found");
            mTvOptionsManager.onClosedCaptionsChanged(null, UNDEFINED_TRACK_INDEX);
        }
    }

    public void updateAudioSettings(String language, String trackId, int channelCount) {
        TvSettings.setMultiAudioId(this, trackId);
        TvSettings.setMultiAudioLanguage(this, language);
        TvSettings.setMultiAudioChannelCount(this, channelCount);
        boolean found = false;
        List<TvTrackInfo> tracks = getTracks(TvTrackInfo.TYPE_AUDIO);
        if (tracks != null) {
            for (TvTrackInfo track : tracks) {
                if (track.getId().equals(trackId)) {
                    Log.d(TAG, "updateAudioSettings found " + trackId);
                    mTvOptionsManager.onMultiAudioChanged(
                        track == null ? null : Utils.getMultiAudioString(this, track, false));
                    found = true;
                    break;
                }
            }
        }
        if (!found) {
            Log.d(TAG, "updateAudioSettings not found");
            mTvOptionsManager.onMultiAudioChanged(null);
        }
    }

    private void resetCaptionSettings() {
        mCaptionSettings.setEnableOption(CaptionSettings.OPTION_SYSTEM);
        mCaptionSettings.setLanguage(null);
        mCaptionSettings.setTrackId(null);
        mTvOptionsManager.onClosedCaptionsChanged(null, UNDEFINED_TRACK_INDEX);
    }

    private void resetAudioTrack() {
        TvSettings.setMultiAudioId(this, null);
        TvSettings.setMultiAudioLanguage(this, null);
        TvSettings.setMultiAudioChannelCount(this, 0);
    }

    private void updateAvailabilityToast() {
        if (mTvView.isVideoAvailable()
                || !Objects.equals(
                        mTvView.getCurrentChannel(), mChannelTuner.getCurrentChannel())) {
            return;
        }

        switch (mTvView.getVideoUnavailableReason()) {
            case TunableTvView.VIDEO_UNAVAILABLE_REASON_NOT_TUNED:
            case TunableTvView.VIDEO_UNAVAILABLE_REASON_NO_RESOURCE:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_TUNING:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_BUFFERING:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_AUDIO_ONLY:
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_WEAK_SIGNAL:
                return;
            case TvInputManager.VIDEO_UNAVAILABLE_REASON_UNKNOWN:
            default:
                Toast.makeText(this, R.string.msg_channel_unavailable_unknown, Toast.LENGTH_SHORT)
                        .show();
                break;
        }
    }

    /** Returns {@code true} if some overlay UI will be shown when the activity is resumed. */
    public boolean willShowOverlayUiWhenResume() {
        return mInputToSetUp != null || mShowProgramGuide || mShowSelectInputView;
    }

    /** Returns the current parental control settings. */
    public ParentalControlSettings getParentalControlSettings() {
        return mTvInputManagerHelper.getParentalControlSettings();
    }

    /** Returns a ContentRatingsManager instance. */
    public ContentRatingsManager getContentRatingsManager() {
        return mTvInputManagerHelper.getContentRatingsManager();
    }

    /** Returns the current captioning settings. */
    public CaptionSettings getCaptionSettings() {
        return mCaptionSettings;
    }

    /** Adds the {@link OnActionClickListener}. */
    public void addOnActionClickListener(OnActionClickListener listener) {
        mOnActionClickListeners.add(listener);
    }

    /** Removes the {@link OnActionClickListener}. */
    public void removeOnActionClickListener(OnActionClickListener listener) {
        mOnActionClickListeners.remove(listener);
    }

    @Override
    public boolean onActionClick(String category, int actionId, Bundle params) {
        // There should be only one action listener per an action.
        for (OnActionClickListener l : mOnActionClickListeners) {
            if (l.onActionClick(category, actionId, params)) {
                return true;
            }
        }
        return false;
    }

    // Initialize TV app for test. The setup process should be finished before the Live TV app is
    // started. We only enable all the channels here.
    private void initForTest() {
        if (!CommonUtils.isRunningInTest()) {
            return;
        }

        Utils.enableAllChannels(this);
    }

    // Lazy initialization
    private void lazyInitializeIfNeeded() {
        // Already initialized.
        if (mLazyInitialized) {
            return;
        }
        mLazyInitialized = true;
        // Running initialization.
        mHandler.postDelayed(
                new Runnable() {
                    @Override
                    public void run() {
                        if (mActivityStarted) {
                            initAnimations();
                            initSideFragments();
                            initMenuItemViews();
                        }
                    }
                },
                LAZY_INITIALIZATION_DELAY);
    }

    private void initAnimations() {
        mTvViewUiManager.initAnimatorIfNeeded();
        mOverlayManager.initAnimatorIfNeeded();
    }

    private void initSideFragments() {
        SideFragment.preloadItemViews(this);
    }

    private void initMenuItemViews() {
        mOverlayManager.getMenu().preloadItemViews();
    }

    @Override
    public void onTrimMemory(int level) {
        super.onTrimMemory(level);
        for (MemoryManageable memoryManageable : mMemoryManageables) {
            memoryManageable.performTrimMemory(level);
        }
    }

    private static class MainActivityHandler extends WeakHandler<MainActivity> {
        //for TvClock
        private Context mContext;

        MainActivityHandler(MainActivity mainActivity) {
            super(mainActivity);
            mContext = (Context)mainActivity;
        }

        @Override
        protected void handleMessage(Message msg, @NonNull MainActivity mainActivity) {
            switch (msg.what) {
                case MSG_CHANNEL_DOWN_PRESSED:
                    long startTime = (Long) msg.obj;
                    // message re-sending should be done before moving channel, because we use the
                    // existence of message to decide if users are switching channel.
                    sendMessageDelayed(Message.obtain(msg), getDelay(startTime));
                    mainActivity.moveToAdjacentChannel(false, true);
                    break;
                case MSG_CHANNEL_UP_PRESSED:
                    startTime = (Long) msg.obj;
                    // message re-sending should be done before moving channel, because we use the
                    // existence of message to decide if users are switching channel.
                    sendMessageDelayed(Message.obtain(msg), getDelay(startTime));
                    mainActivity.moveToAdjacentChannel(true, true);
                    break;
                case MSG_TUNE_TO_CEC_DEV:
                    mainActivity.tuneToChannel(ChannelImpl.createPassthroughChannel((String)msg.obj));
                    break;
                case MSG_TUNE_CHANNEL:
                    if (DEBUG) Log.d(TAG, "MSG_CHANNEL_TUNE");
                    Channel channel = (Channel) msg.obj;
                    mainActivity.tuneToChannel(channel);
                    break;
                case MSG_TUNE_CHANNEL_SECOND_STAGE:
                    if (DEBUG) Log.d(TAG, "MSG_TUNE_CHANNEL_SECOND_STAGE");
                    Channel channel1 = (Channel) msg.obj;
                    mainActivity.tuneSecondStage(channel1, msg.arg1 == 1);
                    break;
                case MSG_START_TV:
                    if (DEBUG) Log.d(TAG, "MSG_START_TV");
                    Uri uri = (Uri) msg.obj;
                    mainActivity.startTv(uri);
                    break;
                case MSG_STOP_TV:
                    if (DEBUG) Log.d(TAG, "MSG_STOP_TV");
                    mainActivity.stopTv();
                    break;
                case MSG_UPDTE_CHANNEL_BANNER:
                    if (DEBUG) Log.d(TAG, "MSG_UPDTE_CHANNEL_BANNER");
                    mainActivity.mOverlayManager.updateChannelBannerAndShowIfNeeded(
                            TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
                    break;
                default: // fall out
            }
        }

        private long getDelay(long startTime) {
            if (System.currentTimeMillis() - startTime > CHANNEL_CHANGE_NORMAL_SPEED_DURATION_MS) {
                return CHANNEL_CHANGE_DELAY_MS_IN_MAX_SPEED;
            }
            return CHANNEL_CHANGE_DELAY_MS_IN_NORMAL_SPEED;
        }
    }

    private class MyOnTuneListener implements OnTuneListener {
        boolean mUnlockAllowedRatingBeforeShrunken = true;
        boolean mWasUnderShrunkenTvView;
        Channel mChannel;

        private void onTune(Channel channel, boolean wasUnderShrukenTvView) {
            Debug.getTimer(Debug.TAG_START_UP_TIMER).log("MainActivity.MyOnTuneListener.onTune");
            mChannel = channel;
            mWasUnderShrunkenTvView = wasUnderShrukenTvView;
        }

        @Override
        public void onUnexpectedStop(Channel channel) {
            stopTv();
            startTv(null);
        }

        @Override
        public void onTuneFailed(Channel channel) {
            Log.w(TAG, "onTuneFailed(" + channel + ")");
            if (mTvView.isFadedOut()) {
                mTvView.removeFadeEffect();
            }
            Toast.makeText(
                            MainActivity.this,
                            R.string.msg_channel_unavailable_unknown,
                            Toast.LENGTH_SHORT)
                    .show();
        }

        @Override
        public void onStreamInfoChanged(StreamInfo info) {
            if (info.isVideoAvailable() && mTuneDurationTimer.isRunning()) {
                mTracker.sendChannelTuneTime(info.getCurrentChannel(), mTuneDurationTimer.reset());
            }
            if (info.isVideoOrAudioAvailable() && mChannel.equals(getCurrentChannel())) {
                mQuickKeyInfo.canShowResolution(true);
                mOverlayManager.updateChannelBannerAndShowIfNeeded(
                        TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_UPDATE_STREAM_INFO);
            }
            applyDisplayRefreshRate(info.getVideoFrameRate());
            mTvViewUiManager.updateTvAspectRatio();
            Channel currentOne = info.getCurrentChannel();
            if (currentOne != null && !QuickKeyInfo.DTVKIT_PACKAGE.equals(currentOne.getPackageName())) {
                applyMultiAudio();//only applay it when use select a track for dtvkit channel
            }
            if (info.getCurrentChannel() != null && !QuickKeyInfo.DTVKIT_PACKAGE.equals(currentOne.getPackageName())) {
                applyClosedCaption();//only applay it when use select a track for dtvkit channel
            }
            mOverlayManager.getMenu().onStreamInfoChanged();
            if (mTvView.isVideoAvailable()) {
                mTvViewUiManager.fadeInTvView();
            }
            if (!mTvView.isContentBlocked() && !mTvView.isScreenBlocked()) {
                updateAvailabilityToast();
            }
            mHandler.removeCallbacks(mRestoreMainViewRunnable);
            restoreMainTvView();
        }

        @Override
        public void onChannelRetuned(Uri channel) {
            if (channel == null) {
                return;
            }
            Log.d(TAG, "onChannelRetuned " + channel);
            Channel currentChannel =
                    mChannelDataManager.getChannel(ContentUriUtils.safeParseId(channel));
            saveChannelIdForAtvDtvMode(DroidLogicTvUtils.getChannelId(channel));
            if (currentChannel == null) {
                Log.e(
                        TAG,
                        "onChannelRetuned is called but can't find a channel with the URI "
                                + channel);
                mQuickKeyInfo.setReturnedChannelUri(channel);
                return;
            } else {
                mQuickKeyInfo.resetReturnedChannel();
            }
            if (isChannelChangeKeyDownReceived()) {
                // Ignore this message if the user is changing the channel.
                return;
            }
            mChannelTuner.moveToChannel(currentChannel);
            //mChannelTuner.setCurrentChannel(currentChannel);
            mTvView.setCurrentChannel(currentChannel);
            //hide no channel ui when changed
            mTvView.hideNoDataBaseHint();
            mOverlayManager.updateChannelBannerAndShowIfNeeded(
                    TvOverlayManager.UPDATE_CHANNEL_BANNER_REASON_TUNE);
        }

        @Override
        public void onContentBlocked() {
            Debug.getTimer(Debug.TAG_START_UP_TIMER)
                    .log("MainActivity.MyOnTuneListener.onContentBlocked removes timer");
            Debug.removeTimer(Debug.TAG_START_UP_TIMER);
            mTuneDurationTimer.reset();
            TvContentRating rating = mTvView.getBlockedContentRating();
            // When tuneTo was called while TV view was shrunken, if the channel id is the same
            // with the channel watched before shrunken, we allow the rating which was allowed
            // before.
            if (mWasUnderShrunkenTvView
                    && mUnlockAllowedRatingBeforeShrunken
                    && mChannelBeforeShrunkenTvView.equals(mChannel)
                    && rating.equals(mAllowedRatingBeforeShrunken)) {
                mUnlockAllowedRatingBeforeShrunken = isUnderShrunkenTvView();
                mTvView.unblockContent(rating);
            }
            mOverlayManager.setBlockingContentRating(rating);
            mTvViewUiManager.fadeInTvView();
            mMediaSessionWrapper.update(true, getCurrentChannel(), getCurrentProgram());
        }

        @Override
        public void onContentAllowed() {
            if (!isUnderShrunkenTvView()) {
                mUnlockAllowedRatingBeforeShrunken = false;
            }
            mOverlayManager.setBlockingContentRating(null);
            mMediaSessionWrapper.update(false, getCurrentChannel(), getCurrentProgram());
        }
    }
}
